#include <iostream>
#include <Engine/Headers/Game.h>
#include <Engine/Headers/Message.h>
#include <Engine/Headers/Entity.h>
#include <Engine/Systems/Headers/NetworkSystem.h>
#include <Engine/Systems/Headers/CollisionSystem.h>
#include <Engine/Components/Headers/TransformComponent.h>
#include <Engine/Components/Headers/RenderComponent.h>
#include <Engine/Components/Headers/AABBCollider.h>
#include <Engine/Components/Headers/RayCollider.h>
#include <Engine/Components/Headers/ColliderComponent.h>
#include <Engine/Headers/Window.h>
#include <Engine/Headers/InputManager.h>
#include <Engine/Headers/ResourceManager.h>
#include <Engine/Headers/Renderer_DX.h>
#undef SendMessage

#include "../Headers/SketchScene.h"
#include "../Headers/SketchNetMessages.h"
#include "../Headers/SketchMessages.h"

#ifdef BUILD_DX
#include <Engine/imgui/imgui_impl_dx11.h>
#include <Engine/imgui/imgui_impl_win32.h>
#endif

constexpr SK::UINT32 WIDTH = 128;
constexpr SK::UINT32 HEIGHT = 128;
constexpr SK::FLOAT SCALE = 0.5f;

SketchScene::SketchScene(std::shared_ptr<Renderer> r, SketchGame& game)
	: _eye(0, 46.0, 1.5, 1), _lookAt(0, 0, 0, 1), _renderer(r), _game(game), _up(0,1,0,0)
{
	auto gamePtr = Game::TheGame;
	auto width = gamePtr->GetGameWidth();
	auto windowWidth = static_cast<SK::FLOAT>(Game::TheGame->GetGameWidth());
	auto windowHeight = static_cast<SK::FLOAT>(Game::TheGame->GetGameHeight());
	auto ratio = windowWidth / windowHeight;
	auto viewM = XMMatrixLookAtLH(XMLoadFloat4(&_eye), XMLoadFloat4(&_lookAt), XMLoadFloat4(&_up));
	auto projM = XMMatrixPerspectiveFovLH(_fov, ratio, NEARPLANE, FARPLANE);
	XMStoreFloat4x4(&_view, viewM);
	XMStoreFloat4x4(&_proj, projM);
	XMStoreFloat4x4(&_invVP, DirectX::XMMatrixInverse(nullptr, viewM * projM));
	_renderer->SetProjection(_proj);
	_renderer->SetView(_view);
}

SketchScene::~SketchScene()
{
}

