#include <time.h>
#include "aarcade.h"

/* In-play status bar: score, time left, wave, ammo, and pause overlay. */

/* Uninitialized data. aarcade.h declares these highest address first;
     Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:434E
long g_tTimeLast;


/* Status bar: the time last drawn and the time() it was last counted down from. */
// GLOBAL: AARCADE 0024:434A
long g_lTimeShown;

// FUNCTION: AARCADE 001D:0000
void FAR PASCAL PadNumString(long n, int nWidth, LPSTR lpps)
{
	char sz[256];

	NumToString(n, lpps);
	if ((BYTE)lpps[0] < nWidth) {
		CopyPStr("\x06" "000000", sz);
		sz[0] = nWidth - lpps[0];
		InsertPStr(sz, lpps, 0);
	}
}

// FUNCTION: AARCADE 001D:005A
void FAR PASCAL StatusTextOut(HDC hdc, LPSTR lpps)
{
	DWORD crText, crBk;
	int nBkMode;
	HGDIOBJ hfont;

	if (g_fColor)
		crText = SetTextColor(hdc, PALETTERGB(g_rgbGame[3].rgbRed, g_rgbGame[3].rgbGreen, g_rgbGame[3].rgbBlue));
	else
		crText = SetTextColor(hdc, RGB(0xFF, 0xFF, 0xFF));
	crBk = SetBkColor(hdc, RGB(0, 0, 0));
	nBkMode = SetBkMode(hdc, OPAQUE);
	hfont = SelectObject(hdc, g_hGameFont);
	TextOutPascal(hdc, lpps);
	SelectObject(hdc, hfont);
	SetBkMode(hdc, nBkMode);
	SetTextColor(hdc, crText);
	SetBkColor(hdc, crBk);
}

// FUNCTION: AARCADE 001D:010A
void FAR PASCAL DrawPlayAmmo(HDC hdc)
{
	char sz[256];

	PadNumString((long)g_nPlayAmmo, 3, sz);
	MoveTo(hdc, 0x56, 0x153);
	StatusTextOut(hdc, sz);
}

// FUNCTION: AARCADE 001D:0144
void FAR PASCAL AddPlayAmmo(long n)
{
	HDC hdc;

	g_nPlayAmmo = g_nPlayAmmo + (int)n;
	if (g_nPlayAmmo < 0)
		g_nPlayAmmo = 0;
	hdc = GetDC(g_hwndChild);
	DrawPlayAmmo(hdc);
	ReleaseDC(g_hwndChild, hdc);
}

// FUNCTION: AARCADE 001D:017F
void FAR PASCAL DrawR5Parts(HDC hdc)
{
	char FAR *p;
	char szNum[256];
	char sz[256];

	p = sz;
	*p++ = 8;
	PadNumString((long)g_anR5Count[0], 2, szNum);
	*p++ = szNum[1];
	*p++ = szNum[2];
	*p++ = '-';
	PadNumString((long)g_anR5Count[1], 2, szNum);
	*p++ = szNum[1];
	*p++ = szNum[2];
	*p++ = '-';
	PadNumString((long)g_anR5Count[2], 2, szNum);
	*p++ = szNum[1];
	*p++ = szNum[2];
	MoveTo(hdc, 0x4B, 0x153);
	StatusTextOut(hdc, sz);
}

// FUNCTION: AARCADE 001D:0253
void FAR PASCAL DrawR5Droids(HDC hdc)
{
	char sz[256];

	PadNumString((long)g_anR5Count[3], 2, sz);
	MoveTo(hdc, 0x150, 0x153);
	StatusTextOut(hdc, sz);
}

// FUNCTION: AARCADE 001D:028E
void FAR PASCAL AddR5Count(int n, int i)
{
	int v;
	HDC hdc;

	hdc = GetDC(g_hwndChild);
	v = g_anR5Count[i];
	v += n;
	if (v < 0)
		v = 0;
	g_anR5Count[i] = v;
	if (i == 3)
		DrawR5Droids(hdc);
	else
		DrawR5Parts(hdc);
	ReleaseDC(g_hwndChild, hdc);
}

// FUNCTION: AARCADE 001D:02E7
void FAR PASCAL DrawPlayWave(HDC hdc)
{
	char sz[256];
	int n;

	n = g_nPlayWave;
	if (n < 0)
		n = 0;
	PadNumString((long)(n + 1), 2, sz);
	MoveTo(hdc, 0xCF, 0x153);
	StatusTextOut(hdc, sz);
}

// FUNCTION: AARCADE 001D:032E
void FAR PASCAL NextPlayWave(void)
{
	HDC hdc;

	g_nPlayWave++;
	if (g_nPlayWave > 98)
		g_nPlayWave = 98;
	hdc = GetDC(g_hwndChild);
	DrawPlayWave(hdc);
	ReleaseDC(g_hwndChild, hdc);
}

