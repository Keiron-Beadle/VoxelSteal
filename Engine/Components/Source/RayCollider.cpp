#include "../Headers/RayCollider.h"
#include "../Headers/AABBCollider.h"

RayCollider::RayCollider(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT3 direction, ColliderType type)
	: Collider(type), _origin(origin), _direction(direction)
{
}

SK::BOOL RayCollider::CollidesWith(const Collider& collider)
{
	return false;
	//return collider.CollidesWithRay(*this);
}

SK::BOOL RayCollider::CollidesWithRay(const RayCollider& ray) 
{
	return false;
}
