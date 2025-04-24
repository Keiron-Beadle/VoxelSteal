#pragma once
#include <Engine/Headers/gil.h>
#include <Engine/Headers/Message.h>
#include<Engine/Headers/NetMessages.h>
#include "SketchNetMessages.h"

enum SKAddressee {
};

//---------- Struct defines ------------//

struct DrawBlockPayload {
	SK::INT32 x, y;
	SK::UINT8 player;
};

struct PlayerJoinReqPayload {
	SK::UINT32 ip;
	SK::UINT32 respondId;
	SK::UINT16 port;
};

struct PlayerJoinResPayload : public ResponsePayload{
	PlayerJoinResPayload(SK::UINT32 id, Connection sender, SK::UINT8 freeSlot, SK::UINT8 senderSlot) 
		: ResponsePayload(id, static_cast<SK::UINT8>(PLAYER_JOIN_RES)), 
		Sender(sender), FreeSlot(freeSlot), SenderSlot(senderSlot)
	{}

	virtual ~PlayerJoinResPayload() {}

	Connection Sender;
	SK::UINT8 FreeSlot;
	SK::UINT8 SenderSlot;
};

struct StealVoxelResPayload : public ResponsePayload {
	StealVoxelResPayload(SK::UINT32 id, SK::UINT32 voxelID, SK::BOOL haveVoxel, Connection c)
		: ResponsePayload(id, static_cast<SK::UINT8>(STEAL_VOXEL_RES)), Connection(c), VoxelID(voxelID), HaveVoxel(haveVoxel)
	{}

	virtual ~StealVoxelResPayload() {}

	Connection Connection;
	SK::UINT32 VoxelID;
	SK::BOOL HaveVoxel;
};

struct IntegrityResPayload : public ResponsePayload {
	IntegrityResPayload(SK::UINT32 id)
		: ResponsePayload(id, static_cast<SK::UINT8>(INTEGRITY_RES)){}
	virtual ~IntegrityResPayload() {};
};

struct ChangeColourPayload {
	SK::FLOAT R, G, B;
};

struct StartIntegrityPayload {

};

struct VisualiseIntegrityPayload {

};

struct PlayerJoinPayload {
	Connection c;
	SK::UINT8 playerSpot;
};

struct IntegrityDataPayload {
	SK::UINT32 StartVoxelIndex;
	SK::UINT32 NumOfVoxels;
	std::vector<SK::UINT8> VoxelData;
};

struct StolenVoxelPayload {
	Connection c;
	SK::UINT32 VoxelIdx;
};

struct StealVoxelReqPayload {
	Connection c;
	SK::UINT32 VoxelIdx;
	SK::UINT32 responseID;
};

struct PlayerLeftPayload {
	Connection c;
};

struct SwitchSketchModePayload {
	SK::BYTE mode;
};

//---------------------------------------//

//----------- Message Defs ------------------//

using VisualiseIntegrityMessage = MessageImpl< VisualiseIntegrityPayload>;
using StartIntegrityMessage = MessageImpl<StartIntegrityPayload>;
using SwitchSketchModeMessage = MessageImpl<SwitchSketchModePayload>;
using IntegrityDataMessage = MessageImpl<IntegrityDataPayload>;
using StolenVoxelMessage = MessageImpl<StolenVoxelPayload>;
using StealVoxelReqMessage = MessageImpl<StealVoxelReqPayload>;
using DrawBlockMessage = MessageImpl<DrawBlockPayload>;
using ChangeColourMessage = MessageImpl<ChangeColourPayload>;
using PlayerJoinReqMessage = MessageImpl<PlayerJoinReqPayload>;
using PlayerJoinMessage = MessageImpl<PlayerJoinPayload>;
using PlayerLeftMessage = MessageImpl<PlayerLeftPayload>;
 
//-------------------------------------------//
