#pragma once
#if BUILD_DX
#include "Window.h"
#include <Windows.h>
#include <windowsx.h>
#include <D3D11.h>
#include <DirectXMath.h>

#pragma comment (lib,"d3d11.lib")

class Window_DX : public Window {
private:
	HWND _hWnd;

public:
	Window_DX(Game* game, SK::INT32 width, SK::INT32 height, HINSTANCE hInst, SK::INT32 nCmdShow);
	virtual ~Window_DX();

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void Initialise() override;
	HWND& GetHWND() override { return _hWnd; }
};

#endif