#include <iostream>
#include "../Headers/RenderSystem.h"
#include "../../Components/Headers/RenderComponent.h"
#include "../../Components/Headers/TransformComponent.h"
#include "../../Headers/Game.h"
#include "../../Headers/Message.h"


RenderSystem::RenderSystem(EntityList& entities)
{
	Address |= Addressee::RENDER_SYSTEM;
	for (const auto& e : entities) {
		const auto& rc = e->GetComponent(ComponentType::Render);
		if (!rc) continue;
		const auto& castedRC = std::dynamic_pointer_cast<RenderComponent>(rc);
		_renderables.push_back(RenderComponent(*castedRC));

		const auto& tc = e->GetComponent(ComponentType::Transform);
		if (!tc) {
			//If the entity doesn't have TC
			//Add a null one, such that we can
			//Keep the indices between RenderList/TransformList
			//In order
			_transforms.push_back(TransformComponent(
			DirectX::XMFLOAT3(0,0,0), 
			DirectX::XMFLOAT3(0,0,0)));
			continue;
		}
		const auto& castedTC = std::dynamic_pointer_cast<TransformComponent>(tc);
		_transforms.push_back(TransformComponent(*castedTC));
	}
}

RenderSystem::~RenderSystem()
{
	std::cout << "Render System :: Destructor called" << std::endl;
}

void RenderSystem::OnMessage(std::shared_ptr<Message> message)
{
	//Compare message type to Render System address ("RENDERSYSTEM" hash value)
	auto messageType = message->Type();
	if ((Address & messageType) == 0) return;

	auto uem = std::dynamic_pointer_cast<UpdateEntityMessage>(message);
	if (uem) {
		const auto& payload = uem->GetPayload();
		UpdateEntity(payload.entity);
		return;
	}

	auto rsp = std::dynamic_pointer_cast<SystemMessage>(message);
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
	auto pep = std::dynamic_pointer_cast<PullEntityMessage>(message);
	if (pep) {
		_pullNewEntities.store(true);
		return;
	}
}

void RenderSystem::AddEntity(std::shared_ptr<Entity> e)
{
	//If we're pulling new entities soon, it should get added to the list anyway
	//if (_pullNewEntities.load()) return;

	const auto& rc = e->GetComponent(ComponentType::Render);
	if (!rc) return;
	const auto& castedRC = std::dynamic_pointer_cast<RenderComponent>(rc);
	std::unique_lock<std::mutex> lock(_entityLock);
	_renderables.push_back(RenderComponent(*castedRC));

	const auto& tc = e->GetComponent(ComponentType::Transform);
	if (!tc) { 
		_transforms.push_back(TransformComponent(
			DirectX::XMFLOAT3(0, 0, 0),
			DirectX::XMFLOAT3(0, 0, 0)));	
		return;
	}
	const auto& castedTC = std::dynamic_pointer_cast<TransformComponent>(tc);
	_transforms.push_back(TransformComponent(*castedTC));

}

void RenderSystem::UpdateEntity(std::shared_ptr<Entity> e)
{
	while (_pullNewEntities.load());

	const auto& rc = e->GetComponent(ComponentType::Render);
	if (!rc) return;
	const auto& castedRC = std::dynamic_pointer_cast<RenderComponent>(rc);

	std::unique_lock<std::mutex> lock(_entityLock);
	for (auto t = 0; t < _renderables.size(); ++t) {
		if (_renderables[t].Owner() == e) {
			_renderables[t] = RenderComponent(*castedRC);
			
			const auto& tc = e->GetComponent(ComponentType::Transform);
			if (!tc) {
				_transforms[t] = TransformComponent(
					DirectX::XMFLOAT3(0, 0, 0),
					DirectX::XMFLOAT3(0, 0, 0));
			}
			else {
				const auto& castedTC = std::dynamic_pointer_cast<TransformComponent>(tc);
				_transforms[t] = TransformComponent(*castedTC);
			}
			return;
		}
	}

}

void RenderSystem::Process()
{
	if (!_renderer) { return; }

	{
		std::unique_lock<std::mutex> renderLock(Game::TheGame->RenderMutex);
		_renderer->ClearBackBuffer();

		auto& storedMVM = _renderer->GetVP();
		auto mvm = DirectX::XMLoadFloat4x4(&storedMVM);
		for (auto i = 0; i < _renderables.size(); ++i) {
			if (!_renderables[i].Enabled()) continue;
			_renderer->Draw(_renderables[i], _transforms[i], mvm);
		}

		Game::TheGame->ShouldUIRender = true;
		Game::TheGame->ImGuiCV.notify_one();
		Game::TheGame->RenderCV.wait(renderLock, [] { return !Game::TheGame->ShouldUIRender.load(); });

		_renderer->SwapBuffers();
	}
	
	if (_pullNewEntities.load()) {
		std::unique_lock<std::mutex> updateLock(Game::TheGame->EntityMutex, std::try_to_lock);
		if (updateLock.owns_lock()) {
			std::cout << "Render system :: Pulling new entities" << std::endl;
			//TODO: Change this at some point cause it's inefficient but easy. 
			_renderables.clear();
			_transforms.clear();
			
			for (auto& e : Game::TheGame->GetEntities()) 
				AddEntity(e);
			//	
			_pullNewEntities.store(false);
		}
	}
}
