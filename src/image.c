#include <mem.h>
#include <stdlib.h>
#include <string.h>
#include "aarcade.h"

/* Off-screen images, blitting, and Mac-ported resource / dialog-control helpers. */

/* Resource types, control classes, and the font-name pool (0024:048C..04EB).
     draw.c and move.c have no initialized data, so play.c follows immediately. */
// GLOBAL: AARCADE 0024:048C
char g_szWaveType[] = "WAVE";

// GLOBAL: AARCADE 0024:0491
char g_szPPicType[] = "PPIC";

// GLOBAL: AARCADE 0024:0496
char g_szPPctType[] = "PPCT";

// GLOBAL: AARCADE 0024:049B
char g_szPictType[] = "PICT";

// GLOBAL: AARCADE 0024:04A0
char g_szSwampGasClass[] = "SwampGasDlg";

// GLOBAL: AARCADE 0024:04AC
WORD *g_pwPalIndex = 0;

// GLOBAL: AARCADE 0024:04AE
BITMAPINFO *g_pbmi = 0;

// GLOBAL: AARCADE 0024:04B0
char g_szUserItemClass[] = "USERITEM";

// GLOBAL: AARCADE 0024:04B9
char g_szUserDblClkClass[] = "USERDBLCLK";

// GLOBAL: AARCADE 0024:04C4
char g_szParamTextClass[] = "PARAMTEXT";

// GLOBAL: AARCADE 0024:04CE
char g_szAnimItemClass[] = "ANIMITEM";

// GLOBAL: AARCADE 0024:04D7
char g_szPicItemClass[] = "PICITEM";

// GLOBAL: AARCADE 0024:04DF
int g_fParamPainting = 0;

/* Face name and the empty default, then the anim-window title and the %ld format. */
// GLOBAL: AARCADE 0024:04E1
char g_szHelv[] = "Helv";

// GLOBAL: AARCADE 0024:04E6
char g_szFontFace[] = "";

// GLOBAL: AARCADE 0024:04E7
char g_szAnimText[] = "";

// GLOBAL: AARCADE 0024:04E8
char g_szLongFmt[] = "%ld";

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:3216
GLOBALENTRY g_aGlobals[256];


// GLOBAL: AARCADE 0024:320E
HINSTANCE g_ahLibs[4];


// GLOBAL: AARCADE 0024:2E0E
char g_aszParam[4][256];


// GLOBAL: AARCADE 0024:2DE6
char g_szNumBuf[40];

// FUNCTION: AARCADE 000A:0000
LPVOID FAR PASCAL GAllocLock(UINT fuFlags, DWORD cb)
{
	LPVOID lp = NULL;
	HGLOBAL h;
	int i;

	h = GlobalAlloc(fuFlags, cb);
	if (h) {
		if ((i = FindGlobal(NULL)) >= 0) {
			lp = GlobalLock(h);
			g_aGlobals[i].h = h;
			g_aGlobals[i].lp = lp;
		} else {
			/* empty else blocks (probably a debug macro) still emit a jmp */
		}
	}
	return lp;
}

// FUNCTION: AARCADE 000A:006F
void FAR PASCAL GUnlockFree(LPVOID lp)
{
	int i;
	HGLOBAL h;

	if (lp) {
		if ((i = FindGlobal(lp)) >= 0) {
			if ((h = g_aGlobals[i].h) != 0) {
				GlobalUnlock(h);
				GlobalFree(h);
				g_aGlobals[i].h = 0;
				g_aGlobals[i].lp = 0;
			} else {
			}
		} else {
		}
	}
}

// FUNCTION: AARCADE 000A:00D0
int FAR PASCAL FindGlobal(LPVOID lp)
{
	int i;

	for (i = 0; i < 256; i++)
		if (g_aGlobals[i].lp == lp)
			return i;
	return -1;
}

// FUNCTION: AARCADE 000A:0104
void FAR PASCAL GFree(HGLOBAL h)
{
	if (h)
		GlobalFree(h);
}

// FUNCTION: AARCADE 000A:011C
LPVOID FAR PASCAL GLock(HGLOBAL h)
{
	return h ? GlobalLock(h) : NULL;
}

// FUNCTION: AARCADE 000A:013C
void FAR PASCAL GUnlock(HGLOBAL h)
{
	if (h)
		GlobalUnlock(h);
}

// FUNCTION: AARCADE 000A:0154
LPVOID FAR PASCAL LockRes(HGLOBAL h)
{
	return h ? LockResource(h) : NULL;
}

// FUNCTION: AARCADE 000A:0174
void FAR PASCAL UnlockRes(HGLOBAL h)
{
	if (h)
		UnlockResource(h);
}

// FUNCTION: AARCADE 000A:018C
void FAR PASCAL FreeRes(HGLOBAL h)
{
	if (h)
		FreeResource(h);
}