// FUNCTION: AARCADE 001D:0362
void FAR PASCAL DrawTimeLeft(HDC hdc)
{
	char sz[256];

	FormatTime(g_lTimeLeft, sz);
	MoveTo(hdc, 0x131, 0x153);
	StatusTextOut(hdc, sz);
	g_lTimeShown = g_lTimeLeft;
}

// FUNCTION: AARCADE 001D:03AC
void FAR PASCAL TickTimeLeft(void)
{
	HDC hdc;

	g_lTimeLeft -= time(NULL) - g_tTimeLast;
	if (g_lTimeLeft < 0)
		g_lTimeLeft = 0;
	g_tTimeLast = time(NULL);
	if (g_lTimeLeft != g_lTimeShown) {
		hdc = GetDC(g_hwndChild);
		DrawTimeLeft(hdc);
		ReleaseDC(g_hwndChild, hdc);
	}
}

// FUNCTION: AARCADE 001D:0425
void FAR PASCAL PauseTimeLeft(void)
{
	TickTimeLeft();
}

// FUNCTION: AARCADE 001D:0430
void FAR PASCAL ResumeTimeLeft(void)
{
	g_tTimeLast = time(NULL);
}

// FUNCTION: AARCADE 001D:0446
void FAR PASCAL SetTimeLeft(long t)
{
	g_lTimeLeft = t;
	g_tTimeLast = time(NULL);
	g_lTimeShown = 0;
}

// FUNCTION: AARCADE 001D:0477
void FAR PASCAL DrawPlayScore(HDC hdc)
{
	char sz[256];

	PadNumString(g_lPlayScore, 6, sz);
	MoveTo(hdc, 0x1A9, 0x153);
	StatusTextOut(hdc, sz);
}

// FUNCTION: AARCADE 001D:04B4
void FAR PASCAL AddPlayScore(long n)
{
	HDC hdc;
	long l;

	if (g_nRound == 6) {
		l = g_lPlayScore % 5000;
		if (l + n >= 5000) {
			AddPlayAmmo(1L);
			PlaySfx(0x4653, 1);
		}
	}
	g_lPlayScore += n;
	if (g_lPlayScore < 0)
		g_lPlayScore = 0;
	hdc = GetDC(g_hwndChild);
	DrawPlayScore(hdc);
	ReleaseDC(g_hwndChild, hdc);
}

// FUNCTION: AARCADE 001D:054F
void FAR PASCAL DrawStatusBar(void)
{
	HDC hdc;
	int nId;
	RECT rc;
	char sz[256];

	nId = -1;
	hdc = GetDC(g_hwndChild);
	SetRect(&rc, 0, 0x146, 0x200, 0x156);
	FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
	switch (g_nRound - 1) {
	case 0:
		MoveTo(hdc, 0x33, 0x153);
		nId = 1;
		break;
	case 1:
		MoveTo(hdc, 0x20, 0x153);
		nId = 2;
		break;
	case 2:
		MoveTo(hdc, 0x17, 0x153);
		nId = 3;
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		MoveTo(hdc, 0x28, 0x153);
		nId = 25;
		break;
	}
	if (nId >= 0) {
		GetIndString(sz, 0x80, nId);
		StatusTextOut(hdc, sz);
		DrawPlayAmmo(hdc);
	} else if (g_nRound == 5) {
		MoveTo(hdc, 0x20, 0x153);
		GetIndString(sz, 0x80, 4);
		StatusTextOut(hdc, sz);
		DrawR5Parts(hdc);
	} else {
		MoveTo(hdc, 0x20, 0x153);
		GetIndString(sz, 0x80, 18);
		StatusTextOut(hdc, sz);
	}
	MoveTo(hdc, 0xA3, 0x153);
	GetIndString(sz, 0x80, 5);
	StatusTextOut(hdc, sz);
	DrawPlayWave(hdc);
	if (g_nRound == 5) {
		MoveTo(hdc, 0x101, 0x153);
		GetIndString(sz, 0x80, 23);
		StatusTextOut(hdc, sz);
		DrawR5Droids(hdc);
	} else {
		MoveTo(hdc, 0x10A, 0x153);
		GetIndString(sz, 0x80, 6);
		StatusTextOut(hdc, sz);
		DrawTimeLeft(hdc);
	}
	MoveTo(hdc, 0x17D, 0x153);
	GetIndString(sz, 0x80, 7);
	StatusTextOut(hdc, sz);
	DrawPlayScore(hdc);
	ReleaseDC(g_hwndChild, hdc);
}

// FUNCTION: AARCADE 001D:0737
void FAR PASCAL FormatTime(long t, LPSTR lpps)
{
	int n;

	lpps[0] = 5;
	n = (int)(t / 600);
	t -= n * 600;
	if (n > 9)
		n = 9;
	lpps[1] = n + '0';
	n = (int)(t / 60);
	t -= n * 60;
	lpps[2] = n + '0';
	lpps[3] = ':';
	n = (int)(t / 10);
	t -= n * 10;
	lpps[4] = n + '0';
	lpps[5] = (char)t + '0';
}
