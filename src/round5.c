/* Round 5: Andromedroids. Keep the factory assembling robots. */
#include <stdlib.h>
#include <windows.h>
#include "aarcade.h"

typedef int (FAR PASCAL *R5HOOK)(int n, SPRITE *s, void *pm, int fReady);

/* AllocSpriteMove's block, laid out like round 4's: hook at +0x1a, mark after it. */
typedef struct tagR5POS {
	int id;
	int x0;
	int x1;
	int y0;
	int y1;
	int unused[8];
	R5HOOK lpfn;
	int FAR *pMark;
} R5POS;

void FAR PASCAL ClearRound5Marks(void);
int FAR *FAR PASCAL AllocRound5Mark(void);
int FAR PASCAL HookRound5(int n, SPRITE *s, void *pm, int fReady);
SPRITE *FAR PASCAL AddRound5Sprite(IMAGE *pimg, IMAGE *pmask, int x, int y);
void FAR PASCAL SetRound5Move(SPRITE *s, int id, R5HOOK lpfn, int FAR *pMark);
void FAR PASCAL SetRound5Frame(SPRITE *s, IMAGE *pimg, int nFrame, int nFrames, IMAGE *pmask);
void FAR PASCAL MoveRound5Player(SPRITE *s);
void FAR PASCAL AddRound5Player(void);
void FAR PASCAL MoveRound5Part(SPRITE *s);
void FAR PASCAL MovedRound5Part(SPRITE *s);
void FAR PASCAL DropRound5Part(SPRITE *s);
int FAR PASCAL PickRound5Color(void);
void FAR PASCAL SpawnRound5Part(void);
void FAR PASCAL LayRound5(void);
void FAR PASCAL SetupRound5(void);
void FAR PASCAL BackRound5(IMAGE *pimg, int n, RECT *prc);

/* Column x positions, score-sprite offsets, level rows, the mark pool,
   and the move scripts. */
// GLOBAL: AARCADE 0024:1220
int g_anR5ColX[9] = {
	0, 39, 100, 161, 222, 283, 344, 405, 444
};

// GLOBAL: AARCADE 0024:1232
int g_anR5Off[3] = {
	16, 8, 6
};

// GLOBAL: AARCADE 0024:1238
int g_anR5Droids[10] = {
	2, 3, 4, 5, 5, 6, 7, 8, 9, 10
};

// GLOBAL: AARCADE 0024:124C
int g_anR5Odds[10] = {
	5, 5, 4, 4, 3, 3, 3, 3, 4, 4
};

// GLOBAL: AARCADE 0024:1260
int g_anR5Vy[10] = {
	2, 3, 4, 5, 6, 7, 8, 8, 9, 9
};

// GLOBAL: AARCADE 0024:1274
int g_anR5BugVy[10] = {
	4, 5, 6, 9, 9, 9, 10, 10, 11, 11
};

// GLOBAL: AARCADE 0024:1288
int g_anR5Delay[10] = {
	30, 26, 22, 20, 18, 16, 14, 14, 12, 12
};

