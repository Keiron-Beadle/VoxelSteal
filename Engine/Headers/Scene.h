#pragma once
#include "gil.h"

class SceneManager;
class RenderSystem;

class Scene {
protected:
	std::shared_ptr<SceneManager> _sceneManager;

public:
	Scene();
	virtual ~Scene();

	void SetSceneManager(std::shared_ptr<SceneManager> s) { _sceneManager = s; }
	nodconst std::shared_ptr<SceneManager> GetSceneManager() { return _sceneManager; }

	virtual void Initialise() = 0;
	virtual void OnMessage(std::shared_ptr<Message> msg);
	virtual void Update(const SK::DOUBLE dT);
	virtual void Render(std::shared_ptr<RenderSystem> rs) = 0;
	virtual void ImGui() {};

};