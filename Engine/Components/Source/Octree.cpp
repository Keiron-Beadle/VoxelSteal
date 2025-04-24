#include <iostream>
#include "../Headers/OctTree.h"
#include "../../Headers/Game.h"

OctreeNode* Octree::CreateChildNode(const AABB& parentBounds, const DirectX::XMFLOAT3& centre, const DirectX::XMFLOAT3& halfSize)
{
	AABB childBounds({ centre.x - halfSize.x, centre.y - halfSize.y, centre.z - halfSize.z }, { centre.x + halfSize.x, centre.y + halfSize.y, centre.z + halfSize.z });
	return new OctreeNode({ childBounds, {} });
}

void Octree::CreateChildNodes(OctreeNode* node)
{
	const AABB& parentBounds = node->Bounds;
	DirectX::XMFLOAT3 center = parentBounds.Centre();
	DirectX::XMFLOAT3 half = { parentBounds.Size().x * 0.25f, parentBounds.Size().y * 0.25f, parentBounds.Size().z * 0.25f };
	
	node->children[0] = CreateChildNode(parentBounds, DirectX::XMFLOAT3(center.x - half.x, center.y - half.y, center.z - half.z), half);
	node->children[1] = CreateChildNode(parentBounds, DirectX::XMFLOAT3(center.x + half.x, center.y - half.y, center.z - half.z), half);
	node->children[2] = CreateChildNode(parentBounds, DirectX::XMFLOAT3(center.x - half.x, center.y + half.y, center.z - half.z), half);
	node->children[3] = CreateChildNode(parentBounds, DirectX::XMFLOAT3(center.x + half.x, center.y + half.y, center.z - half.z), half);
	node->children[4] = CreateChildNode(parentBounds, DirectX::XMFLOAT3(center.x - half.x, center.y - half.y, center.z + half.z), half);
	node->children[5] = CreateChildNode(parentBounds, DirectX::XMFLOAT3(center.x + half.x, center.y - half.y, center.z + half.z), half);
	node->children[6] = CreateChildNode(parentBounds, DirectX::XMFLOAT3(center.x - half.x, center.y + half.y, center.z + half.z), half);
	node->children[7] = CreateChildNode(parentBounds, DirectX::XMFLOAT3(center.x + half.x, center.y + half.y, center.z + half.z), half);
}

void Octree::Insert(OctreeNode* node, const std::shared_ptr<ColliderComponent> object) {
	auto result = std::dynamic_pointer_cast<AABBCollider>(object->GetCollider());
	if (!result) return;
	const auto& aabb = result->GetBounds();
	if (!node->Bounds.Intersects(aabb)) {
		return;
	}
	if (node->children[0] == nullptr && node->Elements.size() < MAX_OCTNODE_SIZE) {
		node->Elements.push_back(object);
	}
	else {
		if (node->children[0] == nullptr) {
			CreateChildNodes(node);
			for (auto& o : node->Elements) {
				for (auto* c : node->children) {
					Insert(c, o);
				}
			}
			node->Elements.erase(node->Elements.begin(), node->Elements.end());
		}
		for (auto* c : node->children) {
			Insert(c, object);
		}
	}
}

Octree::Octree(DirectX::XMFLOAT3 min, DirectX::XMFLOAT3 max) : Collider(ColliderType::STATIC) {
	_root = new OctreeNode{
		{ min, max },
		{},
		{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
	};
}

Octree::~Octree() {
	delete _root;
}

SK::BOOL Octree::CollidesWith(const Collider& _collider) {
	return false;
}


SK::BOOL Octree::CollidesWithRay(const RayCollider& ray) {
	if (!AABBRayCollision(_root->Bounds, ray)) {
		return false;
	}

	return TraverseNode(_root, ray);
}

SK::BOOL Octree::TraverseNode(const OctreeNode* node, const RayCollider& ray) {
	if (!node->children[0]) {
		for (auto& element : node->Elements) {
			if (!element->Enabled()) continue;
			if (element->GetCollider()->CollidesWithRay(ray)) {
				if (element != _lastCollider)
					element->OnCollision(); //Q.E.D.
				_lastCollider = element;
				return true;
			}
		}
		return false;
	}

	SK::BOOL collided = false;
	for (const auto* c : node->children) {
		if (AABBRayCollision(c->Bounds, ray)) {
			collided |= TraverseNode(c, ray);
		}
	}
	return collided;
}

void Octree::UpdateEntity(UpdateEntityPayload p) {
	{
		std::unique_lock<std::mutex> lock(Game::TheGame->EntityMutex);
		auto newCollider = std::dynamic_pointer_cast<ColliderComponent>(p.entity->GetComponent(ComponentType::Collision));
		if (!newCollider) { return; }
		if (!UpdateCollider(newCollider, p.entity, _root)) {
			std::cout << "New Collider : " << (newCollider->Owner().get()) << '\n';
			std::cout << "Entity Passed: " << (p.entity.get()) << '\n';
			std::cout << "Octree :: Failed to update collider\n";
		}
	}

}

SK::BOOL Octree::UpdateCollider(std::shared_ptr<ColliderComponent>& collider, std::shared_ptr<Entity> e, OctreeNode* node) {

	if (!node) return false;
	for (auto& c : node->Elements) {
		if (c->Owner().get() == e.get()) { // Check here if failed
			c->SetCollider(collider->GetCollider());
			c->SetEnabled(collider->Enabled());
			return true;
		}
	}

	for (auto& child : node->children) {
		if (UpdateCollider(collider, e, child))
			return true;
	}

	return false;
}