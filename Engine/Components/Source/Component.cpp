#include "../Headers/Component.h"
#include "../../Headers/Entity.h"
#include "../../Headers/Game.h"

Component::Component(ComponentType type, std::shared_ptr<Entity> entity)
	: _entity(entity), _type(type), _enabled(true) {
}

Component::Component(const Component& c) {
	_entity = c._entity;
	_type = c._type;
	_enabled = c._enabled;
}

void Component::SendMessage(const std::shared_ptr<Message> msg) {
	Game::TheGame->SendMessage(msg);
}