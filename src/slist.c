#include "aarcade.h"

/* In-memory high-score lists (daily and all-time) and day-of-year checks. */

/* Near pointer: time.h's prototype is far and would push two zeros. */
long cdecl time(long *);

struct tm {
	int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst;
};

struct tm *cdecl localtime(long *);

/* Header of a score global: stride, count, maximum, day-of-year. */

// FUNCTION: AARCADE 0007:0000
HGLOBAL FAR PASCAL AllocScoreList(int cbName, int nMax, int fAllTime)
{
	HGLOBAL h;
	int cb;
	int FAR *lp;
	long t;
	struct tm FAR *ptm;

	h = GlobalAlloc(GMEM_MOVEABLE, 8);
	if (h) {
		lp = (int FAR *)GLock(h);
		cb = cbName + 5;
		if (cb % 2)
			cb++;
		lp[0] = cb;
		lp[1] = 0;
		lp[2] = nMax;
		if (fAllTime != 0)
			lp[3] = 0;
		else {
			time(&t);
			ptm = localtime(&t);
			lp[3] = ptm->tm_yday;
		}
		GUnlock(h);
	}
	return h;
}

// FUNCTION: AARCADE 0007:009E
void FAR PASCAL SetScoreDay(HGLOBAL h, int nDay, int fAllTime)
{
	int FAR *lp;

	if (h) {
		lp = (int FAR *)GLock(h);
		if (fAllTime != 0)
			lp[3] = 0;
		else
			lp[3] = nDay;
		GUnlock(h);
	}
}

// FUNCTION: AARCADE 0007:00E0
void FAR PASCAL ResetScoreList(HGLOBAL h)
{
	int FAR *lp;

	if (h) {
		lp = (int FAR *)GLock(h);
		lp[1] = 0;
		GUnlock(h);
		GlobalReAlloc(h, 8, GMEM_ZEROINIT);
	}
}

// FUNCTION: AARCADE 0007:011C
int FAR PASCAL GetYDay(void)
{
	long t;
	struct tm FAR *ptm;

	time(&t);
	ptm = localtime(&t);
	return ptm->tm_yday;
}

// FUNCTION: AARCADE 0007:0149
void FAR PASCAL CheckScoreDay(HGLOBAL h)
{
	int FAR *lp;
	int nToday;

	if (h) {
		lp = (int FAR *)GLock(h);
		if (lp[3] != 0) {
			nToday = GetYDay();
			if (lp[3] != nToday) {
				lp[3] = nToday;
				ResetScoreList(h);
			}
		}
		GUnlock(h);
	}
}

// FUNCTION: AARCADE 0007:0197
int FAR PASCAL ScoreQualifies(HGLOBAL h, long lScore)
{
	BYTE FAR *lp;
	int cb, nCount, nMax;
	int i;

	/* return 0 sits after return i; an early return 0 would be emitted inline */
	if (lScore != 0 && h != 0) {
		CheckScoreDay(h);
		lp = (BYTE FAR *)GLock(h);
		nCount = ((int FAR *)lp)[1];
		nMax = ((int FAR *)lp)[2];
		cb = ((int FAR *)lp)[0];
		lp += 8;
		for (i = 1; i <= nCount; i++) {
			/* unsigned long compare: high word in AX, low in DX */
			if (*(unsigned long FAR *)lp <= (unsigned long)lScore)
				return i;
			lp += cb;
		}
		/* stays locked; falling out is the shared return 0 */
		if (i <= nMax)
			return i;
	}
	return 0;
}

// FUNCTION: AARCADE 0007:021E
int FAR PASCAL InsertScore(HGLOBAL h, long lScore, LPSTR lpsz)
{
	BYTE FAR *lp;
	int cb, nOff;
	char nLimit;
	int nMax;
	char sz[256];
	int nRank;

	nRank = 0;
	if (h != 0) {
		CheckScoreDay(h);
		nRank = ScoreQualifies(h, lScore);
		if (nRank != 0) {
			lp = (BYTE FAR *)GLock(h);
			cb = ((int FAR *)lp)[0];
			nOff = (nRank - 1) * cb + 8;
			GUnlock(h);
			nMax = ((int FAR *)lp)[2];
			if ((unsigned)((int FAR *)lp)[1] < (unsigned)nMax)
				((int FAR *)lp)[1]++;
			else
				GlobalReAlloc(h, (unsigned long)(unsigned)((nMax - 1) * cb + 8),
					GMEM_ZEROINIT);
			Munger(h, (long)nOff, NULL, 0L, sz, (long)cb);
			lp = (BYTE FAR *)GLock(h);
			lp += nOff;
			*(long FAR *)lp = lScore;
			lp += 4;
			CopyPStr(lpsz, sz);
			/* bcc emits add al,-4 for a byte subtract; this is sub al,4; dec al */
			_AL = (BYTE)cb;
			asm sub al, 4;
			asm dec al;
			nLimit = _AL;
			if ((BYTE)sz[0] > (BYTE)nLimit)
				sz[0] = nLimit;
			CopyPStr(sz, lp);
			GUnlock(h);
		}
	}
	return nRank;
}

// FUNCTION: AARCADE 0007:033B
void FAR PASCAL GetScoreEntry(HGLOBAL h, int i, long *plScore, LPSTR lpsz)
{
	BYTE FAR *lp;

	if (h) {
		CheckScoreDay(h);
		lp = (BYTE FAR *)GLock(h);
		*plScore = 0;
		*lpsz = 0;
		if ((unsigned)((int FAR *)lp)[1] >= (unsigned)i) {
			lp += (i - 1) * ((int FAR *)lp)[0] + 8;
			*plScore = *(long FAR *)lp;
			lp += 4;
			CopyPStr(lp, lpsz);
		}
		GUnlock(h);
	}
}

// FUNCTION: AARCADE 0007:03BE
void FAR PASCAL GetScoreInfo(HGLOBAL h, int *pcbName, int *pnCount, int *pnMax)
{
	int FAR *lp;

	if (h) {
		CheckScoreDay(h);
		lp = (int FAR *)GLock(h);
		/* sub ax,4; dec ax — one subtraction folds to add ax,-5 */
		_AX = lp[0];
		_AX -= 4;
		--_AX;
		*pcbName = _AX;
		*pnCount = lp[1];
		*pnMax = lp[2];
		GUnlock(h);
	}
}
