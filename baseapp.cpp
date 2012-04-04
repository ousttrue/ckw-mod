#include "baseapp.h"

//
// WM_CREATEでSetWindowLongPtrを切り替える
//
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

