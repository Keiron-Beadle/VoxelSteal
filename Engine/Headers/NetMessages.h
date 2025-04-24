#pragma once
#include "gil.h"
#include <Windows.h>
#include "SerializeHelpers.h"

static constexpr SK::UINT16 REC_TCP_BUFFER_SIZE = 512;
const auto CURRENT_PROCESS_ID = GetCurrentProcessId();

struct NetMessage {
	SK::BOOL isTCP;
	SK::BOOL isRequest;
	SK::UINT8 messageType;
	SK::INT32 pid;
	SK::ULONG messageId;
	SK::ULONG messageSize;

	NetMessage(SK::BOOL tcp, SK::BOOL req, SK::UINT8 mt, SK::ULONG mid)
	: isTCP(tcp), isRequest(req), messageType(mt), messageId(mid), messageSize(0) {
		pid = static_cast<SK::INT32>(CURRENT_PROCESS_ID);
		//Change this if you ever change the NetMessage member variables
		messageSize += 15;
	}
	virtual ~NetMessage() {}

	virtual std::vector<SK::BYTE> Serialize() {
		std::vector<SK::BYTE> data;
		SerializeU32(data, messageSize);
		SerializeU32(data, pid);
		data.push_back(messageType);
		data.push_back(static_cast<SK::BYTE>(isTCP));
		data.push_back(static_cast<SK::BYTE>(isRequest));
		SerializeU32(data, messageId);
		return data;
	}

	static std::shared_ptr<NetMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx, std::shared_ptr<NetMessage> nm) {
		nm->messageType = data[idx++];
		nm->isTCP = data[idx++];
		nm->isRequest = data[idx++];
		nm->messageId = DeserializeU32(data, idx);
		return nm;
	}
};

struct UDPMessage : public NetMessage {
	SK::UINT16 Port;
	UDPMessage(SK::UINT16 port, SK::ULONG id)
		: NetMessage(0,0, 0,id), Port(port)
	{
		messageSize += 2;
	}

	virtual ~UDPMessage() {}

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = NetMessage::Serialize();
		SerializeU16(data, Port);
		return data;
	}

	static std::shared_ptr<UDPMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx) {
		auto udp = std::make_shared<UDPMessage>(0,0);
		NetMessage::Deserialize(data, idx, udp);
		auto port = DeserializeU16(data, idx);
		udp->Port = port;
		return udp;
	}

	static std::shared_ptr<UDPMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx, std::shared_ptr<UDPMessage> udp) {
		NetMessage::Deserialize(data, idx, udp);
		auto port = DeserializeU16(data, idx);
		udp->Port = port;
		return udp;
	}
};

struct TCPMessage : public NetMessage {
	SK::UINT32 Ip;
	SK::UINT16 Port;
	TCPMessage(SK::UINT32 ip, SK::UINT16 port, SK::ULONG id)
		: NetMessage(1,0, 1,id), Ip(ip), Port(port) {
		messageSize += 6;
	}

	virtual ~TCPMessage() {};

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = NetMessage::Serialize();
		SerializeU32(data, Ip);
		SerializeU16(data, Port);
		return data;
	}

	static std::shared_ptr<TCPMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx) {
		auto tcp = std::make_shared<TCPMessage>(0, 0, 0);
		NetMessage::Deserialize(data, idx, tcp);
		auto ip = DeserializeU32(data, idx);
		auto port = DeserializeU16(data, idx);
		tcp->Ip = ip;
		tcp->Port = port;
		return tcp;
	}

	static std::shared_ptr<TCPMessage> Deserialize(std::vector<SK::BYTE>& data, SK::INT32& idx, std::shared_ptr<TCPMessage> tcp) {
		NetMessage::Deserialize(data, idx, tcp);
		auto ip = DeserializeU32(data, idx);
		auto port = DeserializeU16(data, idx);
		tcp->Ip = ip;
		tcp->Port = port;
		return tcp;
	}
};

struct TCPReqMessage : public TCPMessage {
	SK::UINT32 SenderIP;
	SK::UINT16 SenderPort;
	SK::UINT8 ExpectedResponses;
	TCPReqMessage(SK::UINT8 responseCount, SK::UINT32 destIp, SK::UINT16 destPort, SK::ULONG id, SK::UINT32 localIp = 0, SK::UINT16 localPort = 0)
		: TCPMessage(destIp, destPort, id), SenderIP(localIp), SenderPort(localPort), ExpectedResponses(responseCount) {
		isRequest = true;
		messageType = 1;
		messageSize += 1;
	}

	virtual ~TCPReqMessage() {}

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPMessage::Serialize();
		data.push_back(ExpectedResponses);
		return data;
	}

	static std::shared_ptr<TCPReqMessage> Deserialize(std::vector<SK::BYTE>& data, const Connection& sender, SK::INT32& idx) {
		auto trp = std::make_shared<TCPReqMessage>(0, 0, 0, 0, 0);
		TCPMessage::Deserialize(data, idx, trp);
		trp->ExpectedResponses = DeserializeU8(data, idx);
		trp->SenderIP = sender.first;
		trp->SenderPort = sender.second;
		return trp;
	}

	static std::shared_ptr<TCPReqMessage> Deserialize(std::vector<SK::BYTE>& data, const Connection& sender, SK::INT32& idx, std::shared_ptr<TCPReqMessage> trp) {
		TCPMessage::Deserialize(data, idx, trp);
		trp->ExpectedResponses = DeserializeU8(data, idx);
		trp->SenderIP = sender.first;
		trp->SenderPort = sender.second;
		return trp;
	}
};

struct ShutdownMessage : public TCPMessage {
	std::string Message;
	SK::UINT32 SenderIP;
	SK::UINT16 SenderPort;
	ShutdownMessage(SK::ULONG id) : TCPMessage(EVERYONE, 0, id), Message("GOODBYE"), SenderIP(0), SenderPort(0) {
		messageType = 3;
		messageSize += Message.size() + 4; //1 per char + 4 for size
	}

	virtual ~ShutdownMessage() {}

	virtual std::vector<SK::BYTE> Serialize() override {
		auto data = TCPMessage::Serialize();
		SerializeString(data, Message);
		return data;
	}

	static std::shared_ptr<ShutdownMessage> Deserialize(std::vector<SK::BYTE>& data, const std::pair<SK::UINT32, SK::UINT16>& sender, SK::INT32& idx) {
		auto sm = std::make_shared<ShutdownMessage>(0);
		TCPMessage::Deserialize(data, idx, sm);
		DeserializeString(data, idx, sm->Message);
		sm->SenderIP = sender.first;
		sm->SenderPort = sender.second;
		return sm;
	}
};

struct ResponsePayload {
	ResponsePayload(SK::UINT32 id, SK::UINT8 type) : MessageID(id), Type(type) {}
	virtual ~ResponsePayload() {}
	SK::ULONG MessageID;
	SK::UINT8 Type;
};

struct RemoveConnectionPayload {
	Connection c;
};

using RemoveConnectionMessage = MessageImpl<RemoveConnectionPayload>;
using ResponseMessage = MessageImpl<std::shared_ptr<ResponsePayload>>;
using SendNetMessage = MessageImpl<std::shared_ptr<NetMessage>>;
using ResponseMap = std::unordered_map<SK::ULONG, std::pair<std::vector<std::shared_ptr<ResponseMessage>>, SK::UINT8>>;
