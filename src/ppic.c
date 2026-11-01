#include <mem.h>
#include "aarcade.h"

/* PPIC/PPCT resources: a big-endian image count, then size-prefixed packed
   images. The unpackers are a transliteration of 68k code: the labels and
   `if (--c != -1) goto` loops are dbra loops and compile to the original's
   jump shapes. `x ^ 0xFF` gives the original's `xor al,0xff` (`~x` is
   `not`), and `lpDst = lpDst + n` stores both words of the far pointer
   where `+=` would only add to the offset. */

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:3D42
LPSTR g_lpPicClut;


// GLOBAL: AARCADE 0024:3D40
HLOCAL g_hPicXlat;


// GLOBAL: AARCADE 0024:3D3E
HLOCAL g_hPicPal;

// FUNCTION: AARCADE 0019:0000
BOOL FAR PASCAL LoadPPicImages(int id, int iFirst, int n, IMAGE *pimg, int nDepth)
{
	HGLOBAL h;
	LPSTR lp;
	int nImages;
	BOOL fOk;

	fOk = TRUE;
	h = FindRes(g_szPPicType, id);
	if (h) {
		lp = LockRes(h);
		nImages = GETWORD(lp);
		UnlockRes(h);
		lp = SeekPicRes(h, iFirst, nDepth);
		if (iFirst >= nImages)
			iFirst = nImages - 1;
		if (iFirst + n >= nImages || !n)
			n = nImages - iFirst;
		while (n) {
			if (!(fOk = DecodePicImage(&lp, pimg, nDepth)))
				break;
			pimg++;
			n--;
		}
		FreePicRes(h);
	} else
		fOk = FALSE;
	if (!fOk) {
		while (n--) {
			memset(pimg, 0, sizeof(IMAGE));
			pimg++;
		}
	}
	return fOk;
}

// FUNCTION: AARCADE 0019:00F0
BOOL FAR PASCAL GetPPicImage(int id, int index, IMAGE *pimg, int nDepth)
{
	return LoadPicResImage(g_szPPicType, id, index, pimg, nDepth);
}

// FUNCTION: AARCADE 0019:0110
BOOL FAR PASCAL GetPPctImage(int id, IMAGE *pimg)
{
	return LoadPicResImage(g_szPPctType, id, 1, pimg, g_nDepth);
}

// FUNCTION: AARCADE 0019:0130
BOOL FAR PASCAL LoadPicResImage(LPCSTR lpType, int id, int index, IMAGE *pimg, int nDepth)
{
	LPSTR lp;
	BOOL fOk;
	HGLOBAL h;

	fOk = FALSE;
	h = FindRes(lpType, id);
	if (h) {
		lp = SeekPicRes(h, index, nDepth);
		fOk = DecodePicImage(&lp, pimg, nDepth);
		FreePicRes(h);
	}
	return fOk;
}

// FUNCTION: AARCADE 0019:0187
LPSTR FAR PASCAL SeekPicRes(HGLOBAL h, int index, int nDepth)
{
	int nLast;
	LPSTR lp;
	int i;
	BYTE tbl[16];

	lp = LockRes(h);
	nLast = GETWORD(lp) - 1;
	lp += 2;
	if (index > nLast)
		index = nLast;
	g_lpPicClut = NULL;
	if (GETWORD(lp + 6) < 0 && GETWORD(lp + 4) < 0)
		g_lpPicClut = lp;
	while (index--) {
		lp = (BYTE huge *)lp + GETLONG(lp);
		if (GETWORD(lp + 6) < 0 && GETWORD(lp + 4) < 0)
			g_lpPicClut = lp;
	}
	g_hPicXlat = g_hPicPal = 0;
	if (nDepth >= 4) {
		/* empty then-branch: the original has je/jmp */
		if (g_lpPicClut) {
		} else {
			for (i = 0; i < 16; i++)
				tbl[i] = i;
		}
		if (nDepth == 4) {
			BYTE *ptbl;
			int hi;
			int lo;
			BYTE *pb;

			g_hPicXlat = LocalAlloc(LMEM_MOVEABLE, 256);
			ptbl = tbl;
			pb = (BYTE *)LocalLock(g_hPicXlat);
			for (i = 0; i < 256; i++) {
				hi = i;
				lo = hi & 0xF;
				hi >>= 4;
				hi = ptbl[hi];
				hi <<= 4;
				*pb = (BYTE)hi | ptbl[lo] & 0xF;
				pb++;
			}
			LocalUnlock(g_hPicXlat);
		} else {
			BYTE *ptbl;
			WORD FAR *pw;
			int hi;
			int x;

			g_hPicXlat = LocalAlloc(LMEM_MOVEABLE, 512);
			ptbl = tbl;
			pw = (WORD FAR *)LocalLock(g_hPicXlat);
			for (i = 0; i < 256; i++) {
				x = i;
				hi = x >> 4;
				x = ptbl[x & 0xF];
				x <<= 8;
				*pw = x | ptbl[hi];
				pw++;
			}
			LocalUnlock(g_hPicXlat);
		}
	}
	return lp;
}

