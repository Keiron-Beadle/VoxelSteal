#include <iostream>
#include <random>
#include "../Headers/NetworkSystem.h"
#include "../../Headers/Message.h"
#include "../../Headers/Game.h"

std::random_device rd;
std::mt19937 gen(rd());

std::atomic<SK::ULONG> NetworkSystem::_staticMessageID = 0;

SK::BOOL operator==(const std::pair<in_addr, SK::UINT16>& lhs, const std::pair<in_addr, SK::UINT16>& rhs) {
	return lhs.first.s_addr == rhs.first.s_addr && lhs.second == rhs.second;
}

NetworkSystem::NetworkSystem(SK::UINT32 freq, SK::UINT32 core) : System(freq, core), 
_handler(nullptr), _tcpSocket(INVALID_SOCKET), _udpSocket(INVALID_SOCKET)
{
	Address |= Addressee::NETWORK_SYSTEM;
}

NetworkSystem::~NetworkSystem()
{

}

void NetworkSystem::StartSystem()
{
	if (!_handler) {
		throw std::exception("Network system did not have Handler setup. Call .SetHandler(...)");
	}

	//Setup winsock
	WSADATA wd{};
	SK::INT32 result = WSAStartup(MAKEWORD(2, 2), &wd);
	if (result != 0) throw std::exception("Error starting winsock.");

	//Create UDP Socket
	_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (_udpSocket == INVALID_SOCKET) {
		throw std::exception("UDP Socket was invalid.");
	}
	SK::BYTE broadcastEnable = 1;
	SK::INT32 r = setsockopt(_udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
	if (r == SOCKET_ERROR) throw std::exception("Failed to set BROADCAST options.");

	_udpListener = std::thread([this]() { UDPListening(); });
	Sleep(100);
	_tcpListener = std::thread([this]() { TCPListening(); });
	_sender = std::thread([this]() { Sender(); });
	System::StartSystem();

	//Send a UDP broadcast, with the TCP Listener port
	auto m = std::make_shared<UDPMessage>(
		static_cast<SK::UINT16>(UDP_PORT), //TCP Listener should be same port as UDP PORT
		NetworkSystem::GetMessageID()
		);
	OnMessage(std::make_shared<SendNetMessage>(Addressee::ALL, m));
}

void NetworkSystem::CancelSystem()
{
	_shouldJoin = true;
	System::CancelSystem();

	SK::BYTE buffer[256];
	SK::INT32 r;
	_stopTCP = true;
	_stopUDP = true;
	_stopSender = true;
	if (shutdown(_udpSocket, 2) == SOCKET_ERROR)
		std::cerr << "Failed to shutdown UDP Socket: " << WSAGetLastError() << '\n';

	
	auto shutdownMessage = ShutdownMessage{ GetMessageID() };
	auto shutdownBytes = shutdownMessage.Serialize();
	for (auto& [p, s] : _connectionMap) {
		//Send goodbye message via TCP
		if (p.first != 0) {
			SK::INT32 bytesSent = send(s, shutdownBytes.data(), shutdownBytes.size(), 0);
			if (bytesSent == SOCKET_ERROR) {
				auto errCode = WSAGetLastError();
				std::cerr << "NetworkSystem :: Failed to send goodbye.\n";
			}
		}
	
		//Afterwards close, such that a 'FIN' is sent.
		if (shutdown(s, 2) == SOCKET_ERROR)
			std::cerr << "Failed to shutdown socket (ErrorCode): " << WSAGetLastError() << '\n';

		//Finally, close the socket
		if (closesocket(s) == SOCKET_ERROR)
			std::cerr << "Failed to close socket (ErrorCode): " << WSAGetLastError() << '\n';
	}
	_connectionMap.clear();
	if (closesocket(_udpSocket) == SOCKET_ERROR) {
		std::cerr << "Failed to close UDP Socket: " << WSAGetLastError() << '\n';
	}
	WSACleanup();

	//Join the TCP Listener
	if (_tcpListener.joinable()) {
		_tcpListener.join();
	}

	//Join the UDP Listener
	if (_udpListener.joinable()) {
		_udpListener.join();
	}

	//Join the Sender 
	if (_sender.joinable()) {
		_outCV.notify_one();
		_sender.join();
	}
}

void NetworkSystem::OnMessage(std::shared_ptr<Message> msg)
{
	auto type = msg->Type();
	if ((Address & type) == 0) return;

	auto netReq = std::dynamic_pointer_cast<SendNetMessage>(msg);
	if (netReq) {
		HandleNetReq(netReq);
		return;
	}

	auto resMsg = std::dynamic_pointer_cast<ResponseMessage>(msg);
	if (resMsg) {
		HandleResponse(resMsg);
		return;
	}

	auto systemReq = std::dynamic_pointer_cast<SystemMessage>(msg);
	if (systemReq) {
		HandleSystemReq(systemReq);
		return;
	}

	auto removeConn = std::dynamic_pointer_cast<RemoveConnectionMessage>(msg);
	if (removeConn) {
		auto& payload = removeConn->GetPayload();
		auto c = payload.c;
		{
			std::unique_lock<std::mutex> lock(_cnnMapMutex);
			if (!_connectionMap.erase(c)) 
				std::cerr << "NetworkSystem :: Failed to remove " << c.first << ':' << c.second << " from connection map.\n";
		}
		return;
	}
}

std::optional<std::pair<SK::UINT32, SK::UINT16>> NetworkSystem::GetAnyConnection() const
{
	for (auto& [key, value] : _connectionMap) {
		if (key.first == 0) { continue; }
		return key;
	}
	return std::nullopt;
}

void NetworkSystem::Sender()
{
	auto core = GetLeastUtilisedCore();
	HANDLE h = _sender.native_handle();
	DWORD_PTR affinityMask = static_cast<DWORD_PTR>(1) << core;
	if (!SetThreadAffinityMask(h, affinityMask))
		std::cerr << "NetworkSystem :: Failed to move Sender Core\n";

	while (!_stopSender) {
		{
			std::unique_lock<std::mutex> lock(_outMutex);
			if (!_shouldSend) continue;
			if (_stopSender)  break;
			while (!_outQueue.empty()) {
				auto& msg = _outQueue.front();
				SendQueueItem(msg);
				_outQueue.pop();
			}
			for (auto& [s, d] : _tcpBuffers) {
				FlushTCPBuffer(s, d);
				d.clear();
			}
			_shouldSend = false;
		}

		if (_senderMoveCore) {
			DWORD_PTR affinityMask = static_cast<DWORD_PTR>(1) << core;
			std::cout << "NetworkSystem :: Moved Sender Core\n";
			if (!SetThreadAffinityMask(h, affinityMask))
				std::cerr << "NetworkSystem :: Failed to move Sender Core\n";
			_senderMoveCore = false;
		}
	}
}

void NetworkSystem::Process()
{
	_shouldSend = true;
	SK::ULONG removeMessageID = -1;
	{
		std::shared_ptr<NetMessage> removeMessage = nullptr;
		std::unique_lock<std::mutex> lock(_responseTimeoutMutex);
		for (auto& res : _responseTimeoutMap) {
			res.second -= dT();
			if (res.second < 0) {
				HandleTimedOutRequest(res.first);
				removeMessageID = res.first->messageId;
				removeMessage = res.first;
			}
		}
		if (removeMessage) { _responseTimeoutMap.erase(removeMessage); }
	}
	if (removeMessageID >= 0)
	{
		std::unique_lock<std::mutex> lock(_responseMutex);
		_responseMap.erase(removeMessageID);
	}
	{
		std::unique_lock<std::mutex> lock(_inMutex);
		if (!_shouldProcess) return;
		while (!_inQueue.empty()) {
			auto& netMsg = _inQueue.front();
			_handler->ProcessNetMessage(netMsg);
			_inQueue.pop();
		}
		_shouldProcess = false;
	}
}

void NetworkSystem::UDPListening()
{
	auto core = GetLeastUtilisedCore();
	HANDLE h = _udpListener.native_handle();
	DWORD_PTR affinityMask = static_cast<DWORD_PTR>(1) << core;
	if (!SetThreadAffinityMask(h, affinityMask))
		std::cerr << "NetworkSystem :: Failed to move UDP Listener Core\n";

	SK::INT32 attempts = 3;
retryBind:
	sockaddr_in recv{};
	recv.sin_family = AF_INET;
	recv.sin_port = htons(UDP_PORT);
	recv.sin_addr.s_addr = htonl(INADDR_ANY);
	SK::INT32 r = bind(_udpSocket, (sockaddr*)&recv, sizeof(recv));
	if (r == SOCKET_ERROR) {
		SK::INT32 errCode = WSAGetLastError();
		if (errCode == WSAEINVAL && attempts > 0) {
			--attempts;
			goto retryBind;
		}
		else if (errCode == WSAEADDRINUSE && UDP_PORT < MAX_UDP_PORT) {
			++UDP_PORT;
			goto retryBind;
		}
		throw std::exception("Failed to bind udp send socket.");
	}

	SK::BYTE buffer[256];
	SK::INT32 readBytes;
	struct sockaddr_in senderAddress {};
	SK::INT32 senderAddressLength = sizeof(senderAddress);
	while (!_stopUDP) {
		readBytes = recvfrom(_udpSocket, buffer, sizeof(buffer), 0, (sockaddr*)&senderAddress, &senderAddressLength);

		//Check to move core.
		if (_udpMoveCore) {
			DWORD_PTR affinityMask = static_cast<DWORD_PTR>(1) << core;
			std::cout << "NetworkSystem :: Moved UDP Core\n";
			if (!SetThreadAffinityMask(h, affinityMask))
				std::cerr << "NetworkSystem :: Failed to move UDP Core\n";
			_udpMoveCore = false;
		}

		if (readBytes == 0) {
			std::cerr << "NetworkSystem :: UDP Listener closed. Got 0 bytes.\n";
			break;
		}
		if (readBytes == SOCKET_ERROR) {
			SK::INT32 errCode = WSAGetLastError();
			std::cerr << "NetworkSystem :: UDP Listener socket error. Code: " << errCode << '\n';
			continue;
		}
		SK::INT32 netPID;
		std::memcpy(&netPID, &buffer[4], sizeof(netPID));
		SK::INT32 pid = ntohl(netPID);
		if (pid == CURRENT_PROCESS_ID) { continue; }

		SK::UINT16 netPort;
		std::memcpy(&netPort, &buffer[15], sizeof(netPort));
		SK::UINT16 port = ntohs(netPort);

		auto netIp = senderAddress.sin_addr.s_addr;
		auto hostIp = ntohl(netIp);

		std::cout << "Received UDP: " << hostIp << ':' << port << std::endl;

		//Send a TCP Connection request to sender.
		auto tcc= std::make_shared<TCPMessage>(
			hostIp,
			port,
			NetworkSystem::GetMessageID() );
		OnMessage(std::make_shared<SendNetMessage>(Addressee::ALL, tcc));
	}
}

void NetworkSystem::TCPListening()
{
	auto core = GetLeastUtilisedCore();
	HANDLE h = _tcpListener.native_handle();
	DWORD_PTR affinityMask = static_cast<DWORD_PTR>(1) << core;
	if (!SetThreadAffinityMask(h, affinityMask))
		std::cerr << "NetworkSystem :: Failed to move TCP Core\n";

	_tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_tcpSocket == INVALID_SOCKET) {
		throw std::exception("TCP Socket was invalid.");
	}
	struct sockaddr_in local {};
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_port = htons(UDP_PORT);
	std::cout << "Bound TCP Listener to: " << INADDR_ANY << ':' << UDP_PORT << std::endl;
	SK::INT32 r = bind(_tcpSocket, (struct sockaddr*)&local, sizeof(local));
	if (r == SOCKET_ERROR) {
		SK::INT32 errCode = WSAGetLastError();
		if (errCode == WSAEADDRINUSE) {
			std::cerr << "TCP Local Listener - Address already in use." << std::endl;
		}
		std::cerr << "Failed to bind local TCP Listener, Error Code: " << errCode << std::endl;
		throw std::exception("Failed to bind.");
	}
	auto pair = std::make_pair(ntohl(local.sin_addr.s_addr), UDP_PORT);
	{
		std::lock_guard<std::mutex> lock(_cnnMapMutex);
		_connectionMap[pair] = _tcpSocket;
	}
	r = listen(_tcpSocket, SOMAXCONN);
	if (r == SOCKET_ERROR) {
		SK::INT32 errCode = WSAGetLastError();
		if (errCode == WSAEADDRINUSE) {
			std::cerr << "TCP Listener - Listen call failed - Socket local address in use." << std::endl;
			throw std::exception("TCP Listener address in use.");
		}
		throw std::exception("Error calling listen on socket.");
	}
	fd_set readSet{};
	for (auto& cnn : _connectionMap){
		auto s = cnn.second;
		FD_SET(s, &readSet);
	}

	fd_set peerSet{};
	FD_SET(_tcpSocket, &peerSet);

	timeval timeout{};
	timeout.tv_sec = 0;
	timeout.tv_usec = 50000;
	timeval peerTimeout{};
	peerTimeout.tv_sec = 0;
	peerTimeout.tv_usec = 50000;

	std::vector<SK::BYTE> msgData;
	std::vector< std::pair<SK::UINT32, SK::UINT16>> removeList;
	msgData.reserve(10000);
	while (!_stopTCP) {
		removeList.clear();

		if (_tcpMoveCore) {
			DWORD_PTR affinityMask = static_cast<DWORD_PTR>(1) << core;
			std::cout << "NetworkSystem :: Moved TCP Core\n";
			if (!SetThreadAffinityMask(h, affinityMask))
				std::cerr << "NetworkSystem :: Failed to move TCP Core\n";
			_tcpMoveCore = false;
		}

		SK::INT32 peerReady = select(0, &peerSet, nullptr, nullptr, &peerTimeout);
		if (peerReady < 0) {
			auto errCode = WSAGetLastError();
			if (errCode == WSAENOTSOCK) {
				std::cerr << "Peer select failed with code WSAENOTSOCK. Clearing socket from set and returning." << std::endl;
				FD_CLR(_tcpSocket, &peerSet);
				break;
			}
			//throw std::exception("Peer select failed.");
			std::cerr << "Peer select failed in TCP Listening." << std::endl;
		}

		if (FD_ISSET(_tcpSocket, &peerSet)) {
			struct sockaddr_in peerAddr {};
			SK::INT32 addrSize = sizeof(peerAddr);
			SOCKET newPeer = accept(_tcpSocket, (sockaddr*)&peerAddr, &addrSize);
			if (newPeer == INVALID_SOCKET) {
				auto errCode = WSAGetLastError();
				std::cerr << "Failed to get peer, Error: " << errCode << '\n';
				if (errCode == WSAENOTSOCK) {
					std::cerr << "Assuming _tcpSocket is dead. Leaving listener thread." << std::endl;
					return;
				}
				//throw std::exception("Failed to get peer.");
			}
			auto ipHost = ntohl(peerAddr.sin_addr.s_addr);
			auto portHost = ntohs(peerAddr.sin_port);
			auto pair = std::make_pair(ipHost, portHost);
			{
				std::lock_guard<std::mutex> lock(_cnnMapMutex);
				_connectionMap[pair] = newPeer;
			}
			FD_SET(newPeer, &readSet);
			std::cout << "Accepted connection from: " << ipHost << ':' << portHost << std::endl;
		}
		FD_SET(_tcpSocket, &peerSet);

		SK::INT32 ready = select(0, &readSet, nullptr, nullptr, &timeout);
		if (ready < 0) {
			auto errCode = WSAGetLastError();
			std::cerr << "Read select failed. Error: " << errCode << std::endl;
			//throw std::exception("Select failed.");
		}
		{
			std::unique_lock<std::mutex> lock(_inMutex);
			if (_stopTCP.load()) { return; }
			for (auto& [p, s] : _connectionMap) {
				if (CheckForTCPMessage(s, msgData, readSet, p)) {
					removeList.push_back(p);
					std::cout << "Removing " << p.first << ':' << p.second << " from connection map." << std::endl;
				}
			}
		}
		if (!removeList.empty()) {
			std::lock_guard<std::mutex> lock(_cnnMapMutex);
			for (const auto& p : removeList) {
				_connectionMap.erase(p);	
				ConnectionDiedPayload payload{ p };
				Game::TheGame->SendMessage(std::make_shared<ConnectionDiedMessage>(Addressee::SCENE, payload));
			}
		}
	}
}

