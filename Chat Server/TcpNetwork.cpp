#include "TcpNetwork.h"


#include <stdio.h>
#include <vector>
#include <deque>

#include "ILog.h"


namespace ServerNetLib
{
	TcpNetwork::TcpNetwork() {}

	TcpNetwork::~TcpNetwork() 
	{
		// delete all buffers in the Client_SessionPool
		for (auto& client : m_ClientSessionPool)
		{
			if (client.pRecvBuffer) {
				delete[] client.pRecvBuffer;
			} 

			if (client.pSendBuffer) {
				delete[] client.pSendBuffer;
			}
		}
	}

	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig* pConfig, ILog* pLogger)
	{
		memcpy(&m_Config, pConfig, sizeof(ServerConfig));

		m_pRefLogger = pLogger;

		auto initRet = InitServerSocket();
		if (initRet != NET_ERROR_CODE::NONE)
		{
			return initRet;
		}

		auto bindListenRet = BindListen(pConfig->Port, pConfig->BackLogCount);
		if (bindListenRet != NET_ERROR_CODE::NONE)
		{
			return bindListenRet;
		}

		FD_ZERO(&m_Readfds);
		FD_SET(m_ServerSockfd, &m_Readfds);

		auto SessionPoolSize = CreateSessionPool(pConfig->MaxClientCount + pConfig->ExtraClientCount);

		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Session Pool Size: %d", __FUNCTION__, SessionPoolSize);

		return NET_ERROR_CODE::NONE;
	}


	void TcpNetwork::Release()
	{
		// clean up the Networks;
		WSACleanup();
	}

	NET_ERROR_CODE TcpNetwork::SendData(const int SessionIdx, const short packetId, const short size, const char* pMsg)
	{
		auto& session = m_ClientSessionPool[SessionIdx];

		auto pos = session.SendSize;
		if ((pos + size + PACKET_HEADER_SIZE) > m_Config.MaxClientSendBufferSize) {
			return NET_ERROR_CODE::CLIENT_SEND_BUFFER_FULL;
		}

		PacketHeader pktHeader{ packetId, size };
		memcpy(&session.pSendBuffer[pos], (char*)&pktHeader, sizeof(PACKET_HEADER_SIZE));
		memcpy(&session.pSendBuffer[pos + PACKET_HEADER_SIZE], pMsg, size);
		session.SendSize += (size + PACKET_HEADER_SIZE);

		return NET_ERROR_CODE::NONE;
	}

	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo packetInfo;

		if (m_PacketQueue.empty() == false) {
			packetInfo = m_PacketQueue.front();
			m_PacketQueue.pop_front();
		}

		return packetInfo;
	}

	void TcpNetwork::ForcingClose(const int sessionIndex)
	{
		if (m_ClientSessionPool[sessionIndex].IsConnected() == false)
			return;

		CloseSession(SOCKET_CLOSE_CASE::FORCING_CLOSE, m_ClientSessionPool[sessionIndex].SocketFD, sessionIndex);
	}

	void TcpNetwork::Run()
	{
		auto read_set = m_Readfds;
		auto write_set = m_Readfds;

		timeval timeout{ 0, 1000 }; // tv_sec, tv_usec
		auto selectResult = select(0, &read_set, &write_set, 0, &timeout);

		auto isFDSetChanged = RunCheckSelectResult(selectResult);
		if (isFDSetChanged == false)
		{
			return;
		}

		// Accept
		if (FD_ISSET(m_ServerSockfd, &read_set))
		{
			NewSession();
		}

		RunCheckSelectClients(read_set, write_set);
	}

	NET_ERROR_CODE TcpNetwork::InitServerSocket()
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);

		m_ServerSockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_ServerSockfd < 0) {
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
		}

		auto n = 1;
		// set server socket to reuseable socket.
		if (setsockopt(m_ServerSockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;
		}

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::BindListen(short port, int backlogCount)
	{
		SOCKADDR_IN server_addr;
		ZeroMemory(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
		server_addr.sin_port = ::htons(port);

		if (bind(m_ServerSockfd, (SOCKADDR*)&server_addr, sizeof(server_addr)) < 0) {
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;
		}

		unsigned long mode = 1;
		if (ioctlsocket(m_ServerSockfd, FIONBIO, &mode) == SOCKET_ERROR)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_FIONBIO_FAIL;
		}

		if (listen(m_ServerSockfd, backlogCount) == SOCKET_ERROR) {
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;
		}

		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockfd(%I64u)", __FUNCTION__, m_ServerSockfd);
		return NET_ERROR_CODE::NONE;
	}

	bool TcpNetwork::RunCheckSelectResult(const int result)
	{
		if (result == 0)
			return false;
		else if (result == -1)
			return false;
		return true;
	}

	void TcpNetwork::RunCheckSelectClients(fd_set& read_set, fd_set& write_set)
	{
		for (auto& session : m_ClientSessionPool) {

			if (session.IsConnected() == false)
				continue;

			SOCKET fd = session.SocketFD;
			auto sessionIndex = session.Index;

			auto retReceive = RunProcessReceive(sessionIndex, fd, read_set);
			if (retReceive == false)
				continue;

			RunProcessWrite(sessionIndex, fd, write_set);
		}
	}

	bool TcpNetwork::RunProcessReceive(const int sessionIndex, const SOCKET fd, fd_set& read_set)
	{
		if (!FD_ISSET(fd, &read_set))
			return true;

		auto ret = RecvSocket(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE) {
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, fd, sessionIndex);
			return false;
		}

		ret = RecvBufferProcess(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, fd, sessionIndex);
			return false;
		}

		return true;
	}



	int TcpNetwork::AllocClientSessionIndex()
	{
		if (m_ClientSessionPool.empty())
			return -1;

		int index = m_ClientSessionPoolIndex.front();
		m_ClientSessionPoolIndex.pop_front();
		return index;
	}

	void TcpNetwork::ReleaseSessionIndex(const int index)
	{ 
		m_ClientSessionPoolIndex.push_back(index);
		m_ClientSessionPool[index].Clear();
	}

	int TcpNetwork::CreateSessionPool(const int maxClientCount)
	{
		for (int i = 0; i < maxClientCount; i++) {
			ClientSession session;
			ZeroMemory(&session, sizeof(session));
			session.Index = i;
			session.pRecvBuffer = new char[m_Config.MaxClientRecvBufferSize];
			session.pSendBuffer = new char[m_Config.MaxClientSendBufferSize];

			m_ClientSessionPool.push_back(session);
			m_ClientSessionPoolIndex.push_back(session.Index);
		}	
		return 0;
	}



	void TcpNetwork::SetSockOption(const SOCKET fd)
	{
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		int size1 = m_Config.MaxClientSockOptRecvBufferSize;
		int size2 = m_Config.MaxClientSockOptSendBufferSize;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&size1, sizeof(size1));
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&size2, sizeof(size2));
	}

}