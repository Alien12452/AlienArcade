/* Round 4: Fungus Patrol. Scrape fungus off the mothership. */
#include <mem.h>
#include <stdlib.h>
#include "aarcade.h"

typedef int (FAR PASCAL *R4HOOK)(int n, SPRITE *s, void *pm, int fReady);

/* AllocSpriteMove's block. The script hook is at +0x1a; the mark follows it. */
typedef struct tagR4POS {
	int id;
	int x0;
	int x1;
	int y0;
	int y1;
	int unused[8];
	R4HOOK lpfn;
	int FAR *pMark;
} R4POS;

/* Which axis dominates, and the signed step along it. */
typedef struct tagR4STEP {
	int dx;
	int dy;
	int adx;
	int ady;
	int nMajor;
	int nAbs;
	int nFlag;
} R4STEP;

void FAR PASCAL SetupRound4Step(R4STEP *p, int dx, int dy);
SPRITE *FAR PASCAL AddRound4Sprite(IMAGE *pimg, IMAGE *pmask, int x, int y);
SPRITE *FAR PASCAL InsertRound4Sprite(SPRITE *before, IMAGE *pimg, IMAGE *pmask, int x, int y);
void FAR PASCAL SetRound4Move(SPRITE *s, int id, R4HOOK lpfn, int FAR *pMark);
void FAR PASCAL SetRound4Frame(SPRITE *s, IMAGE *pimg, int nFrame, int nFrames, IMAGE *pmask);
void FAR PASCAL ClearRound4Marks(void);
int FAR *FAR PASCAL AllocRound4Mark(void);
int FAR PASCAL MoveRound4Script(int n, SPRITE *s, void *pm, int fReady);
void FAR PASCAL MoveRound4Ball(SPRITE *s);
void FAR PASCAL HitTestRound4Ball(SPRITE *s, SPRITE *other, BOOL fFirst, int *pfHit);
void FAR PASCAL MovedRound4Ball(SPRITE *s);
void FAR PASCAL BurstRound4Ball(void);
void FAR PASCAL AddRound4Ball(void);
void FAR PASCAL MoveRound4Bungee(SPRITE *s, BOOL fFirst);
void FAR PASCAL DrawRound4Bungee(HDC hdc, SPRITE *s, IMAGE *pimg, BOOL fCur);
void FAR PASCAL AddRound4Bungee(void);
void FAR PASCAL LayRound4(void);
void FAR PASCAL AllocRound4Pool(void);
R4SLOT *FAR PASCAL FindRound4Slot(void);
void FAR PASCAL FreeRound4Slot(SPRITE *s);
void FAR PASCAL FreeRound4Pool(void);
void FAR PASCAL MovedRound4Fungus(SPRITE *s);
void FAR PASCAL TickRound4Fungus(SPRITE *s);
SPRITE *FAR PASCAL SpawnRound4Fungus(void);
void FAR PASCAL SpawnRound4Wave(void);
void FAR PASCAL SetupRound4(void);
void FAR PASCAL ArmRound4(void);
void FAR PASCAL EmptyRound4Back(IMAGE *pimg, int n, RECT *prc);
void FAR PASCAL BackRound4(IMAGE *pimg, int n, RECT *prc);
int FAR PASCAL OverlapRound4(int nFrame, SPRITE *s, SPRITE *other);
HRGN FAR PASCAL FungusRgnRound4(int n);
HRGN FAR PASCAL MakeFungusRgn(SPRITE *s);
HRGN FAR PASCAL MakeBallRgn(SPRITE *s);
HRGN FAR PASCAL BallRgnRound4(int nFrame, RECT *prc);

/* Polygon outlines (point count, then x, y pairs), level rows, launch
   speeds, and the move script. */
// GLOBAL: AARCADE 0024:161C
int g_anR4Ball0[] = {
	8, 20, 3, 52, 3, 52, 8, 45, 15,
	45, 20, 26, 20, 26, 15, 20, 8
};

// GLOBAL: AARCADE 0024:163E
int g_anR4Ball1[] = {
	8, 25, 1, 58, 6, 57, 10, 50, 16,
	48, 23, 29, 18, 30, 13, 25, 6
};

// GLOBAL: AARCADE 0024:1660
int g_anR4Ball2[] = {
	5, 33, 0, 61, 12, 49, 22, 34, 16,
	31, 2
};

// GLOBAL: AARCADE 0024:1676
int g_anR4Ball3[] = {
	8, 38, 0, 66, 16, 66, 19, 56, 24,
	52, 27, 38, 16, 38, 14, 38, 4
};

