#include "console.h"
#include "ckw.h"
#include "option.h"
#include <vector>

// for Windows SDK v7.0 エラーが発生する場合はコメントアウト。
#ifdef _MSC_VER
#include <winternl.h>
#else
// for gcc
#include <ddk/ntapi.h>
#define STARTF_TITLEISLINKNAME 0x00000800 
#endif


static const wchar_t WORD_BREAK_CHARS[] = {
	L' ',  L'\t', L'\"', L'&', L'\'', L'(', L')', L'*',
	L',',  L';',  L'<',  L'=', L'>',  L'?', L'@', L'[',
	L'\\', L']',  L'^',  L'`', L'{',  L'}', L'~',
	0x3000,
	0x3001,
	0x3002,
	/**/
	0,
};


// console
#define SCRN_InvalidArea(x,y) \
	(y < gCSI.srWindow.Top    ||	\
	 y > gCSI.srWindow.Bottom ||	\
	 x < gCSI.srWindow.Left   ||	\
	 x > gCSI.srWindow.Right)

#define SELECT_GetScrn(x,y) \
	(gScreen + CSI_WndCols(&gCSI) * (y - gCSI.srWindow.Top) + x)

/* setConsoleFont */
#define MAX_FONTS 128

typedef struct _CONSOLE_FONT {
  DWORD index;
  COORD dim;
} CONSOLE_FONT, *PCONSOLE_FONT;

typedef BOOL  (WINAPI *GetConsoleFontInfoT)( HANDLE,BOOL,DWORD,PCONSOLE_FONT );
typedef DWORD (WINAPI *GetNumberOfConsoleFontsT)( VOID );
typedef BOOL  (WINAPI *SetConsoleFontT)( HANDLE, DWORD );

GetConsoleFontInfoT		GetConsoleFontInfo;
GetNumberOfConsoleFontsT	GetNumberOfConsoleFonts;
SetConsoleFontT			SetConsoleFont;

/* (craftware) */
static void copyStringToClipboard( HWND hWnd, const wchar_t * str )
{
	size_t length = wcslen(str) +1;
	HANDLE hMem;
	wchar_t* ptr;
	bool	result = true;

	hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * length);
	if(!hMem) result = false;

	if(result && !(ptr = (wchar_t*) GlobalLock(hMem))) {
		result = false;
	}
	if(result) {
		memcpy(ptr, str, sizeof(wchar_t) * length);
		GlobalUnlock(hMem);
	}
	if(result && !OpenClipboard(hWnd)) {
		Sleep(10);
		if(!OpenClipboard(hWnd))
			result = false;
	}
	if(result) {
		if(!EmptyClipboard() ||
		   !SetClipboardData(CF_UNICODETEXT, hMem))
			result = false;
		CloseClipboard();
	}
	if(!result && hMem) {
		GlobalFree(hMem);
	}
}

static BOOL WINAPI sig_handler(DWORD n)
{
	return(TRUE);
}

