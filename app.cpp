#include "app.h"
#include "ime_wrap.h"
#include "option.h"
#include "rsrc.h"
#include "console.h"
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>


//////////////////////////////////////////////////////////////////////////////
// BaseApp
//////////////////////////////////////////////////////////////////////////////
//
// WM_CREATEでSetWindowLongPtrを切り替える
//
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_CREATE:
            {
                // lParamの初期化パラメータをSetWindowLongPtrする
                BaseApp *app=(BaseApp*)((LPCREATESTRUCT)lParam)->lpCreateParams;
                SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)app);
                // WndProcを切り替える
                SetWindowLongPtr(hwnd, GWL_WNDPROC, (LONG_PTR)BaseApp::WndProcProxy);
                return app->WndProc(hwnd, message, wParam, lParam);
            }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

// main loop
int BaseApp::start()
{
    MSG msg;
    while(GetMessage(&msg, NULL, 0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

// WndProcをメンバ関数に転送する
LRESULT CALLBACK BaseApp::WndProcProxy(
        HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    BaseApp *app=(BaseApp*)GetWindowLongPtr(hwnd, GWL_USERDATA);
    return app->WndProc(hwnd, message, wParam, lParam);
}


//////////////////////////////////////////////////////////////////////////////
// ChildApp
//////////////////////////////////////////////////////////////////////////////
ChildApp::ChildApp()
    :
        m_console(new Console),
        m_opt(new ckOpt)
{
}

ChildApp::~ChildApp()
{
}

bool ChildApp::initialize()
{
    if(! m_opt->initialize()) {
        return false;
    }

    if(!m_console->initialize(m_opt)){
        trace("create_console failed\n");
        return false;
    }
    if(! m_console->create_child_process(m_opt->getCmd(), m_opt->getCurDir())) {
        trace("create_child_process failed\n");
        return false;
    }
    if(! create_window()) {
        trace("create_window failed\n");
        return false;
    }

    return true;
}

bool ChildApp::create_window()
{
    trace("create_window\n");

    HINSTANCE hInstance = GetModuleHandle(NULL);
    LPCWSTR className = L"CkwWindowChildClass";
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_ACCEPTFILES;
    LONG posx, posy;

    const char* conf_title = m_opt->getTitle();
    std::wstring title(L"ckw");
    if(conf_title && conf_title[0]){
        std::vector<wchar_t> buf(strlen(conf_title), 0);
        MultiByteToWideChar(CP_ACP, 0, conf_title, (int)strlen(conf_title), 
                &buf[0], (int)(sizeof(wchar_t) * buf.size()));
        title=std::wstring(buf.begin(), buf.end());
    }

    /* calc window size */
    CONSOLE_SCREEN_BUFFER_INFO csi=m_console->GetConsoleScreenBufferInfo();

    LONG width = csi.srWindow.Right - csi.srWindow.Left + 1;
    LONG height = csi.srWindow.Bottom - csi.srWindow.Top + 1;
    DebugPrintf("%d\n", width);

    posx = CW_USEDEFAULT;
    posy = CW_USEDEFAULT;

    /**/
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = ::WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, (LPCTSTR)IDR_ICON);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    //wc.hbrBackground = CreateSolidBrush(gColorTable[0]);
    wc.lpszMenuName = NULL;
    wc.lpszClassName=className;
    wc.hIconSm = NULL;
    if(! RegisterClassEx(&wc))
        return(FALSE);

    HWND hWnd = CreateWindowEx(exstyle, className, title.c_str(), style,
            posx, posy, width, height,
            NULL, NULL, hInstance, this);
    if(!hWnd){
        return(FALSE);
    }

    //ShowWindow(hWnd, SW_SHOW);
    return(TRUE);
}

// WndProc
LRESULT ChildApp::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
        case WM_CREATE:
            SetTimer(hWnd, 0x3571, 10, NULL);
            break;

        case WM_DESTROY:
            KillTimer(hWnd, 0x3571);
            PostQuitMessage(0);
            m_console->onDestroy();
            break;

        case WM_TIMER:
            {
                /* title update */
                static int timer_count = 0;
                if((++timer_count & 0xF) == 1) {
                    wchar_t str[256]={0};
                    GetConsoleTitle(str, 256);
                    if(!gTitle.empty() && !wcscmp(gTitle.c_str(), str)) {
                        // 変化なし
                    }
                    else {
                        gTitle = str;
                        //SetWindowText(hWnd, gTitle.c_str());
                    }
                }

                /*
                int w;
                int h;
                if(m_console->onTimer(hWnd, gVScrollHide, &w, &h)){
                    SetWindowPos(hWnd, NULL, 0,0
                            , (w * gFontW) + (gBorderSize * 2)+(gFrame.right - gFrame.left)
                            , (h * gFontH) + (gBorderSize * 2)+(gFrame.bottom - gFrame.top)
                            , SWP_NOMOVE|SWP_NOZORDER);
                }
                */
            }
            break;

        case WM_ERASEBKGND:
            break;

        default:
            return( DefWindowProc(hWnd, msg, wp, lp) );
    }
    return(1);
}


