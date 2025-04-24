#pragma once
#include "System.h"
#include "../../Headers/Renderer.h"



class RenderSystem : public System {
protected:
	RenderList _renderables;
	TransformList _transforms;
	std::shared_ptr<Renderer> _renderer;

public:
	RenderSystem(EntityList& entities);
	virtual ~RenderSystem();

	nodconst std::shared_ptr<Renderer> GetRenderer()  const { return _renderer; }
	void SetRenderer(std::shared_ptr<Renderer> renderer) { _renderer = renderer; }
	
	void OnMessage(std::shared_ptr<Message> message) override;
	void AddEntity(std::shared_ptr<Entity> e) override;
	void UpdateEntity(std::shared_ptr<Entity> e) override;
	void Process() override;
};