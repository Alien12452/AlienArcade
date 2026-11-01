#include <string.h>
#include "aarcade.h"

int cdecl sprintf(char *, const char *, ...);

int FAR PASCAL FindDlls(void);
int FAR PASCAL DllPresent(char *szName);
void FAR PASCAL WriteConfig(void);

/* INI config (AARCADE.INI) and locating the AACOLOR / AABW picture DLLs. */

/* Profile text. Linked after play.c's control table (0024:05C0).
     The cursor, the game font, and the title sit between the format strings
     and the ini filename; the high-score filename and the sound flag sit
   between the ini filename and g_fUseMMIO. */
// GLOBAL: AARCADE 0024:05C0
char g_szConfig[] = "Configuration";

// GLOBAL: AARCADE 0024:05CE
char g_szVersion[] = "version";

// GLOBAL: AARCADE 0024:05D6
char g_szLevel[] = "level";

// GLOBAL: AARCADE 0024:05DC
char g_szTopics[] = "topics";

// GLOBAL: AARCADE 0024:05E3
char g_szEncounters[] = "encounters";

// GLOBAL: AARCADE 0024:05EE
char g_szMissions[] = "missions";

// GLOBAL: AARCADE 0024:05F7
char g_szSoundOn[] = "soundOn";

// GLOBAL: AARCADE 0024:05FF
char g_szUseMMIO[] = "useMMIO";

// GLOBAL: AARCADE 0024:0607
char g_szPatrol[] = "Fungus Patrol";

// GLOBAL: AARCADE 0024:0615
char g_szBackground[] = "background";

// GLOBAL: AARCADE 0024:0620
char g_szAabw[] = "aabw.dll";

// GLOBAL: AARCADE 0024:0629
char g_szAacolor[] = "aacolor.dll";

/* Each call has its own copy. */
// GLOBAL: AARCADE 0024:0635
char g_szFmtVer[] = "%u";

// GLOBAL: AARCADE 0024:0638
char g_szFmtLevel[] = "%u";

// GLOBAL: AARCADE 0024:063B
char g_szFmtTopics[] = "%u";

// GLOBAL: AARCADE 0024:063E
char g_szFmtEnc[] = "%u";

// GLOBAL: AARCADE 0024:0641
char g_szFmtMis[] = "%u";

// GLOBAL: AARCADE 0024:0644
HCURSOR g_hCursor = 0;

// GLOBAL: AARCADE 0024:0646
HGDIOBJ g_hGameFont = 0;

// GLOBAL: AARCADE 0024:0648
char g_szAppTitle[] = "Alien Arcade";

// GLOBAL: AARCADE 0024:0655
char g_szIni[] = "AARCADE.INI";

// GLOBAL: AARCADE 0024:0661
char g_szHiFile[] = "AARCADE.HI";

// GLOBAL: AARCADE 0024:066C
BOOL g_fSoundOn = TRUE;

/* Missing key reads as 1, and the loader only clears this. */
// GLOBAL: AARCADE 0024:066E
BOOL g_fUseMMIO = TRUE;

/* Uninitialized data, including the 3C31..3C7F block util.c reads.
   aarcade.h lists these highest address first. */

/* Set from a random layout pick. With g_fLayoutFits, centering shifts by (47, 36). */
// GLOBAL: AARCADE 0024:3C31
int g_fAltLayout;

// GLOBAL: AARCADE 0024:3C33
RECT g_rcLayout;

/* Nonzero when the main client area is large enough for the playfield. */
// GLOBAL: AARCADE 0024:3C3B
int g_fLayoutFits;

// GLOBAL: AARCADE 0024:3C3D
RGBQUAD g_rgbGame[16];

// GLOBAL: AARCADE 0024:3C7D
int g_nUserWndExtra;

// GLOBAL: AARCADE 0024:3C7F
int g_nUserClsExtra;


// GLOBAL: AARCADE 0024:3B31
char g_szPlayName[256];


/* Time left copied aside while the bonus dialog counts it down. */
// GLOBAL: AARCADE 0024:3B2D
long g_lBonusTime;


// GLOBAL: AARCADE 0024:3B29
long g_lSavedCount;

/* 0024:3B25. Nothing refers to these bytes. */
// GLOBAL: AARCADE 0024:3B25
char g_abBss3B25[4];


// GLOBAL: AARCADE 0024:3B21
long g_lActivateTime;

/* 0024:3AD9. Nothing refers to these bytes. */
// GLOBAL: AARCADE 0024:3AD9
char g_abBss3AD9[72];


// GLOBAL: AARCADE 0024:3AD8
unsigned char g_nMissions;


// GLOBAL: AARCADE 0024:3AD7
unsigned char g_nEncounters;


// GLOBAL: AARCADE 0024:3AD6
unsigned char g_nTopics;


// GLOBAL: AARCADE 0024:3AD5
unsigned char g_nLevel;


