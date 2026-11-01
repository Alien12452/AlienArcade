#include "aarcade.h"

/* Main-window layout and painting (cabinet frame around the playfield). */

int cdecl rand(void);

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */

/* 0024:38B2. Nothing refers to these bytes. */
// GLOBAL: AARCADE 0024:38B2
char g_abBss38B2[6];


// GLOBAL: AARCADE 0024:38AA
unsigned g_anDailyRank[4];


// GLOBAL: AARCADE 0024:38A2
unsigned g_anAllTimeRank[4];

/* 0024:387E. Nothing refers to these bytes. */
// GLOBAL: AARCADE 0024:387E
char g_abBss387E[36];


// GLOBAL: AARCADE 0024:387C
BOOL g_fColor;


// GLOBAL: AARCADE 0024:387A
int g_nDepth;


// GLOBAL: AARCADE 0024:3878
int g_nDepth2;

/* 0024:3874. Nothing refers to these bytes. */
// GLOBAL: AARCADE 0024:3874
char g_abBss3874[4];


// GLOBAL: AARCADE 0024:386C
RECT g_rcPlaySrc;


/* Main client rect moved so its top-left is the origin. */
// GLOBAL: AARCADE 0024:3864
RECT g_rcMain;


// GLOBAL: AARCADE 0024:385A
HGDIOBJ g_hGdiObjects[5];


/* How many round-1 enemies are still required. */
// GLOBAL: AARCADE 0024:3858
int g_nR1Left;


/* Views 1–3 start this at 20 and count it down. */
// GLOBAL: AARCADE 0024:3856
int g_nPlayAmmo;


/* Reset around the call that opens a round; other segments count it up. */
// GLOBAL: AARCADE 0024:3854
int g_nPlayWave;


/* Seconds left in the round, shown as m:ss in the status bar. */
// GLOBAL: AARCADE 0024:3850
long g_lTimeLeft;


// GLOBAL: AARCADE 0024:384C
long g_lPlayScore;


// GLOBAL: AARCADE 0024:3844
RECT g_rcPlay;


/* Top, left, right, and bottom pieces around the playfield. */
// GLOBAL: AARCADE 0024:3824
RECT g_arcFrame[4];


/* 0, 10, 20, or 30: which set of frame pictures was picked. */
// GLOBAL: AARCADE 0024:3822
int g_nLayout;

// FUNCTION: AARCADE 000E:0000
void FAR PASCAL LayoutMain(HWND hwnd)
{
	int nHeight, nWidth;
	RECT rc, rcClient, rcMargin;
	HBITMAP hbm;

	g_fLayoutFits = 0;
	GetClientRect(hwnd, &rcClient);
	rc = rcClient;
	g_rcPlaySrc.top = (rc.bottom + rc.top + -0x156) / 2;
	g_rcPlaySrc.left = (rc.right + rc.left + -0x200) / 2;
	g_rcPlaySrc.bottom = g_rcPlaySrc.top + 0x156;
	g_rcPlaySrc.right = g_rcPlaySrc.left + 0x200;
	g_rcMain = rc;
	OffsetRect(&g_rcMain, -rcClient.left, -rcClient.top);
	/* assignment in the test keeps the value in AX */
	if ((nWidth = rc.right - rc.left) < 0x200)
		return;
	if ((nHeight = rc.bottom - rc.top) < 0x156)
		return;
	g_fLayoutFits = 1;
	g_rcLayout = g_rcPlaySrc;
	OffsetRect(&g_rcLayout, -rcClient.left, -rcClient.top);
	g_rcLayout.left -= 0x42;
	g_rcLayout.top -= 0x57;
	g_rcLayout.right += 0x3e;
	g_rcLayout.bottom += 0x33;
	g_nLayout = (rand() & 3) * 10;
	if (g_nLayout == 0x1e) {
		g_fAltLayout = 1;
		SetRect(&rcMargin, 0x6e, 0x79, 0x10, 0x0f);
	} else {
		g_fAltLayout = 0;
		SetRect(&rcMargin, 0x3f, 0x55, 0x3f, 0x33);
	}
	SetRect(&g_arcFrame[0], g_rcLayout.left, g_rcLayout.top, g_rcLayout.right,
		g_rcLayout.top + rcMargin.top);
	SetRect(&g_arcFrame[1], g_rcLayout.left, g_rcLayout.top + rcMargin.top,
		g_rcLayout.left + rcMargin.left, g_rcLayout.bottom - rcMargin.bottom);
	SetRect(&g_arcFrame[2], g_rcLayout.right - rcMargin.right,
		g_rcLayout.top + rcMargin.top, g_rcLayout.right,
		g_rcLayout.bottom - rcMargin.bottom);
	SetRect(&g_arcFrame[3], g_rcLayout.left, g_rcLayout.bottom - rcMargin.bottom,
		g_rcLayout.right, g_rcLayout.bottom);
	/* four separate preloads, one call for each picture */
	hbm = LoadPPic(g_nLayout + 0x190, 0, TRUE);
	FreePPic(hbm, FALSE);
	hbm = LoadPPic(g_nLayout + 0x191, 0, TRUE);
	FreePPic(hbm, FALSE);
	hbm = LoadPPic(g_nLayout + 0x192, 0, TRUE);
	FreePPic(hbm, FALSE);
	hbm = LoadPPic(g_nLayout + 0x193, 0, TRUE);
	FreePPic(hbm, FALSE);
}

// FUNCTION: AARCADE 000E:0214
void FAR PASCAL PaintMain(HWND hwnd)
{
	HDC hdc;
	RECT rcClient, rcHit;
	PAINTSTRUCT ps;
	int i;

	hdc = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rcClient);
	if (g_fLayoutFits != 0) {
		for (i = 0; i < 4; i++) {
			if (IntersectRect(&rcHit, &g_arcFrame[i], &rcClient))
				DrawItemPic(hdc, i + g_nLayout + 0x190, &g_arcFrame[i]);
		}
	}
	EndPaint(hwnd, &ps);
}
