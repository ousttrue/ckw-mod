#ifndef MAIN_H
#define MAIN_H

#include <windows.h>

extern DWORD	gLineSpace;
extern HBITMAP gBgBmp;
extern HBRUSH	gBgBrush;

// console
extern HANDLE	gStdIn;	
extern HANDLE	gStdOut;
extern HANDLE	gStdErr;
extern HWND	gConWnd;

extern HANDLE	gChild;

extern RECT	gFrame;		/* window frame size */
extern HBITMAP gBgBmp;
extern HBRUSH	gBgBrush;
extern DWORD	gLineSpace;
extern BOOL	gVScrollHide;


#endif
