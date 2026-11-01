#ifndef AARCADE_H
#define AARCADE_H

#include <windows.h>
#include <mmsystem.h>

/* Menu and internal command IDs (WM_COMMAND wParam). */
#define IDM_CHILD_FIRST 101
#define IDM_CHILD_LAST 103
#define IDM_GAME_FIRST 110
#define IDM_GAME_LAST 119
#define IDC_START 10000
#define IDC_PLAY 10100
#define IDC_NEXT 10101

/* globals.c */
extern HPALETTE g_hPalette;
extern int g_nInactive;
extern BOOL g_fBlackWhite;
extern char g_szMainClass[];
extern char g_szPropLo[];
extern char g_szPropHi[];
extern char g_szPropData[];
extern HWND g_hwndTimer;
extern int g_nTimerRate;
extern HHOOK g_hMsgHook;
extern HACCEL g_hDlgAccel;
extern char g_szSwampGasClass[];
extern char g_szUserItemClass[];
extern char g_szUserDblClkClass[];
extern char g_szParamTextClass[];
extern char g_szAnimItemClass[];
extern char g_szPicItemClass[];
extern char g_szHelv[];
extern char g_szFontFace[];
extern char g_szAnimText[];
extern char g_szLongFmt[];
extern HCURSOR g_hCursor;
extern HGDIOBJ g_hGameFont;
extern char g_szAppTitle[];
extern char g_szHiFile[];
extern BOOL g_fSoundOn;
extern BOOL g_fUseMMIO;
extern RGBQUAD g_rgbVga[16];
extern RGBQUAD g_rgbCustom[16];
extern HWND g_hwndChild;
extern BOOL g_fAnimate;
extern int g_nRound;
extern int g_fDailySaved;
extern HGLOBAL g_hDaily;
extern int g_fAllTimeSaved;
extern HGLOBAL g_hAllTime;
extern unsigned char g_abNextA[20];
extern unsigned char g_abNextB[20];
extern HINSTANCE g_hDataDll;
extern int g_fSession;
/* inifile.c uninitialized, highest address first. The bytes through 3C7F
   are used by util.c; a separate module would word-align and shift 3C31. */
extern int g_nUserClsExtra;
extern int g_nUserWndExtra;
extern RGBQUAD g_rgbGame[16];
extern int g_fLayoutFits;
extern RECT g_rcLayout;
extern int g_fAltLayout;
extern char g_szPlayName[256];
extern long g_lBonusTime;
extern long g_lSavedCount;
extern char g_abBss3B25[4];
extern long g_lActivateTime;
extern char g_abBss3AD9[72];
extern unsigned char g_nMissions;
extern unsigned char g_nEncounters;
extern unsigned char g_nTopics;
extern unsigned char g_nLevel;
extern unsigned g_nCfgVersion;
extern HGLOBAL g_hHold3;
extern char g_abBss3ACC[5];
extern int g_wHold3;
extern HGLOBAL g_hHold2;
extern char g_abBss3AC3[5];
extern int g_wHold2;
extern HGLOBAL g_hHold1;
extern char g_abBss3ABA[5];
extern int g_wHold1;
extern char g_szBWDll[256];
extern char g_szColorDll[256];
extern char g_szScoreFmt[];
extern char g_szList[];
extern char g_szDate[];
extern char g_szAllTime[];
extern char g_szDaily[];
extern char g_szBlank[];
extern char g_szEq1[];
extern char g_szSemi1[];
extern char g_szEq2[];
extern char g_szSemi2[];
/* main.c uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern FARPROC g_lpfnModalDlgProc;
extern int g_nDlgData;
extern int g_nColorDepth;
extern HWND g_hwndMain;
extern HINSTANCE g_hInstance;
extern HICON g_hIcon;
extern POINT g_ptCursor;
extern HWND g_hwndCapture;
/* play.c uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern int g_nPlay;
extern long g_lPlayDeadline;
extern HFONT g_hPlayFont;
extern HDC g_hdcMemA;
extern HDC g_hdcMemB;
/* layout.c uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern char g_abBss38B2[6];
extern unsigned g_anDailyRank[4];
extern unsigned g_anAllTimeRank[4];
extern char g_abBss387E[36];
extern BOOL g_fColor;
extern int g_nDepth;
extern int g_nDepth2;
extern char g_abBss3874[4];
extern RECT g_rcPlaySrc;
extern RECT g_rcMain;
extern HGDIOBJ g_hGdiObjects[5];
extern int g_nR1Left;
extern int g_nPlayAmmo;
extern int g_nPlayWave;
extern long g_lTimeLeft;
extern long g_lPlayScore;
extern RECT g_rcPlay;
extern RECT g_arcFrame[4];
extern int g_nLayout;
extern int g_anPlayCtl[];

/* main.c (segment 2) */
LRESULT CALLBACK _export MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void FAR PASCAL ActivatePrevInstance(HINSTANCE hInstance, HINSTANCE hPrevInstance);
void FAR PASCAL OnActivateApp(BOOL fActive);
void FAR PASCAL DeleteGdiObjects(void);
HPALETTE FAR PASCAL GetGamePalette(void);
void FAR PASCAL PostMainCommand(WORD id);
int FAR PASCAL ShowMessage(HWND hwnd, int idString, UINT fuStyle);

