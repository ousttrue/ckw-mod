/*-----------------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *---------------------------------------------------------------------------*/
#include "ckw.h"
#include "app.h"
#include "option.h"
#include <memory>

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#ifdef _MSC_VER
int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR lpCmdLine, int nCmdShow)
#else
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow)
#endif
{
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
		       _CRTDBG_LEAK_CHECK_DF |
		       /*_CRTDBG_CHECK_ALWAYS_DF |*/
		       _CRTDBG_DELAY_FREE_MEM_DF);
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_FILE );
	_CrtSetReportMode( _CRT_WARN,   _CRTDBG_MODE_FILE );
	_CrtSetReportMode( _CRT_ERROR,  _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_ASSERT, _CRTDBG_FILE_STDERR );
	_CrtSetReportFile( _CRT_WARN,   _CRTDBG_FILE_STDERR );
	_CrtSetReportFile( _CRT_ERROR,  _CRTDBG_FILE_STDERR );
#endif

    ChildApp child;
    if(!child.initialize()){
        // 初期化失敗
        return 1;
    }
    return child.start();
}

