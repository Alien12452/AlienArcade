/* Round 6: Quasar Rain. Bounce the falling klutzes into bed. */
#include <mem.h>
#include <stdlib.h>
#include <windows.h>
#include "aarcade.h"

typedef int (FAR PASCAL *R6HOOK)(int n, SPRITE *s, void *pm, int fReady);

/* AllocSpriteMove's block, laid out like the other rounds: hook at +0x1a. */
typedef struct tagR6POS {
	int id;
	int x0;
	int x1;
	int y0;
	int y1;
	int unused[8];
	R6HOOK lpfn;
	int FAR *pMark;
} R6POS;

typedef struct tagR6SLOT {
	SPRITE *s;
	char *p;
} R6SLOT;

// GLOBAL: AARCADE 0024:1920
int g_anR6Count[10] = { 16, 16, 16, 16, 18, 20, 22, 24, 26, 28 };

// GLOBAL: AARCADE 0024:1934
int g_anR6Mod[10] = { 14, 12, 11, 10, 12, 12, 12, 12, 12, 10 };

// GLOBAL: AARCADE 0024:1948
int g_anR6Spread[10] = { 40, 40, 40, 50, 50, 50, 50, 60, 60, 60 };

// GLOBAL: AARCADE 0024:195C
int g_anR6Range[10] = { 200, 200, 200, 100, 200, 300, 400, 500, 500, 500 };

// GLOBAL: AARCADE 0024:1970
int g_anR6Dy[10] = { 8, 10, 13, 16, 20, 24, 24, 26, 28, 30 };

// GLOBAL: AARCADE 0024:1984
int g_anR6Delay[10] = { 60, 56, 52, 30, 28, 26, 24, 22, 20, 18 };

// GLOBAL: AARCADE 0024:1998
char g_r6CmdIdle[] = { 0x00, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0A, 0x00 };

// GLOBAL: AARCADE 0024:19A0
int g_anR6Mark[16] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

/* Copied into a slot with a count of 0x80, so the copy reads 46 bytes past
   this array: g_fStart, the idle script, the mark pool, and the first four
   bytes of g_cmdStartGap. */
// GLOBAL: AARCADE 0024:19C0
char g_r6Cmd[82] = {
	0x00, 0x02, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00,
	0x63, 0x00, 0x00, 0x00, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x06, 0x02, 0x04, 0x08,
	0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x06, 0x02, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00,
	0x04, 0x08, 0x04, 0x00, 0xEC, 0xFF, 0x18, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x51, 0x46, 0x09, 0x04,
	0x04, 0x00, 0x04, 0x08, 0x0C, 0x00, 0xC4, 0xFF, 0x00, 0x00, 0x01, 0x02, 0x0C, 0x04, 0xFD, 0xFF,
	0x0E, 0x02
};

/* Not in aarcade.h, so this order is the one Borland reverses: highest address first. */
// GLOBAL: AARCADE 0024:4EDE
IMAGE g_r6Img[26];

// GLOBAL: AARCADE 0024:4EB4
SCENE g_r6Scene;

// GLOBAL: AARCADE 0024:4EB2
SPRITE *g_pR6MomETron;

// GLOBAL: AARCADE 0024:4EB0
int g_nR6Saved;

// GLOBAL: AARCADE 0024:4EAE
int g_nR6Cursor;

// GLOBAL: AARCADE 0024:4EAA
DWORD g_dwR6Next;

// GLOBAL: AARCADE 0024:4EA8
int g_nR6Level;

// GLOBAL: AARCADE 0024:4EA6
int g_nR6Mod;

// GLOBAL: AARCADE 0024:4EA4
int g_nR6Spread;

// GLOBAL: AARCADE 0024:4EA2
int g_nR6Range;

// GLOBAL: AARCADE 0024:4EA0
int g_nR6Dy;

// GLOBAL: AARCADE 0024:4E9E
int g_nR6Delay;

// GLOBAL: AARCADE 0024:4E9C
int g_nR6Left;

// GLOBAL: AARCADE 0024:4E9A
int g_nR6Active;

// GLOBAL: AARCADE 0024:4E98
int g_nR6Aim;

// GLOBAL: AARCADE 0024:4E96
int g_fR6Miss;

// GLOBAL: AARCADE 0024:4E94
char *g_hR6Scripts;

// GLOBAL: AARCADE 0024:4E54
R6SLOT g_aR6Slot[16];

// GLOBAL: AARCADE 0024:4E50
DWORD g_dwR6Spawn;


