#ifndef SELECTION_H
#define SELECTION_H

#include <windows.h>

void copyStringToClipboard( HWND hWnd, const wchar_t * str );
void copyChar(wchar_t*& p, CHAR_INFO* src, SHORT start, SHORT end, bool ret=true);
bool selectionGetArea(SMALL_RECT& sr);
void selectionClear(HWND hWnd);

#endif
