/* Round 3: Quasar Klutzes. Feed them bananas. */
#include "aarcade.h"

int cdecl rand(void);

typedef int (FAR PASCAL *R3HOOK)(int n, SPRITE *s, void *pm, int fReady);

/* Same block AllocSpriteMove returns. The hook sits where round 2 stores lPos. */
typedef struct tagR3POS {
	int id;
	int x0;
	int x1;
	int y0;
	int y1;
	int unused[8];
	R3HOOK lpfn;
} R3POS;

SPRITE *FAR PASCAL AddRound3Sprite(IMAGE *pimg, int x, int y);
void FAR PASCAL SetRound3Move(SPRITE *s, int id, R3HOOK lpfn);
void FAR PASCAL SetRound3Frame(SPRITE *s, IMAGE *pimg, int nFrame, int nFrames, IMAGE *pmask);
void FAR PASCAL ClearRound3Bolts(void);
void FAR PASCAL MoveRound3Bolt(SPRITE *s);
void FAR PASCAL TickRound3Bolt(SPRITE *s);
void FAR PASCAL HitRound3Bolt(SPRITE *s, SPRITE *other, BOOL fFirst);
void FAR PASCAL SpawnRound3Bolt(SPRITE *parent);
int FAR PASCAL MoveRound3Script(int n, SPRITE *s, void *pm, int fReady);
void FAR PASCAL MoveRound3Klutz(SPRITE *s);
void FAR PASCAL TickRound3Klutz(SPRITE *s);
void FAR PASCAL HitRound3Klutz(SPRITE *s, SPRITE *other, BOOL fFirst);
void FAR PASCAL SetupRound3(void);
void FAR PASCAL SpawnRound3Klutz(void);
void FAR PASCAL MoveRound3Player(SPRITE *s);
void FAR PASCAL AddRound3Player(void);
void FAR PASCAL ClearRound3Banana(void);
void FAR PASCAL MoveRound3Banana(SPRITE *s);
void FAR PASCAL TickRound3Banana(SPRITE *s);
void FAR PASCAL HitRound3Banana(SPRITE *s, SPRITE *other, BOOL fFirst);
void FAR PASCAL ThrowRound3Banana(void);
void FAR PASCAL ClearRound3RobotDogs(void);
void FAR PASCAL MoveRound3RobotDog(SPRITE *s);
void FAR PASCAL TickRound3RobotDog(SPRITE *s);
void FAR PASCAL HitRound3RobotDog(SPRITE *s, SPRITE *other, BOOL fFirst);
void FAR PASCAL SpawnRound3RobotDog(void);
void FAR PASCAL MoveRound3Cloud(SPRITE *s);
void FAR PASCAL TickRound3Cloud(SPRITE *s);
void FAR PASCAL HitRound3Cloud(SPRITE *s, SPRITE *other, BOOL fFirst);
void FAR PASCAL SpawnRound3Cloud(void);
void FAR PASCAL LayRound3(void);
void FAR PASCAL ArmRound3(void);
void FAR PASCAL BackRound3(IMAGE *pimg, LPRECT lprc);

/* Level rows, ten entries each, then the move scripts. */
// GLOBAL: AARCADE 0024:0C1A
int g_anR3Max[10] = {
	6, 6, 6, 8, 8, 8, 10, 10, 10, 10
};

// GLOBAL: AARCADE 0024:0C2E
int g_anR3Left[10] = {
	6, 8, 8, 10, 10, 12, 12, 14, 16, 18
};

// GLOBAL: AARCADE 0024:0C42
int g_anR3BoltVy[10] = {
	5, 5, 5, 6, 6, 7, 7, 8, 8, 9
};

// GLOBAL: AARCADE 0024:0C56
int g_anR3Arm[10] = {
	5, 4, 4, 3, 3, 2, 2, 1, 1, 1
};

// GLOBAL: AARCADE 0024:0C6A
int g_anR3Wave[10] = {
	14, 12, 10, 9, 9, 8, 8, 7, 6, 5
};

// GLOBAL: AARCADE 0024:0C7E
int g_anR3Mod[10] = {
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2
};

// GLOBAL: AARCADE 0024:0C92
int g_anR3DogSpeed[10] = {
	6, 7, 8, 8, 8, 8, 8, 8, 8, 8
};

// GLOBAL: AARCADE 0024:0CA6
int g_anR3CloudMax[10] = {
	4, 4, 4, 5, 5, 5, 5, 5, 6, 6
};