// GLOBAL: AARCADE 0024:1698
int g_anR4Ball4[] = {
	6, 49, 2, 70, 25, 63, 29, 54, 30,
	44, 17, 44, 9
};

// GLOBAL: AARCADE 0024:16B2
int g_anR4Ball5[] = {
	5, 54, 4, 71, 33, 62, 34, 57, 33,
	45, 19
};

// GLOBAL: AARCADE 0024:16C8
int g_anR4Ball6[] = {
	6, 61, 10, 72, 41, 67, 40, 54, 37,
	48, 21, 57, 10
};

// GLOBAL: AARCADE 0024:16E2
int g_anR4Ball7[] = {
	6, 66, 13, 71, 46, 65, 46, 54, 41,
	51, 24, 60, 15
};

/* Ninth outline. The eight frames in g_apR4Ball do not point here. */
// GLOBAL: AARCADE 0024:16FC
int g_anR4BallSpare[] = {
	8, 70, 20, 70, 52, 63, 52, 57, 46,
	52, 46, 52, 26, 57, 26, 63, 20
};

// GLOBAL: AARCADE 0024:171E
int *g_apR4Ball[8] = {
	g_anR4Ball0, g_anR4Ball1, g_anR4Ball2, g_anR4Ball3,
	g_anR4Ball4, g_anR4Ball5, g_anR4Ball6, g_anR4Ball7
};

// GLOBAL: AARCADE 0024:172E
int g_anR4Fungus0[] = {
	4, 2, 0, 15, 0, 15, 13, 2, 13
};

// GLOBAL: AARCADE 0024:1740
int g_anR4Fungus1[] = {
	8, 0, 6, 6, 2, 11, 2, 18, 6,
	18, 9, 11, 13, 6, 13, 0, 9
};

// GLOBAL: AARCADE 0024:1762
int g_anR4Fungus2[] = {
	8, 13, 8, 20, 8, 24, 11, 25, 19,
	20, 26, 11, 24, 6, 18, 8, 11
};

// GLOBAL: AARCADE 0024:1784
int g_anR4Fungus3[] = {
	8, 14, 5, 24, 9, 27, 18, 25, 23,
	18, 27, 9, 25, 5, 21, 6, 9
};

// GLOBAL: AARCADE 0024:17A6
int g_anR4Fungus4[] = {
	10, 1, 10, 7, 1, 26, 1, 30, 6,
	30, 14, 27, 22, 26, 28, 22, 29, 10,
	29, 1, 18
};

// GLOBAL: AARCADE 0024:17D0
int *g_apR4Fungus[5] = {
	g_anR4Fungus0, g_anR4Fungus1, g_anR4Fungus2, g_anR4Fungus3, g_anR4Fungus4
};

// GLOBAL: AARCADE 0024:17DA
int g_anR4Script[10] = {
	24, 24, 24, 32, 32, 32, 24, 16, 12, 8
};

// GLOBAL: AARCADE 0024:17EE
int g_anR4Div[10] = {
	2, 2, 2, 3, 3, 3, 4, 4, 5, 5
};

// GLOBAL: AARCADE 0024:1802
int g_anR4Wave[10] = {
	3, 3, 3, 3, 4, 4, 5, 5, 5, 5
};

// GLOBAL: AARCADE 0024:1816
int g_anR4Burst[10] = {
	2, 3, 4, 5, 5, 6, 6, 7, 7, 8
};

// GLOBAL: AARCADE 0024:182A
int g_anR4Arm[10] = {
	10, 9, 9, 8, 8, 6, 6, 5, 4, 4
};

// GLOBAL: AARCADE 0024:183E
int g_anR4Vel[32][2] = {
	{0, -32},
	{6, -31},
	{12, -29},
	{17, -26},
	{22, -22},
	{26, -17},
	{29, -11},
	{31, -6},
	{32, 0},
	{31, 6},
	{29, 12},
	{26, 17},
	{22, 22},
	{17, 26},
	{11, 29},
	{6, 31},
	{0, 32},
	{-6, 31},
	{-12, 29},
	{-17, 26},
	{-22, 22},
	{-26, 17},
	{-29, 11},
	{-31, 6},
	{-32, 0},
	{-31, -6},
	{-29, -12},
	{-26, -17},
	{-22, -22},
	{-17, -26},
	{-11, -29},
	{-6, -31}
};

/* 0024:18BE. Same eight bytes as the other idle scripts.
   Nothing takes the address. */
// GLOBAL: AARCADE 0024:18BE
char g_r4CmdIdle[8] = {
	0x00, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:18C6
int g_anR4Mark[16] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1
};

