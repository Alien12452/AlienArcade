#include "aarcade.h"

/* Between-game next dialog: scrolling strips and today's / all-time lists. */

/* Move scripts for the two strips. Same bytes as the line-view scripts. */
// GLOBAL: AARCADE 0024:06FE
unsigned char g_abNextA[20] = {
	0x03, 0x08, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x02, 0x02, 0x04, 0x08, 0x80, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:0712
unsigned char g_abNextB[20] = {
	0x03, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x02, 0x04, 0x08, 0x80, 0x00, 0x00, 0xFF,
	0x00, 0x00, 0x0A, 0x00
};

/* Near pointer: time.h's prototype is far and would push two zeros. */
long cdecl time(long *);

/* First word of the AllocSpriteMove block is the script pointer. */
typedef struct tagNEXTMOVE {
	int id;
} NEXTMOVE;

LRESULT CALLBACK NextItemProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK NextBorderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL NextDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:3CDA
IMAGE g_imgNext[2];


// GLOBAL: AARCADE 0024:3CB0
SCENE g_sceneNextA;


// GLOBAL: AARCADE 0024:3C86
SCENE g_sceneNextB;


// GLOBAL: AARCADE 0024:3C82
long g_lNextDeadline;

// FUNCTION: AARCADE 0012:0000
SPRITE *FAR PASCAL AddNextSprite(SCENE *ps, unsigned char FAR *pc, IMAGE *pimg)
{
	SPRITE *s;
	NEXTMOVE *p;

	/* test AX, not the copies in SI and DI */
	if ((s = AddSprite(ps)) != 0) {
		if ((p = AllocSpriteMove()) != 0) {
			s->dwData = p;
			s->lpfnMoveEx = MoveSprite;
			/* far-to-int keeps the offset; the segment stays on the stack */
			p->id = (int)pc;
		}
		s->cur.fVisible = 0;
		s->cur.img = *pimg;
		s->cur.x = 0;
		s->cur.y = 0;
		UpdateSpriteRect(s);
	}
	return s;
}

// FUNCTION: AARCADE 0012:0066
void FAR PASCAL BackNext(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[1]);
}

// FUNCTION: AARCADE 0012:007E
void FAR PASCAL OpenNext(HWND hwnd, int idA, int idB)
{
	RECT rc;
	SCENEBKPROC lpfn;
	HWND hwndItem;

	lpfn = NULL;
	if (g_fColor)
		lpfn = BackNext;
	LoadPPicImages(5000, 0, 2, g_imgNext, g_nDepth);
	hwndItem = GetDlgItem(hwnd, idA);
	GetClientRect(hwndItem, &rc);
	SubclassAnimItem(hwnd, idA, NextItemProc, 3);
	InitScene(hwndItem, &g_sceneNextA, lpfn, &rc, g_nDepth2, 2, 0);
	AddNextSprite(&g_sceneNextA, g_abNextA, &g_imgNext[0]);
	hwndItem = GetDlgItem(hwnd, idB);
	GetClientRect(hwndItem, &rc);
	SubclassUserItem(hwnd, idB, NextItemProc);
	InitScene(hwndItem, &g_sceneNextB, lpfn, &rc, g_nDepth2, 2, 0);
	AddNextSprite(&g_sceneNextB, g_abNextB, &g_imgNext[1]);
}

// FUNCTION: AARCADE 0012:015F
void FAR PASCAL CloseNext(void)
{
	FreeScene(&g_sceneNextA);
	FreeScene(&g_sceneNextB);
	FreeImages(g_imgNext, 2, 1);
}