//////////////////////////////////////////////////////////////////////////////
// App
//////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK AboutDlgProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
        case WM_INITDIALOG:
            {
                HWND hEdit = GetDlgItem(hWnd, IDC_EDIT1);
                SetWindowText(hEdit,
                        L"This program is free software; you can redistribute it and/or\r\n"
                        L"modify it under the terms of the GNU General Public License\r\n"
                        L"as published by the Free Software Foundation; either version 2\r\n"
                        L"of the License, or (at your option) any later version.\r\n"
                        L"\r\n"
                        L"This program is distributed in the hope that it will be useful,\r\n"
                        L"but WITHOUT ANY WARRANTY; without even the implied warranty of\r\n"
                        L"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\r\n"
                        L"See the GNU General Public License for more details.\r\n"
                        L"\r\n"
                        L"You should have received a copy of the GNU General Public License\r\n"
                        L"along with this program; if not, write to the Free Software Foundation, Inc.,\r\n"
                        L" 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA."
                        );
            }
            return(TRUE);
        case WM_COMMAND:
            switch(LOWORD(wp)) {
                case IDOK:
                case IDCANCEL:
                    EndDialog(hWnd, 0);
                    return(TRUE);
            }
    }
    return(FALSE);
}

App::App()
    :
        m_opt(new ckOpt),
        gBorderSize(0),
        gColorTable(kColorMax, 0),
        gBgBmp(NULL),
        gBgBrush(NULL),
        gLineSpace(0),
        gVScrollHide(false),
        gImeOn(FALSE)
{
    gFrame.left=0;
    gFrame.right=0;
    gFrame.top=0;
    gFrame.bottom=0;
}

App::~App()
{
    SAFE_DeleteObject(gFont);
    SAFE_DeleteObject(gBgBrush);
    SAFE_DeleteObject(gBgBmp);
    ime_wrap_term();
}

void changeStateTopMostMenu(HWND hWnd,HMENU hMenu)
{
    DWORD dwExStyle = GetWindowLong(hWnd,GWL_EXSTYLE);

    if ((dwExStyle & WS_EX_TOPMOST) == 0) {
        CheckMenuItem(hMenu, IDM_TOPMOST, MF_BYCOMMAND | MFS_UNCHECKED);
    } else {
        CheckMenuItem(hMenu, IDM_TOPMOST, MF_BYCOMMAND | MFS_CHECKED);
    }
}

void sysmenu_init_topmost(HWND hWnd, HMENU hMenu)
{
    MENUITEMINFO mii;

    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_CHECKMARKS;

    mii.fType = MFT_STRING;
    mii.wID = IDM_TOPMOST;
    mii.dwTypeData=const_cast<wchar_t*>(L"TopMost (&T)");
    mii.cch = (UINT) wcslen(mii.dwTypeData);

    InsertMenuItem(hMenu, SC_CLOSE, FALSE, &mii);

    changeStateTopMostMenu(hWnd,hMenu);
}

bool App::initialize()
{
    if(! ime_wrap_init()) {
        trace("ime_wrap_init failed\n");
    }

    if(! m_opt->initialize()) {
        return false;
    }

    applyConf();

    if(! create_font(m_opt->getFont(), m_opt->getFontSize())) {
        trace("create_font failed\n");
        return false;
    }
    if(! create_window()) {
        trace("create_window failed\n");
        return false;
    }

    return true;
}

void sysmenu_init(HWND hWnd)
{
    MENUITEMINFO mii;
    HMENU hMenu = GetSystemMenu(hWnd, FALSE);

    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;

    mii.fType = MFT_STRING;
    mii.wID = IDM_COPYALL;
    mii.dwTypeData=const_cast<wchar_t*>(L"Copy All(&C)");
    mii.cch = (UINT) wcslen(mii.dwTypeData);
    InsertMenuItem(hMenu, SC_CLOSE, FALSE, &mii);

    mii.fType = MFT_STRING;
    mii.wID = IDM_NEW;
    mii.dwTypeData=const_cast<wchar_t*>(L"New (&N)");
    mii.cch = (UINT) wcslen(mii.dwTypeData);
    InsertMenuItem(hMenu, SC_CLOSE, FALSE, &mii);

    sysmenu_init_topmost(hWnd, hMenu);

    // sysmenu_init_subconfig(hWnd, hMenu);

    mii.fType = MFT_SEPARATOR;
    mii.wID = 0;
    mii.dwTypeData = 0;
    mii.cch = 0;
    InsertMenuItem(hMenu, SC_CLOSE, FALSE, &mii);

    mii.fType = MFT_STRING;
    mii.wID = IDM_ABOUT;
    mii.dwTypeData=const_cast<wchar_t*>(L"About (&A)");
    mii.cch = (UINT) wcslen(mii.dwTypeData);
    InsertMenuItem(hMenu, SC_CLOSE, FALSE, &mii);

    mii.fType = MFT_SEPARATOR;
    mii.wID = 0;
    mii.dwTypeData = 0;
    mii.cch = 0;
    InsertMenuItem(hMenu, SC_CLOSE, FALSE, &mii);
}

