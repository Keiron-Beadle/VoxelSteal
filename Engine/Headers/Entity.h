#pragma once
#include "gil.h"

class Component;


class Entity
{
private:
	static constexpr Addressee Address{ENTITY};
protected:
	std::string _name;
	ComponentMap _components;
	ListenerMap _messageListeners;
	SK::BOOL _enabled, _delete, _listeningAtAll;
public:
	Entity(std::string name);
	virtual ~Entity();

	nodconst std::string Name() const { return _name; }
	nodconst SK::BOOL Enabled() const { return _enabled; }
	nodconst SK::BOOL DeleteFlag() const { return _delete; }

	nodconst SK::BOOL ToggleEnable() { return _enabled = !_enabled; }
	void SetDelete() { _delete = true; }

	SK::BOOL AddComponent(const std::shared_ptr<Component> comp);
	SK::BOOL RemoveComponent(const std::shared_ptr<Component> comp);
	SK::BOOL RemoveComponent(const ComponentType compType);
	nodconst std::shared_ptr<Component> GetComponent(ComponentType compType);

	void RegisterListener(const Addressee& msg, std::shared_ptr<Component> comp);
	void UnregisterListener(const Addressee& msg, std::shared_ptr<Component> comp);

	virtual void Start();
	virtual void Update(const SK::DOUBLE dT);

	virtual void OnMessage(const std::shared_ptr<Message> msg);
	virtual void End();
	virtual void Reset();

private:
	Entity(const Entity&);
	Entity& operator=(const Entity&);
};

