#include <mem.h>
#include <time.h>
#include "aarcade.h"

/* WinMain and MainWndProc: the top-level message loop and window. */

/* First game data in DGROUP, directly after the startup image. */
// GLOBAL: AARCADE 0024:0056
HPALETTE g_hPalette = 0;

// GLOBAL: AARCADE 0024:0058
int g_nInactive = 0;

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:2382
FARPROC g_lpfnModalDlgProc;


// GLOBAL: AARCADE 0024:2380
int g_nDlgData;


// GLOBAL: AARCADE 0024:237E
int g_nColorDepth;


// GLOBAL: AARCADE 0024:237C
HWND g_hwndMain;


// GLOBAL: AARCADE 0024:237A
HINSTANCE g_hInstance;


// GLOBAL: AARCADE 0024:2378
HICON g_hIcon;


// GLOBAL: AARCADE 0024:2374
POINT g_ptCursor;


// GLOBAL: AARCADE 0024:2372
HWND g_hwndCapture;

// FUNCTION: AARCADE 0002:0000
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HACCEL hAccel = 0;
	BOOL ok = TRUE;
	MSG msg;

	if (hPrevInstance) {
		ActivatePrevInstance(hInstance, hPrevInstance);
		return 0;
	}

	ParseCmdLine(lpCmdLine);
	if (!InitApplication(hInstance))
		return 0;

	if ((g_hwndMain = CreateMainWindow(hInstance, nCmdShow)) == 0)
		return 0;

	if ((ok = InitGame()) != 0) {
		CreateGamePalette(g_hwndMain);
		InitControls(hInstance);
		OpenSession();
		LayoutMain(g_hwndMain);
		ShowWindow(g_hwndMain, nCmdShow);
		UpdateWindow(g_hwndMain);
		hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(1));
		PostMainCommand(IDC_START);
	} else {
		DestroyWindow(g_hwndMain);
	}

	while (GetMessage(&msg, 0, 0, 0)) {
		if (TranslateAccelerator(g_hwndMain, hAccel, &msg))
			continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (ok) {
		TermControls();
		DeleteGdiObjects();
	}
	RemoveMsgHook();
	if (g_hIcon)
		DestroyIcon(g_hIcon);
	return msg.wParam;
}

// FUNCTION: AARCADE 0002:0123
void FAR PASCAL ActivatePrevInstance(HINSTANCE hInstance, HINSTANCE hPrevInstance)
{
	HWND hwnd;
	HWND hwndDesktop;

	hwndDesktop = GetDesktopWindow();
	for (hwnd = GetWindow(hwndDesktop, GW_CHILD); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT)) {
		if (GetWindowWord(hwnd, GWW_HINSTANCE) == (WORD)hPrevInstance)
			break;
	}
	if (hwnd) {
		if (IsIconic(hwnd))
			ShowWindow(hwnd, SW_SHOWNORMAL);
		SetActiveWindow(hwnd);
	}
}

