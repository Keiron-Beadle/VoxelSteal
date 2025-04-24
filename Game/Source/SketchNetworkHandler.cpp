#include <Engine/Headers/Game.h>
#include <Engine/Systems/Headers/NetworkSystem.h>
#include "../Headers/SketchGame.h"
#include "../Headers/SketchNetworkHandler.h"
#include "../Headers/SketchNetMessages.h"
#include "../Headers/SketchMessages.h"
#include <iostream>

SketchNetworkHandler::SketchNetworkHandler(SketchGame& game)
    : _game(game)
{

}

void SketchNetworkHandler::ProcessNetMessage(std::shared_ptr<NetMessage> msg)
{
    //Incoming Net Message of any type
    //Parse it for the type and do whatever is needed, 
    //Usually sending a message to TheGame for an event to be handled.

    switch (msg->messageType) {
    case SEND_STRING:
    {
        auto casted = std::dynamic_pointer_cast<SendStringNetMessage>(msg);
        if (!casted) { std::cerr << "Error casting NetMessage to SendStringNetMessage\n"; return; }
        std::cout << casted->Text << std::endl;
        break;
    }
    case DRAW_BLOCK:
    {
        auto casted = std::dynamic_pointer_cast<DrawBlockNetMessage>(msg);
        if (!casted) { std::cerr << "Error casting NetMessage to DrawBlockNetMessage\n";  return; }
        DrawBlockPayload p{ casted->X, casted->Y, casted->Player };
        Game::TheGame->SendMessage(std::make_shared<DrawBlockMessage>(Addressee::SCENE, p));
        break;
    }
    case PLAYER_JOIN_REQ:
        HandlePlayerJoinReq(msg);
        break;
    case PLAYER_JOIN_RES:
        HandlePlayerJoinRes(msg);
        break;
    case PLAYER_JOIN:
        HandlePlayerJoin(msg);
        break;
    case _RESERVED_SHUTDOWN:
        HandlePeerShutdown(msg);
        break;
    case STEAL_VOXEL_REQ:
        HandleStealVoxelReq(msg);
        break;
    case STEAL_VOXEL_RES:
        HandleStealVoxelRes(msg);
        break;
    case INTEGRITY_REQ:
        HandleIntegrityReq(msg);
        break;
    case INTEGRITY_RES:
        HandleIntegrityRes(msg);
        break;
    case INTEGRITY_START:
        HandleIntegrityStart(msg);
    case INTEGRITY_DATA:
        HandleIntegrityData(msg);
        break;
    }

}

std::shared_ptr<NetMessage> SketchNetworkHandler::CreateMessage(std::vector<SK::BYTE>& data, const Connection& senderAddr)
{
    //Read some amount of data from a recv()
    //Need to parse that data to get the net message out.
    //Usually it's as simple as just reading BYTE[4] which gives the messageType
    //Limited to 255 different types of messages (but that's probably more than enough)

    //IMPORTANT: Data was sent in order: 4 (messageSize) 4 (pid) 1 (messageType)
    //In the Listener, we discard the first 4 bytes for message size
    //Meaning the remaining data is 4 (pid) 1 (messageType) => type at idx 4

    SK::INT32 idx = 4;
    SK::UINT8 type = data[idx];
    switch (type) {
    case SEND_STRING:
        return SendStringNetMessage::Deserialize(data, idx);
    case DRAW_BLOCK:
        return DrawBlockNetMessage::Deserialize(data, idx);
    case PLAYER_JOIN_REQ:
        return PlayerJoinReqNetMessage::Deserialize(data, senderAddr, idx);
    case PLAYER_JOIN_RES:
        return PlayerJoinResNetMessage::Deserialize(data, senderAddr, idx);
    case PLAYER_JOIN:
        return PlayerJoinNetMessage::Deserialize(data, senderAddr, idx);
    case STEAL_VOXEL_REQ:
        return StealVoxelReqNetMessage::Deserialize(data, senderAddr, idx);
    case STEAL_VOXEL_RES:
        return StealVoxelResNetMessage::Deserialize(data, senderAddr, idx);
    case INTEGRITY_REQ:
        return IntegrityReqNetMessage::Deserialize(data, senderAddr, idx);
    case INTEGRITY_RES:
        return IntegrityResNetMessage::Deserialize(data, idx);
    case INTEGRITY_START:
        return IntegrityStartNetMessage::Deserialize(data, idx);
    case INTEGRITY_DATA:
        return IntegrityDataNetMessage::Deserialize(data, idx);
    case _RESERVED_TCP:
        return TCPMessage::Deserialize(data, idx);
    case _RESERVED_UDP:
        return UDPMessage::Deserialize(data, idx);
    case _RESERVED_REQ_TCP:
        return TCPReqMessage::Deserialize(data, senderAddr, idx);
    case _RESERVED_SHUTDOWN:
        return ShutdownMessage::Deserialize(data, senderAddr, idx);
    }

    std::cerr << "Invalid message type to CreateMessage in SketchNetworkHandler." << std::endl;
    std::cerr << "Message type: " << type << std::endl;

    return nullptr;
    //throw std::exception("Invalid message type to CreateMessage in SketchNetworkHandler.");
}