void SketchScene::Initialise()
{
	auto& rm = ResourceManager::Instance(); 
	auto cubeVertices = rm.loadCube();							//Start async loading of cube vertices
	auto shaderFuture = rm.loadShaderFromFile("skShader");		//Start async load of shader

	DirectX::XMFLOAT3 octreeMin = { -50,-15,-50 };
	DirectX::XMFLOAT3 octreeMax = { 50, 15, 50 };
	_canvasOctree = std::make_shared<Octree>(octreeMin, octreeMax);
	_canvas = std::make_unique<VoxelCanvas>(*this, 0, 0, WIDTH, HEIGHT, SCALE);//Do some init work for canvas while cube loads
	_localMassTotal = _canvas->GetTransforms().size();
	//_canvas = std::make_unique<VoxelCanvas>(*this, 0, 0, 512, 512, 1.5);//Do some init work for canvas while cube loads
	auto& transforms = _canvas->GetTransforms();			//Get the canvas transforms
	cubeVertices.wait();										//wait until cube is loaded
	auto mesh = std::make_shared<Mesh>(cubeVertices.get());		//Create a mesh from cube vertices
	mesh->CreateVBO(_renderer);									//Create VBO for the mesh

	EntityList batchedEntities;

	//<----------- CREATE MOUSE ENTITY -------------->
	_mouse = std::make_shared<Entity>("Mouse");
	auto rayCollider = std::make_shared<RayCollider>(DirectX::XMFLOAT3(0, 5, 0), DirectX::XMFLOAT3(0, -1, 0));
	_mouseCollider = std::make_shared<ColliderComponent>(rayCollider, _mouse);
	_mouse->AddComponent(_mouseCollider);
	batchedEntities.push_back(_mouse);
	_game.GetCollisionSystem()->AddEntity(_mouse);
	//<----------- ------------------ -------------->


	//<----------- CREATE VOXELS-------------------->
	for (SK::SIZE_T t = 0; t < transforms.size(); ++t) {
		auto e = std::make_shared<Entity>("Cube" + std::to_string(t));

		auto rc = std::make_shared<RenderComponent>(e);

		transforms[t]->SetOwner(e);
		auto pos = transforms[t]->Position();
		auto scale = transforms[t]->Scale();

		auto boxCollider = std::make_shared<AABBCollider>(
			DirectX::XMFLOAT3(pos.x - 0.5f * scale.x, pos.y - 0.5f * scale.y, pos.z - 0.5f * scale.z),
			DirectX::XMFLOAT3(pos.x + 0.5f * scale.x, pos.y + 0.5f * scale.y, pos.z + 0.5f * scale.z)
			);

		std::shared_ptr<ColliderComponent> cc = std::make_shared<ColliderComponent>(boxCollider, e);
		cc->SetCollisionHandler([this, t, cc]() {VoxelCollisionHandler(*cc, t); });
		_canvasOctree->Insert(cc);

		rc->SetMesh(mesh);
		e->AddComponent(transforms[t]);
		e->AddComponent(rc);
		e->AddComponent(cc);
		e->RegisterListener(Addressee::ENTITY, rc);
		batchedEntities.push_back(e);
	}
	_game.GetCollisionSystem()->AddCollider(_canvasOctree);
	//<----------- ------------------ -------------->

	//<----------- CREATE SHADERS -------------->

	shaderFuture.wait();		//When shader is finished loading, complete init with _renderer.
	if (!shaderFuture.valid()) {
		throw std::exception("Invalid shader result.");
	}
	auto shader = shaderFuture.get();
	std::vector<D3D11_INPUT_ELEMENT_DESC> vertexLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOUR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

#ifdef BUILD_DX
	auto dxR = std::dynamic_pointer_cast<Renderer_DX>(_renderer);
	if (dxR) {
		HRESULT hr = shader.SetInputLayout(vertexLayout, dxR->Device());
		if (FAILED(hr)) throw std::exception("Failed to set input layout.");
		shader.Use(dxR->Context());
	}
#endif
	//<----------- ------------------ -------------->

	Game::TheGame->AddEntities(batchedEntities);
}

