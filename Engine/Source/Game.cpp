#include <iostream>
#include <algorithm>
#include "../Headers/Game.h"
#include "../Headers/Window.h"
#include "../Headers/SceneManager.h"
#include "../Systems/Headers/RenderSystem.h"
#include "../Systems/Headers/CollisionSystem.h"
#include "../Systems/Headers/NetworkSystem.h"
#include "../Headers/Message.h"

#include "../imgui/imgui_impl_win32.h"
#include "../imgui/imgui_impl_dx11.h"
#include "../Headers/Renderer_DX.h"
#undef SendMessage

Game* Game::TheGame = nullptr;

Game::Game()
	: _quit(false), _shouldJoinImGui(false)
{
	TheGame = this;
	_sceneManager = std::make_unique<SceneManager>(this);

	//Imgui setup
	auto ctx = ImGui::CreateContext();
	ImGui::SetCurrentContext(ctx);
	ImGui::StyleColorsDark();
	
}

Game::~Game() {
	//clear meshes or whatever, meshes should probably be handled by 
	//resource manager? 
	_renderSystem->CancelSystem();
	_collisionSystem->CancelSystem();
	_networkSystem->CancelSystem();

	_shouldJoinImGui.store(true);
	std::cout << "Game :: Destructor called.\n";
	RenderCV.notify_all();
	if (_ImGuiThread.native_handle()) {
		ImGuiCV.notify_all();
		_ImGuiThread.join();
	}
#ifdef BUILD_DX
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}

void Game::AddFrameTime(const SK::FLOAT f)
{
	_frameTimes.push_back(f);
	if (_frameTimes.size() > MAX_REN_FRAME_COUNT) {
		_frameTimes.pop_front();
	}
}

void Game::AddNetTime(const SK::FLOAT f)
{
	_netTimes.push_back(f);
	if (_netTimes.size() > MAX_NET_FRAME_COUNT) {
		_netTimes.pop_front();
	}
}

void Game::AddCollisionTime(const SK::FLOAT f) {
	_collisionTimes.push_back(f);
	if (_collisionTimes.size() > MAX_COL_FRAME_COUNT) {
		_collisionTimes.pop_front();
	}
}

void Game::Initialise(Window* window) {
	_window = window;
	_renderer = _window->GetRenderer();
	_renderSystem = std::make_shared<RenderSystem>(_entities);
	_collisionSystem = std::make_shared<CollisionSystem>(_entities);
	_networkSystem = std::make_shared<NetworkSystem>();
	_renderSystem->SetRenderer(_renderer);
	
	//init imgui
#ifdef BUILD_DX
	ImGui_ImplWin32_Init(reinterpret_cast<void*>(window->GetHWND()));
	auto dxR = std::dynamic_pointer_cast<Renderer_DX>(_renderer);
	ImGui_ImplDX11_Init(dxR->Device(), dxR->Context());
#endif

	_ImGuiThread = std::thread([this]() { 
		while (!_shouldJoinImGui.load()) {
			try {
				ImGui();
			}
			catch (const std::exception& e) {
				std::cerr << "ImGui :: Errored :: " << e.what() << std::endl;
			}
		}	
		std::cout << "ImGui :: Joining" << std::endl;
		std::cout << "ImGui :: Should Join? " << _shouldJoinImGui.load() << std::endl;
	});

	_init = true;
}

void Game::AddEntities(std::vector<std::shared_ptr<Entity>>& v)
{
	{
		std::unique_lock<std::mutex> lock(EntityMutex);
		_entities.insert(_entities.end(), v.begin(), v.end());
	}

	if (_init) {
		CommandSystemsToPull();
	}
}

void Game::AddEntity(std::shared_ptr<Entity> obj)
{
	{
		std::unique_lock<std::mutex> lock(EntityMutex);
		_entities.push_back(obj);
	}

	if (_init) {
		CommandSystemsToPull();
	}
}

nodconst SK::INT32 Game::GetGameWidth() const {
	return _window->GetWidth();
}

nodconst SK::INT32 Game::GetGameHeight() const {
	return _window->GetHeight();
}

