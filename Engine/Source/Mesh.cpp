#include "../Headers/Mesh.h"
#if BUILD_DX
#include "../Headers/VBO_DX.h"
#endif
#include "../Headers/gil.h"
#include "../Headers/ResourceManager.h"

Mesh::Mesh() : _locked(false), _vbo(nullptr), _loaded(false)
{}

Mesh::Mesh(const std::vector<Vertex>& vertices) 
	: _locked(false), _vbo(nullptr), _loaded(false), _vertices(vertices)
{

}

Mesh::~Mesh(){}

SK::BOOL Mesh::AddVertex(const Vertex& v) {
	if (_locked) return false;
	_vertices.push_back(v);
	return true;
}

SK::BOOL Mesh::AddVertices(const std::vector<Vertex>& vertices)
{
	if (_locked) return false;
	_vertices = vertices;
	return true;
}

SK::BOOL Mesh::DeleteVertex(const SK::INT32 i) {
	if (_locked) return false;
	_vertices.erase(_vertices.begin() + i);
	return true;
}

SK::BOOL Mesh::Clear() {
	if (_locked) return false;
	_vertices.clear();
	return true;
}

SK::BOOL Mesh::LoadFromFile(const std::string& filename)
{
	auto future = ResourceManager::Instance().loadMeshFromFile(filename);
	_vertices = future.get();
	
	//If _vertices is not empty {} then it succeeded.
	return _vertices.size() > 0;
}

void Mesh::Reset() {
	_vbo.reset();
	_locked = false;
	_loaded = false;
}

std::shared_ptr<VBO> Mesh::CreateVBO(const std::shared_ptr<Renderer> r) {
	
	_locked = true;

#if BUILD_DX
	_vbo = std::make_shared<VBO_DX>();
#endif
	_vbo->Create(r, _vertices.data(), VertexCount());

	return _vbo;
}

SK::FLOAT Mesh::CalculateMaxSize() {
	SK::FLOAT max = 0;
	for (SK::INT32 i = 0; i < VertexCount(); ++i) {
		auto v = DirectX::XMLoadFloat3(&_vertices[i].Position);
		DirectX::XMFLOAT3 result;
		DirectX::XMStoreFloat3(&result,DirectX::XMVector3Length(v));
		if (result.x > max) max = result.x;
	}
	return sqrtf(max);
}