bool App::create_window()
{
    trace("create_window\n");

    HINSTANCE hInstance = GetModuleHandle(NULL);
    LPCWSTR className = L"CkwWindowClass";
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_ACCEPTFILES;
    LONG posx, posy;

    if(m_opt->isTranspColor() ||
            (0 < m_opt->getTransp() && m_opt->getTransp() < 255))
        exstyle |= WS_EX_LAYERED;

    if(m_opt->isScrollRight())
        exstyle |= WS_EX_RIGHTSCROLLBAR;
    else
        exstyle |= WS_EX_LEFTSCROLLBAR;

    if(m_opt->isTopMost())
        exstyle |= WS_EX_TOPMOST;

    if(m_opt->isScrollHide() || m_opt->getSaveLines() < 1)
        gVScrollHide = TRUE;
    else
        style |= WS_VSCROLL;

    if(m_opt->isIconic())
        style |= WS_MINIMIZE;

    const char* conf_title = m_opt->getTitle();
    std::wstring title(L"ckw");
    if(conf_title && conf_title[0]){
        std::vector<wchar_t> buf(strlen(conf_title), 0);
        MultiByteToWideChar(CP_ACP, 0, conf_title, (int)strlen(conf_title), 
                &buf[0], (int)(sizeof(wchar_t) * buf.size()));
        title=std::wstring(buf.begin(), buf.end());
    }

    /* calc window size */
    /*
    CONSOLE_SCREEN_BUFFER_INFO csi=m_console->GetConsoleScreenBufferInfo();

    AdjustWindowRectEx(&gFrame, style, FALSE, exstyle);
    if(!gVScrollHide)
        gFrame.right += GetSystemMetrics(SM_CXVSCROLL);

    LONG width = csi.srWindow.Right - csi.srWindow.Left + 1;
    LONG height = csi.srWindow.Bottom - csi.srWindow.Top + 1;
    */
    LONG width=m_opt->getWinCharW();
    LONG height=m_opt->getWinCharH();

    width *= gFontW;
    height *= gFontH;
    DebugPrintf("%d\n", width);

    width += gBorderSize * 2;
    height += gBorderSize * 2;
    DebugPrintf("%d\n", width);

    width += gFrame.right - gFrame.left;
    height += gFrame.bottom - gFrame.top;
    DebugPrintf("%d\n", width);


    if(m_opt->isWinPos()) {
        RECT rc;
        SystemParametersInfo(SPI_GETWORKAREA,0,(LPVOID)&rc,0);
        posx = m_opt->getWinPosX();
        if(posx < 0) posx = rc.right - (width - posx -1);
        else posx += rc.left;
        if(posx < rc.left) posx = rc.left;
        if(posx > rc.right-5) posx = rc.right -5;
        posy = m_opt->getWinPosY();
        if(posy < 0) posy = rc.bottom - (height - posy -1);
        else posy += rc.top;
        if(posy < rc.top) posy = rc.top;
        if(posy > rc.bottom-5) posy = rc.bottom -5;
    }
    else {
        posx = CW_USEDEFAULT;
        posy = CW_USEDEFAULT;
    }

    /**/
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = 0;
    wc.lpfnWndProc = ::WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, (LPCTSTR)IDR_ICON);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(gColorTable[0]);
    wc.lpszMenuName = NULL;
    wc.lpszClassName=className;
    wc.hIconSm = NULL;
    if(! RegisterClassEx(&wc))
        return(FALSE);

    HWND hWnd = CreateWindowEx(exstyle, className, title.c_str(), style,
            posx, posy, width, height,
            NULL, NULL, hInstance, this);
    if(!hWnd){
        return(FALSE);
    }

    sysmenu_init(hWnd);

    if(0 < m_opt->getTransp() && m_opt->getTransp() < 255)
        SetLayeredWindowAttributes(hWnd, 0, m_opt->getTransp(), LWA_ALPHA);
    else if(m_opt->isTranspColor())
        SetLayeredWindowAttributes(hWnd, m_opt->getTranspColor(), 255, LWA_COLORKEY);

    ShowWindow(hWnd, SW_SHOW);
    return(TRUE);
}

