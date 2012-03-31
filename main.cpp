/*-----------------------------------------------------------------------------
 * File: main.cpp
 *-----------------------------------------------------------------------------
 * Copyright (c) 2005       Kazuo Ishii <k-ishii@wb4.so-net.ne.jp>
 *				- original version
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *---------------------------------------------------------------------------*/
#include "ckw.h"
#include "option.h"
#include "ime_wrap.h"
#include "app.h"
#include "rsrc.h"

/*****************************************************************************/

HANDLE	gChild = NULL;	/* child process */

// console
HANDLE	gStdIn=NULL;	
HANDLE	gStdOut=NULL;
HANDLE	gStdErr=NULL;
HWND	gConWnd=NULL;


RECT	gFrame;		/* window frame size */
HBITMAP gBgBmp = NULL;	/* background image */
HBRUSH	gBgBrush = NULL;/* background brush */
DWORD	gBorderSize = 0;/* internal border */
DWORD	gLineSpace = 0;	/* line space */
BOOL	gVScrollHide = FALSE;

BOOL	gImeOn = FALSE; /* IME-status */

/* screen buffer - copy */
CONSOLE_SCREEN_BUFFER_INFO* gCSI = NULL;
CHAR_INFO*	gScreen = NULL;
wchar_t*	gTitle = NULL;



BOOL WINAPI ReadConsoleOutput_Unicode(HANDLE con, CHAR_INFO* buffer,
				      COORD size, COORD pos, SMALL_RECT *sr)
{
	if(!ReadConsoleOutputA(con, buffer, size, pos, sr))
		return(FALSE);

	CHAR_INFO* s = buffer;
	CHAR_INFO* e = buffer + (size.X * size.Y);
	DWORD	codepage = GetConsoleOutputCP();
	BYTE	ch[2];
	WCHAR	wch;

	while(s < e) {
		ch[0] = s->Char.AsciiChar;

		if(s->Attributes & COMMON_LVB_LEADING_BYTE) {
			if((s+1) < e && ((s+1)->Attributes & COMMON_LVB_TRAILING_BYTE)) {
				ch[1] = (s+1)->Char.AsciiChar;
				if(MultiByteToWideChar(codepage, 0, (LPCSTR)ch, 2, &wch, 1)) {
					s->Char.UnicodeChar = wch;
					s++;
					s->Char.UnicodeChar = wch;
					s++;
					continue;
				}
			}
		}

		if(MultiByteToWideChar(codepage, 0, (LPCSTR)ch, 1, &wch, 1)) {
			s->Char.UnicodeChar = wch;
		}
		s->Attributes &= ~(COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE);
		s++;
	}
	return(TRUE);
}

/*****************************************************************************/
#include "option.h"

/*----------*/
/*----------*/


#ifdef _DEBUG
#include <crtdbg.h>
#endif

#ifdef _MSC_VER
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow)
#else
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
#endif
{
#ifdef _DEBUG
	char *a = new char[1];
	*a = 0x22;
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
		       _CRTDBG_LEAK_CHECK_DF |
		       /*_CRTDBG_CHECK_ALWAYS_DF |*/
		       _CRTDBG_DELAY_FREE_MEM_DF);
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportMode( _CRT_WARN,   _CRTDBG_MODE_FILE );
	_CrtSetReportMode( _CRT_ERROR,  _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
	_CrtSetReportFile( _CRT_WARN,   _CRTDBG_FILE_STDERR );
	_CrtSetReportFile( _CRT_ERROR,  _CRTDBG_FILE_STDERR );
#endif

    App app;
    if(!app.initialize()){
        // 初期化失敗
        return 1;
    }
    return app.start();
}

/* 新規ウインドウの作成 */
void makeNewWindow()
{
	LPWSTR cd = new TCHAR[MAX_PATH+1];
	GetCurrentDirectory(MAX_PATH, cd);

	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));
	if(CreateProcess(NULL, GetCommandLine(), NULL, NULL, FALSE, 0,
					   NULL, NULL, &si, &pi)){
		// 使用しないので，すぐにクローズしてよい
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

/* EOF */
