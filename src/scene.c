#include "aarcade.h"

/* Scene engine: init, tick, collisions, and the backbuffer for a playfield. */

// FUNCTION: AARCADE 0004:0000
void FAR PASCAL EmptyProc(void)
{
}

// FUNCTION: AARCADE 0004:0007
int FAR PASCAL InitScene(HWND hwnd, SCENE *ps, SCENEBKPROC lpfnBack, LPRECT lprc, int nDepth, int nRate, int n)
{
	int nResult = 0;
	IMAGE img;

	ps->hwnd = hwnd;
	ps->unk04 = nDepth != 1;
	ps->unk06 = nDepth != 1;
	ps->rc = *lprc;
	ps->nRate = nRate ? nRate : 1;
	ps->dwLastTick = 0;
	OffsetRect(&ps->rc, -ps->rc.left, -ps->rc.top);
	ps->pFirst = 0;
	ps->pLast = 0;
	ps->lpfnBack = lpfnBack;
	CreateImage(hwnd, &img, &ps->rc, nDepth, 2, NULL, n);
	ps->img = img;
	return nResult;
}

// FUNCTION: AARCADE 0004:00D9
void FAR PASCAL TickScene(SCENE *ps)
{
	SPRITE *s;

	if (ps->nRate > 0) {
		ps->hdc = GetDC(ps->hwnd);
		{
			int i;
			DWORD dwNow;
			long nSteps;

			dwNow = GetTickCount() / 17;
			nSteps = 1;
			if (ps->dwLastTick > 0) {
				nSteps = (dwNow - ps->dwLastTick) / ps->nRate;
				if (nSteps < 1)
					nSteps = 1;
				else if (nSteps > 3)
					nSteps = 3;
			}
			ps->dwLastTick = dwNow;

			for (i = 0; i < nSteps; i++) {
				for (s = ps->pFirst; s; s = s->pNext) {
					if (s->fDead)
						continue;
					if (i == 0)
						s->fDirty = 0;
					if (s->cur.fVisible != s->old.fVisible)
						s->fDirty = 1;
					s->nMoveCount++;
					if (s->nMoveCount == s->nMoveRate) {
						s->nMoveCount = 0;
						if (s->lpfnMove)
							s->lpfnMove(s);
						if (s->lpfnMoveEx)
							s->lpfnMoveEx(s, i == 0);
						else {
							if (i == 0)
								s->old = s->cur;
							if (!s->fHold) {
								_DX = s->dx;
								_CX = s->dy;
								if (_DX || _CX) {
									s->fDirty = 1;
									/* _DX and _CX are unsigned; without the casts each add goes through AX */
									s->cur.x += (int)_DX;
									s->cur.y += (int)_CX;
									s->cur.rc.left += (int)_DX;
									s->cur.rc.right += (int)_DX;
									s->cur.rc.top += (int)_CX;
									s->cur.rc.bottom += (int)_CX;
								}
							}
						}
						if (s->lpfnMoved)
							s->lpfnMoved(s);
					}
					if (s->nAnimRate) {
						s->nAnimCount++;
						if (s->nAnimCount == s->nAnimRate) {
							s->nAnimCount = 0;
							if (s->lpfnAnimate)
								s->lpfnAnimate(s);
							else {
								char nFrame;

								nFrame = s->nFrame;
								nFrame += s->nStep;
								if (s->nAnimMode == 0) {
									if (nFrame < 0 || s->nFrames <= nFrame) {
										nFrame -= s->nStep;
										s->nStep = -s->nStep;
									}
								} else if (s->nAnimMode == 1) {
									if (nFrame < 0)
										nFrame += s->nFrames;
									else if (s->nFrames <= nFrame)
										nFrame -= s->nFrames;
								}
								s->nFrame = nFrame;
								if (s->pImages)
									s->cur.img = s->pImages[nFrame];
								if (s->pMasks)
									s->cur.mask = s->pMasks[nFrame];
								s->fDirty = 1;
							}
							s->cur.rc.left = s->cur.img.rc.left + s->cur.x;
							s->cur.rc.right = s->cur.img.rc.right + s->cur.x;
							s->cur.rc.top = s->cur.img.rc.top + s->cur.y;
							s->cur.rc.bottom = s->cur.img.rc.bottom + s->cur.y;
						}
					}
					s->fHold = 0;
				}

				for (s = ps->pFirst; s; s = s->pNext)
					s->fHitDone = s->fDead;

				for (s = ps->pFirst; s; s = s->pNext) {
					if (!s->fHitDone) {
						SPRITE *other;
						int fHit;

						for (other = s->pNext; other; other = other->pNext) {
							if (!other->fHitDone && s->nGroup != other->nGroup &&
							    s->cur.fVisible && other->cur.fVisible &&
							    (s->dwHitMask & other->dwHitMask) &&
							    RectsOverlap(&s->cur.rc, &other->cur.rc)) {
								fHit = 1;
								if (s->lpfnHitTest) {
									s->lpfnHitTest(s, other, TRUE, &fHit);
									if (fHit && other->lpfnHitTest)
										other->lpfnHitTest(other, s, FALSE, &fHit);
								}
								if (fHit) {
									if (s->lpfnHit)
										s->lpfnHit(s, other, TRUE);
									if (other->lpfnHit)
										other->lpfnHit(other, s, FALSE);
									other->fHitDone = 1;
								}
							}
						}
					}
				}
			}
		}

		for (s = ps->pFirst; s; s = s->pNext) {
			/* declared here: in the then-block below it adds a jmp after the merged call */
			RECT rc;

			if (s->fDirty) {
				if (s->old.fVisible) {
					if (s->cur.fVisible) {
						UnionRects(&s->old.rc, &s->cur.rc, &rc);
						DrawScene(ps, s, &rc, TRUE);
					} else
						DrawScene(ps, s, &s->old.rc, FALSE);
				} else if (s->cur.fVisible)
					DrawScene(ps, s, &s->cur.rc, TRUE);
			}
		}

		{
			SPRITE *next;

			for (s = ps->pFirst; s; s = next) {
				next = s->pNext;
				if (s->lpfnTick)
					s->lpfnTick(s);
				if (s->fDead)
					FreeSprite(ps, s);
			}
		}
		ReleaseDC(ps->hwnd, ps->hdc);
		ps->hdc = 0;
	}
}