/* init.c (segment 3) */
void FAR PASCAL ParseCmdLine(LPSTR lpCmdLine);
int FAR PASCAL ParseOption(LPSTR lpCmdLine, int i);
BOOL FAR PASCAL InitApplication(HINSTANCE hInstance);
HWND FAR PASCAL CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
BOOL FAR PASCAL InitGame(void);
BOOL FAR PASCAL CreateGamePalette(HWND hwnd);
HBRUSH FAR PASCAL MakeBrush(COLORREF cr);
BOOL FAR PASCAL InitControls(HINSTANCE hInstance);

/* dialog.c (segment 5) */
typedef BOOL (FAR PASCAL *DLGHOOKPROC)(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam, int nData);
int FAR PASCAL RunModalDialog(int idTemplate, DLGHOOKPROC lpfnHook, HWND hwndParent, int nData);
void FAR PASCAL EndModalDialog(HWND hDlg, int nResult);
BOOL CALLBACK _export ModalDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void FAR PASCAL MoveWindowTo(HWND hwnd, int x, int y, BOOL fRepaint);
void FAR PASCAL FlashDlgItem(HWND hDlg, int id);
void FAR PASCAL StartAnimTimer(HWND hwnd, int nRate, BOOL fCheckSibling);
void FAR PASCAL StopAnimTimer(void);
BOOL FAR PASCAL IsTimerSibling(HWND hwnd);
void FAR PASCAL SaveAnimTimer(LPPOINT lppt);
void FAR PASCAL RestoreAnimTimer(LPPOINT lppt);
LRESULT CALLBACK _export TimerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void FAR PASCAL InstallMsgHook(HINSTANCE hInstance, HWND hwnd);
void FAR PASCAL RemoveMsgHook(void);
LRESULT CALLBACK _export MsgHookProc(int code, WPARAM wParam, LPARAM lParam);

/* scene.c (segment 4) */
typedef struct tagIMAGE {
	RECT rc;
	HBITMAP hbm;
	int nDepth;
} IMAGE;

/* The part of a sprite that is saved each tick to find what to redraw. */
typedef struct tagSPRITESTATE {
	char fVisible;
	int x;
	int y;
	RECT rc;
	IMAGE img;
	IMAGE mask;
} SPRITESTATE;

typedef struct tagSPRITE SPRITE;
typedef void (FAR PASCAL *SPRITEPROC)(SPRITE *s);
typedef void (FAR PASCAL *SPRITEMOVEPROC)(SPRITE *s, BOOL fFirst);
typedef void (FAR PASCAL *SPRITEHITTESTPROC)(SPRITE *s, SPRITE *other, BOOL fFirst, int *pfHit);
typedef void (FAR PASCAL *SPRITEHITPROC)(SPRITE *s, SPRITE *other, BOOL fFirst);
typedef void (FAR PASCAL *SPRITEDRAWPROC)(HDC hdc, SPRITE *s, IMAGE *pimg, BOOL fCur);
typedef void (FAR PASCAL *SCENEBKPROC)(IMAGE *pimg, LPRECT lprc);

struct tagSPRITE {
	int nGroup;
	int unk02;
	int nMoveCount;
	int nMoveRate;
	int dx;
	int dy;
	SPRITESTATE cur;
	SPRITESTATE old;
	DWORD dwHitMask;
	char fHitDone;
	char fDirty;
	char fHold;
	char unk5D;
	char fDead;
	char unk5F;
	int nAnimCount;
	int nAnimRate;
	char nFrame;
	char nFrames;
	char nStep;
	char nAnimMode;
	IMAGE *pImages;
	IMAGE *pMasks;
	int nDrawMode; /* < 0: masked blit, else passed to the plain blit */
	void FAR *dwData; /* near block, stored with DS; FreeSprite frees the offset */
	SPRITEPROC lpfnMove;
	SPRITEMOVEPROC lpfnMoveEx;
	SPRITEPROC lpfnMoved;
	SPRITEPROC lpfnAnimate;
	SPRITEHITTESTPROC lpfnHitTest;
	SPRITEHITPROC lpfnHit;
	SPRITEDRAWPROC lpfnDraw;
	SPRITEPROC lpfnTick;
	SPRITE *pPrev;
	SPRITE *pNext;
};

typedef struct tagSCENE {
	HWND hwnd;
	HDC hdc;
	BOOL unk04;
	BOOL unk06;
	int nRate;
	DWORD dwLastTick;
	IMAGE img;
	RECT rc;
	SPRITE *pFirst;
	SPRITE *pLast;
	SCENEBKPROC lpfnBack;
} SCENE;

/* Round 4 keeps a sprite and its move-script bytes in each slot. */
typedef struct tagR4SLOT {
	SPRITE *s;
	char *p;
} R4SLOT;

void FAR PASCAL EmptyProc(void);
int FAR PASCAL InitScene(HWND hwnd, SCENE *ps, SCENEBKPROC lpfnBack, LPRECT lprc, int nDepth, int nRate, int n);
void FAR PASCAL TickScene(SCENE *ps);
void FAR PASCAL FreeScene(SCENE *ps);
void FAR PASCAL PaintScene(SCENE *ps);
BOOL FAR PASCAL RectsOverlap(RECT *a, RECT *b);
void FAR PASCAL UnionRects(RECT *a, RECT *b, RECT *dst);

