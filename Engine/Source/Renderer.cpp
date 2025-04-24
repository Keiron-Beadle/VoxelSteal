#include "../Headers/Renderer.h"
#include "../Components/Headers/RenderComponent.h"
#include "../Components/Headers/TransformComponent.h"

Renderer::Renderer() : _clearColour(0,0,0,1)
{
	DirectX::XMStoreFloat4x4(&_vp, DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&_proj, DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&_view, DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&_invVP, DirectX::XMMatrixIdentity());
}

Renderer::~Renderer() {
}

void Renderer::SetProjection(DirectX::XMFLOAT4X4 projection)
{
	_proj = projection;
	DirectX::XMStoreFloat4x4(&_vp, DirectX::XMLoadFloat4x4(&_view) * DirectX::XMLoadFloat4x4(&_proj));
	DirectX::XMStoreFloat4x4(&_invVP, DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&_vp)));
}

void Renderer::SetView(DirectX::XMFLOAT4X4 view) {
	_view = view;
	DirectX::XMStoreFloat4x4(&_vp, DirectX::XMLoadFloat4x4(&_view) * DirectX::XMLoadFloat4x4(&_proj));
	DirectX::XMStoreFloat4x4(&_invVP, DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&_vp)));
}

void Renderer::Draw(RenderComponent& rc, TransformComponent& tc, DirectX::XMMATRIX& vp) {
	DirectX::XMMATRIX mvp;

	//Since changing to use data locality, we're not using pointers
	//Means TransformComponent& tc
	//May be a null transform (which is identified with a scale of 0)
	auto scale = tc.Scale();

	if (scale.x != 0 && scale.y != 0 && scale.z != 0) {
		auto pos = tc.Position();

		mvp = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) *
			DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z) * vp;
	}
	else {
		mvp = vp;
	}

	const auto& mesh = rc.GetMesh();
	if (mesh) {
		Draw(mesh, mvp, rc.Colour());
	}

}