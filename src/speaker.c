#include <dos.h>
#include "aarcade.h"

/* PC speaker, used when MMSYSTEM is missing or useMMIO=0.
   Notes are (frequency, duration) pairs ended by -1. A duration with a
   positive frequency is timed by a Windows timer; the rest are waited out. */


/* Zero so they stay with the note tables instead of in BSS. */
// GLOBAL: AARCADE 0024:096C
int *g_pSpkScript = 0;

// GLOBAL: AARCADE 0024:096E
int g_nSpkIndex = 0;

// GLOBAL: AARCADE 0024:0970
int g_fSpkOn = 0;

// GLOBAL: AARCADE 0024:0972
UINT g_nSpkTimer = 0;

// GLOBAL: AARCADE 0024:0974
FARPROC g_lpfnSpkTimer = 0;

// GLOBAL: AARCADE 0024:0978
int g_anSpkId[56] = {
	9000, 9001, 9002, 9003, 9004, 9005, 9006, 9007, 9008, 9009, 9010, 9010, 9011, 9012, 9013, 9014,
	9015, 9016, 9017, 9018, 6000, 7000, 7001, 7002, 7003, 7004, 6001, 6002, 6003, 6004, 8000, 8001,
	8002, 8003, 8004, 8005, 8006, 16000, 16001, 16002, 16003, 16004, 16005, 16006, 16007, 16008, 16009, 18000,
	18001, 18002, 17000, 17001, 17002, 17003, 17004, 0,
};

// GLOBAL: AARCADE 0024:09E8
int g_anSpkNote[] = {
	700, 50, 750, 50, 850, 50, -1, 100, 400, 50, 350, -1, -1, 0, -710, 70,
	-4, 70, -710, 70, -825, 300, -1, -1, 0, -300, 250, -250, 200, -1, -1, 0,
	-600, 50, -1, -800, 50, -1, -710, 70, -4, 70, -710, 70, -825, 300, -1, -1,
	0, -1, 0, -1, 0, -200, 50, -1, 1, 50, 700, 50, 750, 50, 800, 50,
	-1, 500, 100, -1, 600, 100, -1, 700, 100, -1, 800, 100, -1, 300, 100, -1,
	75, 75, 50, 75, -1, 600, 75, -1, 2000, 75, 1800, 75, 1000, 75, -1, 700,
	100, 800, 50, -1, 200, 75, -1, 350, 100, -1, 600, 75, -1, 750, 100, -1,
	450, 100, -1, 650, 50, 800, 50, -1, 600, 75, -1, 700, 25, 725, 25, 775,
	25, 875, 25, -1, 700, 25, 725, 25, 775, 25, 875, 25, -1, -1, 0, 2000,
	75, 1800, 75, -1, 19, 150, -1, -1, 0, 2000, 50, 3000, 50, -1, 1500, 50,
	-1, 1000, 25, 800, 25, 1000, 25, 800, 25, 1000, 25, -1, 100, 175, -1, 300,
	125, -1, 800, 50, 900, 75, -1, 1000, 75, 1800, 75, 2000, 75, -1, -1, 0,
	-1, 0, -1, 0, 600, 75, -1, 700, 25, 725, 25, 775, 25, 875, 25, -1,
	2000, 75, 1800, 75, -1, 600, 75, -1, 750, 100, -1, 450, 100, -1, 650, 75,
	-1,
};

// GLOBAL: AARCADE 0024:0BAA
int *g_apSpkScript[56] = {
	&g_anSpkNote[0], &g_anSpkNote[7], &g_anSpkNote[12], &g_anSpkNote[14],
	&g_anSpkNote[23], &g_anSpkNote[25], &g_anSpkNote[30], &g_anSpkNote[32],
	&g_anSpkNote[35], &g_anSpkNote[38], &g_anSpkNote[47], &g_anSpkNote[49],
	&g_anSpkNote[51], &g_anSpkNote[53], &g_anSpkNote[56], &g_anSpkNote[65],
	&g_anSpkNote[68], &g_anSpkNote[71], &g_anSpkNote[74], &g_anSpkNote[77],
	&g_anSpkNote[80], &g_anSpkNote[85], &g_anSpkNote[88], &g_anSpkNote[95],
	&g_anSpkNote[100], &g_anSpkNote[103], &g_anSpkNote[106], &g_anSpkNote[109],
	&g_anSpkNote[112], &g_anSpkNote[115], &g_anSpkNote[120], &g_anSpkNote[123],
	&g_anSpkNote[132], &g_anSpkNote[141], &g_anSpkNote[143], &g_anSpkNote[148],
	&g_anSpkNote[151], &g_anSpkNote[153], &g_anSpkNote[158], &g_anSpkNote[161],
	&g_anSpkNote[172], &g_anSpkNote[175], &g_anSpkNote[178], &g_anSpkNote[183],
	&g_anSpkNote[190], &g_anSpkNote[192], &g_anSpkNote[194], &g_anSpkNote[208],
	&g_anSpkNote[199], &g_anSpkNote[80], &g_anSpkNote[213], &g_anSpkNote[216],
	&g_anSpkNote[219], &g_anSpkNote[222], &g_anSpkNote[80], 0,
};
void FAR PASCAL PlaySpkPair(int nUnused, int fTimer);
void FAR PASCAL AdvanceSpeaker(int nUnused);
void FAR PASCAL SetSpeakerPitch(int freq);
void FAR PASCAL SpeakerOff(void);
void FAR PASCAL StartSpeaker(int freq, long ms, int fTimer);
void FAR PASCAL StopSpeaker(int fKill);
int *FAR PASCAL FindSpeakerScript(int id);
void CALLBACK _export SpeakerTimerProc(HWND hwnd, UINT msg, UINT id, DWORD dwTime);

