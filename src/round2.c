/* Round 2: Nerdoids. Keep them out of the computer room with comics. */
#include "aarcade.h"

int cdecl rand(void);

/* Same block AllocSpriteMove returns. */
typedef struct tagR2POS {
	int id;
	int x0;
	int x1;
	int y0;
	int y1;
	int unused[8];
	long lPos;
} R2POS;

SPRITE *FAR PASCAL AddRound2Sprite(IMAGE *pimg, int x, int y, int nMode);
void FAR PASCAL SetRound2Move(SPRITE *s, int id, long lPos);
void FAR PASCAL SetRound2Frame(SPRITE *s, IMAGE *pimg, int nFrame, int nFrames, IMAGE *pmask);
void FAR PASCAL ClearRound2Falls(void);
void FAR PASCAL MoveRound2Fall(SPRITE *s);
void FAR PASCAL TickRound2Fall(SPRITE *s);
void FAR PASCAL HitRound2Fall(SPRITE *s, SPRITE *other, BOOL fFirst);
void FAR PASCAL SpawnRound2Fall(SPRITE *parent);
void FAR PASCAL MoveRound2Nerdoid(SPRITE *s);
void FAR PASCAL TickRound2Nerdoid(SPRITE *s);
void FAR PASCAL HitRound2Nerdoid(SPRITE *s, SPRITE *other, BOOL fFirst);
void FAR PASCAL SetupRound2(void);
void FAR PASCAL SpawnRound2Nerdoid(void);
void FAR PASCAL ReleaseRound2(void);
void FAR PASCAL MoveRound2Player(SPRITE *s);
void FAR PASCAL AddRound2Player(void);
void FAR PASCAL ClearRound2Comic(void);
void FAR PASCAL MoveRound2Comic(SPRITE *s);
void FAR PASCAL TickRound2Comic(SPRITE *s);
void FAR PASCAL HitRound2Comic(SPRITE *s, SPRITE *other, BOOL fFirst);
SPRITE *FAR PASCAL MakeRound2Comic(void);
void FAR PASCAL ThrowRound2Comic(void);
void FAR PASCAL DropRound2Comic(SPRITE *parent);
void FAR PASCAL LayRound2(void);
void FAR PASCAL ArmRound2(void);
void FAR PASCAL BackRound2(IMAGE *pimg, LPRECT lprc);

/* Level rows, sixteen entries each, then the four move scripts. */
// GLOBAL: AARCADE 0024:0F56
int g_anR2Left[16] = {
	5, 6, 7, 8, 12, 12, 12, 12, 10, 10,
	10, 10, 14, 16, 20, 5
};

// GLOBAL: AARCADE 0024:0F76
int g_anR2Max[16] = {
	2, 3, 3, 3, 4, 4, 5, 6, 3, 4,
	5, 6, 4, 5, 6, 1
};

// GLOBAL: AARCADE 0024:0F96
int g_anR2Bonus[16] = {
	30, 35, 40, 45, 30, 35, 40, 45, 35, 40,
	45, 50, 40, 50, 55, 160
};

// GLOBAL: AARCADE 0024:0FB6
int g_anR2Lanes[16] = {
	5, 4, 3, 3, 3, 3, 2, 2, 3, 3,
	3, 3, 2, 2, 2, 4
};

// GLOBAL: AARCADE 0024:0FD6
int g_anR2Pts[16] = {
	10, 10, 10, 10, 12, 12, 12, 12, 15, 15,
	15, 15, 18, 18, 18, 75
};

// GLOBAL: AARCADE 0024:0FF6
int g_anR2Vy[16] = {
	5, 5, 6, 6, 7, 8, 8, 9, 5, 6,
	6, 6, 4, 5, 6, 0
};

// GLOBAL: AARCADE 0024:1016
int g_anR2Dx[16] = {
	0, 0, 0, 0, 0, 0, 0, 0, 5, 3,
	4, 5, 3, 5, 5, 0
};

// GLOBAL: AARCADE 0024:1036
int g_anR2Falls[16] = {
	1, 1, 2, 2, 3, 3, 4, 4, 2, 2,
	3, 3, 3, 4, 4, 0
};

// GLOBAL: AARCADE 0024:1056
int g_anR2ComicVy[16] = {
	10, 10, 10, 11, 11, 12, 12, 13, 13, 14,
	14, 15, 15, 16, 16, 17
};

