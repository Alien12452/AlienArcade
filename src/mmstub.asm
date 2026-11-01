; Run-time binding of MMSYSTEM.DLL, so the game still loads under Windows 3.0
; without the multimedia extensions. Each API name is a thunk that jumps
; through its table entry. An entry starts out pointing at MmLoad, which binds
; it with GetModuleHandle/GetProcAddress on the first call. If MMSYSTEM is not
; loaded, the entry is bound to MmFail instead, which drops the arguments and
; returns the entry's default (or, with cbArgs -1, to the default address).

		.286

		EXTRN	GETMODULEHANDLE:FAR, GETPROCADDRESS:FAR

DGROUP		GROUP	_DATA

_DATA		SEGMENT WORD PUBLIC 'DATA'

		PUBLIC	MmModule
		PUBLIC	pmmsystemGetVersion
		PUBLIC	pwaveOutGetNumDevs
		PUBLIC	pwaveOutGetDevCaps
		PUBLIC	pwaveOutGetErrorText
		PUBLIC	pwaveOutOpen
		PUBLIC	pwaveOutClose
		PUBLIC	pwaveOutPrepareHeader
		PUBLIC	pwaveOutUnprepareHeader
		PUBLIC	pwaveOutWrite
		PUBLIC	pwaveOutPause
		PUBLIC	pwaveOutRestart
		PUBLIC	pwaveOutReset
		PUBLIC	pwaveOutGetPosition
		PUBLIC	pwaveOutGetPitch
		PUBLIC	pwaveOutSetPitch
		PUBLIC	pwaveOutGetVolume
		PUBLIC	pwaveOutSetVolume
		PUBLIC	pwaveOutGetPlaybackRate
		PUBLIC	pwaveOutSetPlaybackRate
		PUBLIC	pwaveOutBreakLoop
		PUBLIC	pwaveOutGetID
		PUBLIC	pmmioOpen
		PUBLIC	pmmioClose
		PUBLIC	pmmioRead
		PUBLIC	pmmioSeek
		PUBLIC	pmmioWrite
		PUBLIC	pmmioGetInfo
		PUBLIC	pmmioSetInfo
		PUBLIC	pmmioSetBuffer
		PUBLIC	pmmioFlush
		PUBLIC	pmmioAdvance
		PUBLIC	pmmioStringToFOURCC
		PUBLIC	pmmioInstallIOProc
		PUBLIC	pmmioSendMessage
		PUBLIC	pmmioDescend
		PUBLIC	pmmioAscend
		PUBLIC	pmmioCreateChunk

; struct { HMODULE hModule; char szName[]; }, hModule -1 until looked up
; GLOBAL: AARCADE 0024:02C4 MmModule
MmModule	dw	-1
		db	'MMSYSTEM', 0

; struct { FARPROC pfn; WORD wOrdinal; WORD cbArgs; DWORD dwDefault; }
; GLOBAL: AARCADE 0024:02CF pmmsystemGetVersion
pmmsystemGetVersion dd	MmLoad
		dw	5, 0
		dd	0
; GLOBAL: AARCADE 0024:02DB pwaveOutGetNumDevs
pwaveOutGetNumDevs dd	MmLoad
		dw	401, 0
		dd	0
; GLOBAL: AARCADE 0024:02E7 pwaveOutGetDevCaps
pwaveOutGetDevCaps dd	MmLoad
		dw	402, 8
		dd	1
; GLOBAL: AARCADE 0024:02F3 pwaveOutGetErrorText
pwaveOutGetErrorText dd	MmLoad
		dw	403, 8
		dd	1
; GLOBAL: AARCADE 0024:02FF pwaveOutOpen
pwaveOutOpen	dd	MmLoad
		dw	404, 22
		dd	1
; GLOBAL: AARCADE 0024:030B pwaveOutClose
pwaveOutClose	dd	MmLoad
		dw	405, 2
		dd	1
; GLOBAL: AARCADE 0024:0317 pwaveOutPrepareHeader
pwaveOutPrepareHeader dd	MmLoad
		dw	406, 8
		dd	1
; GLOBAL: AARCADE 0024:0323 pwaveOutUnprepareHeader
pwaveOutUnprepareHeader dd	MmLoad
		dw	407, 8
		dd	1
; GLOBAL: AARCADE 0024:032F pwaveOutWrite
pwaveOutWrite	dd	MmLoad
		dw	408, 8
		dd	1
; GLOBAL: AARCADE 0024:033B pwaveOutPause
pwaveOutPause	dd	MmLoad
		dw	409, 2
		dd	1