// FUNCTION: AARCADE 0012:0182
LRESULT CALLBACK NextItemProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ERASEBKGND:
		break;
	case WM_PAINT: {
		int id;
		int y;
		RECT rc;
		HPEN hpen;
		HGDIOBJ hOld;
		HDC hdc;

		id = GetDlgCtrlID(hwnd);
		GetClientRect(hwnd, &rc);
		if (id == 1) {
			PaintScene(&g_sceneNextA);
			y = rc.bottom - 1;
		} else {
			PaintScene(&g_sceneNextB);
			y = rc.top;
		}
		hdc = GetDC(hwnd);
		hpen = CreatePen(0, 2, 0);
		hOld = SelectObject(hdc, hpen);
		MoveTo(hdc, 0, y);
		LineTo(hdc, 0x200, y);
		DeleteObject(SelectObject(hdc, hOld));
		ReleaseDC(hwnd, hdc);
	}
		/* break after the block: two jumps out of the case */
		break;
	case WM_TIMER: {
		HWND hwndParent;

		TickSound();
		TickScene(&g_sceneNextA);
		TickScene(&g_sceneNextB);
		if (time(0) >= g_lNextDeadline) {
			hwndParent = GetParent(hwnd);
			SendMessage(hwndParent, WM_COMMAND, 1, 0L);
		}
		break;
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0012:029D
LRESULT CALLBACK NextBorderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ERASEBKGND: {
		HDC hdc;
		RECT rc;

		hdc = (HDC)wParam;
		GetClientRect(hwnd, &rc);
		if (g_fColor)
			FillRect(hdc, &rc, g_hGdiObjects[1]);
		else
			FillRect(hdc, &rc, GetStockObject(WHITE_BRUSH));
		break;
	}
	case WM_PAINT: {
		HPALETTE hpal;
		PAINTSTRUCT ps;
		HDC hdc;

		hdc = BeginPaint(hwnd, &ps);
		if (g_fColor) {
			hpal = GetGamePalette();
			if (hpal != 0) {
				SelectPalette(hdc, hpal, FALSE);
				RealizePalette(hdc);
			}
		}
		PaintNextScores(hdc);
		EndPaint(hwnd, &ps);
	}
		/* break after the block: two jumps out of the case */
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0012:0358
void FAR PASCAL SubclassNextBorder(HWND hwnd, int id)
{
	SubclassUserItem(hwnd, id, NextBorderProc);
}

// FUNCTION: AARCADE 0012:0372
int FAR PASCAL RunNextDialog(HWND hwnd)
{
	return RunModalDialog(0x10E, NextDlgProc, hwnd, 0);
}

// FUNCTION: AARCADE 0012:0390
BOOL FAR PASCAL NextDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData)
{
	int i;

	switch (msg) {
	case WM_INITDIALOG:
		MoveWindow(GetDlgItem(hDlg, 1), 0, 0, 0x202, 0x23, FALSE);
		MoveWindow(GetDlgItem(hDlg, 2), 0, 0x23, 0x202, 0x112, FALSE);
		MoveWindow(GetDlgItem(hDlg, 3), 0, 0x135, 0x202, 0x23, FALSE);
		SubclassNextBorder(hDlg, 2);
		OpenNext(hDlg, 1, 3);
		MoveWindow(hDlg, 0, 0, 0x202, 0x158, FALSE);
		CenterPopup(hDlg);
		g_lNextDeadline = time(0) + 15;
		ShowWindow(hDlg, SW_SHOW);
		UpdateWindow(hDlg);
		for (i = 0; i < 4; i++) {
			if (g_anDailyRank[i] != 0) {
				PlaySoundEx(0x2331, 1, 1);
				break;
			}
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
	case WM_DESTROY:
		CloseNext();
		UnsubclassAnimItem(hDlg, 1);
		UnsubclassUserItem(hDlg, 2);
		UnsubclassUserItem(hDlg, 3);
		return 0;
	case WM_COMMAND:
		switch (wParam) {
		case 12: {
			int nState;

			nState = GetKeyState(VK_SHIFT);
			if ((nState & 0x8000) == 0)
				RunResetScores(hDlg);
			else
				RunEraseScores(hDlg);
			HideDlgItem(hDlg, 2);
			ShowDlgItem(hDlg, 2);
			g_lNextDeadline = time(0) + 15;
			break;
		}
		default:
			wParam = 1;
		case IDM_CHILD_FIRST:
			EndModalDialog(hDlg, wParam);
			break;
		}
		break;
	default:
		return 0;
	}
	return 1;
}
