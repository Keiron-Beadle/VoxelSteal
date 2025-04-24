#include <Engine/Components/Headers/TransformComponent.h>
#include <Engine/Components/Headers/ColliderComponent.h>
#include <Engine/Components/Headers/RenderComponent.h>
#include <Engine/Headers/Message.h>
#include <Engine/Headers/Game.h>
#include <Engine/Headers/NetMessages.h>
#include "../Headers/VoxelCanvas.h"
#include "../Headers/SketchScene.h"
#include "../Headers/SketchMessages.h"
#include <iostream>
#undef SendMessage

VoxelCanvas::VoxelCanvas(SketchScene& game, const SK::DOUBLE x, const SK::DOUBLE y, const SK::UINT32 blocksWide, const SK::UINT32 blocksHigh, const SK::DOUBLE blockScale)
	: _game(game), _cX(x), _cY(y), _width(blocksWide), _height(blocksHigh), _scale(blockScale)
{
	_halfWidth = _width * _scale / 2.0f;
	_halfHeight = _height * _scale / 2.0f;
	_invScale = 1.0f / _scale;

	SK::FLOAT left = x - (_width / 2.0f) * _scale;
	SK::FLOAT top = y - (_height / 2.0f) * _scale;

	_transforms.reserve(static_cast<SK::SIZE_T>(_width * _height));
	_masses.reserve(static_cast<SK::SIZE_T>(_width * _height));
	_integrityColourStore.reserve(static_cast<SK::SIZE_T>(_width * _height));
	_integrityMassCache.insert(_integrityMassCache.begin(), _width * _height, 0);
	_visibilityCache.reserve(static_cast<SK::SIZE_T>(_width * _height));
	for (SK::UINT32 y = 0; y < _height; ++y) {
		for (SK::UINT32 x = 0; x < _width; ++x) {
			SK::FLOAT blockLeft = left + x * _scale;
			SK::FLOAT blockTop = top + y * _scale;
			_transforms.push_back(std::make_shared<TransformComponent>(
			DirectX::XMFLOAT3(blockLeft, 0.0f, blockTop),
			DirectX::XMFLOAT3(_scale,_scale,_scale)
			));
			_masses.push_back(1);
		}
	}
}

std::shared_ptr<TransformComponent>& VoxelCanvas::GetTransformAtCoordinate(SK::INT32 x, SK::INT32 y)
{
	return _transforms[static_cast<SK::SIZE_T>(y * _width + x)];
}

void VoxelCanvas::GetCoordinateFromTransform(std::shared_ptr<TransformComponent> tc, SK::INT32& x, SK::INT32& y)
{
	SK::FLOAT dx = tc->Position().x - _cX + _halfWidth;
	SK::FLOAT dy = tc->Position().z - _cY + _halfWidth;
	x = static_cast<SK::INT32>(std::round(dx * _invScale));
	y = static_cast<SK::INT32>(std::round(dy * _invScale));
}

std::shared_ptr<TransformComponent>& VoxelCanvas::GetTransformAtIndex(SK::SIZE_T x) {
	return _transforms[x];
}

void VoxelCanvas::AddMassToIndex(SK::INT32 idx) {
	_masses[idx] += 1;
	_game.IncMass();
}

void VoxelCanvas::RemoveMassFromIndex(SK::INT32 idx) {
	_masses[idx] -= 1;
	_game.DecMass();
	if (_masses[idx] <= 0) {
		//Send message to disable this voxel.
		auto entity = _transforms[idx]->Owner();
		UpdateEntityPayload p{ entity };
		//_transforms[idx]->SetPosition(DirectX::XMFLOAT3(200, 200, 200));
		auto rc = std::dynamic_pointer_cast<RenderComponent>(entity->GetComponent(ComponentType::Render));
		rc->SetEnabled(false);
		auto colliderComp = std::dynamic_pointer_cast<ColliderComponent>(entity->GetComponent(ComponentType::Collision));
		colliderComp->SetEnabled(false);
		Addressee addr = Addressee::RENDER_SYSTEM;
		addr |= Addressee::SCENE;
		Game::TheGame->SendMessage(std::make_shared<UpdateEntityMessage>(addr, p));
	}
}

nodconst SK::BOOL VoxelCanvas::HasVoxel(SK::INT32 idx) {
	return _masses[idx] > 0;
}

SK::SIZE_T VoxelCanvas::GetIndexFromTransform(std::shared_ptr<TransformComponent> tc) {
	SK::INT32 x, y;
	GetCoordinateFromTransform(tc,x,y);
	return static_cast<SK::SIZE_T>(y * _width + x);
}

void VoxelCanvas::SaveColour(std::shared_ptr<Entity> e) {
	auto rc = std::dynamic_pointer_cast<RenderComponent>(e->GetComponent(ComponentType::Render));
	if (!rc) return;
	auto& floatColour = rc->Colour();
	SK::UINT8 R = static_cast<SK::UINT8>(std::round(floatColour.x * 255));
	SK::UINT8 G = static_cast<SK::UINT8>(std::round(floatColour.y * 255));
	SK::UINT8 B = static_cast<SK::UINT8>(std::round(floatColour.z * 255));
	ColourByte colour { R,G,B };
	_integrityColourStore.push_back(colour);
	_visibilityCache.push_back(rc->Enabled());
}

void VoxelCanvas::ClearColours() {
	_integrityColourStore.clear();
	_visibilityCache.clear();
}

void VoxelCanvas::ReinstateVoxelColour(std::shared_ptr<Entity> e, SK::UINT32& idx) {
	auto rc = std::dynamic_pointer_cast<RenderComponent>(e->GetComponent(ComponentType::Render));
	if (!rc) return;
	auto& storedColour = _integrityColourStore[idx];
	auto visible = _visibilityCache[idx];
	rc->SetColour(DirectX::XMFLOAT4(storedColour.R / 255.0f, storedColour.G / 255.0f, storedColour.B / 255.0f, 1.0f));
	rc->SetEnabled(visible);
	++idx;
}

void VoxelCanvas::GetSliceOfMassBuffer(SK::UINT32& idx, std::vector<SK::UINT8>& data, SK::UINT32& dataSize) {
	auto batch = min(REC_TCP_BUFFER_SIZE - 29, _masses.size() - idx);
	data.insert(data.begin(), _masses.begin() + idx, _masses.begin() + idx + batch);
	idx = batch;
	dataSize = data.size();
}

void VoxelCanvas::ClearIntegrityCache() {
	_integrityMassCache.assign(_integrityMassCache.size(), 0);
}

void VoxelCanvas::AddLocalMassToCache() {
	for (int i = 0; i < _integrityMassCache.size(); ++i) {
		_integrityMassCache[i] += _masses[i];
	}
}

void VoxelCanvas::AddSliceToCache(SK::UINT32& idx, const std::vector<SK::UINT8>& data, SK::UINT32& dataSize) {
	SK::SIZE_T dataPointer = 0;
	SK::SIZE_T upperBound = min(idx + dataSize, _integrityMassCache.size());
	for (int i = idx; i < upperBound; ++i) {
		_integrityMassCache[i] += data[dataPointer++];
	}
	if (idx + dataPointer >= _integrityMassCache.size()) {
		//We're done, update to reflect the changes.
		VisualiseIntegrityPayload p;
		Game::TheGame->SendMessage(std::make_shared<VisualiseIntegrityMessage>(Addressee::SCENE, p));
	}
}

void VoxelCanvas::ResetMass() {
	_masses.assign(_masses.size(), 1);
}