COORD App::window_to_charpos(int x, int y)
{
    x -= gBorderSize;
    y -= gBorderSize;
    if(x < 0) x = 0;
    if(y < 0) y = 0;
    x /= gFontW;
    y /= gFontH;

    /*
    CONSOLE_SCREEN_BUFFER_INFO csi=m_console->GetCurrentConsoleScreenBufferInfo();
    x += csi.srWindow.Left;
    y += csi.srWindow.Top;
    if(x > csi.srWindow.Right) x = csi.srWindow.Right+1;
    if(y > csi.srWindow.Bottom) y = csi.srWindow.Bottom;
    */
    COORD pos={(short)x, (short)y};

    return pos;
}

void get_directory_path(wchar_t *path)
{
    wchar_t *c;
    GetModuleFileName(NULL, path, MAX_PATH);
    c = wcsrchr(path, L'\\');
    if(c) *c = 0;
}

void sysmenu_init_subconfig(HWND hWnd, HMENU hMenu)
{
    MENUITEMINFO mii;
    HMENU hSubMenu = CreatePopupMenu();

    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;

    wchar_t path[MAX_PATH+1];
    get_directory_path(path);
    wcscat_s(path, L"\\*.cfg");

    WIN32_FIND_DATA fd;
    memset(&fd, 0, sizeof(fd));
    HANDLE hFile = FindFirstFile(path, &fd);
    //MessageBox(hWnd, path, L"", MB_OK);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        for(int i = 0;;i++)
        {
            mii.fType = MFT_STRING;
            mii.wID = IDM_CONFIG_SELECT_1 + i;
            mii.dwTypeData = fd.cFileName;
            mii.cch = (UINT) wcslen(mii.dwTypeData);
            InsertMenuItem(hSubMenu, 0, TRUE, &mii);
            if(FindNextFile(hFile, &fd) == 0) { break; }
        }
        FindClose(hFile);
    }

    mii.fMask |= MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.wID = IDM_CONFIG_SELECT;
    mii.hSubMenu = hSubMenu;
    mii.dwTypeData=const_cast<wchar_t*>(L"Config (&O)");
    mii.cch = (UINT) wcslen(mii.dwTypeData);
    InsertMenuItem(hMenu, SC_CLOSE, FALSE, &mii);
}

void App::reloadConfig(wchar_t *path)
{
    char filepath[MAX_PATH+1];
    wcstombs(filepath, path, MAX_PATH);

    m_opt->setFile(filepath);

    if(!m_opt->initialize()){
        return;
    }

    applyConf();
}

void App::__draw_invert_char_rect(HDC hDC, RECT& rc)
{
    rc.right++;
    rc.bottom++;
    rc.left *= gFontW;
    rc.right *= gFontW;
    rc.top *= gFontH;
    rc.bottom *= gFontH;
    BitBlt(hDC, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, NULL,0,0, DSTINVERT);
}

