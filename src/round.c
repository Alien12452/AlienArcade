/* g_nRound is the game chosen on the title screen, in menu order:
   1 Stink Snakes (round1.c), 2 Nerdoids (round2.c),
   3 Quasar Klutzes (round3.c), 4 Fungus Patrol (round4.c),
   5 Andromedroids (round5.c), 6 Quasar Rain (round6.c). */
#include "aarcade.h"

// FUNCTION: AARCADE 001A:0000
void FAR PASCAL SetRound(int n)
{
	g_nRound = n;
}

// FUNCTION: AARCADE 001A:000F
void FAR PASCAL OpenRound(HWND hwnd)
{
	g_nPlayWave = 0;
	switch (g_nRound - 1) {
	case 0: /* Stink Snakes */
		g_nPlayAmmo = 20;
		OpenRound1(hwnd);
		break;
	case 1: /* Nerdoids */
		g_nPlayAmmo = 20;
		OpenRound2(hwnd);
		break;
	case 2: /* Quasar Klutzes */
		g_nPlayAmmo = 20;
		OpenRound3(hwnd);
		break;
	case 3: /* Fungus Patrol */
		OpenRound4(hwnd);
		break;
	case 4: /* Andromedroids */
		OpenRound5(hwnd);
		break;
	case 5: /* Quasar Rain */
		OpenRound6(hwnd);
		break;
	}
	PaintRound();
	g_nPlayWave = -1;
	RunStepDialog();
	g_nPlayWave = 0;
}

// FUNCTION: AARCADE 001A:0098
void FAR PASCAL ClickRound(void)
{
	switch (g_nRound - 1) {
	case 0: /* Stink Snakes */
		ClickRound1();
		break;
	case 1: /* Nerdoids */
		ClickRound2();
		break;
	case 2: /* Quasar Klutzes */
		ClickRound3();
		break;
	case 3: /* Fungus Patrol */
		ClickRound4();
		break;
	case 4: /* Andromedroids */
		ClickRound5();
		break;
	case 5: /* Quasar Rain */
		ClickRound6();
		break;
	}
}

// FUNCTION: AARCADE 001A:00E6
void FAR PASCAL PaintRound(void)
{
	switch (g_nRound - 1) {
	case 0: /* Stink Snakes */
		PaintRound1();
		break;
	case 1: /* Nerdoids */
		PaintRound2();
		break;
	case 2: /* Quasar Klutzes */
		PaintRound3();
		break;
	case 3: /* Fungus Patrol */
		PaintRound4();
		break;
	case 4: /* Andromedroids */
		PaintRound5();
		break;
	case 5: /* Quasar Rain */
		PaintRound6();
		break;
	}
	DrawStatusBar();
}

// FUNCTION: AARCADE 001A:0139
int FAR PASCAL TickRound(void)
{
	int n;

	SetCursor(g_hCursor);
	switch (g_nRound - 1) {
	case 0: /* Stink Snakes */
		n = TickRound1();
		break;
	case 1: /* Nerdoids */
		n = TickRound2();
		break;
	case 2: /* Quasar Klutzes */
		n = TickRound3();
		break;
	case 3: /* Fungus Patrol */
		n = TickRound4();
		break;
	case 4: /* Andromedroids */
		n = TickRound5();
		break;
	case 5: /* Quasar Rain */
		n = TickRound6();
		break;
	}
	TickTimeLeft();
	if (n)
		RunRoundDone();
	return n;
}

// FUNCTION: AARCADE 001A:01A6
void FAR PASCAL CloseRound(void)
{
	switch (g_nRound - 1) {
	case 0: /* Stink Snakes */
		CloseRound1();
		break;
	case 1: /* Nerdoids */
		CloseRound2();
		break;
	case 2: /* Quasar Klutzes */
		CloseRound3();
		break;
	case 3: /* Fungus Patrol */
		CloseRound4();
		break;
	case 4: /* Andromedroids */
		CloseRound5();
		break;
	case 5: /* Quasar Rain */
		CloseRound6();
		break;
	}
}

// FUNCTION: AARCADE 001A:01F4
void FAR PASCAL DecImageBottoms(IMAGE *pimg, int iFirst, int iLast)
{
	int n;

	/* &= index, not +=: the add goes through DX and iFirst stays in DI */
	pimg = &pimg[iFirst];
	n = iLast - iFirst + 1;
	while (n--) {
		/* _CX keeps the count on the stack; --_CX is dec cx, not a trip through AX */
		_CX = pimg->rc.bottom;
		--_CX;
		pimg->rc.bottom = _CX;
		pimg++;
	}
}
