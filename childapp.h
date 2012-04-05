#ifndef CHILDAPP_H
#define CHILDAPP_H

#include "baseapp.h"
#include <memory>
#include <string>
class Console;


///
/// コンソール上に子プロセスを起動し、コンソールを隠す
///
class ChildApp: public BaseApp
{
    HWND m_hWnd;
    HWND m_hParent;
    std::shared_ptr<Console> m_console;

    // window title
    std::wstring gTitle;

public:
    ChildApp();
    ~ChildApp();

    bool initialize(HWND hParent);

    // ウインドプロシジャ
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    bool create_window();
};

#endif