void FAR PASCAL FreeRound6Slot(SPRITE *s);

// FUNCTION: AARCADE 0021:0000
SPRITE *FAR PASCAL AddRound6Sprite(IMAGE *pimg, IMAGE *pmask, int x, int y)
{
	SPRITE *s;

	if ((s = AddSprite(&g_r6Scene)) != 0) {
		s->cur.img = *pimg;
		if (pmask != 0)
			s->cur.mask = *pmask;
		s->cur.x = x;
		s->cur.y = y;
		UpdateSpriteRect(s);
		s->pImages = pimg;
		s->pMasks = pmask;
		if (pmask != 0)
			s->nDrawMode = -1;
		else
			s->nDrawMode = 0;
	}
	return s;
}

// FUNCTION: AARCADE 0021:0075
SPRITE *FAR PASCAL InsertRound6Sprite(SPRITE *before, IMAGE *pimg, IMAGE *pmask, int x, int y)
{
	SPRITE *s;

	if ((s = InsertSprite(&g_r6Scene, before)) != 0) {
		s->cur.img = *pimg;
		if (pmask != 0)
			s->cur.mask = *pmask;
		s->cur.x = x;
		s->cur.y = y;
		UpdateSpriteRect(s);
		s->pImages = pimg;
		s->pMasks = pmask;
		if (pmask != 0)
			s->nDrawMode = -1;
		else
			s->nDrawMode = 0;
	}
	return s;
}

// FUNCTION: AARCADE 0021:00ED
void FAR PASCAL SetRound6Move(SPRITE *s, int id, R6HOOK lpfn, int FAR *pMark)
{
	R6POS *p;

	if (s->dwData == 0)
		p = AllocSpriteMove();
	else
		p = (R6POS *)s->dwData;
	if (p != 0) {
		s->dwData = p;
		s->lpfnMoveEx = MoveSprite;
		p->id = id;
		p->lpfn = lpfn;
		p->pMark = pMark;
		p->x1 = 0;
		p->x0 = 0;
		p->y0 = 0;
		p->y1 = 0;
	}
}

// FUNCTION: AARCADE 0021:0156
void FAR PASCAL SetRound6Frame(SPRITE *s, IMAGE *pimg, int nFrame, int nFrames, IMAGE *pmask)
{
	s->pImages = pimg;
	s->pMasks = pmask;
	s->nFrame = nFrame;
	s->nFrames = nFrames;
	s->cur.img = pimg[nFrame];
	if (pmask != 0)
		s->cur.mask = pmask[nFrame];
	UpdateSpriteRect(s);
}

// FUNCTION: AARCADE 0021:01BF
void FAR PASCAL ClearRound6Marks(void)
{
	int i;

	for (i = 0; i < 16; i++)
		g_anR6Mark[i] = -1;
}

// FUNCTION: AARCADE 0021:01DA
int FAR *FAR PASCAL AllocRound6Mark(void)
{
	int *p;

	for (p = g_anR6Mark; *p >= 0; p++)
		;
	*p = 0;
	return (int FAR *)p;
}

// FUNCTION: AARCADE 0021:01FA
int FAR PASCAL HookRound6(int n, SPRITE *s, void *pm, int fReady)
{
	R6POS *p;

	p = pm;
	if (n < 0) {
		switch (n) {
		case -3:
			FreeRound6Slot(s);
			*(int *)p->pMark = -1;
			g_nR6Active--;
			break;
		case -2:
			*(int *)p->pMark = -1;
			break;
		case -1:
			SetRound6Move(s, (int)g_r6CmdIdle, 0, 0);
			break;
		}
		return 1;
	}
	if (n > 0) {
		p->y1 = 2;
		if (*(int *)p->pMark >= n) {
			p->y0 = 2;
			return 1;
		} else {
			p->y0 = 1;
			return 0;
		}
		/* both returns already left; the goto keeps the dead jump */
		goto done;
	}
	(*(int *)p->pMark)++;
	return 1;
done:;
}

// FUNCTION: AARCADE 0021:0286
void FAR PASCAL HitTestRound6MomE(SPRITE *s, SPRITE *other, BOOL fFirst, int *pfHit)
{
	int right;
	int bottom;

	if (fFirst != 0) {
		right = other->cur.rc.right;
		bottom = other->cur.rc.bottom;
		*pfHit = 1;
		if (s->cur.rc.left < right && s->cur.rc.right > right &&
		    s->cur.rc.top < bottom &&
		    s->cur.rc.top + s->cur.rc.right - right > bottom)
			*pfHit = 0;
	} else
		*pfHit = 0;
}

