#include "aarcade.h"

/* End-of-round result dialog and name entry when a score qualifies. */

/* Near pointer: time.h's prototype is far and would push two zeros. */
long cdecl time(long *);

BOOL FAR PASCAL ResultDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);

// FUNCTION: AARCADE 0015:0000
LRESULT CALLBACK ResultItemProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_GETDLGCODE:
		return DLGC_WANTCHARS;
	case WM_ERASEBKGND: {
		HDC hdc;
		RECT rc;

		hdc = (HDC)wParam;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
	}
		/* break after the block: two jumps out of the case */
		break;
	case WM_CHAR:
		SendMessage(GetParent(hwnd), WM_COMMAND, GetDlgCtrlID(hwnd), MAKELONG(hwnd, 0));
		break;
	case WM_PAINT: {
		HDC hdc;
		PAINTSTRUCT ps;
		char sz[256];

		hdc = BeginPaint(hwnd, &ps);
		MoveTo(hdc, 0xD0, 0xAA);
		GetIndString(sz, 0x80, 0x16);
		SetTextColor(hdc, RGB(255, 255, 255));
		SetBkColor(hdc, 0L);
		TextOutPascal(hdc, (BYTE FAR *)sz);
		EndPaint(hwnd, &ps);
	}
		/* break after the block: two jumps out of the case */
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0015:0109
int FAR PASCAL RunResultDialog(void)
{
	int nResult;

	nResult = RunModalDialog(0xC8, ResultDlgProc, 0, 0);
	g_lActivateTime = time(0);
	return nResult;
}

// FUNCTION: AARCADE 0015:013C
BOOL FAR PASCAL ResultDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG: {
		HWND hwndItem;

		hwndItem = GetDlgItem(hDlg, 1);
		MoveWindow(hDlg, 0, 0, 0x200, 0x156, FALSE);
		MoveWindow(hwndItem, 0, 0, 0x200, 0x156, FALSE);
		SubclassUserItem(hDlg, 1, ResultItemProc);
		SetFocus(GetDlgItem(hDlg, 1));
		CenterPopup(hDlg);
	}
		/* break after the block: two jumps out of the case */
		break;
	case WM_ERASEBKGND: {
		HDC hdc;
		RECT rc;

		hdc = (HDC)wParam;
		GetClientRect(hDlg, &rc);
		FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
	}
		/* break after the block: two jumps out of the case */
		break;
	case WM_COMMAND:
		EndModalDialog(hDlg, wParam);
		break;
	case WM_USER + 4: {
		HACCEL hAccel;

		hAccel = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(3));
		/* near-to-far puts DS in the high word: push ds; push hAccel */
		SetWindowLong(hDlg, 0, (LONG)(BYTE FAR *)(BYTE NEAR *)hAccel);
	}
		/* break after the block: two jumps out of the case */
		break;
	default:
		return 0;
	}
	return 1;
}
