#pragma once
#include "gil.h"

class Game;
class RenderComponent;
class TransformComponent;
class Mesh;

class Renderer : protected std::enable_shared_from_this<Renderer>
{
public:
	DirectX::XMFLOAT4X4 _vp, _view, _proj, _invVP;
	DirectX::XMFLOAT4 _clearColour;

	Renderer();
	virtual ~Renderer();

public:
	nodconst DirectX::XMFLOAT4 ClearColour() const { return _clearColour; }
	void SetClearColour(const DirectX::XMFLOAT4 c) { _clearColour = c; }

	nodconst DirectX::XMFLOAT4X4& GetVP() const { return _vp; }
	nodconst DirectX::XMFLOAT4X4& GetIVP() const { return _invVP; }
	nodconst DirectX::XMFLOAT4X4& GetProjection() const { return _proj; }
	nodconst DirectX::XMFLOAT4X4& GetView() const { return _view; }

	void SetProjection(DirectX::XMFLOAT4X4 projection);
	void SetView(DirectX::XMFLOAT4X4 view);

	virtual void ClearBackBuffer() = 0;
	virtual void Draw(const std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX& mvm, const DirectX::XMFLOAT4& colour) = 0;
	virtual void Draw(RenderComponent& rc, TransformComponent& tc, DirectX::XMMATRIX& mvm);

	virtual void Destroy() = 0;
	virtual void Init(const SK::INT32 width, const SK::INT32 height) = 0;
	virtual void SwapBuffers() = 0;
};