; GLOBAL: AARCADE 0024:0347 pwaveOutRestart
pwaveOutRestart	dd	MmLoad
		dw	410, 2
		dd	1
; GLOBAL: AARCADE 0024:0353 pwaveOutReset
pwaveOutReset	dd	MmLoad
		dw	411, 2
		dd	1
; GLOBAL: AARCADE 0024:035F pwaveOutGetPosition
pwaveOutGetPosition dd	MmLoad
		dw	412, 8
		dd	1
; GLOBAL: AARCADE 0024:036B pwaveOutGetPitch
pwaveOutGetPitch dd	MmLoad
		dw	413, 6
		dd	1
; GLOBAL: AARCADE 0024:0377 pwaveOutSetPitch
pwaveOutSetPitch dd	MmLoad
		dw	414, 6
		dd	1
; GLOBAL: AARCADE 0024:0383 pwaveOutGetVolume
pwaveOutGetVolume dd	MmLoad
		dw	415, 6
		dd	1
; GLOBAL: AARCADE 0024:038F pwaveOutSetVolume
pwaveOutSetVolume dd	MmLoad
		dw	416, 6
		dd	1
; GLOBAL: AARCADE 0024:039B pwaveOutGetPlaybackRate
pwaveOutGetPlaybackRate dd	MmLoad
		dw	417, 6
		dd	1
; GLOBAL: AARCADE 0024:03A7 pwaveOutSetPlaybackRate
pwaveOutSetPlaybackRate dd	MmLoad
		dw	418, 6
		dd	1
; GLOBAL: AARCADE 0024:03B3 pwaveOutBreakLoop
pwaveOutBreakLoop dd	MmLoad
		dw	419, 2
		dd	1
; GLOBAL: AARCADE 0024:03BF pwaveOutGetID
pwaveOutGetID	dd	MmLoad
		dw	420, 6
		dd	1
; GLOBAL: AARCADE 0024:03CB pmmioOpen
pmmioOpen	dd	MmLoad
		dw	1210, 12
		dd	1
; GLOBAL: AARCADE 0024:03D7 pmmioClose
pmmioClose	dd	MmLoad
		dw	1211, 4
		dd	1
; GLOBAL: AARCADE 0024:03E3 pmmioRead
pmmioRead	dd	MmLoad
		dw	1212, 10
		dd	1
; GLOBAL: AARCADE 0024:03EF pmmioSeek
pmmioSeek	dd	MmLoad
		dw	1214, 8
		dd	1
; GLOBAL: AARCADE 0024:03FB pmmioWrite
pmmioWrite	dd	MmLoad
		dw	1213, 10
		dd	1
; GLOBAL: AARCADE 0024:0407 pmmioGetInfo
pmmioGetInfo	dd	MmLoad
		dw	1215, 8
		dd	1
; GLOBAL: AARCADE 0024:0413 pmmioSetInfo
pmmioSetInfo	dd	MmLoad
		dw	1216, 8
		dd	1
; GLOBAL: AARCADE 0024:041F pmmioSetBuffer
pmmioSetBuffer	dd	MmLoad
		dw	1217, 12
		dd	1
; GLOBAL: AARCADE 0024:042B pmmioFlush
pmmioFlush	dd	MmLoad
		dw	1218, 4
		dd	1
; GLOBAL: AARCADE 0024:0437 pmmioAdvance
pmmioAdvance	dd	MmLoad
		dw	1219, 8
		dd	1
; GLOBAL: AARCADE 0024:0443 pmmioStringToFOURCC
pmmioStringToFOURCC dd	MmLoad
		dw	1220, 6
		dd	1
; GLOBAL: AARCADE 0024:044F pmmioInstallIOProc
pmmioInstallIOProc dd	MmLoad
		dw	1221, 12
		dd	1
; GLOBAL: AARCADE 0024:045B pmmioSendMessage
pmmioSendMessage dd	MmLoad
		dw	1222, 12
		dd	1
; GLOBAL: AARCADE 0024:0467 pmmioDescend
pmmioDescend	dd	MmLoad
		dw	1223, 12
		dd	1
; GLOBAL: AARCADE 0024:0473 pmmioAscend
pmmioAscend	dd	MmLoad
		dw	1224, 8
		dd	1
; GLOBAL: AARCADE 0024:047F pmmioCreateChunk
pmmioCreateChunk dd	MmLoad
		dw	1225, 8
		dd	1

_DATA		ENDS

