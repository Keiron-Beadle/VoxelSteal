#if BUILD_DX
#include "../Headers/Window_DX.h"
#include "../Headers/Game.h"
#include "../Headers/Renderer_DX.h"
#include "../imgui/imgui_impl_win32.h"
#include "../Headers/InputManager.h"
#include <timeapi.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

constexpr SK::INT32 MIN_TIMER_RESOLUTION = 5;

Window_DX::Window_DX(Game* game, SK::INT32 width, SK::INT32 height, HINSTANCE hInst, SK::INT32 nCmdShow) 
	: Window(game, width, height)
{
	WNDCLASSEX wc{};

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = Window_DX::WindowProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindowClass";

	RegisterClassEx(&wc);

	RECT wr = { 0,0,_width,_height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	_hWnd = CreateWindowEx(NULL,
		L"WindowClass",
		L"SketchEngine",
		WS_OVERLAPPEDWINDOW,
		300, 300,
		wr.right - wr.left,
		wr.bottom - wr.top,
		NULL, NULL,
		hInst, NULL);

	SetWindowLongPtr(_hWnd, GWL_STYLE, GetWindowLongPtr(_hWnd, GWL_STYLE) & ~(WS_SIZEBOX | WS_MAXIMIZEBOX));


	ShowWindow(_hWnd, nCmdShow);
	timeBeginPeriod(MIN_TIMER_RESOLUTION);
}

Window_DX::~Window_DX() {
	timeEndPeriod(MIN_TIMER_RESOLUTION);
}

LRESULT CALLBACK Window_DX::WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return true;
	}
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
		break;
	case WM_KEYDOWN:
		//Handle keydown
		break;
	case WM_KEYUP:
		//Handle keyup
		break;
	case WM_ACTIVATE:
		InputManager::Instance().SetActive(LOWORD(wParam) != WA_INACTIVE);
		break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Window_DX::Initialise() {
	_renderer = std::make_shared<Renderer_DX>(_hWnd);
	_renderer->Init(_width, _height);

	_game->Initialise(this);

	MSG msg;
	while (true) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				break;
			}
		}
		_game->Run();
	}
	_renderer->Destroy();
}

#endif