#ifndef __CKW_H__
#define __CKW_H__ 1

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#define _UNICODE 1
#define  UNICODE 1
#include <windows.h>
#include <wchar.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
// for gcc
#include <stdio.h>
#define wcscat_s wcscat
#define sprintf_s sprintf
#define sscanf_s sscanf
#define strcpy_s strcpy
#define strcat_s strcat
#define _splitpath_s _splitpath
#define _makepath_s _makepath
#endif

#ifndef COMMON_LVB_LEADING_BYTE
#define COMMON_LVB_LEADING_BYTE  0x0100
#endif
#ifndef COMMON_LVB_TRAILING_BYTE
#define COMMON_LVB_TRAILING_BYTE 0x0200
#endif

#define CSI_WndCols(csi) ((csi)->srWindow.Right - (csi)->srWindow.Left +1)
#define CSI_WndRows(csi) ((csi)->srWindow.Bottom - (csi)->srWindow.Top +1)


#define SAFE_CloseHandle(handle) \
	if(handle) { CloseHandle(handle); handle = NULL; }

#define SAFE_DeleteObject(handle) \
	if(handle) { DeleteObject(handle); handle = NULL; }


#if 0
#include <stdio.h>
void trace(const char *msg)
{
	fputs(msg, stdout);
	fflush(stdout);
}
#else
#define trace(msg)
#endif

#include <stdio.h>
inline void DebugPrintf(const char *format, ...)
{
	va_list ap;
    va_start(ap, format);
    char allocatedBuffer[1024];
    vsprintf(allocatedBuffer, format, ap);
    va_end(ap);

	OutputDebugStringA(allocatedBuffer);
}

#endif /* __CKW_H__ */
