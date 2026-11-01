#include "aarcade.h"

/* Help / instructions view: animated pictures beside the round text. */

/* Move scripts (0024:007C..02C3), one per help-view sprite. Each ends in the
   0x0A opcode. Their offsets are what OpenViewN passes to SetSpriteMove and
   AddHiddenSprite as `int id`, so the pushes carry a fixup (68 imm16).
   mmstub.asm's table is linked next, at 0024:02C4. */
// GLOBAL: AARCADE 0024:007C
char g_cmdView1[] = {
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x09, 0x04,
	0x01, 0x00, 0x06, 0x02, 0x04, 0x08, 0x01, 0x00, 0xFD, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x04, 0x08,
	0x01, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x04, 0x08, 0x02, 0x00, 0xFC, 0xFF, 0x00, 0x00, 0x04, 0x08,
	0x08, 0x00, 0x10, 0x00, 0x00, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04,
	0x02, 0x00, 0x07, 0x02, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:00C2
char g_cmdView2[] = {
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00, 0x06, 0x02, 0x09, 0x04, 0x01, 0x00,
	0x06, 0x02, 0x04, 0x08, 0x02, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x04, 0x08, 0x04, 0x00, 0xF6, 0xFF,
	0x00, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02,
	0x0A, 0x00
};

// GLOBAL: AARCADE 0024:00F4
char g_cmdView3A[] = {
	0x00, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04,
	0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02,
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x05, 0x04, 0x00, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:0124
char g_cmdView3B[] = {
	0x00, 0x02, 0x02, 0x02, 0x09, 0x04, 0x03, 0x00, 0x06, 0x02, 0x09, 0x04, 0x03, 0x00, 0x06, 0x02,
	0x09, 0x04, 0x03, 0x00, 0x01, 0x02, 0x09, 0x04, 0x07, 0x00, 0x07, 0x02, 0x07, 0x02, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:0144
char g_cmdView3C[] = {
	0x00, 0x02, 0x06, 0x02, 0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0xF6, 0xFF, 0x04, 0x08, 0x03, 0x00,
	0x00, 0x00, 0xFD, 0xFF, 0x09, 0x04, 0x02, 0x00, 0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0x0D, 0x00,
	0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:016C
char g_cmdView5A[] = {
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00,
	0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
	0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x09, 0x04,
	0x02, 0x00, 0x07, 0x02, 0x07, 0x02, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x09, 0x04, 0x02, 0x00,
	0x09, 0x04, 0x02, 0x00, 0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x09, 0x04,
	0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02,
	0x09, 0x04, 0x02, 0x00, 0x06, 0x02, 0x06, 0x02, 0x06, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02,
	0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00, 0x07, 0x02, 0x09, 0x04, 0x02, 0x00,
	0x0A, 0x00
};

// GLOBAL: AARCADE 0024:01EE
char g_cmdView5B[] = {
	0x09, 0x04, 0x04, 0x00, 0x04, 0x08, 0x00, 0x00, 0xFC, 0xFF, 0x00, 0x00, 0x06, 0x02, 0x09, 0x04,
	0x04, 0x00, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x06, 0x02, 0x09, 0x04, 0x04, 0x00, 0x06, 0x02,
	0x09, 0x04, 0x04, 0x00, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02,
	0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x09, 0x04, 0x04, 0x00, 0x07, 0x02, 0x04, 0x08, 0x00, 0x00,
	0x04, 0x00, 0x00, 0x00, 0x09, 0x04, 0x04, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:0238
char g_cmdView6A[] = {
	0x04, 0x08, 0x02, 0x00, 0x00, 0x00, 0x10, 0x00, 0x04, 0x08, 0x02, 0x00, 0x00, 0x00, 0xF0, 0xFF,
	0x04, 0x08, 0x02, 0x00, 0x00, 0x00, 0xFC, 0xFF, 0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF,
	0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x08, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x0A, 0x00
};

// GLOBAL: AARCADE 0024:026A
char g_cmdView6B[] = {
	0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0x18, 0x00, 0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0xE8, 0xFF,
	0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0xFA, 0xFF, 0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF,
	0x04, 0x08, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x08, 0x03, 0x00, 0x00, 0x00, 0x06, 0x00,
	0x0A, 0x00
};

// GLOBAL: AARCADE 0024:029C
char g_cmdLineA[] = {
	0x03, 0x08, 0x00, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x02, 0x02, 0x04, 0x08, 0x80, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x0A, 0x00
};

// GLOBAL: AARCADE 0024:02B0
char g_cmdLineB[] = {
	0x03, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x02, 0x04, 0x08, 0x80, 0x00, 0x00, 0xFF,
	0x00, 0x00, 0x0A, 0x00
};

/* Per-sprite block allocated by AllocSpriteMove (0x22 bytes). */
typedef struct tagSPRITEMOVE {
	int id;
	int x0;
	int x1;
	int y0;
	int y1;
	int unused[8];
	long lPos;
} SPRITEMOVE;

/* Uninitialized data. aarcade.h declares these highest address first;
   Borland emits BSS in reverse order of that first appearance. */


// GLOBAL: AARCADE 0024:2DE4
int g_idLineA;


// GLOBAL: AARCADE 0024:2DE2
int g_idLineB;


// GLOBAL: AARCADE 0024:2D0A
IMAGE g_imgView1[18];


// GLOBAL: AARCADE 0024:2CE0
SCENE g_sceneView1A;


// GLOBAL: AARCADE 0024:2CB6
SCENE g_sceneView1B;


// GLOBAL: AARCADE 0024:2C8C
SCENE g_sceneView1C;


// GLOBAL: AARCADE 0024:2C84
RECT g_rcView1A;


// GLOBAL: AARCADE 0024:2C7C
RECT g_rcView1B;


// GLOBAL: AARCADE 0024:2C74
RECT g_rcView1C;


// GLOBAL: AARCADE 0024:2C72
int g_fView1;


// GLOBAL: AARCADE 0024:2BCA
IMAGE g_imgView2[14];


// GLOBAL: AARCADE 0024:2BA0
SCENE g_sceneView2A;


// GLOBAL: AARCADE 0024:2B76
SCENE g_sceneView2B;


// GLOBAL: AARCADE 0024:2B4C
SCENE g_sceneView2C;


// GLOBAL: AARCADE 0024:2B44
RECT g_rcView2A;


// GLOBAL: AARCADE 0024:2B3C
RECT g_rcView2B;


// GLOBAL: AARCADE 0024:2B34
RECT g_rcView2C;


// GLOBAL: AARCADE 0024:2B30
long g_fView2;


// GLOBAL: AARCADE 0024:29E0
IMAGE g_imgView3[28];


// GLOBAL: AARCADE 0024:29B6
SCENE g_sceneView3A;


// GLOBAL: AARCADE 0024:298C
SCENE g_sceneView3B;


// GLOBAL: AARCADE 0024:2962
SCENE g_sceneView3C;


// GLOBAL: AARCADE 0024:295A
RECT g_rcView3A;


// GLOBAL: AARCADE 0024:2952
RECT g_rcView3B;


// GLOBAL: AARCADE 0024:294A
RECT g_rcView3C;


// GLOBAL: AARCADE 0024:2946
long g_fView3;


// GLOBAL: AARCADE 0024:28CE
IMAGE g_imgView4[10];


// GLOBAL: AARCADE 0024:28A4
SCENE g_sceneView4A;


// GLOBAL: AARCADE 0024:287A
SCENE g_sceneView4B;


// GLOBAL: AARCADE 0024:2850
SCENE g_sceneView4C;


// GLOBAL: AARCADE 0024:2848
RECT g_rcView4A;


// GLOBAL: AARCADE 0024:2840
RECT g_rcView4B;


// GLOBAL: AARCADE 0024:2838
RECT g_rcView4C;


// GLOBAL: AARCADE 0024:2834
long g_fView4;


// GLOBAL: AARCADE 0024:269C
IMAGE g_imgView5[34];


// GLOBAL: AARCADE 0024:2672
SCENE g_sceneView5A;


// GLOBAL: AARCADE 0024:2648
SCENE g_sceneView5B;


// GLOBAL: AARCADE 0024:261E
SCENE g_sceneView5C;


// GLOBAL: AARCADE 0024:2616
RECT g_rcView5A;


// GLOBAL: AARCADE 0024:260E
RECT g_rcView5B;


// GLOBAL: AARCADE 0024:2606
RECT g_rcView5C;


// GLOBAL: AARCADE 0024:2602
long g_fView5;


// GLOBAL: AARCADE 0024:246A
IMAGE g_imgView6[34];


// GLOBAL: AARCADE 0024:2440
SCENE g_sceneView6A;


// GLOBAL: AARCADE 0024:2416
SCENE g_sceneView6B;


// GLOBAL: AARCADE 0024:240E
RECT g_rcView6A;


// GLOBAL: AARCADE 0024:2406
RECT g_rcView6B;


// GLOBAL: AARCADE 0024:2402
long g_fView6;


// GLOBAL: AARCADE 0024:23EA
IMAGE g_imgLine[2];


// GLOBAL: AARCADE 0024:23C0
SCENE g_sceneLineA;


// GLOBAL: AARCADE 0024:2396
SCENE g_sceneLineB;


// GLOBAL: AARCADE 0024:238E
RECT g_rcLineA;


/* view.c (segment 8). One animated picture strip per dialog control. */

// GLOBAL: AARCADE 0024:2386
RECT g_rcLineB;

// FUNCTION: AARCADE 0008:0000
SPRITE *FAR PASCAL AddSpriteImage(SCENE *ps, IMAGE *pimg, IMAGE *pmask, int x, int y, int nMode)
{
	SPRITE *s;

	/* test AX, not the copy in SI */
	if ((s = AddSprite(ps)) != 0) {
		s->cur.img = *pimg;
		if (pmask)
			s->cur.mask = *pmask;
		s->cur.x = x;
		s->cur.y = y;
		UpdateSpriteRect(s);
		s->pImages = pimg;
		s->pMasks = pmask;
		s->nDrawMode = nMode;
	}
	return s;
}

// FUNCTION: AARCADE 0008:006B
void FAR PASCAL SetSpriteMove(SPRITE *s, int id, long lPos)
{
	SPRITEMOVE *p;

	/* test AX, not the copy in SI; each zero is its own immediate store */
	if ((p = AllocSpriteMove()) != 0) {
		s->dwData = p;
		s->lpfnMoveEx = MoveSprite;
		p->id = id;
		p->lPos = lPos;
		p->x1 = 0;
		p->x0 = 0;
		p->y0 = 0;
		p->y1 = 0;
	}
}

// FUNCTION: AARCADE 0008:00BB
void FAR PASCAL SetSpriteFrame(SPRITE *s, IMAGE *pimg, IMAGE *pmask, int nFrame, int nFrames)
{
	s->pImages = pimg;
	s->pMasks = pmask;
	s->nFrame = nFrame;
	s->nFrames = nFrames;
	s->cur.img = pimg[nFrame];
	UpdateSpriteRect(s);
}

// FUNCTION: AARCADE 0008:0104
void FAR PASCAL BackView1(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[0]);
}

// FUNCTION: AARCADE 0008:011C
LRESULT CALLBACK View1WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		PaintScene(&g_sceneView1A);
		if (g_fView1) {
			PaintScene(&g_sceneView1B);
			PaintScene(&g_sceneView1C);
		}
		break;
	case WM_TIMER:
		TickScene(&g_sceneView1A);
		if (g_fView1) {
			TickScene(&g_sceneView1B);
			TickScene(&g_sceneView1C);
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0008:019C
void FAR PASCAL OpenView1(HWND hwnd, int idA, int idB, int idC)
{
	SCENEBKPROC lpfn;
	HWND hwndItem;
	SPRITE *s;

	lpfn = NULL;
	if (g_fColor)
		lpfn = BackView1;
	g_fView1 = idB && idC;
	hwndItem = GetDlgItem(hwnd, idA);
	GetClientRect(hwndItem, &g_rcView1A);
	if (g_fView1) {
		LoadPPicImages(6100, 0, 18, g_imgView1, g_nDepth);
		InitScene(hwndItem, &g_sceneView1A, lpfn, &g_rcView1A, g_nDepth2, 3, 0);
	} else {
		LoadPPicImages(6100, 0, 8, g_imgView1, g_nDepth);
		InitScene(hwndItem, &g_sceneView1A, NULL, &g_rcView1A, g_nDepth2, 3, 0);
	}
	s = AddSpriteImage(&g_sceneView1A, g_imgView1, &g_imgView1[4], 16, 0, -1);
	if (s) {
		SetSpriteMove(s, (int)g_cmdView1, 0L);
		SetSpriteFrame(s, g_imgView1, &g_imgView1[4], 0, 4);
	}
	if (g_fView1) {
		hwndItem = GetDlgItem(hwnd, idB);
		GetClientRect(hwndItem, &g_rcView1B);
		SubclassUserItem(hwnd, idB, View1WndProc);
		InitScene(hwndItem, &g_sceneView1B, lpfn, &g_rcView1B, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView1B, &g_imgView1[8], &g_imgView1[12], 0, 0, -1);
		if (s) {
			SetSpriteFrame(s, &g_imgView1[8], &g_imgView1[12], 0, 4);
			s->nAnimMode = 1;
			s->nAnimRate = 3;
			s->nStep = -1;
		}
		hwndItem = GetDlgItem(hwnd, idC);
		GetClientRect(hwndItem, &g_rcView1C);
		SubclassUserItem(hwnd, idC, View1WndProc);
		InitScene(hwndItem, &g_sceneView1C, lpfn, &g_rcView1C, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView1C, &g_imgView1[16], &g_imgView1[17], 0, 0, -1);
	}
	SubclassAnimItem(hwnd, idA, View1WndProc, 3);
}

// FUNCTION: AARCADE 0008:0367
void FAR PASCAL CloseView1(HWND hwnd, int id)
{
	UnsubclassAnimItem(hwnd, id);
	FreeScene(&g_sceneView1A);
	if (g_fView1) {
		FreeScene(&g_sceneView1B);
		FreeScene(&g_sceneView1C);
		FreeImages(g_imgView1, 18, 1);
	} else
		FreeImages(g_imgView1, 8, 1);
}

// FUNCTION: AARCADE 0008:03AD
void FAR PASCAL BackView2(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[2]);
}

// FUNCTION: AARCADE 0008:03C5
LRESULT CALLBACK View2WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		PaintScene(&g_sceneView2A);
		if (g_fView2) {
			PaintScene(&g_sceneView2B);
			PaintScene(&g_sceneView2C);
		}
		break;
	case WM_TIMER:
		TickScene(&g_sceneView2A);
		if (g_fView2) {
			TickScene(&g_sceneView2B);
			TickScene(&g_sceneView2C);
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0008:0449
void FAR PASCAL OpenView2(HWND hwnd, int idA, int idB, int idC)
{
	SPRITE *s;
	SCENEBKPROC lpfn;
	HWND hwndItem;

	lpfn = NULL;
	if (g_fColor)
		lpfn = BackView2;
	g_fView2 = idB && idC;
	hwndItem = GetDlgItem(hwnd, idA);
	GetClientRect(hwndItem, &g_rcView2A);
	if (g_fView2) {
		LoadPPicImages(7100, 0, 14, g_imgView2, g_nDepth);
		InitScene(hwndItem, &g_sceneView2A, lpfn, &g_rcView2A, g_nDepth2, 3, 0);
	} else {
		LoadPPicImages(7100, 0, 8, g_imgView2, g_nDepth);
		InitScene(hwndItem, &g_sceneView2A, NULL, &g_rcView2A, g_nDepth2, 3, 0);
	}
	s = AddSpriteImage(&g_sceneView2A, g_imgView2, &g_imgView2[4], 0, 0, -1);
	if (s) {
		SetSpriteMove(s, (int)g_cmdView2, 0L);
		SetSpriteFrame(s, g_imgView2, &g_imgView2[4], 0, 4);
	}
	if (g_fView2) {
		hwndItem = GetDlgItem(hwnd, idB);
		GetClientRect(hwndItem, &g_rcView2B);
		SubclassUserItem(hwnd, idB, View2WndProc);
		InitScene(hwndItem, &g_sceneView2B, lpfn, &g_rcView2B, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView2B, &g_imgView2[8], &g_imgView2[11], 0, 1, -1);
		s = AddSpriteImage(&g_sceneView2B, &g_imgView2[9], &g_imgView2[12], 14, 0, -1);
		hwndItem = GetDlgItem(hwnd, idC);
		GetClientRect(hwndItem, &g_rcView2C);
		SubclassUserItem(hwnd, idC, View2WndProc);
		InitScene(hwndItem, &g_sceneView2C, lpfn, &g_rcView2C, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView2C, &g_imgView2[10], &g_imgView2[13], 0, 0, -1);
	}
	SubclassAnimItem(hwnd, idA, View2WndProc, 3);
}

// FUNCTION: AARCADE 0008:0613
void FAR PASCAL CloseView2(HWND hwnd, int id)
{
	UnsubclassAnimItem(hwnd, id);
	FreeScene(&g_sceneView2A);
	if (g_fView2) {
		FreeScene(&g_sceneView2B);
		FreeScene(&g_sceneView2C);
		FreeImages(g_imgView2, 14, 1);
	} else
		FreeImages(g_imgView2, 8, 1);
}

// FUNCTION: AARCADE 0008:065B
void FAR PASCAL BackView3(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[1]);
}

// FUNCTION: AARCADE 0008:0673
LRESULT CALLBACK View3WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		PaintScene(&g_sceneView3A);
		if (g_fView3) {
			PaintScene(&g_sceneView3B);
			PaintScene(&g_sceneView3C);
		}
		break;
	case WM_TIMER:
		TickScene(&g_sceneView3A);
		if (g_fView3) {
			TickScene(&g_sceneView3B);
			TickScene(&g_sceneView3C);
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0008:06F7
void FAR PASCAL OpenView3(HWND hwnd, int idA, int idB, int idC)
{
	SCENEBKPROC lpfn;
	HWND hwndItem;
	SPRITE *s;

	lpfn = NULL;
	if (g_fColor)
		lpfn = BackView3;
	g_fView3 = idB && idC;
	hwndItem = GetDlgItem(hwnd, idA);
	GetClientRect(hwndItem, &g_rcView3A);
	if (g_fView3) {
		LoadPPicImages(8100, 0, 28, g_imgView3, g_nDepth);
		InitScene(hwndItem, &g_sceneView3A, lpfn, &g_rcView3A, g_nDepth2, 3, 0);
	} else {
		LoadPPicImages(8100, 0, 14, g_imgView3, g_nDepth);
		InitScene(hwndItem, &g_sceneView3A, NULL, &g_rcView3A, g_nDepth2, 3, 0);
	}
	s = AddSpriteImage(&g_sceneView3A, g_imgView3, &g_imgView3[7], 0, 0, -1);
	if (s) {
		SetSpriteMove(s, (int)g_cmdView3A, 0L);
		SetSpriteFrame(s, g_imgView3, &g_imgView3[7], 0, 7);
	}
	if (g_fView3) {
		hwndItem = GetDlgItem(hwnd, idB);
		GetClientRect(hwndItem, &g_rcView3B);
		SubclassUserItem(hwnd, idB, View3WndProc);
		InitScene(hwndItem, &g_sceneView3B, lpfn, &g_rcView3B, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView3B, &g_imgView3[18], &g_imgView3[20], 0, 0, -1);
		if (s) {
			SetSpriteFrame(s, &g_imgView3[18], &g_imgView3[20], 0, 2);
			s->nAnimRate = 4;
		}
		s = AddSpriteImage(&g_sceneView3B, &g_imgView3[22], &g_imgView3[25], 48, 0, -1);
		if (s) {
			SetSpriteFrame(s, &g_imgView3[22], &g_imgView3[25], 0, 3);
			SetSpriteMove(s, (int)g_cmdView3B, 0L);
		}
		hwndItem = GetDlgItem(hwnd, idC);
		GetClientRect(hwndItem, &g_rcView3C);
		SubclassUserItem(hwnd, idC, View3WndProc);
		InitScene(hwndItem, &g_sceneView3C, lpfn, &g_rcView3C, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView3C, &g_imgView3[14], &g_imgView3[16], 0, 0, -1);
		if (s) {
			SetSpriteFrame(s, &g_imgView3[14], &g_imgView3[16], 0, 2);
			SetSpriteMove(s, (int)g_cmdView3C, 0L);
		}
	}
	SubclassAnimItem(hwnd, idA, View3WndProc, 3);
}

// FUNCTION: AARCADE 0008:0916
void FAR PASCAL CloseView3(HWND hwnd, int id)
{
	UnsubclassAnimItem(hwnd, id);
	FreeScene(&g_sceneView3A);
	if (g_fView3) {
		FreeScene(&g_sceneView3B);
		FreeScene(&g_sceneView3C);
		FreeImages(g_imgView3, 28, 1);
	} else
		FreeImages(g_imgView3, 14, 1);
}

// FUNCTION: AARCADE 0008:095E
void FAR PASCAL BackView4(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[3]);
}

// FUNCTION: AARCADE 0008:0976
LRESULT CALLBACK View4WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		PaintScene(&g_sceneView4A);
		if (g_fView4) {
			PaintScene(&g_sceneView4B);
			PaintScene(&g_sceneView4C);
		}
		break;
	case WM_TIMER:
		TickScene(&g_sceneView4A);
		if (g_fView4) {
			TickScene(&g_sceneView4B);
			TickScene(&g_sceneView4C);
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0008:09FA
void FAR PASCAL OpenView4(HWND hwnd, int idA, int idB, int idC)
{
	int i;
	SCENEBKPROC lpfn;
	HWND hwndItem;
	SPRITE *s;

	lpfn = NULL;
	if (g_fColor)
		lpfn = BackView4;
	g_fView4 = idB && idC;
	hwndItem = GetDlgItem(hwnd, idA);
	GetClientRect(hwndItem, &g_rcView4A);
	if (g_fView4) {
		LoadPPicImages(17100, 0, 10, g_imgView4, g_nDepth);
		InitScene(hwndItem, &g_sceneView4A, lpfn, &g_rcView4A, g_nDepth2, 3, 0);
	} else {
		LoadPPicImages(17100, 0, 6, g_imgView4, g_nDepth);
		InitScene(hwndItem, &g_sceneView4A, NULL, &g_rcView4A, g_nDepth2, 3, 0);
	}
	if (g_fView4) {
		s = AddSpriteImage(&g_sceneView4A, g_imgView4, &g_imgView4[3], 0, 0, -1);
		if (s) {
			SetSpriteFrame(s, g_imgView4, &g_imgView4[3], 0, 3);
			s->nAnimRate = 3;
		}
	} else {
		for (i = 0; i < 3; i++) {
			int x, y;

			x = i * 22;
			y = (i == 1) ? 0 : 6;
			s = AddSpriteImage(&g_sceneView4A, g_imgView4, &g_imgView4[3], x, y, -1);
			if (s) {
				SetSpriteFrame(s, g_imgView4, &g_imgView4[3], 0, 3);
				s->nAnimRate = 3;
				s->nFrame = (BYTE)i + 1;
				if (s->nFrame > 2)
					s->nFrame += -3;
			}
		}
	}
	if (g_fView4) {
		hwndItem = GetDlgItem(hwnd, idB);
		GetClientRect(hwndItem, &g_rcView4B);
		SubclassUserItem(hwnd, idB, View4WndProc);
		InitScene(hwndItem, &g_sceneView4B, lpfn, &g_rcView4B, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView4B, &g_imgView4[6], &g_imgView4[7], 0, 0, -1);
		hwndItem = GetDlgItem(hwnd, idC);
		GetClientRect(hwndItem, &g_rcView4C);
		SubclassUserItem(hwnd, idC, View4WndProc);
		InitScene(hwndItem, &g_sceneView4C, lpfn, &g_rcView4C, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView4C, &g_imgView4[8], &g_imgView4[9], 0, 0, -1);
	}
	SubclassAnimItem(hwnd, idA, View4WndProc, 3);
}

// FUNCTION: AARCADE 0008:0C20
void FAR PASCAL CloseView4(HWND hwnd, int id)
{
	UnsubclassAnimItem(hwnd, id);
	FreeScene(&g_sceneView4A);
	if (g_fView4) {
		FreeScene(&g_sceneView4B);
		FreeScene(&g_sceneView4C);
		FreeImages(g_imgView4, 10, 1);
	} else
		FreeImages(g_imgView4, 6, 1);
}

// FUNCTION: AARCADE 0008:0C68
void FAR PASCAL BackView5(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[4]);
}

// FUNCTION: AARCADE 0008:0C80
LRESULT CALLBACK View5WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		PaintScene(&g_sceneView5A);
		if (g_fView5) {
			PaintScene(&g_sceneView5B);
			PaintScene(&g_sceneView5C);
		}
		break;
	case WM_TIMER:
		TickScene(&g_sceneView5A);
		if (g_fView5) {
			TickScene(&g_sceneView5B);
			TickScene(&g_sceneView5C);
		}
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0008:0D04
void FAR PASCAL OpenView5(HWND hwnd, int idA, int idB, int idC)
{
	SCENEBKPROC lpfn;
	HWND hwndItem;
	SPRITE *s;

	lpfn = NULL;
	if (g_fColor)
		lpfn = BackView5;
	g_fView5 = idB && idC;
	hwndItem = GetDlgItem(hwnd, idA);
	GetClientRect(hwndItem, &g_rcView5A);
	if (g_fView5) {
		LoadPPicImages(16100, 0, 34, g_imgView5, g_nDepth);
		InitScene(hwndItem, &g_sceneView5A, lpfn, &g_rcView5A, g_nDepth2, 3, 0);
	} else {
		LoadPPicImages(16100, 0, 20, g_imgView5, g_nDepth);
		InitScene(hwndItem, &g_sceneView5A, NULL, &g_rcView5A, g_nDepth2, 3, 0);
	}
	s = AddSpriteImage(&g_sceneView5A, g_imgView5, &g_imgView5[4], 4, 0, -1);
	if (s) {
		SetSpriteMove(s, (int)g_cmdView5A, 0L);
		SetSpriteFrame(s, g_imgView5, &g_imgView5[4], 0, 4);
	}
	s = AddSpriteImage(&g_sceneView5A, &g_imgView5[8], &g_imgView5[13], 4, 17, -1);
	if (s) {
		SetSpriteMove(s, (int)g_cmdView5B, 0L);
		SetSpriteFrame(s, &g_imgView5[8], &g_imgView5[13], 0, 5);
	}
	s = AddSpriteImage(&g_sceneView5A, &g_imgView5[18], &g_imgView5[19], 4, 25, -1);
	if (g_fView5) {
		hwndItem = GetDlgItem(hwnd, idB);
		GetClientRect(hwndItem, &g_rcView5B);
		SubclassUserItem(hwnd, idB, View5WndProc);
		InitScene(hwndItem, &g_sceneView5B, lpfn, &g_rcView5B, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView5B, &g_imgView5[20], &g_imgView5[26], 0, 0, -1);
		if (s) {
			SetSpriteFrame(s, &g_imgView5[20], &g_imgView5[26], 0, 6);
			s->nAnimRate = 1;
		}
		hwndItem = GetDlgItem(hwnd, idC);
		GetClientRect(hwndItem, &g_rcView5C);
		SubclassUserItem(hwnd, idC, View5WndProc);
		InitScene(hwndItem, &g_sceneView5C, lpfn, &g_rcView5C, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView5C, &g_imgView5[32], &g_imgView5[33], 0, 0, -1);
	}
	SubclassAnimItem(hwnd, idA, View5WndProc, 3);
}

// FUNCTION: AARCADE 0008:0F19
void FAR PASCAL CloseView5(HWND hwnd, int id)
{
	UnsubclassAnimItem(hwnd, id);
	FreeScene(&g_sceneView5A);
	if (g_fView5) {
		FreeScene(&g_sceneView5B);
		FreeScene(&g_sceneView5C);
		FreeImages(g_imgView5, 34, 1);
	} else
		FreeImages(g_imgView5, 20, 1);
}

// FUNCTION: AARCADE 0008:0F61
void FAR PASCAL BackView6(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[1]);
}

// FUNCTION: AARCADE 0008:0F79
LRESULT CALLBACK View6WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ERASEBKGND:
		break;
	case WM_PAINT:
		PaintScene(&g_sceneView6A);
		if (g_fView6)
			PaintScene(&g_sceneView6B);
		break;
	case WM_TIMER:
		TickScene(&g_sceneView6A);
		if (g_fView6)
			TickScene(&g_sceneView6B);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0008:0FED
void FAR PASCAL OpenView6(HWND hwnd, int id, int idExtra)
{
	SCENEBKPROC lpfn;
	HWND hwndItem;
	SPRITE *s;

	lpfn = NULL;
	if (g_fColor)
		lpfn = BackView6;
	g_fView6 = idExtra != 0;
	hwndItem = GetDlgItem(hwnd, id);
	GetClientRect(hwndItem, &g_rcView6A);
	if (g_fView6) {
		LoadPPicImages(18100, 0, 6, g_imgView6, g_nDepth);
		InitScene(hwndItem, &g_sceneView6A, lpfn, &g_rcView6A, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView6A, g_imgView6, &g_imgView6[1], 5, 3, -1);
		if (s) {
			SetSpriteMove(s, (int)g_cmdView6B, 0L);
			SetSpriteFrame(s, g_imgView6, &g_imgView6[1], 0, 5);
		}
		s = AddSpriteImage(&g_sceneView6A, &g_imgView6[2], &g_imgView6[3], 0, 38, -1);
	} else {
		LoadPPicImages(18100, 0, 2, g_imgView6, g_nDepth);
		InitScene(hwndItem, &g_sceneView6A, NULL, &g_rcView6A, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView6A, g_imgView6, &g_imgView6[1], 0, 1, -1);
		if (s) {
			SetSpriteMove(s, (int)g_cmdView6A, 0L);
			SetSpriteFrame(s, g_imgView6, &g_imgView6[1], 0, 5);
		}
	}
	if (g_fView6) {
		hwndItem = GetDlgItem(hwnd, idExtra);
		GetClientRect(hwndItem, &g_rcView6B);
		SubclassUserItem(hwnd, idExtra, View6WndProc);
		InitScene(hwndItem, &g_sceneView6B, lpfn, &g_rcView6B, g_nDepth2, 3, 0);
		s = AddSpriteImage(&g_sceneView6B, &g_imgView6[4], &g_imgView6[5], 0, 0, -1);
	}
	SubclassAnimItem(hwnd, id, View6WndProc, 3);
}

// FUNCTION: AARCADE 0008:119A
void FAR PASCAL CloseView6(HWND hwnd, int id)
{
	UnsubclassAnimItem(hwnd, id);
	FreeScene(&g_sceneView6A);
	if (g_fView6) {
		FreeScene(&g_sceneView6B);
		FreeImages(g_imgView6, 6, 1);
	} else
		FreeImages(g_imgView6, 2, 1);
}

// FUNCTION: AARCADE 0008:11DA
LRESULT CALLBACK LineWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_ERASEBKGND:
		break;
	case WM_PAINT: {
		int id;
		int y;
		HDC hdc;
		HPEN hpen;
		HGDIOBJ hOld;

		id = GetDlgCtrlID(hwnd);
		if (id == g_idLineA) {
			PaintScene(&g_sceneLineA);
			y = g_rcLineA.bottom - 1;
		} else {
			PaintScene(&g_sceneLineB);
			y = g_rcLineB.top;
		}
		hdc = GetDC(hwnd);
		hpen = CreatePen(0, 2, 0);
		hOld = SelectObject(hdc, hpen);
		MoveTo(hdc, 0, y);
		LineTo(hdc, 512, y);
		DeleteObject(SelectObject(hdc, hOld));
		ReleaseDC(hwnd, hdc);
	}
		/* break after the block: two jumps out of the case */
		break;
	case WM_TIMER:
		TickScene(&g_sceneLineA);
		TickScene(&g_sceneLineB);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 1;
}

// FUNCTION: AARCADE 0008:12B8
SPRITE *FAR PASCAL AddHiddenSprite(SCENE *ps, int id, IMAGE *pimg)
{
	SPRITE *s;
	SPRITEMOVE *p;

	/* both tests use AX, not the copies in SI and DI */
	if ((s = AddSprite(ps)) != 0) {
		if ((p = AllocSpriteMove()) != 0) {
			s->dwData = p;
			s->lpfnMoveEx = MoveSprite;
			p->id = id;
		}
		s->cur.fVisible = 0;
		s->cur.img = *pimg;
		s->cur.x = 0;
		s->cur.y = 0;
		UpdateSpriteRect(s);
	}
	return s;
}

// FUNCTION: AARCADE 0008:131E
void FAR PASCAL BackLine(IMAGE *pimg, LPRECT lprc)
{
	FillImage(pimg, (RECT *)lprc, g_hGdiObjects[1]);
}

// FUNCTION: AARCADE 0008:1336
void FAR PASCAL OpenLineView(HWND hwnd, int idA, int idB)
{
	SCENEBKPROC lpfn;
	HWND hwndItem;

	lpfn = NULL;
	g_idLineA = idA;
	g_idLineB = idB;
	if (g_fColor)
		lpfn = BackLine;
	LoadPPicImages(5000, 0, 2, g_imgLine, g_nDepth);
	hwndItem = GetDlgItem(hwnd, g_idLineA);
	GetClientRect(hwndItem, &g_rcLineA);
	InitScene(hwndItem, &g_sceneLineA, lpfn, &g_rcLineA, g_nDepth2, 2, 0);
	AddHiddenSprite(&g_sceneLineA, (int)g_cmdLineA, &g_imgLine[0]);
	hwndItem = GetDlgItem(hwnd, g_idLineB);
	GetClientRect(hwndItem, &g_rcLineB);
	SubclassUserItem(hwnd, g_idLineB, LineWndProc);
	InitScene(hwndItem, &g_sceneLineB, lpfn, &g_rcLineB, g_nDepth2, 2, 0);
	AddHiddenSprite(&g_sceneLineB, (int)g_cmdLineB, &g_imgLine[1]);
	SubclassAnimItem(hwnd, g_idLineA, LineWndProc, 3);
}

// FUNCTION: AARCADE 0008:1421
void FAR PASCAL CloseLineView(HWND hwnd, int id)
{
	UnsubclassAnimItem(hwnd, id);
	FreeScene(&g_sceneLineA);
	FreeScene(&g_sceneLineB);
	FreeImages(g_imgLine, 2, 1);
}

// FUNCTION: AARCADE 0008:1451
void FAR PASCAL OpenViews(HWND hwnd)
{
	g_fAnimate = FALSE;
	OpenLineView(hwnd, 22, 23);
	OpenView1(hwnd, 15, 0, 0);
	OpenView2(hwnd, 16, 0, 0);
	OpenView3(hwnd, 17, 0, 0);
	OpenView4(hwnd, 18, 0, 0);
	OpenView5(hwnd, 19, 0, 0);
	OpenView6(hwnd, 20, 0);
	UpdateWindow(hwnd);
	g_fAnimate = TRUE;
}

// FUNCTION: AARCADE 0008:14BA
void FAR PASCAL TickViews(void)
{
	TickScene(&g_sceneLineA);
	TickScene(&g_sceneLineB);
	TickScene(&g_sceneView1A);
	TickScene(&g_sceneView2A);
	TickScene(&g_sceneView3A);
	TickScene(&g_sceneView4A);
	TickScene(&g_sceneView5A);
	TickScene(&g_sceneView6A);
}

// FUNCTION: AARCADE 0008:1501
void FAR PASCAL CloseViews(HWND hwnd)
{
	StopAnimTimer();
	CloseLineView(hwnd, 22);
	CloseView1(hwnd, 15);
	CloseView2(hwnd, 16);
	CloseView3(hwnd, 17);
	CloseView4(hwnd, 18);
	CloseView5(hwnd, 19);
	CloseView6(hwnd, 20);
}