// FUNCTION: AARCADE 0021:02DF
void FAR PASCAL MoveRound6MomETron(SPRITE *s)
{
	s->dx = (g_nR6Cursor - g_nR6Saved) << 2;
	g_nR6Saved = g_nR6Cursor;
	if (s->dx + s->cur.rc.right > 0x200)
		s->dx = 0x200 - s->cur.rc.right;
	else if (s->dx + s->cur.rc.left < 0x43)
		s->dx = 0x43 - s->cur.rc.left;
}

// FUNCTION: AARCADE 0021:032A
void FAR PASCAL AddRound6MomETron(void)
{
	SPRITE *s;

	g_pR6MomETron = 0;
	s = AddRound6Sprite(&g_r6Img[24], &g_r6Img[25],
		(0x200 - g_r6Img[24].rc.right) / 2,
		0x146 - g_r6Img[24].rc.bottom);
	g_pR6MomETron = s;
	if (s != 0) {
		s->nGroup = 1;
		s->dwHitMask = 0xE000L;
		s->nMoveRate = 1;
		s->lpfnMove = MoveRound6MomETron;
		s->lpfnHitTest = HitTestRound6MomE;
	}
}

// FUNCTION: AARCADE 0021:038B
void FAR PASCAL InitRound6Slots(void)
{
	char *p;
	int i;

	g_hR6Scripts = (char *)LocalAlloc(LPTR, 0x800);
	for (i = 0; i < 16; i++) {
		g_aR6Slot[i].s = 0;
		p = g_hR6Scripts + (i << 7);
		g_aR6Slot[i].p = p;
	}
}

// FUNCTION: AARCADE 0021:03D3
R6SLOT *FAR PASCAL FindRound6Slot(void)
{
	int i;

	for (i = 0; i < 16; i++) {
		if (g_aR6Slot[i].s == 0)
			return &g_aR6Slot[i];
	}
	return 0;
}

// FUNCTION: AARCADE 0021:03FE
void FAR PASCAL FreeRound6Slot(SPRITE *s)
{
	int i;

	for (i = 0; i < 16; i++) {
		if (g_aR6Slot[i].s == s) {
			g_aR6Slot[i].s = 0;
			break;
		}
	}
}

// FUNCTION: AARCADE 0021:042C
void FAR PASCAL FreeRound6Scripts(void)
{
	if (g_hR6Scripts != 0)
		LocalFree((HLOCAL)g_hR6Scripts);
}

// FUNCTION: AARCADE 0021:0443
void FAR PASCAL BounceRound6Klutz(SPRITE *s)
{
	R6SLOT *pSlot;
	int nWide;
	int x;
	int row;
	int y;
	int col;

	nWide = s->nGroup == 2 ? 0x0E : 0x19;
	x = s->cur.x - nWide + -36;
	row = 3 - (s->cur.x + -67) / 0x6F;
	y = s->cur.y - (row * 0x48 + -3);
	col = x >> 6;
	PlaySfx(0x4650, FALSE);
	if (s->nGroup == 2)
		y -= 0x0E;
	else
		y -= 0x16;
	pSlot = FindRound6Slot();
	s->dwHitMask = 0;
	if (s->nGroup == 2)
		SetRound6Frame(s, &g_r6Img[0], 0, 10, &g_r6Img[10]);
	else
		SetRound6Frame(s, &g_r6Img[5], 0, 10, &g_r6Img[15]);
	_fmemcpy(pSlot->p, g_r6Cmd, 0x80);
	pSlot->s = s;
	_SI = *(int *)((char *)pSlot + 2);
	_SI += *(char *)(_SI + 1);
	*(int *)(_SI + 2) = col;
	*(int *)(_SI + 4) = -(x >> 2);
	*(int *)(_SI + 6) = -(y >> 1);
	x -= x >> 2;
	y -= y >> 1;
	_SI += *(char *)(_SI + 1);
	_SI += *(char *)(_SI + 1);
	*(int *)(_SI + 4) = -nWide;
	_SI += *(char *)(_SI + 1);
	*(int *)(_SI + 2) = col;
	*(int *)(_SI + 4) = -(x / 3);
	*(int *)(_SI + 6) = -(y >> 1);
	x -= x / 3;
	/* reloaded into DX; y -= y >> 1 is a sub of the memory operand */
	y = y - (y >> 1);
	_SI += *(char *)(_SI + 1);
	_SI += *(char *)(_SI + 1);
	*(int *)(_SI + 2) = col;
	*(int *)(_SI + 4) = -(x >> 1);
	*(int *)(_SI + 6) = -(y >> 1);
	x -= x >> 1;
	y -= y >> 1;
	_SI += *(char *)(_SI + 1);
	_SI += *(char *)(_SI + 1);
	*(int *)(_SI + 2) = col;
	*(int *)(_SI + 4) = -x;
	*(int *)(_SI + 6) = -y;
	SetRound6Move(s, (int)pSlot->p, HookRound6, AllocRound6Mark());
}

