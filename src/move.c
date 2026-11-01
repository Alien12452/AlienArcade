#include "aarcade.h"

/* Sprite move-script interpreter (opcodes driven from a per-sprite block). */

/* Script block for one sprite (0x22 bytes). pos/posHi are one long byte
     cursor; x/y/dx/dy are 16.16 pixels. Opcode 15's body is before opcode 12
     because that is the order the jump table was compiled from. */
typedef struct tagMOVESTATE MOVESTATE;
typedef int (FAR PASCAL *MOVEHOOK)(int n, SPRITE *s, MOVESTATE *pm, int fReady);
typedef struct tagMOVESTATE {
	unsigned char *pc;
	int pos;
	int posHi;
	int nStep;
	int nSteps;
	long x;
	long y;
	long dx;
	long dy;
	MOVEHOOK lpfn;
	int reserved[2];
} MOVESTATE;

// FUNCTION: AARCADE 000C:0000
void FAR PASCAL MoveSprite(SPRITE *s, BOOL fFirst)
{
	unsigned char *ip;
	int fReady;
	int fCont;
	MOVESTATE *pm;

	fReady = 1;
	fCont = 1;
	if (fFirst)
		s->old = s->cur;
	pm = (MOVESTATE *)s->dwData;
	if (pm->nStep != pm->nSteps) {
		pm->nStep++;
		fReady = 0;
	}
	while (fCont) {
		ip = pm->pc + pm->pos;
		fCont = 0;
		switch (*ip) {
		case 0:
			fCont = 1;
			break;
		case 1:
			s->cur.fVisible = 0;
			s->fDirty = 1;
			fCont = 1;
			break;
		case 2:
			s->cur.fVisible = 1;
			s->fDirty = 1;
			fCont = 1;
			break;
		case 3:
		case 4:
		{
			/* locals on this block, not the if: the closing brace emits a jump */
			long nX;
			long nY;
			unsigned char *cmd;

			if (fReady) {
				cmd = ip;
				pm->nSteps = *(int *)(cmd + 2);
				pm->x = (long)s->cur.x << 16;
				pm->y = (long)s->cur.y << 16;
				nX = (long)*(int *)(cmd + 4);
				nY = (long)*(int *)(cmd + 6);
				if (*cmd == 3) {
					nX -= s->cur.x;
					nY -= s->cur.y;
				}
				if (pm->nSteps) {
					pm->nStep = 1;
					pm->dx = (nX << 16) / pm->nSteps;
					pm->dy = (nY << 16) / pm->nSteps;
				} else {
					pm->nStep = 0;
					pm->dx = nX << 16;
					pm->dy = nY << 16;
					fCont = 1;
				}
			}
			pm->x += pm->dx;
			/* rounded integer step, reused in CX */
			_CX = (int)((pm->x + 0x8000L) >> 16) - s->cur.x;
			s->cur.x += (int)_CX;
			s->cur.rc.left += (int)_CX;
			s->cur.rc.right += (int)_CX;
			pm->y += pm->dy;
			_CX = (int)((pm->y + 0x8000L) >> 16) - s->cur.y;
			s->cur.y += (int)_CX;
			s->cur.rc.top += (int)_CX;
			s->cur.rc.bottom += (int)_CX;
			s->fDirty = 1;
		}
			/* break after the block: two jumps out of the case */
			break;
		case 5:
		case 6:
		case 7:
		{
			int n;
			RECT rc;

			if (*ip == 5)
				n = *(int *)(ip + 2);
			else {
				n = s->nFrame;
				if (*ip == 6)
					n += s->nStep;
				else
					n -= s->nStep;
				if (s->nAnimMode == 0) {
					if (n < 0 || s->nFrames <= n) {
						n -= s->nStep;
						s->nStep = -s->nStep;
					}
				} else if (s->nAnimMode == 1) {
					if (n < 0)
						n += s->nFrames;
					else if (s->nFrames <= n)
						n -= s->nFrames;
				}
			}
			s->nFrame = n;
			if (s->pImages)
				s->cur.img = s->pImages[n];
			if (s->pMasks)
				s->cur.mask = s->pMasks[n];
			rc = s->cur.img.rc;
			/* left, right, then top, bottom: that is the original store order */
			s->cur.rc.left = rc.left + s->cur.x;
			s->cur.rc.right = rc.right + s->cur.x;
			s->cur.rc.top = rc.top + s->cur.y;
			s->cur.rc.bottom = rc.bottom + s->cur.y;
			s->fDirty = 1;
		}
		/* after the brace: the block joins the fDirty tail, this joins fCont */
		fCont = 1;
		break;
		case 8:
			s->nDrawMode = *(int *)(ip + 2);
			s->fDirty = 1;
			fCont = 1;
			break;
		case 9:
			if (fReady) {
				pm->nStep = 1;
				pm->nSteps = *(int *)(ip + 2);
				if (pm->nSteps == 0) {
					pm->nStep = 0;
					/* inner break jumps to the exit; the one below is the second jump */
					break;
				}
			}
			break;
		case 10:
			pm->posHi = 0;
			pm->pos = 0;
			fCont = 1;
			break;
		case 11:
			PlaySfx(*(int *)(ip + 2), 0);
			fCont = 1;
			break;
		case 15:
			PlaySoundEx(*(int *)(ip + 2), 0, *(int *)(ip + 4));
			fCont = 1;
			break;
		case 12:
			if (pm->lpfn) {
				fCont = pm->lpfn(*(int *)(ip + 2), s, pm, fReady);
				ip = pm->pc + pm->pos;
			}
			break;
		case 13:
		case 14:
			s->fDead = 1;
			break;
		default:
			break;
		}
		if (fCont) {
			*(long *)&pm->pos += ip[1];
			fReady = 1;
		}
	}
	if (pm->nStep == pm->nSteps)
		*(long *)&pm->pos += ip[1];
}

// FUNCTION: AARCADE 000C:038F
void *FAR PASCAL AllocSpriteMove(void)
{
	void *p;

	p = (void *)LocalAlloc(LPTR, 0x22);
	return p;
}