/* The copy into a slot asks for 0x40 bytes, so it reads six bytes
   past this array, into g_anR6Count. */
// GLOBAL: AARCADE 0024:18E6
char g_r4Cmd[58] = {
	0x00, 0x02, 0x03, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x0B, 0x04, 0x6A, 0x42, 0x06, 0x02,
	0x09, 0x04, 0x63, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x6B, 0x42, 0x04, 0x08, 0x00, 0x00, 0x63, 0x00,
	0x63, 0x00, 0x0C, 0x04, 0xFC, 0xFF, 0x03, 0x08, 0x63, 0x00, 0x63, 0x00, 0x63, 0x00, 0x01, 0x02,
	0x09, 0x04, 0x01, 0x00, 0x0C, 0x04, 0xFD, 0xFF, 0x0E, 0x02
};

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:4AB4
IMAGE g_r4Img[77];


// GLOBAL: AARCADE 0024:4A8A
SCENE g_r4Scene;


// GLOBAL: AARCADE 0024:4A88
int g_nR4SavedX;


// GLOBAL: AARCADE 0024:4A86
int g_nR4SavedY;


// GLOBAL: AARCADE 0024:4A84
int g_nR4CursorX;


// GLOBAL: AARCADE 0024:4A82
int g_nR4CursorY;


// GLOBAL: AARCADE 0024:4A80
int g_nR4Pad;


// GLOBAL: AARCADE 0024:4A7E
SPRITE *g_pR4Bungee;


// GLOBAL: AARCADE 0024:4A7C
SPRITE *g_pR4Ball;


// GLOBAL: AARCADE 0024:4A7A
int g_nR4Frame;


// GLOBAL: AARCADE 0024:4A78
int g_nR4Frame0;


// GLOBAL: AARCADE 0024:4A76
int g_nR4Count;


// GLOBAL: AARCADE 0024:4A74
int g_nR4Left;


// GLOBAL: AARCADE 0024:4A72
int g_nR4Active;


// GLOBAL: AARCADE 0024:4A6E
unsigned long g_dwR4Deadline;


// GLOBAL: AARCADE 0024:4A6C
int g_nR4Script;


// GLOBAL: AARCADE 0024:4A6A
int g_nR4Div;


// GLOBAL: AARCADE 0024:4A68
int g_nR4Wave;


// GLOBAL: AARCADE 0024:4A66
int g_nR4Burst;


// GLOBAL: AARCADE 0024:4A64
int g_nR4Arm;


// GLOBAL: AARCADE 0024:4A62
HLOCAL g_hR4Mem;


// GLOBAL: AARCADE 0024:4A22
R4SLOT g_aR4Slot[16];


/* Round 4. The scene follows the words; the pictures follow the scene. */
// GLOBAL: AARCADE 0024:4A1E
unsigned long g_dwR4Next;

// FUNCTION: AARCADE 0020:0000
void FAR PASCAL SetupRound4Step(R4STEP *p, int dx, int dy)
{
	p->nFlag = 1;
	p->dx = dx;
	p->adx = dx;
	if (p->adx < 0)
		p->adx = -p->adx;
	p->dy = dy;
	p->ady = dy;
	if (p->ady < 0)
		p->ady = -p->ady;
	p->nMajor = dx;
	p->nAbs = p->adx;
	if (p->ady > p->adx) {
		p->nMajor = dy;
		p->nAbs = p->ady;
		p->nFlag = 0;
	}
}