SK::BOOL NetworkSystem::CheckForTCPMessage(SOCKET& s, std::vector<SK::BYTE>& data, fd_set& set, 
	const std::pair<SK::UINT32, SK::UINT16>& senderAddr)
{
	if (!FD_ISSET(s, &set)) {
		FD_SET(s, &set);
		return false;
	}
	if (senderAddr.first == 0) { FD_SET(s, &set); return false; }
	static SK::BYTE buffer[1024];
	static SK::INT32 remainingBytes = 0;
	static SK::INT32 msgSize = 0;
readMessage:
	memset(buffer, 0, sizeof(buffer));
	SK::INT32 bytesRead = recv(s, buffer, 4, 0);
	if (bytesRead == 0) {
		std::cerr << "Got 0 bytes returned from socket " << s << '\n';
		std::cerr << "Assuming socket died. Removing from connection pool.\n";
		FD_CLR(s, &set);
		return true;
	}
	else if (bytesRead < 0) {
		auto  errCode = WSAGetLastError();
		if (errCode == WSAECONNRESET) {
			std::cerr << "Connection reset by peer. CheckForTCPMessage()." << std::endl;
			return true;
		}
		else if (errCode == WSAENOTCONN) {
			std::cerr << "Socket is not connected. CheckForTCPMessage()." << std::endl;
			return true;
		}
		std::cerr << "Error reading bytes from recv(),  Code: " << errCode
			<< " IP: " << senderAddr.first << " Port: " << senderAddr.second << '\n';
		return false;
	}
	std::memcpy(&msgSize, buffer, sizeof(msgSize));
	msgSize = ntohl(msgSize);
	remainingBytes = msgSize-4;
	if (remainingBytes < 0) {
		SK::INT32 i = 0;
	}
	while (remainingBytes > 0) {
		memset(buffer, 0, sizeof(buffer));
		SK::INT32 len = min(remainingBytes, sizeof(buffer));
		bytesRead = recv(s, buffer, len, 0);
		if (bytesRead <= 0) {
			std::cerr << "Got 0 bytes returned from socket " << s << '\n';
			FD_CLR(s, &set);
			return false;
		}
		data.insert(data.end(), buffer, buffer + bytesRead);
		remainingBytes -= bytesRead;
	}
	ProcessTCPMessage(data, senderAddr);
	data.clear();

	fd_set tempSet = set;
	timeval timeout = { 0,0 };
	SK::INT32 r = select(0, &tempSet, nullptr, nullptr, &timeout);
	if (r < 0) {
		std::cerr << "Failed to select from temporary set in CheckForTCPMessage." << std::endl;
	}
	else if (r > 0 && FD_ISSET(s,&tempSet)) {
		msgSize = 0;
		remainingBytes = 0;
		goto readMessage;
	}
	
	FD_SET(s, &set);
	return false;
}

