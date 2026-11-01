#include "aarcade.h"

/* Sprite allocation and the per-scene doubly linked sprite list. */

// FUNCTION: AARCADE 0006:0000
SPRITE *FAR PASCAL NewSprite(void)
{
	SPRITE *s;

	s = (SPRITE *)LocalAlloc(LPTR, sizeof(SPRITE));
	if (s) {
		s->cur.fVisible = 1;
		s->fHold = 1;
		s->nMoveRate = 1;
		s->nStep = 1;
	}
	return s;
}

// FUNCTION: AARCADE 0006:002E
SPRITE *FAR PASCAL InsertSprite(SCENE *ps, SPRITE *before)
{
	SPRITE *s;

	if (!before)
		return AddSprite(ps);
	s = NewSprite();
	if (s) {
		s->pNext = before;
		s->pPrev = before->pPrev;
		before->pPrev = s;
		if (ps->pFirst == before)
			ps->pFirst = s;
		else
			s->pPrev->pNext = s;
	}
	return s;
}

// FUNCTION: AARCADE 0006:0082
SPRITE *FAR PASCAL AddSprite(SCENE *ps)
{
	SPRITE *s;
	SPRITE *last;

	s = NewSprite();
	if (s) {
		last = ps->pLast;
		ps->pLast = s;
		if (last) {
			last->pNext = s;
			s->pPrev = last;
		} else
			ps->pFirst = s;
	}
	return s;
}

// FUNCTION: AARCADE 0006:00C5
void FAR PASCAL UnlinkSprite(SCENE *ps, SPRITE *s)
{
	if (ps->pFirst == s)
		ps->pFirst = s->pNext;
	if (ps->pLast == s)
		ps->pLast = s->pPrev;
	/* the original keeps the neighbour in DX (mov dx,ax ... mov bx,dx) */
	if ((_DX = (unsigned)s->pNext) != 0)
		((SPRITE *)_DX)->pPrev = s->pPrev;
	if ((_DX = (unsigned)s->pPrev) != 0)
		((SPRITE *)_DX)->pNext = s->pNext;
}

// FUNCTION: AARCADE 0006:0118
void FAR PASCAL FreeSprite(SCENE *ps, SPRITE *s)
{
	UnlinkSprite(ps, s);
	if (s->dwData)
		LocalFree((HLOCAL)s->dwData);
	LocalFree((HLOCAL)s);
}

// FUNCTION: AARCADE 0006:0144
void FAR PASCAL UpdateSpriteRect(SPRITE *s)
{
	s->cur.rc = s->cur.img.rc;
	s->cur.rc.left += s->cur.x;
	s->cur.rc.right += s->cur.x;
	s->cur.rc.top += s->cur.y;
	s->cur.rc.bottom += s->cur.y;
}

// FUNCTION: AARCADE 0006:0180
void FAR PASCAL FreeImages(IMAGE *pimg, int n, int unused)
{
	while (n--) {
		FreeImage(pimg);
		pimg++;
	}
}
