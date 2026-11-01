#include "aarcade.h"

/* Shared dialog helpers, palettes, and drawing pictures into dialog items. */

/* Palettes and the shared play-window state. clear.c's score handles follow. */
// GLOBAL: AARCADE 0024:0670
RGBQUAD g_rgbVga[16] = {
	{255, 255, 255, 0}, {255, 0, 0, 0}, {128, 0, 128, 0}, {0, 255, 255, 0},
	{0, 128, 128, 0}, {0, 255, 0, 0}, {0, 128, 0, 0}, {0, 255, 255, 0},
	{0, 128, 128, 0}, {0, 0, 255, 0}, {128, 128, 128, 0}, {255, 0, 255, 0},
	{255, 255, 0, 0}, {0, 128, 128, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}
};

// GLOBAL: AARCADE 0024:06B0
RGBQUAD g_rgbCustom[16] = {
	{255, 255, 255, 0}, {204, 153, 0, 0}, {204, 102, 153, 0}, {102, 255, 255, 0},
	{0, 102, 153, 0}, {51, 204, 102, 0}, {0, 102, 51, 0}, {0, 153, 255, 0},
	{153, 204, 255, 0}, {0, 51, 255, 0}, {136, 136, 136, 0}, {255, 51, 255, 0},
	{255, 255, 153, 0}, {102, 153, 204, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}
};

// GLOBAL: AARCADE 0024:06F0
HWND g_hwndChild = 0;

// GLOBAL: AARCADE 0024:06F2
BOOL g_fAnimate = TRUE;

// GLOBAL: AARCADE 0024:06F4
int g_nRound = 0;

/* Near pointer: time.h's prototype is far and would push two zeros. */
long cdecl time(long *);

BOOL FAR PASCAL ConfirmDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);

/* g_fAltLayout, g_rcLayout, g_fLayoutFits, g_rgbGame and the two class-extra
   words are defined in inifile.c. This module would otherwise start on the
   byte after an odd-sized BSS and the linker would shift them by one. */

// FUNCTION: AARCADE 0010:0000
HWND FAR PASCAL GetDlgItemHwnd(HWND hwnd, int id)
{
	return GetDlgItem(hwnd, id);
}

// FUNCTION: AARCADE 0010:0016
void FAR PASCAL GetDlgItemPstr(HWND hwnd, LPSTR lpps)
{
	GetWindowText(hwnd, lpps, 255);
	CtoPstr(lpps);
}

// FUNCTION: AARCADE 0010:003B
void FAR PASCAL SetDlgItemPstr(HWND hwnd, LPSTR lpps)
{
	PtoCstr(lpps);
	SetWindowText(hwnd, lpps);
	CtoPstr(lpps);
}

// FUNCTION: AARCADE 0010:0068
BOOL FAR PASCAL HideDlgItem(HWND hwnd, int id)
{
	HWND hwndItem;

	hwndItem = GetDlgItem(hwnd, id);
	return ShowWindow(hwndItem, SW_HIDE);
}

// FUNCTION: AARCADE 0010:008E
BOOL FAR PASCAL ShowDlgItem(HWND hwnd, int id)
{
	HWND hwndItem;

	hwndItem = GetDlgItem(hwnd, id);
	return ShowWindow(hwndItem, SW_SHOW);
}

// FUNCTION: AARCADE 0010:00B4
BOOL FAR PASCAL EnableDlgItem(HWND hwnd, int id, BOOL fEnable)
{
	HWND hwndItem;

	hwndItem = GetDlgItem(hwnd, id);
	return EnableWindow(hwndItem, fEnable);
}

// FUNCTION: AARCADE 0010:00DB
void FAR PASCAL EnableByCode(HWND hwnd, int n)
{
	if (n == 255)
		EnableWindow(hwnd, FALSE);
	else if (!n)
		EnableWindow(hwnd, TRUE);
}

// FUNCTION: AARCADE 0010:010A
void FAR PASCAL CenterWindow(HWND hwnd, int fChild, int dx, int dy)
{
	RECT rcWnd;
	RECT rcRef;
	POINT pt;
	HWND hwndRef;

	GetWindowRect(hwnd, &rcWnd);
	OffsetRect(&rcWnd, -rcWnd.left, -rcWnd.top);
	if (IsWindowVisible(g_hwndMain) && !IsIconic(g_hwndMain))
		hwndRef = g_hwndMain;
	else
		hwndRef = GetDesktopWindow();
	GetClientRect(hwndRef, &rcRef);
	OffsetRect(&rcRef, -rcRef.left, -rcRef.top);
	pt.x = pt.y = 0;
	if (!fChild)
		ClientToScreen(hwndRef, &pt);
	if (g_fLayoutFits && g_fAltLayout) {
		pt.x += 0x2f;
		pt.y += 0x24;
	}
	MoveWindow(hwnd,
		(rcRef.right - rcWnd.right + -4) / 2 + pt.x + dx,
		(rcRef.bottom - rcWnd.bottom + -2) / 2 + pt.y + dy,
		rcWnd.right, rcWnd.bottom, FALSE);
}

// FUNCTION: AARCADE 0010:01ED
void FAR PASCAL CenterWindowUnused(HWND hwnd, int nUnused)
{
	CenterWindow(hwnd, 0, 0, 0);
}

// FUNCTION: AARCADE 0010:0203
void FAR PASCAL CenterPopup(HWND hwnd)
{
	CenterWindow(hwnd, 0, 0, 0);
}

// FUNCTION: AARCADE 0010:0219
void FAR PASCAL CenterChild(HWND hwnd)
{
	CenterWindow(hwnd, 1, 0, 0);
}

// FUNCTION: AARCADE 0010:022F
LRESULT CALLBACK FrameItemProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_PAINT: {
		HDC hdc;
		RECT rc;
		PAINTSTRUCT ps;

		hdc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rc);
		FrameRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
		EndPaint(hwnd, &ps);
	}
		/* break after the block: two jumps out of the case */
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0010:02A4
void FAR PASCAL SubclassFrameItem(HWND hwnd, int id)
{
	SubclassUserItem(hwnd, id, FrameItemProc);
}

// FUNCTION: AARCADE 0010:02BE
void FAR PASCAL DrawItemPic(HDC hdc, int id, LPRECT lprc)
{
	HBITMAP hbm;

	if (g_fColor) {
		hbm = LoadPPic(id, 0, FALSE);
		if (hbm) {
			DrawPicture(hdc, hbm, lprc);
			FreePPic(hbm, FALSE);
		}
	} else {
		hbm = LoadPicture(id, FALSE);
		if (hbm) {
			DrawPicture(hdc, hbm, lprc);
			FreePicture(hbm, FALSE);
		}
	}
}

// FUNCTION: AARCADE 0010:0323
void FAR PASCAL ConfirmChildCmd(int *pn)
{
	int nResult;
	char sz[256];

	GetIndString(sz, 0x80, *pn == IDM_CHILD_FIRST + 2 ? 10 : 11);
	ParamText(sz, NULL, NULL, NULL);
	nResult = RunModalDialog(0x12C, ConfirmDlgProc, 0, 0);
	if (nResult == 2)
		*pn = 1;
	g_lActivateTime = time(0);
}

// FUNCTION: AARCADE 0010:0396
BOOL FAR PASCAL ConfirmDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG:
		CenterPopup(hDlg);
		break;
	case WM_COMMAND:
		EndModalDialog(hDlg, wParam);
		break;
	default:
		return 0;
	}
	return 1;
}