/* Nothing calls this. It is the first function in the segment. */
// FUNCTION: AARCADE 0018:0000
void FAR PASCAL SpeakerStub(void)
{
}

// FUNCTION: AARCADE 0018:0007
void FAR PASCAL EndSpeaker(void)
{
	StopSpeaker(1);
}

// FUNCTION: AARCADE 0018:0015
void FAR PASCAL PlaySpeaker(int id, BOOL fForce, int nUnused)
{
	int *p;

	if (g_fSpkOn != 0 && fForce != 0)
		StopSpeaker(1);
	if (g_fSpkOn == 0) {
		if ((p = FindSpeakerScript(id)) != 0) {
			g_pSpkScript = p;
			g_nSpkIndex = 0;
			AdvanceSpeaker(1);
		}
	}
}

// FUNCTION: AARCADE 0018:005E
void FAR PASCAL PlaySpkPair(int nUnused, int fTimer)
{
	int freq;
	long ms;

	freq = g_pSpkScript[g_nSpkIndex];
	ms = g_pSpkScript[g_nSpkIndex + 1];
	StartSpeaker(freq, ms, fTimer);
	g_nSpkIndex += 2;
}

// FUNCTION: AARCADE 0018:00A5
void FAR PASCAL AdvanceSpeaker(int nUnused)
{
	int freq;
	long ms;
	DWORD t;
	int note;
	int fTimer;

	if (g_pSpkScript != 0) {
		do {
			/* reset each pass: a following -1 must not keep the previous 0 */
			fTimer = 1;
			note = g_pSpkScript[g_nSpkIndex];
			if (note != -1) {
				/* label on the next statement: jmp to it is eb 00 */
				goto play;
			play:
				freq = note;
				ms = g_pSpkScript[g_nSpkIndex + 1];
				fTimer = freq > 0;
				PlaySpkPair(nUnused, fTimer);
				if (fTimer == 0) {
					t = GetTickCount();
					while (GetTickCount() < t + ms)
						;
				}
				/* both wait exits land on this eb 00, then the jmp over Stop */
				goto join;
			join:;
			} else
				StopSpeaker(1);
		} while (fTimer == 0);
	}
}

/* 0x123540 is the PIT clock this game divides by. 18 Hz does not fit the counter. */
// FUNCTION: AARCADE 0018:0143
void FAR PASCAL SetSpeakerPitch(int freq)
{
	int nDiv;
	unsigned char b;

	if (freq < 0)
		freq = -freq;
	if (freq > 0x12) {
		nDiv = 0x123540L / freq;
		outportb(0x43, 0xB6);
		outportb(0x42, nDiv);
		outportb(0x42, ((unsigned char *)&nDiv)[1]);
		b = inportb(0x61);
		outportb(0x61, b | 3);
	} else
		SpeakerOff();
}

// FUNCTION: AARCADE 0018:01A0
void FAR PASCAL SpeakerOff(void)
{
	unsigned char b;

	b = inportb(0x61);
	outportb(0x61, b & 0xFC);
}

/* One-shot beep. Nothing in the exe calls it. */
// FUNCTION: AARCADE 0018:01BA
void FAR PASCAL BeepSpeaker(int freq)
{
	if (g_fSoundOn != 0 && g_fUseMMIO == 0)
		StartSpeaker(freq, 10, 0);
}

/* Silence without KillTimer. Nothing in the exe calls it. */
// FUNCTION: AARCADE 0018:01DF
void FAR PASCAL MuteSpeaker(void)
{
	if (g_fSoundOn != 0 && g_fUseMMIO == 0)
		StopSpeaker(0);
}

// FUNCTION: AARCADE 0018:01FB
void FAR PASCAL StartSpeaker(int freq, long ms, int fTimer)
{
	if (g_fSpkOn != 0)
		KillTimer(NULL, g_nSpkTimer);
	SetSpeakerPitch(freq);
	g_fSpkOn = 1;
	if (fTimer != 0) {
		if (g_lpfnSpkTimer == 0)
			g_lpfnSpkTimer = MakeProcInstance((FARPROC)SpeakerTimerProc, g_hInstance);
		g_nSpkTimer = SetTimer(NULL, 0x1F4, (UINT)ms, g_lpfnSpkTimer);
	}
}

// FUNCTION: AARCADE 0018:0260
void FAR PASCAL StopSpeaker(int fKill)
{
	if (g_fSpkOn != 0) {
		SpeakerOff();
		if (fKill != 0)
			KillTimer(NULL, g_nSpkTimer);
		g_fSpkOn = 0;
	}
}

// FUNCTION: AARCADE 0018:028B
void FAR PASCAL TickSpeaker(void)
{
}

// FUNCTION: AARCADE 0018:0292
void CALLBACK _export SpeakerTimerProc(HWND hwnd, UINT msg, UINT id, DWORD dwTime)
{
	if (g_fSpkOn != 0)
		AdvanceSpeaker(0);
}

// FUNCTION: AARCADE 0018:02AF
int *FAR PASCAL FindSpeakerScript(int id)
{
	int i;

	for (i = 0; g_anSpkId[i] > 0; i++) {
		if (g_anSpkId[i] == id)
			return g_apSpkScript[i];
	}
	return 0;
}