void App::__draw_selection(HDC hDC)
{
#if 0
    SMALL_RECT sel;
    if(!m_console->selectionGetArea(sel)){
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO csi=m_console->GetCurrentConsoleScreenBufferInfo();
    if(csi.srWindow.Top <= sel.Top && sel.Top <= csi.srWindow.Bottom)
        ;
    else if(csi.srWindow.Top <= sel.Bottom && sel.Bottom <= csi.srWindow.Bottom)
        ;
    else if(sel.Top < csi.srWindow.Top && csi.srWindow.Bottom < sel.Bottom)
        ;
    else
        return;

    RECT rc;

    if(sel.Top == sel.Bottom) {
        /* single line */
        rc.left = sel.Left - csi.srWindow.Left;
        rc.right = sel.Right-1 - csi.srWindow.Left;
        rc.top = \
                 rc.bottom = sel.Top - csi.srWindow.Top;
        __draw_invert_char_rect(hDC, rc);
        return;
    }

    /* multi line */
    if(csi.srWindow.Top <= sel.Top && sel.Top <= csi.srWindow.Bottom) {
        /* top */
        rc.left = sel.Left - csi.srWindow.Left;
        rc.right = csi.srWindow.Right - csi.srWindow.Left;
        rc.top = \
                 rc.bottom = sel.Top - csi.srWindow.Top;
        __draw_invert_char_rect(hDC, rc);
    }
    if(sel.Top+1 <= sel.Bottom-1) {
        /* center */
        rc.left = 0;
        rc.right = csi.srWindow.Right - csi.srWindow.Left;

        if(csi.srWindow.Top <= sel.Top+1)
            rc.top = sel.Top+1 - csi.srWindow.Top;
        else
            rc.top = 0;

        if(csi.srWindow.Bottom >= sel.Bottom-1)
            rc.bottom = sel.Bottom-1 - csi.srWindow.Top;
        else
            rc.bottom = csi.srWindow.Bottom - csi.srWindow.Top;
        __draw_invert_char_rect(hDC, rc);
    }
    if(csi.srWindow.Top <= sel.Bottom && sel.Bottom <= csi.srWindow.Bottom) {
        /* bottom */
        rc.left = 0;
        rc.right = sel.Right-1 - csi.srWindow.Left;
        rc.top = \
                 rc.bottom = sel.Bottom - csi.srWindow.Top;
        __draw_invert_char_rect(hDC, rc);
    }
#endif
}

void App::__draw_screen(HDC hDC)
{
#if 0
    CONSOLE_SCREEN_BUFFER_INFO csi=m_console->GetCurrentConsoleScreenBufferInfo();
    int pntX, pntY;
    int x, y;
    int color_fg;
    int color_bg;
    CHAR_INFO* ptr = m_console->GetScreen();
    int work_color_fg = -1;
    int work_color_bg = -1;
    wchar_t* work_text = new wchar_t[ CSI_WndCols(&csi) ];
    wchar_t* work_text_ptr;
    INT* work_width = new INT[ CSI_WndCols(&csi) ];
    INT* work_width_ptr;
    int work_pntX;

    pntY = 0;
    for(y = csi.srWindow.Top ; y <= csi.srWindow.Bottom ; y++) {
        pntX = 0;
        work_pntX = 0;
        work_text_ptr = work_text;
        work_width_ptr = work_width;
        for(x = csi.srWindow.Left ; x <= csi.srWindow.Right ; x++) {

            if(ptr->Attributes & COMMON_LVB_TRAILING_BYTE) {
                pntX += gFontW;
                ptr++;
                continue;
            }

            color_fg = ptr->Attributes & 0xF;
            color_bg = (ptr->Attributes>>4) & 0xF;

            if(color_fg != work_color_fg ||
                    color_bg != work_color_bg) {
                if(work_text_ptr > work_text) {
                    ExtTextOut(hDC, work_pntX, pntY, 0, NULL,
                            (LPCWSTR)work_text,
                            (UINT)(work_text_ptr - work_text),
                            work_width);
                }
                work_text_ptr = work_text;
                work_width_ptr = work_width;
                work_pntX = pntX;
                work_color_fg = color_fg;
                work_color_bg = color_bg;
                SetTextColor(hDC, gColorTable[work_color_fg]);
                SetBkColor( hDC, gColorTable[work_color_bg]);
                SetBkMode(hDC, (work_color_bg) ? OPAQUE : TRANSPARENT);
            }

            if(ptr->Attributes & COMMON_LVB_LEADING_BYTE) {
                *work_text_ptr++ = ptr->Char.UnicodeChar;
                *work_width_ptr++ = gFontW * 2;
            }
            else {
                *work_text_ptr++ = ptr->Char.UnicodeChar;
                *work_width_ptr++ = gFontW;
            }
            pntX += gFontW;
            ptr++;
        }

        if(work_text_ptr > work_text) {
            ExtTextOut(hDC, work_pntX, pntY, 0, NULL,
                    (LPCWSTR)work_text,
                    (UINT)(work_text_ptr - work_text),
                    work_width);
        }

        pntY += gFontH;
    }

    /* draw selection */
    __draw_selection(hDC);

    /* draw cursor */
    if(csi.srWindow.Top <= csi.dwCursorPosition.Y &&
            csi.srWindow.Bottom >= csi.dwCursorPosition.Y &&
            csi.srWindow.Left <= csi.dwCursorPosition.X &&
            csi.srWindow.Right >= csi.dwCursorPosition.X) {
        color_fg = (gImeOn) ? kColorCursorImeFg : kColorCursorFg;
        color_bg = (gImeOn) ? kColorCursorImeBg : kColorCursorBg;
        SetTextColor(hDC, gColorTable[ color_fg ]);
        SetBkColor( hDC, gColorTable[ color_bg ]);
        SetBkMode(hDC, OPAQUE);
        pntX = csi.dwCursorPosition.X - csi.srWindow.Left;
        pntY = csi.dwCursorPosition.Y - csi.srWindow.Top;
        ptr = m_console->GetScreen() + CSI_WndCols(&csi) * pntY + pntX;
        pntX *= gFontW;
        pntY *= gFontH;
        *work_width = (ptr->Attributes & COMMON_LVB_LEADING_BYTE) ? gFontW*2 : gFontW;
        ExtTextOut(hDC, pntX, pntY, 0, NULL,
                &ptr->Char.UnicodeChar, 1, work_width);
    }

    delete [] work_width;
    delete [] work_text;
#endif
}

void App::__set_ime_position(HWND hWnd)
{
    /*
    if(!gImeOn) return;
    HIMC imc = ImmGetContext(hWnd);
    CONSOLE_SCREEN_BUFFER_INFO csi=m_console->GetCurrentConsoleScreenBufferInfo();
    LONG px = csi.dwCursorPosition.X - csi.srWindow.Left;
    LONG py = csi.dwCursorPosition.Y - csi.srWindow.Top;
    COMPOSITIONFORM cf;
    cf.dwStyle = CFS_POINT;
    cf.ptCurrentPos.x = px * gFontW + gBorderSize;
    cf.ptCurrentPos.y = py * gFontH + gBorderSize;
    ImmSetCompositionWindow(imc, &cf);
    ImmReleaseContext(hWnd, imc);
    */
}

void App::applyConf()
{
    for(int i = kColor0 ; i <= kColor15 ; i++){
        gColorTable[i] = m_opt->getColor(i);
    }
    gColorTable[kColor7] = m_opt->getColorFg();
    gColorTable[kColor0] = m_opt->getColorBg();

    gColorTable[kColorCursorBg] = m_opt->getColorCursor();
    gColorTable[kColorCursorFg] = ~gColorTable[kColorCursorBg] & 0xFFFFFF;
    gColorTable[kColorCursorImeBg] = m_opt->getColorCursorIme();
    gColorTable[kColorCursorImeFg] = ~gColorTable[kColorCursorImeBg] & 0xFFFFFF;

    gBorderSize = m_opt->getBorderSize();
    gLineSpace = m_opt->getLineSpace();

    if(m_opt->getBgBmp()) {
        gBgBmp = (HBITMAP)LoadImageA(NULL, m_opt->getBgBmp(),
                IMAGE_BITMAP, 0,0, LR_LOADFROMFILE);
    }
    if(gBgBmp) gBgBrush = CreatePatternBrush(gBgBmp);
    if(!gBgBrush) gBgBrush = CreateSolidBrush(gColorTable[0]);
}

// WndProc
LRESULT App::WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg) {
        case WM_CREATE:
            {
                HIMC imc = ImmGetContext(hWnd);
                ImmSetCompositionFontW(imc, &gFontLog);
                ImmReleaseContext(hWnd, imc);
            }
            break;
        case WM_DESTROY:
            KillTimer(hWnd, 0x3571);
            PostQuitMessage(0);
            break;
        case WM_TIMER:

        case WM_ERASEBKGND:
            break;
        case WM_PAINT:
            onPaint(hWnd);
            break;

        case WM_SIZING:
            onSizing(hWnd, (DWORD)wp, (LPRECT)lp);
            break;
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
            onWindowPosChange(hWnd, (WINDOWPOS*)lp);
            //m_console->selectionClear(hWnd);
            break;

        case WM_LBUTTONDOWN:
            /*
            m_console->onLBtnDown(hWnd, (short)LOWORD(lp), (short)HIWORD(lp)
                    , window_to_charpos((short)LOWORD(lp), (short)HIWORD(lp)));
                    */
            break;
        case WM_LBUTTONUP:
            /*
            if(hWnd == GetCapture()){
                ReleaseCapture();
                m_console->onLBtnUp(hWnd, (short)LOWORD(lp), (short)HIWORD(lp));
            }
            */
            break;
        case WM_MOUSEMOVE:
            /*
            m_console->onMouseMove(hWnd, (short)LOWORD(lp),(short)HIWORD(lp)
                    , window_to_charpos((short)LOWORD(lp), (short)HIWORD(lp)));
                    */
            break;

        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            onPasteFromClipboard(hWnd);
            break;

        case WM_DROPFILES:
            onDropFile((HDROP)wp);
            break;

        case WM_IME_STARTCOMPOSITION:
            __set_ime_position(hWnd);
            return( DefWindowProc(hWnd, msg, wp, lp) );

        case WM_IME_NOTIFY:
            if(wp == IMN_SETOPENSTATUS) {
                HIMC imc = ImmGetContext(hWnd);
                gImeOn = ImmGetOpenStatus(imc);
                ImmReleaseContext(hWnd, imc);
                InvalidateRect(hWnd, NULL, TRUE);
            }
            return( DefWindowProc(hWnd, msg, wp, lp) );

        case WM_SYSCOMMAND:
            if(!onSysCommand(hWnd, (DWORD)wp))
                return( DefWindowProc(hWnd, msg, wp, lp) );
            break;
        case WM_VSCROLL:
        case WM_MOUSEWHEEL:
            /* throw console window */
            //m_console->PostMessage(msg, wp, lp);
            break;

        case WM_IME_CHAR:
            //m_console->PostMessage(msg, wp, lp);
            /* break */
        case WM_CHAR:
            //m_console->selectionClear(hWnd);
            break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            //if(wp != VK_RETURN) /* alt+enter */
                //m_console->PostMessage(msg, wp, lp);
            break;
        case WM_KEYDOWN:
        case WM_KEYUP:
            if((wp == VK_NEXT || wp == VK_PRIOR ||
                        wp == VK_HOME || wp == VK_END) &&
                    (GetKeyState(VK_SHIFT) & 0x8000)) {
                if(msg == WM_KEYDOWN) {
                    WPARAM sb = SB_PAGEDOWN;
                    if(wp == VK_PRIOR) sb = SB_PAGEUP;
                    else if(wp == VK_HOME) sb = SB_TOP;
                    else if(wp == VK_END) sb = SB_BOTTOM;
                    //m_console->PostMessage(WM_VSCROLL, sb, 0);
                }
            }
            else if(wp == VK_INSERT &&
                    (GetKeyState(VK_SHIFT) & 0x8000)) {
                if(msg == WM_KEYDOWN)
                    onPasteFromClipboard(hWnd);
            }
            else {
                //m_console->PostMessage(msg, wp, lp);
            }
            break;
        default:
            return( DefWindowProc(hWnd, msg, wp, lp) );
    }
    return(1);
}

