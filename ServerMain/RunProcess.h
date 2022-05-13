#pragma once

#include <memory>

#include "ErrorCode.h"
#include "Packet.h"

using NCommon::ERROR_CODE;

namespace ServerNetLib
{
	struct ServerConfig;
	class ILog;
	class ITcpNetwork;
}

class UserManager;
class LobbyManager;
class PacketProcess;

class RunProcess
{
public:
	RunProcess();
	~RunProcess();

	ERROR_CODE Init();

	void Run();
	void Stop();

private:
	ERROR_CODE LoadConfig();

	void Release();

private:
	bool m_IsRun = false;
	std::unique_ptr<ServerNetLib::ServerConfig> m_pServerConfig;
	std::unique_ptr<ServerNetLib::ILog> m_pLogger;

	std::unique_ptr<ServerNetLib::ITcpNetwork> m_pNetwork;
	std::unique_ptr<PacketProcess> m_pPacketProc;
	std::unique_ptr<UserManager> m_pUserMgr;
	std::unique_ptr<LobbyManager> m_pLobbyMgr;
};

