#include "../Headers/RenderComponent.h"
#include "../Headers/Component.h"

RenderComponent::RenderComponent(std::shared_ptr<Entity> owner)
	: Component(ComponentType::Render, owner),
	_colour(1.0f, 1.0f, 1.0f, 1.0f), _mesh(nullptr)
{
}

RenderComponent::~RenderComponent()
{
	End();
}

RenderComponent::RenderComponent(const RenderComponent& rc)
	: Component(rc)
{
	_mesh = rc._mesh;
	_colour = rc._colour;
}

void RenderComponent::Start() {

}

void RenderComponent::Update(const SK::DOUBLE dT) {

}

void RenderComponent::OnMessage(const std::shared_ptr<Message> msg) {

}

void RenderComponent::End()
{

}

void RenderComponent::AddColour(DirectX::XMFLOAT4 c) {
	_colour.x = std::min(1.0f, _colour.x + c.x);
	_colour.y = std::min(1.0f, _colour.y + c.y );
	_colour.z = std::min(1.0f, _colour.z + c.z);
}

void RenderComponent::SubColour(DirectX::XMFLOAT4 c) {
	_colour.x = std::max(0.0f, _colour.x - c.x);
	_colour.y = std::max(0.0f, _colour.y - c.y);
	_colour.z = std::max(0.0f, _colour.z - c.z);
}