void NetworkSystem::ProcessTCPMessage(std::vector<SK::BYTE>& data, const Connection& senderAddr)
{
 	auto message = _handler->CreateMessage(data, senderAddr);
	if (message) {
		_inQueue.push(message);
		_shouldProcess = true;
	}
	data.clear();
}

void NetworkSystem::SendQueueItem(std::shared_ptr<NetMessage> msg)
{
	if (!msg->isTCP) {
		auto udpMessage = std::dynamic_pointer_cast<UDPMessage>(msg);
		if (!udpMessage) {
			std::cerr << "Error dynamically casting NetMessage to UDP Message\n";
			throw std::exception("Error dynamically casting NetMessage to UDP Message");
		}
		SendUDPMessage(msg);
		return;
	}
	auto tcpMessage = std::dynamic_pointer_cast<TCPMessage>(msg);
	if (!tcpMessage) {

		std::cerr << "Error dynamically casting NetMessage to TCP Message\n";
		throw std::exception("Error dynamically casting NetMessage to TCP Message");
	}
	SendTCPMessage(tcpMessage);
}

void NetworkSystem::SendTCPMessage(std::shared_ptr<TCPMessage> msg)
{
	if (msg->Ip == EVERYONE) {
		auto buffer = msg->Serialize();
		SendDataToEveryone(buffer);
		return;
	}

	bool anyoneConnection = false;
	if (msg->Ip == ANYONE) {
		auto optConnection = GetAnyExternalConnection();
		if (!optConnection.has_value()) {
			//std::cerr << "NetworkSystem :: Failed to get any external connection.\n";
			return;
		}
		auto& connection = optConnection.value();
		msg->Ip = connection.first;
		msg->Port = connection.second;
		anyoneConnection = true;
	}

	auto ip = msg->Ip;
	auto port = msg->Port;
	auto buffer = msg->Serialize();

	auto pair = std::pair<SK::UINT32, SK::UINT16>(ip, port);
	if (!anyoneConnection && _connectionMap.find(pair) == _connectionMap.end()) {
		std::lock_guard<std::mutex> lock(_cnnMapMutex);
		_connectionMap[pair] = CreateTCPConnection(pair);
	}

	auto socket = _connectionMap[pair];
	_tcpBuffers[socket].insert(_tcpBuffers[socket].end(), buffer.begin(), buffer.end());

	if (_tcpBuffers[socket].size() >= REC_TCP_BUFFER_SIZE) {
		FlushTCPBuffer(socket, _tcpBuffers[socket]);
	}

	//auto s = _connectionMap[pair];

	//if (SendDataToSocket(buffer, s) == SOCKET_ERROR) {
	//	if (HandleSendErrors(pair)) {
	//		{
	//			std::lock_guard<std::mutex> lock(_cnnMapMutex);
	//			_connectionMap.erase(pair);
	//		}
	//	}
	//}
}

