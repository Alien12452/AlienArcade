#include "aarcade.h"

/* Title / start-screen animation and RunStartDialog. */

typedef int (FAR PASCAL *STARTHOOK)(int n, SPRITE *s, void *pm, int fReady);

/* AllocSpriteMove's block: hook at +0x1a, mark pointer at +0x1e. */
typedef struct tagSTARTPOS {
	int id;
	int x0;
	int x1;
	int y0;
	int y1;
	int unused[8];
	STARTHOOK lpfn;
	int FAR *pMark;
} STARTPOS;

/* Line step for a sprite. Nothing in the segment calls it. */
typedef struct tagSTARTSTEP {
	int x;
	int y;
	int ax;
	int ay;
	int major;
	int majorAbs;
	int xMajor;
} STARTSTEP;

/* The flag and the idle script sit in the bytes just after round 6's script. */
// GLOBAL: AARCADE 0024:1A12
int g_fStart = 0;

// GLOBAL: AARCADE 0024:1A14
char g_cmdStartIdle[] = { 0x00, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0A, 0x00 };

// GLOBAL: AARCADE 0024:1A1C
int g_anStartMark[16] = {
	-1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};

/* Not referenced. The 0x80 copy of g_r6Cmd reads the first four bytes. */
// GLOBAL: AARCADE 0024:1A3C
char g_cmdStartGap[] = {
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
	0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02,
	0x07, 0x02, 0x07, 0x02, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:1A72
char g_cmdStartA[] = {
	0x0C, 0x04, 0x0F, 0x00, 0x0B, 0x04, 0xA0, 0x0F, 0x03, 0x08, 0x10, 0x00,
	0xB7, 0x01, 0x70, 0x00, 0x0C, 0x04, 0x10, 0x00, 0x09, 0x04, 0x06, 0x00,
	0x03, 0x08, 0x10, 0x00, 0x70, 0x01, 0x46, 0x00, 0x0C, 0x04, 0x00, 0x00,
	0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:1A9C
char g_cmdStartB[] = { 0x0C, 0x04, 0x01, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x0B, 0x00, 0x01, 0x02, 0x0E, 0x02 };

// GLOBAL: AARCADE 0024:1AAA
char g_cmdStartC[] = { 0x0C, 0x04, 0x02, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x0B, 0x00, 0x01, 0x02, 0x0E, 0x02 };

// GLOBAL: AARCADE 0024:1AB8
char g_cmdStartD[] = { 0x0C, 0x04, 0x03, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x0B, 0x00, 0x01, 0x02, 0x0E, 0x02 };

// GLOBAL: AARCADE 0024:1AC6
char g_cmdStartE[] = {
	0x0C, 0x04, 0x04, 0x00, 0x09, 0x04, 0x08, 0x00, 0x04, 0x08, 0x10, 0x00, 0x00, 0x00, 0xC6, 0xFF,
	0x09, 0x04, 0x08, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x10, 0x00, 0x0C, 0x04, 0x00, 0x00,
	0x09, 0x04, 0x08, 0x00, 0x06, 0x02, 0x09, 0x04, 0x10, 0x00, 0x07, 0x02, 0x09, 0x04, 0x08, 0x00,
	0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x10, 0x00,
	0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x08, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x08, 0x00,
	0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x08, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x01, 0x00,
	0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x08, 0x00,
	0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x08, 0x00,
	0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x08, 0x00, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00,
	0x07, 0x02, 0x09, 0x04, 0x14, 0x00, 0x06, 0x02, 0x06, 0x02, 0x04, 0x08, 0x04, 0x00, 0xD0, 0xFF,
	0x00, 0x00, 0x07, 0x02, 0x07, 0x02, 0x04, 0x08, 0x04, 0x00, 0xD0, 0xFF, 0x00, 0x00, 0x06, 0x02,
	0x06, 0x02, 0x04, 0x08, 0x04, 0x00, 0xD0, 0xFF, 0x00, 0x00, 0x07, 0x02, 0x07, 0x02, 0x04, 0x08,
	0x04, 0x00, 0xD0, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x06, 0x02, 0x04, 0x08, 0x04, 0x00, 0xD0, 0xFF,
	0x00, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:1B9E
char g_cmdStartF[] = {
	0x0C, 0x04, 0x05, 0x00, 0x06, 0x02, 0x02, 0x02, 0x0C, 0x04, 0x06, 0x00, 0x01, 0x02,
	0x0C, 0x04, 0x07, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x08, 0x00, 0x01, 0x02, 0x0C, 0x04,
	0x09, 0x00, 0x07, 0x02, 0x02, 0x02, 0x0C, 0x04, 0x0A, 0x00, 0x01, 0x02, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:1BC8
char g_cmdStartG[] = { 0x0C, 0x04, 0x0C, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x11, 0x00, 0x01, 0x02, 0x0E, 0x02 };

// GLOBAL: AARCADE 0024:1BD6
char g_cmdStartH[] = { 0x0C, 0x04, 0x0D, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x11, 0x00, 0x01, 0x02, 0x0E, 0x02 };

// GLOBAL: AARCADE 0024:1BE4
char g_cmdStartI[] = {
	0x0C, 0x04, 0x0E, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x03, 0x08, 0x0A, 0x00, 0x27, 0x00, 0x9C, 0x00,
	0x03, 0x08, 0x06, 0x00, 0x70, 0x00, 0xA7, 0x00, 0x03, 0x08, 0x07, 0x00, 0xC8, 0x00, 0xA7, 0x00,
	0x03, 0x08, 0x09, 0x00, 0x2D, 0x01, 0x9D, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x03, 0x08, 0x09, 0x00,
	0x86, 0x01, 0x86, 0x00, 0x03, 0x08, 0x05, 0x00, 0xB8, 0x01, 0x72, 0x00, 0x01, 0x02, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:1C24
char g_cmdStartJ[] = { 0x0C, 0x04, 0x12, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02 };

// GLOBAL: AARCADE 0024:1C30
char g_cmdStartK[] = { 0x0C, 0x04, 0x13, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02 };

// GLOBAL: AARCADE 0024:1C3C
char g_cmdStartL[] = { 0x0C, 0x04, 0x14, 0x00, 0x02, 0x02, 0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02 };

// GLOBAL: AARCADE 0024:1C48
char g_cmdStartM[] = {
	0x0C, 0x04, 0x15, 0x00, 0x02, 0x02, 0x03, 0x08, 0x08, 0x00, 0x2E, 0x00, 0x4A, 0x01,
	0x0C, 0x04, 0x16, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:1C5C
char g_cmdStartN[] = {
	0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00,
	0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00,
	0x0C, 0x04, 0x0B, 0x00, 0x09, 0x04, 0x0C, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x1E, 0x00,
	0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x1E, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0x11, 0x00,
	0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00,
	0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x18, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0xF0, 0x00,
	0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0xFD, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0E, 0x02
};

/* Not in aarcade.h, so this order is the one Borland reverses: highest address first. */
// GLOBAL: AARCADE 0024:5052
IMAGE g_imgStart[48];

// GLOBAL: AARCADE 0024:5028
SCENE g_sceneStart;

// GLOBAL: AARCADE 0024:5020
RECT g_rcStart;

// GLOBAL: AARCADE 0024:501C
DWORD g_dwStart;

// GLOBAL: AARCADE 0024:501A
SPRITE *g_pStartSprite;

// GLOBAL: AARCADE 0024:5016
int FAR *g_pStartMark;


int FAR PASCAL HookStart(int n, SPRITE *s, void *pm, int fReady);
BOOL FAR PASCAL StartDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);

// FUNCTION: AARCADE 0022:0000
SPRITE *FAR PASCAL AddStartSprite(SCENE *ps, IMAGE *pimg, IMAGE *pmask, int x, int y, int nMode)
{
	SPRITE *s;

	if ((s = AddSprite(ps)) != 0) {
		s->cur.img = *pimg;
		s->cur.mask = *pmask;
		s->cur.x = x;
		s->cur.y = y;
		UpdateSpriteRect(s);
		s->pImages = pimg;
		s->pMasks = pmask;
		s->nDrawMode = nMode;
	}
	return s;
}

// FUNCTION: AARCADE 0022:0067
void FAR PASCAL SetStartMove(SPRITE *s, int id, STARTHOOK lpfn, int FAR *pMark)
{
	STARTPOS *p;

	if (s->dwData == 0)
		p = AllocSpriteMove();
	else
		p = (STARTPOS *)s->dwData;
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

// FUNCTION: AARCADE 0022:00D0
void FAR PASCAL SetStartFrame(SPRITE *s, IMAGE *pimg, IMAGE *pmask, int nFrame, int nFrames)
{
	s->pImages = pimg;
	s->pMasks = pmask;
	s->nFrame = nFrame;
	s->nFrames = nFrames;
	s->cur.img = pimg[nFrame];
	UpdateSpriteRect(s);
}

// FUNCTION: AARCADE 0022:0119
void FAR PASCAL SetupStartStep(STARTSTEP *p, int x, int y)
{
	p->xMajor = 1;
	p->x = x;
	p->ax = x;
	if (p->ax < 0)
		p->ax = -p->ax;
	p->y = y;
	p->ay = y;
	if (p->ay < 0)
		p->ay = -p->ay;
	p->major = x;
	p->majorAbs = p->ax;
	if (p->ay > p->ax) {
		p->major = y;
		p->majorAbs = p->ay;
		p->xMajor = 0;
	}
}

// FUNCTION: AARCADE 0022:0178
void FAR PASCAL ClearStartMarks(void)
{
	int i;

	for (i = 0; i < 16; i++)
		g_anStartMark[i] = -1;
}

// FUNCTION: AARCADE 0022:0193
int FAR *FAR PASCAL AllocStartMark(void)
{
	int *p;

	for (p = g_anStartMark; *p >= 0; p++)
		;
	*p = 0;
	return (int FAR *)p;
}

// FUNCTION: AARCADE 0022:01B3
int FAR PASCAL HookStart(int n, SPRITE *s, void *pm, int fReady)
{
	STARTPOS *p;

	p = pm;
	if (n < 0) {
		switch (n) {
		case -3:
			g_fStart = 1;
			break;
		case -2:
			*(int *)p->pMark = -1;
			break;
		case -1:
			SetStartMove(s, (int)g_cmdStartIdle, 0, 0);
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

// FUNCTION: AARCADE 0022:0232
void FAR PASCAL AddStartBack(void)
{
	AddStartSprite(&g_sceneStart, &g_imgStart[1], &g_imgStart[2], 0x185, 0x3E, -1);
}

// FUNCTION: AARCADE 0022:024D
void FAR PASCAL AddStartFigure(void)
{
	SPRITE *s;

	s = AddStartSprite(&g_sceneStart, &g_imgStart[5], 0, 0x1AD, 0x1C, 0);
	SetStartFrame(s, &g_imgStart[5], 0, 0, 7);
	s->nAnimRate = 2;
	s->nAnimMode = 1;
}

// FUNCTION: AARCADE 0022:0282
void FAR PASCAL AddStartMover(void)
{
	SPRITE *s;

	s = AddStartSprite(&g_sceneStart, &g_imgStart[3], &g_imgStart[4], 0x170, 0x46, -1);
	SetStartMove(s, (int)g_cmdStartA, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[3], &g_imgStart[4], 0, 1);
}

// FUNCTION: AARCADE 0022:02C6
void FAR PASCAL AddStartTrio(void)
{
	SPRITE *s;

	s = AddStartSprite(&g_sceneStart, &g_imgStart[12], &g_imgStart[13], 0x0E, 0x10, -1);
	SetStartMove(s, (int)g_cmdStartB, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[12], &g_imgStart[13], 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[14], &g_imgStart[15], 0x0E, 0x2A, -1);
	SetStartMove(s, (int)g_cmdStartC, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[14], &g_imgStart[15], 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[16], &g_imgStart[17], 0x0E, 0x45, -1);
	SetStartMove(s, (int)g_cmdStartD, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[16], &g_imgStart[17], 0, 1);
	s->cur.fVisible = 0;
}

// FUNCTION: AARCADE 0022:0389
void FAR PASCAL AddStartPair(void)
{
	SPRITE *s;

	s = AddStartSprite(&g_sceneStart, &g_imgStart[30], 0, 0xCD, 0x132, 0);
	SetStartMove(s, (int)g_cmdStartF, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[30], 0, 0, 2);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[18], &g_imgStart[24], 0xBE, 0x156, -1);
	SetStartMove(s, (int)g_cmdStartE, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[18], &g_imgStart[24], 0, 6);
}

// FUNCTION: AARCADE 0022:040C
void FAR PASCAL AddStartWing(void)
{
	SPRITE *s;

	s = AddStartSprite(&g_sceneStart, &g_imgStart[32], &g_imgStart[33], 0x0E, 0x10, -1);
	SetStartMove(s, (int)g_cmdStartG, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[32], &g_imgStart[33], 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[34], &g_imgStart[35], 0x0E, 0x2A, -1);
	SetStartMove(s, (int)g_cmdStartH, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[34], &g_imgStart[35], 0, 1);
	s->cur.fVisible = 0;
}

// FUNCTION: AARCADE 0022:0491
void FAR PASCAL AddStartFloater(void)
{
	SPRITE *s;

	s = AddStartSprite(&g_sceneStart, &g_imgStart[46], &g_imgStart[47], -0x52, 0x6D, -1);
	SetStartMove(s, (int)g_cmdStartI, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[46], &g_imgStart[47], 0, 1);
}

// FUNCTION: AARCADE 0022:04D4
void FAR PASCAL AddStartRow(void)
{
	SPRITE *s;

	s = AddStartSprite(&g_sceneStart, &g_imgStart[36], &g_imgStart[37], 0x3C, 0x13, -1);
	SetStartMove(s, (int)g_cmdStartJ, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[36], &g_imgStart[37], 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[38], &g_imgStart[39], 0x11, 0x55, -1);
	SetStartMove(s, (int)g_cmdStartJ, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[38], &g_imgStart[39], 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[40], 0, 0x3A, 0xB2, 0);
	SetStartMove(s, (int)g_cmdStartK, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[40], 0, 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[41], 0, 0x3A, 0xBF, 0);
	SetStartMove(s, (int)g_cmdStartK, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[41], 0, 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[42], 0, 0x3A, 0xCF, 0);
	SetStartMove(s, (int)g_cmdStartK, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[42], 0, 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[43], 0, 0x3A, 0xEE, 0);
	SetStartMove(s, (int)g_cmdStartL, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[43], 0, 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[44], 0, 0x3A, 0xFC, 0);
	SetStartMove(s, (int)g_cmdStartL, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[44], 0, 0, 1);
	s->cur.fVisible = 0;
	s = AddStartSprite(&g_sceneStart, &g_imgStart[45], 0, 0x2E, 0x156, 0);
	SetStartMove(s, (int)g_cmdStartM, HookStart, g_pStartMark);
	SetStartFrame(s, &g_imgStart[45], 0, 0, 1);
	s->cur.fVisible = 0;
}

// FUNCTION: AARCADE 0022:06C7
void FAR PASCAL LayStartSprites(void)
{
	ClearStartMarks();
	g_pStartMark = AllocStartMark();
	g_pStartSprite = AddStartSprite(&g_sceneStart, 0, 0, 0, 0, 0);
	g_pStartSprite->cur.fVisible = 0;
	SetStartMove(g_pStartSprite, (int)g_cmdStartN, HookStart, g_pStartMark);
	AddStartBack();
	AddStartFigure();
	AddStartMover();
	AddStartTrio();
	AddStartPair();
	AddStartWing();
	AddStartFloater();
	AddStartRow();
}

// FUNCTION: AARCADE 0022:0732
void FAR PASCAL BackStart(IMAGE *pimg, LPRECT lprc)
{
	BlitImage(pimg, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
		&g_imgStart[0], lprc->left, lprc->top, SRCCOPY);
}

// FUNCTION: AARCADE 0022:0782
LRESULT CALLBACK StartWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TickSound();
	switch (msg) {
	case WM_ERASEBKGND: {
		HDC hdc;
		RECT rc;

		hdc = (HDC)wParam;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
	}
		break;
	case WM_PAINT:
		PaintScene(&g_sceneStart);
		break;
	case WM_TIMER: {
		HWND hwndParent;

		TickScene(&g_sceneStart);
		if (g_fStart) {
			hwndParent = GetParent(hwnd);
			PostMessage(hwndParent, WM_COMMAND, 1, 0L);
		}
		break;
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0022:0828
void FAR PASCAL OpenStart(HWND hwnd, int id)
{
	HWND hwndItem;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	hwndItem = GetDlgItem(hwnd, id);
	GetClientRect(hwndItem, &g_rcStart);
	if (g_fColor) {
		LoadPPicImages(0xFA1, 0, 0x26, g_imgStart, g_nDepth);
		LoadPPicImages(0xFA2, 0, 0x0A, &g_imgStart[38], g_nDepth);
	} else
		LoadPPicImages(0xFA0, 0, 0x30, g_imgStart, g_nDepth);
	InitScene(hwndItem, &g_sceneStart, BackStart, &g_rcStart, g_nDepth2, 3, 0);
	g_dwStart = 0;
	LayStartSprites();
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	PreloadSounds(0xFA0, 1);
	SubclassAnimItem(hwnd, id, StartWndProc, 3);
}

// FUNCTION: AARCADE 0022:08F7
void FAR PASCAL CloseStart(HWND hwnd, int id)
{
	UnsubclassAnimItem(hwnd, id);
	FreeScene(&g_sceneStart);
	FreeImages(g_imgStart, 0x30, 1);
}

// FUNCTION: AARCADE 0022:091F
int FAR PASCAL RunStartDialog(void)
{
	return RunModalDialog(0x82, StartDlgProc, 0, 0);
}

// FUNCTION: AARCADE 0022:093A
BOOL FAR PASCAL StartDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG: {
		HWND hwndItem;

		g_fStart = 0;
		MoveWindow(hDlg, 0, 0, 0x200, 0x156, FALSE);
		hwndItem = GetDlgItem(hDlg, 1);
		MoveWindow(hwndItem, 0, 0, 0x200, 0x156, FALSE);
		OpenStart(hDlg, 1);
		CenterPopup(hDlg);
	}
		break;
	case WM_DESTROY:
		CloseStart(hDlg, 1);
		return 0;
	case WM_ERASEBKGND: {
		HDC hdc;
		RECT rc;

		hdc = (HDC)wParam;
		GetClientRect(hDlg, &rc);
		FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
	}
		break;
	case WM_COMMAND:
		EndModalDialog(hDlg, wParam);
		break;
	default:
		return 0;
	}
	return 1;
}
