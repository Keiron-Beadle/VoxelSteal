#include <windows.h>
#include "../Headers/InputManager.h"
#include "../Headers/Window.h"

void InputManager::Poll()
{
#ifdef BUILD_WINDOWS
	_leftClick = GetAsyncKeyState(VK_LBUTTON) != 0;
	_rightClick = GetAsyncKeyState(VK_RBUTTON) != 0;

	{
		std::unique_lock<std::mutex> lock(_keyMutex);
		memcpy(_prevKeys, _keys, sizeof(_keys));
		for (SK::INT32 i = 0; i < sizeof(_keys); ++i)
		{
			_keys[i] = GetAsyncKeyState(i) != 0;
		}
	}
#endif
}

const DirectX::XMFLOAT2 InputManager::GetMousePosition() const {
#ifdef BUILD_WINDOWS
	POINT cursorPos;
	if (GetCursorPos(&cursorPos)) {
		ScreenToClient(Window::TheWindow->GetHWND(), &cursorPos);
		return DirectX::XMFLOAT2(cursorPos.x, cursorPos.y);
	}
#endif
}