void SketchScene::OnMessage(std::shared_ptr<Message> msg)
{	
	auto stealVoxelReq = std::dynamic_pointer_cast<StealVoxelReqMessage>(msg);
	if (stealVoxelReq) {
		auto& payload = stealVoxelReq->GetPayload();
		auto idx = payload.VoxelIdx;
		auto resId = payload.responseID;
		auto connection = payload.c;
		//Check if we have the voxel.
		if (!_canvas->HasVoxel(idx)) {
			//Say no you can't have the voxel to other peer
			auto responsePayload = std::make_shared<StealVoxelResNetMessage>(resId, false, payload.VoxelIdx, connection.first, connection.second, NetworkSystem::GetMessageID());
			Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, responsePayload));
			return;
		}
		auto playerSlot = _game.GetPlayerSlot(connection);
		auto responsePayload = std::make_shared<StealVoxelResNetMessage>(resId, true, payload.VoxelIdx,connection.first, connection.second, NetworkSystem::GetMessageID());
		Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, responsePayload));
		_canvas->RemoveMassFromIndex(idx);
		return;
	}

	auto stolenVoxelRes = std::dynamic_pointer_cast<StolenVoxelMessage>(msg);
	if (stolenVoxelRes) {
		auto& payload = stolenVoxelRes->GetPayload();
		auto idx = payload.VoxelIdx;
		auto player = _game.GetPlayerSlot(payload.c);
		auto colour = _game.GetColour(player);
		_canvas->AddMassToIndex(idx);
		auto& tc = _canvas->GetTransformAtIndex(idx);
		auto optRc = tc->GetComponent<RenderComponent>(ComponentType::Render);
		if (!optRc.has_value()) { std::cerr << "SketchScene :: Stolen Voxel Response -- Failed to get Render Component.\n"; return; }
		auto& rc = optRc.value();
		rc->AddColour(DirectX::XMFLOAT4(colour.x, colour.y, colour.z, 0.0f));

		UpdateEntityPayload p{ rc->Owner() };
		_game.SendMessage(std::make_shared<UpdateEntityMessage>(Addressee::RENDER_SYSTEM, p));
		//Don't know if we can do this. Might be too inefficient.
		//_game.CommandSystemsToPull();
		return;
	}

	auto integrityData = std::dynamic_pointer_cast<IntegrityDataMessage>(msg);
	if (integrityData) {
		auto& payload = integrityData->GetPayload();
		auto dataSize = payload.NumOfVoxels;
		auto& data = payload.VoxelData;
		auto idx = payload.StartVoxelIndex;
		_canvas->AddSliceToCache(idx, data, dataSize);
		return;
	}

	auto updateEntity = std::dynamic_pointer_cast<UpdateEntityMessage>(msg);
	if (updateEntity) {
		auto& payload = updateEntity->GetPayload();
		_canvasOctree->UpdateEntity(payload);
		return;
	}

	auto drawBlockCast = std::dynamic_pointer_cast<DrawBlockMessage>(msg);
	if (drawBlockCast) {
		auto& payload = drawBlockCast->GetPayload();
		auto& tc = _canvas->GetTransformAtCoordinate(payload.x, payload.y);
		auto optRc = tc->GetComponent<RenderComponent>(ComponentType::Render);
		if (!optRc.has_value()) { std::cerr << "Error getting render component on draw block message SketchScene."; return; }
		auto& rc = optRc.value();
		auto playerCol = _game.GetColour(payload.player);
		DirectX::XMFLOAT4 col{ playerCol.x, playerCol.y, playerCol.z, 1 };
		rc->SetColour(col);
		UpdateEntityPayload p{ tc->Owner() };
		Game::TheGame->SendMessage(std::make_shared<UpdateEntityMessage>(Addressee::RENDER_SYSTEM, p));
		return;
	}

	auto playerJoined = std::dynamic_pointer_cast<PlayerJoinMessage>(msg);
	if (playerJoined) {
		auto& payload = playerJoined->GetPayload();
		_game.AddPlayer(payload.c, payload.playerSpot);
		++_numPlayers;
		if (payload.c.first == 0) {
			_game.SetLocalPlayerSlot(payload.playerSpot);
			//Set our colour babyyyyy
			auto col = _game.GetColour(payload.playerSpot);
			ChangeColourPayload p{ col.x,col.y,col.z };
			Game::TheGame->SendMessage(std::make_shared<ChangeColourMessage>(Addressee::SCENE, p));
		}
		return;
	}

	auto playerLeft = std::dynamic_pointer_cast<PlayerLeftMessage>(msg);
	if (playerLeft) {
		auto& payload = playerLeft->GetPayload();
		_game.RemovePlayer(payload.c);
		--_numPlayers;
		return;
	}

	auto changeColour = std::dynamic_pointer_cast<ChangeColourMessage>(msg);
	if (changeColour) {
		auto& payload = changeColour->GetPayload();
		std::thread([payload, this]() {
			const auto& entities = _game.GetEntities();
			ChangeRenderComponentColourPayload ePayload{ payload.R, payload.G, payload.B };
			auto msg = std::make_shared<ChangeRenderComponentMessage>(Addressee::ENTITY, ePayload);
			{
				std::unique_lock<std::mutex> lock(_game.EntityMutex);
				for (auto& e : entities)
					e->OnMessage(msg);
			}
			_game.CommandSystemsToPull();
		}).detach();
		return;
	}

	auto startIntegrity = std::dynamic_pointer_cast<StartIntegrityMessage>(msg);
	if (startIntegrity) {
		std::thread([this]() {
			SK::UINT32 idx = 0;
			SK::UINT32 dataSize = 0;
			std::vector<SK::UINT8> data;
			std::cout << "Started sending integrity data\n";
			do {
				SK::UINT32 start = idx;
				_canvas->GetSliceOfMassBuffer(idx, data, dataSize);
				auto p = std::make_shared<IntegrityDataNetMessage>(start,
					EVERYONE, 0, NetworkSystem::GetMessageID(), dataSize, data);
				_game.SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, p));
			} while (dataSize >= REC_TCP_BUFFER_SIZE - 29);
			std::cout << "Finished sending integrity data\n";
		}).detach();
	}

	auto switchMode = std::dynamic_pointer_cast<SwitchSketchModeMessage>(msg);
	if (switchMode) {
		auto& payload = switchMode->GetPayload();
		switch (payload.mode) {
		case 0:
			SwitchToNormalMode();
			break;
		case 1:
			SwitchToIntegrityMode();
			break;
		}
		return;
	}

	auto visualiseIntegrity = std::dynamic_pointer_cast<VisualiseIntegrityMessage>(msg);
	if (visualiseIntegrity) {
		std::thread([this]() {
			auto& integrityCache = _canvas->GetIntegrityCache();
			auto numPlayers = _game.GetPlayerCount();
			SK::UINT32 idx = 0;
			{
				std::unique_lock<std::mutex> lock(_game.EntityMutex);
				auto& entities = _game.GetEntities();
				for (auto& e : entities) {
					auto rc = std::dynamic_pointer_cast<RenderComponent>(e->GetComponent(ComponentType::Render));
					if (!rc) continue;
					if (integrityCache[idx] < numPlayers) {
						rc->SetColour(DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));
					}
					else if (integrityCache[idx] > numPlayers) {
						rc->SetColour(DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
					}
					++idx;
				}
			}
			_game.CommandSystemsToPull();

		}).detach();
	}

	//auto connectionDied = std::dynamic_pointer_cast<ConnectionDiedMessage>(msg);
	//if (connectionDied) {
	//	auto& payload = connectionDied->GetPayload();
	//	_game.RemovePlayer(payload.connection);
	//	return;
	//}
}

