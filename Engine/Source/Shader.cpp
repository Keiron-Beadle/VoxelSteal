#include <fstream>
#include "../Headers/Shader.h"

#if BUILD_DX
#include <D3DCompiler.h>

Shader::Shader(const std::string& vsPath, const std::string& psPath, const CComPtr<ID3D11Device>& device)
	: _vs(nullptr), _ps(nullptr), _vsBlob(nullptr), _psBlob(nullptr), _inputLayout(nullptr)
{
	HRESULT res;
	
	std::ifstream vsFile(vsPath, std::ios::binary);
	if (!vsFile) throw std::exception("Error opening vsFile.");
	std::vector<SK::BYTE> buffer((std::istreambuf_iterator<SK::BYTE>(vsFile)), std::istreambuf_iterator<SK::BYTE>());
	vsFile.close();
	res = D3DCompile(buffer.data(), buffer.size(), vsPath.c_str(), nullptr, nullptr, "main", "vs_5_0", 0, 0, &_vsBlob, nullptr);
	if (FAILED(res)) throw std::exception("Failed to load VS Blob.");


	std::ifstream psFile(psPath, std::ios::binary);
	if (!psFile) throw std::exception("Error opening psFile.");
	std::vector<SK::BYTE> pbuffer((std::istreambuf_iterator<SK::BYTE>(psFile)), std::istreambuf_iterator<SK::BYTE>());
	psFile.close();
	res = D3DCompile(pbuffer.data(), pbuffer.size(), psPath.c_str(), nullptr, nullptr, "main", "ps_5_0", 0, 0, &_psBlob, nullptr);
	if (FAILED(res)) throw std::exception("Failed to load PS Blob.");

	res = device->CreateVertexShader(_vsBlob->GetBufferPointer(), _vsBlob->GetBufferSize(), nullptr, &_vs.p);
	if (FAILED(res)) throw std::exception("Failed to create vertex shader.");
	res = device->CreatePixelShader(_psBlob->GetBufferPointer(), _psBlob->GetBufferSize(), nullptr, &_ps.p);
	if (FAILED(res)) throw std::exception("Failed to create pixel shader.");
}

Shader::Shader(const std::string& path, const CComPtr<ID3D11Device>& device)
	: Shader(path + "_VS.hlsl", path + "_PS.hlsl", device)
{
}

Shader::Shader(const Shader& rhs)
	: _vs(rhs._vs), _ps(rhs._ps), 
	_vsBlob(rhs._vsBlob), _psBlob(rhs._psBlob),
	_inputLayout(rhs._inputLayout)

{
}

Shader& Shader::operator=(const Shader& rhs)
{
	if (this == &rhs) return *this;
	_vs = rhs._vs;
	_ps = rhs._ps;
	_vsBlob = rhs._vsBlob;
	_psBlob = rhs._psBlob;
	_inputLayout = rhs._inputLayout;
	return *this;
}

nodconst HRESULT Shader::SetInputLayout(const std::vector<D3D11_INPUT_ELEMENT_DESC>& ied, const CComPtr<ID3D11Device>& dev)
{
	HRESULT hr =  dev->CreateInputLayout(ied.data(), 
		static_cast<UINT>(ied.size()),
		_vsBlob->GetBufferPointer(),
		_vsBlob->GetBufferSize(),
		&_inputLayout.p
	);
	if (FAILED(hr)) throw std::exception("Failed to create input layout.");
	return hr;
}

void Shader::Use(const CComPtr<ID3D11DeviceContext>& context)
{
	if (!_inputLayout)
		throw std::exception("Input Layout was not set. Use SetInputLayout(...) on the Shader.");

	context->IASetInputLayout(_inputLayout);
	context->VSSetShader(_vs, nullptr, 0);
	context->PSSetShader(_ps, nullptr, 0);
}

#endif