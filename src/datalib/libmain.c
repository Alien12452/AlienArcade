/* LIBMAIN.C — entry for AACOLOR.DLL and AABW.DLL.
 *
 * Only LibMain lives here. __WEP comes from Borland CWC.LIB (12-byte
 * return-1 stub). UnlockData is omitted. Built twice: -ms for AABW
 * (C0DS), -mc for AACOLOR (C0DC).
 */
#include <windows.h>

// FUNCTION: AACOLOR 0001:017A LIBMAIN
// FUNCTION: AABW 0001:018C LIBMAIN
#pragma argsused
int FAR PASCAL LibMain(HINSTANCE hInstance, WORD wDataSegment,
                       WORD wHeapSize, LPSTR lpszCmdLine)
{
	return 1;
}
