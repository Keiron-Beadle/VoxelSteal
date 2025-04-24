#if BUILD_DX
#include <D3D11SDKLayers.h>
#include "../Headers/Renderer_DX.h"
#include "../Headers/Mesh.h"
#include "../Headers/Entity.h"
#include "../Headers/VBO_DX.h"

Renderer_DX::Renderer_DX(HWND hwnd) : _hwnd(hwnd)
{
}

Renderer_DX::~Renderer_DX() {
#ifdef _DEBUG
	if (_dxDebug) {
		_dxDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	}
#endif
}

void Renderer_DX::ClearBackBuffer() {
	FLOAT col[4] = {_clearColour.x, _clearColour.y, _clearColour.z, _clearColour.w};
	_context->ClearRenderTargetView(_backbuffer, col);
	_context->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Renderer_DX::Destroy() {
	_swapchain->SetFullscreenState(FALSE, nullptr);
}

void Renderer_DX::Draw(const std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX& mvp, const DirectX::XMFLOAT4& colour) {
	ConstantBuffer cb{};
	DirectX::XMStoreFloat4x4(&cb.mvm, mvp);
	cb.colour = colour;

	D3D11_MAPPED_SUBRESOURCE ms{};
	_context->Map(_gCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
	memcpy(ms.pData, &cb, sizeof(ConstantBuffer));
	_context->Unmap(_gCBuffer, 0);

	if (mesh->VBO() == _previousVBO) {
		mesh->VBO()->Draw(this);
	}
	else {
		auto dxVBO = std::dynamic_pointer_cast<VBO_DX>(mesh->VBO());
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		const auto& vertices = dxVBO->GetVertices();
		const auto& topology = dxVBO->GetTopology();
		_context->IASetVertexBuffers(0, 1, &vertices.p, &stride, &offset);
		_context->IASetPrimitiveTopology(topology);
		dxVBO->Draw(this);
		_previousVBO = mesh->VBO();
	}
}

void Renderer_DX::Init(const SK::INT32 width, const SK::INT32 height) {
	DXGI_SWAP_CHAIN_DESC scd{};

	HRESULT hr;
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.Width = width;
	scd.BufferDesc.Height = height;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = _hwnd;
	scd.SampleDesc.Count = 4;
	scd.Windowed = TRUE;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

#ifdef _DEBUG
	UINT devFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
	UINT devFlags = 0;
#endif

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		0, 
		devFlags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&scd,
		&_swapchain,
		&_device,
		nullptr,
		&_context
	);

#ifdef _DEBUG
	hr = _device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&_dxDebug));
	if (FAILED(hr)) throw std::exception("Failed to create debug.");
#endif

	ID3D11Texture2D* p_backBuffer;
	_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&p_backBuffer);

	_device->CreateRenderTargetView(p_backBuffer, nullptr, &_backbuffer);

	//create dsv
	D3D11_TEXTURE2D_DESC descDepth{};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 4;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	_device->CreateTexture2D(&descDepth, nullptr, &_depthStencilTexture);

	D3D11_DEPTH_STENCIL_DESC dsd{};
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;

	hr = _device->CreateDepthStencilState(&dsd, &_depthStencil.p);
	if (FAILED(hr)) throw std::exception("Error creating depth stencil state.");

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV{};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	_device->CreateDepthStencilView(_depthStencilTexture, &descDSV, &_depthStencilView);

	_context->OMSetRenderTargets(1, &_backbuffer.p, _depthStencilView);

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<SK::FLOAT>(width);
	viewport.Height = static_cast<SK::FLOAT>(height);
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;

	_context->RSSetViewports(1, &viewport);

	D3D11_RASTERIZER_DESC rasterD{};
	rasterD.CullMode = D3D11_CULL_NONE;
	rasterD.FillMode = D3D11_FILL_SOLID;
	rasterD.ScissorEnable = false;
	rasterD.DepthBias = 0;
	rasterD.DepthBiasClamp = 0.0f;
	rasterD.DepthClipEnable = true;
	rasterD.MultisampleEnable = false;
	rasterD.SlopeScaledDepthBias = 0.0f;
	hr = _device->CreateRasterizerState(&rasterD, &_rasterState);

	D3D11_BUFFER_DESC bd{};
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;

	hr = _device->CreateBuffer(&bd, nullptr, &_gCBuffer);
	if (FAILED(hr)) throw std::exception("Failed to create constant buffer.");


	_context->VSSetConstantBuffers(0, 1, &_gCBuffer.p);
	_context->PSSetConstantBuffers(0, 1, &_gCBuffer.p);
	_context->RSSetState(_rasterState);
	_context->OMSetDepthStencilState(_depthStencil, 0);
}

void Renderer_DX::SwapBuffers() {
	_swapchain->Present(0, 0);
}
#endif