/* sprite.c (segment 6) */
SPRITE *FAR PASCAL NewSprite(void);
SPRITE *FAR PASCAL InsertSprite(SCENE *ps, SPRITE *before);
SPRITE *FAR PASCAL AddSprite(SCENE *ps);
void FAR PASCAL UnlinkSprite(SCENE *ps, SPRITE *s);
void FAR PASCAL FreeSprite(SCENE *ps, SPRITE *s);
void FAR PASCAL UpdateSpriteRect(SPRITE *s);
void FAR PASCAL FreeImages(IMAGE *pimg, int n, int unused);

/* image.c (segment 10) */
typedef struct tagGLOBALENTRY {
	HGLOBAL h;
	LPVOID lp;
} GLOBALENTRY;

extern WORD *g_pwPalIndex;
extern BITMAPINFO *g_pbmi;
/* image.c uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern GLOBALENTRY g_aGlobals[256];
extern HINSTANCE g_ahLibs[4];
extern char g_aszParam[4][256];
extern char g_szNumBuf[40];
extern char g_szWaveType[];
extern char g_szPPicType[];
extern char g_szPPctType[];
extern char g_szPictType[];
extern int g_fParamPainting;
/* ppic.c uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern LPSTR g_lpPicClut;
extern HLOCAL g_hPicXlat;
extern HLOCAL g_hPicPal;

/* Mac resources are big-endian. */
#define SWAPWORD(w) (HIBYTE(w) | LOBYTE(w) << 8)
/* the (int)(WORD) casts make `GETWORD(p) < 0` test with `or ax,ax` */
#define GETWORD(lp) ((int)(WORD)SWAPWORD(*(WORD FAR *)(lp)))
/* HIWORD makes the low byte a word load with `mov dh,0` */
#define SWAPLONG(l) MAKELONG(SWAPWORD(HIWORD(l)), SWAPWORD(LOWORD(l)))
#define GETLONG(lp) SWAPLONG(*(DWORD FAR *)(lp))

