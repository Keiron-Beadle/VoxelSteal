#pragma once
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <thread>
#include <future>
#include <variant>

#include "Types.h"
#include "KeyCode.h"


//------------CONSTANT DEFINES------------//

const static SK::UINT32 SK_CPU_CORE_COUNT = std::thread::hardware_concurrency();

#define ANYONE 1
#define EVERYONE 2

//----------------------------------------//

//------------FORWARD DECLARES------------//
class Entity;
class Component;
class TransformComponent;
class RenderComponent;
class ColliderComponent;
class Collider;

class Shader;
class Message;
template<class Payload>
class MessageImpl;

//----------------------------------------//

//----------- Macro Defines --------------//

#define nodconst [[nodiscard]] const


//----------------------------------------//

//------------  Enum defines -------------//
enum ComponentType {
	Transform = 1,
	Render = 2,
	Network = 4,
	Collision = 8,
	Camera = 16,
	Light = 32,
	Canvas = 64
};

enum class ColliderType {
	KINEMATIC,
	STATIC
};

//1st Byte addresses are reserved for potential future use of Engine
//Anything after is app-defined
enum Addressee {
	RENDER_SYSTEM = 1,
	COLLISION_SYSTEM = 2,
	NETWORK_SYSTEM = 4,
	ENTITY = 8,
	ALL = 16,
	SCENE = 32,
	_ENGINE_RESERVED_4 = 64,
	_ENGINE_RESERVED_5 = 128,

	_APP_DEFINED_1 = 256,
	_APP_DEFINED_2 = 512,
	_APP_DEFINED_3 = 1024,
	_APP_DEFINED_4 = 2048 //etc
};

Addressee& operator |= (Addressee& lhs, Addressee rhs);

//----------------------------------------//

//---------- Struct defines ------------//

struct ColourByte {
	SK::UINT8 R, G, B;
};

//0 = update freq, 1 = update core
struct SystemPayload {
	SK::UINT8 instruction;
	SK::UINT16 value;
};

struct PullEntityPayload {
	SK::BYTE dummy;
};

using Connection = std::pair<SK::UINT32, SK::UINT16>;

struct ConnectionDiedPayload {
	Connection connection;
};

struct UpdateEntityPayload {
	std::shared_ptr<Entity> entity;
};

struct ChangeRenderComponentColourPayload {
	SK::FLOAT R, G, B;
};

struct ChangeRenderComponentColourAndEnabledPayload {
	SK::FLOAT R, G, B;
	SK::BOOL Enabled;
};

//Normal isn't needed because lighting isn't needed.
struct Vertex {
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Colour;
};

//---------------------------------------//

//----------- Type Defs ------------------//

using ComponentMap = std::unordered_map<ComponentType, std::shared_ptr<Component>>;
using ComponentMapIterator = std::unordered_map<ComponentType, std::shared_ptr<Component>>::iterator;
using ListenerMap = std::unordered_map<Addressee, std::vector<std::shared_ptr<Component>>>;
using ListenerMapIterator = std::unordered_map<Addressee, std::vector<std::shared_ptr<Component>>>::iterator;

using EntityList = std::vector<std::shared_ptr<Entity>>;
using RenderList = std::vector<RenderComponent>;
using CollisionList = std::vector<ColliderComponent>;
using StaticColliderList = std::vector<std::shared_ptr<Collider>>;
using TransformList = std::vector<TransformComponent>;

using ModelResourceMap = std::unordered_map<std::string, std::vector<Vertex>>;
using ShaderResourceMap = std::unordered_map<std::string, Shader>;


//---------------------------------------//

//----------- Message Defs ------------------//

using SystemMessage = MessageImpl<SystemPayload>;
using PullEntityMessage = MessageImpl<PullEntityPayload>;
using ConnectionDiedMessage = MessageImpl<ConnectionDiedPayload>;
using UpdateEntityMessage = MessageImpl<UpdateEntityPayload>;
using ChangeRenderComponentMessage = MessageImpl<ChangeRenderComponentColourPayload>;
using ChangeRenderComponentColourAndEnabledMessage = MessageImpl< ChangeRenderComponentColourAndEnabledPayload>;

//---------------------------------------//
