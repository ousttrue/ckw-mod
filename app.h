#ifndef APP_H
#define APP_H

#include "baseapp.h"

#include <memory>
#include <string>
#include <vector>
class ckOpt;


///
/// ChildAppを起動し、メッセージをやり取りする
///
class App : public BaseApp
{
    // config
    std::shared_ptr<ckOpt> m_opt;

    // font IME
    LOGFONT	gFontLog;	
    // font
    HFONT	gFont;		
    // char width
    DWORD	gFontW;		
    // char height
    DWORD	gFontH;		

    // window frame size
    RECT	gFrame;

    // internal border
    DWORD	gBorderSize;

    // index color
    std::vector<COLORREF> gColorTable;

    // background image
    HBITMAP gBgBmp;
    // background brush
    HBRUSH	gBgBrush;
	// line space
    DWORD	gLineSpace;
    BOOL	gVScrollHide;

    // IME-status
    BOOL gImeOn;

public:
    App();
    ~App();
    
    bool initialize();
    
    // ウインドプロシジャ
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    // 初期化
    bool create_window();
    bool create_font(const char* name, int height);
    // イベントハンドラ
    void onSizing(HWND hWnd, DWORD side, LPRECT rc);
    void onPaint(HWND hWnd);
    void onWindowPosChange(HWND hWnd, WINDOWPOS* wndpos);
    void onPasteFromClipboard(HWND hWnd);
    void onDropFile(HDROP hDrop);
    bool onSysCommand(HWND hWnd, DWORD id);
    bool onTopMostMenuCommand(HWND hWnd);
    bool onConfigMenuCommand(HWND hWnd, DWORD id);
    // 描画
    void __draw_screen(HDC hDC);
    void __draw_invert_char_rect(HDC hDC, RECT& rc);
    void __draw_selection(HDC hDC);
    // その他
    void __set_ime_position(HWND hWnd);
    COORD window_to_charpos(int x, int y);
    void applyConf();
    void reloadConfig(wchar_t *path);
};


#endif
