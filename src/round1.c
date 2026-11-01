/* Round 1: Stink Snakes. Throw pies at the snakes. */
#include "aarcade.h"

int cdecl rand(void);

typedef int (FAR PASCAL *R1HOOK)(int n, SPRITE *s, void *pm, int fReady);

/* Same block AllocSpriteMove returns. The two structs are one layout. */
typedef struct tagR1POS {
	int id;
	int x0;
	int x1;
	int y0;
	int y1;
	int unused[8];
	long lPos;
} R1POS;

typedef struct tagR1MOVE {
	unsigned char *pc;
	int pos;
	int posHi;
	int nStep;
	int nSteps;
	long pad[4];
	R1HOOK lpfn;
} R1MOVE;

SPRITE *FAR PASCAL AddRound1Sprite(IMAGE *pimg, IMAGE *pmask, int x, int y, int nMode);
void FAR PASCAL SetRound1Move(SPRITE *s, int id, long lPos);
void FAR PASCAL SetRound1Frame(SPRITE *s, IMAGE *pimg, IMAGE *pmask, int nFrame, int nFrames);
void FAR PASCAL MoveRound1Player(SPRITE *s);
void FAR PASCAL AddRound1Player(void);
void FAR PASCAL ClearRound1Pies(void);
void FAR PASCAL MoveRound1Pie(SPRITE *s);
void FAR PASCAL TickRound1Pie(SPRITE *s);
void FAR PASCAL HitRound1Pie(SPRITE *s, SPRITE *other, BOOL fFirst);
void FAR PASCAL ThrowRound1Pie(void);
void FAR PASCAL SetupRound1(void);
void FAR PASCAL MoveRound1Snake(SPRITE *s);
void FAR PASCAL TickRound1Snake(SPRITE *s);
int FAR PASCAL HookRound1Happy(int n, SPRITE *s, void *pm, int fReady);
void FAR PASCAL HitRound1Snake(SPRITE *s, SPRITE *other, BOOL fFirst);
void FAR PASCAL SpawnRound1Snake(void);
void FAR PASCAL BackRound1(IMAGE *pimg, LPRECT lprc);
void FAR PASCAL ArmRound1(void);

/* Level rows, ten entries each, then the two move scripts. */
// GLOBAL: AARCADE 0024:110E
int g_anR1A[10] = {
	20, 15, 10, 10, 5, 15, 10, 5, 5, 5
};

// GLOBAL: AARCADE 0024:1122
int g_anR1B[10] = {
	40, 40, 40, 45, 45, 45, 50, 50, 50, 55
};

// GLOBAL: AARCADE 0024:1136
int g_anR1C[10] = {
	0, 0, 0, 0, 0, 15, 15, 20, 20, 25
};

// GLOBAL: AARCADE 0024:114A
int g_anR1D[10] = {
	10, 10, 10, 15, 15, 15, 20, 20, 20, 25
};

// GLOBAL: AARCADE 0024:115E
int g_anR1E[10] = {
	5, 5, 5, 10, 10, 10, 10, 10, 15, 25
};

// GLOBAL: AARCADE 0024:1172
int g_anR1F[10] = {
	-10, -10, -10, -10, -15, -15, -15, -20, -20, -20
};