// FUNCTION: AARCADE 000A:01A4
BITMAPINFO *FAR PASCAL MakeBitmapInfo(IMAGE *pimg, int cx, int cy)
{
	int nColors;
	HPALETTE hpal;
	BITMAPINFO *pbmi;
	int i;

	if (!g_pbmi)
		g_pbmi = (BITMAPINFO *)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
	pbmi = g_pbmi;
	if (pbmi) {
		pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pbmi->bmiHeader.biWidth = cx;
		pbmi->bmiHeader.biHeight = cy;
		pbmi->bmiHeader.biPlanes = 1;
		pbmi->bmiHeader.biBitCount = pimg->nDepth;
		pbmi->bmiHeader.biCompression = 0;
		pbmi->bmiHeader.biSizeImage = 0;
		pbmi->bmiHeader.biXPelsPerMeter = 0;
		pbmi->bmiHeader.biYPelsPerMeter = 0;
		if (!g_pwPalIndex) {
			hpal = GetGamePalette();
			if (hpal) {
				nColors = 256;
				g_pwPalIndex = (WORD *)pbmi->bmiColors;
				for (i = 0; i < 256; i++) {
					if (i < 16)
						g_pwPalIndex[i] = GetNearestPaletteIndex(hpal,
							RGB(g_rgbGame[i].rgbRed, g_rgbGame[i].rgbGreen, g_rgbGame[i].rgbBlue));
					else
						g_pwPalIndex[i] = 0;
				}
			}
			pbmi->bmiHeader.biClrUsed = nColors;
			pbmi->bmiHeader.biClrImportant = 16;
		}
	}
	return pbmi;
}

// FUNCTION: AARCADE 000A:02C6
void FAR PASCAL CreateImageBitmap(HWND hwnd, IMAGE *pimg, LPVOID lpBits, int unused)
{
	HDC hdc = 0;
	HDC hdcRelease = 0;
	int cx, cy;
	BITMAPINFO *pbmi;

	if (!hdc) {
		if (!hwnd)
			hwnd = g_hwndMain;
		hdcRelease = hdc = GetDC(hwnd);
	}
	cx = pimg->rc.right - pimg->rc.left;
	cy = pimg->rc.bottom - pimg->rc.top;
	if (pimg->nDepth <= 1) {
		pimg->hbm = CreateBitmap(cx, cy, 1, 1, lpBits);
		pimg->nDepth = 1;
	} else {
		pbmi = MakeBitmapInfo(pimg, cx, cy);
		if (lpBits)
			pimg->hbm = CreateDIBitmap(hdc, &pbmi->bmiHeader, CBM_INIT, lpBits, pbmi, DIB_PAL_COLORS);
		else
			pimg->hbm = CreateDIBitmap(hdc, &pbmi->bmiHeader, 0, NULL, NULL, 0);
	}
	if (hdcRelease)
		ReleaseDC(hwnd, hdcRelease);
}

// FUNCTION: AARCADE 000A:0391
BOOL FAR PASCAL CreateImage(HWND hwnd, IMAGE *pimg, LPRECT lprc, int nDepth, int unused, LPVOID lpBits, int n)
{
	IMAGE img;

	memset(&img, 0, sizeof(img));
	img.nDepth = nDepth;
	img.rc = *lprc;
	CreateImageBitmap(hwnd, &img, lpBits, n);
	*pimg = img;
	return img.hbm != 0;
}

// FUNCTION: AARCADE 000A:03FA
void FAR PASCAL FreeImage(IMAGE *pimg)
{
	if (pimg) {
		if (pimg->hbm)
			DeleteObject(pimg->hbm);
		pimg->hbm = 0;
	}
}

// FUNCTION: AARCADE 000A:041F
void FAR PASCAL FillImage(IMAGE *pimg, RECT *prc, HBRUSH hbr)
{
	HGDIOBJ hOld;

	hOld = SelectObject(g_hdcMemA, pimg->hbm);
	FillRect(g_hdcMemA, prc, hbr);
	SelectObject(g_hdcMemA, hOld);
}

// FUNCTION: AARCADE 000A:045B
int FAR PASCAL GetImageWidthBytes(IMAGE *pimg)
{
	HBITMAP hbm;
	BITMAP bm;

	hbm = pimg->hbm;
	if (GetObject(hbm, sizeof(bm), &bm))
		return bm.bmWidthBytes;
	return 0;
}

// FUNCTION: AARCADE 000A:048E
void FAR PASCAL SetImageBits(IMAGE *pimg, LPVOID lpBits, DWORD cb)
{
	BITMAPINFO *pbmi;
	BITMAP bm;
	HBITMAP hbm;

	if (pimg->nDepth == 1)
		SetBitmapBits(pimg->hbm, cb, lpBits);
	else {
		hbm = pimg->hbm;
		if (!GetObject(hbm, sizeof(bm), &bm))
			return;
		pbmi = MakeBitmapInfo(pimg, bm.bmWidth, bm.bmHeight);
		SetDIBits(g_hdcMemA, hbm, 0, bm.bmHeight, lpBits, pbmi, DIB_PAL_COLORS);
	}
}

// FUNCTION: AARCADE 000A:04FC
void FAR PASCAL MaskBlitImage(IMAGE *pimgSrc, IMAGE *pimgMask, IMAGE *pimgDst, int a, int b, LPRECT lprc, int c, int d)
{
	BlitImage(pimgDst, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
	          pimgMask, 0, 0, SRCERASE);
	BlitImage(pimgDst, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
	          pimgSrc, 0, 0, SRCINVERT);
}

// FUNCTION: AARCADE 000A:0581
void FAR PASCAL CopyBlitImage(IMAGE *pimgSrc, IMAGE *pimgDst, int a, int b, LPRECT lprc, int nMode, int c)
{
	DWORD dwRop;

	switch (nMode) {
	case 0:
		dwRop = SRCCOPY;
		break;
	case 1:
		dwRop = SRCAND;
		break;
	case 2:
		dwRop = MERGEPAINT;
		break;
	case 3:
		dwRop = SRCINVERT;
		break;
	default:
		break;
	}
	BlitImage(pimgDst, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
	          pimgSrc, 0, 0, dwRop);
}

