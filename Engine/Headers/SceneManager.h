#pragma once
#include <stack>
#include "gil.h"

class RenderSystem;
class Scene;
class Game;

typedef std::stack<std::shared_ptr<Scene>> SceneStack;

class SceneManager : public std::enable_shared_from_this<SceneManager> {
protected:
	const Addressee Address = Addressee::SCENE;
	Game* _game;
	SceneStack _scenes;
public:
	SceneManager(Game* game);
	~SceneManager();

	[[nodiscard]] std::shared_ptr<Scene> CurrentScene() const { return _scenes.size() > 0 ? _scenes.top() : nullptr; }
	[[nodiscard]] Game* GetGame() const { return _game; }
	
	void AddEntity(std::shared_ptr<Entity> entity);
	EntityList& Entities();

	void OnMessage(std::shared_ptr<Message>);

	void Update(const SK::DOUBLE dT);

	void ImGui();

	void Render(std::shared_ptr<RenderSystem>);

	void Pop() { _scenes.pop(); }

	void Push(std::shared_ptr<Scene>);
};