// FUNCTION: AARCADE 0019:03EC
void FAR PASCAL FreePicRes(HGLOBAL h)
{
	if (g_hPicXlat)
		LocalFree(g_hPicXlat);
	if (g_hPicPal)
		LocalFree(g_hPicPal);
	UnlockRes(h);
	FreeRes(h);
}

// FUNCTION: AARCADE 0019:0426
BOOL FAR PASCAL DecodePicImage(LPSTR *plp, IMAGE *pimg, int nDepth)
{
	int cbRow;
	int cy;
	LPSTR lp;
	long cb;
	LPSTR lpBits;
	LPSTR lpRow;
	RECT rc;
	BOOL fOk;
	LPSTR lpHdr;
	int i;

	lpBits = NULL;
	fOk = TRUE;
	lp = *plp + 4;
	if (GETWORD(lp + 2) < 0) {
		fOk = DecodePixImage(plp, pimg, nDepth);
	} else {
		lpHdr = lp;
		lpHdr += 2;
		cbRow = GETWORD(lpHdr);
		lpHdr += 2;
		rc.top = rc.left = 0;
		rc.right = GETWORD(lpHdr);
		lpHdr += 2;
		rc.bottom = cy = GETWORD(lpHdr);
		lpHdr += 2;
		lp = lpHdr;
		cb = (long)cbRow * cy;
		lpBits = GAllocLock(0, cb);
		if (lpBits) {
			for (i = 0; i < cbRow; i++) {
				lpRow = lpBits + i;
				UnpackColumn1(&lp, lpRow, cy, cbRow);
			}
			if ((DWORD)lp & 1)
				lp++;
			*plp = lp;
			CreateImage(0, pimg, &rc, 1, 1, lpBits, 0);
			GUnlockFree(lpBits);
		} else
			fOk = FALSE;
	}
	return fOk;
}

// FUNCTION: AARCADE 0019:05B9
BOOL FAR PASCAL DecodePixImage(LPSTR *plp, IMAGE *pimg, int nDepth)
{
	int cbRow;
	int cy;
	LPSTR lp;
	long cb;
	LPSTR lpBits;
	int nFlags;
	RECT rc;
	BOOL fOk;
	BYTE huge *lpRow;
	BYTE huge *lpBase;
	int i;
	int cx;

	lpBits = NULL;
	fOk = TRUE;
	lp = *plp;
	lp += 4;
	nFlags = GETWORD(lp);
	lp += 2;
	cx = GETWORD(lp) & 0x3FFF;
	lp += 2;
	rc.top = rc.left = 0;
	rc.right = GETWORD(lp);
	lp += 2;
	cy = GETWORD(lp);
	rc.bottom = cy;
	lp += 2;
	if (nFlags < 0)
		lp = (BYTE huge *)lp + ((long)((int FAR *)g_lpPicClut)[9] << 3) + 16;
	cbRow = nDepth == 8 ? cx + cx : (cx + 3) & ~3;
	cb = (long)cy * cbRow;
	lpBits = GAllocLock(0, cb);
	if (lpBits) {
		if (nDepth == 4) {
			lpBase = (BYTE huge *)lpBits + (cb - cbRow);
			for (i = 0; i < cx; i++) {
				lpRow = lpBase + i;
				UnpackColumn4(&lp, lpRow, cy, cbRow);
			}
		} else {
			lpBase = (BYTE huge *)lpBits + (cb - cbRow);
			for (i = 0; i < cx; i++) {
				lpRow = lpBase + i + i;
				UnpackColumn8(&lp, (WORD huge *)lpRow, cy, cx);
			}
		}
		if ((DWORD)lp & 1)
			lp++;
		*plp = lp;
		CreateImage(0, pimg, &rc, nDepth, 2, lpBits, g_hPicPal);
		GUnlockFree(lpBits);
	} else
		fOk = FALSE;
	return fOk;
}

