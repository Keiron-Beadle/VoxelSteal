#include "../Headers/SketchGame.h"

#ifndef _DEBUG
#define new new (_NORMAL_BLOCK, FILE, LINE)
#endif

static const SK::INT32 WIDTH = 1280;
static const SK::INT32 HEIGHT = 720;

#if BUILD_DX

#include <Engine/Headers/Window_DX.h>

SK::INT32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, SK::INT32 nCmdShow) {
	//SK::INT32 flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	//flag |= _CRTDBG_LEAK_CHECK_DF;
	//_CrtSetDbgFlag(flag);

//#ifdef _DEBUG
	//AllocConsole();
	//FILE* pCerr;
	//if (freopen_s(&pCerr, "CONOUT$", "w", stderr) != 0) {
	//	MessageBox(nullptr, L"Failed to redirect stdout to console.", L"Error", MB_OK);
	//}
	//if (freopen_s(&pCerr, "CONOUT$", "w", stdout) != 0) {
	//	MessageBox(nullptr, L"Failed to redirect stdout to console.", L"Error", MB_OK);
	//}
//#endif
	{
		SketchGame game;
		Window_DX app(&game, WIDTH, HEIGHT, hInstance, nCmdShow);
		app.Initialise();
	}
//#ifdef _DEBUG
	//system("pause");
	//FreeConsole();
//#endif
}
#endif