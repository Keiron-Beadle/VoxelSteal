#include "../Headers/Entity.h"
#include "../Components/Headers/Component.h"
#include "../Components/Headers/RenderComponent.h"
#include "../Headers/Message.h"

Entity::Entity(std::string name)
	: _name(name), _delete(false), _enabled(true), _listeningAtAll(false)
{

}

Entity::~Entity() {
	End();
}

SK::BOOL Entity::AddComponent(const std::shared_ptr<Component> comp) {
	auto [it, inserted] = _components.emplace(comp->Type(), comp);
	return inserted;
}

SK::BOOL Entity::RemoveComponent(const std::shared_ptr<Component> comp) {
	return RemoveComponent(comp->Type());
}

SK::BOOL Entity::RemoveComponent(const ComponentType type) {
	return _components.erase(type) > 0 ? true : false;
}

nodconst std::shared_ptr<Component> Entity::GetComponent(ComponentType compType)
{
	auto it = _components.find(compType);
	if (it != _components.end()) {
		return it->second;
	}
	return nullptr;
}

void Entity::RegisterListener(const Addressee& msg, std::shared_ptr<Component> comp)
{
	_listeningAtAll = true;
	auto it = _messageListeners.find(msg);

	if (it == _messageListeners.end()) {
		_messageListeners[msg] = std::vector<std::shared_ptr<Component>>();
		_messageListeners[msg].push_back(comp);
		return;
	}
	_messageListeners[msg].push_back(comp);
}

void Entity::UnregisterListener(const Addressee& msg, std::shared_ptr<Component> comp)
{
	auto it = _messageListeners.find(msg);

	if (it != _messageListeners.end()) {
		auto& list = it->second;
		for (auto cit = list.begin(); cit != list.end(); ++cit) {
			if (*cit == comp) {
				list.erase(cit);
				break;
			}
		}
	}
}

void Entity::Start()
{
	for (auto i = _components.begin(); i != _components.end(); ++i) {
		i->second->Start();
	}
}

void Entity::Update(const SK::DOUBLE dT) {
	for (auto i = _components.begin(); i != _components.end(); ++i) {
		i->second->Update(dT);
	}
}

void Entity::OnMessage(const std::shared_ptr<Message> msg) {
	
	if ((msg->Type() & Address) == 0) return;


	auto rcChange = std::dynamic_pointer_cast<ChangeRenderComponentMessage>(msg);
	if (rcChange) {
		auto cItr = _components.find(ComponentType::Render);
		if (cItr == _components.end()) { return; }
		auto& payload = rcChange->GetPayload();
		auto rc = std::dynamic_pointer_cast<RenderComponent>(cItr->second);
		DirectX::XMFLOAT4 col = { payload.R, payload.G, payload.B, 1 };
		rc->SetColour(col);
	}

	auto rcChangeAndEnable = std::dynamic_pointer_cast<ChangeRenderComponentColourAndEnabledMessage>(msg);
	if (rcChangeAndEnable) {
		auto cItr = _components.find(ComponentType::Render);
		if (cItr == _components.end()) { return; }
		auto& payload = rcChangeAndEnable->GetPayload();
		auto rc = std::dynamic_pointer_cast<RenderComponent>(cItr->second);
		DirectX::XMFLOAT4 col = { payload.R, payload.G, payload.B, 1 };
		auto enabled = payload.Enabled;
		rc->SetColour(col);
		rc->SetEnabled(enabled);
	}
	//if (!_listeningAtAll) return;

	//ListenerMapIterator it = _messageListeners.find(msg->Type());

	//if (it == _messageListeners.end()) { return; }

	//auto& list = it->second;
	//for (auto i = list.begin(); i != list.end(); ++i) {
	//	(*i)->OnMessage(msg);
	//}
}

void Entity::End() {
	for (auto i = _components.begin(); i != _components.end(); ++i) {
		auto& comp = i->second;
		comp->End();
	}
	_components.clear();
}

void Entity::Reset()
{
}