void SketchNetworkHandler::HandleIntegrityStart(std::shared_ptr<NetMessage> msg) {
    StartIntegrityPayload p;
    Game::TheGame->SendMessage(std::make_shared<StartIntegrityMessage>(Addressee::SCENE, p));
}

void SketchNetworkHandler::HandleIntegrityReq(std::shared_ptr<NetMessage> msg) {
    auto casted = std::dynamic_pointer_cast<IntegrityReqNetMessage>(msg);
    if (!casted) {
        std::cerr << "SketchNetworkHandler :: Couldn't cast NetMessage into IntegrityReqNetMessage.\n";
        return;
    }
    SwitchSketchModePayload p{ 1 };
    Game::TheGame->SendMessage(std::make_shared<SwitchSketchModeMessage>(Addressee::SCENE, p));

    auto payload = std::make_shared<IntegrityResNetMessage>(casted->messageId,casted->SenderIP, casted->SenderPort, NetworkSystem::GetMessageID());
    Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, payload));
}

void SketchNetworkHandler::HandleIntegrityRes(std::shared_ptr<NetMessage> msg) {
    auto casted = std::dynamic_pointer_cast<IntegrityResNetMessage>(msg);
    if (!casted) {
        std::cerr << "SketchNetworkHandler :: Failed to cast NetMessage into IntegrityResNetMessage\n.";
        return;
    }
    auto payload = std::make_shared<IntegrityResPayload>(casted->ResponseID);
    Game::TheGame->SendMessage(std::make_shared<ResponseMessage>(Addressee::NETWORK_SYSTEM, payload));
}

void SketchNetworkHandler::HandleIntegrityData(std::shared_ptr<NetMessage> msg) {
    auto casted = std::dynamic_pointer_cast<IntegrityDataNetMessage>(msg);
    if (!casted) {
        std::cerr << "SketchNetworkHandler :: Failed to cast NetMessage into IntegrityDataNetMessage.\n";
        return;
    }
    auto startIdx = casted->StartVoxelIdx;
    auto numOfVoxels = casted->DataSize;
    std::vector<SK::UINT8> voxelData;
    voxelData.reserve(static_cast<SK::SIZE_T>(REC_TCP_BUFFER_SIZE) - 29); // max size based on max size of packet
    voxelData.insert(voxelData.begin(), casted->Data.begin(), casted->Data.begin() + numOfVoxels);
    IntegrityDataPayload p{startIdx, numOfVoxels, voxelData};
    Game::TheGame->SendMessage(std::make_shared<IntegrityDataMessage>(Addressee::SCENE, p));
}

void SketchNetworkHandler::HandleStealVoxelRes(std::shared_ptr<NetMessage> msg) {
    auto casted = std::dynamic_pointer_cast<StealVoxelResNetMessage>(msg);
    if (!casted) {
        std::cerr << "Error casting NetMessage to StealVoxelResNetMessage.\n";
        return;
    }
    auto gotVoxel = casted->HasStolenVoxel;
    if (!gotVoxel) { return; }

    auto responseID = casted->ResponseID;
    auto voxelID = casted->VoxelID;
    Connection c(casted->Ip, casted->Port);
    auto payload = std::make_shared<StealVoxelResPayload>(responseID, voxelID,gotVoxel, c);
    Game::TheGame->SendMessage(std::make_shared<ResponseMessage>(Addressee::NETWORK_SYSTEM, payload));
}