static BOOL WINAPI ReadConsoleOutput_Unicode(HANDLE con, CHAR_INFO* buffer,
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


//////////////////////////////////////////////////////////////////////////////
// Console
//////////////////////////////////////////////////////////////////////////////
Console::Console()
    :
        gStdIn(NULL),	
        gStdOut(NULL),
        gStdErr(NULL),
        gConWnd(NULL),
        gChild(NULL),
        gWinW(0),
        gWinH(0),
        gScreen(NULL)
{
    ZeroMemory(&gCSI, sizeof(gCSI));
    gSelectMode = 0;
    gSelectPos.X = -1;
    gSelectPos.Y = -1;
	gSelectRect.Left = gSelectRect.Right = -1;
	gSelectRect.Top  = gSelectRect.Bottom = -1;
}

Console::~Console()
{
    if(gScreen) {
        delete [] gScreen;
        gScreen = NULL;
    }
    SAFE_CloseHandle(gChild);
    gConWnd = NULL;
    SAFE_CloseHandle(gStdIn);
    SAFE_CloseHandle(gStdOut);
    SAFE_CloseHandle(gStdErr);
}

void Console::onDestroy()
{
    if(WaitForSingleObject(gChild, 0) == WAIT_TIMEOUT)
        TerminateProcess(gChild, 0);
}

void Console::onLBtnDown(HWND hWnd, int x, int y, const COORD &pos)
{
	static DWORD	prev_time = 0;
	static int	prevX = -100;
	static int	prevY = -100;

	{
		/* calc click count */
		DWORD now_time = GetTickCount();
		DWORD stime;
		if(prev_time > now_time)
			stime = now_time + ~prev_time+1;
		else
			stime = now_time - prev_time;
		if(stime <= GetDoubleClickTime()) {
			int sx = (prevX > x) ? prevX-x : x-prevX;
			int sy = (prevY > y) ? prevY-y : y-prevY;
			if(sx <= GetSystemMetrics(SM_CXDOUBLECLK) &&
			   sy <= GetSystemMetrics(SM_CYDOUBLECLK)) {
				if(++gSelectMode > 2)
					gSelectMode = 0;
			}
			else {
				gSelectMode = 0;
			}
		}
		else {
			gSelectMode = 0;
		}
		prev_time = now_time;
		prevX = x;
		prevY = y;
	}

	if(!gScreen)
		return;

	SetCapture(hWnd);

	gSelectPos.X = pos.X;
	gSelectPos.Y = pos.Y;
	gSelectRect.Left = gSelectRect.Right = pos.X;
	gSelectRect.Top  = gSelectRect.Bottom = pos.Y;

	__select_expand();
	InvalidateRect(hWnd, NULL, FALSE);
}

void Console::onLBtnUp(HWND hWnd, int x, int y)
{
	if(!gScreen)
		return;

	wchar_t* str = selectionGetString();
	if(!str) return;

	copyStringToClipboard( hWnd, str );

	delete [] str;
}

/* (craftware) */
void Console::copyAllStringToClipboard(HWND hWnd)
{
	wchar_t* str = getAllString();
	if(!str) return;
	
	std::wstring s = str;

	// skip empty line
	size_t begin = s.find_first_not_of(L"\r\n");
	size_t end = s.find_last_not_of(L"\r\n");
	if(begin!=s.npos && end!=s.npos)
	{
		s = s.substr( begin, end+1-begin );
	}

	copyStringToClipboard( hWnd, s.c_str() );

	delete [] str;
}

void Console::onMouseMove(HWND hWnd, int x, int y, const COORD &pos)
{
	if(hWnd != GetCapture()){
		return;
    }

	if(!gScreen){
		return;
    }

	SMALL_RECT bak = gSelectRect;

	if(pos.Y < gSelectPos.Y || (pos.Y == gSelectPos.Y && pos.X < gSelectPos.X)) {
		gSelectRect.Left   = pos.X;
		gSelectRect.Top    = pos.Y;
		gSelectRect.Right  = gSelectPos.X;
		gSelectRect.Bottom = gSelectPos.Y;
	}
	else {
		gSelectRect.Left   = gSelectPos.X;
		gSelectRect.Top    = gSelectPos.Y;
		gSelectRect.Right  = pos.X;
		gSelectRect.Bottom = pos.Y;
	}

	__select_expand();

	if(memcmp(&bak, &gSelectRect, sizeof(bak))) {
		InvalidateRect(hWnd, NULL, FALSE);
	}

    RECT rc;
    GetClientRect(hWnd, &rc);

    if( y<0 ) {
        ::PostMessage(gConWnd, WM_MOUSEWHEEL, WHEEL_DELTA<<16, y<<16|x );
    }
    else if(y>=rc.bottom) {
        ::PostMessage(gConWnd, WM_MOUSEWHEEL, -WHEEL_DELTA<<16, y<<16|x );
    }
}

// window resizeが必要な場合にtrueを返す
bool Console::onTimer(HWND hWnd, bool gVScrollHide, int *w, int *h)
{
	if(WaitForSingleObject(gChild, 0) != WAIT_TIMEOUT) {
		::PostMessage(hWnd, WM_CLOSE, 0,0);
		return false;
	}

    // refresh handle
    if(gStdOut) CloseHandle(gStdOut);
    gStdOut = CreateFile(L"CONOUT$", GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL);

	CONSOLE_SCREEN_BUFFER_INFO csi=GetConsoleScreenBufferInfo();
	COORD	size;
	size.X = CSI_WndCols(&csi);
	size.Y = CSI_WndRows(&csi);

	/* copy screen buffer */
	DWORD      nb = size.X * size.Y;
	CHAR_INFO* buffer = new CHAR_INFO[nb];
	CHAR_INFO* ptr = buffer;
	SMALL_RECT sr;
	COORD      pos = { 0, 0 };

	/* ReadConsoleOuput - maximum read size 64kByte?? */
	size.Y = 0x8000 / sizeof(CHAR_INFO) / size.X;
	sr.Left  = csi.srWindow.Left;
	sr.Right = csi.srWindow.Right;
	sr.Top   = csi.srWindow.Top;
	do {
		sr.Bottom = sr.Top + size.Y -1;
		if(sr.Bottom > csi.srWindow.Bottom) {
			sr.Bottom = csi.srWindow.Bottom;
			size.Y = sr.Bottom - sr.Top +1;
		}
		ReadConsoleOutput_Unicode(ptr, size, pos, &sr);
		ptr += size.X * size.Y;
		sr.Top = sr.Bottom +1;
	} while(sr.Top <= csi.srWindow.Bottom);

	/* compare */
	if(gScreen &&
	   !memcmp(&csi, &gCSI, sizeof(CONSOLE_SCREEN_BUFFER_INFO)) &&
	   !memcmp(buffer, gScreen, sizeof(CHAR_INFO) * nb)) {
		/* no modified */
		delete [] buffer;
		return false;
	}

	/* swap buffer */
	if(gScreen) delete [] gScreen;
	gScreen = buffer;
	gCSI = csi;

	/* redraw request */
	InvalidateRect(hWnd, NULL, TRUE);

	/* set vertical scrollbar status */
	if(!gVScrollHide) {
		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_DISABLENOSCROLL | SIF_POS | SIF_PAGE | SIF_RANGE;
		si.nPos = gCSI.srWindow.Top;
		si.nPage = CSI_WndRows(&gCSI);
		si.nMin = 0;
		si.nMax = gCSI.dwSize.Y-1;
		SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
	}

	*w = CSI_WndCols(&gCSI);
	*h = CSI_WndRows(&gCSI);
	return gWinW != *w || gWinH != *h;
}


void Console::__write_console_input(LPCWSTR str, DWORD length)
{
	if(!str || !length) return;

	INPUT_RECORD *p, *buf;
	DWORD	i = 0;
	p = buf = new INPUT_RECORD[ length ];

	for( ; i < length ; i++, p++) {
		p->EventType = KEY_EVENT;
		p->Event.KeyEvent.bKeyDown = TRUE;
		p->Event.KeyEvent.wRepeatCount = 1;
		p->Event.KeyEvent.wVirtualKeyCode = 0;
		p->Event.KeyEvent.wVirtualScanCode = 0;
		p->Event.KeyEvent.uChar.UnicodeChar = 0;
		p->Event.KeyEvent.dwControlKeyState = 0;
		if(*str == '\r') {
			str++;
			length--;
		}
		if(*str == '\n') {
			p->Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
			str++;
		} else {
			p->Event.KeyEvent.uChar.UnicodeChar = *str++;
		}
	}

	WriteConsoleInput(gStdIn, buf, length, &length);
	delete [] buf;
}


BOOL Console::PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
    return ::PostMessage(gConWnd, Msg, wParam, lParam);
}

