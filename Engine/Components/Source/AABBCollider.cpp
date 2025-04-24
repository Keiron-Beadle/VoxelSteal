#include "../Headers/AABBCollider.h"
#include "../Headers/RayCollider.h"

AABBCollider::AABBCollider(DirectX::XMFLOAT3 min, DirectX::XMFLOAT3 max, ColliderType type)
	: Collider(type), Bounds({min,max})
{
}

//Deprecated
SK::BOOL AABBCollider::CollidesWith(const Collider& collider)
{
	if (const auto ray = dynamic_cast<const RayCollider*>(&collider)) {
		return AABBRayCollision(Bounds, *ray);
	}
	return false;
}

SK::BOOL AABBCollider::CollidesWithRay(const RayCollider& ray) 
{
	return AABBRayCollision(Bounds, ray);
}