void SketchNetworkHandler::HandleStealVoxelReq(std::shared_ptr<NetMessage> msg) {
    auto casted = std::dynamic_pointer_cast<StealVoxelReqNetMessage>(msg);
    if (!casted) {
        std::cerr << "Error casting NetMessage to StealVoxelReqNetMessage\n";
        return;
    }
    Connection c(casted->SenderIP, casted->SenderPort);
    StealVoxelReqPayload payload{ c, casted->VoxelIdx, casted->messageId };
    Game::TheGame->SendMessage(std::make_shared<StealVoxelReqMessage>(Addressee::SCENE, payload));
}

void SketchNetworkHandler::HandleTimedOutRequest(std::shared_ptr<NetMessage> message) {
    switch (message->messageType) {
    case PLAYER_JOIN_REQ:
        HandlePlayerJoinTimeout(message);
        return;
    case STEAL_VOXEL_REQ:
        //Don't do anything, it is what it is.
        return;
    }
    std::cerr << "SketchNetworkHandler :: Failed to handle a timed out request. Type: " << message->messageType << '\n';
}

void SketchNetworkHandler::HandleFulfilledResponse(std::vector<std::shared_ptr<ResponseMessage>>& messages) {
    auto type = messages[0]->GetPayload()->Type;

    switch (type) {
    case PLAYER_JOIN_RES:
        HandlePlayerJoinResponses(messages);
        return;
    case STEAL_VOXEL_RES:
        HandleVoxelStealResponses(messages);
        return;
    case INTEGRITY_RES:
        HandleIntegrityResponses(); //dont care about msgs here, it's go time
        return;
    }
    std::cerr << "SketchNetworkHandler :: Failed to handle response - Type : " << type << '\n';
}

void SketchNetworkHandler::HandleVoxelStealResponses(std::vector<std::shared_ptr<ResponseMessage>>& messages) {
    auto payload = std::dynamic_pointer_cast<StealVoxelResPayload>(messages[0]->GetPayload());
    if (payload->HaveVoxel) {
        auto voxelIdx = payload->VoxelID;
        StolenVoxelPayload thieveryPayload{ payload->Connection, payload->VoxelID };
        //std::cout << "Stolen Voxel " << voxelIdx << '\n';
        Game::TheGame->SendMessage(std::make_shared<StolenVoxelMessage>(Addressee::SCENE, thieveryPayload));
    }
}

void SketchNetworkHandler::HandleIntegrityResponses() {
    auto p = std::make_shared<IntegrityStartNetMessage>(NetworkSystem::GetMessageID());
    Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, p));
    StartIntegrityPayload startIntegrityPayload;
    Game::TheGame->SendMessage(std::make_shared<StartIntegrityMessage>(Addressee::SCENE, startIntegrityPayload));
}

void SketchNetworkHandler::HandleIntegrityTimeout(std::shared_ptr<NetMessage> msg) {
    //Turn off integrity check locally
    //User can just press M to do it agian.
}

void SketchNetworkHandler::HandlePlayerJoinTimeout(std::shared_ptr<NetMessage> msg) {
    //Assume if this timed out that we're the first player.
    auto joinMessage = std::make_shared< PlayerJoinNetMessage>(0, EVERYONE, 0, NetworkSystem::GetMessageID());
    Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, joinMessage));

    Connection nullC(0, 0);
    PlayerJoinPayload joinLocal{ nullC, 0 };
    Game::TheGame->SendMessage(std::make_shared<PlayerJoinMessage>(Addressee::SCENE, joinLocal));
    std::cout << "SketchNetworkHandler :: Player join request timed out. Sending message to make us player 1.\n";
}

void SketchNetworkHandler::HandlePlayerJoinRes(std::shared_ptr<NetMessage> msg) {
    auto casted = std::dynamic_pointer_cast<PlayerJoinResNetMessage>(msg);
    if (!casted) {
        std::cerr << "Error casting NetMessage to PlayerJoinResMessage\n";
        return;
    }
    Connection c(casted->SenderIP, casted->SenderPort);
    auto payload = std::make_shared<PlayerJoinResPayload>(
        casted->respondToID, c, casted->FreeSlot, casted->LocalSlot);
    Game::TheGame->SendMessage(std::make_shared<ResponseMessage>(Addressee::NETWORK_SYSTEM, payload));
}

