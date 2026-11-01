#include "aarcade.h"

/* Game-select dialog and starting / aborting a round of play. */

/* id, x, y, width, height. The 0x800 bit marks a text control.
     106 words, 0024:04EC..05BF, then inifile.c. */
// GLOBAL: AARCADE 0024:04EC
int g_anPlayCtl[] = {
	3, 44, 98, 125, 58,
	4, 191, 98, 125, 58,
	5, 338, 98, 125, 58,
	6, 44, 195, 125, 58,
	7, 191, 195, 125, 58,
	8, 338, 195, 125, 58,
	0x809, 44, 162, 125, 16,
	0x80A, 191, 162, 125, 16,
	0x80B, 338, 162, 125, 16,
	0x80C, 44, 259, 125, 16,
	0x80D, 191, 259, 125, 16,
	0x80E, 338, 259, 125, 16,
	15, 71, 108, 72, 31,
	16, 217, 116, 74, 19,
	17, 387, 99, 27, 50,
	18, 69, 202, 77, 38,
	19, 233, 198, 39, 46,
	20, 381, 198, 37, 48,
	0x815, 92, 61, 328, 16,
	22, 0, 0, 514, 35,
	23, 0, 310, 514, 35,
	0
};

/* Near pointer: time.h's prototype is far and would push two zeros. */
long cdecl time(long *);

BOOL FAR PASCAL PlayDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);
LRESULT CALLBACK PlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL NameDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:3820
int g_nPlay;


// GLOBAL: AARCADE 0024:381C
long g_lPlayDeadline;


// GLOBAL: AARCADE 0024:381A
HFONT g_hPlayFont;


// GLOBAL: AARCADE 0024:3818
HDC g_hdcMemA;


// GLOBAL: AARCADE 0024:3816
HDC g_hdcMemB;

// FUNCTION: AARCADE 000D:0000
LRESULT CALLBACK PlayBorderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_PAINT: {
		HGDIOBJ hOld;
		HPEN hpen;
		RECT rc;
		PAINTSTRUCT ps;
		HDC hdc;

		hdc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rc);
		hpen = CreatePen(0, 2, 0L);
		hOld = SelectObject(hdc, hpen);
		SelectObject(hdc, GetStockObject(WHITE_BRUSH));
		RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 16, 16);
		SelectObject(hdc, hOld);
		DeleteObject(hpen);
		EndPaint(hwnd, &ps);
	}
		/* break after the block: two jumps out of the case */
		break;
	case WM_TIMER:
		TickViews();
		if (time(0) >= g_lPlayDeadline)
			SendMessage(GetParent(hwnd), WM_COMMAND, -1, 0L);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 000D:00E8
int FAR PASCAL RunPlayDialog(int *pn)
{
	/* pn is pushed by RunPlay and never read */
	return RunModalDialog(0x136, PlayDlgProc, 0, 0);
}

// FUNCTION: AARCADE 000D:0105
BOOL FAR PASCAL PlayDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG: {
		int id;
		int x;
		int y;
		int cx;
		int cy;
		int fText;
		int i;

		g_nPlay = 0;
		MoveWindow(hDlg, 0, 0, 0x202, 0x158, FALSE);
		CenterPopup(hDlg);
		g_hPlayFont = CreateGameFont(0, 2, 9, 1);
		i = 0;
		while ((id = g_anPlayCtl[i++]) != 0) {
			fText = id & 0x800;
			id &= ~0x800;
			x = g_anPlayCtl[i++];
			y = g_anPlayCtl[i++];
			cx = g_anPlayCtl[i++];
			cy = g_anPlayCtl[i++];
			MoveWindow(GetDlgItem(hDlg, id), x, y, cx, cy, FALSE);
			if (fText) {
				SetWindowWord(GetDlgItem(hDlg, id), g_nUserWndExtra + 16, 1);
				SendDlgItemMessage(hDlg, id, WM_SETFONT, g_hPlayFont, 0L);
			}
		}
		SubclassUserItem(hDlg, 4, PlayBorderProc);
		SubclassUserItem(hDlg, 5, PlayBorderProc);
		SubclassUserItem(hDlg, 6, PlayBorderProc);
		SubclassUserItem(hDlg, 7, PlayBorderProc);
		SubclassUserItem(hDlg, 8, PlayBorderProc);
		SubclassUserItem(hDlg, 3, PlayBorderProc);
		OpenViews(hDlg);
		ShowWindow(hDlg, SW_SHOWNORMAL);
		g_lPlayDeadline = time(0) + 30;
		StartAnimTimer(GetDlgItem(hDlg, 3), 3, TRUE);
		break;
	}
	case WM_COMMAND:
		switch (wParam) {
		case 3: /* Stink Snakes, round 1 */
		case 4: /* Nerdoids, round 2 */
		case 5: /* Quasar Klutzes, round 3 */
		case 6: /* Fungus Patrol, round 4 */
		case 7: /* Andromedroids, round 5 */
		case 8: /* Quasar Rain, round 6 */
			g_nPlay = (wParam - 3) + 1;
			wParam = 1;
			/* goto, not a second call: the store stays and jumps forward */
			goto play_end;
		case 1:
		case 2:
		case 22:
		case 23:
		case -1:
			wParam = 1;
		case IDM_CHILD_FIRST:
		play_end:
			EndModalDialog(hDlg, wParam);
			break;
		default:
			break;
		}
		break;
	case WM_ERASEBKGND:
		if (g_fColor) {
			HDC hdc;
			RECT rc;

			hdc = (HDC)wParam;
			GetClientRect(hDlg, &rc);
			FillRect(hdc, &rc, g_hGdiObjects[1]);
		} else
			return 0;
		break;
	case WM_USER + 3:
		_CX = (unsigned)LOWORD(lParam);
		if (g_fColor)
			*(COLORREF *)_CX = PALETTERGB(g_rgbGame[8].rgbRed, g_rgbGame[8].rgbGreen, g_rgbGame[8].rgbBlue);
		else
			*(COLORREF *)_CX = RGB(255, 255, 255);
		break;
	case WM_DESTROY:
		CloseViews(hDlg);
		DeleteObject(g_hPlayFont);
		return 0;
	default:
		return 0;
	}
	return 1;
}