void Console::SetConsoleScreenBufferInfo(const CONSOLE_SCREEN_BUFFER_INFO &csi)
{
	::SetConsoleScreenBufferSize(gStdOut, csi.dwSize);
	::SetConsoleWindowInfo(gStdOut, TRUE, &csi.srWindow);
}

CONSOLE_SCREEN_BUFFER_INFO Console::GetConsoleScreenBufferInfo()
{

	CONSOLE_SCREEN_BUFFER_INFO csi;
	::GetConsoleScreenBufferInfo(gStdOut, &csi);

    return csi;
}

BOOL WINAPI Console::ReadConsoleOutput_Unicode(CHAR_INFO* buffer,
				      COORD size, COORD pos, SMALL_RECT *sr)
{
    return ::ReadConsoleOutput_Unicode(gStdOut, buffer, size, pos, sr);
}

static void copyChar(wchar_t*& p, CHAR_INFO* src, SHORT start, SHORT end, bool ret=true)
{
	CHAR_INFO* pend = src + end;
	CHAR_INFO* test = src + start;
	CHAR_INFO* last = test-1;

	/* search last char */
	for( ; test <= pend ; test++) {
		if(test->Char.UnicodeChar > 0x20)
			last = test;
	}
	/* copy */
	for(test = src+start ; test <= last ; test++) {
		if(!(test->Attributes & COMMON_LVB_TRAILING_BYTE))
			*p++ = test->Char.UnicodeChar;
	}
	if(ret && last < pend) {
		*p++ = L'\r';
		*p++ = L'\n';
	}
	*p = 0;
}