_TEXT		SEGMENT WORD PUBLIC 'CODE'
		ASSUME	CS:_TEXT, DS:DGROUP

		PUBLIC	MmFail, MmBind, MmLoad
		PUBLIC	MMSYSTEMGETVERSION
		PUBLIC	WAVEOUTGETNUMDEVS
		PUBLIC	WAVEOUTGETDEVCAPS
		PUBLIC	WAVEOUTGETERRORTEXT
		PUBLIC	WAVEOUTOPEN
		PUBLIC	WAVEOUTCLOSE
		PUBLIC	WAVEOUTPREPAREHEADER
		PUBLIC	WAVEOUTUNPREPAREHEADER
		PUBLIC	WAVEOUTWRITE
		PUBLIC	WAVEOUTPAUSE
		PUBLIC	WAVEOUTRESTART
		PUBLIC	WAVEOUTRESET
		PUBLIC	WAVEOUTGETPOSITION
		PUBLIC	WAVEOUTGETPITCH
		PUBLIC	WAVEOUTSETPITCH
		PUBLIC	WAVEOUTGETVOLUME
		PUBLIC	WAVEOUTSETVOLUME
		PUBLIC	WAVEOUTGETPLAYBACKRATE
		PUBLIC	WAVEOUTSETPLAYBACKRATE
		PUBLIC	WAVEOUTBREAKLOOP
		PUBLIC	WAVEOUTGETID
		PUBLIC	MMIOOPEN
		PUBLIC	MMIOCLOSE
		PUBLIC	MMIOREAD
		PUBLIC	MMIOSEEK
		PUBLIC	MMIOWRITE
		PUBLIC	MMIOGETINFO
		PUBLIC	MMIOSETINFO
		PUBLIC	MMIOSETBUFFER
		PUBLIC	MMIOFLUSH
		PUBLIC	MMIOADVANCE
		PUBLIC	MMIOSTRINGTOFOURCC
		PUBLIC	MMIOINSTALLIOPROC
		PUBLIC	MMIOSENDMESSAGE
		PUBLIC	MMIODESCEND
		PUBLIC	MMIOASCEND
		PUBLIC	MMIOCREATECHUNK

; FUNCTION: AARCADE 0001:0176 MmFail
MmFail		PROC	FAR
		pop	dx
		pop	ax
		add	sp, [bx + 6]
		push	ax
		push	dx
		mov	ax, [bx + 8]
		mov	dx, [bx + 0Ah]
		ret
MmFail		ENDP

; AX = module, BX = entry
; FUNCTION: AARCADE 0001:0184 MmBind
MmBind:		pusha
		mov	si, ax
		mov	di, bx
		mov	ax, [si]
		or	ax, ax
		jz	BindFailed
		cmp	ax, -1
		jne	HaveModule
		lea	ax, [si + 2]
		push	ds
		push	ax
		call	GETMODULEHANDLE
		mov	[si], ax
		or	ax, ax
		jz	BindFailed
HaveModule:	push	ax
		push	0
		push	word ptr [di + 4]
		call	GETPROCADDRESS
		or	dx, dx
		jz	BindFailed
BindDone:		mov	[di], ax
		mov	[di + 2], dx
		popa
		jmp	dword ptr [bx]
BindFailed:	mov	ax, offset MmFail
		mov	dx, cs
		cmp	word ptr [di + 6], -1
		jne	BindDone
		mov	ax, [di + 8]
		mov	dx, [di + 0Ah]
		jmp	BindDone

; FUNCTION: AARCADE 0001:01CE MmLoad
MmLoad:		mov	ax, offset DGROUP:MmModule
		jmp	MmBind

