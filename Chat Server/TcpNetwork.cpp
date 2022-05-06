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

		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Session Pool Size: %d", __FUNCTION__, sessionPoolSize);

		return NET_ERROR_CODE::NONE;
	}


	void TcpNetwork::Release()
	{
		// clean up the Networks;
		WSACleanup();
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



}