// GLOBAL: AARCADE 0024:1076
int g_anR2Speed[16] = {
	8, 8, 9, 9, 10, 10, 11, 11, 12, 12,
	13, 13, 14, 14, 15, 15
};

// GLOBAL: AARCADE 0024:1096
char g_r2CmdL[42] = {
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00,
	0x06, 0x02, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x00, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
	0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:10C0
char g_r2CmdExitL[14] = {
	0x05, 0x04, 0x03, 0x00, 0x04, 0x08, 0x01, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:10CE
char g_r2CmdR[42] = {
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00,
	0x06, 0x02, 0x04, 0x08, 0x63, 0x00, 0x63, 0x00, 0x00, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
	0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:10F8
char g_r2CmdExitR[14] = {
	0x05, 0x04, 0x03, 0x00, 0x04, 0x08, 0x01, 0x00, 0xF1, 0xFF, 0x00, 0x00, 0x0A, 0x00
};

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:41EE
IMAGE g_r2Img[29];


// GLOBAL: AARCADE 0024:41C4
SCENE g_r2Scene;


// GLOBAL: AARCADE 0024:41C2
SPRITE *g_pR2Player;


// GLOBAL: AARCADE 0024:41C0
int g_nR2Cursor;


// GLOBAL: AARCADE 0024:41BE
int g_nR2Comic;


// GLOBAL: AARCADE 0024:41BC
int g_nR2Falls;


// GLOBAL: AARCADE 0024:41BA
int g_nR2Active;


// GLOBAL: AARCADE 0024:41B8
int g_nR2Gone;


// GLOBAL: AARCADE 0024:41B6
int g_nR2Won;


// GLOBAL: AARCADE 0024:41B4
int g_nR2DxMax;


// GLOBAL: AARCADE 0024:41B2
int g_nR2FallMax;


// GLOBAL: AARCADE 0024:41B0
int g_nR2ComicVy;


// GLOBAL: AARCADE 0024:41AE
int g_nR2Max;


// GLOBAL: AARCADE 0024:41AC
int g_nR2Left;


// GLOBAL: AARCADE 0024:41AA
int g_nR2Vy;


// GLOBAL: AARCADE 0024:41A8
int g_nR2Lanes;


// GLOBAL: AARCADE 0024:41A6
int g_nR2Pts;


// GLOBAL: AARCADE 0024:41A4
int g_nR2Speed;


/* Round 2. The dword is the next spawn tick; the scene sits under the words. */
// GLOBAL: AARCADE 0024:41A0
unsigned long g_dwR2Next;

// FUNCTION: AARCADE 001C:0000
SPRITE *FAR PASCAL AddRound2Sprite(IMAGE *pimg, int x, int y, int nMode)
{
	SPRITE *s;

	/* test AX, not the copy in SI */
	if ((s = AddSprite(&g_r2Scene)) != 0) {
		s->cur.img = *pimg;
		s->cur.x = x;
		s->cur.y = y;
		UpdateSpriteRect(s);
		s->pImages = pimg;
		s->nDrawMode = nMode;
	}
	return s;
}

// FUNCTION: AARCADE 001C:004E
void FAR PASCAL SetRound2Move(SPRITE *s, int id, long lPos)
{
	R2POS *p;

	/* zero falls into the alloc; the existing offset is the else */
	if (s->dwData == 0)
		p = AllocSpriteMove();
	else
		p = (R2POS *)s->dwData;
	if (p != 0) {
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

// FUNCTION: AARCADE 001C:00AB
void FAR PASCAL SetRound2Frame(SPRITE *s, IMAGE *pimg, int nFrame, int nFrames, IMAGE *pmask)
{
	s->pImages = pimg;
	s->pMasks = pmask;
	s->nFrame = nFrame;
	s->nFrames = nFrames;
	s->cur.img = pimg[nFrame];
	UpdateSpriteRect(s);
}

// FUNCTION: AARCADE 001C:00F4
void FAR PASCAL ClearRound2Falls(void)
{
	g_nR2Falls = 0;
}

// FUNCTION: AARCADE 001C:0101
void FAR PASCAL MoveRound2Fall(SPRITE *s)
{
	if (s->cur.rc.top >= 0x146) {
		s->cur.fVisible = 0;
		s->fDead = 1;
	}
}

// FUNCTION: AARCADE 001C:011E
void FAR PASCAL TickRound2Fall(SPRITE *s)
{
	if (s->fDead != 0)
		g_nR2Falls--;
}

// FUNCTION: AARCADE 001C:0136
void FAR PASCAL HitRound2Fall(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	if (other->nGroup == 1) {
		s->cur.fVisible = 0;
		s->fDead = 1;
		AddPlayAmmo(-5L);
		PlaySfx(0x1770, 1);
	}
}

// FUNCTION: AARCADE 001C:0169
void FAR PASCAL SpawnRound2Fall(SPRITE *parent)
{
	int n;
	SPRITE *s;

	if (g_nR2Falls >= g_nR2FallMax)
		return;
	if (parent->nGroup == 7 || parent->nGroup == 5) {
		s = InsertSprite(&g_r2Scene, g_r2Scene.pFirst);
		if (s != 0) {
			n = rand() & 1;
			s->nGroup = n + 3;
			s->cur.y = parent->cur.rc.bottom;
			s->cur.x = parent->cur.rc.left
				+ (parent->cur.rc.right - parent->cur.rc.left
				- (&g_r2Img[16])[n].rc.right) / 2;
			s->cur.img = (&g_r2Img[16])[n];
			UpdateSpriteRect(s);
			s->dy = g_nR2Vy;
			if (g_nR2DxMax != 0) {
				s->dx = -(s->cur.x - g_pR2Player->cur.x + g_r2Img[18].rc.right / 2);
				s->dx = s->dx / ((g_pR2Player->cur.y - s->cur.y
					+ g_r2Img[18].rc.bottom / 2) / g_nR2Vy);
				if (s->dx > g_nR2DxMax)
					s->dx = g_nR2DxMax;
				else if (s->dx < -g_nR2DxMax)
					s->dx = -g_nR2DxMax;
			}
			s->nMoveRate = 1;
			s->dwHitMask = 0x4000L;
			s->lpfnMoved = MoveRound2Fall;
			s->lpfnHit = HitRound2Fall;
			s->lpfnTick = TickRound2Fall;
			g_nR2Falls++;
			PlaySfx(0x1B5A, 0);
		}
	}
}

// FUNCTION: AARCADE 001C:02B2
void FAR PASCAL MoveRound2Nerdoid(SPRITE *s)
{
	/* false side of the && joins the else */
	if (s->nGroup <= 6 && s->cur.rc.left > 0x200) {
		if (s->nGroup == 6) {
			s->cur.fVisible = 0;
			s->fDead = 1;
		} else if (s->cur.rc.top == 0xEA) {
			g_nR2Won = 1;
			PlaySfx(0x232D, 1);
		} else {
			s->nGroup += 2;
			s->cur.y += 0x2C;
			SetRound2Move(s, (int)g_r2CmdR, 0L);
			SetRound2Frame(s, &g_r2Img[8], 0, 4, 0);
			s->fDirty = 1;
		}
	} else if (s->nGroup >= 7 && s->cur.rc.right < 0) {
		if (s->nGroup == 8) {
			s->cur.fVisible = 0;
			s->fDead = 1;
		} else {
			s->nGroup -= 2;
			s->cur.y += 0x2C;
			SetRound2Move(s, (int)g_r2CmdL, 0L);
			SetRound2Frame(s, &g_r2Img[0], 0, 4, 0);
			s->fDirty = 1;
		}
	}
}

// FUNCTION: AARCADE 001C:035A
void FAR PASCAL TickRound2Nerdoid(SPRITE *s)
{
	if (s->fDead != 0) {
		g_nR2Active--;
		g_nR2Left--;
		g_nR2Gone--;
	} else if (s->cur.rc.left >= 0 && s->cur.rc.right <= 0x200) {
		if ((rand() & 7) == 0)
			SpawnRound2Fall(s);
	}
}

// FUNCTION: AARCADE 001C:0398
void FAR PASCAL HitRound2Nerdoid(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	if (other->nGroup != 2)
		return;
	if (s->nGroup == 5 || s->nGroup == 7) {
		g_nR2Gone++;
		s->nGroup++;
		s->pImages = &g_r2Img[(s->nGroup + -5) << 2];
		s->cur.img = s->pImages[s->nFrame];
		UpdateSpriteRect(s);
		s->fDirty = 1;
		AddPlayScore((long)g_nR2Pts);
		PlaySfx(0x1B59, 0);
	} else {
		g_nR2Gone--;
		s->nGroup--;
		s->pImages = &g_r2Img[(s->nGroup + -5) << 2];
		s->cur.img = s->pImages[s->nFrame];
		UpdateSpriteRect(s);
		s->fDirty = 1;
		PlaySfx(0x1B5B, 0);
		DropRound2Comic(s);
	}
}

// FUNCTION: AARCADE 001C:0460
void FAR PASCAL SetupRound2(void)
{
	char FAR *lp;
	int nFrame;
	int n;
	int nBonus;

	g_nR2Active = 0;
	g_nR2Gone = 0;
	nFrame = g_nPlayWave;
	if (nFrame >= 16)
		nFrame = (g_nPlayWave & 3) + 12;
	g_nR2Left = g_anR2Left[nFrame];
	g_nR2Max = g_anR2Max[nFrame];
	g_nR2Vy = g_anR2Vy[nFrame];
	g_nR2DxMax = g_anR2Dx[nFrame];
	g_nR2FallMax = g_anR2Falls[nFrame];
	g_nR2ComicVy = g_anR2ComicVy[nFrame];
	g_nR2Lanes = g_anR2Lanes[nFrame];
	g_nR2Pts = g_anR2Pts[nFrame];
	g_nR2Speed = g_anR2Speed[nFrame];
	/* the compute stays inline; both failing tests jump to the table load */
	if (g_nPlayWave >= 16 && nFrame != 15) {
		nBonus = g_anR2Bonus[nFrame] + ((g_nPlayWave + -16) / 4 + 1) * 10;
		if (nBonus > 100)
			nBonus = 100;
	} else
		nBonus = g_anR2Bonus[nFrame];
	n = 6;
	lp = g_r2CmdL;
	while (n--)
		lp += lp[1];
	*(int FAR *)(lp + 2) = 4;
	*(int FAR *)(lp + 4) = nBonus;
	n = 6;
	lp = g_r2CmdR;
	while (n--)
		lp += lp[1];
	*(int FAR *)(lp + 2) = 4;
	*(int FAR *)(lp + 4) = -nBonus;
}

// FUNCTION: AARCADE 001C:058F
void FAR PASCAL SpawnRound2Nerdoid(void)
{
	volatile int nPick;
	char ab[4];
	int nSpan;
	int n;
	SPRITE *s;

	if (g_nR2Active - g_nR2Gone >= g_nR2Max)
		return;
	if (g_nR2Left <= g_nR2Active)
		return;
	if (g_nPlayAmmo == 0)
		return;
	*(int *)(ab + 2) = -1;
	*(int *)ab = -1;
	s = g_r2Scene.pFirst;
	nSpan = g_r2Img[3].rc.right * 3;
	while (s != 0) {
		if (s->nGroup >= 5 && s->nGroup <= 8) {
			n = s->cur.rc.top;
			if (s->cur.rc.right <= nSpan) {
				if (n <= 0x92)
					ab[((n + -0x3A) / 0x2C + 1) & ~1] = 0;
			} else if (s->cur.rc.left >= 0x200 - nSpan && n <= 0xBE) {
				ab[(n + -0x3A) / 0x2C | 1] = 0;
			}
		}
		s = s->pNext;
	}
	if ((*(int *)ab | *(int *)(ab + 2)) == 0)
		return;
	n = 5 - g_nR2Lanes;
	/* je onto the while; the break is the jmp past it */
	do
		if (ab[n] != 0)
			break;
	while (n-- != 0);
	if (n < 0)
		return;
	do
		nPick = (rand() & 0x7FFF) % (6 - g_nR2Lanes);
	while (!ab[nPick]);
	if (nPick & 1)
		s = AddRound2Sprite(&g_r2Img[8], 0x200, (nPick & ~1) * 0x2C + 0x66, 0);
	else
		s = AddRound2Sprite(&g_r2Img[0], -0x1E, (nPick & ~1) * 0x2C + 0x3A, 0);
	/* je to the epilogue; a return in front of this body is jne plus jmp */
	if (s != 0) {
		if (nPick & 1) {
			SetRound2Move(s, (int)g_r2CmdR, 0L);
			SetRound2Frame(s, &g_r2Img[8], 0, 4, 0);
			s->nGroup = 7;
		} else {
			SetRound2Move(s, (int)g_r2CmdL, 0L);
			SetRound2Frame(s, &g_r2Img[0], 0, 4, 0);
			s->nGroup = 5;
		}
		s->dwHitMask = 0x8000L;
		s->lpfnMove = MoveRound2Nerdoid;
		s->lpfnTick = TickRound2Nerdoid;
		s->lpfnHit = HitRound2Nerdoid;
		g_nR2Active++;
	}
}

// FUNCTION: AARCADE 001C:073F
void FAR PASCAL ReleaseRound2(void)
{
	SPRITE *s;

	if (g_nR2Left == g_nR2Gone) {
		for (s = g_r2Scene.pFirst; s != 0; s = s->pNext) {
			if (s->nGroup == 6 || s->nGroup == 8) {
				s->dwHitMask = 0;
				if (s->nGroup == 8)
					SetRound2Move(s, (int)g_r2CmdExitR, 0L);
				else
					SetRound2Move(s, (int)g_r2CmdExitL, 0L);
			}
		}
	}
}

// FUNCTION: AARCADE 001C:078A
void FAR PASCAL MoveRound2Player(SPRITE *s)
{
	if (g_nR2Cursor < 0)
		g_nR2Cursor = 0;
	else if (0x200 - g_r2Img[18].rc.right < g_nR2Cursor)
		g_nR2Cursor = 0x200 - g_r2Img[18].rc.right;
	s->dx = g_nR2Cursor - s->cur.rc.left;
	/* false side of the && joins the else */
	if (s->cur.rc.left < g_nR2Cursor && s->dx > g_nR2Speed)
		s->dx = g_nR2Speed;
	else if (s->cur.rc.left > g_nR2Cursor) {
		if (s->dx < -g_nR2Speed)
			s->dx = -g_nR2Speed;
	}
}

// FUNCTION: AARCADE 001C:07FC
void FAR PASCAL AddRound2Player(void)
{
	SPRITE *s;

	g_pR2Player = 0;
	s = AddSprite(&g_r2Scene);
	g_pR2Player = s;
	if (s != 0) {
		s->nGroup = 1;
		s->cur.x = (0x200 - g_r2Img[18].rc.right) / 2;
		s->cur.y = 0x144 - g_r2Img[18].rc.bottom;
		s->cur.img = g_r2Img[18];
		UpdateSpriteRect(s);
		s->dwHitMask = 0xC000L;
		s->nMoveRate = 1;
		s->lpfnMove = MoveRound2Player;
	}
}

// FUNCTION: AARCADE 001C:086C
void FAR PASCAL ClearRound2Comic(void)
{
	g_nR2Comic = 0;
}

// FUNCTION: AARCADE 001C:0879
void FAR PASCAL MoveRound2Comic(SPRITE *s)
{
	if (s->cur.rc.bottom <= 0 || s->cur.rc.top >= 0x146) {
		s->cur.fVisible = 0;
		s->fDead = 1;
	}
}

// FUNCTION: AARCADE 001C:089C
void FAR PASCAL TickRound2Comic(SPRITE *s)
{
	if (s->fDead != 0)
		g_nR2Comic = 0;
}

// FUNCTION: AARCADE 001C:08B6
void FAR PASCAL HitRound2Comic(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	s->cur.fVisible = 0;
	s->fDead = 1;
	if (other->nGroup == 1) {
		AddPlayAmmo(1L);
		PlaySfx(0x1B5C, 0);
	}
}

// FUNCTION: AARCADE 001C:08E9
SPRITE *FAR PASCAL MakeRound2Comic(void)
{
	SPRITE *s;

	s = AddSprite(&g_r2Scene);
	if (s != 0) {
		s->nGroup = 2;
		s->dy = -g_nR2ComicVy;
		s->cur.img = g_r2Img[19];
		s->nMoveRate = 1;
		s->dwHitMask = 0x8000L;
		s->lpfnMoved = MoveRound2Comic;
		s->lpfnHit = HitRound2Comic;
		s->lpfnTick = TickRound2Comic;
		g_nR2Comic = 1;
	}
	return s;
}

// FUNCTION: AARCADE 001C:095A
void FAR PASCAL ThrowRound2Comic(void)
{
	SPRITE *s;
	SPRITE *p;

	if (g_nR2Comic == 0 && g_nPlayAmmo != 0) {
		s = MakeRound2Comic();
		p = g_pR2Player;
		s->cur.y = p->cur.rc.top - g_r2Img[19].rc.bottom - 1;
		s->cur.x = p->cur.rc.left + (g_r2Img[18].rc.right - g_r2Img[19].rc.right) / 2;
		UpdateSpriteRect(s);
		AddPlayAmmo(-1L);
		PlaySfx(0x1B58, 0);
	}
}

// FUNCTION: AARCADE 001C:09B5
void FAR PASCAL DropRound2Comic(SPRITE *parent)
{
	SPRITE *s;

	s = MakeRound2Comic();
	s->dy = -s->dy;
	s->cur.y = parent->cur.rc.bottom + 1;
	s->cur.x = parent->cur.rc.left + g_r2Img[0].rc.right / 2;
	UpdateSpriteRect(s);
}

// FUNCTION: AARCADE 001C:09F0
void FAR PASCAL LayRound2(void)
{
	SPRITE *s;
	{
	int y;

	y = 0;
	while (y < 0x108) {
		s = AddSprite(&g_r2Scene);
		s->cur.x = 0;
		s->cur.y = y + 0x20;
		s->cur.img = g_r2Img[20];
		UpdateSpriteRect(s);
		y += 0x2C;
	}
	}
	{
	int i;

	for (i = 0; i <= 2; i++) {
		s = AddSprite(&g_r2Scene);
		s->cur.x = 4;
		s->cur.y = i * 0x58 + 0x28;
		s->cur.img = (&g_r2Img[22])[i];
		UpdateSpriteRect(s);
		s = AddSprite(&g_r2Scene);
		s->cur.x = 0x1CF;
		s->cur.y = i * 0x58 + 0x54;
		if (i == 2) {
			s->cur.x = 0x1A1;
			s->cur.y -= 0x2C;
		}
		s->cur.img = (&g_r2Img[26])[i];
		UpdateSpriteRect(s);
	}
	}
}

// FUNCTION: AARCADE 001C:0AC6
void FAR PASCAL ArmRound2(void)
{
	SetTimeLeft((long)(g_nPlayWave * 10 + 0x2D));
}

// FUNCTION: AARCADE 001C:0ADE
void FAR PASCAL BackRound2(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[2]);
}

// FUNCTION: AARCADE 001C:0AF6
void FAR PASCAL OpenRound2(HWND hwnd)
{
	g_nR2Won = 0;
	LoadPPicImages(0x1B58, 0, 0x1D, g_r2Img, g_nDepth);
	DecImageBottoms(g_r2Img, 0, 0x0F);
	if (g_fColor)
		InitScene(hwnd, &g_r2Scene, BackRound2, &g_rcPlay, g_nDepth2, 3, 0);
	else
		InitScene(hwnd, &g_r2Scene, 0, &g_rcPlay, 1, 3, 0);
	ArmRound2();
	AddRound2Player();
	ClearRound2Comic();
	SetupRound2();
	ClearRound2Falls();
	LayRound2();
	g_dwR2Next = 0;
	PreloadSounds(0x1770, 1);
	PreloadSounds(0x232D, 1);
	PreloadSounds(0x1B58, 5);
}

// FUNCTION: AARCADE 001C:0B9D
void FAR PASCAL ClickRound2(void)
{
	ThrowRound2Comic();
}

// FUNCTION: AARCADE 001C:0BA8
int FAR PASCAL TickRound2(void)
{
	ReleaseRound2();
	if (g_nR2Comic == 0 && g_nR2Active == 0 && g_nR2Falls == 0 && g_nR2Left == 0) {
		RunStepDialog();
		NextPlayWave();
		ArmRound2();
		SetupRound2();
		SpawnRound2Nerdoid();
	}
	g_nR2Cursor = (int)GetMessagePos();
	if (GetTickCount() / 17 >= g_dwR2Next) {
		SpawnRound2Nerdoid();
		g_dwR2Next = GetTickCount() / 17 + 3;
		TickScene(&g_r2Scene);
	}
	/* both-zero and a live won flag share mov ax,1; the else is the xor */
	if (g_nPlayAmmo == 0 && g_nR2Comic == 0 || g_nR2Won != 0)
		return 1;
	else
		return 0;
}

// FUNCTION: AARCADE 001C:0C54
void FAR PASCAL PaintRound2(void)
{
	PaintScene(&g_r2Scene);
}

// FUNCTION: AARCADE 001C:0C63
void FAR PASCAL CloseRound2(void)
{
	FreeScene(&g_r2Scene);
	FreeImages(g_r2Img, 0x1D, 1);
}