void App::onWindowPosChange(HWND hWnd, WINDOWPOS* wndpos)
{
    trace("onWindowPosChange\n");
    if(!(wndpos->flags & SWP_NOSIZE) && !IsIconic(hWnd)) {
        LONG fw = (gFrame.right - gFrame.left) + (gBorderSize * 2);
        LONG fh = (gFrame.bottom - gFrame.top) + (gBorderSize * 2);
        LONG width = wndpos->cx;
        LONG height = wndpos->cy;
        width = (width - fw) / gFontW;
        height = (height - fh) / gFontH;

        //m_console->__set_console_window_size(width, height);

        wndpos->cx = width * gFontW + fw;
        wndpos->cy = height * gFontH + fh;
    }
}
void App::onSizing(HWND hWnd, DWORD side, LPRECT rc)
{
    trace("onSizing\n");
    LONG fw = (gFrame.right - gFrame.left) + (gBorderSize * 2);
    LONG fh = (gFrame.bottom - gFrame.top) + (gBorderSize * 2);
    LONG width = rc->right - rc->left;
    LONG height = rc->bottom - rc->top;

    width -= fw;
    width -= width % gFontW;
    width += fw;

    height -= fh;
    height -= height % gFontH;
    height += fh;

    if(side==WMSZ_LEFT || side==WMSZ_TOPLEFT || side==WMSZ_BOTTOMLEFT)
        rc->left = rc->right - width;
    else
        rc->right = rc->left + width;

    if(side==WMSZ_TOP || side==WMSZ_TOPLEFT || side==WMSZ_TOPRIGHT)
        rc->top = rc->bottom - height;
    else
        rc->bottom = rc->top + height;
}

