#include "aarcade.h"

/* Between waves: the gate dialog, the time-bonus count, and the round-done box. */

BOOL FAR PASCAL RoundHelpDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);
BOOL FAR PASCAL StepDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);
BOOL FAR PASCAL WaveDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);
BOOL FAR PASCAL RoundDoneDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);
void FAR PASCAL CountTimeBonus(HWND hwnd);

/* 10 min, 1 min, 10 sec, 1 sec. FormatTime uses the same divisors. */
// GLOBAL: AARCADE 0024:1CCA
int g_anTimeStep[4] = { 600, 60, 10, 1 };

/* n is the step index, then the score place: both live at [bp-4]. */
// FUNCTION: AARCADE 0023:0000
void FAR PASCAL CountTimeBonus(HWND hwnd)
{
	DWORD n;
	DWORD step;
	DWORD dw;
	DWORD score;
	DWORD t;
	HDC hdc;
	char sz[256];
	int f;

	f = 1;
	t = g_lBonusTime;
	if (t == 0)
		f = 0;
	if (f != 0)
		Delay(60, &dw);
	score = 0;
	n = 0;
	while ((DWORD)(long)g_anTimeStep[n] > t)
		n++;
	n++;
	if (n == 4)
		n = 3;
	while (n < 4) {
		while ((DWORD)(long)g_anTimeStep[n] <= t) {
			dw = GetTickCount() / 17 + 2;
			if (g_nRound <= 3)
				score += (long)(g_anTimeStep[n] * 5);
			else
				score += (long)(g_anTimeStep[n] * 10);
			t -= (long)g_anTimeStep[n];
			FormatTime(t, sz);
			SetDlgItemPstr(GetDlgItemHwnd(hwnd, 4), sz);
			PadNumString(score, 6, sz);
			SetDlgItemPstr(GetDlgItemHwnd(hwnd, 6), sz);
			hdc = GetDC(g_hwndChild);
			g_lTimeLeft = t;
			DrawTimeLeft(hdc);
			ReleaseDC(g_hwndChild, hdc);
			PlaySfx(0x232F, FALSE);
			while (GetTickCount() / 17 < dw)
				;
		}
		n++;
	}
	if (f != 0)
		Delay(15, &dw);
	GetIndString(sz, 0x80, 0x15);
	SetDlgItemPstr(GetDlgItemHwnd(hwnd, 3), sz);
	if (f != 0)
		Delay(15, &dw);
	else {
		PadNumString(g_lSavedCount, 6, sz);
		SetDlgItemPstr(GetDlgItemHwnd(hwnd, 4), sz);
	}
	n = 100000UL;
	while (n > score)
		n /= 10;
	n /= 10;
	if (n == 0)
		n = 1;
	while (n > 0) {
		step = n;
		if (score / n > 4)
			n *= 2;
		while (score >= n) {
			dw = GetTickCount() / 17 + 2;
			g_lSavedCount += n;
			score -= n;
			PadNumString(g_lSavedCount, 6, sz);
			SetDlgItemPstr(GetDlgItemHwnd(hwnd, 4), sz);
			PadNumString(score, 6, sz);
			SetDlgItemPstr(GetDlgItemHwnd(hwnd, 6), sz);
			AddPlayScore(n);
			PlaySfx(0x2330, FALSE);
			while (GetTickCount() / 17 < dw)
				;
		}
		if (n != step)
			n = (n >> 1) * 10;
		n /= 10;
	}
	Delay(60, &dw);
	g_lBonusTime = 0;
	HideDlgItem(hwnd, 5);
	HideDlgItem(hwnd, 6);
}

// FUNCTION: AARCADE 0023:042C
void FAR PASCAL RunRoundHelp(void)
{
	RunModalDialog(g_nRound * 10 + 0x168, RoundHelpDlgProc, 0, 0);
}