void SketchNetworkHandler::HandlePlayerJoinReq(std::shared_ptr<NetMessage> msg) {
    auto casted = std::dynamic_pointer_cast<PlayerJoinReqNetMessage>(msg);
    if (!casted) {
        std::cerr << "Error casting NetMessage to PlayerJoinReqMessage\n";
        return;
    }
    //Need to get what player slot is available (lowest to highest)
    auto slot = _game.GetAvailablePlayerSlot();
    auto localSlot = _game.GetLocalPlayerSlot();
    auto respondId = casted->messageId;
    auto p = std::make_shared<PlayerJoinResNetMessage>(
        slot, localSlot, respondId, casted->SenderIP, casted->SenderPort, NetworkSystem::GetMessageID()
        );
    Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, p));
}

void SketchNetworkHandler::HandlePlayerJoin(std::shared_ptr<NetMessage> msg) {
    auto casted = std::dynamic_pointer_cast<PlayerJoinNetMessage>(msg);
    if (!casted) {
        std::cerr << "Error casting NetMessage to PlayerJoinMessage\n";
        return;
    }
    Connection c(casted->SenderIP, casted->SenderPort);
    auto slot = casted->Slot;
    PlayerJoinPayload joinLocal{ c, slot };
    Game::TheGame->SendMessage(std::make_shared<PlayerJoinMessage>(Addressee::SCENE, joinLocal));
}

void SketchNetworkHandler::HandlePlayerJoinResponses(std::vector<std::shared_ptr<ResponseMessage>>& messages) {

    std::vector<std::pair<Connection, SK::UINT8>> players;
    std::vector<SK::UINT8> openSlots;

    for (const auto& r : messages) {
        auto payload = std::dynamic_pointer_cast<PlayerJoinResPayload>(r->GetPayload());
        openSlots.push_back(payload->FreeSlot);
        players.push_back(std::make_pair(payload->Sender, payload->SenderSlot));
    }
    SK::BOOL validResponses = true;
    auto testSlot = openSlots[0];
    for (const auto& s : openSlots) {
        if (s != testSlot) {
            validResponses = false;
            break;
        }
    }
    if (testSlot == -1 || testSlot == std::numeric_limits<SK::UINT8>::max()) {
        //Sorry I don't have time to do this nicely,
        // GET OUT THE GAME
        exit(0);
    }

    if (!validResponses) {
        //Re-send a game join request. Do it all again. IN ANOTHER THREAD ** TODO
        SK::UINT8 responseCount = static_cast<SK::UINT8>(players.size());
        auto reqMsg = std::make_shared<PlayerJoinReqNetMessage>(responseCount, EVERYONE, 0, NetworkSystem::GetMessageID());
        Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, reqMsg));
        return;
    }

    // Send Net message to JOINGAME (OpenSlot)
    // We'll then respond from each peer their current board data
    // I.e. their mass -- COLOUR SHOULD BE BASED ON PLAYER SLOT
    auto joinMessage = std::make_shared< PlayerJoinNetMessage>(testSlot, EVERYONE, 0, NetworkSystem::GetMessageID());
    Game::TheGame->SendMessage(std::make_shared<SendNetMessage>(Addressee::NETWORK_SYSTEM, joinMessage));

    Connection nullC(0, 0);
    PlayerJoinPayload joinLocal{ nullC, testSlot };
    Game::TheGame->SendMessage(std::make_shared<PlayerJoinMessage>(Addressee::SCENE, joinLocal));

    for (const auto& m : messages) {
        auto p = std::dynamic_pointer_cast<PlayerJoinResPayload>(m->GetPayload());
        PlayerJoinPayload joinRemote{ p->Sender, p->SenderSlot };
        Game::TheGame->SendMessage(std::make_shared<PlayerJoinMessage>(Addressee::SCENE, joinRemote));
    }

}

void SketchNetworkHandler::HandlePeerShutdown(std::shared_ptr<NetMessage> msg) {
    auto casted = std::dynamic_pointer_cast<ShutdownMessage>(msg);
    if (!casted) {
        std::cerr << "SketchNetworkHandler :: Failed to cast Shutdown message.\n";
        return;
    }
    Connection c(casted->SenderIP, casted->SenderPort);
    PlayerLeftPayload pGame{ c };
    RemoveConnectionPayload pNet{ c };
    Game::TheGame->SendMessage(std::make_shared<PlayerLeftMessage>(Addressee::SCENE, pGame));
    Game::TheGame->SendMessage(std::make_shared<RemoveConnectionMessage>(Addressee::NETWORK_SYSTEM, pNet));

}