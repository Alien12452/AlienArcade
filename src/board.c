#include "aarcade.h"

/* High-score board painting for the between-game next screen. */

void FAR PASCAL DrawPictureAt(HDC hdc, int id, int x, int y);

// FUNCTION: AARCADE 0013:0000
void FAR PASCAL TextOutRight(HDC hdc, BYTE FAR *lpps, int x, int y)
{
	int nWidth;

	nWidth = (int)GetTextExtentPascal(hdc, lpps);
	MoveTo(hdc, x - nWidth, y);
	TextOutPascal(hdc, lpps);
}

// FUNCTION: AARCADE 0013:003C
void FAR PASCAL DrawRankRow(HDC hdc, HGLOBAL h, unsigned *pn, unsigned nRank, int x, int y)
{
	COLORREF crOld;
	int i;
	long lScore;
	char sz[256];

	crOld = SetTextColor(hdc, 0);
	for (i = 0; i < 4; i++) {
		if (*pn++ == nRank) {
			if (g_fColor)
				SetTextColor(hdc, PALETTERGB(g_rgbGame[9].rgbRed, g_rgbGame[9].rgbGreen, g_rgbGame[9].rgbBlue));
			break;
		}
	}
	GetScoreEntry(h, nRank, &lScore, sz);
	if (lScore != 0) {
		MoveTo(hdc, x + 0x36, y);
		TextOutPascal(hdc, (BYTE FAR *)sz);
		NumToString(lScore, sz);
		TextOutRight(hdc, (BYTE FAR *)sz, x + 0x2F, y);
	}
	SetTextColor(hdc, crOld);
	NumToString((long)nRank, sz);
	/* Pascal length byte; the index keeps AL, so it is not reloaded */
	sz[(unsigned char)++sz[0]] = ')';
	TextOutRight(hdc, (BYTE FAR *)sz, x, y);
}

// FUNCTION: AARCADE 0013:0152
void FAR PASCAL DrawRankList(HDC hdc, HGLOBAL h, unsigned *pn, int x, int y)
{
	int i;

	for (i = 1; i <= 5; i++)
		DrawRankRow(hdc, h, pn, i, x, y + (i - 1) * 0x0E);
	x += 0xBA;
	/* i - 6 is sub; the add immediate is i + -6 */
	for (i = 6; i <= 10; i++)
		DrawRankRow(hdc, h, pn, i, x, y + (i + -6) * 0x0E);
}

// FUNCTION: AARCADE 0013:01B4
void FAR PASCAL PaintNextScores(HDC hdc)
{
	HGDIOBJ hfont;
	int n;
	COLORREF crOld;
	char sz[256];

	if (g_fColor)
		crOld = SetBkColor(hdc, PALETTERGB(g_rgbGame[8].rgbRed, g_rgbGame[8].rgbGreen, g_rgbGame[8].rgbBlue));
	hfont = CreateGameFont(hdc, 2, 0x0C, 1);
	MoveTo(hdc, 0x59, 0x3B);
	GetIndString(sz, 0x80, 0x0C);
	TextOutPascal(hdc, (BYTE FAR *)sz);
	GetIndString(sz, 0x80, g_nRound + 0x0E);
	TextOutPascal(hdc, (BYTE FAR *)sz);
	GetIndString(sz, 0x80, 0x0E);
	TextOutPascal(hdc, (BYTE FAR *)sz);
	/* sub ax,imm: a plain minus of this constant is add ax,-n */
	_AX = GetCurrentPosition(hdc);
	_AX -= 0x59;
	n = _AX;
	MoveTo(hdc, 0x51, 0x3F);
	LineTo(hdc, n + 0x61, 0x3F);
	MoveTo(hdc, 0xAF, 0xB8);
	GetIndString(sz, 0x80, 0x0D);
	TextOutPascal(hdc, (BYTE FAR *)sz);
	GetIndString(sz, 0x80, g_nRound + 0x0E);
	TextOutPascal(hdc, (BYTE FAR *)sz);
	GetIndString(sz, 0x80, 0x0E);
	TextOutPascal(hdc, (BYTE FAR *)sz);
	_AX = GetCurrentPosition(hdc);
	_AX -= 0xAF;
	n = _AX;
	MoveTo(hdc, 0xA7, 0xBC);
	LineTo(hdc, n + 0xB7, 0xBC);
	DeleteObject(SelectObject(hdc, hfont));
	hfont = CreateGameFont(hdc, 2, 9, 0);
	DrawRankList(hdc, g_hDaily, g_anDailyRank, 0x32, 0x52);
	DrawPictureAt(hdc, 0x10E, 0x19F, 0x29);
	DrawPictureAt(hdc, 0x10F, 0x28, 0xA5);
	DrawRankList(hdc, g_hAllTime, g_anAllTimeRank, 0x8E, 0xCE);
	DeleteObject(SelectObject(hdc, hfont));
	if (g_fColor)
		SetBkColor(hdc, crOld);
}

// FUNCTION: AARCADE 0013:039D
void FAR PASCAL DrawPictureAt(HDC hdc, int id, int x, int y)
{
	RECT rc;
	HBITMAP hbm;

	hbm = LoadPicture(id, FALSE);
	if (hbm) {
		GetPictureRect(hbm, &rc);
		OffsetRect(&rc, x - rc.left, y - rc.top);
		DrawPicture(hdc, hbm, &rc);
	}
	FreePicture(hbm, FALSE);
}