; FUNCTION: AARCADE 0001:01D3 MMSYSTEMGETVERSION
MMSYSTEMGETVERSION:
		mov	bx, offset DGROUP:pmmsystemGetVersion
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:01D8 WAVEOUTGETNUMDEVS
WAVEOUTGETNUMDEVS:
		mov	bx, offset DGROUP:pwaveOutGetNumDevs
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:01DD WAVEOUTGETDEVCAPS
WAVEOUTGETDEVCAPS:
		mov	bx, offset DGROUP:pwaveOutGetDevCaps
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:01E2 WAVEOUTGETERRORTEXT
WAVEOUTGETERRORTEXT:
		mov	bx, offset DGROUP:pwaveOutGetErrorText
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:01E7 WAVEOUTOPEN
WAVEOUTOPEN:
		mov	bx, offset DGROUP:pwaveOutOpen
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:01EC WAVEOUTCLOSE
WAVEOUTCLOSE:
		mov	bx, offset DGROUP:pwaveOutClose
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:01F1 WAVEOUTPREPAREHEADER
WAVEOUTPREPAREHEADER:
		mov	bx, offset DGROUP:pwaveOutPrepareHeader
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:01F6 WAVEOUTUNPREPAREHEADER
WAVEOUTUNPREPAREHEADER:
		mov	bx, offset DGROUP:pwaveOutUnprepareHeader
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:01FB WAVEOUTWRITE
WAVEOUTWRITE:
		mov	bx, offset DGROUP:pwaveOutWrite
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0200 WAVEOUTPAUSE
WAVEOUTPAUSE:
		mov	bx, offset DGROUP:pwaveOutPause
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0205 WAVEOUTRESTART
WAVEOUTRESTART:
		mov	bx, offset DGROUP:pwaveOutRestart
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:020A WAVEOUTRESET
WAVEOUTRESET:
		mov	bx, offset DGROUP:pwaveOutReset
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:020F WAVEOUTGETPOSITION
WAVEOUTGETPOSITION:
		mov	bx, offset DGROUP:pwaveOutGetPosition
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0214 WAVEOUTGETPITCH
WAVEOUTGETPITCH:
		mov	bx, offset DGROUP:pwaveOutGetPitch
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0219 WAVEOUTSETPITCH
WAVEOUTSETPITCH:
		mov	bx, offset DGROUP:pwaveOutSetPitch
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:021E WAVEOUTGETVOLUME
WAVEOUTGETVOLUME:
		mov	bx, offset DGROUP:pwaveOutGetVolume
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0223 WAVEOUTSETVOLUME
WAVEOUTSETVOLUME:
		mov	bx, offset DGROUP:pwaveOutSetVolume
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0228 WAVEOUTGETPLAYBACKRATE
WAVEOUTGETPLAYBACKRATE:
		mov	bx, offset DGROUP:pwaveOutGetPlaybackRate
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:022D WAVEOUTSETPLAYBACKRATE
WAVEOUTSETPLAYBACKRATE:
		mov	bx, offset DGROUP:pwaveOutSetPlaybackRate
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0232 WAVEOUTBREAKLOOP
WAVEOUTBREAKLOOP:
		mov	bx, offset DGROUP:pwaveOutBreakLoop
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0237 WAVEOUTGETID
WAVEOUTGETID:
		mov	bx, offset DGROUP:pwaveOutGetID
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:023C MMIOOPEN
MMIOOPEN:
		mov	bx, offset DGROUP:pmmioOpen
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0241 MMIOCLOSE
MMIOCLOSE:
		mov	bx, offset DGROUP:pmmioClose
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0246 MMIOREAD
MMIOREAD:
		mov	bx, offset DGROUP:pmmioRead
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:024B MMIOSEEK
MMIOSEEK:
		mov	bx, offset DGROUP:pmmioSeek
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0250 MMIOWRITE
MMIOWRITE:
		mov	bx, offset DGROUP:pmmioWrite
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0255 MMIOGETINFO
MMIOGETINFO:
		mov	bx, offset DGROUP:pmmioGetInfo
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:025A MMIOSETINFO
MMIOSETINFO:
		mov	bx, offset DGROUP:pmmioSetInfo
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:025F MMIOSETBUFFER
MMIOSETBUFFER:
		mov	bx, offset DGROUP:pmmioSetBuffer
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0264 MMIOFLUSH
MMIOFLUSH:
		mov	bx, offset DGROUP:pmmioFlush
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0269 MMIOADVANCE
MMIOADVANCE:
		mov	bx, offset DGROUP:pmmioAdvance
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:026E MMIOSTRINGTOFOURCC
MMIOSTRINGTOFOURCC:
		mov	bx, offset DGROUP:pmmioStringToFOURCC
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0273 MMIOINSTALLIOPROC
MMIOINSTALLIOPROC:
		mov	bx, offset DGROUP:pmmioInstallIOProc
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0278 MMIOSENDMESSAGE
MMIOSENDMESSAGE:
		mov	bx, offset DGROUP:pmmioSendMessage
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:027D MMIODESCEND
MMIODESCEND:
		mov	bx, offset DGROUP:pmmioDescend
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0282 MMIOASCEND
MMIOASCEND:
		mov	bx, offset DGROUP:pmmioAscend
		jmp	dword ptr [bx]

; FUNCTION: AARCADE 0001:0287 MMIOCREATECHUNK
MMIOCREATECHUNK:
		mov	bx, offset DGROUP:pmmioCreateChunk
		jmp	dword ptr [bx]

_TEXT		ENDS
		END
