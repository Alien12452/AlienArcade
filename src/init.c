#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include "aarcade.h"

/* App startup: previous-instance check, register class, create window, palette. */

/* The two "" literals in InitControls follow g_szMainClass (0024:006A, 0024:006B). */
// GLOBAL: AARCADE 0024:005A
BOOL g_fBlackWhite = FALSE;

// GLOBAL: AARCADE 0024:005C
char g_szMainClass[] = "AArcadeWClass";

// FUNCTION: AARCADE 0003:0000
void FAR PASCAL ParseCmdLine(LPSTR lpCmdLine)
{
	int i;

	for (i = 0; lpCmdLine[i]; ) {
		if (i == 0 || lpCmdLine[i] == ' ')
			i = ParseOption(lpCmdLine, i);
		else
			i++;
	}
}

// FUNCTION: AARCADE 0003:003A
int FAR PASCAL ParseOption(LPSTR lpCmdLine, int i)
{
	char c1, c2, u1, u2;

	while (lpCmdLine[i] == ' ')
		i++;
	if (lpCmdLine[i] == '-' || lpCmdLine[i] == '/') {
		if ((c1 = lpCmdLine[i + 1]) != 0)
			c2 = lpCmdLine[i + 2];
		else
			c2 = 0;
		u1 = toupper(c1);
		u2 = toupper(c2);
		if (u1 == 'B' && u2 == 'W') {
			g_fBlackWhite = TRUE;
			i += 2;
		} else if (u1 == 'N' && u2 == 'S') {
			g_fSoundOn = TRUE;	/* sic: /NS turns sound on; AARCADE.INI is read later */
			i += 2;
		}
	}
	return i;
}

// FUNCTION: AARCADE 0003:00D9
BOOL FAR PASCAL InitApplication(HINSTANCE hInstance)
{
	WNDCLASS wc;

	g_hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(2));
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = g_hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szMainClass;
	return RegisterClass(&wc);
}

// FUNCTION: AARCADE 0003:0150
HWND FAR PASCAL CreateMainWindow(HINSTANCE hInstance, int nCmdShow)
{
	RECT rc;
	HWND hwnd;

	g_hInstance = hInstance;
	GetWindowRect(GetDesktopWindow(), &rc);
	if (rc.bottom - rc.top < 342 || rc.right - rc.left < 512) {
		ShowMessage(NULL, 8, MB_ICONSTOP);
		return 0;
	}
	hwnd = CreateWindow(g_szMainClass, g_szAppTitle, WS_POPUP,
	                    rc.left, rc.top, rc.right, rc.bottom, NULL, NULL, hInstance, NULL);
	if (!hwnd)
		return 0;
	InstallMsgHook(hInstance, hwnd);
	return hwnd;
}

// FUNCTION: AARCADE 0003:01D6
BOOL FAR PASCAL InitGame(void)
{
	HDC hdc;
	int n;

	if (LoadConfig())
		return FALSE;
	srand((unsigned)time(NULL));
	hdc = GetDC(g_hwndMain);
	if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
		n = GetDeviceCaps(hdc, COLORRES);
	else {
		/* Block-scoped and in this order: they share stack slots with
		   dwFree below, and nPlanes must get the higher one. */
		int nPlanes, nBits;

		nBits = GetDeviceCaps(hdc, BITSPIXEL);
		nPlanes = GetDeviceCaps(hdc, PLANES);
		n = nBits * nPlanes;
	}
	if (g_fBlackWhite)
		n = 1;
	g_nColorDepth = n;
	ReleaseDC(g_hwndMain, hdc);
	if (g_szColorDll[0] && n >= 4) {
		if (n > 4)
			n = 4;
	} else if (g_szBWDll[0]) {
		n = 1;
	} else {
		ShowMessage(g_hwndMain, 6, MB_ICONSTOP);
		return FALSE;
	}
	g_fColor = n >= 4;
	g_nDepth2 = g_nDepth = g_fColor ? n : 1;
	{
		DWORD dwFree;
		DWORD dwCompact;

		dwCompact = GlobalCompact(g_nColorDepth <= 4 ? 0x180000L : 0x200000L);
		dwFree = GetFreeSpace(0);
		if (g_nColorDepth == 1 && dwCompact < 0xB4000L) {
			ShowMessage(g_hwndMain, 2, MB_ICONSTOP);
			return FALSE;
		}
		if (g_nColorDepth == 4 && dwCompact < 0xE1800L) {
			ShowMessage(g_hwndMain, 3, MB_ICONSTOP);
			return FALSE;
		}
		if (g_nColorDepth >= 8 && dwCompact < 0x152000L) {
			ShowMessage(g_hwndMain, 4, MB_ICONSTOP);
			return FALSE;
		}
	}
	g_hGameFont = CreateGameFont(0, 2, 9, 1);
	return TRUE;
}