// FUNCTION: AARCADE 0020:005F
SPRITE *FAR PASCAL AddRound4Sprite(IMAGE *pimg, IMAGE *pmask, int x, int y)
{
	SPRITE *s;

	if ((s = AddSprite(&g_r4Scene)) != 0) {
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

// FUNCTION: AARCADE 0020:00D4
SPRITE *FAR PASCAL InsertRound4Sprite(SPRITE *before, IMAGE *pimg, IMAGE *pmask, int x, int y)
{
	SPRITE *s;

	if ((s = InsertSprite(&g_r4Scene, before)) != 0) {
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

// FUNCTION: AARCADE 0020:014C
void FAR PASCAL SetRound4Move(SPRITE *s, int id, R4HOOK lpfn, int FAR *pMark)
{
	R4POS *p;

	if (s->dwData == 0)
		p = AllocSpriteMove();
	else
		p = (R4POS *)s->dwData;
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

// FUNCTION: AARCADE 0020:01B5
void FAR PASCAL SetRound4Frame(SPRITE *s, IMAGE *pimg, int nFrame, int nFrames, IMAGE *pmask)
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

// FUNCTION: AARCADE 0020:021E
void FAR PASCAL ClearRound4Marks(void)
{
	int i;

	for (i = 0; i < 16; i++)
		g_anR4Mark[i] = -1;
}

// FUNCTION: AARCADE 0020:0239
int FAR *FAR PASCAL AllocRound4Mark(void)
{
	int *p;

	for (p = g_anR4Mark; *p >= 0; p++)
		;
	*p = 0;
	return (int FAR *)p;
}

// FUNCTION: AARCADE 0020:0259
int FAR PASCAL MoveRound4Script(int n, SPRITE *s, void *pm, int fReady)
{
	R4POS *p;

	p = pm;
	if (n < 0) {
		switch (n) {
		case -4:
			s->nAnimRate = 3;
			s->nAnimCount = 0;
			s->pImages = &g_r4Img[68];
			s->pMasks = &g_r4Img[73];
			s->nFrame = 0;
			s->nFrames = 3;
			s->nStep = 1;
			break;
		case -3:
			FreeRound4Slot(s);
			*(int *)p->pMark = -1;
			g_nR4Count++;
			break;
		case -2:
			*(int *)p->pMark = -1;
			break;
		case -1:
			SetRound4Move(s, 0x18BE, 0, 0);
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
		goto done;
	}
	(*(int *)p->pMark)++;
	return 1;
done:;
}

// FUNCTION: AARCADE 0020:0311
void FAR PASCAL MoveRound4Ball(SPRITE *s)
{
	R4STEP st;

	if (s->nGroup != 2) {
		SetupRound4Step(&st, g_nR4CursorX - g_nR4SavedX, g_nR4SavedY - g_nR4CursorY);
		g_nR4Frame0 = g_nR4Frame;
		g_nR4Frame += st.dx >> 2;
		if (g_nR4Frame >= 0x20)
			g_nR4Frame = 0;
		else if (g_nR4Frame < 0)
			g_nR4Frame = 0x1f;
		/* ~3 stays a signed int; 0xFFFC is unsigned and the add reloads */
		g_nR4SavedX += st.dx & ~3;
		g_nR4SavedY = g_nR4CursorY;
		s->cur.img = g_r4Img[g_nR4Frame];
		s->cur.mask = (&g_r4Img[32])[g_nR4Frame];
	}
	if (s->nGroup == 2) {
		if (s->cur.rc.top <= 0 || s->cur.rc.bottom >= 0x146 || s->cur.rc.left <= 0 || s->cur.rc.right >= 0x200) {
			s->dx = -s->dx;
			s->dy = -s->dy;
			s->nGroup = 3;
			s->dwHitMask = 0;
		}
	} else if (s->nGroup == 3) {
		if (s->cur.rc.left == 0xDD && s->cur.rc.top == 0x80) {
			if (GetAsyncKeyState(1) || GetAsyncKeyState(2)) {
				s->dwHitMask = 0x8000L;
				s->dx = g_anR4Vel[g_nR4Frame][0];
				s->dy = g_anR4Vel[g_nR4Frame][1];
				s->nGroup = 2;
			} else {
				s->dx = 0;
				s->dy = 0;
				s->nGroup = 1;
				s->dwHitMask = 0;
			}
		}
	}
}

// FUNCTION: AARCADE 0020:045E
void FAR PASCAL HitTestRound4Ball(SPRITE *s, SPRITE *other, BOOL fFirst, int *pfHit)
{
	long n;

	if (fFirst) {
		n = 0;
		if (other->cur.rc.left <= 0x10F && other->cur.rc.right >= 0xF1 && other->cur.rc.top <= 0xB2 && other->cur.rc.bottom >= 0x94) {
			*pfHit = 0;
			return;
		}
		n = OverlapRound4(g_nR4Frame, s, other);
		*pfHit = (int)n;
		if (n != 0) {
			g_nR4Left--;
			g_nR4Active--;
			FreeRound4Slot(other);
			*(int *)((R4POS *)other->dwData)->pMark = -1;
			PlaySfx(0x4269, 1);
			if (other->cur.img.hbm == g_r4Img[67].hbm)
				AddPlayScore(25L);
			else
				AddPlayScore(15L);
			other->nGroup = 5;
			s->dx = -s->dx;
			s->dy = -s->dy;
			s->nGroup = 3;
			s->dwHitMask = 0;
		}
	} else
		*pfHit = 0;
}

// FUNCTION: AARCADE 0020:0534
void FAR PASCAL MovedRound4Ball(SPRITE *s)
{
	if (s->cur.rc.right <= 0 || s->cur.rc.bottom <= 0 || s->cur.rc.left >= 0x200 || s->cur.rc.top >= 0x146) {
		s->fDead = 1;
		g_pR4Ball = 0;
	}
}

// FUNCTION: AARCADE 0020:0566
void FAR PASCAL BurstRound4Ball(void)
{
	SPRITE *s;

	if (g_pR4Ball != 0) {
		s = g_pR4Ball;
		s->nGroup = 0;
		g_pR4Bungee->nGroup = 5;
		SetRound4Frame(s, g_r4Img, g_nR4Frame, 0x20, &g_r4Img[32]);
		s->nAnimRate = 1;
		s->nAnimCount = 0;
		s->nStep = 2;
		s->nAnimMode = 1;
		s->lpfnMove = 0;
		s->lpfnMoved = MovedRound4Ball;
		s->lpfnMoveEx = 0;
		s->dwHitMask = 0;
		s->dx = rand() % 0x18;
		if (s->dx < 0)
			s->dy = s->dx + 0x18;
		else
			s->dy = 0x18 - s->dx;
		if (rand() < 0)
			s->dy = -s->dy;
		s->nMoveRate = 1;
		g_nR4Count++;
		PlaySfx(0x426C, 1);
	}
}

// FUNCTION: AARCADE 0020:061D
void FAR PASCAL AddRound4Ball(void)
{
	SPRITE *s;

	g_nR4Frame0 = g_nR4Frame = 0;
	g_nR4Count = 0;
	s = AddSprite(&g_r4Scene);
	g_pR4Ball = s;
	if (s != 0) {
		s->nGroup = 1;
		s->cur.x = 0xDD;
		s->cur.y = 0x80;
		s->cur.img = g_r4Img[0];
		s->cur.mask = g_r4Img[32];
		s->nDrawMode = -1;
		UpdateSpriteRect(s);
		s->nAnimRate = 1;
		s->lpfnMove = MoveRound4Ball;
		s->lpfnHitTest = HitTestRound4Ball;
	}
}

// FUNCTION: AARCADE 0020:06A0
void FAR PASCAL MoveRound4Bungee(SPRITE *s, BOOL fFirst)
{
	s->old = s->cur;
	if (s->nGroup == 5) {
		s->cur.fVisible = 0;
		s->fDirty = 1;
	} else if (g_pR4Ball->nGroup != 1) {
		if (g_pR4Ball->cur.x <= 0xFA) {
			s->cur.x = s->cur.rc.left = g_pR4Ball->cur.x + 0x23;
			s->cur.rc.right = 0x102;
		} else {
			s->cur.rc.left = 0x100;
			s->cur.x = s->cur.rc.right = g_pR4Ball->cur.x + 0x23;
		}
		if (g_pR4Ball->cur.y <= 0x9D) {
			s->cur.y = s->cur.rc.top = g_pR4Ball->cur.y + 0x23;
			s->cur.rc.bottom = 0xA5;
		} else {
			s->cur.rc.top = 0xA3;
			s->cur.y = s->cur.rc.bottom = g_pR4Ball->cur.y + 0x23;
		}
		s->fDirty = 1;
	}
}

// FUNCTION: AARCADE 0020:074E
void FAR PASCAL DrawRound4Bungee(HDC hdc, SPRITE *s, IMAGE *pimg, BOOL fCur)
{
	HGDIOBJ hOld, hPen;

	if (s->cur.x != 0xFA || s->cur.y != 0x9D) {
		hPen = CreatePen(0, 2, 0);
		hOld = SelectObject(hdc, hPen);
		MoveTo(hdc, s->cur.x, s->cur.y);
		LineTo(hdc, 0x100, 0xA3);
		SelectObject(hdc, hOld);
		DeleteObject(hPen);
	}
}

// FUNCTION: AARCADE 0020:07B7
void FAR PASCAL AddRound4Bungee(void)
{
	g_pR4Bungee = AddSprite(&g_r4Scene);
	if (g_pR4Bungee != 0) {
		g_pR4Bungee->cur.x = 0xFA;
		g_pR4Bungee->cur.y = 0x9D;
		g_pR4Bungee->lpfnMoveEx = MoveRound4Bungee;
		g_pR4Bungee->lpfnDraw = DrawRound4Bungee;
	}
}

// FUNCTION: AARCADE 0020:0800
void FAR PASCAL LayRound4(void)
{
	AddRound4Ball();
	AddRound4Bungee();
	AddRound4Sprite(&g_r4Img[65], 0, 0xFA, 0x9D);
	AddRound4Sprite(&g_r4Img[76], 0, 0, 0);
}

// FUNCTION: AARCADE 0020:082B
void FAR PASCAL AllocRound4Pool(void)
{
	int i;
	char *p;

	g_hR4Mem = LocalAlloc(0x40, 0x400);
	for (i = 0; i < 16; i++) {
		g_aR4Slot[i].s = 0;
		p = (char *)g_hR4Mem + (i << 6);
		g_aR4Slot[i].p = p;
	}
}

// FUNCTION: AARCADE 0020:0873
R4SLOT *FAR PASCAL FindRound4Slot(void)
{
	int i;

	for (i = 0; i < 16; i++) {
		if (g_aR4Slot[i].s == 0)
			return &g_aR4Slot[i];
	}
	return 0;
}

// FUNCTION: AARCADE 0020:089E
void FAR PASCAL FreeRound4Slot(SPRITE *s)
{
	int i;

	for (i = 0; i < 16; i++) {
		if (g_aR4Slot[i].s == s) {
			g_aR4Slot[i].s = 0;
			break;
		}
	}
}

// FUNCTION: AARCADE 0020:08CC
void FAR PASCAL FreeRound4Pool(void)
{
	if (g_hR4Mem != 0)
		LocalFree(g_hR4Mem);
}

// FUNCTION: AARCADE 0020:08E3
void FAR PASCAL MovedRound4Fungus(SPRITE *s)
{
	if (s->nGroup == 5) {
		s->cur.fVisible = 0;
		s->fDirty = 1;
	}
}

// FUNCTION: AARCADE 0020:08FE
void FAR PASCAL TickRound4Fungus(SPRITE *s)
{
	/* unsigned char: mov ah,0 rather than cbw */
	if (!(unsigned char)s->old.fVisible && !(unsigned char)s->cur.fVisible)
		s->fDead = 1;
}

// FUNCTION: AARCADE 0020:0922
SPRITE *FAR PASCAL SpawnRound4Fungus(void)
{
	R4SLOT *pSlot;
	SPRITE *s;
	int x, tx, ty, nSpeed, nSpeed2, nPick;
	RECT rc;
	POINT pt;
	int nLo;
	R4STEP st;
	register int y;

	pSlot = FindRound4Slot();
	if (pSlot == 0)
		return 0;
	nPick = rand() & 3;
	if (--nPick < 0) {
		x = -0x200;
		y = 0;
	} else if (--nPick < 0) {
		x = -0x200;
		y = 0x146;
	} else {
		if (--nPick < 0)
			x = 0;
		else
			x = 0x200;
		y = -0x146;
	}
	if (x < 0)
		x = (unsigned)rand() % (unsigned)(-x);
	else if (y == 0)
		x -= g_r4Img[66].rc.right;
	if (y < 0)
		y = (unsigned)rand() % (unsigned)(-y);
	else if (y == 0)
		y -= g_r4Img[66].rc.bottom;
	rc.left = rc.right = 0x100;
	rc.top = rc.bottom = 0xA3;
	InflateRect(&rc, 0x50, 0x40);
	do {
		if (x < 0x100) {
			nLo = 4;
			_SI = 0x100;
		} else {
			nLo = 0x100;
			_SI = 0x1FC;
		}
		_SI -= g_r4Img[66].rc.right;
		tx = (unsigned)rand() % (unsigned)(_SI - nLo) + nLo;
		if (y < 0xA3) {
			nLo = 4;
			_SI = 0xA3;
		} else {
			nLo = 0xA3;
			_SI = 0x142;
		}
		_SI -= g_r4Img[66].rc.bottom;
		ty = (unsigned)rand() % (unsigned)(_SI - nLo) + nLo;
		pt.x = tx;
		pt.y = ty;
		if (PtInRect(&rc, pt) == 0)
			break;
	} while (1);
	SetupRound4Step(&st, tx - x, ty - y);
	nSpeed = st.nAbs >> 3;
	SetupRound4Step(&st, 0x100 - tx, ty + -0xA3);
	nSpeed2 = (st.nAbs << 2) / g_nR4Div;
	s = InsertRound4Sprite(g_pR4Bungee, &g_r4Img[66], &g_r4Img[71], x, y);
	s->dwHitMask = 0x8000L;
	s->nGroup = 4;
	s->lpfnTick = TickRound4Fungus;
	s->lpfnMoved = MovedRound4Fungus;
	SetRound4Frame(s, &g_r4Img[66], 0, 5, &g_r4Img[71]);
	_fmemcpy(pSlot->p, g_r4Cmd, 0x40);
	pSlot->s = s;
	g_nR4Active++;
	_SI = *(int *)((char *)pSlot + 2);
	_SI += *(char *)(_SI + 1);
	*(int *)(_SI + 2) = nSpeed;
	*(int *)(_SI + 4) = tx;
	*(int *)(_SI + 6) = ty;
	_SI += *(char *)(_SI + 1);
	_SI += *(char *)(_SI + 1);
	_SI += *(char *)(_SI + 1);
	*(int *)(_SI + 2) = g_nR4Script;
	_SI += *(char *)(_SI + 1);
	_SI += *(char *)(_SI + 1);
	_SI += *(char *)(_SI + 1);
	*(int *)(_SI + 4) = -8;
	*(int *)(_SI + 6) = -8;
	_SI += *(char *)(_SI + 1);
	_SI += *(char *)(_SI + 1);
	*(int *)(_SI + 2) = nSpeed2;
	*(int *)(_SI + 4) = 0xF0;
	*(int *)(_SI + 6) = 0x93;
	SetRound4Move(s, (int)pSlot->p, MoveRound4Script, AllocRound4Mark());
	return s;
}

// FUNCTION: AARCADE 0020:0BAA
void FAR PASCAL SpawnRound4Wave(void)
{
	int n;

	if (g_nR4Wave != 0) {
		if (GetTickCount() / 17 >= g_dwR4Deadline || g_nR4Active == 0) {
			n = g_nR4Burst;
			while (n-- != 0) {
				if (SpawnRound4Fungus() == 0)
					g_nR4Left--;
			}
			g_nR4Wave--;
			g_dwR4Deadline = GetTickCount() / 17 + (long)(g_nR4Arm * 0x3C);
		}
	}
}

// FUNCTION: AARCADE 0020:0C23
void FAR PASCAL SetupRound4(void)
{
	int n;

	g_nR4Active = 0;
	n = g_nPlayWave;
	if (n >= 10)
		n = (n & 3) + 6;
	g_nR4Script = g_anR4Script[n];
	g_nR4Div = g_anR4Div[n];
	if (g_nPlayWave >= 10)
		g_nR4Div += g_nPlayWave + -9;
	g_nR4Wave = g_anR4Wave[n];
	g_nR4Burst = g_anR4Burst[n];
	g_nR4Arm = g_anR4Arm[n];
	g_nR4Left = g_nR4Wave * g_nR4Burst;
}

// FUNCTION: AARCADE 0020:0C95
void FAR PASCAL ArmRound4(void)
{
	SetTimeLeft((long)(g_nPlayWave * 5 + 0x1E));
}

// FUNCTION: AARCADE 0020:0CAD
void FAR PASCAL EmptyRound4Back(IMAGE *pimg, int n, RECT *prc)
{
}

// FUNCTION: AARCADE 0020:0CB6
void FAR PASCAL BackRound4(IMAGE *pimg, int n, RECT *prc)
{
	FillImage(pimg, prc, g_hGdiObjects[3]);
}

// FUNCTION: AARCADE 0020:0CCE
void FAR PASCAL OpenRound4(HWND hwnd)
{
	int n;

	n = GetPatrolBackground();
	if (g_fColor) {
		LoadPPicImages(0x4268, 0, 0x4C, g_r4Img, g_nDepth);
		if (n != 0)
			InitScene(hwnd, &g_r4Scene, (SCENEBKPROC)BackRound4, &g_rcPlay, g_nDepth2, 3, 0);
		else {
			GetPPicImage(0x1B59, 0, &g_r4Img[76], g_nDepth);
			InitScene(hwnd, &g_r4Scene, (SCENEBKPROC)EmptyRound4Back, &g_rcPlay, g_nDepth2, 3, 0);
		}
	} else {
		LoadPPicImages(0x4268, 0, 0x4C, g_r4Img, g_nDepth);
		InitScene(hwnd, &g_r4Scene, 0, &g_rcPlay, 1, 3, 0);
	}
	ArmRound4();
	SetupRound4();
	ClearRound4Marks();
	AllocRound4Pool();
	LayRound4();
	g_nR4Pad = 0;
	g_dwR4Deadline = 0;
	PreloadSounds(0x4268, 5);
}

// FUNCTION: AARCADE 0020:0D9B
int FAR PASCAL OverlapRound4(int nFrame, SPRITE *s, SPRITE *other)
{
	int f;
	HRGN a, b, c;

	f = 1;
	a = MakeBallRgn(s);
	b = MakeFungusRgn(other);
	if (a != 0 && b != 0) {
		c = CreateRectRgn(0, 0, 0, 0);
		if (CombineRgn(c, a, b, 1) == 1)
			f = 0;
		DeleteObject(c);
	}
	if (a != 0)
		DeleteObject(a);
	if (b != 0)
		DeleteObject(b);
	return f;
}

// FUNCTION: AARCADE 0020:0E13
HRGN FAR PASCAL FungusRgnRound4(int n)
{
	int i;
	int *p;
	int nPts;
	POINT pt[12];
	int k;

	p = g_apR4Fungus[n];
	nPts = *p;
	i = 0;
	k = 1;
	while (i < nPts) {
		pt[i].x = p[k++];
		pt[i].y = p[k++];
		i++;
	}
	return CreatePolygonRgn(pt, nPts, 2);
}

// FUNCTION: AARCADE 0020:0E8F
HRGN FAR PASCAL MakeFungusRgn(SPRITE *s)
{
	HRGN rgn;
	int i;

	rgn = 0;
	for (i = 0; i < 5; i++) {
		if (g_r4Img[66 + i].hbm == s->cur.img.hbm) {
			rgn = FungusRgnRound4(i);
			OffsetRgn(rgn, s->cur.rc.left, s->cur.rc.top);
			break;
		}
	}
	return rgn;
}

// FUNCTION: AARCADE 0020:0EDC
HRGN FAR PASCAL MakeBallRgn(SPRITE *s)
{
	HRGN rgn;
	RECT rc;

	rc = s->cur.rc;
	rgn = BallRgnRound4(g_nR4Frame, &rc);
	return rgn;
}

// FUNCTION: AARCADE 0020:0F12
HRGN FAR PASCAL BallRgnRound4(int nFrame, RECT *prc)
{
	HRGN rgn;
	int *p;
	int nPts;
	int nRem;
	int nDy;
	int nDx;
	int nFlip;
	POINT pt[10];
	int i, j;

	rgn = 0;
	nFlip = nDy = nDx = 0;
	switch (nFrame / 8) {
	case 1:
		nFlip = 1;
		nDy = -0x47;
		break;
	case 2:
		nDy = nDx = -0x47;
		break;
	case 3:
		nFlip = 1;
		nDx = -0x47;
		break;
	default:
		break;
	}
	nRem = nFrame % 8;
	p = g_apR4Ball[nRem];
	nPts = *p;
	i = 0;
	j = 1;
	while (i < nPts) {
		pt[i].x = abs(p[j + nFlip] + nDy);
		j++;
		pt[i].y = abs(p[j - nFlip] + nDx);
		j++;
		i++;
	}
	rgn = CreatePolygonRgn(pt, nPts, 2);
	OffsetRgn(rgn, prc->left, prc->top);
	return rgn;
}

// FUNCTION: AARCADE 0020:100E
void FAR PASCAL ClickRound4(void)
{
	if (g_pR4Ball->nGroup == 1) {
		g_pR4Ball->dx = g_anR4Vel[g_nR4Frame][0];
		g_pR4Ball->dy = g_anR4Vel[g_nR4Frame][1];
		g_pR4Ball->nMoveRate = 1;
		g_pR4Ball->nGroup = 2;
		g_pR4Ball->dwHitMask = 0x8000L;
		PlaySfx(0x4268, 0);
	}
}

// FUNCTION: AARCADE 0020:106B
int FAR PASCAL TickRound4(void)
{
	SpawnRound4Wave();
	if (g_nR4Active == 0 && g_nR4Left == 0 && g_pR4Ball->nGroup == 1) {
		RunStepDialog();
		NextPlayWave();
		ArmRound4();
		SetupRound4();
		g_nR4SavedX = (int)GetMessagePos();
	}
	g_nR4CursorX = (int)GetMessagePos();
	g_nR4CursorY = (int)(GetMessagePos() >> 16);
	if (GetTickCount() / 17 >= g_dwR4Next) {
		g_dwR4Next = GetTickCount() / 17 + 3;
		TickScene(&g_r4Scene);
		if (g_nR4Count == 1)
			BurstRound4Ball();
		return g_pR4Ball == 0 ? 1 : 0;
		goto done;
	}
	return 0;
done:;
}

// FUNCTION: AARCADE 0020:111C
void FAR PASCAL PaintRound4(void)
{
	PaintScene(&g_r4Scene);
}

// FUNCTION: AARCADE 0020:112B
void FAR PASCAL CloseRound4(void)
{
	FreeRound4Pool();
	FreeScene(&g_r4Scene);
	FreeImages(g_r4Img, 0x4D, 1);
}