void App::onPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hWnd, &ps);
    RECT rc;
    GetClientRect(hWnd, &rc);

    HDC hMemDC = CreateCompatibleDC(hDC);
    HBITMAP hBmp = CreateCompatibleBitmap(hDC, rc.right-rc.left, rc.bottom-rc.top);
    HGDIOBJ oldfont = SelectObject(hMemDC, gFont);
    HGDIOBJ oldbmp = SelectObject(hMemDC, hBmp);

    FillRect(hMemDC, &rc, gBgBrush);

    /*
    if(m_console->GetScreen()) {
        SetWindowOrgEx(hMemDC, -(int)gBorderSize, -(int)gBorderSize, NULL);
        __draw_screen(hMemDC);
        SetWindowOrgEx(hMemDC, 0, 0, NULL);
    }
    */

    BitBlt(hDC,rc.left,rc.top, rc.right-rc.left, rc.bottom-rc.top, hMemDC,0,0, SRCCOPY);

    SelectObject(hMemDC, oldfont);
    SelectObject(hMemDC, oldbmp);
    DeleteObject(hBmp);
    DeleteDC(hMemDC);

    EndPaint(hWnd, &ps);
}

bool App::create_font(const char* name, int height)
{
    trace("create_font\n");

    memset(&gFontLog, 0, sizeof(gFontLog));
    gFontLog.lfHeight = -height;
    gFontLog.lfWidth = 0;
    gFontLog.lfEscapement = 0;
    gFontLog.lfOrientation = 0;
    gFontLog.lfWeight = FW_NORMAL;
    gFontLog.lfItalic = 0;
    gFontLog.lfUnderline = 0;
    gFontLog.lfStrikeOut = 0;
    gFontLog.lfCharSet = DEFAULT_CHARSET;
    gFontLog.lfOutPrecision = OUT_DEFAULT_PRECIS;
    gFontLog.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    gFontLog.lfQuality = DEFAULT_QUALITY;
    gFontLog.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    if(name) {
        MultiByteToWideChar(CP_ACP,0, name, -1, gFontLog.lfFaceName, LF_FACESIZE);
    }

    gFont = CreateFontIndirect(&gFontLog);

    /* calc font size */
    HDC hDC = GetDC(NULL);
    HGDIOBJ oldfont = SelectObject(hDC, gFont);
    TEXTMETRIC met;
    INT width1[26], width2[26], width = 0;

    GetTextMetrics(hDC, &met);
    GetCharWidth32(hDC, 0x41, 0x5A, width1);
    GetCharWidth32(hDC, 0x61, 0x7A, width2);
    SelectObject(hDC, oldfont);
    ReleaseDC(NULL, hDC);

    for(int i = 0 ; i < 26 ; i++) {
        width += width1[i];
        width += width2[i];
    }
    width /= 26 * 2;
    gFontW = width; /* met.tmAveCharWidth; */
    gFontH = met.tmHeight + gLineSpace;

    return(TRUE);
}