wchar_t* Console::ReadStdOut(int nb, 
        const CONSOLE_SCREEN_BUFFER_INFO &gCSI, const SMALL_RECT &gSelectRect)
{
	COORD      size = { (short)CSI_WndCols(&gCSI), (short)1 };
    std::vector<CHAR_INFO> work(size.X);
	COORD      pos = { 0,0 };
	SMALL_RECT sr = { gCSI.srWindow.Left, 0, gCSI.srWindow.Right, 0 };

	wchar_t*   buffer = new wchar_t[ nb +32 ];
	wchar_t*   wp = buffer;
	*wp = 0;
	if(gSelectRect.Top == gSelectRect.Bottom) {
		sr.Top = sr.Bottom = gSelectRect.Top;
		::ReadConsoleOutput_Unicode(gStdOut, &work[0], size, pos, &sr);
		copyChar(wp, &work[0], gSelectRect.Left, gSelectRect.Right-1, false);
	}
	else {
		sr.Top = sr.Bottom = gSelectRect.Top;
		::ReadConsoleOutput_Unicode(gStdOut, &work[0], size, pos, &sr);
		copyChar(wp, &work[0], gSelectRect.Left, gCSI.srWindow.Right);
		for(int y = gSelectRect.Top+1 ; y <= gSelectRect.Bottom-1 ; y++) {
			sr.Top = sr.Bottom = y;
			::ReadConsoleOutput_Unicode(gStdOut, &work[0], size, pos, &sr);
			copyChar(wp, &work[0], gCSI.srWindow.Left, gCSI.srWindow.Right);
		}
		sr.Top = sr.Bottom = gSelectRect.Bottom;
		::ReadConsoleOutput_Unicode(gStdOut, &work[0], size, pos, &sr);
		copyChar(wp, &work[0], gCSI.srWindow.Left, gSelectRect.Right-1, false);
	}

	return (buffer);
}

bool Console::create_child_process(const char* cmd, const char* curdir)
{
	trace("create_child_process\n");

	char* buf = NULL;

	if(!cmd || !cmd[0]) {
		buf = new char[32768];
		buf[0] = 0;
		if(!GetEnvironmentVariableA("COMSPEC", buf, 32768))
			strcpy(buf, "cmd.exe");
	}
	else {
		buf = new char[ strlen(cmd)+1 ];
		strcpy(buf, cmd);
	}

	PROCESS_INFORMATION pi;
	STARTUPINFOA si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput  = gStdIn;
	si.hStdOutput = gStdOut;
	si.hStdError  = gStdErr;

	if (curdir)
		if (char *p = strstr((char*)curdir, ":\""))
			*(p+1) = '\\';

	if(! CreateProcessA(NULL, buf, NULL, NULL, TRUE,
			    0, NULL, curdir, &si, &pi)) {
		delete [] buf;
		return(FALSE);
	}
	delete [] buf;
	CloseHandle(pi.hThread);
	gChild = pi.hProcess;
	return(TRUE);
}