// FUNCTION: AARCADE 0019:0822
void FAR PASCAL UnpackColumn1(LPSTR *plp, LPSTR lpDst, int cy, int cbStride)
{
	LPSTR lp;
	LPSTR lpRun;
	int n;
	int cnt;
	int b;
	int c;
	int nLeft;

	lp = *plp;
	nLeft = cy;
next:
	if (!nLeft)
		goto done;
	if ((c = *lp++) == 0)
		goto ext;
	if (c < 0)
		goto rep;
	nLeft -= c;
	c--;
lit:
	*lpDst = *lp ^ 0xFF;
	lp++;
	lpDst = lpDst + cbStride;
	if (--c != -1)
		goto lit;
	goto next;
rep:
	nLeft += c;
	c = ~c;
	b = *lp ^ 0xFFFF;
	lp++;
rep1:
	*lpDst = b;
	lpDst = (BYTE huge *)lpDst + cbStride;
	if (--c != -1)
		goto rep1;
	goto next;
ext:
	c = *lp;
	lp++;
	n = c;
	n >>= 4;
	c <<= 4;
	c &= 0xFF;
	c <<= 4;
	c |= (BYTE)*lp;
	lp++;
	c--;
	lpRun = lp;
ext1:
	lp = lpRun;
	cnt = n;
ext2:
	*lpDst = *lp ^ 0xFF;
	lp++;
	lpDst = lpDst + cbStride;
	if (--cnt != -1)
		goto ext2;
	nLeft -= n;
	nLeft--;
	if (--c != -1)
		goto ext1;
	goto next;
done:
	*plp = lp;
}

// FUNCTION: AARCADE 0019:096C
void FAR PASCAL UnpackColumn4(LPSTR *plp, BYTE huge *lpDst, int cy, int cbStride)
{
	LPSTR lp;
	LPSTR lpRun;
	BYTE *ptbl;
	int n;
	int cnt;
	int b;
	int c;
	int nLeft;

	/* empty then-branch: the original has a jne to the next instruction */
	if ((ptbl = (BYTE *)LocalLock(g_hPicXlat)) == NULL) {
	}
	lp = *plp;
	nLeft = cy;
next:
	if (!nLeft)
		goto done;
	if ((c = *lp++) == 0)
		goto ext;
	if (c < 0)
		goto rep;
	nLeft -= c;
	c--;
lit:
	b = (BYTE)*lp;
	lp++;
	*lpDst = ptbl[b];
	lpDst = lpDst - cbStride;
	if (--c != -1)
		goto lit;
	goto next;
rep:
	nLeft += c;
	c = ~c;
	b = (BYTE)*lp;
	lp++;
	b = ptbl[b];
rep1:
	*lpDst = b;
	lpDst = lpDst - cbStride;
	if (--c != -1)
		goto rep1;
	goto next;
ext:
	c = (BYTE)*lp;
	lp++;
	n = c;
	n >>= 4;
	c <<= 4;
	c &= 0xFF;
	c <<= 4;
	c |= (BYTE)*lp;
	lp++;
	c--;
	lpRun = lp;
ext1:
	lp = lpRun;
	cnt = n;
ext2:
	b = (BYTE)*lp;
	lp++;
	*lpDst = ptbl[b];
	lpDst = lpDst - cbStride;
	if (--cnt != -1)
		goto ext2;
	nLeft -= n;
	nLeft--;
	if (--c != -1)
		goto ext1;
	goto next;
done:
	LocalUnlock(g_hPicXlat);
	*plp = lp;
}

// FUNCTION: AARCADE 0019:0B1C
void FAR PASCAL UnpackColumn8(LPSTR *plp, WORD huge *lpDst, int cy, int cStride)
{
	LPSTR lp;
	LPSTR lpRun;
	WORD *ptbl;
	int n;
	int cnt;
	int b;
	int c;
	int nLeft;

	/* empty then-branch: the original has a jne to the next instruction */
	if ((ptbl = (WORD *)LocalLock(g_hPicXlat)) == NULL) {
	}
	lp = *plp;
	nLeft = cy;
next:
	if (!nLeft)
		goto done;
	if ((c = *lp++) == 0)
		goto ext;
	if (c < 0)
		goto rep;
	nLeft -= c;
	c--;
lit:
	b = (BYTE)*lp;
	lp++;
	*lpDst = ptbl[b];
	lpDst = lpDst - cStride;
	if (--c != -1)
		goto lit;
	goto next;
rep:
	nLeft += c;
	c = ~c;
	b = (BYTE)*lp;
	lp++;
	b = ptbl[b];
rep1:
	*lpDst = b;
	lpDst = lpDst - cStride;
	if (--c != -1)
		goto rep1;
	goto next;
ext:
	c = (BYTE)*lp;
	lp++;
	n = c;
	n >>= 4;
	c <<= 4;
	c &= 0xFF;
	c <<= 4;
	c |= (BYTE)*lp;
	lp++;
	c--;
	lpRun = lp;
ext1:
	lp = lpRun;
	cnt = n;
ext2:
	b = (BYTE)*lp;
	lp++;
	*lpDst = ptbl[b];
	lpDst = lpDst - cStride;
	if (--cnt != -1)
		goto ext2;
	nLeft -= n;
	nLeft--;
	if (--c != -1)
		goto ext1;
	goto next;
done:
	LocalUnlock(g_hPicXlat);
	*plp = lp;
}