LPVOID FAR PASCAL GAllocLock(UINT fuFlags, DWORD cb);
void FAR PASCAL GUnlockFree(LPVOID lp);
int FAR PASCAL FindGlobal(LPVOID lp);
void FAR PASCAL GFree(HGLOBAL h);
LPVOID FAR PASCAL GLock(HGLOBAL h);
void FAR PASCAL GUnlock(HGLOBAL h);
LPVOID FAR PASCAL LockRes(HGLOBAL h);
void FAR PASCAL UnlockRes(HGLOBAL h);
void FAR PASCAL FreeRes(HGLOBAL h);
BITMAPINFO *FAR PASCAL MakeBitmapInfo(IMAGE *pimg, int cx, int cy);
void FAR PASCAL CreateImageBitmap(HWND hwnd, IMAGE *pimg, LPVOID lpBits, int unused);
BOOL FAR PASCAL CreateImage(HWND hwnd, IMAGE *pimg, LPRECT lprc, int nDepth, int unused, LPVOID lpBits, int n);
void FAR PASCAL FreeImage(IMAGE *pimg);
void FAR PASCAL FillImage(IMAGE *pimg, RECT *prc, HBRUSH hbr);
int FAR PASCAL GetImageWidthBytes(IMAGE *pimg);
void FAR PASCAL SetImageBits(IMAGE *pimg, LPVOID lpBits, DWORD cb);
void FAR PASCAL MaskBlitImage(IMAGE *pimgSrc, IMAGE *pimgMask, IMAGE *pimgDst, int a, int b, LPRECT lprc, int c, int d);
void FAR PASCAL CopyBlitImage(IMAGE *pimgSrc, IMAGE *pimgDst, int a, int b, LPRECT lprc, int nMode, int c);
BOOL FAR PASCAL BlitImage(IMAGE *pimgDst, int x, int y, int cx, int cy, IMAGE *pimgSrc, int xSrc, int ySrc, DWORD dwRop);
BOOL FAR PASCAL BlitImageToDC(HDC hdc, int x, int y, int cx, int cy, IMAGE *pimg, int xSrc, int ySrc, DWORD dwRop);
int FAR PASCAL OpenDataFile(char *pszName, int unused, HFILE *phf);
int FAR PASCAL SeekFile(HFILE hf, int nOrigin, LONG lOffset);
BOOL FAR PASCAL ReadFileHuge(HFILE hf, DWORD *pcb, BYTE huge *pb);
int FAR PASCAL CloseFile(HFILE hf);
int FAR PASCAL FindLibrarySlot(HINSTANCE hLib);
HINSTANCE FAR PASCAL LoadDataLibrary(char *pszName);
void FAR PASCAL FreeDataLibrary(HINSTANCE hLib);
HGLOBAL FAR PASCAL LoadRes(HINSTANCE hInst, LPCSTR lpType, int id);
HGLOBAL FAR PASCAL FindRes(LPCSTR lpType, int id);
void FAR PASCAL CopyResToGlobal(HGLOBAL FAR *ph);
char *FAR PASCAL LoadStr(int id, char *buf, int cb);
HGDIOBJ FAR PASCAL CreateGameFont(HDC hdc, int nFace, int nHeight, UINT fuStyle);
void FAR PASCAL FillRectWhite(HDC hdc, LPRECT lprc);
void FAR PASCAL FillRectBlack(HDC hdc, LPRECT lprc);
void FAR PASCAL FrameRoundRect(HDC hdc, LPRECT lprc, int cxEllipse, int cyEllipse);
void FAR PASCAL InvertRoundRect(HDC hdc, LPRECT lprc, int cxEllipse, int cyEllipse);
void FAR PASCAL DrawTextInRect(HDC hdc, LPSTR lpsz, int cch, LPRECT lprc, int fCenter);
void FAR PASCAL TextOutPascal(HDC hdc, BYTE FAR *lpps);
DWORD FAR PASCAL GetTextExtentPascal(HDC hdc, BYTE FAR *lpps);
void FAR PASCAL TermControls(void);
LRESULT CALLBACK _export SwampGasDlgClsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK _export UserWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND FAR PASCAL SubclassAnimItem(HWND hDlg, int id, WNDPROC lpfn, int nRate);
void FAR PASCAL UnsubclassAnimItem(HWND hDlg, int id);
HWND FAR PASCAL SubclassUserItem(HWND hDlg, int id, WNDPROC lpfn);
void FAR PASCAL UnsubclassUserItem(HWND hDlg, int id);
HWND FAR PASCAL CreateUserWindow(HWND hwndParent, int id, LPCSTR lpszClass, LPCSTR lpszText, DWORD dwStyle, LPRECT lprc, WNDPROC lpfn);
HWND FAR PASCAL CreateAnimWindow(HWND hwndParent, LPRECT lprc, WNDPROC lpfn, BOOL fFlag, int nRate);
void FAR PASCAL DestroyAnimWindow(HWND hwnd);
BOOL FAR PASCAL TrackUserButton(HWND hwnd, LPARAM lParam);
int FAR PASCAL GetWindowNumber(HWND hwnd);
LRESULT FAR PASCAL PicItemProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT FAR PASCAL ParamTextProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void FAR PASCAL ParamText(LPSTR lpps0, LPSTR lpps1, LPSTR lpps2, LPSTR lpps3);
LPSTR FAR PASCAL FindChar(LPSTR lpsz, int ch);
LPSTR FAR PASCAL ExpandParamText(LPSTR lpsz, int cbMax);
int FAR PASCAL CountIndStrings(int id);
void FAR PASCAL GetIndStr(LPSTR lpps, int id, int index);
void FAR PASCAL GetIndString(LPSTR lpps, int id, int index);
LPSTR FAR PASCAL PtoCstr(LPSTR lps);
LPSTR FAR PASCAL CtoPstr(LPSTR lps);
void FAR PASCAL NumToString(LONG n, LPSTR lpps);
void FAR PASCAL StringToNum(LPSTR lpps, LONG FAR *lpn);
HBITMAP FAR PASCAL LoadPicture(int id, BOOL fCache);
void FAR PASCAL GetPictureRect(HBITMAP hbm, LPRECT lprc);
BOOL FAR PASCAL LoadPictImage(int id, IMAGE *pimg);
void FAR PASCAL DrawPicture(HDC hdc, HBITMAP hbm, LPRECT lprc);
void FAR PASCAL FreePicture(HBITMAP hbm, BOOL fCache);
void FAR PASCAL FreeBitmap(HBITMAP hbm);
HBITMAP FAR PASCAL LoadPPic(int id, int n, BOOL fCache);
void FAR PASCAL FreePPic(HBITMAP hbm, BOOL fCache);
void FAR PASCAL CacheAdd(int nType, int id, HANDLE h);
HANDLE FAR PASCAL CacheFind(int nType, int id);
BOOL FAR PASCAL CacheRelease(int nType, HANDLE h, int n);
BOOL FAR PASCAL CacheIsCached(int nType, HANDLE h);
void FAR PASCAL CacheFlush(void);
HBITMAP FAR PASCAL LoadLibBitmap(int id);
HBITMAP FAR PASCAL LoadPPct(int id);
DWORD FAR PASCAL GetCaretTime(void);
void FAR PASCAL Delay(DWORD dwTicks, LPDWORD lpdwFinal);
LONG FAR PASCAL Munger(HGLOBAL h, LONG lOffset, LPSTR lpFind, LONG cbFind, LPSTR lpRepl, LONG cbRepl);

/* draw.c (segment 11) */
void FAR PASCAL DrawScene(SCENE *ps, SPRITE *s, LPRECT lprc, BOOL fInclude);
BOOL FAR PASCAL DrawRectsOverlap(RECT *a, RECT *b);
void FAR PASCAL DrawSpriteProc(SPRITE *s, IMAGE *pimg, BOOL fCur);
void FAR PASCAL DrawUnionRects(RECT *a, RECT *b, RECT *dst);

