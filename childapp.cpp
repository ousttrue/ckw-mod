#include "childapp.h"
#include "option.h"
#include "console.h"
#include "rsrc.h"
#include <vector>


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
                        // •Ï‰»‚È‚µ
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

