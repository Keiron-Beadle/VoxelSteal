#pragma once
#include <d3d11.h>
#include <atlbase.h>
#include "gil.h"

using namespace DirectX;

#ifdef BUILD_DX
class Shader {
private:
	CComPtr<ID3DBlob> _vsBlob;
	CComPtr<ID3DBlob> _psBlob;

	CComPtr<ID3D11InputLayout> _inputLayout;

	CComPtr<ID3D11VertexShader> _vs;
	CComPtr<ID3D11PixelShader> _ps;
	
public:
	Shader(const std::string& vsPath, const std::string& psPath, const CComPtr<ID3D11Device>& device);
	Shader(const std::string& path, const CComPtr<ID3D11Device>& device);
	~Shader() = default;
	Shader(const Shader&);
	Shader& operator=(const Shader&);

	nodconst CComPtr<ID3D11InputLayout> GetInputLayout() const { return _inputLayout; }
	nodconst HRESULT SetInputLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& ied, const CComPtr<ID3D11Device>& dev);

	void Use(const CComPtr<ID3D11DeviceContext>& context);
};

#endif

#ifdef BUILD_GX

#endif