bool __hide_alloc_console()
{

	/*
	 * Open Console Window
	 * hack StartupInfo.wShowWindow flag
	 */

#ifdef _MSC_VER
#ifdef _WIN64
	INT_PTR peb = *(INT_PTR*)((INT_PTR)NtCurrentTeb() + 0x60);
	INT_PTR param = *(INT_PTR*) (peb + 0x20);
	DWORD* pflags = (DWORD*) (param + 0xa4);
	WORD* pshow = (WORD*) (param + 0xa8); 
#else
#ifndef _WINTERNL_
	INT_PTR peb = *(INT_PTR*)((INT_PTR)NtCurrentTeb() + 0x30);
	INT_PTR param = *(INT_PTR*) (peb + 0x10);
#else
	// for Windows SDK v7.0
	PPEB peb = *(PPEB*)((INT_PTR)NtCurrentTeb() + 0x30);
	PRTL_USER_PROCESS_PARAMETERS param = peb->ProcessParameters;
#endif // _WINTERNL_

	DWORD* pflags = (DWORD*)((INT_PTR)param + 0x68);
	WORD* pshow = (WORD*)((INT_PTR)param + 0x6C);
#endif // _WIN64
#else
	// for gcc
	INT_PTR peb = *(INT_PTR*)((INT_PTR)NtCurrentTeb() + 0x30);
    PRTL_USER_PROCESS_PARAMETERS param = *(PRTL_USER_PROCESS_PARAMETERS*)(peb + 0x10);
	DWORD* pflags = (DWORD*)&(param->dwFlags);
	WORD* pshow = (WORD*)&(param->wShowWindow); 
#endif // _MSC_VER

	DWORD	backup_flags = *pflags;
	WORD	backup_show  = *pshow;

	STARTUPINFO si;
	GetStartupInfo(&si);

	/* check */
	bool bResult = false;
	if(si.dwFlags == backup_flags && si.wShowWindow == backup_show) {
		// 詳細は不明だがSTARTF_TITLEISLINKNAMEが立っていると、
		// Console窓隠しに失敗するので除去(Win7-64bit)
		if (*pflags & STARTF_TITLEISLINKNAME) {
			*pflags &= ~STARTF_TITLEISLINKNAME;
		}
		*pflags |= STARTF_USESHOWWINDOW;
		*pshow  = SW_HIDE;
		bResult = true;
	}

	AllocConsole();

	/* restore */
	*pflags = backup_flags;
	*pshow  = backup_show;

    return bResult;
}

