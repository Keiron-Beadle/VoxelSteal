#pragma once
#include <Engine/Headers/gil.h>
#include <Engine/Headers/NetMessages.h>
#include <Engine/Headers/Message.h>

enum NET_MESSAGE_TYPES {
	_RESERVED_UDP,
	_RESERVED_TCP,
	_RESERVED_REQ_TCP,
	_RESERVED_SHUTDOWN,
	SEND_STRING,
	DRAW_BLOCK,
	PLAYER_JOIN_REQ,
	PLAYER_JOIN_RES,
	PLAYER_JOIN,
	STEAL_VOXEL_REQ,
	STEAL_VOXEL_RES,
	INTEGRITY_REQ,
	INTEGRITY_RES,
	INTEGRITY_DATA,
	INTEGRITY_START
};

struct IntegrityStartNetMessage : public TCPMessage {
	IntegrityStartNetMessage(SK::UINT32 id)
		: TCPMessage(EVERYONE, 0, id) {
		messageType = INTEGRITY_START;
	}
	virtual ~IntegrityStartNetMessage() {}
	virtual std::vector<SK::BYTE> Serialize() override {
		return TCPMessage::Serialize();
	}

	static std::shared_ptr<IntegrityStartNetMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx) {
		auto ism = std::make_shared<IntegrityStartNetMessage>(0);
		TCPMessage::Deserialize(data, idx, ism);
		return ism;
	}
};

struct IntegrityDataNetMessage : public TCPMessage {
	SK::UINT32 StartVoxelIdx;
	SK::UINT32 DataSize;
	std::vector<SK::UINT8> Data;
	IntegrityDataNetMessage(SK::UINT32 startVoxelId, SK::UINT32 ip, SK::UINT16 port, SK::UINT32 id, SK::UINT32 dataSize, std::vector<SK::UINT8> data)
		: TCPMessage(ip,port,id), StartVoxelIdx(startVoxelId), DataSize(dataSize)
	{
		Data.insert(Data.begin(), data.data(), data.data() + dataSize);
		messageSize += sizeof(StartVoxelIdx);
		messageSize += sizeof(DataSize);
		messageSize += DataSize;
		messageType = INTEGRITY_DATA;
	}

	virtual ~IntegrityDataNetMessage() {};

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPMessage::Serialize();
		SerializeU32(data, StartVoxelIdx);
		SerializeU32(data, DataSize);
		data.insert(data.end(), Data.begin(), Data.begin() + DataSize);
		return data;
	}

	static std::shared_ptr<IntegrityDataNetMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx) {
		auto idm = std::make_shared<IntegrityDataNetMessage>(0, 0, 0, 0, 0, std::vector<SK::UINT8>());
		TCPMessage::Deserialize(data, idx, idm);
		idm->StartVoxelIdx = DeserializeU32(data, idx);
		idm->DataSize = DeserializeU32(data, idx);
		idm->Data.insert(idm->Data.begin(), data.begin() + idx, data.begin() + idx + idm->DataSize);
		return idm;
	}
};

struct IntegrityResNetMessage : public TCPMessage {
	SK::UINT32 ResponseID;
	IntegrityResNetMessage(SK::UINT32 responseID, SK::UINT32 ip, SK::UINT16 port, SK::UINT32 id) 
		: TCPMessage(ip, port, id), ResponseID(responseID) {
		messageType = static_cast<SK::UINT8>(INTEGRITY_RES);
		messageSize += 4;
	}

	virtual ~IntegrityResNetMessage() { };

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPMessage::Serialize(); 
		SerializeU32(data, ResponseID);
		return data;
	}

	static std::shared_ptr<IntegrityResNetMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx) {
		auto irm = std::make_shared<IntegrityResNetMessage>(0,0, 0, 0);
		TCPMessage::Deserialize(data, idx, irm);
		irm->ResponseID = DeserializeU32(data, idx);
		return irm;
	}
};

struct IntegrityReqNetMessage : public TCPReqMessage {
	IntegrityReqNetMessage(SK::UINT8 responseCount, SK::ULONG id) : TCPReqMessage(responseCount,EVERYONE, 0, id) 
	{
		messageType = static_cast<SK::UINT8>(INTEGRITY_REQ);
	}

	virtual ~IntegrityReqNetMessage() {};

	virtual std::vector<SK::BYTE> Serialize() override {
		return TCPReqMessage::Serialize();
	}