void SketchScene::Update(const SK::DOUBLE dT)
{
	Scene::Update(dT);

	if (InputManager::Instance().GetActive()) {
		DoInputResponse(dT);
	}
}

void SketchScene::DoInputResponse(const SK::DOUBLE dT) {
	auto& inputManager = InputManager::Instance();
	if (inputManager.LeftClicked()) {
		_mouseCollider->SetEnabled(true);
		DoMouseRaycast();
		UpdateEntityPayload p{ _mouse };
		Game::TheGame->SendMessage(std::make_shared<UpdateEntityMessage>(Addressee::COLLISION_SYSTEM, p));
		_firstNoneLeftClickedFrame = true;
	}
	else if (_firstNoneLeftClickedFrame) {
		_firstNoneLeftClickedFrame = false;
		_mouseCollider->SetEnabled(false);
		UpdateEntityPayload p{ _mouse };
		Game::TheGame->SendMessage(std::make_shared<UpdateEntityMessage>(Addressee::COLLISION_SYSTEM, p));
	}

	if (inputManager.KeyDown(KeyCode::Left) || inputManager.KeyDown(KeyCode::A)) {
		DoRotateCamera(-dT);
	}
	if (inputManager.KeyDown(KeyCode::Right) || inputManager.KeyDown(KeyCode::D)) {
		DoRotateCamera(dT);
	}
	if (inputManager.KeyDown(KeyCode::Up)) {
		DoCameraZoom(dT);
	}
	if (inputManager.KeyDown(KeyCode::Down)) {
		DoCameraZoom(-dT);
	}
	if (inputManager.KeyPressed(KeyCode::R)) {
		Reset();
	}
	if (inputManager.KeyPressed(KeyCode::M)) {
		_integrityMode = !_integrityMode;
		ToggleIntegrity();
	}
}

void SketchScene::DoMouseRaycast() {
	auto& inputManager = InputManager::Instance();
	//Get direction
	auto mousePos = inputManager.GetMousePosition();
	auto ndcX = (2.0f * mousePos.x) / Window::TheWindow->GetWidth() - 1.0f;
	auto ndcY = 1.0f - (2.0f * mousePos.y) / Window::TheWindow->GetHeight();

	if (ndcX < -1.0f || ndcX > 1.0f || ndcY < -1.0f || ndcY > 1.0f) return;
	DirectX::XMFLOAT3 rayNearClip(ndcX, ndcY, 0.1f);
	DirectX::XMFLOAT3 rayFarClip(ndcX, ndcY, 1.0f);
	DirectX::XMFLOAT3 rayNear, rayFar;
	auto invVP = XMLoadFloat4x4(&_invVP);
	DirectX::XMStoreFloat3(&rayNear, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&rayNearClip), invVP));
	DirectX::XMStoreFloat3(&rayFar, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&rayFarClip), invVP));

	DirectX::XMFLOAT3 rayDirection;
	XMStoreFloat3(&rayDirection, XMVector4Normalize(XMLoadFloat3(&rayFar) - XMLoadFloat3(&rayNear)));

	auto c = std::make_shared<RayCollider>(rayNear, rayDirection);
	_mouseCollider->SetCollider(c);
}

void SketchScene::Render(std::shared_ptr<RenderSystem> rs)
{

}

