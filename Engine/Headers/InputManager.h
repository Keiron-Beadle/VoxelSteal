#pragma once
#include "gil.h"

class InputManager
{
private:
	std::atomic<SK::BOOL> _leftClick{false}, _rightClick{false}, _active{false};
	
	SK::BOOL _keys[256] { false };
	SK::BOOL _prevKeys[256]{ false };
	std::mutex _keyMutex;
private:
	InputManager() = default;
	~InputManager() = default;

public:
	InputManager(const InputManager&) = delete;
	InputManager& operator=(const InputManager&) = delete;

	[[nodiscard]] static InputManager& Instance() {
		static InputManager instance;
		return instance;
	}

	nodconst SK::BOOL LeftClicked() const { return _leftClick.load(); }
	nodconst SK::BOOL RightClicked() const { return _rightClick.load(); }

	const DirectX::XMFLOAT2 GetMousePosition() const;

	nodconst SK::BOOL KeyDown(const KeyCode k) {
		std::unique_lock<std::mutex> lock(_keyMutex);
		const auto key = _keys[static_cast<SK::INT32>(k)];
		return key;
	}

	nodconst SK::BOOL KeyUp(const KeyCode k) {
		std::unique_lock<std::mutex> lock(_keyMutex);
		const auto key = _keys[static_cast<SK::INT32>(k)];
		return key;
	}

	nodconst SK::BOOL KeyPressed(const KeyCode k) {
		std::unique_lock<std::mutex> lock(_keyMutex);
		const auto key = static_cast<SK::INT32>(k);
		const auto nowKey = _keys[key];
		const auto lastKey = _prevKeys[key];
		return nowKey && !lastKey;
	}

	void SetActive(const SK::BOOL active) { _active = active; }
	SK::BOOL GetActive() { return _active; }

	void Poll();
};