// GLOBAL: AARCADE 0024:0CBA
int g_anR3CloudDx[10] = {
	2, 3, 3, 4, 4, 5, 5, 6, 7, 8
};

// GLOBAL: AARCADE 0024:0CCE
int g_anR3CloudVy[10] = {
	4, 5, 5, 5, 6, 6, 6, 7, 7, 8
};

// GLOBAL: AARCADE 0024:0CE2
int g_anR3CloudPeriod[10] = {
	16, 15, 15, 14, 14, 12, 12, 10, 10, 10
};

// GLOBAL: AARCADE 0024:0CF6
char g_r3CmdL[108] = {
	0x00, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09, 0x04,
	0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00,
	0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
	0x04, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08,
	0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x0C, 0x04, 0x00, 0x00, 0x04, 0x08,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x04, 0x00, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:0D62
char g_r3CmdL2[120] = {
	0x00, 0x02, 0x05, 0x04, 0x07, 0x00, 0x04, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x09, 0x04,
	0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00,
	0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
	0x04, 0x08, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08,
	0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x05, 0x00, 0x04, 0x08,
	0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x0C, 0x04, 0x01, 0x00
};

// GLOBAL: AARCADE 0024:0DDA
char g_r3CmdR[108] = {
	0x00, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFB, 0xFF, 0x00, 0x00,
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFB, 0xFF, 0x00, 0x00, 0x09, 0x04,
	0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00,
	0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFB, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
	0x04, 0x08, 0x00, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08,
	0x00, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x0C, 0x04, 0x02, 0x00, 0x04, 0x08,
	0x00, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x05, 0x04, 0x00, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:0E46
char g_r3CmdR2[120] = {
	0x00, 0x02, 0x05, 0x04, 0x07, 0x00, 0x04, 0x08, 0x00, 0x00, 0xF5, 0xFF, 0x00, 0x00, 0x09, 0x04,
	0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0xFC, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00,
	0x06, 0x02, 0x04, 0x08, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
	0x04, 0x08, 0x00, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08,
	0x00, 0x00, 0xFA, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x05, 0x00, 0x04, 0x08,
	0x00, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x00, 0x00,
	0xF9, 0xFF, 0x00, 0x00, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x00, 0x00, 0x04, 0x08, 0x00, 0x00,
	0xFD, 0xFF, 0x00, 0x00, 0x0C, 0x04, 0x03, 0x00
};

// GLOBAL: AARCADE 0024:0EBE
char g_r3CmdHit[36] = {
	0x05, 0x04, 0x0D, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x0F, 0x00, 0x0B, 0x04,
	0x43, 0x1F, 0x06, 0x02, 0x04, 0x08, 0x0A, 0x00, 0x00, 0x00, 0x40, 0x01, 0x01, 0x02, 0x09, 0x04,
	0x01, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:0EE2
char g_r3CmdA[58] = {
	0x05, 0x04, 0x0C, 0x00, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
	0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x06, 0x02,
	0x09, 0x04, 0x0F, 0x00, 0x0B, 0x04, 0x43, 0x1F, 0x06, 0x02, 0x04, 0x08, 0x0E, 0x00, 0x00, 0x00,
	0x40, 0x01, 0x01, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0E, 0x02
};

// GLOBAL: AARCADE 0024:0F1C
char g_r3CmdB[58] = {
	0x05, 0x04, 0x0C, 0x00, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x00, 0x00, 0xF6, 0xFF, 0x00, 0x00,
	0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x00, 0x00, 0xFB, 0xFF, 0x00, 0x00, 0x06, 0x02,
	0x09, 0x04, 0x0F, 0x00, 0x0B, 0x04, 0x43, 0x1F, 0x06, 0x02, 0x04, 0x08, 0x0E, 0x00, 0x00, 0x00,
	0x40, 0x01, 0x01, 0x02, 0x09, 0x04, 0x01, 0x00, 0x0E, 0x02
};

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:3DA4
IMAGE g_r3Img[85];


// GLOBAL: AARCADE 0024:3DA2
IMAGE *g_pR3Base;


// GLOBAL: AARCADE 0024:3D78
SCENE g_r3Scene;


// GLOBAL: AARCADE 0024:3D76
SPRITE *g_pR3Player;


// GLOBAL: AARCADE 0024:3D74
int g_nR3Cursor;


// GLOBAL: AARCADE 0024:3D72
int g_nR3Bolts;


// GLOBAL: AARCADE 0024:3D70
int g_nR3Active;


// GLOBAL: AARCADE 0024:3D6E
int g_nR3Left;


// GLOBAL: AARCADE 0024:3D6C
int g_nR3Max;


// GLOBAL: AARCADE 0024:3D6A
int g_nR3Cool;


// GLOBAL: AARCADE 0024:3D68
int g_nR3BoltVy;


// GLOBAL: AARCADE 0024:3D66
int g_nR3RobotDogs;


// GLOBAL: AARCADE 0024:3D64
SPRITE *g_pR3RobotDog;


// GLOBAL: AARCADE 0024:3D60
unsigned long g_dwR3Deadline;


// GLOBAL: AARCADE 0024:3D5E
int g_nR3Arm;


// GLOBAL: AARCADE 0024:3D5C
int g_nR3Wave;


// GLOBAL: AARCADE 0024:3D5A
int g_nR3Mod;


// GLOBAL: AARCADE 0024:3D58
int g_nR3DogSpeed;


// GLOBAL: AARCADE 0024:3D56
int g_nR3Clouds;


// GLOBAL: AARCADE 0024:3D54
int g_nR3CloudMax;


// GLOBAL: AARCADE 0024:3D52
int g_nR3CloudDx;


// GLOBAL: AARCADE 0024:3D50
int g_nR3CloudVy;


// GLOBAL: AARCADE 0024:3D4E
int g_nR3CloudPeriod;


// GLOBAL: AARCADE 0024:3D4C
int g_nR3CloudDelay;


// GLOBAL: AARCADE 0024:3D4A
int g_nR3Banana;


/* Round 3. The scene follows the words; the pictures follow the base pointer. */
// GLOBAL: AARCADE 0024:3D46
unsigned long g_dwR3Next;

// FUNCTION: AARCADE 001B:0000
SPRITE *FAR PASCAL AddRound3Sprite(IMAGE *pimg, int x, int y)
{
	SPRITE *s;

	/* test AX, not the copy in SI */
	if ((s = InsertSprite(&g_r3Scene, g_r3Scene.pFirst)) != 0) {
		s->cur.img = *pimg;
		s->cur.x = x;
		s->cur.y = y;
		UpdateSpriteRect(s);
		s->pImages = pimg;
		s->nDrawMode = 0;
	}
	return s;
}

// FUNCTION: AARCADE 001B:0051
void FAR PASCAL SetRound3Move(SPRITE *s, int id, R3HOOK lpfn)
{
	R3POS *p;

	/* zero falls into the alloc; the existing offset is the else */
	if (s->dwData == 0)
		p = AllocSpriteMove();
	else
		p = (R3POS *)s->dwData;
	if (p != 0) {
		s->dwData = p;
		s->lpfnMoveEx = MoveSprite;
		p->id = id;
		p->lpfn = lpfn;
		p->x1 = 0;
		p->x0 = 0;
		p->y0 = 0;
		p->y1 = 0;
	}
}

// FUNCTION: AARCADE 001B:00AE
void FAR PASCAL SetRound3Frame(SPRITE *s, IMAGE *pimg, int nFrame, int nFrames, IMAGE *pmask)
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

// FUNCTION: AARCADE 001B:0117
void FAR PASCAL ClearRound3Bolts(void)
{
	g_nR3Bolts = 0;
}

// FUNCTION: AARCADE 001B:0124
void FAR PASCAL MoveRound3Bolt(SPRITE *s)
{
	if (s->cur.rc.top >= 0x146) {
		s->cur.fVisible = 0;
		s->fDead = 1;
	}
}

// FUNCTION: AARCADE 001B:0141
void FAR PASCAL TickRound3Bolt(SPRITE *s)
{
	if (s->fDead != 0)
		g_nR3Bolts--;
}

// FUNCTION: AARCADE 001B:0159
void FAR PASCAL HitRound3Bolt(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	/* equal groups jump to the epilogue; the body is the nested fall-through */
	if (other->nGroup != 9)
		if (other->nGroup != 2)
			if (other->nGroup != 10) {
				s->cur.fVisible = 0;
				s->fDead = 1;
				s->fDirty = 1;
				if (other->nGroup == 1) {
					PlaySfx(0x1770, 1);
					AddPlayAmmo(-5L);
				}
			}
}

// FUNCTION: AARCADE 001B:019F
void FAR PASCAL SpawnRound3Bolt(SPRITE *parent)
{
	SPRITE *s;
	RECT rc;

	if (g_nR3Bolts >= 10)
		return;
	if (parent->cur.img.rc.right <= 0x18)
		return;
	if (parent->cur.x >= 0x1E8)
		return;
	if (parent->nGroup != 4 && parent->nGroup != 3)
		return;
	s = InsertSprite(&g_r3Scene, g_r3Scene.pFirst);
	if (s == 0)
		return;
	rc = g_r3Img[32].rc;
	if (rand() & 3) {
		s->nGroup = 9;
		s->cur.y = parent->cur.rc.bottom - rc.bottom + 6;
		if (parent->nGroup == 4)
			s->cur.x = parent->cur.rc.right + 1;
		else
			s->cur.x = parent->cur.rc.left - rc.right - 1;
		s->dwHitMask = 0x8C00L;
	} else {
		s->nGroup = 9;
		s->dy = g_nR3BoltVy;
		s->cur.y = parent->cur.rc.bottom + 1;
		s->cur.x = parent->cur.rc.left + (parent->cur.rc.right - parent->cur.rc.left - rc.right) / 2;
		s->dwHitMask = 0x400L;
	}
	s->nDrawMode = -1;
	s->cur.img = g_r3Img[32];
	s->cur.mask = g_r3Img[76];
	UpdateSpriteRect(s);
	s->nMoveRate = 1;
	s->lpfnMoved = MoveRound3Bolt;
	s->lpfnHit = HitRound3Bolt;
	s->lpfnTick = TickRound3Bolt;
	g_nR3Bolts++;
}

// FUNCTION: AARCADE 001B:02CA
int FAR PASCAL MoveRound3Script(int n, SPRITE *s, void *pm, int fReady)
{
	switch (n) {
	case 0:
		if (s->nGroup == 5)
			SetRound3Move(s, (int)g_r3CmdL2, MoveRound3Script);
		break;
	case 1:
		s->nGroup = 3;
		s->dwHitMask = 0xB800L;
		SetRound3Move(s, (int)g_r3CmdL, MoveRound3Script);
		SpawnRound3Bolt(s);
		break;
	case 2:
		if (s->nGroup == 6)
			SetRound3Move(s, (int)g_r3CmdR2, MoveRound3Script);
		break;
	case 3:
		s->nGroup = 4;
		s->dwHitMask = 0xB800L;
		SetRound3Move(s, (int)g_r3CmdR, MoveRound3Script);
		SpawnRound3Bolt(s);
		break;
	}
	return 1;
}

// FUNCTION: AARCADE 001B:0355
void FAR PASCAL MoveRound3Klutz(SPRITE *s)
{
	if (s->nGroup == 3 && s->cur.rc.left > 0x200 || s->nGroup == 4 && s->cur.rc.right < 0) {
		s->fDead = 1;
		s->fDirty = 1;
	}
}

// FUNCTION: AARCADE 001B:0382
void FAR PASCAL TickRound3Klutz(SPRITE *s)
{
	if (s->fDead != 0) {
		g_nR3Active--;
		if (s->nGroup >= 7) {
			g_nR3Left--;
			AddPlayScore(10L);
		}
	}
}

// FUNCTION: AARCADE 001B:03AC
void FAR PASCAL HitRound3Klutz(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	int n;

	if (other->nGroup == 9) {
		s->nGroup = s->nGroup + 4;
		s->dwHitMask = 0x4000L;
		SetRound3Move(s, (int)g_r3CmdHit, 0);
		PlaySfx(0x1F41, 0);
		s->fDirty = 1;
	} else if (s->nGroup <= 4 && other->nGroup == 2) {
		s->nGroup += 2;
		s->dwHitMask = 0xA800L;
		PlaySfx(0x1F44, 0);
	} else if (other->nGroup == 1) {
		s->cur.fVisible = 0;
		s->fDead = 1;
		s->fDirty = 1;
		AddPlayAmmo(-5L);
		PlaySfx(0x1770, 1);
	} else if (other->nGroup >= 3 && other->nGroup <= 8) {
		if (other->nGroup < 5 && s->nGroup < 5)
			return;
		/* the failing tests jump to the epilogue; the body is the fall-through */
		if (s->nGroup < 7 || other->nGroup < 7)
			if (((s->nGroup & 1) ^ (other->nGroup & 1)) != 0) {
				n = other->cur.x - s->cur.x;
				if (other->nGroup & 1)
					n = -n;
				if (s->cur.img.rc.right - s->cur.img.rc.left + -0x10 < n) {
					if ((s->nGroup + -3) & 1)
						SetRound3Move(s, (int)g_r3CmdA, 0);
					else
						SetRound3Move(s, (int)g_r3CmdB, 0);
					s->nGroup = ((s->nGroup + -3) & 1) + 7;
					s->dwHitMask = 0x4000L;
					s->fDirty = 1;
					AddPlayScore(25L);
					PlaySfx(0x1F42, 0);
				}
			}
	}
}

// FUNCTION: AARCADE 001B:04E4
void FAR PASCAL SetupRound3(void)
{
	int nFrame;

	g_nR3Cool = 0;
	nFrame = g_nPlayWave;
	if (nFrame >= 10)
		nFrame = 9;
	g_nR3Left += g_anR3Left[nFrame];
	g_nR3Max = g_anR3Max[nFrame];
	g_nR3BoltVy = g_anR3BoltVy[nFrame];
	g_nR3Arm = g_anR3Arm[nFrame];
	g_nR3Wave = g_anR3Wave[nFrame];
	g_nR3Mod = g_anR3Mod[nFrame];
	g_nR3DogSpeed = g_anR3DogSpeed[nFrame];
	g_nR3CloudMax = g_anR3CloudMax[nFrame];
	g_nR3CloudDx = g_anR3CloudDx[nFrame];
	g_nR3CloudVy = g_anR3CloudVy[nFrame];
	g_nR3CloudDelay = g_nR3CloudPeriod = g_anR3CloudPeriod[nFrame];
}

// FUNCTION: AARCADE 001B:057A
void FAR PASCAL SpawnRound3Klutz(void)
{
	int nSide;
	int anCount[3];
	int nSpan;
	int nUsed;
	volatile int i;
	int anFlat[6];
	int nLane;
	SPRITE *s;

	if (g_nR3Active < 3 && g_nR3Cool > 0x14)
		g_nR3Cool = (rand() & 0x7FFF) % 12 + 8;
	if (g_nR3Cool != 0)
		g_nR3Cool--;
	if (g_nR3Cool != 0)
		return;
	if (g_nR3Active >= g_nR3Max)
		return;
	if (g_nR3Left <= g_nR3Active)
		return;
	if (g_nPlayAmmo == 0)
		return;
	nUsed = 0;
	for (i = 0; i < 3; i++) {
		anCount[i] = 0;
		anFlat[i] = 0;
		anFlat[i + 3] = 0;
	}
	s = g_r3Scene.pFirst;
	nSpan = g_r3Img[0].rc.right * 2;
	for (; s != 0; s = s->pNext) {
		if (s->nGroup >= 3 && s->nGroup <= 8) {
			i = 0;
			if (s->cur.rc.top > 0x32) {
				i++;
				if (s->cur.rc.top > 0x78)
					i++;
			}
			anCount[i]++;
			if (s->cur.rc.right <= nSpan) {
				anFlat[i * 2] = 1;
				nUsed++;
			} else if (s->cur.rc.left >= 0x200 - nSpan) {
				anFlat[i * 2 + 1] = 1;
				nUsed++;
			}
		}
	}
	if (nUsed != 6) {
		nLane = 0;
		if (anCount[0] != 1) {
			nLane++;
			if (anCount[1] != 1) {
				nLane++;
				if (anCount[2] != 1)
					nLane++;
			}
		}
		if (nLane == 3) {
			do {
				nLane = (rand() & 0x7FFF) % 3;
				if (anFlat[nLane * 2] == 0 || anFlat[nLane * 2 + 1] == 0)
					break;
			} while (1);
		}
		do
			nSide = rand() & 1;
		while (anFlat[nLane * 2 + 1 - nSide] != 0);
		nLane = nLane * 0x46 + 0x32;
		if (nSide != 0)
			s = AddRound3Sprite(&g_r3Img[0], -0x1E, nLane);
		else
			s = AddRound3Sprite(&g_r3Img[16], 0x200, nLane);
		if (s != 0) {
			if (nSide != 0) {
				SetRound3Move(s, (int)g_r3CmdL, MoveRound3Script);
				SetRound3Frame(s, &g_r3Img[0], 0, 0x10, g_pR3Base);
				s->nGroup = 3;
			} else {
				SetRound3Move(s, (int)g_r3CmdR, MoveRound3Script);
				SetRound3Frame(s, &g_r3Img[16], 0, 0x10, &g_pR3Base[16]);
				s->nGroup = 4;
			}
			s->nDrawMode = -1;
			s->dwHitMask = 0xB800L;
			s->lpfnMove = MoveRound3Klutz;
			s->lpfnTick = TickRound3Klutz;
			s->lpfnHit = HitRound3Klutz;
			g_nR3Active++;
		}
	}
	g_nR3Cool = 0x28;
}

// FUNCTION: AARCADE 001B:07CC
void FAR PASCAL MoveRound3Player(SPRITE *s)
{
	RECT rc;

	rc = g_r3Img[41].rc;
	if (g_nR3Cursor < 0)
		g_nR3Cursor = 0;
	else if (0x200 - rc.right < g_nR3Cursor)
		g_nR3Cursor = 0x200 - rc.right;
	s->dx = g_nR3Cursor - s->cur.rc.left;
	/* false side of the && joins the else */
	if (s->cur.rc.left < g_nR3Cursor && s->dx > 0x30)
		s->dx = 0x30;
	else if (s->cur.rc.left > g_nR3Cursor) {
		if (s->dx < -0x30)
			s->dx = -0x30;
	}
}

// FUNCTION: AARCADE 001B:0845
void FAR PASCAL AddRound3Player(void)
{
	SPRITE *s;
	RECT rc;

	rc = g_r3Img[41].rc;
	g_pR3Player = 0;
	s = AddSprite(&g_r3Scene);
	g_pR3Player = s;
	if (s != 0) {
		s->nGroup = 1;
		s->cur.x = (0x200 - rc.right) / 2;
		s->cur.y = 0x144 - rc.bottom;
		s->cur.img = g_r3Img[41];
		UpdateSpriteRect(s);
		s->dwHitMask = 0x4D00L;
		s->nMoveRate = 1;
		s->lpfnMove = MoveRound3Player;
	}
}

// FUNCTION: AARCADE 001B:08C7
void FAR PASCAL ClearRound3Banana(void)
{
	g_nR3Banana = 0;
}

// FUNCTION: AARCADE 001B:08D4
void FAR PASCAL MoveRound3Banana(SPRITE *s)
{
	if (s->cur.rc.bottom <= 0) {
		s->cur.fVisible = 0;
		s->fDead = 1;
	}
}

// FUNCTION: AARCADE 001B:08F0
void FAR PASCAL TickRound3Banana(SPRITE *s)
{
	if (s->fDead != 0)
		g_nR3Banana--;
}

// FUNCTION: AARCADE 001B:0908
void FAR PASCAL HitRound3Banana(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	if (other->nGroup != 9) {
		AddPlayScore(5L);
		s->cur.img = g_r3Img[40];
		s->cur.mask = g_r3Img[84];
		UpdateSpriteRect(s);
		s->fDirty = 1;
		s->dwHitMask = 0;
	}
}

// FUNCTION: AARCADE 001B:0963
void FAR PASCAL ThrowRound3Banana(void)
{
	SPRITE *s;
	SPRITE *p;
	RECT rcPlayer;
	RECT rcBomb;

	rcPlayer = g_r3Img[41].rc;
	rcBomb = g_r3Img[39].rc;
	if (g_nR3Banana >= 3)
		return;
	if (g_nPlayAmmo == 0)
		return;
	s = InsertSprite(&g_r3Scene, g_r3Scene.pFirst);
	if (s == 0)
		return;
	p = g_pR3Player;
	s->nGroup = 2;
	s->dy = -10;
	s->cur.y = p->cur.rc.top - rcBomb.bottom;
	s->cur.x = p->cur.rc.left + (rcPlayer.right - rcBomb.right) / 2;
	s->cur.img = g_r3Img[39];
	s->cur.mask = g_r3Img[83];
	UpdateSpriteRect(s);
	s->nMoveRate = 1;
	s->nDrawMode = -1;
	s->dwHitMask = 0x1000L;
	s->lpfnMoved = MoveRound3Banana;
	s->lpfnHit = HitRound3Banana;
	s->lpfnTick = TickRound3Banana;
	AddPlayAmmo(-1L);
	g_nR3Banana++;
	PlaySfx(0x1F40, 0);
}

// FUNCTION: AARCADE 001B:0A5E
void FAR PASCAL ClearRound3RobotDogs(void)
{
	g_nR3RobotDogs = 0;
}

// FUNCTION: AARCADE 001B:0A6B
void FAR PASCAL MoveRound3RobotDog(SPRITE *s)
{
	if (s->cur.rc.right <= 0) {
		s->cur.fVisible = 0;
		s->fDead = 1;
	}
}

// FUNCTION: AARCADE 001B:0A87
void FAR PASCAL TickRound3RobotDog(SPRITE *s)
{
	if (s->fDead != 0)
		g_nR3RobotDogs--;
}

// FUNCTION: AARCADE 001B:0A9F
void FAR PASCAL HitRound3RobotDog(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	if (other->nGroup == 9) {
		AddPlayScore(5L);
		s->cur.img = g_r3Img[35];
		s->cur.mask = g_pR3Base[35];
		UpdateSpriteRect(s);
		s->fDirty = 1;
	}
}

// FUNCTION: AARCADE 001B:0AF4
void FAR PASCAL SpawnRound3RobotDog(void)
{
	int n;
	RECT rc;
	SPRITE *s;

	rc = g_r3Img[33].rc;
	if (g_nR3RobotDogs != 0)
		return;
	if (g_nR3Left <= 1)
		return;
	s = AddSprite(&g_r3Scene);
	if (s == 0)
		return;
	s->nGroup = 10;
	s->dx = -g_nR3DogSpeed;
	n = (rand() & 0x7FFF) % g_nR3Mod;
	s->cur.y = n * 0x46 + 0x5F - rc.bottom;
	s->cur.x = 0x200;
	SetRound3Frame(s, &g_r3Img[33], 0, 2, &g_pR3Base[33]);
	s->nDrawMode = -1;
	s->nMoveRate = 1;
	s->nAnimRate = 4;
	s->dwHitMask = 0x200L;
	s->lpfnMoved = MoveRound3RobotDog;
	s->lpfnHit = HitRound3RobotDog;
	s->lpfnTick = TickRound3RobotDog;
	g_nR3RobotDogs++;
	g_pR3RobotDog = s;
	PlaySfx(0x1F46, 0);
}

// FUNCTION: AARCADE 001B:0BC6
void FAR PASCAL MoveRound3Cloud(SPRITE *s)
{
	if (s->cur.rc.top >= 0x146) {
		s->cur.fVisible = 0;
		s->fDead = 1;
	}
}

// FUNCTION: AARCADE 001B:0BE3
void FAR PASCAL TickRound3Cloud(SPRITE *s)
{
	if (s->fDead != 0)
		g_nR3Clouds--;
}

// FUNCTION: AARCADE 001B:0BFB
void FAR PASCAL HitRound3Cloud(SPRITE *s, SPRITE *other, BOOL fFirst)
{
	if (other->nGroup == 1) {
		s->cur.fVisible = 0;
		s->fDead = 1;
		AddPlayAmmo(-2L);
		PlaySfx(0x1770, 1);
	}
}

// FUNCTION: AARCADE 001B:0C2E
void FAR PASCAL SpawnRound3Cloud(void)
{
	RECT rc;
	SPRITE *s;

	rc = g_r3Img[41].rc;
	if (g_nR3RobotDogs != 0 && g_nR3CloudDelay-- == 0) {
		if (g_nR3Clouds < g_nR3CloudMax) {
			s = InsertSprite(&g_r3Scene, g_r3Scene.pFirst);
			if (s != 0) {
				s->nGroup = 11;
				s->cur.y = g_pR3RobotDog->cur.y + 5;
				s->cur.x = g_pR3RobotDog->cur.x + 0x28;
				SetRound3Frame(s, &g_r3Img[36], 0, 3, &g_pR3Base[36]);
				s->nDrawMode = -1;
				s->dy = g_nR3CloudVy;
				if (g_nR3CloudDx != 0) {
					s->dx = -(s->cur.x - g_pR3Player->cur.x + rc.right / 2);
					s->dx = s->dx / ((g_pR3Player->cur.y - s->cur.y + rc.bottom / 2) / g_nR3CloudVy);
					if (s->dx > g_nR3CloudDx)
						s->dx = g_nR3CloudDx;
					else if (s->dx < -g_nR3CloudDx)
						s->dx = -g_nR3CloudDx;
				}
				s->nMoveRate = 1;
				s->dwHitMask = 0x100L;
				s->lpfnMoved = MoveRound3Cloud;
				s->lpfnHit = HitRound3Cloud;
				s->lpfnTick = TickRound3Cloud;
				g_nR3Clouds++;
				PlaySoundEx(0x1F45, 0, 1);
			}
		}
		g_nR3CloudDelay = g_nR3CloudPeriod;
	}
}

// FUNCTION: AARCADE 001B:0D74
void FAR PASCAL LayRound3(void)
{
	SPRITE *s;
	int i;

	s = AddSprite(&g_r3Scene);
	s->cur.x = 0;
	s->cur.y = 0;
	s->cur.img = g_r3Img[43];
	UpdateSpriteRect(s);
	for (i = 0; i < 3; i++) {
		s = AddSprite(&g_r3Scene);
		s->cur.x = 0;
		s->cur.y = i * 0x46 + 0x5F;
		s->cur.img = g_r3Img[42];
		UpdateSpriteRect(s);
	}
}

// FUNCTION: AARCADE 001B:0DE9
void FAR PASCAL ArmRound3(void)
{
	SetTimeLeft((long)(g_nPlayWave * 10 + 0x2D));
}

// FUNCTION: AARCADE 001B:0E01
void FAR PASCAL BackRound3(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[1]);
}

// FUNCTION: AARCADE 001B:0E19
void FAR PASCAL OpenRound3(HWND hwnd)
{
	g_nR3Clouds = 0;
	g_nR3Active = 0;
	g_nR3Left = 0;
	LoadPPicImages(0x1F40, 0, 0x55, g_r3Img, g_nDepth);
	g_pR3Base = &g_r3Img[44];
	DecImageBottoms(g_r3Img, 0, 0x0E);
	DecImageBottoms(g_r3Img, 0x10, 0x1E);
	if (g_fColor)
		InitScene(hwnd, &g_r3Scene, BackRound3, &g_rcPlay, g_nDepth2, 3, 0);
	else
		InitScene(hwnd, &g_r3Scene, 0, &g_rcPlay, 1, 3, 0);
	ArmRound3();
	AddRound3Player();
	ClearRound3Banana();
	SetupRound3();
	ClearRound3Bolts();
	ClearRound3RobotDogs();
	LayRound3();
	g_dwR3Next = 0;
	g_dwR3Deadline = GetTickCount() / 17 + (long)(g_nR3Arm * 0x3C);
	g_nR3CloudDelay = g_nR3CloudPeriod;
	PreloadSounds(0x1770, 1);
	PreloadSounds(0x1F40, 7);
}

// FUNCTION: AARCADE 001B:0F05
void FAR PASCAL ClickRound3(void)
{
	ThrowRound3Banana();
}

// FUNCTION: AARCADE 001B:0F10
int FAR PASCAL TickRound3(void)
{
	if (g_nR3Active <= 1 && g_nR3Left <= 1 && g_nR3Banana == 0 && g_nR3Clouds == 0) {
		RunStepDialog();
		NextPlayWave();
		ArmRound3();
		SetupRound3();
		AddPlayAmmo((long)g_nR3Left);
		g_dwR3Deadline = GetTickCount() / 17 + (long)(g_nR3Arm * 0x3C);
		SpawnRound3Klutz();
	}
	g_nR3Cursor = (int)GetMessagePos();
	if (GetTickCount() / 17 >= g_dwR3Next) {
		SpawnRound3Klutz();
		if (GetTickCount() / 17 >= g_dwR3Deadline) {
			SpawnRound3RobotDog();
			g_dwR3Deadline = GetTickCount() / 17 + (long)(g_nR3Wave * 0x3C);
		}
		SpawnRound3Cloud();
		g_dwR3Next = GetTickCount() / 17 + 3;
		TickScene(&g_r3Scene);
	}
	return g_nPlayAmmo == 0 && g_nR3Banana == 0 ? 1 : 0;
}

// FUNCTION: AARCADE 001B:102E
void FAR PASCAL PaintRound3(void)
{
	PaintScene(&g_r3Scene);
}

// FUNCTION: AARCADE 001B:103D
void FAR PASCAL CloseRound3(void)
{
	FreeScene(&g_r3Scene);
	FreeImages(g_r3Img, 0x55, 1);
}