void SketchScene::ImGui()
{
	ImGui::Begin("Info Panel");

	std::string playerText = "Players: " + std::to_string(_numPlayers);
	ImGui::Text(playerText.c_str());

	std::string localMass = "Local Mass: " + std::to_string(_localMassTotal);
	ImGui::Text(localMass.c_str());

	std::string totalMass = "Total Mass: " + std::to_string(_remoteMassTotal);
	ImGui::Text(totalMass.c_str());

	SK::INT16 minR = 1, maxR = 200;
	if (ImGui::SliderScalar("Render Freq", ImGuiDataType_U16, &_reqRenderFreq, &minR, &maxR)) {
		SystemPayload p{ 0, _reqRenderFreq };
		Game::TheGame->SendMessage(std::make_shared<SystemMessage>(Addressee::RENDER_SYSTEM, p));
	}

	SK::INT16 minCore = 0, maxCore = SK_CPU_CORE_COUNT-1;
	if (ImGui::SliderScalar("Render Core", ImGuiDataType_U16, &_reqRenderCore, &minCore, &maxCore)) {
		SystemPayload p{ 1, _reqRenderCore };
		Game::TheGame->SendMessage(std::make_shared<SystemMessage>(Addressee::RENDER_SYSTEM, p));
	}

	SK::INT16 minN = 1, maxN = 40;
	if (ImGui::SliderScalar("Network TPS", ImGuiDataType_U16, &_reqNetFreq, &minN, &maxN)) {
		SystemPayload p{ 0, _reqNetFreq };
		Game::TheGame->SendMessage(std::make_shared<SystemMessage>(Addressee::NETWORK_SYSTEM, p));
	}

	SK::INT16 minNCore = 0, maxNCore = SK_CPU_CORE_COUNT - 1;
	if (ImGui::SliderScalar("Network Core", ImGuiDataType_U16, &_reqNetCore, &minNCore, &maxNCore)) {
		SystemPayload p{ 1, _reqNetFreq };
		Game::TheGame->SendMessage(std::make_shared<SystemMessage>(Addressee::NETWORK_SYSTEM, p));
	}

	SK::INT16 minC = 1, maxC = 160;
	if (ImGui::SliderScalar("Collision TPS", ImGuiDataType_U16, &_reqColFreq, &minC, &maxC)) {
		SystemPayload p{ 0, _reqColFreq };
		Game::TheGame->SendMessage(std::make_shared<SystemMessage>(Addressee::COLLISION_SYSTEM, p));
	}

	SK::INT16 minCCore = 0, maxCCore = SK_CPU_CORE_COUNT - 1;
	if (ImGui::SliderScalar("Collision Core", ImGuiDataType_U16, &_reqColCore, &minCCore, &maxCCore)) {
		SystemPayload p{ 1, _reqColCore };
		Game::TheGame->SendMessage(std::make_shared<SystemMessage>(Addressee::COLLISION_SYSTEM, p));
	}

}

void SketchScene::VoxelCollisionHandler(ColliderComponent& sta, SK::SIZE_T t)
{
	if (_integrityMode) { return; }

	auto localPlayerSlot = _game.GetLocalPlayerSlot();
	if (localPlayerSlot == std::numeric_limits<SK::UINT8>::max()) { return;  }

	auto e = sta.Owner();
	auto& transforms = _canvas->GetTransforms();
	SK::UINT32 idx = _canvas->GetIndexFromTransform(transforms[t]);
	auto colour = _game.GetColour(localPlayerSlot);
	auto netMsg = std::make_shared<StealVoxelReqNetMessage>(idx, ANYONE, 0, NetworkSystem::GetMessageID());
	Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, netMsg));
	UpdateEntityPayload p{ e };
	Game::TheGame->SendMessage(std::make_shared<UpdateEntityMessage>(Addressee::RENDER_SYSTEM, p));
}

void SketchScene::Reset()
{
	_eye = XMFLOAT4(0, 46.0f, 1.5f, 1.0f);
	_lookAt = XMFLOAT4(0, 0, 0, 1);
	_up = XMFLOAT4(0, 1, 0, 0);
	auto viewM = XMMatrixLookAtLH(XMLoadFloat4(&_eye), XMLoadFloat4(&_lookAt), XMLoadFloat4(&_up));
	auto projM = XMMatrixPerspectiveFovLH(_fov, 1.7777f, NEARPLANE, FARPLANE);
	XMFLOAT4X4 viewMat, projMat;
	XMStoreFloat4x4(&viewMat, viewM);
	XMStoreFloat4x4(&projMat, projM);
	XMStoreFloat4x4(&_invVP, DirectX::XMMatrixInverse(nullptr, viewM * projM));
	_renderer->SetProjection(projMat);
	_renderer->SetView(viewMat);

	auto colour = _game.GetColour(_game.GetLocalPlayerSlot());
	_canvas->ResetMass();
	_localMassTotal = _canvas->GetTransforms().size();
	_remoteMassTotal = 0;
	ChangeRenderComponentColourAndEnabledPayload p{ colour.x, colour.y, colour.z, true };
	_game.SendMessage(std::make_shared<ChangeRenderComponentColourAndEnabledMessage>(Addressee::ENTITY, p));
	_game.CommandSystemsToPull();
}

