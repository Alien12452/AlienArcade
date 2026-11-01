#include "aarcade.h"

/* Pascal strings: a length byte, then that many characters. */

// FUNCTION: AARCADE 0009:0000
void FAR PASCAL CopyPStr(LPSTR lpSrc, LPSTR lpDst)
{
	/* the count is the length byte, so the loop copies it too */
	int n;

	n = (BYTE)*lpSrc;
	do {
		*lpDst = *lpSrc;
		lpSrc++;
		lpDst++;
	} while (--n >= 0);
}

// FUNCTION: AARCADE 0009:0028
int FAR PASCAL PStrDiff(LPSTR lpA, LPSTR lpB)
{
	int n;

	n = (BYTE)*lpA;
	do {
		if (*lpA++ != *lpB++)
			return 1;
	} while (--n >= 0);
	return 0;
}

// FUNCTION: AARCADE 0009:005B
void FAR PASCAL SplicePStr(LPSTR lpStr, BYTE nPos, BYTE nDel, LPSTR lpIns)
{
	LPSTR lpTail;
	int n;
	int nLen;
	int iPos, iDel;
	int c;

	iPos = nPos;
	iDel = nDel;
	n = -iDel;
	if (lpIns != 0) {
		n = (BYTE)*lpIns - iDel;
		if (n > 0) {
			nLen = (BYTE)*lpStr;
			c = nLen - iPos - iDel;
			*lpStr = (BYTE)*lpStr + (BYTE)n;
			lpStr++;
			lpStr += nLen;
			lpTail = lpStr + n;
			while (--c >= 0) {
				lpTail--;
				lpStr--;
				*lpTail = *lpStr;
			}
		} else
			lpTail = lpStr + iPos + iDel + iDel + n + 1;
		c = (BYTE)*lpIns;
		lpIns++;
		lpIns += c;
		while (--c >= 0) {
			lpTail--;
			lpIns--;
			*lpTail = *lpIns;
		}
	}
	if (n < 0) {
		c = (BYTE)*lpStr - iPos - iDel;
		*lpStr = (BYTE)*lpStr + (BYTE)n;
		lpStr += iPos + iDel + 1;
		lpTail = lpStr + n;
		while (--c >= 0) {
			*lpTail = *lpStr;
			lpStr++;
			lpTail++;
		}
	}
}

// FUNCTION: AARCADE 0009:0180
void FAR PASCAL InsertPStr(LPSTR lpIns, LPSTR lpStr, BYTE nPos)
{
	SplicePStr(lpStr, nPos, 0, lpIns);
}