void NetworkSystem::SendUDPMessage(std::shared_ptr<NetMessage> msg)
{
	SK::INT32 attempts = 3;
	struct sockaddr_in broadcast {};
	auto buffer = msg->Serialize();
	broadcast.sin_family = AF_INET;
	broadcast.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	std::cout << "Our UDP Port: " << UDP_PORT << std::endl;
	//Broadcast our message to all ports in the valid range, don't broadcast to ourselves.
	for (SK::UINT16 port = MIN_UDP_PORT; port < MAX_UDP_PORT; ++port) {
		if (port == UDP_PORT) continue;
		broadcast.sin_port = htons(port);
		SK::INT32 r = sendto(_udpSocket, buffer.data(), buffer.size(), 0, (sockaddr*)&broadcast, sizeof(broadcast));
		if (r == SOCKET_ERROR) {
			SK::INT32 errCode = WSAGetLastError();
			continue;
		}
	}
}

SOCKET NetworkSystem::CreateTCPConnection(std::pair<SK::UINT32, SK::UINT16>& key)
{
	std::cout << "Creating TCP: " << key.first << ':' << key.second << std::endl;
	SOCKET tcp = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (tcp == INVALID_SOCKET) {
		throw std::exception("TCP Socket was invalid.");
	}
	struct sockaddr_in local{};
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
tryAnotherPort:
	local.sin_port = htons(TCP_SEND_PORT);
	SK::INT32 r = bind(tcp, (struct sockaddr*)&local, sizeof(local));
	if (r == SOCKET_ERROR) {
		SK::INT32 errCode = WSAGetLastError();
		if (errCode == WSAEADDRINUSE && TCP_SEND_PORT < MAX_TCP_SEND_PORT) {
			TCP_SEND_PORT++;
			goto tryAnotherPort;
		}
		throw std::exception("Failed to bind.");
	}
	std::cout << "Our TCP send port: " << TCP_SEND_PORT << std::endl;
	struct sockaddr_in remote {};
	remote.sin_family = AF_INET;
	remote.sin_addr.s_addr = htonl(key.first);
	remote.sin_port = htons(key.second);
	SK::INT32 retryAttempts = 3;
retryConnection:
	r = connect(tcp, (struct sockaddr*)&remote, sizeof(remote));
	if (r == SOCKET_ERROR) {
		SK::INT32 errCode = WSAGetLastError();
		if (errCode == WSAECONNREFUSED) {
			std::cerr << "Connection refused. Tried to connect to " << key.first << ':' << key.second << std::endl;
			if (--retryAttempts > 0) { goto retryConnection; }
		}
		throw std::exception("Failed to connect to remote.");
	}
	std::cout << "Connected to: " << key.first << ':' << key.second << std::endl;

	return tcp;
}

