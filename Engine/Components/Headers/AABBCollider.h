#pragma once
#include "Collider.h"

struct AABB {
	DirectX::XMFLOAT3 Min;
	DirectX::XMFLOAT3 Max;

	DirectX::XMFLOAT3 Size() const {
		return {
			Max.x - Min.x,
			Max.y - Min.y,
			Max.z - Min.z
		};
	}

	DirectX::XMFLOAT3 Centre() const {
		return {
			(Max.x + Min.x) * 0.5f,
			(Max.y + Min.y) * 0.5f,
			(Max.z + Min.z) * 0.5f
		};
	}

	bool Intersects(const AABB& other)
	{
		if (Max.x < other.Min.x || Min.x > other.Max.x) return false;
		if (Max.y < other.Min.y || Min.y > other.Max.y) return false;
		if (Max.z < other.Min.z || Min.z > other.Max.z) return false;
		return true;
	}
};

class AABBCollider : public Collider {
private:
	AABB Bounds;

public:
	AABBCollider(DirectX::XMFLOAT3 min, DirectX::XMFLOAT3 max, ColliderType type = ColliderType::STATIC);
	virtual ~AABBCollider() = default;

	nodconst AABB& GetBounds() const { return Bounds; }

	void SetMin(DirectX::XMFLOAT3 value) { Bounds.Min = value; }
	void SetMax(DirectX::XMFLOAT3 value) { Bounds.Max = value; }

	nodconst DirectX::XMFLOAT3& GetMin() const { return Bounds.Min; }
	nodconst DirectX::XMFLOAT3& GetMax() const { return Bounds.Max; }

	SK::BOOL CollidesWith(const Collider& collider) override;
	SK::BOOL CollidesWithRay(const RayCollider& ray) override;
};