// FUNCTION: AARCADE 000A:0615
BOOL FAR PASCAL BlitImage(IMAGE *pimgDst, int x, int y, int cx, int cy, IMAGE *pimgSrc, int xSrc, int ySrc, DWORD dwRop)
{
	BOOL fResult;
	HGDIOBJ hOldA;
	HGDIOBJ hOldB;

	if (pimgDst->hbm && pimgSrc->hbm) {
		hOldA = SelectObject(g_hdcMemA, pimgSrc->hbm);
		hOldB = SelectObject(g_hdcMemB, pimgDst->hbm);
		fResult = BitBlt(g_hdcMemB, x, y, cx, cy, g_hdcMemA, xSrc, ySrc, dwRop);
		SelectObject(g_hdcMemA, hOldA);
		SelectObject(g_hdcMemB, hOldB);
		return fResult;
	} else
		return FALSE;
}

// FUNCTION: AARCADE 000A:06A0
BOOL FAR PASCAL BlitImageToDC(HDC hdc, int x, int y, int cx, int cy, IMAGE *pimg, int xSrc, int ySrc, DWORD dwRop)
{
	BOOL fResult;
	HGDIOBJ hOld;

	if (hdc && pimg->hbm) {
		hOld = SelectObject(g_hdcMemA, pimg->hbm);
		fResult = BitBlt(hdc, x, y, cx, cy, g_hdcMemA, xSrc, ySrc, dwRop);
		SelectObject(g_hdcMemA, hOld);
		return fResult;
	} else
		return FALSE;
}

// FUNCTION: AARCADE 000A:070B
int FAR PASCAL OpenDataFile(char *pszName, int unused, HFILE *phf)
{
	int nErr = 0;
	char *p;
	OFSTRUCT of;
	char szPath[128];
	HFILE hf;

	GetModuleFileName(g_hInstance, szPath, sizeof(szPath));
	if ((p = strrchr(szPath, '\\')) != 0) {
		p[1] = 0;
		strcat(szPath, pszName);
	} else
		strcpy(szPath, pszName);
	hf = OpenFile(szPath, &of, OF_READWRITE);
	if (hf == HFILE_ERROR) {
		nErr = of.nErrCode;
		hf = 0;
	}
	*phf = hf;
	return nErr;
}

// FUNCTION: AARCADE 000A:079B
int FAR PASCAL SeekFile(HFILE hf, int nOrigin, LONG lOffset)
{
	if (_llseek(hf, lOffset, nOrigin) == -1L)
		return -1;
	else
		return 0;
}

// FUNCTION: AARCADE 000A:07CA
BOOL FAR PASCAL ReadFileHuge(HFILE hf, DWORD *pcb, BYTE huge *pb)
{
	long cbRead;
	long cbTotal;
	long cbChunk;
	long cbLeft;
	BOOL fErr;

	cbLeft = *pcb;
	cbTotal = 0;
	do {
		if (cbLeft > 0xFFFFL)
			cbChunk = 0xFFFFL;
		else
			cbChunk = cbLeft;
		cbRead = _lread(hf, pb, (UINT)cbChunk);
		fErr = cbRead != cbChunk;
		cbTotal += cbRead;
		cbLeft -= cbRead;
		pb = pb + cbRead;
	} while (cbLeft > 0 && !fErr);
	*pcb = cbTotal;
	return fErr;
}

// FUNCTION: AARCADE 000A:08A0
int FAR PASCAL CloseFile(HFILE hf)
{
	return _lclose(hf);
}

// FUNCTION: AARCADE 000A:08B3
int FAR PASCAL FindLibrarySlot(HINSTANCE hLib)
{
	int i;

	for (i = 0; i < 4; i++)
		if (g_ahLibs[i] == hLib)
			return i;
	return -1;
}

// FUNCTION: AARCADE 000A:08DC
HINSTANCE FAR PASCAL LoadDataLibrary(char *pszName)
{
	int i;
	char *p;
	char szPath[128];
	HINSTANCE hLib;

	GetModuleFileName(g_hInstance, szPath, sizeof(szPath));
	if ((p = strrchr(szPath, '\\')) != 0) {
		p[1] = 0;
		strcat(szPath, pszName);
	} else
		strcpy(szPath, pszName);
	hLib = LoadLibrary(szPath);
	if (hLib >= 32) {
		if ((i = FindLibrarySlot(0)) >= 0)
			g_ahLibs[i] = hLib;
	} else {
	}
	return hLib;
}

// FUNCTION: AARCADE 000A:0968
void FAR PASCAL FreeDataLibrary(HINSTANCE hLib)
{
	int i;

	if (hLib >= 32) {
		FreeLibrary(hLib);
		if ((i = FindLibrarySlot(hLib)) >= 0)
			g_ahLibs[i] = 0;
	} else {
	}
}

// FUNCTION: AARCADE 000A:099D
HGLOBAL FAR PASCAL LoadRes(HINSTANCE hInst, LPCSTR lpType, int id)
{
	HGLOBAL h;
	HRSRC hrsrc;

	hrsrc = FindResource(hInst, MAKEINTRESOURCE(id), lpType);
	if (!hrsrc)
		return 0;
	h = LoadResource(hInst, hrsrc);
	return h;
}

// FUNCTION: AARCADE 000A:09DA
HGLOBAL FAR PASCAL FindRes(LPCSTR lpType, int id)
{
	int i = 4;
	HGLOBAL h = 0;

	while (--i >= 0) {
		if (g_ahLibs[i]) {
			if ((h = LoadRes(g_ahLibs[i], lpType, id)) != 0)
				break;
		}
	}
	if (!h)
		h = LoadRes(g_hInstance, lpType, id);
	return h;
}

