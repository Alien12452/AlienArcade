#include <string.h>
#include "aarcade.h"

/* Confirm dialogs to reset today's lists or erase the whole high-score file. */

/* Zero-initialized so they stay in _DATA, ahead of next.c's scripts.
   Daily list is round + 1000; all-time is round + 2000. */
// GLOBAL: AARCADE 0024:06F6
int g_fDailySaved = 0;

// GLOBAL: AARCADE 0024:06F8
HGLOBAL g_hDaily = 0;

// GLOBAL: AARCADE 0024:06FA
int g_fAllTimeSaved = 0;

// GLOBAL: AARCADE 0024:06FC
HGLOBAL g_hAllTime = 0;

BOOL FAR PASCAL ResetScoresDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);
BOOL FAR PASCAL EraseScoresDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);
void FAR PASCAL EraseScoreFile(void);

/* Confirm clearing this round's daily and all-time lists. */
// FUNCTION: AARCADE 0011:0000
int FAR PASCAL RunResetScores(HWND hwnd)
{
	/* no return: `return` inserts a jmp over the epilogue */
	RunModalDialog(0x118, ResetScoresDlgProc, hwnd, 0);
}

// FUNCTION: AARCADE 0011:001C
BOOL FAR PASCAL ResetScoresDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG:
		CenterPopup(hDlg);
		break;
	case WM_COMMAND:
		switch (wParam) {
		case 3:
			if (g_hDaily != 0) {
				ResetScoreList(g_hDaily);
				g_fDailySaved = 1;
			}
			if (g_hAllTime != 0) {
				ResetScoreList(g_hAllTime);
				g_fAllTimeSaved = 1;
			}
			SaveRoundScores();
			LoadRoundScores();
			/* fall through: 1 and 2 close without clearing */
		case 1:
		case 2:
			EndModalDialog(hDlg, wParam == 3);
			break;
		}
		break;
	default:
		return 0;
	}
	return 1;
}

/* Confirm deleting AARCADE.HI, then reload. Shift-click of the same button. */
// FUNCTION: AARCADE 0011:00AF
int FAR PASCAL RunEraseScores(HWND hwnd)
{
	RunModalDialog(0x122, EraseScoresDlgProc, hwnd, 0);
}

// FUNCTION: AARCADE 0011:00CB
BOOL FAR PASCAL EraseScoresDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	switch (msg) {
	case WM_INITDIALOG:
		CenterPopup(hDlg);
		break;
	case WM_COMMAND:
		switch (wParam) {
		case 3:
			EraseScoreFile();
			LoadRoundScores();
		case 1:
		case 2:
			EndModalDialog(hDlg, wParam == 3);
			break;
		}
		break;
	default:
		return 0;
	}
	return 1;
}

/* Windows directory plus g_szHiFile, then OF_DELETE. */
// FUNCTION: AARCADE 0011:0132
void FAR PASCAL EraseScoreFile(void)
{
	OFSTRUCT of;
	char sz[160];
	int n;

	n = GetWindowsDirectory(sz, 160);
	/* through BX: sz[n - 1] folds into [bp+si-0x129] */
	_BX = n;
	--_BX;
	if (sz[_BX] != '\\') {
		sz[n] = '\\';
		n++;
		sz[n] = 0;
	}
	strcat(sz, g_szHiFile);
	OpenFile(sz, &of, OF_DELETE);
}
