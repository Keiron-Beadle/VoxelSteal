#pragma once
#include <Engine/Headers/Game.h>
#include <Engine/Headers/Renderer.h>
#include <Engine/Headers/Window.h>

class SketchGame : public Game
{
public:
	static constexpr SK::BYTE MAX_PLAYERS = 4;
	//maps a player idx to a connection in NetworkSystem
	std::unordered_map<SK::BYTE, Connection> _playerMap;
	std::unordered_map<SK::BYTE, DirectX::XMFLOAT3> _playerColourMap;

	SK::INT16 _inputFreq;
	SK::FLOAT _inputElapsed;
	SK::FLOAT _inputPollRate;
	SK::BYTE _localPlayerSlot;
public:
	SketchGame();
	virtual ~SketchGame() = default;

	nodconst std::shared_ptr<CollisionSystem> GetCollisionSystem() const { return _collisionSystem; }
	nodconst SK::SIZE_T GetConnectionCount() const;
	nodconst SK::SIZE_T GetPlayerCount() const { return _playerMap.size(); }
	void AddPlayer(Connection c, const SK::BYTE p) { _playerMap[p] = c; }
	void RemovePlayer(Connection c);

	nodconst SK::BYTE GetAvailablePlayerSlot();

	nodconst DirectX::XMFLOAT3 GetColour(const SK::BYTE slot) { return _playerColourMap[slot]; }
	nodconst SK::BYTE GetPlayerSlot(Connection c);
	nodconst SK::BYTE GetLocalPlayerSlot() const { return _localPlayerSlot; }
	void SetLocalPlayerSlot(SK::UINT8 slot) { _localPlayerSlot = slot; }

	void UpdateInputPollRate() { _inputPollRate = 1.0f/static_cast<SK::FLOAT>(_inputFreq); }

	virtual void Initialise(Window* w);
	virtual void Render();
	virtual void Run();
};