// FUNCTION: AARCADE 000A:0A34
void FAR PASCAL CopyResToGlobal(HGLOBAL FAR *ph)
{
	LPVOID lpSrc;
	LPVOID lpDst;
	DWORD cb;
	HGLOBAL h;

	if ((lpSrc = LockRes(*ph)) != NULL) {
		cb = GlobalSize(*ph);
		h = GlobalAlloc(GMEM_MOVEABLE, cb);
		if ((lpDst = GLock(h)) != NULL)
			_fmemcpy(lpDst, lpSrc, (size_t)cb);
		else {
		}
		UnlockRes(*ph);
		FreeRes(*ph);
		GUnlock(h);
		*ph = h;
	}
}

// FUNCTION: AARCADE 000A:0ABD
char *FAR PASCAL LoadStr(int id, char *buf, int cb)
{
	LoadString(g_hInstance, id, buf, cb);
	return buf;
}

// FUNCTION: AARCADE 000A:0AE0
HGDIOBJ FAR PASCAL CreateGameFont(HDC hdc, int nFace, int nHeight, UINT fuStyle)
{
	char *pszFace;
	HGDIOBJ hfont;
	HGDIOBJ hRet;
	LOGFONT *plf;

	plf = (LOGFONT *)LocalAlloc(LPTR, sizeof(LOGFONT));
	if (plf) {
		plf->lfHeight = -nHeight;
		if (fuStyle & 1)
			plf->lfWeight = FW_BOLD;
		else if (fuStyle & 2)
			plf->lfWeight = FW_EXTRABOLD;
		if (fuStyle & 4)
			plf->lfUnderline = 1;
		switch (nFace) {
		case 2:
			pszFace = g_szHelv;
			break;
		default:
			pszFace = g_szFontFace;
			break;
		}
		lstrcpy(plf->lfFaceName, pszFace);
		hfont = CreateFontIndirect(plf);
		LocalFree((HLOCAL)plf);
	} else
		hfont = GetStockObject(SYSTEM_FONT);
	hRet = hdc ? SelectObject(hdc, hfont) : hfont;
	return hRet;
}

// FUNCTION: AARCADE 000A:0B8D
void FAR PASCAL FillRectWhite(HDC hdc, LPRECT lprc)
{
	FillRect(hdc, lprc, GetStockObject(WHITE_BRUSH));
}

// FUNCTION: AARCADE 000A:0BAC
void FAR PASCAL FillRectBlack(HDC hdc, LPRECT lprc)
{
	HGDIOBJ hOld;

	hOld = SelectObject(hdc, GetStockObject(BLACK_BRUSH));
	/* fills with the previously selected brush, as in the original */
	FillRect(hdc, lprc, hOld);
	SelectObject(hdc, hOld);
}

// FUNCTION: AARCADE 000A:0BE0
void FAR PASCAL FrameRoundRect(HDC hdc, LPRECT lprc, int cxEllipse, int cyEllipse)
{
	HGDIOBJ hOld;

	hOld = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	RoundRect(hdc, lprc->left, lprc->top, lprc->right, lprc->bottom, cxEllipse, cyEllipse);
	SelectObject(hdc, hOld);
}

// FUNCTION: AARCADE 000A:0C32
void FAR PASCAL InvertRoundRect(HDC hdc, LPRECT lprc, int cxEllipse, int cyEllipse)
{
	int nOldRop;
	HGDIOBJ hOld;

	nOldRop = SetROP2(hdc, R2_NOT);
	hOld = SelectObject(hdc, GetStockObject(NULL_PEN));
	RoundRect(hdc, lprc->left, lprc->top, lprc->right, lprc->bottom, cxEllipse, cyEllipse);
	SetROP2(hdc, nOldRop);
	SelectObject(hdc, hOld);
}

// FUNCTION: AARCADE 000A:0C98
void FAR PASCAL DrawTextInRect(HDC hdc, LPSTR lpsz, int cch, LPRECT lprc, int fCenter)
{
	int y;
	int cxText;
	int x;

	x = lprc->left;
	y = lprc->bottom;
	if (fCenter == 1) {
		cxText = GetTextExtent(hdc, lpsz, cch);
		x = x + ((lprc->right - lprc->left) >> 1) - (cxText >> 1);
	}
	SetTextAlign(hdc, TA_BOTTOM | TA_UPDATECP);
	MoveTo(hdc, x, y);
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED, lprc, lpsz, cch, NULL);
}

// FUNCTION: AARCADE 000A:0D20
void FAR PASCAL TextOutPascal(HDC hdc, BYTE FAR *lpps)
{
	SetTextAlign(hdc, TA_BASELINE | TA_UPDATECP);
	TextOut(hdc, 0, 0, (LPCSTR)(lpps + 1), *lpps);
}

// FUNCTION: AARCADE 000A:0D51
DWORD FAR PASCAL GetTextExtentPascal(HDC hdc, BYTE FAR *lpps)
{
	return GetTextExtent(hdc, (LPCSTR)(lpps + 1), *lpps);
}

// FUNCTION: AARCADE 000A:0D75
void FAR PASCAL TermControls(void)
{
	UnregisterClass(g_szPicItemClass, g_hInstance);
	UnregisterClass(g_szParamTextClass, g_hInstance);
	UnregisterClass(g_szAnimItemClass, g_hInstance);
	UnregisterClass(g_szUserDblClkClass, g_hInstance);
	UnregisterClass(g_szUserItemClass, g_hInstance);
	UnregisterClass(g_szSwampGasClass, g_hInstance);
	if (g_pbmi) {
		LocalFree((HLOCAL)g_pbmi);
		g_pbmi = 0;
	}
	if (g_hdcMemA)
		DeleteDC(g_hdcMemA);
	if (g_hdcMemB)
		DeleteDC(g_hdcMemB);
	DestroyCursor(g_hCursor);
}

