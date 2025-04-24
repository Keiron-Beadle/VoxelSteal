#pragma once
#include "System.h"
#include "../../Components/Headers/ColliderComponent.h"
#include <queue>
#include <set>

class CollisionSystem : public System {
private:
	CollisionList _kinematics;
	StaticColliderList _statics;
	std::queue < std::pair<std::shared_ptr<Entity>, std::shared_ptr<ColliderComponent>>> _updateQueue;
	std::thread _updateEntityThread;
	std::recursive_mutex _mutex;
	std::mutex _queueMutex;
	std::condition_variable _updateEntityCV;
	std::atomic<SK::BOOL> _cancelUpdateEntityThread;
public:
	CollisionSystem(EntityList& entities, SK::UINT32 freq = 60,
		SK::UINT32 reqCore = std::numeric_limits<SK::UINT32>::max());
	virtual ~CollisionSystem();

	void AddEntity(std::shared_ptr<Entity> e) override;
	void AddCollider(std::shared_ptr<Collider> collider);
	void UpdateEntity(std::shared_ptr<Entity> e) override;
	void UpdateEntityThreadFunc();

	void CancelSystem() override;
	void OnMessage(std::shared_ptr<Message> msg) override;

private:
	void Process() override;


};