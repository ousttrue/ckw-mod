#ifndef APP_H
#define APP_H

#include <windows.h>


class ckOpt;
class App
{
    /* index color */
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

    bool __select_invalid();
    void __draw_screen(HDC hDC);
};


#endif