// FUNCTION: AARCADE 0003:034D
BOOL FAR PASCAL CreateGamePalette(HWND hwnd)
{
	HPALETTE hPal = 0;
	long i;
	BOOL fOk = TRUE;
	long nColors;
	long nReserved;
	long j;
	HDC hdc;
	NPLOGPALETTE plp;

	if (g_nDepth > 1) {
		if (g_nColorDepth > 4) {
			for (i = 0; i < 16; i++)
				g_rgbGame[i] = g_rgbCustom[i];
		} else {
			for (i = 0; i < 16; i++)
				g_rgbGame[i] = g_rgbVga[i];
		}
		hdc = GetDC(hwnd);
		/* if/else, not ?:, here and for plp: the branches share their tail. */
		if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
			nColors = GetDeviceCaps(hdc, SIZEPALETTE);
		else
			nColors = GetDeviceCaps(hdc, NUMCOLORS);
		nReserved = GetDeviceCaps(hdc, NUMRESERVED);
		if (nColors)
			plp = (NPLOGPALETTE)LocalAlloc(LPTR, sizeof(LOGPALETTE) + (WORD)nColors * sizeof(PALETTEENTRY));
		else
			plp = NULL;
		if (plp) {
			GetSystemPaletteEntries(hdc, 0, (WORD)nColors, plp->palPalEntry);
			plp->palPalEntry[0].peRed = 0;
			plp->palPalEntry[0].peGreen = 0;
			plp->palPalEntry[0].peBlue = 0;
			plp->palPalEntry[0].peFlags = 0;
			plp->palPalEntry[nColors - 1].peRed = 0xFF;
			plp->palPalEntry[nColors - 1].peGreen = 0xFF;
			plp->palPalEntry[nColors - 1].peBlue = 0xFF;
			plp->palPalEntry[nColors - 1].peFlags = 0;
			for (i = nReserved; i < nReserved + 15 && i < nColors; i++) {
				j = i - nReserved + 1;
				plp->palPalEntry[i].peRed = g_rgbGame[j].rgbRed;
				plp->palPalEntry[i].peGreen = g_rgbGame[j].rgbGreen;
				plp->palPalEntry[i].peBlue = g_rgbGame[j].rgbBlue;
				plp->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
			}
			plp->palNumEntries = (WORD)nColors;
			plp->palVersion = 0x300;
			hPal = CreatePalette(plp);
			LocalFree((HLOCAL)plp);
			if (hPal) {
				SelectPalette(hdc, hPal, FALSE);
				RealizePalette(hdc);
			}
		}
		ReleaseDC(hwnd, hdc);
	}
	g_hPalette = hPal;
	if (fOk) {
		g_hGdiObjects[4] = MakeBrush(PALETTERGB(g_rgbGame[3].rgbRed, g_rgbGame[3].rgbGreen, g_rgbGame[3].rgbBlue));
		g_hGdiObjects[3] = MakeBrush(RGB(192, 192, 192));
		g_hGdiObjects[2] = MakeBrush(PALETTERGB(g_rgbGame[12].rgbRed, g_rgbGame[12].rgbGreen, g_rgbGame[12].rgbBlue));
		g_hGdiObjects[1] = MakeBrush(PALETTERGB(g_rgbGame[8].rgbRed, g_rgbGame[8].rgbGreen, g_rgbGame[8].rgbBlue));
		g_hGdiObjects[0] = MakeBrush(PALETTERGB(g_rgbGame[5].rgbRed, g_rgbGame[5].rgbGreen, g_rgbGame[5].rgbBlue));
	}
	return fOk;
}