// FUNCTION: AARCADE 0004:0525
void FAR PASCAL FreeScene(SCENE *ps)
{
	SPRITE *s;

	if (ps->hwnd) {
		FreeImage(&ps->img);
		while ((s = ps->pFirst) != 0)
			FreeSprite(ps, s);
		ps->hwnd = 0;
	}
}

// FUNCTION: AARCADE 0004:055F
void FAR PASCAL PaintScene(SCENE *ps)
{
	PAINTSTRUCT paint;

	ps->hdc = BeginPaint(ps->hwnd, &paint);
	DrawScene(ps, ps->pLast, &paint.rcPaint, TRUE);
	EndPaint(ps->hwnd, &paint);
	ps->hdc = 0;
}

// FUNCTION: AARCADE 0004:05A0
BOOL FAR PASCAL RectsOverlap(RECT *a, RECT *b)
{
	if (a->left <= b->right && b->left <= a->right && a->top <= b->bottom && b->top <= a->bottom)
		return TRUE;
	return FALSE;
}

// FUNCTION: AARCADE 0004:05DA
void FAR PASCAL UnionRects(RECT *a, RECT *b, RECT *dst)
{
	*dst = *a;
	if (b->top < dst->top)
		dst->top = b->top;
	if (b->left < dst->left)
		dst->left = b->left;
	if (b->bottom > dst->bottom)
		dst->bottom = b->bottom;
	if (b->right > dst->right)
		dst->right = b->right;
}
