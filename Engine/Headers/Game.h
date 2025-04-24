#pragma once
#include <deque>
#include "gil.h"
#include "Timer.h"
#include "../imgui/imgui.h"

class Entity;
class Message;
class Window;
class Renderer;
class RenderSystem;
class NetworkSystem;
class NetworkHandler;
class CollisionSystem;
class SceneManager;

class Game {
public:
	static Game* TheGame;
	static constexpr SK::INT32 MAX_REN_FRAME_COUNT = 20;
	static constexpr SK::INT32 MAX_NET_FRAME_COUNT = 20;
	static constexpr SK::INT32 MAX_COL_FRAME_COUNT = 20;
private:
	std::deque<SK::FLOAT> _frameTimes, _netTimes, _collisionTimes;
protected:
	SK::BOOL _quit, _init = false;

	EntityList _entities;		//The "Golden" Copy
	std::thread _ImGuiThread;
	Timer _timer;
	std::shared_ptr<Renderer> _renderer;
	Window* _window;
	std::shared_ptr<RenderSystem> _renderSystem;
	std::shared_ptr<CollisionSystem> _collisionSystem;
	std::shared_ptr<NetworkSystem> _networkSystem;
	std::shared_ptr<NetworkHandler> _networkHandler; //used to process game-specifc msgs
	std::shared_ptr<SceneManager> _sceneManager;

	std::atomic<SK::BOOL> _shouldJoinImGui;
public:
	std::mutex RenderMutex, EntityMutex;
	std::condition_variable RenderCV, ImGuiCV;
	std::atomic<SK::BOOL> ShouldUIRender { false };

public:
	Game();
	virtual ~Game();

private:
	void AddFrameTime(const SK::FLOAT f);
	void AddNetTime(const SK::FLOAT f);
	void AddCollisionTime(const SK::FLOAT f);

public:

	nodconst SK::BOOL Quit() const { return _quit; }
	nodconst std::shared_ptr<Renderer> GetRenderer() { return _renderer; }
	nodconst EntityList& GetEntities() const { return _entities; }

	nodconst SK::INT32 GetGameWidth() const;
	nodconst SK::INT32 GetGameHeight() const;
	
	void AddEntities(std::vector<std::shared_ptr<Entity>>& v);
	void AddEntity(std::shared_ptr<Entity> obj);
	void SetQuit(SK::BOOL value) { _quit = value; }

	/// <summary>
	/// For best performance, add all entities to game before calling Initialise(...)
	/// </summary>
	virtual void Initialise(Window* window);
	virtual void Render() = 0;
	virtual void Run();
	virtual void ImGui();

	void SendMessage(const std::shared_ptr<Message> msg);
	void CommandSystemsToPull();
};