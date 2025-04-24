#include <algorithm>
#include "../Headers/Collider.h"
#include "../Headers/RayCollider.h"
#include "../Headers/AABBCollider.h"

Collider::Collider(ColliderType type)
	: _type(type), _enabled(true)
{
}

const SK::BOOL Collider::AABBRayCollision(const AABB& aabb, const RayCollider& ray)
{
	SK::FLOAT tmin = -std::numeric_limits<SK::FLOAT>::infinity();
	SK::FLOAT tmax = std::numeric_limits<SK::FLOAT>::infinity();

	//auto checkForCollision = [&](SK::FLOAT aabbMin, SK::FLOAT aabbMax, SK::FLOAT rayO, SK::FLOAT rayD, SK::FLOAT& tmin, SK::FLOAT& tmax) {
	//	SK::FLOAT invD = 1.0f / rayD;
	//	SK::FLOAT t0 = (aabbMin - rayO) * invD;
	//	SK::FLOAT t1 = (aabbMax - rayO) * invD;
	//	if (invD < 0.0f) {
	//		std::swap(t0, t1);
	//	}
	//	tmin = t0 > tmin ? t0 : tmin;
	//	tmax = t1 < tmax ? t1 : tmax;
	//	if (tmax < tmin)
	//		return false;
	//};
	auto& rayOrigin = ray.GetOrigin();
	auto& rayDir = ray.GetDirection();

	SK::FLOAT invD = 1.0f / rayDir.x;
	if (invD > -1e-6 && invD < 1e-6) return false;
	SK::FLOAT t0 = (aabb.Min.x - rayOrigin.x) * invD;
	SK::FLOAT t1 = (aabb.Max.x - rayOrigin.x) * invD;
	if (invD < 0.0f)
		std::swap(t0, t1);
	tmin = t0 > tmin ? t0 : tmin;
	tmax = t1 < tmax ? t1 : tmax;
	if (tmax < tmin) return false;

	invD = 1.0f / rayDir.y;
	if (invD > -1e-6 && invD < 1e-6) return false;
	t0 = (aabb.Min.y - rayOrigin.y) * invD;
	t1 = (aabb.Max.y - rayOrigin.y) * invD;
	if (invD < 0.0f)
		std::swap(t0, t1);
	tmin = t0 > tmin ? t0 : tmin;
	tmax = t1 < tmax ? t1 : tmax;
	if (tmax < tmin) return false;


	invD = 1.0f / rayDir.z;
	if (invD > -1e-6 && invD < 1e-6) return false;
	t0 = (aabb.Min.z - rayOrigin.z) * invD;
	t1 = (aabb.Max.z - rayOrigin.z) * invD;
	if (invD < 0.0f)
		std::swap(t0, t1);
	tmin = t0 > tmin ? t0 : tmin;
	tmax = t1 < tmax ? t1 : tmax;
	if (tmax < tmin) return false;


	return true;

	//if (!checkForCollision(aabb.Min.x, aabb.Max.x, ray.GetOrigin().x, ray.GetDirection().x, tmin, tmax))
	//	return false;

	//if (!checkForCollision(aabb.Min.y, aabb.Max.y, ray.GetOrigin().y, ray.GetDirection().y, tmin, tmax))
	//	return false;

	//if (!checkForCollision(aabb.Min.z, aabb.Max.z, ray.GetOrigin().z, ray.GetDirection().z, tmin, tmax))
	//	return false;

	//return true;
}
