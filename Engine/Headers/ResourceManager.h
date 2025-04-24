#pragma once
#include "gil.h"
#include "Shader.h"

class ResourceManager {
private:
	ModelResourceMap _models;
	ShaderResourceMap _shaders;
	ResourceManager() = default;
	~ResourceManager() = default;
#ifdef BUILD_DX
	CComPtr<ID3D11Device> _device;
#endif

public:
	static ResourceManager& Instance() {
		static ResourceManager instance;
		return instance;
	}
	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;

	[[nodiscard]] std::future<std::vector<Vertex>> loadMeshFromFile(const std::string& filename) const;
	[[nodiscard]] std::future<std::vector<Vertex>> loadCube() const;
	
	[[nodiscard]] std::future<Shader> loadShaderFromFile(const std::string& vsPath, const std::string& psPath) const;
	[[nodiscard]] std::future<Shader> loadShaderFromFile(const std::string& path) const;

#ifdef BUILD_DX
	void SetDevice(const CComPtr<ID3D11Device>& dev) { _device = dev; }
#endif
};