/* next.c uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern IMAGE g_imgNext[2];
extern SCENE g_sceneNextA;
extern SCENE g_sceneNextB;
extern long g_lNextDeadline;

/* view.c (segment 8) */
extern char g_cmdView1[], g_cmdView2[], g_cmdView3A[], g_cmdView3B[], g_cmdView3C[];
extern char g_cmdView5A[], g_cmdView5B[], g_cmdView6A[], g_cmdView6B[];
extern char g_cmdLineA[], g_cmdLineB[];
/* view.c uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern int g_idLineA;
extern int g_idLineB;
extern IMAGE g_imgView1[18];
extern SCENE g_sceneView1A;
extern SCENE g_sceneView1B;
extern SCENE g_sceneView1C;
extern RECT g_rcView1A;
extern RECT g_rcView1B;
extern RECT g_rcView1C;
extern int g_fView1;
extern IMAGE g_imgView2[14];
extern SCENE g_sceneView2A;
extern SCENE g_sceneView2B;
extern SCENE g_sceneView2C;
extern RECT g_rcView2A;
extern RECT g_rcView2B;
extern RECT g_rcView2C;
extern long g_fView2;
extern IMAGE g_imgView3[28];
extern SCENE g_sceneView3A;
extern SCENE g_sceneView3B;
extern SCENE g_sceneView3C;
extern RECT g_rcView3A;
extern RECT g_rcView3B;
extern RECT g_rcView3C;
extern long g_fView3;
extern IMAGE g_imgView4[10];
extern SCENE g_sceneView4A;
extern SCENE g_sceneView4B;
extern SCENE g_sceneView4C;
extern RECT g_rcView4A;
extern RECT g_rcView4B;
extern RECT g_rcView4C;
extern long g_fView4;
extern IMAGE g_imgView5[34];
extern SCENE g_sceneView5A;
extern SCENE g_sceneView5B;
extern SCENE g_sceneView5C;
extern RECT g_rcView5A;
extern RECT g_rcView5B;
extern RECT g_rcView5C;
extern long g_fView5;
extern IMAGE g_imgView6[34];
extern SCENE g_sceneView6A;
extern SCENE g_sceneView6B;
extern RECT g_rcView6A;
extern RECT g_rcView6B;
extern long g_fView6;
extern IMAGE g_imgLine[2];
extern SCENE g_sceneLineA;
extern SCENE g_sceneLineB;
extern RECT g_rcLineA;
extern RECT g_rcLineB;
extern int g_anR2Left[16], g_anR2Max[16], g_anR2Bonus[16], g_anR2Lanes[16];
extern int g_anR2Pts[16], g_anR2Vy[16], g_anR2Dx[16], g_anR2Falls[16];
extern int g_anR2ComicVy[16], g_anR2Speed[16];
extern char g_r2CmdL[42], g_r2CmdExitL[14], g_r2CmdR[42], g_r2CmdExitR[14];
/* round2.c (Nerdoids) uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern IMAGE g_r2Img[29];
extern SCENE g_r2Scene;
extern SPRITE *g_pR2Player;
extern int g_nR2Cursor;
extern int g_nR2Comic;
extern int g_nR2Falls;
extern int g_nR2Active;
extern int g_nR2Gone;
extern int g_nR2Won;
extern int g_nR2DxMax;
extern int g_nR2FallMax;
extern int g_nR2ComicVy;
extern int g_nR2Max;
extern int g_nR2Left;
extern int g_nR2Vy;
extern int g_nR2Lanes;
extern int g_nR2Pts;
extern int g_nR2Speed;
extern unsigned long g_dwR2Next;
extern int g_anR3Max[10], g_anR3Left[10], g_anR3BoltVy[10], g_anR3Arm[10];
extern int g_anR3Wave[10], g_anR3Mod[10], g_anR3DogSpeed[10], g_anR3CloudMax[10];
extern int g_anR3CloudDx[10], g_anR3CloudVy[10], g_anR3CloudPeriod[10];
extern char g_r3CmdL[108], g_r3CmdL2[120], g_r3CmdR[108], g_r3CmdR2[120];
extern char g_r3CmdHit[36], g_r3CmdA[58], g_r3CmdB[58];
/* round3.c (Quasar Klutzes) uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern IMAGE g_r3Img[85];
extern IMAGE *g_pR3Base;
extern SCENE g_r3Scene;
extern SPRITE *g_pR3Player;
extern int g_nR3Cursor;
extern int g_nR3Bolts;
extern int g_nR3Active;
extern int g_nR3Left;
extern int g_nR3Max;
extern int g_nR3Cool;
extern int g_nR3BoltVy;
extern int g_nR3RobotDogs;
extern SPRITE *g_pR3RobotDog;
extern unsigned long g_dwR3Deadline;
extern int g_nR3Arm;
extern int g_nR3Wave;
extern int g_nR3Mod;
extern int g_nR3DogSpeed;
extern int g_nR3Clouds;
extern int g_nR3CloudMax;
extern int g_nR3CloudDx;
extern int g_nR3CloudVy;
extern int g_nR3CloudPeriod;
extern int g_nR3CloudDelay;
extern int g_nR3Banana;
extern unsigned long g_dwR3Next;
extern int *g_apR4Ball[8], *g_apR4Fungus[5];
extern int g_anR4Script[10], g_anR4Div[10], g_anR4Wave[10], g_anR4Burst[10];
extern int g_anR4Arm[10], g_anR4Vel[32][2], g_anR4Mark[16];
extern char g_r4Cmd[58];
/* round4.c (Fungus Patrol) uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern IMAGE g_r4Img[77];
extern SCENE g_r4Scene;
extern int g_nR4SavedX;
extern int g_nR4SavedY;
extern int g_nR4CursorX;
extern int g_nR4CursorY;
extern int g_nR4Pad;
extern SPRITE *g_pR4Bungee;
extern SPRITE *g_pR4Ball;
extern int g_nR4Frame;
extern int g_nR4Frame0;
extern int g_nR4Count;
extern int g_nR4Left;
extern int g_nR4Active;
extern unsigned long g_dwR4Deadline;
extern int g_nR4Script;
extern int g_nR4Div;
extern int g_nR4Wave;
extern int g_nR4Burst;
extern int g_nR4Arm;
extern HLOCAL g_hR4Mem;
extern R4SLOT g_aR4Slot[16];
extern unsigned long g_dwR4Next;
extern int g_anR1A[10], g_anR1B[10], g_anR1C[10];
extern int g_anR1D[10], g_anR1E[10], g_anR1F[10];
extern char g_r1Cmd[122], g_r1Cmd2[16];
/* round1.c (Stink Snakes) uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern IMAGE g_r1Img[26];
extern SCENE g_r1Scene;
extern SPRITE *g_pR1Player;
extern int g_nR1Cursor;
extern int g_nR1Active;
extern int g_nR1Gone;
extern int g_nR1Mode;
extern int g_nR1Vx;
extern int g_nR1Vy;
extern int g_nR1Span;
extern int g_nR1Max;
extern int g_nR1Speed;
extern int g_nR1Pies;

SPRITE *FAR PASCAL AddSpriteImage(SCENE *ps, IMAGE *pimg, IMAGE *pmask, int x, int y, int nMode);
void FAR PASCAL SetSpriteMove(SPRITE *s, int id, long lPos);
void FAR PASCAL SetSpriteFrame(SPRITE *s, IMAGE *pimg, IMAGE *pmask, int nFrame, int nFrames);
void FAR PASCAL OpenView1(HWND hwnd, int idA, int idB, int idC);
void FAR PASCAL CloseView1(HWND hwnd, int id);
void FAR PASCAL OpenView2(HWND hwnd, int idA, int idB, int idC);
void FAR PASCAL CloseView2(HWND hwnd, int id);
void FAR PASCAL OpenView3(HWND hwnd, int idA, int idB, int idC);
void FAR PASCAL CloseView3(HWND hwnd, int id);
void FAR PASCAL OpenView4(HWND hwnd, int idA, int idB, int idC);
void FAR PASCAL CloseView4(HWND hwnd, int id);
void FAR PASCAL OpenView5(HWND hwnd, int idA, int idB, int idC);
void FAR PASCAL CloseView5(HWND hwnd, int id);
void FAR PASCAL OpenView6(HWND hwnd, int id, int idExtra);
void FAR PASCAL CloseView6(HWND hwnd, int id);
void FAR PASCAL OpenLineView(HWND hwnd, int idA, int idB);
void FAR PASCAL CloseLineView(HWND hwnd, int id);
void FAR PASCAL OpenViews(HWND hwnd);
void FAR PASCAL TickViews(void);
void FAR PASCAL CloseViews(HWND hwnd);

/* ppic.c (segment 25) */
BOOL FAR PASCAL LoadPPicImages(int id, int iFirst, int n, IMAGE *pimg, int nDepth);
BOOL FAR PASCAL GetPPicImage(int id, int index, IMAGE *pimg, int nDepth);
BOOL FAR PASCAL GetPPctImage(int id, IMAGE *pimg);
BOOL FAR PASCAL LoadPicResImage(LPCSTR lpType, int id, int index, IMAGE *pimg, int nDepth);
LPSTR FAR PASCAL SeekPicRes(HGLOBAL h, int index, int nDepth);
void FAR PASCAL FreePicRes(HGLOBAL h);
BOOL FAR PASCAL DecodePicImage(LPSTR *plp, IMAGE *pimg, int nDepth);
BOOL FAR PASCAL DecodePixImage(LPSTR *plp, IMAGE *pimg, int nDepth);
void FAR PASCAL UnpackColumn1(LPSTR *plp, LPSTR lpDst, int cy, int cbStride);
void FAR PASCAL UnpackColumn4(LPSTR *plp, BYTE huge *lpDst, int cy, int cbStride);
void FAR PASCAL UnpackColumn8(LPSTR *plp, WORD huge *lpDst, int cy, int cStride);

