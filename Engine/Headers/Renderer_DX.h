#pragma once
#if BUILD_DX
#include <d3d11.h>
#include <atlbase.h>
#include "gil.h"
#include "Renderer.h"
#include "VBO_DX.h"

typedef struct ConstantBuffer {
	DirectX::XMFLOAT4X4 mvm;
	DirectX::XMFLOAT4 colour;
} ConstantBuffer;

class Renderer_DX : public Renderer, public std::enable_shared_from_this<Renderer_DX> {
protected:
	CComPtr<IDXGISwapChain> _swapchain;
	CComPtr<ID3D11Device> _device;
	CComPtr<ID3D11DeviceContext> _context;
	CComPtr<ID3D11RenderTargetView> _backbuffer;
	CComPtr<ID3D11Buffer> _gCBuffer;
	CComPtr<ID3D11Texture2D> _depthStencilTexture;
	CComPtr<ID3D11DepthStencilState> _depthStencil;
	CComPtr<ID3D11DepthStencilView> _depthStencilView;
	CComPtr<ID3D11RasterizerState> _rasterState;
	CComPtr<ID3D11Debug> _dxDebug;

	std::shared_ptr<VBO> _previousVBO;

	HWND _hwnd;

public:
	Renderer_DX(HWND hwnd);
	virtual ~Renderer_DX();

	nodconst CComPtr<ID3D11Device>& Device() const { return _device; }
	nodconst CComPtr<ID3D11DeviceContext>& Context() const { return _context; }

	void ClearBackBuffer() override;
	void Destroy() override;
	void Draw(const std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX& mvp, const DirectX::XMFLOAT4& colour) override;
	void Init(const SK::INT32 width, const SK::INT32 height) override;
	void SwapBuffers() override;
};

#endif