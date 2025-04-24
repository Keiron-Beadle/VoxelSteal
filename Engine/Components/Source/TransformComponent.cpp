#include "../Headers/TransformComponent.h"

TransformComponent::TransformComponent(DirectX::XMFLOAT3 pos,
	DirectX::XMFLOAT3 scale, std::shared_ptr<Entity> owner) 
	: Component(ComponentType::Transform, owner), 
	_position(pos), _scale(scale)
{

}

TransformComponent::~TransformComponent()
{
	End();
}

TransformComponent::TransformComponent(const TransformComponent& tc)
	: Component(tc)
{
	_position = tc._position;
	_scale = tc._scale;
}

void TransformComponent::Start()
{
}

void TransformComponent::Update(const SK::DOUBLE dT)
{
}

void TransformComponent::End()
{
}

void TransformComponent::OnMessage(const std::shared_ptr<Message> msg)
{
}
