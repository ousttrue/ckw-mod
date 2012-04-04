#ifndef BASEAPP_H
#define BASEAPP_H

#include <windows.h>


class BaseApp
{
public:
    virtual ~BaseApp(){}

    // ウインドプロシジャ
    virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)=0;

    // main loop
    int start();

    static LRESULT CALLBACK WndProcProxy(
            HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    virtual bool create_window()=0;
};


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


#endif
