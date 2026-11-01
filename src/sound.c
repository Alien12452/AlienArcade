/* Sound effects: WAVE resources played through MMSYSTEM, or the PC speaker
   (segment 24) when MMSYSTEM is missing or switched off with useMMIO=0. */
#include <mem.h>
#include "aarcade.h"

/* The volume is only stored; nothing passes it to MMSYSTEM. */
// GLOBAL: AARCADE 0024:0940
DWORD g_dwVolume = 0;

// GLOBAL: AARCADE 0024:0944
WAVE *g_pWave = 0;

/* The resource of the wave that is playing, locked while it plays. */
// GLOBAL: AARCADE 0024:0946
HGLOBAL g_hSnd = 0;

// GLOBAL: AARCADE 0024:0948
int g_anVolume[10] = {
	0, 0x40, 0x80, 0x400, 0x800, 0x2000, 0x4000, 0x8000, 0xC000, 0xFFFF
};

/* An array, not a literal, so the word below is still initialized data.
   OpenWave pushes this address. */
// GLOBAL: AARCADE 0024:095C
static char g_szMMSystem[] = "MMSYSTEM.DLL";

/* 0024:0969. The array ends on an odd address and this word follows it
   with no pad, so the module ends at 096B. The linker then word-aligns
   speaker.c to 096C. Nothing reads it. */
// GLOBAL: AARCADE 0024:0969
static int g_nSndPad = 0;

/* Uninitialized data. Order here does not matter: aarcade.h declares
   these first, highest address first, and that is the order Borland reverses. */

/* Cache of loaded WAVE resources by id, replaced round-robin from g_nSndNext. */
// GLOBAL: AARCADE 0024:3CFC
HGLOBAL g_ahSnd[16];

// GLOBAL: AARCADE 0024:3D1C
int g_anSndId[16];

// GLOBAL: AARCADE 0024:3D3C
int g_nSndNext;

// GLOBAL: AARCADE 0024:3CF2
WAVE g_wave;

// FUNCTION: AARCADE 0017:0000
void FAR PASCAL InitSound(void)
{
	int *p;
	int n;

	p = g_anSndId;
	g_nSndNext = 0;
	n = 16;
	while (n--)
		*p++ = 0;
	if (OpenWave(&g_pWave)) {
	}
	SetWaveState(g_pWave, 0);
}

/* Releases the wave once it has finished; FALSE while it still plays. */
// FUNCTION: AARCADE 0017:003E
BOOL FAR PASCAL FinishWave(void)
{
	if (g_fUseMMIO) {
		if (IsWaveDone(g_pWave)) {
			SetWaveState(g_pWave, 0);
			UnlockRes(g_hSnd);
			FreeWave(g_pWave);
			g_hSnd = 0;
			return 1;
		} else
			return 0;
	} else
		return 1;
}

// FUNCTION: AARCADE 0017:008B
void FAR PASCAL TickSound(void)
{
	if (!g_fUseMMIO)
		TickSpeaker();
	FinishWave();
}

// FUNCTION: AARCADE 0017:00A2
void FAR PASCAL StopSfx(void)
{
	if (!FinishWave() && IsWaveBusy(g_pWave)) {
		StopWave(g_pWave);
		SetWaveState(g_pWave, 0);
		UnlockRes(g_hSnd);
		g_hSnd = 0;
	}
}

// FUNCTION: AARCADE 0017:00E1
void FAR PASCAL CloseSfx(void)
{
	HGLOBAL *ph;
	int id;
	register int n;

	/* p would take SI and ph DI; the count is the one in DI */
	_SI = (unsigned)g_anSndId;
	ph = g_ahSnd;
	StopSfx();
	g_nSndNext = 0;
	n = 16;
	while (n--) {
		id = *(int *)_SI;
		*(int *)_SI = 0;
		_SI += 2;
		if (id != 0)
			FreeRes(*ph++);
	}
	CloseWave(g_pWave);
	g_pWave = 0;
}

/* Finds the WAVE resource in the cache or loads it into the next slot, locked. */
// FUNCTION: AARCADE 0017:013C
HGLOBAL FAR PASCAL LoadSound(int id)
{
	int *p;
	int i;
	HGLOBAL h;

	p = g_anSndId;
	for (i = 0; i < 16; i++) {
		if (*p++ == id) {
			h = g_ahSnd[i];
			if (h != 0) {
				if (LockRes(h) != 0)
					return h;
				else {
					FreeRes(h);
					g_anSndId[i] = 0;
					g_ahSnd[i] = 0;
					break;
				}
			}
		}
	}
	if (i == 16) {
		i = ++g_nSndNext;
		if (i == 16)
			i = g_nSndNext = 0;
		if (g_anSndId[i] != 0) {
			FreeRes(g_ahSnd[i]);
			g_anSndId[i] = 0;
			g_ahSnd[i] = 0;
		}
	}
	if ((h = FindRes(g_szWaveType, id)) == 0) {
	}
	if (h != 0) {
		g_anSndId[i] = id;
		g_ahSnd[i] = h;
		LockRes(h);
	}
	return h;
}