// FUNCTION: AARCADE 0021:05FE
void FAR PASCAL MovedRound6(SPRITE *s)
{
	if (s->cur.y > 0x146) {
		s->fDead = 1;
		if (s->nGroup != 4) {
			g_nR6Active--;
			g_fR6Miss = 1;
		}
	}
}

// FUNCTION: AARCADE 0021:0626
void FAR PASCAL HitRound6Klutz(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	BounceRound6Klutz(s);
	AddPlayScore((long)(g_nR6Level + g_nR6Level + 0x14));
}

// FUNCTION: AARCADE 0021:0648
void FAR PASCAL HitRound6Peel(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	s->cur.fVisible = 0;
	PlaySfx(0x4652, TRUE);
	AddPlayScore((long)(-15 - g_nR6Level - g_nR6Level));
}

// FUNCTION: AARCADE 0021:0677
void FAR PASCAL SpawnRound6(void)
{
	SPRITE *s;

	/* body is longer than 127 bytes, so the < path is jae/jmp over it */
	if (g_nR6Left == 0)
		goto done;
	if (GetTickCount() / 17 >= g_dwR6Next) {
		if (g_nR6Active < 16 && (unsigned)rand() % (unsigned)g_nR6Mod != 0) {
			int side;
			int y;
			int x0;
			int x1;
			int x;
			int dx;
			int n;

			side = rand() & 1;
			n = side * 5;
			y = 0 - g_r6Img[n].rc.bottom;
			if (g_nR6Aim < 0) {
				x0 = 0x43;
				x1 = 0x200 - g_r6Img[n].rc.right;
			} else {
				x0 = g_nR6Aim - g_nR6Range;
				if (x0 < 0x43)
					x0 = 0x43;
				x1 = g_nR6Aim + g_nR6Range;
				if (x1 > 0x200 - g_r6Img[n].rc.right)
					x1 = 0x200 - g_r6Img[n].rc.right;
			}
			do {
				x = x0 + (unsigned)rand() % (unsigned)(x1 - x0);
				dx = x - g_nR6Aim;
				if (dx < 0)
					dx = -dx;
				/* jl straight to the loop-back jmp; continue would be jge/jmp */
				if (dx >= g_nR6Spread) {
					g_nR6Aim = x;
					break;
				}
			} while (1);
			s = InsertRound6Sprite(g_pR6MomETron->pNext->pNext, &g_r6Img[n],
				&g_r6Img[n + 10], g_nR6Aim, y);
			if (s != 0) {
				s->nGroup = side + 2;
				s->dwHitMask = side != 0 ? 0x8000U : 0x4000U;
				s->dy = g_nR6Dy;
				s->lpfnMoved = MovedRound6;
				s->lpfnHit = HitRound6Klutz;
			}
			g_nR6Active++;
			g_nR6Left--;
		} else {
			int x;
			int y;

			x = 0x43 + (unsigned)rand() % (unsigned)(0x1BD - g_r6Img[20].rc.right);
			y = 0 - g_r6Img[20].rc.bottom;
			s = InsertRound6Sprite(g_pR6MomETron->pNext->pNext, &g_r6Img[20],
				&g_r6Img[21], x, y);
			if (s != 0) {
				s->nGroup = 4;
				s->dwHitMask = 0x2000L;
				s->dy = 0x0A;
				s->lpfnMoved = MovedRound6;
				s->lpfnHit = HitRound6Peel;
			}
		}
		g_dwR6Next = GetTickCount() / 17 + g_nR6Delay;
	}
done:;
}

// FUNCTION: AARCADE 0021:0891
void FAR PASCAL LayRound6(void)
{
	int i;

	AddRound6Sprite(&g_r6Img[22], 0, 0, 0);
	for (i = 0; i < 5; i++)
		AddRound6Sprite(&g_r6Img[23], 0, 10, i * 0x48 + -3);
}