bool Console::initialize(std::shared_ptr<ckOpt> opt)
{
    std::wstring title=L"ckw";

    std::string conf_title = opt->getTitle() ? opt->getTitle() : "";
    if(!conf_title.empty()){
        std::vector<wchar_t> buf(conf_title.size(), 0);
        MultiByteToWideChar(CP_ACP, 0, &conf_title[0], (int)conf_title.size(), 
                &title[0], (int)(sizeof(wchar_t) * title.size()) );
        title=std::wstring(buf.begin(), buf.end());
    }

    bool bResult=__hide_alloc_console();
	while((gConWnd = GetConsoleWindow()) == NULL) {
		Sleep(10);
	}

	if (!bResult){
		while (!IsWindowVisible(gConWnd)) {
			Sleep(10);
		}
		while(IsWindowVisible(gConWnd)) {
			ShowWindow(gConWnd, SW_HIDE);
			Sleep(10);
		}
	}

    SetConsoleTitle(title.c_str());

    SetConsoleCtrlHandler(sig_handler, TRUE);

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    gStdIn  = CreateFile(L"CONIN$",  GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            &sa, OPEN_EXISTING, 0, NULL);
    gStdOut = CreateFile(L"CONOUT$",  GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            &sa, OPEN_EXISTING, 0, NULL);
    gStdErr = CreateFile(L"CONOUT$",  GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE,
            &sa, OPEN_EXISTING, 0, NULL);

    if(!gConWnd || !gStdIn || !gStdOut || !gStdErr)
        return(FALSE);

    HINSTANCE hLib = LoadLibraryW( L"KERNEL32.DLL" );
    if (hLib == NULL)
        goto done;

#define GetProc( proc ) \
    do { \
        proc = (proc##T)GetProcAddress( hLib, #proc ); \
        if (proc == NULL) \
        goto freelib; \
    } while (0)
    GetProc( GetConsoleFontInfo );
    GetProc( GetNumberOfConsoleFonts );
    GetProc( SetConsoleFont );
#undef GetProc

    {
        CONSOLE_FONT font[MAX_FONTS];
        DWORD fonts = GetNumberOfConsoleFonts();
        if (fonts > MAX_FONTS){
            fonts = MAX_FONTS;
        }

        GetConsoleFontInfo(gStdOut, 0, fonts, font);
        CONSOLE_FONT minimalFont = { 0, {0, 0}};
        for(DWORD i=0;i<fonts;i++){
            if(minimalFont.dim.X < font[i].dim.X && minimalFont.dim.Y < font[i].dim.Y)
                minimalFont = font[i];
        }
        SetConsoleFont(gStdOut, minimalFont.index);
    }
freelib:
    FreeLibrary( hLib );
done:

    /* set buffer & window size */
    SMALL_RECT sr = {0,0,0,0};
    SetConsoleWindowInfo(gStdOut, TRUE, &sr);
    COORD size;
    size.X = opt->getWinCharW();
    size.Y = opt->getWinCharH() + opt->getSaveLines();
    SetConsoleScreenBufferSize(gStdOut, size);
    sr.Left = 0;
    sr.Right = opt->getWinCharW()-1;
    sr.Top = size.Y - opt->getWinCharH();
    sr.Bottom = size.Y-1;
    SetConsoleWindowInfo(gStdOut, TRUE, &sr);
    size.X = sr.Left;
    size.Y = sr.Top;
    SetConsoleCursorPosition(gStdOut, size);

    return true;
}

bool Console::__select_invalid()
{
	return ( gSelectRect.Top > gSelectRect.Bottom ||
	         (gSelectRect.Top == gSelectRect.Bottom &&
	         gSelectRect.Left >= gSelectRect.Right) );
}

void Console::__select_word_expand_left()
{
	if(SCRN_InvalidArea(gSelectRect.Left, gSelectRect.Top))
		return;
	CHAR_INFO* base  = SELECT_GetScrn(gSelectRect.Left, gSelectRect.Top);
	CHAR_INFO* ptr = base;
	int c = gSelectRect.Left;

	for( ; c >= gCSI.srWindow.Left ; c--, ptr--) {
		if(wcschr(WORD_BREAK_CHARS, ptr->Char.UnicodeChar)) {
			c++;
			break;
		}
	}
	if(c < 0)
		c = 0;

	if(gSelectRect.Left > c)
		gSelectRect.Left = c;
}

void Console::__select_word_expand_right()
{
	if(SCRN_InvalidArea(gSelectRect.Right, gSelectRect.Bottom))
		return;
	CHAR_INFO* base  = SELECT_GetScrn(gSelectRect.Right, gSelectRect.Bottom);
	CHAR_INFO* ptr = base;
	int c = gSelectRect.Right;

	for( ; c <= gCSI.srWindow.Right ; c++, ptr++) {
		if(wcschr(WORD_BREAK_CHARS, ptr->Char.UnicodeChar)) {
			break;
		}
	}

	if(gSelectRect.Right < c)
		gSelectRect.Right = c;
}

void Console::__select_char_expand()
{
	CHAR_INFO* base;

	if(SCRN_InvalidArea(gSelectRect.Left, gSelectRect.Top)) {
	}
	else if(gSelectRect.Left-1 >= gCSI.srWindow.Left) {
		base  = SELECT_GetScrn(gSelectRect.Left, gSelectRect.Top);
		if(base->Attributes & COMMON_LVB_TRAILING_BYTE)
			gSelectRect.Left--;
	}

	if(SCRN_InvalidArea(gSelectRect.Right, gSelectRect.Bottom)) {
	}
	else {
		base  = SELECT_GetScrn(gSelectRect.Right, gSelectRect.Bottom);
		if(base->Attributes & COMMON_LVB_TRAILING_BYTE)
			gSelectRect.Right++;
	}
}

void Console::__select_expand()
{
	if(gSelectMode == 0) {
		__select_char_expand();
	}
	else if(gSelectMode == 1) {
		__select_word_expand_left();
		__select_word_expand_right();
	}
	else if(gSelectMode == 2) {
		gSelectRect.Left = gCSI.srWindow.Left;
		gSelectRect.Right = gCSI.srWindow.Right+1;
	}
}

wchar_t* Console::selectionGetString()
{
	if( __select_invalid() ){
		return(NULL);
    }

	int nb;
	if(gSelectRect.Top == gSelectRect.Bottom) {
		nb = gSelectRect.Right - gSelectRect.Left;
	}
	else {
		nb = gCSI.srWindow.Right - gSelectRect.Left+1;
		for(int y = gSelectRect.Top+1 ; y <= gSelectRect.Bottom-1 ; y++){
			nb += CSI_WndCols(&gCSI);
        }
		nb += gSelectRect.Right - gCSI.srWindow.Left;
	}

    return ReadStdOut(nb, gCSI, gSelectRect);
}

/* (craftware) */
wchar_t * Console::getAllString()
{
	int nb = gCSI.dwSize.X * gCSI.dwSize.Y;

	COORD      size = { gCSI.dwSize.X, 1 };
	CHAR_INFO* work = new CHAR_INFO[ gCSI.dwSize.X ];
	wchar_t*   buffer = new wchar_t[ nb ];
	wchar_t*   wp = buffer;
	COORD      pos = { 0,0 };
	SMALL_RECT sr = { 0, 0, (short)(gCSI.dwSize.X-1), 0 };

	*wp = 0;

	for( int y=0 ; y<gCSI.dwSize.Y ; ++y )
	{
		sr.Top = sr.Bottom = y;
		ReadConsoleOutput_Unicode(work, size, pos, &sr);
		copyChar( wp, work, 0, gCSI.dwSize.X-1 );
	}

	delete [] work;

	return(buffer);
}

void Console::__set_console_window_size(LONG cols, LONG rows)
{
	gWinW = cols;
	gWinH = rows;

	if(cols == CSI_WndCols(&gCSI) && rows == CSI_WndRows(&gCSI)){
		return;
    }

	gCSI.dwSize.X = (SHORT)cols;
	gCSI.srWindow.Left = 0;
	gCSI.srWindow.Right = (SHORT)(cols -1);

	if(gCSI.dwSize.Y < rows || gCSI.dwSize.Y == CSI_WndRows(&gCSI))
		gCSI.dwSize.Y = (SHORT)rows;

	gCSI.srWindow.Bottom += (SHORT)(rows - CSI_WndRows(&gCSI));
	if(gCSI.dwSize.Y <= gCSI.srWindow.Bottom) {
		gCSI.srWindow.Top -= gCSI.srWindow.Bottom - gCSI.dwSize.Y +1;
		gCSI.srWindow.Bottom = gCSI.dwSize.Y -1;
	}

    SetConsoleScreenBufferInfo(gCSI);
}

bool Console::selectionGetArea(SMALL_RECT& sr)
{
	if( __select_invalid() ){
		return(FALSE);
    }
	sr = gSelectRect;
	return(TRUE);
}

void Console::selectionClear(HWND hWnd)
{
	if( __select_invalid() ){
		return;
    }
	gSelectRect.Left = gSelectRect.Right = \
	gSelectRect.Top = gSelectRect.Bottom = 0;
	InvalidateRect(hWnd, NULL, FALSE);
}

