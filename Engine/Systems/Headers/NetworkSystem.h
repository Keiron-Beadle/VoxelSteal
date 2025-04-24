#pragma once
#include <queue>
#include <optional>
#include <Windows.h>
#undef SendMessage
#include "System.h"
#include "../../Headers/gil.h"
#include "../../Headers/NetMessages.h"
#include "../../Headers/NetworkHandler.h"

struct ConnectionHash {
	SK::SIZE_T operator()(const std::pair<SK::UINT32, SK::UINT16>& addr) const {
		// Combine the hash values of the two components of the pair
		return std::hash<SK::UINT32>()(addr.first) ^ std::hash<SK::UINT16>()(addr.second);
	}
};

using ConnectionMap = std::unordered_map<Connection, SOCKET, ConnectionHash>;

class NetworkSystem : public System {
private:
#ifdef _DEBUG
	static constexpr SK::FLOAT REQ_TIMEOUT = 2.5; //Secs
#else
	static constexpr SK::FLOAT REQ_TIMEOUT = 2.5; //Secs
#endif
	
	static constexpr SK::UINT16 MIN_TCP_SEND_PORT = 5500;
	static constexpr SK::UINT16 MAX_TCP_SEND_PORT = 5550;

	static constexpr SK::UINT16 MIN_UDP_PORT = 5400;
	static constexpr SK::UINT16 MAX_UDP_PORT = 5450;


	static std::atomic<SK::ULONG> _staticMessageID;

	SK::UINT16 UDP_PORT = MIN_UDP_PORT;
	SK::UINT16 TCP_SEND_PORT = MIN_TCP_SEND_PORT;

private:
	ConnectionMap _connectionMap;
	ResponseMap _responseMap;
	std::unordered_map<SOCKET, std::vector<SK::BYTE>> _tcpBuffers;
	std::unordered_map<std::shared_ptr<NetMessage>, SK::FLOAT> _responseTimeoutMap;
	std::queue<std::shared_ptr<NetMessage>> _inQueue, _outQueue;
	std::vector<SK::UINT16> _usedTCPPorts;
	std::vector<SK::BYTE> _tcpBuffer;

	SOCKET _udpSocket, _tcpSocket; //Used for first time setup
	//Afterwards the ConnectionMap is used.

	std::thread _tcpListener, _udpListener, _sender;

	std::shared_ptr<NetworkHandler> _handler;
	std::condition_variable _outCV;
	std::mutex _inMutex, _outMutex, _responseMutex, _responseTimeoutMutex, _cnnMapMutex;
	std::atomic<SK::BOOL> _stopTCP{ false }, _stopUDP{ false }, _stopSender{ false };
	std::atomic<SK::BOOL> _shouldProcess{ false }, _shouldSend{false};
	std::atomic<SK::BOOL> _senderMoveCore{ false }, _tcpMoveCore{ false }, _udpMoveCore{ false };

public:
	NetworkSystem(SK::UINT32 freq = 20, SK::UINT32 core = 2);
	virtual ~NetworkSystem();

	void SetHandler(std::shared_ptr<NetworkHandler> nh) { _handler = nh; }
	void StartSystem() override;
	void CancelSystem() override;
	void AddEntity(std::shared_ptr<Entity> e) override {}
	void UpdateEntity(std::shared_ptr<Entity> e) override {}
	void OnMessage(std::shared_ptr<Message> msg) override;

	std::optional<std::pair<SK::UINT32, SK::UINT16>> GetAnyConnection() const;

	static SK::ULONG GetMessageID() { return _staticMessageID.fetch_add(1); }
	nodconst SK::SIZE_T GetExternalConnectionCount();

private:
	void HandleSystemReq(std::shared_ptr<SystemMessage> msg);
	void HandleNetReq(std::shared_ptr<SendNetMessage> msg);
	void HandleResponse(std::shared_ptr<ResponseMessage> msg);
	void HandleTimedOutRequest(std::shared_ptr<NetMessage> msg);

	nodconst std::optional<Connection> GetAnyExternalConnection();

	void Sender();
	void Process() override;
	void UDPListening();
	void TCPListening();
	void MoveToCore() override;

	//Returns if the sender should be removed from the _connectionMap
	SK::BOOL CheckForTCPMessage(SOCKET& s, std::vector<SK::BYTE>& data,
		fd_set& set, const std::pair<SK::UINT32, SK::UINT16>& senderAddr);

	void ProcessTCPMessage(std::vector<SK::BYTE>& data, const Connection& senderAddr);
	void SendQueueItem(std::shared_ptr<NetMessage>);
	void SendTCPMessage(std::shared_ptr<TCPMessage>);
	void FlushTCPBuffer(const SOCKET& s, std::vector<SK::BYTE>& data);
	SK::INT32 SendDataToSocket(const SOCKET& s, std::vector<SK::BYTE>& data);
	void SendDataToEveryone(std::vector<SK::BYTE>& data);
	void SendUDPMessage(std::shared_ptr<NetMessage>);

	//Returns if the connection should be removed from the _connectionMap
	SK::BOOL HandleSendErrors(const Connection& c);

	SOCKET CreateTCPConnection(std::pair<SK::UINT32, SK::UINT16>& key);
};