// FUNCTION: AARCADE 000A:0E09
LRESULT CALLBACK _export SwampGasDlgClsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TickSound();
	return DefDlgProc(hwnd, msg, wParam, lParam);
}

// FUNCTION: AARCADE 000A:0E34
LRESULT CALLBACK _export UserWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC lpfnWnd;
	WNDPROC lpfnCls;
	LRESULT lResult;
	HWND hwndParent;
	int id;

	lResult = 0;
	lpfnWnd = (WNDPROC)GetWindowLong(hwnd, g_nUserWndExtra + 2);
	lpfnCls = (WNDPROC)GetClassLong(hwnd, g_nUserClsExtra);
	if (lpfnWnd)
		lResult = lpfnWnd(hwnd, msg, wParam, lParam);
	if (!lResult && lpfnCls)
		lResult = lpfnCls(hwnd, msg, wParam, lParam);
	if (!lResult) {
		switch (msg) {
		case WM_LBUTTONDOWN:
			if (TrackUserButton(hwnd, lParam)) {
				id = GetDlgCtrlID(hwnd);
				hwndParent = GetParent(hwnd);
				PostMessage(hwndParent, WM_COMMAND, id, MAKELPARAM(hwnd, 0));
			}
			lResult = 1;
			break;
		case WM_LBUTTONUP:
			lResult = 1;
			break;
		default:
			if (!lpfnWnd && !lpfnCls)
				lResult = DefWindowProc(hwnd, msg, wParam, lParam);
			break;
		}
	}
	return lResult;
}

// FUNCTION: AARCADE 000A:0F48
HWND FAR PASCAL SubclassAnimItem(HWND hDlg, int id, WNDPROC lpfn, int nRate)
{
	HWND hwnd;

	if ((hwnd = SubclassUserItem(hDlg, id, lpfn)) != 0)
		StartAnimTimer(hwnd, nRate, TRUE);
	return hwnd;
}

// FUNCTION: AARCADE 000A:0F79
void FAR PASCAL UnsubclassAnimItem(HWND hDlg, int id)
{
	StopAnimTimer();
	UnsubclassUserItem(hDlg, id);
}

// FUNCTION: AARCADE 000A:0F92
HWND FAR PASCAL SubclassUserItem(HWND hDlg, int id, WNDPROC lpfn)
{
	HWND hwnd;

	if ((hwnd = GetDlgItem(hDlg, id)) != 0)
		SetWindowLong(hwnd, g_nUserWndExtra + 2, (LONG)lpfn);
	return hwnd;
}

// FUNCTION: AARCADE 000A:0FC5
void FAR PASCAL UnsubclassUserItem(HWND hDlg, int id)
{
	HWND hwnd;

	if ((hwnd = GetDlgItem(hDlg, id)) != 0)
		SetWindowLong(hwnd, g_nUserWndExtra + 2, 0L);
}

// FUNCTION: AARCADE 000A:0FF6
HWND FAR PASCAL CreateUserWindow(HWND hwndParent, int id, LPCSTR lpszClass, LPCSTR lpszText, DWORD dwStyle, LPRECT lprc, WNDPROC lpfn)
{
	HWND hwnd;

	hwnd = CreateWindow(lpszClass ? lpszClass : g_szUserItemClass, lpszText,
		WS_CHILD | dwStyle,
		lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
		hwndParent, (HMENU)id, g_hInstance, NULL);
	if (hwnd)
		SetWindowLong(hwnd, g_nUserWndExtra + 2, (LONG)lpfn);
	return hwnd;
}

// FUNCTION: AARCADE 000A:1085
HWND FAR PASCAL CreateAnimWindow(HWND hwndParent, LPRECT lprc, WNDPROC lpfn, BOOL fFlag, int nRate)
{
	RECT rc;
	HWND hwnd;

	rc = *lprc;
	InflateRect(&rc, 1, 1);
	hwnd = CreateUserWindow(hwndParent, 10, g_szAnimItemClass, g_szAnimText, WS_CHILD | WS_BORDER, &rc, lpfn);
	if (hwnd) {
		if (fFlag)
			CenterChild(hwnd);
		StartAnimTimer(hwnd, nRate, FALSE);
		ShowWindow(hwnd, SW_SHOW);
	}
	return hwnd;
}

// FUNCTION: AARCADE 000A:10FE
void FAR PASCAL DestroyAnimWindow(HWND hwnd)
{
	StopAnimTimer();
	if (GetCapture() == hwnd)
		ReleaseCapture();
	DestroyWindow(hwnd);
}

// FUNCTION: AARCADE 000A:1125
BOOL FAR PASCAL TrackUserButton(HWND hwnd, LPARAM lParam)
{
	POINT pt;
	BOOL fIn;
	RECT rc;
	HWND hwndOld;
	MSG msg;
	BOOL fResult;

	fResult = TRUE;
	if ((hwndOld = GetCapture()) != 0)
		ReleaseCapture();
	SetCapture(hwnd);
	GetClientRect(hwnd, &rc);
	for (;;) {
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE | PM_NOYIELD)) {
			switch (msg.message) {
			case WM_MOUSEMOVE:
				pt = MAKEPOINT(msg.lParam);
				fIn = PtInRect(&rc, pt);
				if (fIn != fResult)
					fResult = fIn;
				break;
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
				ReleaseCapture();
				if (hwndOld)
					SetCapture(hwndOld);
				PostMessage(hwnd, msg.message, msg.wParam, msg.lParam);
				return fResult;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
				break;
			default:
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				break;
			}
		}
	}
}