// GLOBAL: AARCADE 0024:1186
char g_r1Cmd[122] = {
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x09, 0x04,
	0x01, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x04, 0x08,
	0x01, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x04, 0x08, 0x63, 0x00, 0x9D, 0xFF, 0x63, 0x00, 0x07, 0x02,
	0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
	0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x01, 0x00, 0x06, 0x02,
	0x04, 0x08, 0x01, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xF9, 0xFF,
	0x00, 0x00, 0x04, 0x08, 0x63, 0x00, 0x9D, 0xFF, 0x9D, 0xFF, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
	0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:1200
char g_r1Cmd2[16] = {
	0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00
};

/* 0024:1210. Nothing takes the address. The bytes sit between g_r1Cmd2
   and round 5. */
// GLOBAL: AARCADE 0024:1210
char g_r1Tail[16] = {
	0x0A, 0x00, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x00, 0x00
};

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:4392
IMAGE g_r1Img[26];


// GLOBAL: AARCADE 0024:4368
SCENE g_r1Scene;


// GLOBAL: AARCADE 0024:4366
SPRITE *g_pR1Player;


// GLOBAL: AARCADE 0024:4364
int g_nR1Cursor;


// GLOBAL: AARCADE 0024:4362
int g_nR1Active;


// GLOBAL: AARCADE 0024:4360
int g_nR1Gone;


// GLOBAL: AARCADE 0024:435E
int g_nR1Mode;


// GLOBAL: AARCADE 0024:435C
int g_nR1Vx;


// GLOBAL: AARCADE 0024:435A
int g_nR1Vy;


// GLOBAL: AARCADE 0024:4358
int g_nR1Span;


// GLOBAL: AARCADE 0024:4356
int g_nR1Max;


// GLOBAL: AARCADE 0024:4354
int g_nR1Speed;


/* time() + 15: the between-rounds dialog closes itself after this. */
/* Round 1 sprites. The scene follows the words; the pictures follow the scene. */
// GLOBAL: AARCADE 0024:4352
int g_nR1Pies;

// FUNCTION: AARCADE 001E:0000
SPRITE *FAR PASCAL AddRound1Sprite(IMAGE *pimg, IMAGE *pmask, int x, int y, int nMode)
{
	SPRITE *s;

	/* test AX, not the copy in SI */
	if ((s = AddSprite(&g_r1Scene)) != 0) {
		s->cur.img = *pimg;
		s->cur.x = x;
		s->cur.y = y;
		UpdateSpriteRect(s);
		s->pImages = pimg;
		if (pmask)
			s->cur.mask = *pmask;
		s->pMasks = pmask;
		s->nDrawMode = nMode;
	}
	return s;
}

// FUNCTION: AARCADE 001E:006B
void FAR PASCAL SetRound1Move(SPRITE *s, int id, long lPos)
{
	R1POS *p;

	/* test AX, not the copy in SI; each zero is its own immediate store */
	if ((p = AllocSpriteMove()) != 0) {
		s->dwData = p;
		s->lpfnMoveEx = MoveSprite;
		p->id = id;
		p->lPos = lPos;
		p->x1 = 0;
		p->x0 = 0;
		p->y0 = 0;
		p->y1 = 0;
	}
}

// FUNCTION: AARCADE 001E:00BB
void FAR PASCAL SetRound1Frame(SPRITE *s, IMAGE *pimg, IMAGE *pmask, int nFrame, int nFrames)
{
	s->pImages = pimg;
	s->pMasks = pmask;
	s->nFrame = nFrame;
	s->nFrames = nFrames;
	s->cur.img = pimg[nFrame];
	UpdateSpriteRect(s);
}

// FUNCTION: AARCADE 001E:0104
void FAR PASCAL MoveRound1Player(SPRITE *s)
{
	if (g_nR1Cursor < 5)
		g_nR1Cursor = 5;
	else if (0x141 - g_r1Img[0].rc.bottom < g_nR1Cursor)
		g_nR1Cursor = 0x141 - g_r1Img[0].rc.bottom;
	s->dy = g_nR1Cursor - s->cur.rc.top;
	/* false side of the && joins the else, so the clamp's jle lands there */
	if (s->cur.rc.top < g_nR1Cursor && s->dy > 0x14)
		s->dy = 0x14;
	else if (s->cur.rc.top > g_nR1Cursor) {
		if (s->dy < -0x14)
			s->dy = -0x14;
	}
}

// FUNCTION: AARCADE 001E:016B
void FAR PASCAL AddRound1Player(void)
{
	SPRITE *s;

	/* global is stored from AX, then the copy in SI is tested */
	s = g_pR1Player = AddRound1Sprite(&g_r1Img[0], &g_r1Img[13], 5, 0xA3, g_nR1Mode);
	if (s != 0) {
		s->nGroup = 1;
		s->dwHitMask = 0xE0000000L;
		s->lpfnMove = MoveRound1Player;
	}
}

// FUNCTION: AARCADE 001E:01A8
void FAR PASCAL ClearRound1Pies(void)
{
	g_nR1Pies = 0;
}

// FUNCTION: AARCADE 001E:01B5
void FAR PASCAL MoveRound1Pie(SPRITE *s)
{
	if (s->cur.rc.left >= 0x200)
		s->dx = -s->dx;
	else if (s->cur.rc.right <= 0) {
		s->cur.fVisible = 0;
		s->fDead = 1;
	}
}

// FUNCTION: AARCADE 001E:01E2
void FAR PASCAL TickRound1Pie(SPRITE *s)
{
	if (s->fDead != 0)
		g_nR1Pies--;
}

// FUNCTION: AARCADE 001E:01FA
void FAR PASCAL HitRound1Pie(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	s->fDead = 1;
	s->cur.fVisible = 0;
	if (other == g_pR1Player) {
		AddPlayAmmo(1L);
		PlaySfx(0x1773, 0);
	}
}

// FUNCTION: AARCADE 001E:022C
void FAR PASCAL ThrowRound1Pie(void)
{
	SPRITE *s;

	if (g_nR1Pies >= 5)
		return;
	/* the two == 0 tests are je straight to the epilogue, not an early return */
	if (g_nPlayAmmo != 0 && g_nR1Active != 0) {
		s = AddRound1Sprite(&g_r1Img[1], &g_r1Img[14],
			g_pR1Player->cur.rc.right + 1, g_pR1Player->cur.rc.top + 0xC, g_nR1Mode);
		if (s != 0) {
			s->nGroup = 2;
			s->dx = 0x14;
			s->dwHitMask = 0x40000000L;
			s->lpfnMoved = MoveRound1Pie;
			s->lpfnHit = HitRound1Pie;
			s->lpfnTick = TickRound1Pie;
			g_nR1Pies++;
			AddPlayAmmo(-1L);
			PlaySfx(0x1771, 0);
		}
	}
}

// FUNCTION: AARCADE 001E:02C1
void FAR PASCAL SetupRound1(void)
{
	char FAR *lp;
	int n;

	g_nR1Active = 0;
	g_nR1Gone = 0;
	n = g_nPlayWave;
	if (n >= 10)
		n = (n & 3) + 6;
	g_nR1Vx = g_anR1A[n];
	g_nR1Vy = g_anR1B[n];
	g_nR1Span = g_anR1C[n];
	g_nR1Left = g_anR1D[n];
	g_nR1Max = g_anR1E[n];
	g_nR1Speed = g_anR1F[n];
	/* n is dead, so n = 8 is emitted before the pointer is built */
	n = 8;
	lp = g_r1Cmd;
	while (n--)
		lp += lp[1];
	*(int FAR *)(lp + 2) = g_nR1Vx;
	*(int FAR *)(lp + 4) = -g_nR1Vy;
	*(int FAR *)(lp + 6) = g_nR1Span;
	n = 14;
	while (n--)
		lp += lp[1];
	*(int FAR *)(lp + 2) = g_nR1Vx;
	*(int FAR *)(lp + 4) = -g_nR1Vy;
	*(int FAR *)(lp + 6) = -g_nR1Span;
}

// FUNCTION: AARCADE 001E:03A4
void FAR PASCAL MoveRound1Snake(SPRITE *s)
{
	if (s->cur.rc.right <= 0) {
		s->cur.fVisible = 0;
		s->fDead = 1;
		if (s->nGroup == 3) {
			AddPlayAmmo(-5L);
			PlaySfx(0x1770, 1);
		}
	}
}

// FUNCTION: AARCADE 001E:03D8
void FAR PASCAL TickRound1Snake(SPRITE *s)
{
	if (s->fDead != 0) {
		g_nR1Active--;
		if (s->nGroup != 3)
			g_nR1Gone--;
	}
}

// FUNCTION: AARCADE 001E:03F9
int FAR PASCAL HookRound1Happy(int n, SPRITE *s, void *pm, int fReady)
{
	s->lpfnMoveEx = 0;
	s->nGroup = 5;
	s->dx = g_nR1Speed;
	s->nAnimRate = 2;
	s->nFrame = 0;
	s->pImages = &g_r1Img[9];
	s->cur.img = g_r1Img[9];
	s->pMasks = &g_r1Img[22];
	s->cur.mask = g_r1Img[22];
	UpdateSpriteRect(s);
	s->fDirty = 1;
	return 0;
}

// FUNCTION: AARCADE 001E:0462
void FAR PASCAL HitRound1Snake(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	R1MOVE *pm;

	if (other->nGroup == 1) {
		if (s->nGroup == 5) {
			s->cur.fVisible = 0;
			s->fDead = 1;
			AddPlayScore(10L);
			PlaySfx(0x1774, 0);
		} else if (other->nGroup == 1) {
			/* empty: reloads other and jne +0 onto the exit jmp */
		}
		return;
	}
	if (other->nGroup != 2)
		return;
	AddPlayScore(1L);
	g_nR1Gone++;
	g_nR1Left--;
	pm = (R1MOVE *)s->dwData;
	pm->pc = (unsigned char *)g_r1Cmd2;
	pm->posHi = 0;
	pm->pos = 0;
	pm->nStep = 0;
	pm->nSteps = 0;
	pm->lpfn = HookRound1Happy;
	s->dwHitMask = 0x20000000L;
	s->nGroup = 4;
	s->nFrame = 0;
	s->pImages = &g_r1Img[6];
	s->cur.img = g_r1Img[6];
	s->pMasks = &g_r1Img[19];
	s->cur.mask = g_r1Img[19];
	UpdateSpriteRect(s);
	s->fDirty = 1;
	PlaySfx(0x1772, 0);
}

// FUNCTION: AARCADE 001E:053E
void FAR PASCAL SpawnRound1Snake(void)
{
	int nY;
	int nHalf;
	int nTry;
	int fBlocked;
	SPRITE *s;

	if (g_nR1Active != 0) {
		if ((rand() & 0x7FFF) % 16 != 0)
			return;
	}
	if (g_nR1Active >= g_nR1Max)
		return;
	if (g_nR1Left <= g_nR1Active)
		return;
	if (g_nPlayAmmo == 0)
		return;
	fBlocked = 1;
	nHalf = g_r1Img[2].rc.bottom;
	nTry = 0x15;
	while (fBlocked != 0 && nTry-- != 0) {
		fBlocked = 0;
		nY = (rand() & 0x7FFF) % (0x13C - nHalf - g_nR1Span) + 5;
		for (s = g_r1Scene.pFirst; s != 0; s = s->pNext) {
			if (s->nGroup == 3
					&& s->cur.rc.right >= 0x200 - g_r1Img[2].rc.right + -11
					&& s->cur.rc.top - nHalf < nY
					&& s->cur.rc.bottom + nHalf > nY) {
				fBlocked = 1;
				break;
			}
		}
	}
	if (fBlocked == 0) {
		s = AddRound1Sprite(&g_r1Img[2], &g_r1Img[15], 0x1F6, nY, g_nR1Mode);
		if (s != 0) {
			SetRound1Move(s, (int)g_r1Cmd, 0L);
			SetRound1Frame(s, &g_r1Img[2], &g_r1Img[15], 0, 4);
			s->nAnimMode = 1;
			s->nGroup = 3;
			s->dwHitMask = 0xC0000000L;
			s->lpfnMoved = MoveRound1Snake;
			s->lpfnHit = HitRound1Snake;
			s->lpfnTick = TickRound1Snake;
			g_nR1Active++;
		}
	}
}

// FUNCTION: AARCADE 001E:0673
void FAR PASCAL BackRound1(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[0]);
}