void NetworkSystem::HandleSystemReq(std::shared_ptr<SystemMessage> msg) {

	auto& payload = msg->GetPayload();
	switch (payload.instruction) {
	case 0:
		SetFrequency(payload.value);
		break;
	case 1:
		SetCore(payload.value);
		break;
	}
}

void NetworkSystem::HandleNetReq(std::shared_ptr<SendNetMessage> msg) {
	auto& payload = msg->GetPayload();
	if (payload->isRequest) {
		auto castedPayload = std::dynamic_pointer_cast<TCPReqMessage>(payload);
		if (!castedPayload) {
			std::cerr << "NetworkSystem :: Failed to cast NetMessage to TCPReqMessage in HandleNetRequest()\n";
		}
		else 
		{
			{
				std::unique_lock<std::mutex> responseLock(_responseMutex);
				auto vec = std::vector < std::shared_ptr<ResponseMessage>>();
				auto expResponseCount = castedPayload->ExpectedResponses;
				auto pair = std::make_pair(vec, expResponseCount);
				_responseMap[payload->messageId] = pair;
			}
			{
				std::unique_lock<std::mutex> waitLock(_responseTimeoutMutex);
				_responseTimeoutMap[payload] = REQ_TIMEOUT; //Start timer for this
			}
		}
	}
	std::lock_guard<std::mutex> outLock(_outMutex);
	_outQueue.push(payload);
	_outCV.notify_one();
}

