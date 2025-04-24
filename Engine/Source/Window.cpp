#include "../Headers/Window.h"

Window* Window::TheWindow = nullptr;

Window::Window(Game* game, SK::INT32 width, SK::INT32 height)
	: _game(game), _width(width), _height(height)
{
	TheWindow = this;
}

Window::~Window()
{
}
