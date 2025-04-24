#include <iostream>
#include <Engine/Headers/SceneManager.h>
#include <Engine/Headers/InputManager.h>
#include <Engine/Systems/Headers/RenderSystem.h>
#include <Engine/Systems/Headers/CollisionSystem.h>
#include <Engine/Systems/Headers/NetworkSystem.h>
#include <Engine/Headers/ResourceManager.h>
#include <Engine/Headers/Renderer_DX.h>

#include "../Headers/SketchGame.h"
#include "../Headers/SketchScene.h"
#include "../Headers/SketchNetworkHandler.h"
#include "../Headers/SketchNetMessages.h"
#undef max

SketchGame::SketchGame() : _inputElapsed(0.0f), _inputFreq(60), _localPlayerSlot(std::numeric_limits<SK::UINT8>::max())
{
	_inputPollRate = 1.0f / static_cast<SK::FLOAT>(_inputFreq);

	_playerColourMap[0] = {0.0039f, 0.1098f, 0.1529f};
	_playerColourMap[1] = {0.2863f, 0.2235f, 0};
	_playerColourMap[2] = {0.1922f, 0.0667f, 0.0667f};
	_playerColourMap[3] = {0.0078f, 0.2392f, 0.0471f};
}

void SketchGame::Initialise(Window* w)
{
	Game::Initialise(w);

	_renderer->SetClearColour(DirectX::XMFLOAT4(0.2549f, 0.2471f, 0.3294f, 1.0f));
#ifdef BUILD_DX
	auto dxR = std::dynamic_pointer_cast<Renderer_DX>(_renderer);
	if (dxR) {
		ResourceManager::Instance().SetDevice(dxR->Device());
	}
#endif

	_sceneManager->Push(std::make_shared<SketchScene>(_renderer, *this));

	_renderSystem->StartSystem();
	_collisionSystem->StartSystem();

	_networkHandler = std::make_shared<SketchNetworkHandler>(*this);
	_networkSystem->SetHandler(_networkHandler);
	_networkSystem->StartSystem();

	//Edit: 5s is a little too SK::LONG for my liking, but I've run into issues
	//Where peers don't 'accept()' 'connect()' until late, and this causes problems joining games.
	//Send the looking to join game message a little after starting. Gives time to warm up connections.
	std::thread([this] {
#ifdef _DEBUG
		Sleep(5000);
#else
		Sleep(5000);
#endif
		SK::UINT16 expectedResponses = static_cast<SK::UINT16>(GetConnectionCount());
		std::cout << "SketchGame :: Expected responses: " << expectedResponses << '\n';
		auto joinReq = std::make_shared<PlayerJoinReqNetMessage>(expectedResponses, EVERYONE, 0, NetworkSystem::GetMessageID());
		Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, joinReq));
	}).detach();

}

void SketchGame::Render()
{
	//RenderSystem runs in another thread. This method is here... 
	//Because I might want to use it another time.
}

void SketchGame::Run()
{
	Game::Run();

	//_inputElapsed += _timer.dT();
	//if (_inputElapsed > _inputPollRate) {
	InputManager::Instance().Poll();
	//_inputElapsed = 0.0f;
	//}

	if (_sceneManager->CurrentScene() == nullptr) {
		SetQuit(true);
	}
}

nodconst SK::BYTE SketchGame::GetAvailablePlayerSlot() {
	for (auto i = 0; i < MAX_PLAYERS; ++i) {
		if (_playerMap.find(i) == _playerMap.end()) {
			return i;
		}
	}
	return std::numeric_limits<SK::UINT8>::max();
}

void SketchGame::RemovePlayer(Connection c) {
	SK::BYTE slotToRemove = -1;
	for (const auto& [k, v] : _playerMap) {
		if (c == v) {
			slotToRemove = k;
			break;
		}
	}
	if (slotToRemove >= 0) {
		try {
			std::cout << "Removing player : " << slotToRemove << '\n';
			_playerMap.erase(slotToRemove);
		}
		catch (std::exception& e) {
			std::cerr << "SketchGame :: Slot to remove was not a valid index into the player map.\n";
		}
	}
	else {
		std::cerr << "SketchGame :: Failed to remove " << c.first << ':' << c.second << " from player map.\n";
	}
}

nodconst SK::BYTE SketchGame::GetPlayerSlot(Connection c) {
	for (const auto& [k, v] : _playerMap) {
		if (c == v) {
			return k;
		}
	}
}

nodconst SK::SIZE_T SketchGame::GetConnectionCount() const {
	return _networkSystem->GetExternalConnectionCount();
}