#pragma once

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <vector>
#include <deque>
#include <unordered_map>

#include "ITcpNetwork.h"

namespace ServerNetLib 
{
	
	// class ILog
	class TcpNetwork : public ITcpNetwork
	{
	public:
		TcpNetwork();
		virtual ~TcpNetwork();

		// initialize TcpNetwork
		NET_ERROR_CODE Init(const ServerConfig* pConfig, ILog* pLogger) override;

		void Release() override;

		NET_ERROR_CODE SendData(const int SessionIdx, const short packetId, const short size, const char* pMsg) override;

		RecvPacketInfo GetPacketInfo() override;

		int ClientSessionPoolSize() override { return (int)m_ClientSessionPool.size(); }

		void ForcingClose(const int sessionIndex);

		void Run() override;

	protected:
		NET_ERROR_CODE InitServerSocket();
		NET_ERROR_CODE BindListen(short port, int backlogCount);

		int AllocClientSessionIndex();
		void ReleaseSessionIndex(const int index);

		int CreateSessionPool(const int maxClientCount);
		NET_ERROR_CODE NewSession();
		void SetSockOption(const SOCKET fd); // set RCV_BUF, SNF_BUF size 
		void ConnectedSession(const int sessionIndex, const SOCKET fd, const char* pIP);

		void CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex);

		NET_ERROR_CODE RecvSocket(const int sessionIndex);
		NET_ERROR_CODE RecvBufferProcess(const int sessionIndex);
		void AddPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char* pDataPos);

		void RunProcessWrite(const int sessionIndex, const SOCKET fd, fd_set& write_set);
		NetError FlushSendBuff(const int sessionIndex);
		NetError SendSocket(const SOCKET fd, const char* pMsg, const int size);

		bool RunCheckSelectResult(const int result);
		void RunCheckSelectClients(fd_set& read_set, fd_set& write_set);
		bool RunProcessReceive(const int sessionIndex, const SOCKET fd, fd_set& read_set);

	protected:
		ServerConfig m_Config;

		SOCKET m_ServerSockfd;

		fd_set m_Readfds;
		size_t m_ConnectedSessionCount = 0;

		int64_t m_ConnectSeq = 0;

		std::vector<ClientSession> m_ClientSessionPool; // manage ClientSession
		std::deque<int> m_ClientSessionPoolIndex; // manage ClientSession Pool Index

		std::deque<RecvPacketInfo> m_PacketQueue;

		ILog* m_pRefLogger;
	};

}


