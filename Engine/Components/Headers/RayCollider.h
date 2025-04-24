#pragma once
#include "Collider.h"

class RayCollider : public Collider {
private:
	DirectX::XMFLOAT3 _origin, _direction;
public:
	RayCollider(DirectX::XMFLOAT3 origin, DirectX::XMFLOAT3 direction, ColliderType type = ColliderType::KINEMATIC);
	virtual ~RayCollider() = default;

	void SetOrigin(DirectX::XMFLOAT3 value) { _origin = value; }
	void SetDirection(DirectX::XMFLOAT3 value) { _direction = value; }

	nodconst DirectX::XMFLOAT3& GetOrigin() const { return _origin; }
	nodconst DirectX::XMFLOAT3& GetDirection() const { return _direction; }

	SK::BOOL CollidesWith(const Collider& collider) override;
	SK::BOOL CollidesWithRay(const RayCollider& ray) override;

};