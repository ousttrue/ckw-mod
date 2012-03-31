#ifndef APP_H
#define APP_H

#include <windows.h>


class ckOpt;
class App
{
    // window columns
    int	gWinW;		
    // window rows
    int	gWinH;	

    // index color
    COLORREF *gColorTable;

public:
    App();

    ~App();
    
    bool initialize();
    
    // main loop
    int start();

    // �E�C���h�v���V�W��
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
    static LRESULT CALLBACK WndProcProxy(
            HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    // ������
    bool create_window(ckOpt& opt);
    bool create_child_process(const char* cmd, const char* curdir);
    bool create_font(const char* name, int height);
    bool create_console(ckOpt& opt);
    
    // �C�x���g�n���h��
    void onSizing(HWND hWnd, DWORD side, LPRECT rc);
    void onPaint(HWND hWnd);
    void onTimer(HWND hWnd);
    void onWindowPosChange(HWND hWnd, WINDOWPOS* wndpos);
    void onPasteFromClipboard(HWND hWnd);
    void onDropFile(HDROP hDrop);
    bool onSysCommand(HWND hWnd, DWORD id);
    bool onTopMostMenuCommand(HWND hWnd);
    bool onConfigMenuCommand(HWND hWnd, DWORD id);

    void __hide_alloc_console();
    bool __select_invalid();
    void __draw_screen(HDC hDC);
    void __set_console_window_size(LONG cols, LONG rows);
    void __write_console_input(LPCWSTR str, DWORD length);
    wchar_t * getAllString();
    void copyAllStringToClipboard(HWND hWnd);
};


#endif