// FUNCTION: AARCADE 001E:068B
void FAR PASCAL ArmRound1(void)
{
	SetTimeLeft((long)(g_nPlayWave * 5 + 0x1E));
}

// FUNCTION: AARCADE 001E:06A3
void FAR PASCAL OpenRound1(HWND hwnd)
{
	if (g_fColor) {
		LoadPPicImages(0x1770, 0, 0x1A, g_r1Img, g_nDepth);
		InitScene(hwnd, &g_r1Scene, BackRound1, &g_rcPlay, g_nDepth2, 2, 0);
		g_nR1Mode = -1;
	} else {
		LoadPPicImages(0x1770, 0, 0x0E, g_r1Img, g_nDepth);
		InitScene(hwnd, &g_r1Scene, 0, &g_rcPlay, g_nDepth2, 2, 0);
		g_nR1Mode = 0;
	}
	ArmRound1();
	AddRound1Player();
	ClearRound1Pies();
	SetupRound1();
	PreloadSounds(0x1770, 5);
}

// FUNCTION: AARCADE 001E:073A
void FAR PASCAL ClickRound1(void)
{
	ThrowRound1Pie();
}

// FUNCTION: AARCADE 001E:0745
int FAR PASCAL TickRound1(void)
{
	SpawnRound1Snake();
	/* > 0 on any of these is jg past the reset; a return here would be inline */
	if (g_nR1Pies <= 0 && g_nR1Left <= 0 && g_nR1Active <= 0 && g_nR1Gone <= 0) {
		RunStepDialog();
		NextPlayWave();
		ArmRound1();
		SetupRound1();
	}
	g_nR1Cursor = HIWORD(GetMessagePos());
	if (g_r1Scene.pFirst != 0)
		TickScene(&g_r1Scene);
	/* one else, so the three jg's and the je share the xor ax,ax */
	if (g_nPlayAmmo <= 0 && g_nR1Pies <= 0 && g_nR1Gone <= 0
			&& (g_nR1Left != 0 || g_nR1Active != 0))
		return 1;
	else
		return 0;
}

// FUNCTION: AARCADE 001E:07C4
void FAR PASCAL PaintRound1(void)
{
	PaintScene(&g_r1Scene);
}

// FUNCTION: AARCADE 001E:07D3
void FAR PASCAL CloseRound1(void)
{
	FreeScene(&g_r1Scene);
	FreeImages(g_r1Img, 0x1A, 1);
}