/* move.c (segment 12) */
void FAR PASCAL MoveSprite(SPRITE *s, BOOL fFirst);
void *FAR PASCAL AllocSpriteMove(void);

/* slist.c (segment 7) */
HGLOBAL FAR PASCAL AllocScoreList(int cbName, int nMax, int fAllTime);
void FAR PASCAL SetScoreDay(HGLOBAL h, int nDay, int fAllTime);
void FAR PASCAL ResetScoreList(HGLOBAL h);
int FAR PASCAL GetYDay(void);
int FAR PASCAL ScoreQualifies(HGLOBAL h, long lScore);
int FAR PASCAL InsertScore(HGLOBAL h, long lScore, LPSTR lpsz);
void FAR PASCAL GetScoreEntry(HGLOBAL h, int i, long *plScore, LPSTR lpsz);
void FAR PASCAL GetScoreInfo(HGLOBAL h, int *pcbName, int *pnCount, int *pnMax);

/* pstr.c (segment 9) */
void FAR PASCAL CopyPStr(LPSTR lpSrc, LPSTR lpDst);
void FAR PASCAL InsertPStr(LPSTR lpIns, LPSTR lpStr, BYTE nPos);

/* play.c (segment 13) */
int FAR PASCAL RunPlay(void);
void FAR PASCAL RunNameDialog(void);

