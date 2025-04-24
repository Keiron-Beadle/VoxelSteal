#pragma once
#include "gil.h"
#include "Message.h"
#include "NetMessages.h"
#include <queue>
#include <variant>
#include <type_traits>

class NetworkHandler {
public:
	NetworkHandler() = default;
	virtual ~NetworkHandler() = default;
public:

	virtual void ProcessNetMessage(std::shared_ptr<NetMessage> msg) = 0;
	virtual void HandleFulfilledResponse(std::vector<std::shared_ptr<ResponseMessage>>& messages) = 0;
	virtual void HandleTimedOutRequest(std::shared_ptr<NetMessage> message) = 0;
	virtual std::shared_ptr<NetMessage> CreateMessage(std::vector<SK::BYTE>& data, const std::pair<SK::UINT32, SK::UINT16>& senderAddr) = 0;
};