// FUNCTION: AARCADE 0017:0219
void FAR PASCAL PreloadSounds(int id, int n)
{
	HGLOBAL h;

	if (g_fSoundOn && g_fUseMMIO) {
		while (n--) {
			h = LoadSound(id++);
			if (h != 0)
				UnlockRes(h);
		}
	}
}

// FUNCTION: AARCADE 0017:025F
void FAR PASCAL PlaySfx(int id, BOOL fForce)
{
	PlaySoundEx(id, fForce, 0);
}

/* fForce cuts off a sound that is still playing; otherwise the new one is dropped. */
// FUNCTION: AARCADE 0017:0275
void FAR PASCAL PlaySoundEx(int id, BOOL fForce, int n)
{
	FinishWave();
	if (g_fSoundOn) {
		if (g_fUseMMIO) {
			if (IsWaveIdle(g_pWave) || fForce) {
				if (!IsWaveIdle(g_pWave))
					StopSfx();
				StartWave(g_pWave, id, n);
			}
		} else
			PlaySpeaker(id, fForce, n);
	}
}

/* Volume menu (IDM_GAME_FIRST..IDM_GAME_LAST); the lowest entry turns sound off. */
// FUNCTION: AARCADE 0017:02D5
int FAR PASCAL SelectVolume(WPARAM id)
{
	UINT err;
	DWORD dw;
	int n;

	n = id - IDM_GAME_FIRST;
	if (!g_fUseMMIO || n < 0 || n > 9)
		return 0;
	else {
		err = 0;
		dw = MAKELONG(g_anVolume[n], g_anVolume[n]);
		if (err == 0) {
			g_dwVolume = dw;
			g_fSoundOn = dw != 0;
		}
	}
	return 0;
}

// FUNCTION: AARCADE 0017:0346
int FAR PASCAL OpenWave(WAVE **pp)
{
	HFILE hf;
	OFSTRUCT of;
	HGLOBAL h;

	if (g_fUseMMIO) {
		hf = OpenFile(g_szMMSystem, &of, OF_EXIST);
		g_fUseMMIO = hf != -1;
		if (g_fUseMMIO) {
			if ((h = FindRes(g_szWaveType, 4000)) != 0) {
				LockRes(h);
				if (!ParseWave(h, 0, 0, 0))
					g_fUseMMIO = 0;
				UnlockRes(h);
				FreeRes(h);
			} else
				g_fUseMMIO = 0;
		}
	}
	*pp = &g_wave;
	return 0;
}

// FUNCTION: AARCADE 0017:03D7
void FAR PASCAL SetWaveState(WAVE *p, int n)
{
	p->nState = n;
}

// FUNCTION: AARCADE 0017:03EA
BOOL FAR PASCAL IsWaveIdle(WAVE *p)
{
	if (IsWaveDone(p))
		FinishWave();
	return p->nState == 0;
}

// FUNCTION: AARCADE 0017:0414
BOOL FAR PASCAL IsWaveDone(WAVE *p)
{
	LPWAVEHDR lpHdr;

	lpHdr = p->lpHdr;
	if (lpHdr == 0 || p->nState == 1 && (lpHdr->dwFlags & WHDR_DONE))
		p->nState = 2;
	return p->nState == 2;
}

// FUNCTION: AARCADE 0017:0465
BOOL FAR PASCAL IsWaveBusy(WAVE *p)
{
	return !IsWaveIdle(p);
}

// FUNCTION: AARCADE 0017:047C
void FAR PASCAL StopWave(WAVE *p)
{
	FreeWave(p);
}

// FUNCTION: AARCADE 0017:048D
void FAR PASCAL CloseWave(WAVE *p)
{
	StopSfx();
	if (!g_fUseMMIO)
		EndSpeaker();
}

// FUNCTION: AARCADE 0017:04A6
void FAR PASCAL FreeWave(WAVE *p)
{
	WAVECTX FAR *lpCtx;
	LPWAVEHDR lpHdr;

	lpCtx = p->lpCtx;
	lpHdr = p->lpHdr;
	if (lpCtx == 0)
		return;
	waveOutReset(lpCtx->hwo);
	while (!(lpHdr->dwFlags & WHDR_DONE))
		;
	waveOutUnprepareHeader(lpCtx->hwo, lpHdr, sizeof(WAVEHDR));
	waveOutClose(lpCtx->hwo);
	GlobalUnlock(lpCtx->hHdr);
	GlobalFree(lpCtx->hHdr);
	GlobalUnlock(lpCtx->hCtx);
	GlobalFree(lpCtx->hCtx);
	p->lpCtx = 0;
	p->lpHdr = 0;
}

// FUNCTION: AARCADE 0017:055E
int FAR PASCAL StartWave(WAVE *p, int id, int n)
{
	if (g_fUseMMIO) {
		g_hSnd = LoadSound(id);
		if (g_hSnd != 0) {
			SetWaveState(p, 1);
			return OpenWaveOut(p, g_hSnd, n);
		}
	}
	return -1;
}

