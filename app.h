#ifndef APP_H
#define APP_H

#include <windows.h>


class ckOpt;
class App
{
    // font IME
    LOGFONT	gFontLog;	
    // font
    HFONT	gFont;		
    // char width
    DWORD	gFontW;		
    // char height
    DWORD	gFontH;		

    // window columns
    int	gWinW;		
    // window rows
    int	gWinH;	

    CHAR_INFO *gScreen;
    wchar_t *gTitle;

    // index color
    COLORREF *gColorTable;

public:
    App();

    ~App();
    
    bool initialize();
    
    // main loop
    int start();

    // ウインドプロシジャ
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK WndProcProxy(
            HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    // 初期化
    bool create_window(ckOpt& opt);
    bool create_child_process(const char* cmd, const char* curdir);
    bool create_font(const char* name, int height);
    bool create_console(ckOpt& opt);
    
    // イベントハンドラ
    void onSizing(HWND hWnd, DWORD side, LPRECT rc);
    void onPaint(HWND hWnd);
    void onTimer(HWND hWnd);
    void onWindowPosChange(HWND hWnd, WINDOWPOS* wndpos);
    void onPasteFromClipboard(HWND hWnd);
    void onDropFile(HDROP hDrop);
    bool onSysCommand(HWND hWnd, DWORD id);
    bool onTopMostMenuCommand(HWND hWnd);
    bool onConfigMenuCommand(HWND hWnd, DWORD id);
    void onLBtnDown(HWND hWnd, int x, int y);
    void onLBtnUp(HWND hWnd, int x, int y);
    void onMouseMove(HWND hWnd, int x, int y);

    void __hide_alloc_console();
    bool __select_invalid();
    void __draw_screen(HDC hDC);
    void __draw_invert_char_rect(HDC hDC, RECT& rc);
    void __draw_selection(HDC hDC);
    void __set_ime_position(HWND hWnd);
    void __set_console_window_size(LONG cols, LONG rows);
    void __write_console_input(LPCWSTR str, DWORD length);
    wchar_t * getAllString();
    void copyAllStringToClipboard(HWND hWnd);
    void __select_expand();
    void __select_word_expand_left();
    void __select_word_expand_right();
    void __select_char_expand();
};


#endif