void App::onPasteFromClipboard(HWND hWnd)
{
    bool result = true;
    HANDLE hMem;
    wchar_t *ptr;

    if(! IsClipboardFormatAvailable(CF_UNICODETEXT))
        return;
    if(!OpenClipboard(hWnd)) {
        Sleep(10);
        if(!OpenClipboard(hWnd))
            return;
    }
    hMem = GetClipboardData(CF_UNICODETEXT);
    if(!hMem)
        result = false;
    if(result && !(ptr = (wchar_t*)GlobalLock(hMem)))
        result = false;
    if(result) {
        //m_console->__write_console_input(ptr, (DWORD)wcslen(ptr));
        GlobalUnlock(hMem);
    }
    CloseClipboard();
}

void App::onDropFile(HDROP hDrop)
{
    DWORD i, nb, len;
    wchar_t wbuf[MAX_PATH+32];
    wchar_t* wp;

    nb = DragQueryFile(hDrop, (DWORD)-1, NULL, 0);
    for(i = 0 ; i < nb ; i++) {
        len = DragQueryFile(hDrop, i, NULL, 0);
        if(len < 1 || len > MAX_PATH)
            continue;
        wp = wbuf + 1;
        if(! DragQueryFile(hDrop, i, wp, MAX_PATH))
            continue;
        wp[len] = 0;
        while(*wp > 0x20) wp++;
        if(*wp) {
            wp = wbuf;
            len++;
            wp[0] = wp[len++] = L'\"';
        }
        else {
            wp = wbuf + 1;
        }
        wp[len++] = L' ';

        //m_console->__write_console_input(wp, len);
    }
    DragFinish(hDrop);
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

bool App::onSysCommand(HWND hWnd, DWORD id)
{
    switch(id) {
        case IDM_COPYALL:
            //m_console->copyAllStringToClipboard(hWnd);
            return(TRUE);
        case IDM_ABOUT:
            DialogBox(GetModuleHandle(NULL),
                    MAKEINTRESOURCE(IDD_DIALOG1),
                    hWnd,
                    AboutDlgProc);
            return(TRUE);
        case IDM_NEW:
            // ckwchildの生成
            //makeNewWindow();
            return(TRUE);
        case IDM_TOPMOST:
            return onTopMostMenuCommand(hWnd);
    }
    if(IDM_CONFIG_SELECT < id && id <= IDM_CONFIG_SELECT_MAX) {
        return onConfigMenuCommand(hWnd, id);
    }
    return(FALSE);
}

bool App::onTopMostMenuCommand(HWND hWnd)
{
    HMENU hMenu = GetSystemMenu(hWnd, FALSE);

    UINT uState = GetMenuState( hMenu, IDM_TOPMOST, MF_BYCOMMAND);
    if( uState & MFS_CHECKED ) {
        SetWindowPos(hWnd, HWND_NOTOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE); 
    }else{
        SetWindowPos(hWnd, HWND_TOPMOST,0, 0, 0, 0,SWP_NOMOVE | SWP_NOSIZE); 
    }

    changeStateTopMostMenu(hWnd, hMenu);

    return(TRUE);
}

bool App::onConfigMenuCommand(HWND hWnd, DWORD id)
{
    wchar_t path[MAX_PATH+1];
    get_directory_path(path);

    MENUITEMINFO mii;
    memset(&mii, 0, sizeof(mii));
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_TYPE | MIIM_ID;
    mii.fType = MFT_STRING;

    HMENU hMenu = GetSystemMenu(hWnd, FALSE);
    if (GetMenuItemInfo(hMenu, id, 0, &mii))
    {
        wchar_t sz[MAX_PATH+1], filepath[MAX_PATH+1];
        mii.dwTypeData = sz;
        mii.cch++;
        GetMenuItemInfo(hMenu, id, 0, &mii);
        wsprintf(filepath, L"%s\\%s", path, sz);
        /* need to open file and update config here */
        MessageBox(hWnd, filepath, L"", MB_OK);
        reloadConfig(filepath);
    }

    return(TRUE);
}

