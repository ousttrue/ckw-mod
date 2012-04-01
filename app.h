#ifndef APP_H
#define APP_H

#include <windows.h>
#include <memory>


class ckOpt;
class Console;
class App
{
    std::shared_ptr<Console> m_console;

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
    // window frame size
    RECT	gFrame;

    // internal border
    DWORD	gBorderSize;

    // screen buffer - copy
    CONSOLE_SCREEN_BUFFER_INFO gCSI;

    CHAR_INFO *gScreen;
    wchar_t *gTitle;

    // index color
    COLORREF *gColorTable;

    // background image
    HBITMAP gBgBmp;
    // background brush
    HBRUSH	gBgBrush;
	// line space
    DWORD	gLineSpace;
    BOOL	gVScrollHide;

    // IME-status
    bool gImeOn;

    // selection
    int	gSelectMode;
    // pick point
    COORD	gSelectPos;
    // expanded selection area
    SMALL_RECT gSelectRect;

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
    bool create_font(const char* name, int height);
    
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

    bool __select_invalid();
    void __draw_screen(HDC hDC);
    void __draw_invert_char_rect(HDC hDC, RECT& rc);
    void __draw_selection(HDC hDC);
    void __set_ime_position(HWND hWnd);
    void __set_console_window_size(LONG cols, LONG rows);
    wchar_t * getAllString();
    void copyAllStringToClipboard(HWND hWnd);
    void __select_expand();
    void __select_word_expand_left();
    void __select_word_expand_right();
    void __select_char_expand();
    void window_to_charpos(int& x, int& y);

    wchar_t* selectionGetString();
    bool selectionGetArea(SMALL_RECT& sr);
    void selectionClear(HWND hWnd);
};


#endif