// FUNCTION: AARCADE 0017:059F
int FAR PASCAL OpenWaveOut(WAVE *p, HGLOBAL h, int n)
{
	HWAVEOUT hwo;
	LPWAVEHDR lpHdr;
	WAVECTX FAR *lpCtx;
	LPWAVEFORMAT lpFmt;
	DWORD cb;
	UINT err;
	LPSTR lpData;
	HGLOBAL hHdr;
	HGLOBAL hCtx;

	if (!ParseWave(h, &lpFmt, &lpData, &cb)
	    || waveOutOpen(&hwo, WAVE_MAPPER, lpFmt, 0L, 0L, 0L))
		return -1;
	hHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(WAVEHDR));
	if (hHdr == 0)
		return -1;
	lpHdr = (LPWAVEHDR)GlobalLock(hHdr);
	if (lpHdr == 0) {
		GlobalFree(hHdr);
		return -1;
	}
	hCtx = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(WAVECTX));
	if (hCtx == 0) {
		GlobalUnlock(hHdr);
		GlobalFree(hHdr);
		return -1;
	}
	lpCtx = (WAVECTX FAR *)GlobalLock(hCtx);
	if (lpCtx == 0) {
		GlobalUnlock(hHdr);
		GlobalFree(hHdr);
		GlobalFree(hCtx);
		return -1;
	}
	lpCtx->hwo = hwo;
	lpCtx->hCtx = hCtx;
	lpCtx->hHdr = hHdr;
	lpHdr->lpData = lpData;
	lpHdr->dwBufferLength = cb;
	lpHdr->dwFlags = 0;
	lpHdr->dwLoops = 0;
	lpHdr->dwUser = (DWORD)lpCtx;
	if (waveOutPrepareHeader(hwo, lpHdr, sizeof(WAVEHDR))) {
		GlobalUnlock(hHdr);
		GlobalFree(hHdr);
		GlobalUnlock(hCtx);
		GlobalFree(hCtx);
		return -1;
	}
	err = waveOutWrite(hwo, lpHdr, sizeof(WAVEHDR));
	if (err) {
		waveOutUnprepareHeader(hwo, lpHdr, sizeof(WAVEHDR));
		return -1;
	}
	p->lpCtx = lpCtx;
	p->lpHdr = lpHdr;
	return 0;
}

/* Checks that a WAVE resource is PCM the wave mapper can play and finds its
   format and data chunks. */
// FUNCTION: AARCADE 0017:073C
BOOL FAR PASCAL ParseWave(HGLOBAL h, LPWAVEFORMAT *plpFmt, LPSTR *plpData, DWORD *pcb)
{
	HWAVEOUT hwo;
	LPWAVEFORMAT lpFmt;
	DWORD cbData;
	DWORD cb;
	LPSTR lp;
	LPSTR lpData;
	BOOL fOk;
	MMIOINFO mmi;
	MMCKINFO ckRiff;
	MMCKINFO ck;
	HMMIO hmmio;

	fOk = 0;
	cb = GlobalSize(h);
	lp = GLock(h);
	if (lp == 0 || cb == 0)
		goto done;
	memset(&mmi, 0, sizeof(mmi));
	mmi.pIOProc = 0;
	mmi.fccIOProc = FOURCC_MEM;
	mmi.pchBuffer = lp;
	mmi.cchBuffer = cb;
	mmi.adwInfo[0] = 0;
	if ((hmmio = mmioOpen(0, &mmi, MMIO_READWRITE)) == 0)
		goto done;
	ckRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	if (mmioDescend(hmmio, &ckRiff, 0, MMIO_FINDRIFF))
		goto close;
	ck.ckid = mmioFOURCC('f', 'm', 't', ' ');
	if (mmioDescend(hmmio, &ck, &ckRiff, MMIO_FINDCHUNK))
		goto close;
	lpFmt = (LPWAVEFORMAT)((DWORD)lp + ck.dwDataOffset);
	if (lpFmt->wFormatTag != WAVE_FORMAT_PCM)
		goto close;
	if (waveOutOpen(&hwo, WAVE_MAPPER, lpFmt, 0L, 0L, WAVE_FORMAT_QUERY))
		goto close;
	mmioAscend(hmmio, &ck, 0);
	ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
	if (mmioDescend(hmmio, &ck, &ckRiff, MMIO_FINDCHUNK))
		goto close;
	cbData = ck.cksize;
	if (cbData == 0)
		goto close;
	lpData = (LPSTR)((DWORD)lp + ck.dwDataOffset);
	if (plpFmt)
		*plpFmt = lpFmt;
	if (plpData)
		*plpData = lpData;
	if (pcb)
		*pcb = cbData;
	fOk = 1;
close:
	mmioClose(hmmio, 0);
done:
	GUnlock(h);
	return fOk;
}
