#pragma once
#include <Engine/Headers/gil.h>

class SketchScene;

class VoxelCanvas
{
private:
	std::vector<ColourByte> _integrityColourStore;
	std::vector<std::shared_ptr<TransformComponent>> _transforms;
	std::vector<SK::UINT8> _masses;
	std::vector<SK::UINT8> _integrityMassCache;
	std::vector<SK::BOOL> _visibilityCache;
	const SK::DOUBLE _cX, _cY, _scale;
	SK::FLOAT _halfWidth, _halfHeight, _invScale;
	const SK::UINT32 _width, _height;
	SketchScene& _game;
public:
	VoxelCanvas(SketchScene& game, const SK::DOUBLE x, const SK::DOUBLE y, const SK::UINT32 blocksWide, const SK::UINT32 blocksHigh, const SK::DOUBLE blockScale);
	~VoxelCanvas() = default;

	[[nodiscard]] std::vector<std::shared_ptr<TransformComponent>>& GetTransforms() { return _transforms; }

	[[nodiscard]] std::shared_ptr<TransformComponent>& GetTransformAtCoordinate(SK::INT32 x, SK::INT32 y);
	void GetCoordinateFromTransform(std::shared_ptr<TransformComponent> tc, SK::INT32& x, SK::INT32& y);
	[[nodiscard]] std::shared_ptr<TransformComponent>& GetTransformAtIndex(SK::SIZE_T x);
	[[nodiscard]] SK::SIZE_T GetIndexFromTransform(std::shared_ptr<TransformComponent> tc);
	[[nodiscard]] std::vector<ColourByte>& GetSavedColours() { return _integrityColourStore; }
	[[nodiscard]] std::vector<SK::UINT8>& GetIntegrityCache() { return _integrityMassCache; }
	void ReinstateVoxelColour(std::shared_ptr<Entity> e, SK::UINT32& idx);
	void SaveColour(std::shared_ptr<Entity> e);
	void ClearColours(); //Free memory
	void AddMassToIndex(SK::INT32 idx);
	void RemoveMassFromIndex(SK::INT32 idx);
	void ClearIntegrityCache();
	void AddLocalMassToCache();
	void ResetMass();
	void AddSliceToCache(SK::UINT32& idx, const std::vector<SK::UINT8>& data, SK::UINT32& dataSize);
	void GetSliceOfMassBuffer(SK::UINT32& idx, std::vector<SK::UINT8>& data, SK::UINT32& dataSize);
	nodconst SK::BOOL HasVoxel(SK::INT32 idx);
};