void Game::SendMessage(const std::shared_ptr<Message> message) {
	_sceneManager->OnMessage(message);
	_renderSystem->OnMessage(message);
	_collisionSystem->OnMessage(message);
	_networkSystem->OnMessage(message);

	if ((message->Type() & Addressee::ENTITY) != 0) {
		for (auto& e : _entities) {
			e->OnMessage(message);
		}
	}
}

void Game::CommandSystemsToPull()
{
	PullEntityPayload p;
	SendMessage(std::make_shared<PullEntityMessage>(Addressee::ALL, p));
}

void Game::Run() {
	_timer.Tick();

	_sceneManager->Update(_timer.dT());
}

void Game::ImGui()
{
	std::unique_lock<std::mutex> lock(RenderMutex);
	ImGuiCV.wait(lock, [this] { return ShouldUIRender.load() || _shouldJoinImGui.load(); });
	if (_shouldJoinImGui.load()) {
		RenderCV.notify_one();
		return;
	}

#ifdef BUILD_DX
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
#endif
	ImGui::NewFrame();
	_sceneManager->ImGui();

	auto renderFreq = _renderSystem->GetActualFrequency();
	std::string renderFPS = "Render FPS: " + std::to_string(renderFreq);
	AddFrameTime(renderFreq);

	auto netFreq = _networkSystem->GetActualFrequency();
	std::string netFPS = "Network TPS: " + std::to_string(netFreq);
	AddNetTime(netFreq);

	auto colFreq = _collisionSystem->GetActualFrequency();
	std::string colFPS = "Collision TPS: " + std::to_string(colFreq);
	AddCollisionTime(colFreq);

	SK::FLOAT frameTimes[MAX_REN_FRAME_COUNT];
	SK::FLOAT minFrame = std::numeric_limits<SK::FLOAT>::max(), maxFrame = 0;
	for (auto i = 0; i < _frameTimes.size(); ++i) {
		frameTimes[i] = _frameTimes[i];
		if (frameTimes[i] < minFrame) minFrame = frameTimes[i];
		if (frameTimes[i] > maxFrame) maxFrame = frameTimes[i];
	}
	ImGui::PlotLines("", frameTimes, _frameTimes.size(),
		0, renderFPS.c_str(), std::max(0.0f, minFrame - 20), maxFrame + 20, ImVec2(0.0f, 40.0f));

	SK::FLOAT netTimes[MAX_NET_FRAME_COUNT];
	SK::FLOAT minNet = std::numeric_limits<SK::FLOAT>::max(), maxNet = 0;
	for (auto i = 0; i < _netTimes.size(); ++i) {
		netTimes[i] = _netTimes[i];
		if (netTimes[i] < minNet) minNet = netTimes[i];
		if (netTimes[i] > maxNet) maxNet = netTimes[i];
	}
	ImGui::PlotLines("", netTimes, _netTimes.size(),
		0, netFPS.c_str(), std::max(0.0f, minNet - 20), maxNet + 20, ImVec2(0.0f, 40.0f));

	SK::FLOAT colTimes[MAX_COL_FRAME_COUNT];
	SK::FLOAT minCol = std::numeric_limits<SK::FLOAT>::max(), maxCol = 0;
	for (auto i = 0; i < _collisionTimes.size(); ++i) {
		colTimes[i] = _collisionTimes[i];
		if (colTimes[i] < minCol) minCol = colTimes[i];
		if (colTimes[i] > maxCol) maxCol = colTimes[i];
	}
	ImGui::PlotLines("", colTimes, _collisionTimes.size(), 0, colFPS.c_str(),
		std::max(0.0f, minCol - 20), maxCol + 20, ImVec2(0.0f, 40.0f));

	//ImGui_ImplDX11_NewFrame();
	//ImGui_ImplWin32_NewFrame();
	///ImGui::NewFrame();
	//ImGui::ShowDemoWindow();
	ImGui::End();
	ImGui::Render();
#ifdef BUILD_DX
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif

	ShouldUIRender = false;
	RenderCV.notify_one();
}