/* Configuration values from AARCADE.INI. */
// GLOBAL: AARCADE 0024:3AD3
unsigned g_nCfgVersion;


// GLOBAL: AARCADE 0024:3AD1
HGLOBAL g_hHold3;

/* 0024:3ACC. Nothing refers to these bytes. */
// GLOBAL: AARCADE 0024:3ACC
char g_abBss3ACC[5];


// GLOBAL: AARCADE 0024:3ACA
int g_wHold3;


// GLOBAL: AARCADE 0024:3AC8
HGLOBAL g_hHold2;

/* 0024:3AC3. Nothing refers to these bytes. */
// GLOBAL: AARCADE 0024:3AC3
char g_abBss3AC3[5];


// GLOBAL: AARCADE 0024:3AC1
int g_wHold2;


// GLOBAL: AARCADE 0024:3ABF
HGLOBAL g_hHold1;

/* 0024:3ABA. Nothing refers to these bytes. */
// GLOBAL: AARCADE 0024:3ABA
char g_abBss3ABA[5];

/* Three records freed at shutdown: a word, five unused bytes, then the handle. */
// GLOBAL: AARCADE 0024:3AB8
int g_wHold1;


// GLOBAL: AARCADE 0024:39B8
char g_szBWDll[256];


// GLOBAL: AARCADE 0024:38B8
char g_szColorDll[256];

// FUNCTION: AARCADE 000F:0000
int FAR PASCAL LoadConfig(void)
{
	g_nCfgVersion = GetPrivateProfileInt(g_szConfig, g_szVersion, 1, g_szIni);
	g_nLevel = GetPrivateProfileInt(g_szConfig, g_szLevel, 0, g_szIni);
	g_nTopics = GetPrivateProfileInt(g_szConfig, g_szTopics, 0x80, g_szIni);
	g_nEncounters = GetPrivateProfileInt(g_szConfig, g_szEncounters, 0, g_szIni);
	g_nMissions = GetPrivateProfileInt(g_szConfig, g_szMissions, 0, g_szIni);
	if (GetPrivateProfileInt(g_szConfig, g_szSoundOn, 1, g_szIni) == 0)
		g_fSoundOn = 0;
	if (GetPrivateProfileInt(g_szConfig, g_szUseMMIO, 1, g_szIni) == 0)
		g_fUseMMIO = 0;
	return FindDlls();
}

// FUNCTION: AARCADE 000F:00B7
void FAR PASCAL WriteConfig(void)
{
	char sz[16];

	sprintf(sz, g_szFmtVer, g_nCfgVersion);
	WritePrivateProfileString(g_szConfig, g_szVersion, sz, g_szIni);
	sprintf(sz, g_szFmtLevel, g_nLevel);
	WritePrivateProfileString(g_szConfig, g_szLevel, sz, g_szIni);
	sprintf(sz, g_szFmtTopics, g_nTopics);
	WritePrivateProfileString(g_szConfig, g_szTopics, sz, g_szIni);
	sprintf(sz, g_szFmtEnc, g_nEncounters);
	WritePrivateProfileString(g_szConfig, g_szEncounters, sz, g_szIni);
	sprintf(sz, g_szFmtMis, g_nMissions);
	WritePrivateProfileString(g_szConfig, g_szMissions, sz, g_szIni);
}

// FUNCTION: AARCADE 000F:0196
int FAR PASCAL GetPatrolBackground(void)
{
	/* 1 when the setting is 0 */
	return !GetPrivateProfileInt(g_szPatrol, g_szBackground, 0, g_szIni);
}

// FUNCTION: AARCADE 000F:01B7
void FAR PASCAL SaveConfig(void)
{
	WriteConfig();
}

// FUNCTION: AARCADE 000F:01C2
int FAR PASCAL FindDlls(void)
{
	g_szBWDll[0] = 0;
	g_szColorDll[0] = 0;
	g_wHold1 = 0;
	g_hHold1 = 0;
	g_wHold2 = 0;
	g_hHold2 = 0;
	g_wHold3 = 0;
	g_hHold3 = 0;
	if (DllPresent(g_szAabw))
		lstrcpy(g_szBWDll, g_szAabw);
	if (DllPresent(g_szAacolor))
		lstrcpy(g_szColorDll, g_szAacolor);
	return 0;
}

// FUNCTION: AARCADE 000F:022D
int FAR PASCAL DllPresent(char *szName)
{
	OFSTRUCT of;
	char szPath[128];
	char *p;

	GetModuleFileName(g_hInstance, szPath, 0x80);
	if ((p = strrchr(szPath, '\\')) != 0) {
		p[1] = 0;
		strcat(szPath, szName);
	} else
		strcpy(szPath, szName);
	if (OpenFile(szPath, &of, OF_EXIST) != HFILE_ERROR)
		of.nErrCode = 0;
	/* ?: keeps both jumps; if/return merges the first onto the epilogue */
	return of.nErrCode == 0 ? 1 : 0;
}
