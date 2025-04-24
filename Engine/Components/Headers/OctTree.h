#pragma once
#include <vector>
#include "AABBCollider.h"
#include "ColliderComponent.h"

static constexpr int MAX_OCTNODE_SIZE = 5192;

struct OctreeNode {
	AABB Bounds;
	std::vector<std::shared_ptr<ColliderComponent>> Elements;
	OctreeNode* children[8];

	~OctreeNode() {
		for (auto* c : children) {
			if (c) {
				delete c;
			}
		}
	}
};

class Octree : public Collider {
private:
	OctreeNode* _root;
	std::shared_ptr<ColliderComponent> _lastCollider;
public:
	Octree(DirectX::XMFLOAT3 min, DirectX::XMFLOAT3 max);
	~Octree();

	void Insert(const std::shared_ptr<ColliderComponent> object) { Insert(_root, object); }
	SK::BOOL CollidesWith(const Collider& _collider) override;
	SK::BOOL CollidesWithRay(const RayCollider& ray) override;
	void UpdateEntity(UpdateEntityPayload p);

private:
	SK::BOOL UpdateCollider(std::shared_ptr<ColliderComponent>& collider, std::shared_ptr<Entity> e, OctreeNode* node);
	void Insert(OctreeNode* node, const std::shared_ptr<ColliderComponent> object);
	OctreeNode* CreateChildNode(const AABB& parentBounds, const DirectX::XMFLOAT3& centre, const DirectX::XMFLOAT3& halfSize);
	void CreateChildNodes(OctreeNode* node);
	SK::BOOL TraverseNode(const OctreeNode* node, const RayCollider& ray);
};