#include <string.h>
#include "aarcade.h"

/* Custom dialog plumbing: USERITEM procs, properties, timers, and accelerators. */

/* Property atoms and the timer hook. HHOOK is a DWORD, so g_hDlgAccel is at 007A. */
// GLOBAL: AARCADE 0024:006C
char g_szPropLo[] = "l";

// GLOBAL: AARCADE 0024:006E
char g_szPropHi[] = "h";

// GLOBAL: AARCADE 0024:0070
char g_szPropData[] = "d";

// GLOBAL: AARCADE 0024:0072
HWND g_hwndTimer = 0;

// GLOBAL: AARCADE 0024:0074
int g_nTimerRate = 0;

// GLOBAL: AARCADE 0024:0076
HHOOK g_hMsgHook = 0;

// GLOBAL: AARCADE 0024:007A
HACCEL g_hDlgAccel = 0;

// FUNCTION: AARCADE 0005:0000
int FAR PASCAL RunModalDialog(int idTemplate, DLGHOOKPROC lpfnHook, HWND hwndParent, int nData)
{
	int nResult;
	POINT ptTimer;
	HWND hwndCapture;

	SaveAnimTimer(&ptTimer);
	hwndCapture = GetCapture();
	if (hwndCapture)
		ReleaseCapture();
	if (g_lpfnModalDlgProc == NULL)
		g_lpfnModalDlgProc = MakeProcInstance(ModalDlgProc, g_hInstance);
	if (!hwndParent)
		hwndParent = g_hwndMain;
	g_nDlgData = nData;
	nResult = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(idTemplate), hwndParent,
	                         g_lpfnModalDlgProc, (LPARAM)lpfnHook);
	if (nResult == -1) {
		/* empty in the original too: it still emits the compare */
	}
	if (hwndCapture)
		SetCapture(hwndCapture);
	RestoreAnimTimer(&ptTimer);
	return nResult;
}

// FUNCTION: AARCADE 0005:0099
void FAR PASCAL EndModalDialog(HWND hDlg, int nResult)
{
	EndDialog(hDlg, nResult);
}

// FUNCTION: AARCADE 0005:00AD
BOOL CALLBACK _export ModalDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	DLGHOOKPROC lpfnHook;
	int nData;

	if (msg == WM_INITDIALOG) {
		HPALETTE hPal;
		HDC hdc;

		hPal = GetGamePalette();
		if (hPal) {
			hdc = GetDC(hDlg);
			SelectPalette(hdc, hPal, FALSE);
			RealizePalette(hdc);
			ReleaseDC(hDlg, hdc);
		}
		SetProp(hDlg, g_szPropLo, LOWORD(lParam));
		SetProp(hDlg, g_szPropHi, HIWORD(lParam));
		SetProp(hDlg, g_szPropData, g_nDlgData);
		lpfnHook = (DLGHOOKPROC)lParam;
		nData = g_nDlgData;
	} else {
		lpfnHook = (DLGHOOKPROC)MAKELONG(GetProp(hDlg, g_szPropLo), GetProp(hDlg, g_szPropHi));
		nData = GetProp(hDlg, g_szPropData);
		if (msg == WM_DESTROY) {
			RemoveProp(hDlg, g_szPropLo);
			RemoveProp(hDlg, g_szPropHi);
			RemoveProp(hDlg, g_szPropData);
		}
	}
	if (lpfnHook) {
		BOOL fResult;

		fResult = lpfnHook(hDlg, msg, wParam, lParam, nData);
		if (!fResult && msg == WM_ERASEBKGND) {
			HDC hdc;
			HBRUSH hbr;
			RECT rc;
			char szClass[16];

			GetClassName(hDlg, szClass, 15);
			if (stricmp(szClass, g_szSwampGasClass) == 0) {
				hdc = (HDC)wParam;
				GetClientRect(hDlg, &rc);
				hbr = CreateSolidBrush(RGB(255, 255, 255));
				FillRect(hdc, &rc, hbr);
				DeleteObject(hbr);
				fResult = TRUE;
			}
		}
		return fResult;
	} else	/* the else places `return FALSE` after the body, as in the original */
		return FALSE;
}

// FUNCTION: AARCADE 0005:022B
void FAR PASCAL MoveWindowTo(HWND hwnd, int x, int y, BOOL fRepaint)
{
	RECT rc;

	GetWindowRect(hwnd, &rc);
	if (x >= 0) {
		rc.right += x - rc.left;
		rc.left = x;
	}
	if (y >= 0) {
		rc.bottom += y - rc.top;
		rc.top = y;
	}
	MoveWindow(hwnd, rc.left, rc.top, rc.right - rc.left + 1, rc.bottom - rc.top + 1, fRepaint);
}

