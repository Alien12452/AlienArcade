#include "aarcade.h"

/* Session open/close and loading the AACOLOR or AABW picture library. */

/* Color or black-and-white data DLL. A handle below 32 is a load error.
     Zero so these stay in _DATA, immediately before score.c. */
// GLOBAL: AARCADE 0024:0726
HINSTANCE g_hDataDll = 0;

// GLOBAL: AARCADE 0024:0728
int g_fSession = 0;

int FAR PASCAL LoadSessionDll(void);

// FUNCTION: AARCADE 0014:0000
int FAR PASCAL OpenSession(void)
{
	int nErr;

	g_szPlayName[0] = 0;
	g_nRound = 1;
	LoadRoundScores();
	InitSound();
	nErr = LoadSessionDll();
	g_fSession = 1;
	return nErr;
}

// FUNCTION: AARCADE 0014:0032
int FAR PASCAL LoadSessionDll(void)
{
	char sz[256];
	int nErr;

	if (g_fColor)
		lstrcpy(sz, g_szColorDll);
	else
		lstrcpy(sz, g_szBWDll);
	g_hDataDll = LoadDataLibrary(sz);
	/* jae to the zero: the failure arm is the then-branch */
	if (g_hDataDll < 32)
		nErr = -1;
	else
		nErr = 0;
	return nErr;
}

// FUNCTION: AARCADE 0014:0080
void FAR PASCAL FreeSessionDll(void)
{
	if (g_hDataDll >= 32) {
		FreeDataLibrary(g_hDataDll);
		g_hDataDll = 0;
	}
}

// FUNCTION: AARCADE 0014:009D
void FAR PASCAL CloseSession(void)
{
	if (g_fSession != 0) {
		FreeSessionDll();
		if (g_hwndChild != 0)
			CloseRound();
		CloseSfx();
		SaveRoundScores();
		SaveConfig();
		GFree(g_hDaily);
		GFree(g_hAllTime);
		g_hDaily = 0;
		g_hAllTime = 0;
		GFree(g_hHold1);
		GFree(g_hHold2);
		GFree(g_hHold3);
		g_hHold1 = 0;
		g_hHold2 = 0;
		g_hHold3 = 0;
	}
}