// FUNCTION: AARCADE 0021:08C5
void FAR PASCAL SetupRound6(void)
{
	int n;

	n = g_nR6Level;
	if (n >= 10)
		n = (n & 3) + 6;
	g_nR6Aim = -1;
	g_nR6Left += g_anR6Count[n];
	g_nR6Mod = g_anR6Mod[n];
	g_nR6Spread = g_anR6Spread[n];
	g_nR6Range = g_anR6Range[n];
	g_nR6Dy = g_anR6Dy[n];
	g_nR6Delay = g_anR6Delay[n];
	if (g_nR6Level >= 10) {
		n = g_nR6Level + -9;
		g_nR6Spread += n * 5;
		if (g_nR6Spread > 0x5A)
			g_nR6Spread = 0x5A;
		g_nR6Dy += n;
		if (g_nR6Dy > 0x26)
			g_nR6Dy = 0x26;
	}
}

// FUNCTION: AARCADE 0021:095E
void FAR PASCAL ArmRound6(void)
{
	SetTimeLeft(0x1EL);
}

// FUNCTION: AARCADE 0021:096E
void FAR PASCAL ShrinkRound6Bottom(IMAGE *pimg)
{
	int n;

	n = pimg->rc.bottom;
	n--;
	pimg->rc.bottom = n;
}

// FUNCTION: AARCADE 0021:0983
void FAR PASCAL BackRound6(IMAGE *pimg, int n, RECT *prc)
{
	FillImage(pimg, prc, g_hGdiObjects[1]);
}

// FUNCTION: AARCADE 0021:099B
void FAR PASCAL OpenRound6(HWND hwnd)
{
	LoadPPicImages(0x4650, 0, 0x1A, g_r6Img, g_nDepth);
	ShrinkRound6Bottom(&g_r6Img[4]);
	ShrinkRound6Bottom(&g_r6Img[9]);
	ShrinkRound6Bottom(&g_r6Img[14]);
	ShrinkRound6Bottom(&g_r6Img[19]);
	if (g_fColor != 0)
		InitScene(hwnd, &g_r6Scene, (SCENEBKPROC)BackRound6, &g_rcPlay, 3, g_nDepth2, 0);
	else
		InitScene(hwnd, &g_r6Scene, 0, &g_rcPlay, 1, 3, 0);
	ArmRound6();
	ClearRound6Marks();
	g_nR6Active = 0;
	g_nR6Left = 0;
	g_nR6Level = 0;
	SetupRound6();
	InitRound6Slots();
	AddRound6MomETron();
	LayRound6();
	g_dwR6Spawn = 0;
	g_dwR6Next = 0;
	g_fR6Miss = 0;
	PreloadSounds(0x4650, 4);
	g_nPlayAmmo = 3;
	PreloadSounds(0x1770, 1);
}

// FUNCTION: AARCADE 0021:0A6E
void FAR PASCAL ClickRound6(void)
{
}

// FUNCTION: AARCADE 0021:0A75
int FAR PASCAL TickRound6(void)
{
	if (g_nR6Active == 0 && g_nR6Left == 0) {
		RunStepDialog();
		NextPlayWave();
		g_nR6Level++;
		ArmRound6();
		g_nR6Active = 0;
		g_nR6Left = 0;
		SetupRound6();
		SpawnRound6();
		g_nR6Saved = (int)GetMessagePos();
	}
	g_nR6Cursor = (int)GetMessagePos();
	if (GetTickCount() / 17 >= g_dwR6Spawn) {
		SpawnRound6();
		g_dwR6Spawn = GetTickCount() / 17 + 3;
		TickScene(&g_r6Scene);
		if (g_fR6Miss != 0) {
			if (g_nPlayAmmo != 0) {
				AddPlayAmmo(-1L);
				if (g_nR6Level != 0) {
					g_nR6Level--;
					SetupRound6();
					g_nR6Left = g_nR6Left / 2;
				}
				g_fR6Miss = 0;
				PlaySfx(0x1770, TRUE);
			} else
				return 1;
		}
	}
	return 0;
}

// FUNCTION: AARCADE 0021:0B53
void FAR PASCAL PaintRound6(void)
{
	PaintScene(&g_r6Scene);
}

// FUNCTION: AARCADE 0021:0B62
void FAR PASCAL CloseRound6(void)
{
	FreeRound6Scripts();
	FreeScene(&g_r6Scene);
	FreeImages(g_r6Img, 0x1A, 1);
}
