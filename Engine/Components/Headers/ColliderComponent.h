#pragma once
#include "Component.h"
#include "Collider.h"

class ColliderComponent : public Component{
public:
	std::function<void ()> OnCollision;
private:
	std::shared_ptr<Collider> _collider;
public:
	ColliderComponent(const std::shared_ptr<Collider> c, const std::shared_ptr<Entity> owner = nullptr);
	virtual ~ColliderComponent() = default;
	ColliderComponent(const ColliderComponent&);

	void Start() override;
	void Update(const SK::DOUBLE dT) override;
	void End() override;
	void OnMessage(const std::shared_ptr<Message> msg) override;
	SK::BOOL CollidesWith(const ColliderComponent& cc);

	void SetCollisionHandler(std::function<void()> onCollide) { OnCollision = onCollide; }
	void SetCollider(std::shared_ptr<Collider> collider) { _collider = collider; }
	nodconst std::shared_ptr<Collider> GetCollider() const { return _collider; }
	nodconst ColliderType& GetColliderType() const { return _collider->GetType(); }
};