// FUNCTION: AARCADE 000A:121A
int FAR PASCAL GetWindowNumber(HWND hwnd)
{
	char sz[16];
	int n;

	n = 0;
	GetWindowText(hwnd, sz, 15);
	if (sz[0] == '#')
		n = (int)atol(sz + 1);
	return n;
}

// FUNCTION: AARCADE 000A:124F
LRESULT FAR PASCAL PicItemProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_PAINT: {
		HDC hdc;
		RECT rc;
		int id;
		PAINTSTRUCT ps;
		HBITMAP hbm;

		if ((hbm = GetWindowWord(hwnd, g_nUserWndExtra + 6)) == 0) {
			id = GetWindowNumber(hwnd);
			hbm = LoadPicture(id, TRUE);
			SetWindowWord(hwnd, g_nUserWndExtra + 6, hbm);
		}
		hdc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rc);
		DrawPicture(hdc, hbm, &rc);
		EndPaint(hwnd, &ps);
	}
		/* break after the block: the original has two jmps here */
		break;
	case WM_DESTROY: {
		HBITMAP hbm;

		if ((hbm = GetWindowWord(hwnd, g_nUserWndExtra + 6)) != 0) {
			FreePicture(hbm, TRUE);
			SetWindowWord(hwnd, g_nUserWndExtra + 6, 0);
		}
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1L;
}

// FUNCTION: AARCADE 000A:1325
LRESULT FAR PASCAL ParamTextProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CREATE:
		SetWindowLong(hwnd, g_nUserWndExtra + 10, 0x00FFFFFFL);
		break;
	case WM_ERASEBKGND:
		break;
	case WM_PAINT: {
		HFONT hfont;
		HFONT hfontOld;
		RECT rc;
		COLORREF rgb;
		COLORREF rgbOld;
		char sz[256];
		PAINTSTRUCT ps;
		HDC hdc;

		g_fParamPainting = 1;
		if (GetWindowText(hwnd, sz, 256)) {
			ExpandParamText(sz, 256);
			SetWindowText(hwnd, sz);
		}
		hfont = GetWindowWord(hwnd, g_nUserWndExtra + 8);
		hdc = BeginPaint(hwnd, &ps);
		rgb = GetWindowLong(hwnd, g_nUserWndExtra + 10);
		SendMessage(GetParent(hwnd), WM_USER + 3, 0, (LPARAM)(LPSTR)&rgb);
		if (g_fColor) {
			HPALETTE hpal;

			hpal = GetGamePalette();
			if (hpal) {
				SelectPalette(hdc, hpal, FALSE);
				RealizePalette(hdc);
			}
		}
		if (hfont)
			hfontOld = SelectObject(hdc, hfont);
		GetClientRect(hwnd, &rc);
		InflateRect(&rc, -1, -1);
		rgbOld = SetBkColor(hdc, rgb);
		{
			/* own block so fmt shares hpal's stack slot */
			UINT fmt;

			fmt = GetWindowWord(hwnd, g_nUserWndExtra + 16) & 3;
			switch (fmt) {
			case 1:
				fmt = DT_CENTER;
				break;
			case 2:
				fmt = DT_RIGHT;
				break;
			case 0:
			default:
				fmt = DT_LEFT;
				break;
			}
			DrawText(hdc, sz, strlen(sz), &rc, fmt | DT_WORDBREAK | DT_NOPREFIX);
		}
		SetBkColor(hdc, rgbOld);
		if (hfont)
			SelectObject(hdc, hfontOld);
		EndPaint(hwnd, &ps);
		g_fParamPainting = 0;
		break;
	}
	case WM_GETFONT:
		return GetWindowWord(hwnd, g_nUserWndExtra + 8);
	case WM_SETFONT:
		SetWindowWord(hwnd, g_nUserWndExtra + 8, wParam);
		return 0L;
	case WM_SETTEXT: {
		LRESULT lResult;

		lResult = DefWindowProc(hwnd, msg, wParam, lParam);
		if (!g_fParamPainting) {
			InvalidateRect(hwnd, NULL, TRUE);
			UpdateWindow(hwnd);
		}
		return lResult;
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1L;
}

// FUNCTION: AARCADE 000A:156F
void FAR PASCAL ParamText(LPSTR lpps0, LPSTR lpps1, LPSTR lpps2, LPSTR lpps3)
{
	if (lpps0) {
		CopyPStr(lpps0, g_aszParam[0]);
		PtoCstr(g_aszParam[0]);
	} else
		g_aszParam[0][0] = 0;
	if (lpps1) {
		CopyPStr(lpps1, g_aszParam[1]);
		PtoCstr(g_aszParam[1]);
	} else
		g_aszParam[1][0] = 0;
	if (lpps2) {
		CopyPStr(lpps2, g_aszParam[2]);
		PtoCstr(g_aszParam[2]);
	} else
		g_aszParam[2][0] = 0;
	if (lpps3) {
		CopyPStr(lpps3, g_aszParam[3]);
		PtoCstr(g_aszParam[3]);
	} else
		g_aszParam[3][0] = 0;
}

// FUNCTION: AARCADE 000A:1614
LPSTR FAR PASCAL FindChar(LPSTR lpsz, int ch)
{
	while (*lpsz) {
		if (*lpsz == ch)
			return lpsz;
		lpsz = AnsiNext(lpsz);
	}
	return NULL;
}

