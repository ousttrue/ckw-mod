#ifndef CHILDAPP_H
#define CHILDAPP_H

#include "baseapp.h"
#include <memory>
#include <string>
class ckOpt;
class Console;


///
/// コンソール上に子プロセスを起動し、コンソールを隠す
///
class ChildApp: public BaseApp
{
    std::shared_ptr<Console> m_console;

    // window title
    std::wstring gTitle;

    // config
    std::shared_ptr<ckOpt> m_opt;

public:
    ChildApp();
    ~ChildApp();

    bool initialize();

    // ウインドプロシジャ
    LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    bool create_window();
};

#endif
