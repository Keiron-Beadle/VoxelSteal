#include "../Headers/SceneManager.h"
#include "../Headers/Scene.h"
#include "../Headers/Message.h"

SceneManager::SceneManager(Game* game)
	: _game(game)
{
}

SceneManager::~SceneManager()
{
}

void SceneManager::OnMessage(std::shared_ptr<Message> msg)
{
	if ((Address & msg->Type()) == 0) return;
	auto current = CurrentScene();
	if (!current) return;
	current->OnMessage(msg);
}

void SceneManager::Update(const SK::DOUBLE dT)
{
	auto current = CurrentScene();
	if (!current) return;
	current->Update(dT);
}

void SceneManager::ImGui()
{
	auto current = CurrentScene();
	if (!current) return;
	current->ImGui();
}

void SceneManager::Render(std::shared_ptr<RenderSystem> rs)
{
	auto current = CurrentScene();
	if (!current) return;
	current->Render(rs);
}

void SceneManager::Push(std::shared_ptr<Scene> s)
{
	_scenes.push(s);
	s->SetSceneManager(shared_from_this());
	s->Initialise();
}