// GLOBAL: AARCADE 0024:129C
char g_r5CmdIdle[8] = {
	0x00, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:12A4
int g_anR5Mark[8] = {
	-1, -1, -1, -1, -1, -1, -1, -1
};

// GLOBAL: AARCADE 0024:12B4
char g_r5CmdClear2[194] = {
	0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x0B, 0x04, 0x86, 0x3E, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02,
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x04,
	0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02,
	0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00,
	0x06, 0x02, 0x0C, 0x04, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x04,
	0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02,
	0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x09, 0x04, 0x0A, 0x00, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04,
	0x00, 0x00, 0x09, 0x04, 0x01, 0x00, 0x04, 0x08, 0x06, 0x00, 0x00, 0x00, 0x47, 0x00, 0x09, 0x04,
	0x0A, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0x04, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04,
	0x06, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0xFA, 0xFF, 0x0C, 0x04, 0xFC, 0xFF, 0x09, 0x04,
	0x01, 0x00, 0x04, 0x08, 0x08, 0x00, 0x20, 0x00, 0x00, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02,
	0x06, 0x02, 0x06, 0x02, 0x04, 0x08, 0x78, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x0C, 0x04, 0xFE, 0xFF,
	0x0E, 0x02
};

// GLOBAL: AARCADE 0024:1376
char g_r5CmdClear1[66] = {
	0x00, 0x02, 0x0C, 0x04, 0x01, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x0C, 0x04,
	0x02, 0x00, 0x04, 0x08, 0x06, 0x00, 0x00, 0x00, 0x47, 0x00, 0x0C, 0x04, 0x03, 0x00, 0x01, 0x02,
	0x0C, 0x04, 0x07, 0x00, 0x04, 0x08, 0x09, 0x00, 0x24, 0x00, 0x00, 0x00, 0x06, 0x02, 0x06, 0x02,
	0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x02, 0x02, 0x04, 0x08, 0x77, 0x00, 0xDC, 0x01, 0x00, 0x00,
	0x0E, 0x02
};

// GLOBAL: AARCADE 0024:13B8
char g_r5CmdClear0[38] = {
	0x00, 0x02, 0x0C, 0x04, 0x02, 0x00, 0x04, 0x08, 0x06, 0x00, 0x00, 0x00, 0x47, 0x00, 0x0C, 0x04,
	0x07, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x04, 0x08, 0x80, 0x00,
	0x00, 0x02, 0x00, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:13DE
char g_r5CmdBinClear[26] = {
	0x00, 0x02, 0x0C, 0x04, 0x02, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x88, 0x3E, 0x09, 0x04, 0x0A, 0x00,
	0x07, 0x02, 0x0B, 0x04, 0x89, 0x3E, 0x0C, 0x04, 0xFF, 0xFF
};

// GLOBAL: AARCADE 0024:13F8
char g_r5CmdScoreA[58] = {
	0x00, 0x02, 0x0C, 0x04, 0x05, 0x00, 0x02, 0x02, 0x0B, 0x04, 0x87, 0x3E, 0x04, 0x08, 0x05, 0x00,
	0x00, 0x00, 0xF6, 0xFF, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04, 0x07, 0x00, 0x09, 0x04, 0x01, 0x00,
	0x04, 0x08, 0x08, 0x00, 0x20, 0x00, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
	0x04, 0x08, 0x78, 0x00, 0xE0, 0x01, 0x00, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:1432
char g_r5CmdScoreB[36] = {
	0x00, 0x02, 0x0C, 0x04, 0x05, 0x00, 0x02, 0x02, 0x04, 0x08, 0x05, 0x00, 0x00, 0x00, 0xF6, 0xFF,
	0x0C, 0x04, 0x07, 0x00, 0x09, 0x04, 0x01, 0x00, 0x04, 0x08, 0x08, 0x00, 0x20, 0x00, 0x00, 0x00,
	0x01, 0x02, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:1456
char g_r5CmdScoreC[50] = {
	0x00, 0x02, 0x0C, 0x04, 0x03, 0x00, 0x02, 0x02, 0x09, 0x04, 0x04, 0x00, 0x06, 0x02, 0x09, 0x04,
	0x04, 0x00, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x06, 0x02, 0x0C, 0x04, 0x00, 0x00, 0x0C, 0x04,
	0x07, 0x00, 0x09, 0x04, 0x01, 0x00, 0x04, 0x08, 0x09, 0x00, 0x24, 0x00, 0x00, 0x00, 0x01, 0x02,
	0x0E, 0x02
};

// GLOBAL: AARCADE 0024:1488
char g_r5CmdBinBad0[42] = {
	0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x88, 0x3E, 0x0C, 0x04, 0x00, 0x00,
	0x09, 0x04, 0x0A, 0x00, 0x07, 0x02, 0x0B, 0x04, 0x89, 0x3E, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04,
	0xFA, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04, 0xFF, 0xFF
};

// GLOBAL: AARCADE 0024:14B2
char g_r5CmdBinBad1[50] = {
	0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x88, 0x3E, 0x0C, 0x04, 0x00, 0x00,
	0x09, 0x04, 0x01, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x0A, 0x00, 0x07, 0x02, 0x0B, 0x04,
	0x89, 0x3E, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04, 0xFA, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04,
	0xFF, 0xFF
};

// GLOBAL: AARCADE 0024:14E4
char g_r5CmdBinBad2[58] = {
	0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x06, 0x02, 0x0B, 0x04, 0x88, 0x3E, 0x0C, 0x04, 0x00, 0x00,
	0x09, 0x04, 0x01, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x09, 0x04, 0x01, 0x00, 0x0C, 0x04, 0x00, 0x00,
	0x09, 0x04, 0x0A, 0x00, 0x07, 0x02, 0x0B, 0x04, 0x89, 0x3E, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04,
	0xFA, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04, 0xFF, 0xFF
};

// GLOBAL: AARCADE 0024:151E
char g_r5CmdBad2[16] = {
	0x00, 0x02, 0x0C, 0x04, 0x03, 0x00, 0x04, 0x08, 0x05, 0x00, 0x00, 0x00, 0x87, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:152E
char g_r5CmdBad1[16] = {
	0x00, 0x02, 0x0C, 0x04, 0x02, 0x00, 0x04, 0x08, 0x05, 0x00, 0x00, 0x00, 0x87, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:153E
char g_r5CmdBad0[16] = {
	0x00, 0x02, 0x0C, 0x04, 0x01, 0x00, 0x04, 0x08, 0x05, 0x00, 0x00, 0x00, 0x87, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:154E
char g_r5CmdBug2[46] = {
	0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0B, 0x04, 0x85, 0x3E, 0x04, 0x08,
	0x01, 0x00, 0x00, 0x00, 0x15, 0x00, 0x09, 0x04, 0x08, 0x00, 0x0C, 0x04, 0xFB, 0xFF, 0x0C, 0x04,
	0xFA, 0xFF, 0x0C, 0x04, 0xFD, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04, 0xFF, 0xFF
};

// GLOBAL: AARCADE 0024:157C
char g_r5CmdBug3[66] = {
	0x00, 0x02, 0x09, 0x04, 0x0A, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0B, 0x04, 0x85, 0x3E, 0x04, 0x08,
	0x01, 0x00, 0x00, 0x00, 0x09, 0x00, 0x09, 0x04, 0x0A, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x0B, 0x04,
	0x85, 0x3E, 0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0x15, 0x00, 0x09, 0x04, 0x08, 0x00, 0x0C, 0x04,
	0xFB, 0xFF, 0x0C, 0x04, 0xFA, 0xFF, 0x0C, 0x04, 0xFD, 0xFF, 0x0C, 0x04, 0xFE, 0xFF, 0x0C, 0x04,
	0xFF, 0xFF
};

// GLOBAL: AARCADE 0024:15BE
char g_r5CmdSink0[10] = {
	0x00, 0x02, 0x0C, 0x04, 0x01, 0x00, 0x01, 0x02, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:15C8
char g_r5CmdSink1[10] = {
	0x00, 0x02, 0x0C, 0x04, 0x02, 0x00, 0x01, 0x02, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:15D2
char g_r5CmdRollL[26] = {
	0x00, 0x02, 0x09, 0x04, 0x08, 0x00, 0x04, 0x08, 0x04, 0x00, 0xE2, 0xFF, 0xF6, 0xFF, 0x04, 0x08,
	0x04, 0x00, 0xE2, 0xFF, 0x0A, 0x00, 0x0C, 0x04, 0xF9, 0xFF
};

// GLOBAL: AARCADE 0024:15EC
char g_r5CmdRollR[26] = {
	0x00, 0x02, 0x09, 0x04, 0x08, 0x00, 0x04, 0x08, 0x04, 0x00, 0x1E, 0x00, 0xF6, 0xFF, 0x04, 0x08,
	0x04, 0x00, 0x1E, 0x00, 0x0A, 0x00, 0x0C, 0x04, 0xF9, 0xFF
};

// GLOBAL: AARCADE 0024:1606
char g_r5CmdLoop[22] = {
	0x00, 0x02, 0x09, 0x04, 0x03, 0x00, 0x06, 0x02, 0x09, 0x04, 0x03, 0x00, 0x06, 0x02, 0x09, 0x04,
	0x03, 0x00, 0x06, 0x02, 0x0A, 0x00
};

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:4592
IMAGE g_r5Img[97];


// GLOBAL: AARCADE 0024:4568
SCENE g_r5Scene;


// GLOBAL: AARCADE 0024:455A
SPRITE *g_apR5AsmBin[7];


// GLOBAL: AARCADE 0024:4548
int g_anR5Floor[9];


// GLOBAL: AARCADE 0024:4512
SPRITE *g_apR5Stack[9][3];


// GLOBAL: AARCADE 0024:4500
int g_anR5Depth[9];


// GLOBAL: AARCADE 0024:44FE
SPRITE *g_pR5Before;


// GLOBAL: AARCADE 0024:44FC
SPRITE *g_pR5Part;


// GLOBAL: AARCADE 0024:44FA
int g_nR5Col;


// GLOBAL: AARCADE 0024:44F8
int g_nR5Active;


// GLOBAL: AARCADE 0024:44F4
unsigned long g_dwR5Next;


// GLOBAL: AARCADE 0024:44F2
int g_nR5PlayerX;


// GLOBAL: AARCADE 0024:44F0
int g_fR5Drop;


// GLOBAL: AARCADE 0024:44EE
int g_nR5BugBins;


// GLOBAL: AARCADE 0024:44EC
int g_nR5NextCol;


// GLOBAL: AARCADE 0024:44EA
SPRITE *g_pR5Player;


// GLOBAL: AARCADE 0024:44E8
int g_nR5SavedX;


// GLOBAL: AARCADE 0024:44E6
int g_nR5CursorX;


// GLOBAL: AARCADE 0024:44E4
int g_nR5PlayerCol;


// GLOBAL: AARCADE 0024:44E2
int g_nR5Vy;


// GLOBAL: AARCADE 0024:44E0
int g_nR5BugVy;


// GLOBAL: AARCADE 0024:44DE
int g_nR5Odds;


// GLOBAL: AARCADE 0024:44DC
int g_nR5Delay;


/* Status bar: three colours still wanted (NN-NN-NN), then the bombs left. */
// GLOBAL: AARCADE 0024:44D4
int g_anR5Count[4];


/* Pieces of each colour dropped off the left edge; PickRound5Color discounts them. */
// GLOBAL: AARCADE 0024:44CE
int g_anR5Recycled[3];


/* Round 5. Pieces fall into nine columns; columns 1-7 stack three under a lid. */
// GLOBAL: AARCADE 0024:44CA
unsigned long g_dwR5Tick;

// FUNCTION: AARCADE 001F:0000
void FAR PASCAL ClearRound5Marks(void)
{
	int i;

	for (i = 0; i < 8; i++)
		g_anR5Mark[i] = -1;
}

// FUNCTION: AARCADE 001F:001B
int FAR *FAR PASCAL AllocRound5Mark(void)
{
	int *p;

	for (p = g_anR5Mark; *p >= 0; p++)
		;
	*p = 0;
	return (int FAR *)p;
}

// FUNCTION: AARCADE 001F:003B
int FAR PASCAL HookRound5(int n, SPRITE *s, void *pm, int fReady)
{
	R5POS *p;

	p = pm;
	if (n < 0) {
		/* case bodies in the original's address order */
		switch (n) {
		case -7:
			*(int *)p->pMark = -1;
			s->lpfnMoveEx = 0;
			DropRound5Part(s);
			break;
		case -5:
			if (g_nPlayWave > 0) {
				g_nR5Active--;
				g_dwR5Next = GetTickCount() / 17;
				g_pR5Part = 0;
			}
			break;
		case -6:
			if (g_nPlayWave == 0) {
				g_nR5Active--;
				g_dwR5Next = GetTickCount() / 17;
				g_pR5Part = 0;
			}
			break;
		case -4:
			AddR5Count(-1, 3);
			break;
		case -3:
			g_nR5BugBins++;
			break;
		case -2:
			*(int *)p->pMark = -1;
			break;
		case -1:
			SetRound5Move(s, (int)g_r5CmdIdle, 0, 0);
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

// FUNCTION: AARCADE 001F:0149
SPRITE *FAR PASCAL AddRound5Sprite(IMAGE *pimg, IMAGE *pmask, int x, int y)
{
	SPRITE *s;

	if ((s = AddSprite(&g_r5Scene)) != 0) {
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

// FUNCTION: AARCADE 001F:01BE
void FAR PASCAL SetRound5Move(SPRITE *s, int id, R5HOOK lpfn, int FAR *pMark)
{
	R5POS *p;

	if (s->dwData == 0)
		p = AllocSpriteMove();
	else
		p = (R5POS *)s->dwData;
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

// FUNCTION: AARCADE 001F:0227
void FAR PASCAL SetRound5Frame(SPRITE *s, IMAGE *pimg, int nFrame, int nFrames, IMAGE *pmask)
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

// FUNCTION: AARCADE 001F:0290
void FAR PASCAL MoveRound5Player(SPRITE *s)
{
	register int x;

	_CX = g_nR5CursorX - g_nR5SavedX;
	if ((int)_CX >= 0 && (int)_CX > 0x20)
		_CX = 0x20;
	else if ((int)_CX < 0 && (int)_CX < -0x20)
		_CX = -0x20;
	x = s->cur.rc.left + _CX;
	if (x < 0) {
		_CX += -x;
		x = 0;
		g_nR5SavedX = g_nR5CursorX;
	} else if (0x200 - g_r5Img[79].rc.right < x) {
		_CX -= x - (0x200 - g_r5Img[79].rc.right);
		x = 0x200 - g_r5Img[79].rc.right;
		g_nR5SavedX = g_nR5CursorX;
	} else
		g_nR5SavedX += (int)_CX;
	g_nR5PlayerX = s->cur.rc.left + _CX;
	s->dx = _CX;
}

// FUNCTION: AARCADE 001F:0316
void FAR PASCAL AddRound5Player(void)
{
	SPRITE *s;

	s = AddSprite(&g_r5Scene);
	if (s != 0) {
		g_pR5Player = s;
		g_nR5PlayerCol = 4;
		s->nGroup = 1;
		s->cur.x = g_anR5ColX[g_nR5PlayerCol];
		s->cur.y = 0;
		s->cur.img = g_r5Img[79];
		UpdateSpriteRect(s);
		s->nMoveRate = 1;
		s->lpfnMove = MoveRound5Player;
	}
}

// FUNCTION: AARCADE 001F:0375
void FAR PASCAL MoveRound5Part(SPRITE *s)
{
	int nBest;
	int nBestAbs;
	int d;
	int i;
	int a;

	s->dx = 0;
	if (s->cur.rc.bottom < 0xCB) {
		s->dx = g_nR5PlayerX + 0x13 - s->cur.x;
		return;
	}
	if (g_nR5Col == -1) {
		nBest = 0x7D00;
		nBestAbs = 0x7D00;
		for (i = 0; i < 9; i++) {
			d = s->cur.x + -0x13 - g_anR5ColX[i];
			a = d;
			if (a < 0)
				a = -a;
			if (a < nBestAbs) {
				nBestAbs = a;
				nBest = d;
				g_nR5Col = i;
			}
		}
	}
	s->dx = g_anR5ColX[g_nR5Col] - s->cur.x + 0x13;
	if (s->cur.rc.bottom + s->dy > g_anR5Floor[g_nR5Col])
		s->dy -= s->cur.rc.bottom + s->dy - g_anR5Floor[g_nR5Col];
	else if (s->dy < 8) {
		if (s->dx < 0) {
			if (s->dx < 4)
				s->dx = -4;
		} else if (s->dx > 4)
			s->dx = 4;
	}
}

// FUNCTION: AARCADE 001F:044C
void FAR PASCAL MovedRound5Part(SPRITE *s)
{
	int i;
	SPRITE **pStack;
	int fNext;

	if (g_nR5Col == -1)
		return;
	if (g_anR5Floor[g_nR5Col] > s->cur.rc.bottom)
		return;
	pStack = g_apR5Stack[g_nR5Col];
	fNext = 1;
	if (pStack[0]->nGroup == 6 && s->nGroup != 6) {
		if (g_anR5Floor[g_nR5Col] == 0x100) {
			s->cur.fVisible = 0;
			s->fDead = 1;
			g_anR5Floor[g_nR5Col] -= 0x15;
			if (g_anR5Depth[g_nR5Col] == 2)
				g_anR5Floor[g_nR5Col] -= 0x15;
			PlaySfx(0x3E85, 0);
			AddPlayScore(-5L);
			AddR5Count(-1, s->unk02);
			goto next;
		} else {
			if (g_fR5Drop)
				s->dy = 0x18;
			else
				s->dy = g_nR5Vy;
			g_anR5Floor[g_nR5Col] = 0x100;
			return;
		}
	}
	s->dx = 0;
	s->dy = 0;
	s->lpfnMove = 0;
	s->lpfnMoved = 0;
	if (g_nR5Col != 0 && g_nR5Col != 8) {
		pStack[g_anR5Depth[g_nR5Col]++] = s;
		g_anR5Floor[g_nR5Col] -= s->cur.img.rc.bottom;
		if (s->nGroup == 6) {
			int n;

			n = g_anR5Depth[g_nR5Col];
			s->nAnimRate = 2;
			if (n == 1) {
				g_anR5Floor[g_nR5Col] = 0xEB;
				g_nR5BugBins++;
			} else if (n == 2) {
				if (pStack[0]->nGroup == 6) {
					g_anR5Floor[g_nR5Col] = 0xD6;
					pStack[0]->nAnimRate = 0;
				} else {
					int FAR *pMark;

					pMark = AllocRound5Mark();
					fNext = 0;
					SetRound5Move(s, (int)g_r5CmdBug2, HookRound5, pMark);
					SetRound5Move(pStack[0], (int)g_r5CmdSink0, HookRound5, pMark);
					pStack[0] = s;
					pStack[1] = 0;
					g_anR5Depth[g_nR5Col] = 1;
					g_anR5Floor[g_nR5Col] = 0xEB;
					AddPlayScore(-5L);
				}
			} else {
				int FAR *pMark;

				pMark = AllocRound5Mark();
				fNext = 0;
				if (pStack[1]->nGroup == 6) {
					if (g_nR5Col == 1) {
						g_nR5NextCol = 2;
						s->unk02 = 1;
					} else if (g_nR5Col == 7) {
						g_nR5NextCol = 6;
						s->unk02 = -1;
					} else if (s->unk02 == 0) {
						if (rand() < 0) {
							g_nR5NextCol = g_nR5Col - 1;
							s->unk02 = -1;
						} else {
							g_nR5NextCol = g_nR5Col + 1;
							s->unk02 = 1;
						}
					} else
						g_nR5NextCol = g_nR5Col + s->unk02;
					if (g_nR5NextCol < g_nR5Col)
						SetRound5Move(s, (int)g_r5CmdRollL, HookRound5, pMark);
					else
						SetRound5Move(s, (int)g_r5CmdRollR, HookRound5, pMark);
					g_anR5Floor[g_nR5Col] = 0xD6;
					g_anR5Depth[g_nR5Col]--;
					g_nR5Col = g_nR5NextCol;
					pStack[2] = 0;
				} else {
					SetRound5Move(s, (int)g_r5CmdBug3, HookRound5, pMark);
					SetRound5Move(pStack[0], (int)g_r5CmdSink1, HookRound5, pMark);
					SetRound5Move(pStack[1], (int)g_r5CmdSink0, HookRound5, pMark);
					pStack[0] = s;
					pStack[1] = 0;
					pStack[2] = 0;
					g_anR5Depth[g_nR5Col] = 1;
					g_anR5Floor[g_nR5Col] = 0xEB;
					AddPlayScore(-10L);
				}
			}
		} else {
			AddR5Count(-1, s->unk02);
			for (i = 0; i < 3; i++) {
				SPRITE *p;

				p = pStack[i];
				if (p != 0) {
					if (p->unk02 != 2 - i) {
						int FAR *pMark;

						pMark = AllocRound5Mark();
						PlaySfx(0x3E83, 0);
						SetRound5Move(pStack[0], (int)g_r5CmdBad0, HookRound5, pMark);
						if ((p = pStack[1]) != 0) {
							SetRound5Move(p, (int)g_r5CmdBad1, HookRound5, pMark);
							if ((p = pStack[2]) != 0)
								SetRound5Move(p, (int)g_r5CmdBad2, HookRound5, pMark);
						}
						if (i == 0) {
							AddPlayScore(-10L);
							SetRound5Move(g_apR5AsmBin[g_nR5Col - 1], (int)g_r5CmdBinBad0, HookRound5, pMark);
						} else if (i == 1) {
							AddPlayScore(-20L);
							SetRound5Move(g_apR5AsmBin[g_nR5Col - 1], (int)g_r5CmdBinBad1, HookRound5, pMark);
						} else if (i == 2) {
							AddPlayScore(-30L);
							SetRound5Move(g_apR5AsmBin[g_nR5Col - 1], (int)g_r5CmdBinBad2, HookRound5, pMark);
						}
						fNext = 0;
						break;
					} else if (i + 1 == 3) {
						SPRITE *q;
						SPRITE *t;
						IMAGE *pimg;
						int n;
						int FAR *pMark;

						q = pStack[0];
						pMark = AllocRound5Mark();
						PlaySfx(0x3E81, 0);
						if (q->nGroup == pStack[1]->nGroup && q->nGroup == pStack[2]->nGroup)
							AddPlayScore(50L);
						else
							AddPlayScore(30L);
						SetRound5Move(q, (int)g_r5CmdClear0, HookRound5, pMark);
						SetRound5Move(pStack[1], (int)g_r5CmdClear1, HookRound5, pMark);
						SetRound5Move(pStack[2], (int)g_r5CmdClear2, HookRound5, pMark);
						SetRound5Move(g_apR5AsmBin[g_nR5Col - 1], (int)g_r5CmdBinClear, HookRound5, pMark);
						t = AddRound5Sprite(&g_r5Img[61] + pStack[2]->nGroup, 0, q->cur.rc.left, 0x11C);
						SetRound5Move(t, (int)g_r5CmdScoreA, HookRound5, pMark);
						t->nDrawMode = 1;
						t->cur.fVisible = 0;
						n = pStack[2]->nGroup - 3;
						t = AddRound5Sprite(&g_r5Img[64] + n, 0, q->cur.rc.left + g_anR5Off[n], 0x11C);
						SetRound5Move(t, (int)g_r5CmdScoreB, HookRound5, pMark);
						t->nDrawMode = 1;
						t->cur.fVisible = 0;
						pimg = &g_r5Img[48] + ((pStack[1]->nGroup - 3) << 2);
						t = AddRound5Sprite(pimg, &g_r5Img[60], q->cur.rc.left - 4, 0x12A);
						SetRound5Move(t, (int)g_r5CmdScoreC, HookRound5, pMark);
						t->pImages = pimg;
						t->pMasks = &g_r5Img[60];
						t->nFrame = 0;
						t->nFrames = 4;
						t->nDrawMode = -1;
						t->cur.fVisible = 0;
						fNext = 0;
					}
				}
			}
			if (fNext)
				AddPlayScore(5L);
			else {
				pStack[0] = 0;
				pStack[1] = 0;
				pStack[2] = 0;
				g_anR5Depth[g_nR5Col] = 0;
				g_anR5Floor[g_nR5Col] = 0x100;
			}
		}
	} else {
		int id;
		int nPts;

		s->cur.fVisible = 0;
		s->fDead = 1;
		if (g_nR5Col == 0) {
			if (s->nGroup == 6) {
				id = 0x3E85;
				nPts = -5;
				AddR5Count(-1, 0);
				AddR5Count(-1, 1);
				AddR5Count(-1, 2);
			} else {
				id = 0x3E82;
				nPts = -2;
				g_anR5Recycled[s->unk02]++;
			}
		} else if (s->nGroup == 6) {
			id = 0x3E84;
			nPts = 5;
		} else {
			id = 0x3E85;
			nPts = -5;
			AddR5Count(-1, s->unk02);
		}
		PlaySfx(id, 0);
		AddPlayScore((long)nPts);
	}
	if (fNext) {
next:
		g_nR5Active--;
		g_dwR5Next = GetTickCount() / 17 + g_nR5Delay;
		g_pR5Part = 0;
		if (s->nGroup != 6)
			PlaySfx(0x3E81, 0);
	}
}

// FUNCTION: AARCADE 001F:0B9C
void FAR PASCAL DropRound5Part(SPRITE *s)
{
	s->nMoveRate = 1;
	s->dy = 8;
	s->lpfnMove = MoveRound5Part;
	s->lpfnMoved = MovedRound5Part;
}

// FUNCTION: AARCADE 001F:0BC8
int FAR PASCAL PickRound5Color(void)
{
	int a, b, c, n;
	int i;

	for (i = 0; i < 2; i++) {
		a = g_anR5Count[0] - g_anR5Recycled[0];
		b = g_anR5Count[1] - g_anR5Recycled[1];
		c = g_anR5Count[2] - g_anR5Recycled[2];
		n = a + b + c;
		if (n != 0)
			break;
		g_anR5Recycled[0] = 0;
		g_anR5Recycled[1] = 0;
		g_anR5Recycled[2] = 0;
	}
	n = (unsigned)rand() % n;
	if (n < a)
		return 0;
	n -= a;
	if (n < b)
		return 1;
	return 2;
}

// FUNCTION: AARCADE 001F:0C49
void FAR PASCAL SpawnRound5Part(void)
{
	SPRITE *s;

	s = InsertSprite(&g_r5Scene, g_pR5Before);
	if (s == 0)
		return;
	g_pR5Part = s;
	/* n on the then-block: it keeps its jmp past the else after the merged dy store */
	if ((unsigned)rand() % g_nR5Odds != g_nR5Odds >> 1) {
		int n;

		s->nGroup = (rand() & 0x7FFF) % 3 + 3;
		s->unk02 = PickRound5Color();
		n = (s->nGroup - 3) * 8 + s->unk02;
		s->pImages = &g_r5Img[n];
		s->cur.img = *s->pImages;
		s->pMasks = &g_r5Img[24] + n;
		s->cur.mask = *s->pMasks;
		s->nFrame = 0;
		s->nFrames = 8;
		s->nDrawMode = -1;
		s->cur.x = g_pR5Player->cur.x + 0x13;
		s->cur.y = g_pR5Player->cur.img.rc.bottom - s->cur.img.rc.bottom + 4;
		s->dy = g_nR5Vy;
	} else {
		s->nGroup = 6;
		s->pImages = &g_r5Img[67];
		s->cur.img = *s->pImages;
		s->pMasks = &g_r5Img[73];
		s->cur.mask = *s->pMasks;
		s->nFrame = 0;
		s->nFrames = 6;
		s->nAnimRate = 1;
		s->nDrawMode = -1;
		s->cur.x = g_pR5Player->cur.x + 0x13;
		s->cur.y = g_pR5Player->cur.img.rc.bottom - s->cur.img.rc.bottom + 4;
		s->dy = g_nR5BugVy;
	}
	UpdateSpriteRect(s);
	s->nMoveRate = 1;
	s->lpfnMove = MoveRound5Part;
	s->lpfnMoved = MovedRound5Part;
	g_nR5Col = -1;
	g_nR5Active++;
	g_fR5Drop = 0;
	PlaySfx(0x3E80, 1);
}

// FUNCTION: AARCADE 001F:0DBA
void FAR PASCAL LayRound5(void)
{
	SPRITE *s;
	int i;

	g_pR5Before = 0;
	AddRound5Player();
	s = AddRound5Sprite(&g_r5Img[84], &g_r5Img[85], 0, 0xA4);
	s = AddRound5Sprite(&g_r5Img[87], &g_r5Img[88], 0x1C6, 0xA3);
	s = AddRound5Sprite(&g_r5Img[90], &g_r5Img[93], 0xC, 0xD6);
	SetRound5Move(s, (int)g_r5CmdLoop, 0, 0);
	SetRound5Frame(s, &g_r5Img[90], 0, 3, &g_r5Img[93]);
	s->nAnimMode = 1;
	for (i = 0; i < 6; i++)
		s = AddRound5Sprite(&g_r5Img[82], &g_r5Img[83], i * 0x3D + 0x58, 0xCB);
	for (i = 0; i < 7; i++) {
		s = AddRound5Sprite(&g_r5Img[80], 0, i * 0x3D + 0x22, 0xFF);
		if (g_pR5Before == 0)
			g_pR5Before = s;
		SetRound5Frame(s, &g_r5Img[80], 0, 2, 0);
		s->nGroup = 2;
		s->unk02 = i;
		g_apR5AsmBin[i] = s;
	}
	s = AddRound5Sprite(&g_r5Img[86], 0, 0xC, 0xB0);
	s = AddRound5Sprite(&g_r5Img[89], 0, 0x1D3, 0xB1);
}

// FUNCTION: AARCADE 001F:0EB2
void FAR PASCAL SetupRound5(void)
{
	int n;
	int k;

	n = g_nPlayWave;
	if (n >= 10)
		n = 9;
	if (n == 0)
		g_anR5Count[3] = g_anR5Droids[n];
	else
		AddR5Count(g_anR5Droids[n], 3);
	g_nR5Vy = g_anR5Vy[n];
	g_nR5BugVy = g_anR5BugVy[n];
	g_nR5Odds = g_anR5Odds[n];
	g_nR5Delay = g_anR5Delay[n];
	k = g_anR5Count[3] + 1;
	if (n == 0) {
		g_anR5Count[0] += k;
		g_anR5Count[1] += k;
		g_anR5Count[2] += k;
	} else {
		if (g_nPlayWave >= 5)
			k--;
		AddR5Count(k, 0);
		AddR5Count(k, 1);
		AddR5Count(k, 2);
	}
	g_anR5Recycled[0] = 0;
	g_anR5Recycled[1] = 0;
	g_anR5Recycled[2] = 0;
	g_dwR5Next = 0;
}

// FUNCTION: AARCADE 001F:0F6B
void FAR PASCAL BackRound5(IMAGE *pimg, int n, RECT *prc)
{
	FillImage(pimg, prc, g_hGdiObjects[4]);
}

// FUNCTION: AARCADE 001F:0F83
void FAR PASCAL OpenRound5(HWND hwnd)
{
	int i;
	int *p;

	for (i = 0; i < 9; i++) {
		p = (int *)g_apR5Stack[i];
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		g_anR5Depth[i] = 0;
		g_anR5Floor[i] = 0x100;
		if (i == 0)
			g_anR5Floor[i] -= 0x11;
	}
	g_anR5Count[0] = 0;
	g_anR5Count[1] = 0;
	g_anR5Count[2] = 0;
	g_anR5Count[3] = 0;
	LoadPPicImages(0x3E80, 0, 0x61, g_r5Img, g_nDepth);
	if (g_fColor)
		InitScene(hwnd, &g_r5Scene, (SCENEBKPROC)BackRound5, &g_rcPlay, g_nDepth2, 3, 0);
	else
		InitScene(hwnd, &g_r5Scene, 0, &g_rcPlay, 1, 3, 0);
	ClearRound5Marks();
	SetupRound5();
	LayRound5();
	SetTimeLeft(0L);
	g_nR5BugBins = 0;
	PreloadSounds(0x3E80, 0xB);
	g_nR5Active = 0;
	g_pR5Part = 0;
}

// FUNCTION: AARCADE 001F:1073
void FAR PASCAL ClickRound5(void)
{
	if (g_pR5Part != 0) {
		g_pR5Part->dy = 0x18;
		g_fR5Drop = 1;
	}
}

// FUNCTION: AARCADE 001F:1090
int FAR PASCAL TickRound5(void)
{
	int i;
	SPRITE *p;

	if (g_anR5Count[3] == 0 && g_pR5Part == 0) {
		RunStepDialog();
		NextPlayWave();
		SetupRound5();
	}
	if (g_dwR5Next == 0) {
		g_dwR5Next = GetTickCount() / 17 + g_nR5Delay;
		g_nR5SavedX = g_nR5CursorX = LOWORD(GetMessagePos()) * 2;
	}
	g_nR5CursorX = LOWORD(GetMessagePos()) * 2;
	if (GetTickCount() / 17 >= g_dwR5Tick) {
		if (g_nR5Active == 0 && GetTickCount() / 17 >= g_dwR5Next)
			SpawnRound5Part();
		g_dwR5Tick = GetTickCount() / 17 + 3;
		TickScene(&g_r5Scene);
		if (g_pR5Part == 0) {
			if (g_anR5Count[3] == 0)
				return 0;
			for (i = 0; i < 3; i++) {
				if (g_anR5Count[i] == 0) {
					int n;
					SPRITE **pp;

					pp = &g_apR5Stack[1][0];
					n = 21;
					do {
						/* through BX: plain *pp++ loads before the increment */
						_BX = (unsigned)pp++;
						p = *(SPRITE **)_BX;
						if (p != 0 && p->nGroup != 6 && p->unk02 == i)
							break;
					} while (--n);
					if (n == 0)
						return 1;
				}
			}
		}
		return g_nR5BugBins == 7 ? 1 : 0;
		goto done;
	}
	return 0;
done:;
}

// FUNCTION: AARCADE 001F:11D1
void FAR PASCAL PaintRound5(void)
{
	PaintScene(&g_r5Scene);
}

// FUNCTION: AARCADE 001F:11E0
void FAR PASCAL CloseRound5(void)
{
	FreeScene(&g_r5Scene);
	FreeImages(g_r5Img, 0x61, 1);
}