nodconst std::optional<Connection> NetworkSystem::GetAnyExternalConnection() {
	SK::BOOL canSend = false;
	for (const auto& [k, v] : _connectionMap) {
		if (k.first != 0)
			canSend = true;
	}
	if (!canSend) {
		//std::cerr << "Can't send TCP message as only IP 0 exists in the connection map\n";
		return std::nullopt;
	}
	if (_connectionMap.size() <= 1) { return std::nullopt; }
	std::uniform_int_distribution<> dis(0, _connectionMap.size()-1);
	
	Connection address;
	do {
		auto it = _connectionMap.begin();
		std::advance(it, dis(gen));
		address = it->first;
	} while (address.first == 0);

	return address;
}

nodconst SK::SIZE_T NetworkSystem::GetExternalConnectionCount() {
	SK::SIZE_T counter = 0;
	for (const auto& [k, v] : _connectionMap) {
		if (k.first == 0) continue;
		++counter;
	}
	return counter;
}

SK::INT32 NetworkSystem::SendDataToSocket(const SOCKET& s, std::vector<SK::BYTE>& data) {
	return send(s, data.data(), data.size(), 0);
}

void NetworkSystem::FlushTCPBuffer(const SOCKET& pSocket, std::vector<SK::BYTE>& data) {
	Connection connection;
	for (const auto& [p, s] : _connectionMap) {
		if (pSocket == s) {
			connection = p;
			break;
		}
	}
	if (SendDataToSocket(pSocket,data) == SOCKET_ERROR) {
		if (HandleSendErrors(connection)) {
			{
				std::lock_guard<std::mutex> lock(_cnnMapMutex);
				_connectionMap.erase(connection);
			}
		}
	}
}