/* layout.c (segment 14) */
void FAR PASCAL LayoutMain(HWND hwnd);
void FAR PASCAL PaintMain(HWND hwnd);

/* inifile.c (segment 15) */
int FAR PASCAL LoadConfig(void);
int FAR PASCAL GetPatrolBackground(void);
void FAR PASCAL SaveConfig(void);

/* util.c (segment 16) */
HWND FAR PASCAL GetDlgItemHwnd(HWND hwnd, int id);
void FAR PASCAL GetDlgItemPstr(HWND hwnd, LPSTR lpps);
void FAR PASCAL SetDlgItemPstr(HWND hwnd, LPSTR lpps);
BOOL FAR PASCAL HideDlgItem(HWND hwnd, int id);
BOOL FAR PASCAL ShowDlgItem(HWND hwnd, int id);
void FAR PASCAL CenterPopup(HWND hwnd);
void FAR PASCAL CenterChild(HWND hwnd);
void FAR PASCAL DrawItemPic(HDC hdc, int id, LPRECT lprc);
void FAR PASCAL ConfirmChildCmd(int *pn);

/* result.c (segment 21) */
int FAR PASCAL RunResultDialog(void);

/* clear.c (segment 17) */
int FAR PASCAL RunResetScores(HWND hwnd);
int FAR PASCAL RunEraseScores(HWND hwnd);

/* board.c (segment 19) */
void FAR PASCAL PaintNextScores(HDC hdc);

/* next.c (segment 18) */
int FAR PASCAL RunNextDialog(HWND hwnd);

/* start.c (segment 20) */
int FAR PASCAL OpenSession(void);
void FAR PASCAL CloseSession(void);

/* score.c (segment 22) */
void FAR PASCAL LoadRoundScores(void);
void FAR PASCAL RecordScore(void);
void FAR PASCAL SaveRoundScores(void);

/* round.c (segment 26) */
void FAR PASCAL SetRound(int n);
void FAR PASCAL OpenRound(HWND hwnd);
void FAR PASCAL ClickRound(void);
void FAR PASCAL PaintRound(void);
int FAR PASCAL TickRound(void);
void FAR PASCAL CloseRound(void);
void FAR PASCAL DecImageBottoms(IMAGE *pimg, int iFirst, int iLast);

/* round3.c (segment 27): Quasar Klutzes */
void FAR PASCAL OpenRound3(HWND hwnd);
void FAR PASCAL ClickRound3(void);
int FAR PASCAL TickRound3(void);
void FAR PASCAL PaintRound3(void);
void FAR PASCAL CloseRound3(void);

/* round2.c (segment 28): Nerdoids */
void FAR PASCAL OpenRound2(HWND hwnd);
void FAR PASCAL ClickRound2(void);
int FAR PASCAL TickRound2(void);
void FAR PASCAL PaintRound2(void);
void FAR PASCAL CloseRound2(void);

/* status.c (segment 29) */
/* status.c uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern long g_tTimeLast;
extern long g_lTimeShown;

void FAR PASCAL PadNumString(long n, int nWidth, LPSTR lpps);
void FAR PASCAL StatusTextOut(HDC hdc, LPSTR lpps);
void FAR PASCAL DrawPlayAmmo(HDC hdc);
void FAR PASCAL AddPlayAmmo(long n);
void FAR PASCAL DrawR5Parts(HDC hdc);
void FAR PASCAL DrawR5Droids(HDC hdc);
void FAR PASCAL AddR5Count(int n, int i);
void FAR PASCAL DrawPlayWave(HDC hdc);
void FAR PASCAL NextPlayWave(void);
void FAR PASCAL DrawTimeLeft(HDC hdc);
void FAR PASCAL TickTimeLeft(void);
void FAR PASCAL PauseTimeLeft(void);
void FAR PASCAL ResumeTimeLeft(void);
void FAR PASCAL SetTimeLeft(long t);
void FAR PASCAL DrawPlayScore(HDC hdc);
void FAR PASCAL AddPlayScore(long n);
void FAR PASCAL DrawStatusBar(void);
void FAR PASCAL FormatTime(long t, LPSTR lpps);

/* round1.c (segment 30): Stink Snakes */
void FAR PASCAL OpenRound1(HWND hwnd);
void FAR PASCAL ClickRound1(void);
int FAR PASCAL TickRound1(void);
void FAR PASCAL PaintRound1(void);
void FAR PASCAL CloseRound1(void);