// FUNCTION: AARCADE 0005:028D
void FAR PASCAL FlashDlgItem(HWND hDlg, int id)
{
	DWORD dwStart;
	RECT rc;
	HWND hwnd;
	HDC hdc;

	hwnd = GetDlgItem(hDlg, id);
	GetClientRect(hwnd, &rc);
	hdc = GetDC(hwnd);
	InvertRect(hdc, &rc);
	Delay(8L, &dwStart);
	InvertRect(hdc, &rc);
	ReleaseDC(hwnd, hdc);
}

// FUNCTION: AARCADE 0005:02E8
void FAR PASCAL StartAnimTimer(HWND hwnd, int nRate, BOOL fCheckSibling)
{
	UINT uElapse;

	/* One combined condition; an early return for the sibling test adds a jump. */
	if (g_fAnimate && g_hwndTimer != hwnd && (!fCheckSibling || !IsTimerSibling(hwnd))) {
		StopAnimTimer();
		if (nRate == 0)
			uElapse = 10;
		else
			uElapse = nRate * 17L;
		if (hwnd)
			SetTimer(hwnd, 1000, uElapse, (TIMERPROC)TimerWndProc);
		g_hwndTimer = hwnd;
		g_nTimerRate = nRate;
	}
}

// FUNCTION: AARCADE 0005:035D
void FAR PASCAL StopAnimTimer(void)
{
	if (g_hwndTimer) {
		KillTimer(g_hwndTimer, 1000);
		g_hwndTimer = 0;
	}
}

// FUNCTION: AARCADE 0005:037D
BOOL FAR PASCAL IsTimerSibling(HWND hwnd)
{
	BOOL fSibling = FALSE;

	if (g_hwndTimer && hwnd
	    && (GetWindowLong(hwnd, GWL_STYLE) & (WS_POPUP | WS_CHILD)) == WS_CHILD
	    && (GetWindowLong(g_hwndTimer, GWL_STYLE) & (WS_POPUP | WS_CHILD)) == WS_CHILD
	    && GetParent(hwnd) == GetParent(g_hwndTimer))
		fSibling = TRUE;
	return fSibling;
}

// FUNCTION: AARCADE 0005:03EB
void FAR PASCAL SaveAnimTimer(LPPOINT lppt)
{
	lppt->x = g_hwndTimer;
	lppt->y = g_nTimerRate;
	StopAnimTimer();
}

// FUNCTION: AARCADE 0005:040B
void FAR PASCAL RestoreAnimTimer(LPPOINT lppt)
{
	StartAnimTimer(lppt->x, lppt->y, FALSE);
}

// FUNCTION: AARCADE 0005:0427
LRESULT CALLBACK _export TimerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult;

	lResult = UserWndProc(hwnd, msg, wParam, lParam);
	return lResult;
}

// FUNCTION: AARCADE 0005:045E
void FAR PASCAL InstallMsgHook(HINSTANCE hInstance, HWND hwnd)
{
	g_hDlgAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(2));
	g_hMsgHook = SetWindowsHookEx(WH_MSGFILTER, (HOOKPROC)MsgHookProc, hInstance, GetCurrentTask());
}

// FUNCTION: AARCADE 0005:0494
void FAR PASCAL RemoveMsgHook(void)
{
	UnhookWindowsHookEx(g_hMsgHook);
}

// FUNCTION: AARCADE 0005:04A8
LRESULT CALLBACK _export MsgHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	LPMSG lpmsg;
	HWND hwnd;
	HACCEL hAccel;

	if (code < 0)
		CallNextHookEx(g_hMsgHook, code, wParam, lParam);
	else if (code == MSGF_DIALOGBOX) {
		lpmsg = (LPMSG)lParam;
		if ((lpmsg->message == WM_KEYUP || lpmsg->message == WM_KEYDOWN
		     || lpmsg->message == WM_SYSKEYUP || lpmsg->message == WM_SYSKEYDOWN)
		    && code == MSGF_DIALOGBOX) {	/* redundant, but the original tests it again */
			hwnd = (GetWindowLong(lpmsg->hwnd, GWL_STYLE) & (WS_POPUP | WS_CHILD)) == WS_CHILD
				? GetParent(lpmsg->hwnd) : lpmsg->hwnd;
			hAccel = (HACCEL)SendMessage(hwnd, WM_USER + 4, 0, 0L);
			if (!hAccel)
				hAccel = g_hDlgAccel;
			if (hAccel)
				return TranslateAccelerator(hwnd, hAccel, lpmsg);
		}
	}
	return 0L;
}
