#ifndef CONSOLE_H
#define CONSOLE_H

#include <windows.h>


class ckOpt;
class Console
{
    HANDLE	gStdIn;	
    HANDLE	gStdOut;
    HANDLE	gStdErr;
    HWND	gConWnd;

    // child process
    HANDLE	gChild;	

public:
    Console();
    ~Console();
    void onDestroy();
    bool initialize(ckOpt &opt);
    bool create_child_process(const char* cmd, const char* curdir);
    void __write_console_input(LPCWSTR str, DWORD length);

    void onMouseMove(HWND hWnd, int x, int y);
    BOOL PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
    void onTimer(HWND hWnd);

    BOOL WINAPI ReadConsoleOutput_Unicode(CHAR_INFO* buffer,
            COORD size, COORD pos, SMALL_RECT *sr);
    wchar_t* ReadStdOut(int nb, 
            const CONSOLE_SCREEN_BUFFER_INFO &csi, const SMALL_RECT &sr);
    CONSOLE_SCREEN_BUFFER_INFO GetConsoleScreenBufferInfo();
    void SetConsoleScreenBufferInfo(const CONSOLE_SCREEN_BUFFER_INFO &csi);
};

#endif
