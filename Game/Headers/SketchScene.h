#pragma once
#include <Engine/Headers/Scene.h>
#include <Engine/Components/Headers/ColliderComponent.h>
#include <Engine/Components/Headers/OctTree.h>
#include "SketchGame.h"
#include "VoxelCanvas.h"
#undef max

using namespace DirectX;
class SketchScene : public Scene
{
private:
	std::string fpsString;
	std::unique_ptr<VoxelCanvas> _canvas;
	std::shared_ptr<Entity> _mouse;
	std::shared_ptr<ColliderComponent> _mouseCollider;
	std::shared_ptr<Renderer> _renderer;
	std::shared_ptr<Octree> _canvasOctree;
	SketchGame& _game;
	XMFLOAT4 _eye;
	XMFLOAT4 _lookAt;
	XMFLOAT4 _up;
	XMFLOAT4X4 _view, _proj, _invVP;

	const SK::FLOAT _fov = 3.14f / 2.0f;
	const SK::FLOAT NEARPLANE = 0.01f;
	const SK::FLOAT FARPLANE = 100.0f;
	const SK::FLOAT CAM_ROTATE_SPEED = 1.0f;

	SK::INT32 _localMassTotal = 0;
	SK::INT32 _remoteMassTotal = 0;

	SK::INT16 _reqRenderFreq = 60;
	SK::INT16 _actRenderFreq = std::numeric_limits<SK::INT16>::max(); //undefined at start
	SK::INT16 _reqRenderCore = 1;

	SK::INT16 _numPlayers = 0; //undefined at start

	SK::INT16 _reqNetFreq = 20; 
	SK::INT16 _actNetFreq = std::numeric_limits<SK::INT16>::max(); //undefined at start
	SK::INT16 _reqNetCore = 2;

	SK::INT16 _reqColFreq = 60;
	SK::INT16 _actColFreq = std::numeric_limits<SK::INT16>::max(); //undefined at start
	SK::INT16 _reqColCore = 3;
	
	SK::BOOL _firstNoneLeftClickedFrame = false;
	SK::BOOL _integrityMode = false;
public:
	SketchScene(std::shared_ptr<Renderer> r, SketchGame& game);
	virtual ~SketchScene();

	void Initialise() override;
	void OnMessage(std::shared_ptr<Message> msg);
	void Update(const SK::DOUBLE dT) override;
	void Render(std::shared_ptr<RenderSystem> rs) override;
	void ImGui() override;
	void VoxelCollisionHandler(ColliderComponent& sta, SK::SIZE_T t);

	void IncMass() { ++_localMassTotal; };
	void DecMass() { --_localMassTotal; }

	void DoInputResponse(const SK::DOUBLE dT);
	void DoMouseRaycast();
	void DoRotateCamera(const SK::DOUBLE& dt);
	void DoCameraZoom(const SK::DOUBLE& dt);
	void ToggleIntegrity();
	void SwitchToNormalMode();
	void SwitchToIntegrityMode();

	void Reset();
};