	static std::shared_ptr<IntegrityReqNetMessage> Deserialize(std::vector<SK::BYTE>& data, const Connection& c, SK::INT32& idx) {
		auto irm = std::make_shared<IntegrityReqNetMessage>(0, 0);
		TCPReqMessage::Deserialize(data, c, idx, irm);
		return irm;
	}

};

struct StealVoxelResNetMessage : public TCPMessage {
	SK::UINT32 ResponseID;
	SK::UINT32 VoxelID;
	SK::BYTE HasStolenVoxel;
	StealVoxelResNetMessage(SK::UINT32 responseID, SK::BOOL hasStolenVoxel, SK::UINT32 voxelId, SK::UINT32 ip, SK::UINT16 port, SK::ULONG id)
		: TCPMessage(ip,port,id), ResponseID(responseID), HasStolenVoxel(hasStolenVoxel), VoxelID(voxelId) {
		messageSize += 9;
		messageType = static_cast<SK::UINT8>(STEAL_VOXEL_RES);
	}

	virtual ~StealVoxelResNetMessage() {}

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPMessage::Serialize();
		SerializeU32(data, ResponseID);
		data.push_back(HasStolenVoxel);
		SerializeU32(data, VoxelID);
		return data;
	}

	static std::shared_ptr<StealVoxelResNetMessage> Deserialize(std::vector<SK::BYTE>& data, const Connection& c, SK::INT32& idx) {
		auto svr = std::make_shared<StealVoxelResNetMessage>(0, 0, 0,0, 0, 0);
		TCPMessage::Deserialize(data, idx, svr);
		svr->ResponseID = DeserializeU32(data, idx);
		svr->HasStolenVoxel = DeserializeU8(data, idx);
		svr->VoxelID = DeserializeU32(data, idx);
		return svr;
	}
};

struct StealVoxelReqNetMessage : public TCPReqMessage {
	SK::UINT32 VoxelIdx;

	StealVoxelReqNetMessage(SK::UINT32 voxel, SK::UINT32 ip, SK::UINT16 port, SK::ULONG id) 
		: TCPReqMessage(1, ip, port, id), VoxelIdx(voxel) {
		messageSize += 4;
		messageType = static_cast<SK::UINT8>(STEAL_VOXEL_REQ);
	}

	virtual ~StealVoxelReqNetMessage() {};

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPReqMessage::Serialize();
		SerializeU32(data, VoxelIdx);
		return data;
	}

	static std::shared_ptr<StealVoxelReqNetMessage> Deserialize(std::vector<SK::BYTE>& data, const Connection& c, SK::INT32& idx) {
		auto svr = std::make_shared<StealVoxelReqNetMessage>(0,0,0,0);
		TCPReqMessage::Deserialize(data, c, idx, svr);
		svr->VoxelIdx = DeserializeU32(data,idx);
		return svr;
	}

};

struct PlayerJoinNetMessage : public TCPMessage {
	SK::UINT32 SenderIP;
	SK::UINT16 SenderPort;
	SK::UINT8 Slot;
	PlayerJoinNetMessage(SK::UINT8 playerIdx, SK::UINT32 ip, SK::UINT16 port, SK::ULONG id)
		: TCPMessage(ip, port, id), Slot(playerIdx), SenderIP(0), SenderPort(0) {
		messageType = static_cast<SK::UINT8>(PLAYER_JOIN);
		messageSize += 1;
	}
	virtual ~PlayerJoinNetMessage() {};
	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPMessage::Serialize();
		data.push_back(Slot);
		return data;
	}
	static std::shared_ptr<PlayerJoinNetMessage> Deserialize(std::vector<SK::BYTE>& data, const Connection& c, SK::INT32& idx) {
		auto pjm = std::make_shared<PlayerJoinNetMessage>(0, 0, 0, 0);
		TCPMessage::Deserialize(data, idx, pjm);
		pjm->Slot = DeserializeU8(data, idx);
		pjm->SenderIP = c.first;
		pjm->SenderPort = c.second;
		return pjm;
	}
};