// FUNCTION: AARCADE 000A:1653
LPSTR FAR PASCAL ExpandParamText(LPSTR lpsz, int cbMax)
{
	LPSTR lpCaret;
	LPSTR lp;
	char szRest[256];
	int i;

	if ((lpCaret = FindChar(lpsz, '^')) != NULL) {
		lp = lpCaret;
		lp++;
		i = *lp++ - '0';
		lstrcpy(szRest, lp);
		ExpandParamText(szRest, cbMax);
		if (i >= 0 && i <= 3) {
			*lpCaret = 0;
			if (lstrlen(lpsz) + lstrlen(g_aszParam[i]) < cbMax)
				lstrcat(lpsz, g_aszParam[i]);
		}
		if (lstrlen(lpsz) + lstrlen(szRest) < cbMax)
			lstrcat(lpsz, szRest);
	}
	return lpsz;
}

// FUNCTION: AARCADE 000A:1738
int FAR PASCAL CountIndStrings(int id)
{
	char sz[256];
	int n;

	n = LoadString(g_hInstance, id, sz, 255);
	/* empty then-branch: the original has jne/jmp */
	if (!n) {
	} else
		n = (BYTE)sz[0];
	return n;
}

// FUNCTION: AARCADE 000A:1770
void FAR PASCAL GetIndStr(LPSTR lpps, int id, int index)
{
	int n;

	n = LoadString(g_hInstance, id + index, lpps, 255);
	if (n == 0)
		*lpps = 0;
	CtoPstr(lpps);
}

// FUNCTION: AARCADE 000A:17B0
void FAR PASCAL GetIndString(LPSTR lpps, int id, int index)
{
	GetIndStr(lpps, id, index);
}

// FUNCTION: AARCADE 000A:17C9
LPSTR FAR PASCAL PtoCstr(LPSTR lps)
{
	LPSTR lpSrc;
	LPSTR lpDst;
	int n;

	n = *lps;
	lpSrc = lps + 1;
	lpDst = lps;
	while (n--)
		*lpDst++ = *lpSrc++;
	*lpDst = 0;
	return lps;
}

// FUNCTION: AARCADE 000A:1821
LPSTR FAR PASCAL CtoPstr(LPSTR lps)
{
	LPSTR lpSrc;
	LPSTR lpDst;
	int nLen;
	int n;

	n = lstrlen(lps);
	if (n > 255)
		n = 255;
	nLen = n;
	lpSrc = lps + n - 1;
	lpDst = lpSrc + 1;
	while (n--)
		*lpDst-- = *lpSrc--;
	*lpDst = (char)nLen;
	return lps;
}

// FUNCTION: AARCADE 000A:1890
void FAR PASCAL NumToString(LONG n, LPSTR lpps)
{
	wsprintf(lpps, g_szLongFmt, n);
	CtoPstr(lpps);
}

// FUNCTION: AARCADE 000A:18BB
void FAR PASCAL StringToNum(LPSTR lpps, LONG FAR *lpn)
{
	CopyPStr(lpps, g_szNumBuf);
	PtoCstr(g_szNumBuf);
	*lpn = strtol(g_szNumBuf, NULL, 10);
}

// FUNCTION: AARCADE 000A:18F4
HBITMAP FAR PASCAL LoadPicture(int id, BOOL fCache)
{
	IMAGE img;
	HBITMAP hbm;

	if ((hbm = CacheFind(2, id)) == 0) {
		if ((hbm = LoadPPct(id)) == 0) {
			if (LoadPictImage(id, &img)) {
				hbm = img.hbm;
				img.hbm = 0;
				FreeImage(&img);
			}
		}
		if (fCache)
			CacheAdd(2, id, hbm);
	}
	return hbm;
}

// FUNCTION: AARCADE 000A:1952
void FAR PASCAL GetPictureRect(HBITMAP hbm, LPRECT lprc)
{
	BITMAP bm;

	if (hbm && GetObject(hbm, sizeof(BITMAP), &bm))
		SetRect(lprc, 0, 0, bm.bmWidth, bm.bmHeight);
	else
		SetRect(lprc, 0, 0, 0, 0);
}

// FUNCTION: AARCADE 000A:199D
BOOL FAR PASCAL LoadPictImage(int id, IMAGE *pimg)
{
	RECT rc;
	LPSTR lp;
	WORD FAR *lpw;
	int cx;
	int cy;
	IMAGE img;
	HGLOBAL h;

	h = FindRes(g_szPictType, id);
	if (h) {
		lpw = (WORD FAR *)(lp = LockRes(h));
		cx = SWAPWORD(lpw[0]);
		cy = SWAPWORD(lpw[1]);
		SetRect(&rc, 0, 0, cx, cy);
		lp += 4;
		CreateImage(0, &img, &rc, 1, 1, lp, 0);
		UnlockRes(h);
		FreeRes(h);
		*pimg = img;
		return TRUE;
	}
	return FALSE;
}

// FUNCTION: AARCADE 000A:1A5F
void FAR PASCAL DrawPicture(HDC hdc, HBITMAP hbm, LPRECT lprc)
{
	HGDIOBJ hOld;
	HPALETTE hpal;
	HDC hdcMem;

	hpal = GetGamePalette();
	hdcMem = CreateCompatibleDC(hdc);
	if (hpal) {
		SelectPalette(hdcMem, hpal, FALSE);
		RealizePalette(hdcMem);
	}
	hOld = SelectObject(hdcMem, hbm);
	BitBlt(hdc, lprc->left, lprc->top, lprc->right - lprc->left, lprc->bottom - lprc->top,
		hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdcMem, hOld);
	DeleteDC(hdcMem);
}

