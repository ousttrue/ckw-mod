#ifndef MAIN_H
#define MAIN_H

#include <windows.h>

extern DWORD	gLineSpace;
extern HBITMAP gBgBmp;
extern HBRUSH	gBgBrush;
extern CONSOLE_SCREEN_BUFFER_INFO* gCSI;

// console
extern HANDLE	gStdIn;	
extern HANDLE	gStdOut;
extern HANDLE	gStdErr;
extern HWND	gConWnd;

extern HANDLE	gChild;

extern RECT	gFrame;		/* window frame size */
extern HBITMAP gBgBmp;
extern HBRUSH	gBgBrush;
extern DWORD	gBorderSize;
extern DWORD	gLineSpace;
extern BOOL	gVScrollHide;

extern BOOL	gImeOn;

#endif
