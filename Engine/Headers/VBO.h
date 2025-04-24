#pragma once
#include "gil.h"

class Renderer;

class VBO {
protected:
	SK::INT32 _numVertices;

public:
	VBO();
	virtual ~VBO();

	virtual void Create(std::shared_ptr<Renderer> renderer, Vertex vertices[], SK::INT32 numVertices) = 0;

	virtual void Draw(Renderer* renderer) = 0;

};