// FUNCTION: AARCADE 000D:03AE
int FAR PASCAL RunPlay(void)
{
	RECT rc;
	int n;

	SaveRoundScores();
	n = RunPlayDialog(&g_nPlay);
	if (g_nPlay == 0)
		return n;
	SetRound(g_nPlay);
	SetRect(&rc, 0, 0, 0x200, 0x156);
	g_hwndChild = CreateAnimWindow(g_hwndMain, &rc, PlayWndProc, TRUE, 1);
	SetCapture(g_hwndChild);
	g_rcPlay = g_rcPlaySrc;
	g_rcPlay.bottom -= 16;
	g_lPlayScore = 0;
	OpenRound(g_hwndChild);
	return 10;
}

// FUNCTION: AARCADE 000D:0443
LRESULT CALLBACK PlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int fRet;

	fRet = 1;
	switch (msg) {
	case WM_ERASEBKGND: {
		HDC hdc;
		RECT rc;

		hdc = (HDC)wParam;
		GetClientRect(hwnd, &rc);
		FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));
	}
		/* break after the block: two jumps out of the case */
		break;
	case WM_TIMER: {
		TickSound();
		fRet = !TickRound();
	}
		/* break after the block: two jumps out of the case */
		break;
	case WM_SETCURSOR:
		break;
	case WM_LBUTTONDOWN:
		ClickRound();
		break;
	case WM_PAINT:
		PaintRound();
		break;
	case WM_COMMAND:
		switch (wParam) {
		case IDM_CHILD_FIRST + 1:
			PauseTimeLeft();
			fRet = RunResultDialog();
			ResumeTimeLeft();
			/* je/je/jmp: the failure skips the fallthrough as well */
			if (fRet != IDM_CHILD_FIRST + 2 && fRet != IDM_CHILD_FIRST)
				break;
			wParam = fRet;
			/* fall through */
		case IDM_CHILD_FIRST:
		case IDM_CHILD_FIRST + 2:
			fRet = wParam;
			PauseTimeLeft();
			ConfirmChildCmd(&fRet);
			if (fRet != 1)
				break;
			ResumeTimeLeft();
			break;
		default:
			return 0;
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	if (fRet != 1) {
		CloseRound();
		DestroyAnimWindow(g_hwndChild);
		g_hwndChild = 0;
		if (fRet == IDM_CHILD_FIRST)
			DestroyWindow(g_hwndMain);
		else if (fRet != IDM_CHILD_FIRST + 2) {
			LoadRoundScores();
			g_lSavedCount = g_lPlayScore;
			RecordScore();
			SaveRoundScores();
		}
		fRet = IDC_NEXT;
		if (fRet != IDM_CHILD_FIRST)
			PostMainCommand(fRet);
	}
	return 1;
}

// FUNCTION: AARCADE 000D:05BC
LRESULT CALLBACK NamePaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_PAINT: {
		HDC hdc;
		RECT rc;
		PAINTSTRUCT ps;

		hdc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rc);
		DrawItemPic(hdc, 0xFA, &rc);
		EndPaint(hwnd, &ps);
	}
		/* break after the block: two jumps out of the case */
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 000D:062C
void FAR PASCAL RunNameDialog(void)
{
	RunModalDialog(0x1AE, NameDlgProc, 0, 0);
}

// FUNCTION: AARCADE 000D:0645
BOOL FAR PASCAL NameDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	BYTE sz[256];

	switch (msg) {
	case WM_INITDIALOG:
		SubclassUserItem(hDlg, 3, NamePaintProc);
		SetDlgItemPstr(GetDlgItemHwnd(hDlg, 2), g_szPlayName);
		SendDlgItemMessage(hDlg, 2, EM_LIMITTEXT, 20, 0L);
		SendDlgItemMessage(hDlg, 2, EM_SETSEL, 0, MAKELONG(0, -1));
		SetFocus(GetDlgItemHwnd(hDlg, 2));
		CenterPopup(hDlg);
		PlaySfx(0x2331, 1);
		return 0;
	case WM_COMMAND:
		switch (wParam) {
		case 1:
			GetDlgItemPstr(GetDlgItemHwnd(hDlg, 2), (LPSTR)sz);
			if (sz[0] > 20)
				sz[0] = 20;
			CopyPStr((LPSTR)sz, g_szPlayName);
			EndModalDialog(hDlg, wParam == 1);
			break;
		default:
			return 0;
		}
		break;
	default:
		return 0;
	}
	return 1;
}