// FUNCTION: AARCADE 0023:044C
BOOL FAR PASCAL RoundHelpDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG: {
		char sz[64];

		if (g_nRound == 5) {
			if (g_fColor != 0)
				GetIndString(sz, 0x80, 0x18);
			else
				sz[0] = 0;
			ParamText(sz, NULL, NULL, NULL);
		}
		switch (g_nRound - 1) {
		case 0:
			OpenView1(hDlg, 8, 9, 10);
			break;
		case 1:
			OpenView2(hDlg, 8, 9, 10);
			break;
		case 2:
			OpenView3(hDlg, 8, 9, 10);
			break;
		case 3:
			OpenView4(hDlg, 9, 8, 10);
			break;
		case 4:
			OpenView5(hDlg, 8, 9, 10);
			break;
		case 5:
			OpenView6(hDlg, 8, 9);
			break;
		}
		CenterPopup(hDlg);
		break;
	}
	case WM_COMMAND:
		if (wParam == 1)
			EndModalDialog(hDlg, wParam);
		break;
	case WM_ERASEBKGND:
		if (g_fColor != 0) {
			HDC hdc;
			RECT rc;
			HGDIOBJ hbr;

			hdc = (HDC)wParam;
			GetClientRect(hDlg, &rc);
			switch (g_nRound - 1) {
			case 0:
				hbr = g_hGdiObjects[0];
				break;
			case 1:
				hbr = g_hGdiObjects[2];
				break;
			case 3:
				hbr = g_hGdiObjects[3];
				break;
			case 4:
				hbr = g_hGdiObjects[4];
				break;
			case 2:
			case 5:
			default:
				hbr = g_hGdiObjects[1];
				break;
			}
			FillRect(hdc, &rc, hbr);
		} else
			return 0;
		break;
	case WM_DESTROY:
		switch (g_nRound - 1) {
		case 0:
			CloseView1(hDlg, 8);
			break;
		case 1:
			CloseView2(hDlg, 8);
			break;
		case 2:
			CloseView3(hDlg, 8);
			break;
		case 3:
			CloseView4(hDlg, 9);
			break;
		case 4:
			CloseView5(hDlg, 8);
			break;
		case 5:
			CloseView6(hDlg, 8);
			break;
		}
		return 0;
	case WM_USER + 3:
		_CX = (unsigned)LOWORD(lParam);
		if (g_fColor != 0) {
			switch (g_nRound - 1) {
			case 0:
				*(COLORREF *)_CX = PALETTERGB(g_rgbGame[5].rgbRed, g_rgbGame[5].rgbGreen, g_rgbGame[5].rgbBlue);
				break;
			case 1:
				*(COLORREF *)_CX = PALETTERGB(g_rgbGame[12].rgbRed, g_rgbGame[12].rgbGreen, g_rgbGame[12].rgbBlue);
				break;
			case 3:
				*(COLORREF *)_CX = 0xC0C0C0L;
				break;
			case 4:
				*(COLORREF *)_CX = PALETTERGB(g_rgbGame[3].rgbRed, g_rgbGame[3].rgbGreen, g_rgbGame[3].rgbBlue);
				break;
			case 2:
			case 5:
			default:
				*(COLORREF *)_CX = PALETTERGB(g_rgbGame[8].rgbRed, g_rgbGame[8].rgbGreen, g_rgbGame[8].rgbBlue);
				break;
			}
		} else
			*(COLORREF *)_CX = 0xFFFFFFL;
		break;
	default:
		return 0;
	}
	return 1;
}

/* g_nPlayWave < 0 is the gate at OpenRound; later calls count the bonus. */
// FUNCTION: AARCADE 0023:06E2
void FAR PASCAL RunStepDialog(void)
{
	int n;

	if (g_nPlayWave < 0) {
		do {
			n = RunModalDialog(0x168, StepDlgProc, 0, 0);
			/* else goto, so jne lands on the jmp out of the loop */
			if (n == 3)
				RunRoundHelp();
			else
				goto finished;
		} while (1);
	finished:;
	} else {
		RunModalDialog(0x14A, WaveDlgProc, 0, 0);
		if (g_nRound == 1 || g_nRound == 2)
			AddPlayAmmo(20L);
	}
}

// FUNCTION: AARCADE 0023:0743
BOOL FAR PASCAL StepDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG:
		CenterPopup(hDlg);
		break;
	case WM_COMMAND:
		if (wParam != IDM_CHILD_FIRST)
			EndModalDialog(hDlg, wParam);
		break;
	default:
		return 0;
	}
	return 1;
}

// FUNCTION: AARCADE 0023:0784
BOOL FAR PASCAL WaveDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG: {
		char sz[256];

		NumToString((long)(g_nPlayWave + 1), sz);
		ParamText(sz, "", "", "");
		SetWindowWord(GetDlgItem(hDlg, 3), g_nUserWndExtra + 16, 2);
		SetWindowWord(GetDlgItem(hDlg, 5), g_nUserWndExtra + 16, 2);
		SetWindowWord(GetDlgItem(hDlg, 7), g_nUserWndExtra + 16, 1);
		if (g_lTimeLeft == 0) {
			HideDlgItem(hDlg, 3);
			HideDlgItem(hDlg, 4);
			HideDlgItem(hDlg, 5);
			HideDlgItem(hDlg, 6);
			if (g_nRound == 5)
				HideDlgItem(hDlg, 7);
			PlaySfx(0x232B, TRUE);
		} else
			HideDlgItem(hDlg, 7);
		CenterPopup(hDlg);
		if (g_lTimeLeft != 0) {
			ShowWindow(GetDlgItemHwnd(hDlg, 1), SW_HIDE);
			ShowWindow(hDlg, SW_SHOW);
			UpdateWindow(hDlg);
			PlaySfx(0x232B, TRUE);
			g_lBonusTime = g_lTimeLeft;
			g_lSavedCount = g_lPlayScore;
			CountTimeBonus(hDlg);
			ShowWindow(GetDlgItemHwnd(hDlg, 1), SW_SHOW);
		}
		break;
	}
	case WM_COMMAND:
		if (wParam != IDM_CHILD_FIRST)
			EndModalDialog(hDlg, wParam);
		break;
	default:
		return 0;
	}
	return 1;
}

// FUNCTION: AARCADE 0023:08E9
void FAR PASCAL RunRoundDone(void)
{
	RunModalDialog(0x154, RoundDoneDlgProc, 0, 0);
}

// FUNCTION: AARCADE 0023:0902
BOOL FAR PASCAL RoundDoneDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG: {
		char sz[256];

		SetWindowWord(GetDlgItem(hDlg, 2), g_nUserWndExtra + 16, 1);
		PadNumString(g_lPlayScore, 6, sz);
		ParamText(sz, NULL, NULL, NULL);
		CenterPopup(hDlg);
		PlaySfx(0x232C, TRUE);
	}
		break;
	case WM_COMMAND:
		if (wParam == 1)
			EndModalDialog(hDlg, wParam);
		break;
	default:
		return 0;
	}
	return 1;
}
