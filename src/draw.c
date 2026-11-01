#include "aarcade.h"

/* Scene redraw: dirty rectangles and sprite blit into the off-screen image. */

// FUNCTION: AARCADE 000B:0000
void FAR PASCAL DrawScene(SCENE *ps, SPRITE *s, LPRECT lprc, BOOL fInclude)
{
	RECT rc;
	IMAGE *pimg;
	int n;
	SPRITE *p;

	if (!s)
		return;
	rc = *lprc;
	n = ps->rc.left;
	if (rc.left < n)
		rc.left = n;
	n = ps->rc.right;
	if (rc.right > n)
		rc.right = n;
	n = ps->rc.top;
	if (rc.top < n)
		rc.top = n;
	n = ps->rc.bottom;
	if (rc.bottom > n)
		rc.bottom = n;
	if (rc.left >= rc.right || rc.top >= rc.bottom)
		return;

	pimg = &ps->img;
	if (ps->lpfnBack)
		ps->lpfnBack(pimg, &rc);
	else
		FillImage(pimg, &rc, GetStockObject(WHITE_BRUSH));

	for (p = ps->pLast; p != s; p = p->pPrev) {
		if (p->old.fVisible && DrawRectsOverlap(&rc, &p->old.rc)) {
			if (p->lpfnDraw)
				DrawSpriteProc(p, pimg, FALSE);
			else if (p->nDrawMode < 0)
				MaskBlitImage(&p->old.img, &p->old.mask, pimg, 0, 0, &p->old.rc, 0, 0);
			else
				CopyBlitImage(&p->old.img, pimg, 0, 0, &p->old.rc, p->nDrawMode, 0);
		}
	}
	if (fInclude && p->cur.fVisible && DrawRectsOverlap(&rc, &p->cur.rc)) {
		if (p->lpfnDraw)
			DrawSpriteProc(p, pimg, TRUE);
		else if (p->nDrawMode < 0)
			MaskBlitImage(&p->cur.img, &p->cur.mask, pimg, 0, 0, &p->cur.rc, 0, 0);
		else
			CopyBlitImage(&p->cur.img, pimg, 0, 0, &p->cur.rc, p->nDrawMode, 0);
	}
	for (p = p->pPrev; p; p = p->pPrev) {
		if (p->cur.fVisible && DrawRectsOverlap(&rc, &p->cur.rc)) {
			if (p->lpfnDraw)
				DrawSpriteProc(p, pimg, TRUE);
			else if (p->nDrawMode < 0)
				MaskBlitImage(&p->cur.img, &p->cur.mask, pimg, 0, 0, &p->cur.rc, 0, 0);
			else
				CopyBlitImage(&p->cur.img, pimg, 0, 0, &p->cur.rc, p->nDrawMode, 0);
		}
	}

	BlitImageToDC(ps->hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
	              pimg, rc.left, rc.top, SRCCOPY);
}

/* Same code as RectsOverlap and UnionRects in scene.c. */

// FUNCTION: AARCADE 000B:027C
BOOL FAR PASCAL DrawRectsOverlap(RECT *a, RECT *b)
{
	if (a->left <= b->right && b->left <= a->right && a->top <= b->bottom && b->top <= a->bottom)
		return TRUE;
	return FALSE;
}

// FUNCTION: AARCADE 000B:02B6
void FAR PASCAL DrawSpriteProc(SPRITE *s, IMAGE *pimg, BOOL fCur)
{
	HGDIOBJ hOld;

	hOld = SelectObject(g_hdcMemA, pimg->hbm);
	s->lpfnDraw(g_hdcMemA, s, pimg, fCur);
	SelectObject(g_hdcMemA, hOld);
}

// FUNCTION: AARCADE 000B:02F4
void FAR PASCAL DrawUnionRects(RECT *a, RECT *b, RECT *dst)
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
