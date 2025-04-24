#pragma once
#include <Engine/Headers/gil.h>
#include <Engine/Headers/NetworkHandler.h>
#include "SketchGame.h"

class SketchNetworkHandler : public NetworkHandler
{
private:
	SketchGame& _game;
public:
	SketchNetworkHandler(SketchGame& game);
	virtual ~SketchNetworkHandler() = default;

public:

	void ProcessNetMessage(std::shared_ptr<NetMessage> msg) override;
	std::shared_ptr<NetMessage> CreateMessage(std::vector<SK::BYTE>& data, const std::pair<SK::UINT32, SK::UINT16>& senderAddr) override;
	void HandleFulfilledResponse(std::vector<std::shared_ptr<ResponseMessage>>& messages) override;
	void HandleTimedOutRequest(std::shared_ptr<NetMessage> message) override;

private:
	//<-------------HANDLERS------------>
	 
	void HandlePlayerJoinRes(std::shared_ptr<NetMessage> msg);
	void HandlePlayerJoinReq(std::shared_ptr<NetMessage> msg);
	void HandlePlayerJoin(std::shared_ptr<NetMessage> msg);
	void HandlePeerShutdown(std::shared_ptr<NetMessage> msg);
	void HandleStealVoxelRes(std::shared_ptr<NetMessage> msg);
	void HandleStealVoxelReq(std::shared_ptr<NetMessage> msg);
	void HandleIntegrityReq(std::shared_ptr<NetMessage> msg);
	void HandleIntegrityRes(std::shared_ptr<NetMessage> msg);
	void HandleIntegrityStart(std::shared_ptr<NetMessage> msg);
	void HandleIntegrityData(std::shared_ptr<NetMessage> msg);

	void HandlePlayerJoinTimeout(std::shared_ptr<NetMessage> msg);
	void HandleIntegrityTimeout(std::shared_ptr<NetMessage> msg);

	void HandleVoxelStealResponses(std::vector<std::shared_ptr<ResponseMessage>>& messages);
	void HandlePlayerJoinResponses(std::vector<std::shared_ptr<ResponseMessage>>& messages);
	void HandleIntegrityResponses();

	//---------------------------------->
};