SK::BOOL NetworkSystem::HandleSendErrors(const Connection& c) {
	SK::INT32 errCode = WSAGetLastError();
	if (errCode == WSAECONNRESET) {
		return true;
	}
	else if (errCode == WSAENOTCONN) {
		std::cerr << "Socket not conected when sending TCP. " << c.first << ':' << c.second << std::endl;
		return true;
	}
	std::cerr << "<<SOCKET ERROR>> Code:" << errCode << " SendTCPMessage()" << std::endl;
	return false;
}

void NetworkSystem::SendDataToEveryone(std::vector<SK::BYTE>& data) {
	std::vector<Connection> deleteQueue;
	for (auto& [c, s] : _connectionMap) {
		if (c.first == 0) continue;
		if (SendDataToSocket(s, data) == SOCKET_ERROR)
			if (HandleSendErrors(c))
				deleteQueue.push_back(c);
	}
	{
		std::lock_guard<std::mutex> lock(_cnnMapMutex);
		for (auto& c : deleteQueue)
			_connectionMap.erase(c);
	}
}

void NetworkSystem::HandleResponse(std::shared_ptr<ResponseMessage> msg) {
	auto& payload = msg->GetPayload();
	auto id = payload->MessageID;
	SK::BOOL fulfilled = false;
	{
		std::unique_lock<std::mutex> lock(_responseMutex);
		if (_responseMap.find(id) == _responseMap.end()) { return; }
		auto& vec = _responseMap[id].first;
		auto count = _responseMap[id].second;
		vec.push_back(msg);
		if (vec.size() >= count) fulfilled = true;
	}
	{
		std::unique_lock<std::mutex> lock(_responseTimeoutMutex);
		if (fulfilled) {
			std::shared_ptr<NetMessage> removeMessage;
			for (const auto& [k, v] : _responseTimeoutMap) {
				if (k->messageId == id) {
					removeMessage = k;
					break;
				}
			}
			_responseTimeoutMap.erase(removeMessage);
		} 
		else {
			for (auto& [k, v] : _responseTimeoutMap) {
				if (k->messageId == id) {
					_responseTimeoutMap[k] = REQ_TIMEOUT;
					break;
				}
			}
		}
	}
	{
		std::unique_lock<std::mutex> lock(_responseMutex);
		auto& vec = _responseMap[id].first;
		auto count = _responseMap[id].second;
		if (vec.size() < count) return; 
		_handler->HandleFulfilledResponse(vec);
		_responseMap.erase(id);
	}
}

void NetworkSystem::HandleTimedOutRequest(std::shared_ptr<NetMessage> msg) {
	_handler->HandleTimedOutRequest(msg);
}

void NetworkSystem::MoveToCore() {
	System::MoveToCore();
	_senderMoveCore = true;
	_tcpMoveCore = true;
	_udpMoveCore = true;
}