#pragma once
#include "../../Headers/gil.h"
#include "../../Headers/Observer.h"
#include "../../Headers/Entity.h"
#include <optional>

class Component : public Observer, std::enable_shared_from_this<Component>
{
private:
	std::shared_ptr<Entity> _entity;
	ComponentType _type;
protected:
	SK::BOOL _enabled;
public:
	Component(ComponentType type, std::shared_ptr<Entity> owner = nullptr);
	virtual ~Component() = default;
	Component(const Component&);

	template <typename T>
	std::optional<std::shared_ptr<T>> GetComponent(ComponentType type) {
		std::shared_ptr<T> component = std::dynamic_pointer_cast<T>(_entity->GetComponent(type));
		if (component)
			return std::move(component);  // Transfer ownership using std::move
		return std::nullopt;
	}

	nodconst SK::BOOL Enabled() const { return _enabled; }
	void SetEnabled(const SK::BOOL e) { _enabled = e; }
	nodconst ComponentType Type() const { return _type; }
	nodconst std::shared_ptr<Entity> Owner() const { return _entity; }
	void SetOwner(std::shared_ptr<Entity> entity) { _entity = entity; }

	virtual void Start() = 0;
	virtual void Update(const SK::DOUBLE dT) = 0;
	virtual void End() = 0;
	void SendMessage(const std::shared_ptr<Message> msg);
};