// FUNCTION: AARCADE 0002:0179
LRESULT CALLBACK _export MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_QUERYNEWPALETTE: {
		HDC hdc;
		HPALETTE hOldPal;
		int nChanged;

		hdc = GetDC(hwnd);
		hOldPal = SelectPalette(hdc, g_hPalette, FALSE);
		nChanged = RealizePalette(hdc);
		SelectPalette(hdc, hOldPal, FALSE);
		ReleaseDC(hwnd, hdc);
		if (nChanged) {
			InvalidateRect(hwnd, NULL, TRUE);
			return 1;
		}
		return 0;
		break;	/* unreachable, but the original emits its jump */
	}

	case WM_PALETTECHANGED:
		if ((HWND)wParam != hwnd) {
			HDC hdc;
			HPALETTE hOldPal;
			int nChanged;

			hdc = GetDC(hwnd);
			hOldPal = SelectPalette(hdc, g_hPalette, FALSE);
			nChanged = RealizePalette(hdc);
			if (nChanged)
				InvalidateRect(hwnd, NULL, TRUE);
			SelectPalette(hdc, hOldPal, FALSE);
			ReleaseDC(hwnd, hdc);
		}
		break;

	case WM_ACTIVATEAPP:
		OnActivateApp(wParam);
		break;

	case WM_COMMAND: {
		int n;

		switch (wParam) {
		case IDC_START:
			/* Empty then-branch: `if (!IsIconic(...))` compiles to a different jump. */
			if (IsIconic(g_hwndMain)) {
			} else {
				n = RunStartDialog();
				switch (n) {
				case 2:
				case 101:
					DestroyWindow(g_hwndMain);
					break;
				default:
					PostMainCommand(IDC_PLAY);
					break;
				}
			}
			break;

		case IDC_PLAY:
			n = RunPlay();
			if (n == 101)
				DestroyWindow(g_hwndMain);
			if (n != 1)
				break;
			/* fall through */
		case IDC_NEXT:
			n = RunNextDialog(hwnd);
			if (n == 101)
				DestroyWindow(g_hwndMain);
			else if (wParam == IDC_PLAY) {
				if (++g_nRound > 6)
					g_nRound = 1;
				LoadRoundScores();
				PostMainCommand(IDC_START);
			} else
				PostMainCommand(IDC_PLAY);
			break;

		case 101:
		case 102:
		case 103:
			if (g_hwndChild)
				SendMessage(g_hwndChild, msg, wParam, lParam);
			break;

		case 110:
		case 111:
		case 112:
		case 113:
		case 114:
		case 115:
		case 116:
		case 117:
		case 118:
		case 119:
			SelectVolume(wParam);
			break;

		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	}

	case WM_PAINT:
		PaintMain(hwnd);
		break;

	case WM_DESTROY:
		CloseSession();
		PostQuitMessage(0);
		break;

	case WM_SYSCOMMAND: {
		BOOL fHandled = TRUE;

		switch (wParam) {
		case SC_CLOSE:
			if (g_hwndChild)
				PostMainCommand(101);
			else
				fHandled = FALSE;
			break;

		case SC_SIZE:
			break;

		case SC_MOVE:
			if (IsIconic(hwnd))
				fHandled = FALSE;
			break;

		case SC_MAXIMIZE:
		case SC_RESTORE:
			if (!g_hwndChild)
				PostMainCommand(IDC_START);
			/* fall through */
		case SC_MINIMIZE:
			OnActivateApp(wParam == SC_MAXIMIZE || wParam == SC_RESTORE);
			fHandled = FALSE;
			break;

		default:
			fHandled = FALSE;
			break;
		}
		if (!fHandled)
			return DefWindowProc(hwnd, msg, wParam, lParam);
		break;
	}

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		return 1;

	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONUP:
	case WM_NCLBUTTONDBLCLK:
		if (wParam == HTCAPTION && !IsIconic(hwnd))
			break;
		/* fall through */
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

// FUNCTION: AARCADE 0002:04AF
void FAR PASCAL OnActivateApp(BOOL fActive)
{
	if (fActive) {
		if (g_nInactive > 0 && --g_nInactive == 0) {
			g_lActivateTime = time(NULL);
			if (g_hwndCapture)
				SetCapture(g_hwndCapture);
			RestoreAnimTimer(&g_ptCursor);
			g_hwndCapture = 0;
			memset(&g_ptCursor, 0, sizeof(g_ptCursor));
		}
	} else {
		if (g_nInactive == 0) {
			CacheFlush();
			SaveAnimTimer(&g_ptCursor);
			g_hwndCapture = GetCapture();
		}
		g_nInactive++;
	}
}

// FUNCTION: AARCADE 0002:052B
void FAR PASCAL DeleteGdiObjects(void)
{
	DeleteObject(g_hGdiObjects[4]);
	DeleteObject(g_hGdiObjects[3]);
	DeleteObject(g_hGdiObjects[2]);
	DeleteObject(g_hGdiObjects[1]);
	DeleteObject(g_hGdiObjects[0]);
	if (g_hPalette) {
		DeleteObject(g_hPalette);
		g_hPalette = 0;
	}
	if (g_hGameFont)
		DeleteObject(g_hGameFont);
}

// FUNCTION: AARCADE 0002:0585
HPALETTE FAR PASCAL GetGamePalette(void)
{
	if (g_hPalette)
		return g_hPalette;
	else
		return GetStockObject(DEFAULT_PALETTE);
}

// FUNCTION: AARCADE 0002:05A3
void FAR PASCAL PostMainCommand(WORD id)
{
	PostMessage(g_hwndMain, WM_COMMAND, id, 0L);
}

// FUNCTION: AARCADE 0002:05BF
int FAR PASCAL ShowMessage(HWND hwnd, int idString, UINT fuStyle)
{
	char szText[128];

	if (!hwnd)
		hwnd = g_hwndMain;
	return MessageBox(hwnd, LoadStr(idString, szText, sizeof(szText)), g_szAppTitle, fuStyle);
}
