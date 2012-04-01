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

    // window columns
    int	gWinW;		
    // window rows
    int	gWinH;	

    // CONSOLEÇÃÉfÅ[É^
    CONSOLE_SCREEN_BUFFER_INFO gCSI;
    CHAR_INFO *gScreen;

    // selection
    int	gSelectMode;
    // pick point
    COORD	gSelectPos;
    // expanded selection area
    SMALL_RECT gSelectRect;

public:
    Console();
    ~Console();
    void onDestroy();
    bool initialize(ckOpt &opt);
    void __set_console_window_size(LONG cols, LONG rows);
    bool create_child_process(const char* cmd, const char* curdir);
    void __write_console_input(LPCWSTR str, DWORD length);
    wchar_t *selectionGetString();
    wchar_t *getAllString();

    void onLBtnDown(HWND hWnd, int x, int y, const COORD &pos);
    void onLBtnUp(HWND hWnd, int x, int y);
    void onMouseMove(HWND hWnd, int x, int y, const COORD &pos);
    BOOL PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
    bool onTimer(HWND hWnd, bool gVScrollHide, int *x, int *y);

    BOOL WINAPI ReadConsoleOutput_Unicode(CHAR_INFO* buffer,
            COORD size, COORD pos, SMALL_RECT *sr);
    wchar_t* ReadStdOut(int nb, 
            const CONSOLE_SCREEN_BUFFER_INFO &csi, const SMALL_RECT &sr);

    CONSOLE_SCREEN_BUFFER_INFO GetCurrentConsoleScreenBufferInfo(){ return gCSI; }
    CONSOLE_SCREEN_BUFFER_INFO GetConsoleScreenBufferInfo();
    void SetConsoleScreenBufferInfo(const CONSOLE_SCREEN_BUFFER_INFO &csi);
    void copyAllStringToClipboard(HWND hWnd);
    CHAR_INFO *GetScreen(){ return gScreen; }

    bool __select_invalid();
    void __select_expand();
    void __select_word_expand_left();
    void __select_word_expand_right();
    void __select_char_expand();
    bool selectionGetArea(SMALL_RECT& sr);
    void selectionClear(HWND hWnd);
};

#endif
