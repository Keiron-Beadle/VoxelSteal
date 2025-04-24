#pragma once
#include "gil.h"
#include <Windows.h>

class Game;
class Renderer;

class Window {
public:
	static Window* TheWindow;

public:
	Game* _game;
	std::shared_ptr<Renderer> _renderer;
	SK::INT32 _width, _height;

public:
	Window(Game* game, SK::INT32 width, SK::INT32 height);
	virtual ~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

public:
	virtual HWND& GetHWND() = 0;
	nodconst Game* GetGame() const { return _game; } 
	nodconst std::shared_ptr<Renderer> GetRenderer() const { return _renderer; }
	nodconst SK::INT32 GetWidth() const { return _width; }
	nodconst SK::INT32 GetHeight() const { return _height; }

public:
	virtual void Initialise() = 0;
};