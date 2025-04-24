#include "../Headers/ColliderComponent.h"

ColliderComponent::ColliderComponent(const std::shared_ptr<Collider> c, const std::shared_ptr<Entity> owner)
	: Component(ComponentType::Collision, owner), _collider(c)
{

}

ColliderComponent::ColliderComponent(const ColliderComponent& cc)
	:Component(cc)
{
	OnCollision = cc.OnCollision;
	_collider = cc._collider;
}

void ColliderComponent::Start()
{
}

void ColliderComponent::Update(const SK::DOUBLE dT)
{
}

void ColliderComponent::End()
{
}

void ColliderComponent::OnMessage(const std::shared_ptr<Message> msg)
{
}

SK::BOOL ColliderComponent::CollidesWith(const ColliderComponent& cc)
{
	return _collider->CollidesWith(*cc._collider);
}