// FUNCTION: AARCADE 0003:0659
HBRUSH FAR PASCAL MakeBrush(COLORREF cr)
{
	return CreateSolidBrush(cr);
}

// FUNCTION: AARCADE 0003:066F
BOOL FAR PASCAL InitControls(HINSTANCE hInstance)
{
	HDC hdc;
	HPALETTE hPal;
	WNDCLASS wc;
	HWND hwnd;

	wc.lpszClassName = g_szSwampGasClass;
	wc.style = CS_SAVEBITS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = SwampGasDlgClsProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 30;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = COLOR_WINDOW + 1;
	wc.lpszMenuName = NULL;
	RegisterClass(&wc);

	ParamText(NULL, NULL, NULL, NULL);
	g_nUserWndExtra = g_nUserClsExtra = 0;

	wc.style = CS_PARENTDC;
	wc.lpszClassName = g_szUserItemClass;
	wc.lpfnWndProc = UserWndProc;
	wc.cbClsExtra = g_nUserClsExtra + 8;
	wc.cbWndExtra = g_nUserWndExtra + 24;
	wc.hInstance = hInstance;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	if (!RegisterClass(&wc))
		return FALSE;

	GetClassInfo(NULL, g_szUserItemClass, &wc);
	wc.lpszClassName = g_szUserDblClkClass;
	wc.style = wc.style | CS_DBLCLKS;
	if (!RegisterClass(&wc))
		return FALSE;

	GetClassInfo(NULL, g_szUserItemClass, &wc);
	wc.lpszClassName = g_szAnimItemClass;
	wc.style = (wc.style | CS_OWNDC) & ~CS_PARENTDC;
	if (!RegisterClass(&wc))
		return FALSE;

	GetClassInfo(NULL, g_szUserItemClass, &wc);
	wc.lpszClassName = g_szPicItemClass;
	if (!RegisterClass(&wc))
		return FALSE;
	hwnd = CreateWindow(g_szPicItemClass, "", WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if (hwnd) {
		SetClassLong(hwnd, g_nUserClsExtra, (LONG)(FARPROC)PicItemProc);
		DestroyWindow(hwnd);
	} else
		return FALSE;

	GetClassInfo(NULL, g_szUserItemClass, &wc);
	wc.lpszClassName = g_szParamTextClass;
	if (!RegisterClass(&wc))
		return FALSE;
	hwnd = CreateWindow(g_szParamTextClass, "", WS_POPUP, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if (hwnd) {
		SetClassLong(hwnd, g_nUserClsExtra, (LONG)(FARPROC)ParamTextProc);
		DestroyWindow(hwnd);
	} else
		return FALSE;

	hdc = GetDC(g_hwndMain);
	hPal = GetGamePalette();
	g_hdcMemA = CreateCompatibleDC(hdc);
	g_hdcMemB = CreateCompatibleDC(hdc);
	if (hPal) {
		SelectPalette(g_hdcMemA, hPal, FALSE);
		RealizePalette(g_hdcMemA);
		SelectPalette(g_hdcMemB, hPal, FALSE);
		RealizePalette(g_hdcMemB);
	}
	ReleaseDC(g_hwndMain, hdc);
	g_hCursor = LoadCursor(g_hInstance, MAKEINTRESOURCE(100));
	return TRUE;
}
