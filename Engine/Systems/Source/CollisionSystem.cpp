#include "../../Components/Headers/Collider.h"
#include "../../Components/Headers/RayCollider.h"
#include "../Headers/CollisionSystem.h"
#include "../../Headers/Game.h"
#include "../../Headers/Message.h"
#include <iostream>
#include <Windows.h>
#undef SendMessage

CollisionSystem::CollisionSystem(EntityList& entities, SK::UINT32 freq, SK::UINT32 reqCore)
	: System(freq, reqCore)
{
	Address |= Addressee::COLLISION_SYSTEM;
	for (auto& e : entities) {
		auto c = e->GetComponent(ComponentType::Collision);
		if (!c) continue;
		auto cc = std::dynamic_pointer_cast<ColliderComponent>(c);

		if (cc->GetColliderType() == ColliderType::KINEMATIC)
			_kinematics.push_back(ColliderComponent(*cc));
		//else
		//	_statics.push_back(*cc->GetCollider());
	}

	//_updateEntityThread = std::thread([this]() { UpdateEntityThreadFunc(); });
}

CollisionSystem::~CollisionSystem()
{

}

void CollisionSystem::AddCollider(std::shared_ptr<Collider> collider) {
	_statics.push_back(collider);
}

void CollisionSystem::AddEntity(std::shared_ptr<Entity> e)
{
	auto c = e->GetComponent(ComponentType::Collision);
	if (!c) return;
	auto cc = std::dynamic_pointer_cast<ColliderComponent>(c);

	std::unique_lock<std::recursive_mutex> lock(_mutex);
	if (cc->GetColliderType() == ColliderType::KINEMATIC)
		_kinematics.push_back(ColliderComponent(*cc));
	//else
	//	_statics.push_back(std::make*cc->GetCollider());
}

void CollisionSystem::UpdateEntity(std::shared_ptr<Entity> e)
{
	auto c = e->GetComponent(ComponentType::Collision);
	if (!c) return;
	auto cc = std::dynamic_pointer_cast<ColliderComponent>(c);

	//Try get lock, if can't get it, update the entity asynchronously
	//Else if you did get it, great, just update it normally.
	
	std::unique_lock<std::recursive_mutex> lock(_mutex);
	for (SK::SIZE_T t = 0; t < _kinematics.size(); ++t) {
		if (_kinematics[t].Owner() == e) {
			_kinematics[t] = ColliderComponent(*cc);
			return;
		}
	}

	// 
	//std::unique_lock<std::mutex> lock(_mutex, std::try_to_lock);
	//if (!lock.owns_lock()) {
	//	std::unique_lock<std::mutex> updateLock(_queueMutex);
	//	_updateQueue.emplace(e, cc);
	//	_updateEntityCV.notify_one();
	//	std::cout << "Told thread to upate entity\n";
	//}
	//else {
	//	for (SK::SIZE_T t = 0; t < _kinematics.size(); ++t) {
	//		if (_kinematics[t].Owner() == e) {
	//			_kinematics[t] = ColliderComponent(*cc);
	//			return;
	//		}
	//	}
	//	std::cout << "Updated mouse entity\n";
	//	//for (SK::SIZE_T t = 0; t < _statics.size(); ++t) {
	//	//	if (_statics[t].Owner() == e) {
	//	//		_statics[t] = ColliderComponent(*cc);
	//	//		return;
	//	//	}
	//	//}
	//}
}

void CollisionSystem::UpdateEntityThreadFunc()
{
	std::shared_ptr<Entity> e;
	std::shared_ptr<ColliderComponent> cc;
	while (!_cancelUpdateEntityThread.load()) {
		
		{
			std::unique_lock<std::mutex> lock(_queueMutex);
			_updateEntityCV.wait(lock, [this]() {return !_updateQueue.empty() || _cancelUpdateEntityThread.load(); });
			if (_cancelUpdateEntityThread.load()) {
				break;
			}
			auto update = std::move(_updateQueue.front());
			_updateQueue.pop();

			e = update.first;
			cc = update.second;
		}

		{
			SK::BOOL found = false;
			std::unique_lock<std::recursive_mutex> lock(_mutex);
			for (SK::SIZE_T t = 0; t < _kinematics.size(); ++t) {
				if (_kinematics[t].Owner() == e) {
					_kinematics[t] = ColliderComponent(*cc);
					found = true;
					break;
				}
			}
			//for (SK::SIZE_T t = 0; t < _statics.size(); ++t) {
			//	if (found) break;
			//	if (_statics[t].Owner() == e) {
			//		_statics[t] = ColliderComponent(*cc);
			//		break;
			//	}
			//}
		}
	}
}

void CollisionSystem::CancelSystem()
{
	if (_updateEntityThread.joinable()) {
		_cancelUpdateEntityThread.store(true);
		_updateEntityCV.notify_all();
		_updateEntityThread.join();
	}
	System::CancelSystem();
}

void CollisionSystem::OnMessage(std::shared_ptr<Message> msg)
{
	auto messageType = msg->Type();
	if ((Address & messageType) == 0) return;

	auto rsp = std::dynamic_pointer_cast<SystemMessage>(msg);
	if (rsp) {
		const auto& payload = rsp->GetPayload();
		switch (payload.instruction) {
		case 0:
			SetFrequency(payload.value);
			break;
		case 1:
			SetCore(payload.value);
			break;
		default:
			break;
		}
		return;
	}

	auto uem = std::dynamic_pointer_cast<UpdateEntityMessage>(msg);
	if (uem) {
		const auto& payload = uem->GetPayload();
		UpdateEntity(payload.entity);
		return;
	}
}

void CollisionSystem::Process()
{
	if (_pullNewEntities.load()) {
		std::unique_lock<std::mutex> updateLock(Game::TheGame->EntityMutex);
		if (updateLock.owns_lock()) {
			_kinematics.clear();
			_statics.clear();
			for (auto& e : Game::TheGame->GetEntities())
				AddEntity(e);
			_pullNewEntities.store(false);
		}
	}

	std::unique_lock<std::recursive_mutex> lock(_mutex);
	//for (auto& kin : _kinematics) {
	//	if (!kin.Enabled()) {
	//		continue;
	//	}
	//	for (auto& st : _statics) {
	//		if (!st.Enabled()) {
	//			continue;
	//		}
	//		
	//		//if (kin.CollidesWith(st)) {
	//			//if (kin.OnCollision)
	//			//	kin.OnCollision(st);

	//			//if (st.OnCollision)
	//			//	st.OnCollision(kin);
	//		//}
	//	}
	//}

	//Very bad and not scalable or maintainable, but it might work for performance 
	for (auto& kin : _kinematics) {
		
		if (!kin.Enabled()) { continue; }

		//Only allow ray casters for now 
		auto rayTest = dynamic_cast<RayCollider*>(kin.GetCollider().get());
		if (!rayTest) { continue; }

		for (auto& sta : _statics) {
			if (!sta->Enabled()) continue;
			if (sta->CollidesWithRay(*rayTest)) {
				if (kin.OnCollision)
				{
					kin.OnCollision();
				}
			}
		}
	}

}
