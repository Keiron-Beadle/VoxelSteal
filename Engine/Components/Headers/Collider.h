#pragma once
#include "../../Headers/gil.h"

class AABBCollider;
class RayCollider;
struct AABB;

class Collider {
private:
	ColliderType _type;
	SK::BOOL _enabled;
public:
	Collider(ColliderType type);
	virtual ~Collider() = default;

	virtual SK::BOOL CollidesWith(const Collider& _collider) = 0;
	virtual SK::BOOL CollidesWithRay(const RayCollider& ray) = 0;

	nodconst SK::BOOL Enabled() const { return _enabled; }
	void SetEnabled(SK::BOOL value) { _enabled = value; }

	nodconst ColliderType& GetType() const { return _type; }

protected:
	static nodconst SK::BOOL AABBRayCollision(const AABB& aabb, const RayCollider& ray);
};