struct PlayerJoinResNetMessage : public TCPMessage {
	SK::ULONG respondToID;
	SK::UINT32 SenderIP;
	SK::UINT16 SenderPort;
	SK::UINT8 FreeSlot;
	SK::UINT8 LocalSlot;
	PlayerJoinResNetMessage(SK::UINT8 freeSlot, SK::UINT8 localSlot, SK::ULONG respondID, SK::UINT32 ip, SK::UINT16 port, SK::ULONG id)
		: TCPMessage(ip,port,id), respondToID(respondID), FreeSlot(freeSlot), LocalSlot(localSlot), SenderIP(0), SenderPort(0)
	{
		messageType = static_cast<SK::UINT8>(PLAYER_JOIN_RES);
		messageSize += 6;
	}
	virtual ~PlayerJoinResNetMessage() {};
	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPMessage::Serialize();
		SerializeU32(data, respondToID);
		data.push_back(FreeSlot);
		data.push_back(LocalSlot);
		return data;
	}
	static std::shared_ptr<PlayerJoinResNetMessage> Deserialize(std::vector<SK::BYTE>& data, const Connection& c, SK::INT32& idx) {
		auto pjr = std::make_shared<PlayerJoinResNetMessage>(0, 0, 0,0,0,0);
		TCPMessage::Deserialize(data, idx, pjr);
		pjr->messageType = static_cast<SK::UINT8>(PLAYER_JOIN_RES);
		pjr->respondToID = DeserializeU32(data, idx);
		pjr->FreeSlot = DeserializeU8(data, idx);
		pjr->LocalSlot = DeserializeU8(data, idx);
		pjr->SenderIP = c.first;
		pjr->SenderPort = c.second;
		return pjr;
	}
};

struct PlayerJoinReqNetMessage : public TCPReqMessage {
	PlayerJoinReqNetMessage(SK::UINT8 responseCount, SK::UINT32 ip, SK::UINT16 port, SK::ULONG id)
		: TCPReqMessage(responseCount,ip, port, id, 0, 0)
	{
		isRequest = true;
		messageType = static_cast<SK::UINT8>(PLAYER_JOIN_REQ);
	}
	virtual ~PlayerJoinReqNetMessage() {};
	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPReqMessage::Serialize();
		return data;
	}
	static std::shared_ptr<PlayerJoinReqNetMessage> Deserialize(std::vector<SK::BYTE>& data, const Connection& c, SK::INT32& idx) {
		auto prm = std::make_shared<PlayerJoinReqNetMessage>(0,0, 0, 0);
		TCPReqMessage::Deserialize(data, c, idx, prm);
		prm->messageType = static_cast<SK::UINT8>(PLAYER_JOIN_REQ);
		prm->isRequest = true;
		return prm;
	}
};

struct SendStringNetMessage : public TCPMessage {
	std::string Text;

	SendStringNetMessage(std::string text, SK::UINT32 ip, SK::UINT16 port, SK::ULONG id)
		: TCPMessage(ip, port, id), Text(text) {
		isRequest = true;
		messageSize += text.size() + 4; //+4 for len of text + 1 for null terminator
		messageType = static_cast<SK::UINT8>(SEND_STRING);
	}

	virtual ~SendStringNetMessage() {};

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPMessage::Serialize();
		SerializeString(data, Text);
		return data;
	}

	static std::shared_ptr<SendStringNetMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx) {
		auto ssm = std::make_shared<SendStringNetMessage>("", 0, 0, 0);
		TCPMessage::Deserialize(data, idx, ssm);
		DeserializeString(data, idx,ssm->Text);
		ssm->messageType = static_cast<SK::UINT8>(SEND_STRING);
		return ssm;
	}
};

struct DrawBlockNetMessage : public TCPMessage {
	SK::INT32 X, Y;
	SK::UINT8 Player;

	DrawBlockNetMessage(SK::UINT8 player, SK::INT32 x, SK::INT32 y, SK::UINT32 ip, SK::UINT16 port, SK::ULONG id) 
		: TCPMessage(ip, port, id), X(x), Y(y), Player(player) {
		isRequest = false;
		messageSize += 9;
		messageType = static_cast<SK::UINT8>(DRAW_BLOCK);
	}

	virtual ~DrawBlockNetMessage() {};

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPMessage::Serialize();
		data.push_back(Player);
		SerializeU32(data, X);
		SerializeU32(data, Y);
		return data;
	}

	static std::shared_ptr<DrawBlockNetMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx) {
		auto dbm = std::make_shared<DrawBlockNetMessage>(0,0, 0, 0, 0, 0);
		TCPMessage::Deserialize(data, idx);
		dbm->Player = DeserializeU8(data, idx);
		dbm->X = DeserializeU32(data, idx);
		dbm->Y = DeserializeU32(data, idx);
		return dbm;
	}
};