/* round5.c (segment 31): Andromedroids */
extern int g_anR5ColX[9], g_anR5Off[3], g_anR5Droids[10], g_anR5Odds[10];
extern int g_anR5Vy[10], g_anR5BugVy[10], g_anR5Delay[10];
extern char g_r5CmdIdle[8];
extern int g_anR5Mark[8];
extern char g_r5CmdClear2[194], g_r5CmdClear1[66], g_r5CmdClear0[38], g_r5CmdBinClear[26];
extern char g_r5CmdScoreA[58], g_r5CmdScoreB[36], g_r5CmdScoreC[50];
extern char g_r5CmdBinBad0[42], g_r5CmdBinBad1[50], g_r5CmdBinBad2[58];
extern char g_r5CmdBad2[16], g_r5CmdBad1[16], g_r5CmdBad0[16];
extern char g_r5CmdBug2[46], g_r5CmdBug3[66], g_r5CmdSink0[10], g_r5CmdSink1[10];
extern char g_r5CmdRollL[26], g_r5CmdRollR[26], g_r5CmdLoop[22];
/* round5.c (Andromedroids) uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern IMAGE g_r5Img[97];
extern SCENE g_r5Scene;
extern SPRITE *g_apR5AsmBin[7];
extern int g_anR5Floor[9];
extern SPRITE *g_apR5Stack[9][3];
extern int g_anR5Depth[9];
extern SPRITE *g_pR5Before;
extern SPRITE *g_pR5Part;
extern int g_nR5Col;
extern int g_nR5Active;
extern unsigned long g_dwR5Next;
extern int g_nR5PlayerX;
extern int g_fR5Drop;
extern int g_nR5BugBins;
extern int g_nR5NextCol;
extern SPRITE *g_pR5Player;
extern int g_nR5SavedX;
extern int g_nR5CursorX;
extern int g_nR5PlayerCol;
extern int g_nR5Vy;
extern int g_nR5BugVy;
extern int g_nR5Odds;
extern int g_nR5Delay;
extern int g_anR5Count[4];
extern int g_anR5Recycled[3];
extern unsigned long g_dwR5Tick;

void FAR PASCAL OpenRound5(HWND hwnd);
void FAR PASCAL ClickRound5(void);
int FAR PASCAL TickRound5(void);
void FAR PASCAL PaintRound5(void);
void FAR PASCAL CloseRound5(void);

/* round4.c (segment 32): Fungus Patrol */
void FAR PASCAL OpenRound4(HWND hwnd);
void FAR PASCAL ClickRound4(void);
int FAR PASCAL TickRound4(void);
void FAR PASCAL PaintRound4(void);
void FAR PASCAL CloseRound4(void);

/* round6.c (segment 33): Quasar Rain */
void FAR PASCAL OpenRound6(HWND hwnd);
void FAR PASCAL ClickRound6(void);
int FAR PASCAL TickRound6(void);
void FAR PASCAL PaintRound6(void);
void FAR PASCAL CloseRound6(void);

/* sound.c (segment 23) */
typedef struct {
	HWAVEOUT hwo;
	HGLOBAL hCtx;                   /* this block */
	HGLOBAL hHdr;                   /* the WAVEHDR */
} WAVECTX;

typedef struct {
	int nState;                     /* 0 idle, 1 playing, 2 done */
	WAVECTX FAR *lpCtx;
	LPWAVEHDR lpHdr;
} WAVE;

extern DWORD g_dwVolume;
extern WAVE *g_pWave;
extern HGLOBAL g_hSnd;
extern int g_anVolume[10];
/* sound.c uninitialized, highest address first. Borland emits BSS in reverse order of first appearance. */
extern int g_nSndNext;
extern int g_anSndId[16];
extern HGLOBAL g_ahSnd[16];
extern WAVE g_wave;

void FAR PASCAL InitSound(void);
BOOL FAR PASCAL FinishWave(void);
void FAR PASCAL TickSound(void);
void FAR PASCAL StopSfx(void);
void FAR PASCAL CloseSfx(void);
HGLOBAL FAR PASCAL LoadSound(int id);
void FAR PASCAL PreloadSounds(int id, int n);
void FAR PASCAL PlaySfx(int id, BOOL fForce);
void FAR PASCAL PlaySoundEx(int id, BOOL fForce, int n);
int FAR PASCAL SelectVolume(WPARAM id);
int FAR PASCAL OpenWave(WAVE **pp);
void FAR PASCAL SetWaveState(WAVE *p, int n);
BOOL FAR PASCAL IsWaveIdle(WAVE *p);
BOOL FAR PASCAL IsWaveDone(WAVE *p);
BOOL FAR PASCAL IsWaveBusy(WAVE *p);
void FAR PASCAL StopWave(WAVE *p);
void FAR PASCAL CloseWave(WAVE *p);
void FAR PASCAL FreeWave(WAVE *p);
int FAR PASCAL StartWave(WAVE *p, int id, int n);
int FAR PASCAL OpenWaveOut(WAVE *p, HGLOBAL h, int n);
BOOL FAR PASCAL ParseWave(HGLOBAL h, LPWAVEFORMAT *plpFmt, LPSTR *plpData, DWORD *pcb);

/* speaker.c (segment 24) */
void FAR PASCAL EndSpeaker(void);
void FAR PASCAL PlaySpeaker(int id, BOOL fForce, int n);
void FAR PASCAL TickSpeaker(void);

/* intro.c (segment 34) */
int FAR PASCAL RunStartDialog(void);

/* between.c (segment 35) */
void FAR PASCAL RunStepDialog(void);
void FAR PASCAL RunRoundDone(void);

#endif