void SketchScene::ToggleIntegrity() {
	if (_integrityMode) {
		std::cout << "Toki o tomeru koto o youkyuu! Za Warudo!\n";
		auto playerCount = _game.GetPlayerCount();
		auto payload = std::make_shared<IntegrityReqNetMessage>(static_cast<SK::UINT8>(playerCount- 1), NetworkSystem::GetMessageID());
		_game.SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, payload));
		auto p = SwitchSketchModePayload{ 1 };
		OnMessage(std::make_shared<SwitchSketchModeMessage>(Addressee::SCENE, p));
	}
	else {
		//Send net cancel message too.
		auto p = SwitchSketchModePayload{ 0 };
		OnMessage(std::make_shared<SwitchSketchModeMessage>(Addressee::SCENE, p));
	}
}

void SketchScene::SwitchToIntegrityMode() {
	std::thread([this]() {
		const auto& entities = _game.GetEntities();
		ChangeRenderComponentColourAndEnabledPayload ePayload{ 0.2f,0.2f,0.2f, true };
		auto msg = std::make_shared<ChangeRenderComponentColourAndEnabledMessage>(Addressee::ENTITY, ePayload);
		{
			std::unique_lock<std::mutex> lock(_game.EntityMutex);
			for (auto& e : entities) {
				_canvas->SaveColour(e);
				e->OnMessage(msg);
			}
		}
		_canvas->AddLocalMassToCache();
		_game.CommandSystemsToPull();
	}).detach();
}

void SketchScene::SwitchToNormalMode() {
	std::thread([this]() {
		const auto& entities = _game.GetEntities();
		SK::UINT32 voxel = 0;
		{
			std::unique_lock<std::mutex> lock(_game.EntityMutex);
			for (auto& e : entities) {
				_canvas->ReinstateVoxelColour(e, voxel);
			}
		}
		_game.CommandSystemsToPull();
		_canvas->ClearColours();
		_canvas->ClearIntegrityCache();
	}).detach();
}

void SketchScene::DoCameraZoom(const SK::DOUBLE& dt) {
	auto eyeV = XMLoadFloat4(&_eye);
	auto lookAtV = XMLoadFloat4(&_lookAt);
	auto dirV = lookAtV - eyeV;
	auto normDirV = XMVector3Normalize(dirV);
	eyeV = eyeV + normDirV * dt * 8.0;
	XMStoreFloat4(&_eye, eyeV);
	auto viewM = XMMatrixLookAtLH(eyeV, lookAtV, XMLoadFloat4(&_up));
	auto projM = XMLoadFloat4x4(&_proj);
	XMStoreFloat4x4(&_invVP, DirectX::XMMatrixInverse(nullptr, viewM * projM));
	XMFLOAT4X4 viewMat;
	XMStoreFloat4x4(&viewMat, viewM);
	_renderer->SetView(viewMat);
}

void SketchScene::DoRotateCamera(const SK::DOUBLE& dT) {
	auto eyeV = XMLoadFloat4(&_eye);
	auto upV = XMLoadFloat4(&_up);
	auto rotatedEye = XMVector4Transform(eyeV, XMMatrixRotationZ(CAM_ROTATE_SPEED * dT));
	auto rotatedUp = XMVector4Transform(upV, XMMatrixRotationZ(CAM_ROTATE_SPEED * dT));
	XMStoreFloat4(&_eye, rotatedEye);
	XMStoreFloat4(&_up, rotatedUp);
	auto viewM = XMMatrixLookAtLH(rotatedEye, XMLoadFloat4(&_lookAt), rotatedUp);
	auto projM = XMLoadFloat4x4(&_proj);
	XMStoreFloat4x4(&_invVP, DirectX::XMMatrixInverse(nullptr, viewM * projM));
	XMFLOAT4X4 viewMat;
	XMStoreFloat4x4(&viewMat, viewM);
	_renderer->SetView(viewMat);
}
