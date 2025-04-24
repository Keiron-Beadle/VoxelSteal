#pragma once
#include "gil.h"
#include "VBO.h"

class Renderer;

class Mesh {
private:
	std::vector<Vertex> _vertices;
	std::shared_ptr<VBO> _vbo;
	SK::BOOL _locked, _loaded;

public:
	Mesh();
	Mesh(const std::vector<Vertex>& v);
	~Mesh();

	std::shared_ptr<VBO> CreateVBO(const std::shared_ptr<Renderer> r);
	SK::BOOL AddVertex(const Vertex& v);
	SK::BOOL AddVertices(const std::vector<Vertex>& v);
	SK::BOOL Clear();
	SK::BOOL DeleteVertex(const SK::INT32 idx);

	SK::BOOL LoadFromFile(const std::string& filename);
	void Reset();
	SK::FLOAT CalculateMaxSize();

	nodconst std::shared_ptr<VBO> VBO() const { return _vbo; }
	nodconst SK::INT32 VertexCount() const { return static_cast<SK::INT32>(_vertices.size()); }
	nodconst Vertex VertexAt(const SK::INT32 idx) const { return _vertices[idx]; }
	nodconst Vertex& RefVertexAt(const SK::INT32 idx) { return _vertices[idx]; }
};