// FUNCTION: AARCADE 000A:1AEE
void FAR PASCAL FreePicture(HBITMAP hbm, BOOL fCache)
{
	if (hbm) {
		if (fCache || !CacheIsCached(2, hbm))
			FreeBitmap(hbm);
		if (fCache)
			CacheRelease(2, hbm, 0);
	}
}

// FUNCTION: AARCADE 000A:1B29
void FAR PASCAL FreeBitmap(HBITMAP hbm)
{
	if (hbm)
		DeleteObject(hbm);
}

// FUNCTION: AARCADE 000A:1B41
HBITMAP FAR PASCAL LoadPPic(int id, int n, BOOL fCache)
{
	HGLOBAL h;
	IMAGE img;
	HBITMAP hbm;

	hbm = 0;
	if (n || (hbm = CacheFind(3, id)) == 0) {
		h = FindRes(g_szPPicType, id);
		if (h) {
			GetPPicImage(id, n, &img, g_nDepth);
			FreeRes(h);
			hbm = img.hbm;
			img.hbm = 0;
			FreeImage(&img);
		}
		if (fCache && !n)
			CacheAdd(3, id, hbm);
	}
	return hbm;
}

// FUNCTION: AARCADE 000A:1BBD
void FAR PASCAL FreePPic(HBITMAP hbm, BOOL fCache)
{
	if (hbm) {
		if (fCache || !CacheIsCached(3, hbm))
			FreeBitmap(hbm);
		if (fCache)
			CacheRelease(3, hbm, 0);
	}
}

// FUNCTION: AARCADE 000A:1BF7
void FAR PASCAL CacheAdd(int nType, int id, HANDLE h)
{
}

// FUNCTION: AARCADE 000A:1C00
HANDLE FAR PASCAL CacheFind(int nType, int id)
{
	return 0;
}

// FUNCTION: AARCADE 000A:1C0D
BOOL FAR PASCAL CacheRelease(int nType, HANDLE h, int n)
{
	return 0;
}

// FUNCTION: AARCADE 000A:1C1A
BOOL FAR PASCAL CacheIsCached(int nType, HANDLE h)
{
	return 0;
}

// FUNCTION: AARCADE 000A:1C27
void FAR PASCAL CacheFlush(void)
{
}

// FUNCTION: AARCADE 000A:1C2E
HBITMAP FAR PASCAL LoadLibBitmap(int id)
{
	int i = 4;
	HBITMAP hbm = 0;

	while (--i >= 0) {
		if (g_ahLibs[i]) {
			if ((hbm = LoadBitmap(g_ahLibs[i], MAKEINTRESOURCE(id))) != 0)
				break;
		}
	}
	if (!hbm)
		hbm = LoadBitmap(g_hInstance, MAKEINTRESOURCE(id));
	return hbm;
}

// FUNCTION: AARCADE 000A:1C82
HBITMAP FAR PASCAL LoadPPct(int id)
{
	IMAGE img;
	HBITMAP hbm;

	hbm = 0;
	if (GetPPctImage(id, &img)) {
		hbm = img.hbm;
		img.hbm = 0;
		FreeImage(&img);
	}
	return hbm;
}

/* Mac ticks are 1/60 s; 17 ms is the nearest whole number. */

// FUNCTION: AARCADE 000A:1CB6
DWORD FAR PASCAL GetCaretTime(void)
{
	return GetCaretBlinkTime() / 17;
}

// FUNCTION: AARCADE 000A:1CCD
void FAR PASCAL Delay(DWORD dwTicks, LPDWORD lpdwFinal)
{
	DWORD dwStart;

	dwStart = GetTickCount() / 17;
	while (GetTickCount() / 17 - dwStart <= dwTicks)
		;
	*lpdwFinal = GetTickCount() / 17;
}

// FUNCTION: AARCADE 000A:1D2D
LONG FAR PASCAL Munger(HGLOBAL h, LONG lOffset, LPSTR lpFind, LONG cbFind, LPSTR lpRepl, LONG cbRepl)
{
	LPSTR lpBase;
	LPSTR lpDst;
	LPSTR lpSrc;
	LONG cbOld;
	LONG cbNew;

	if (lpFind)
		return 0L;
	cbOld = GlobalSize(h);
	cbNew = cbOld - cbFind + cbRepl;
	if (cbNew > cbOld)
		GlobalReAlloc(h, cbNew, GMEM_ZEROINIT);
	lpBase = GLock(h);
	if (cbFind >= cbRepl) {
		lpSrc = lpRepl;
		lpDst = lpBase + lOffset;
		while (cbRepl--)
			*lpDst++ = *lpSrc++;
		lpSrc = lpBase + lOffset + cbFind;
		cbRepl = cbNew - lOffset - cbFind;
		while (cbRepl--)
			*lpDst++ = *lpSrc++;
	} else {
		lpSrc = lpBase + cbOld;
		lpDst = lpBase + cbNew;
		while (cbOld-- >= lOffset)
			*lpDst-- = *lpSrc--;
		lpSrc = lpRepl;
		lpDst = lpBase + lOffset;
		while (cbRepl--)
			*lpDst++ = *lpSrc++;
	}
	GUnlock(h);
	if (cbNew < cbOld)
		GlobalReAlloc(h, cbNew, GMEM_ZEROINIT);
	return (DWORD)OFFSETOF(lpDst) - OFFSETOF(lpBase);
}
