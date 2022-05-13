#pragma once
#include <vector>
#include <unordered_map>
#include "ITcpNetwork.h"

namespace ServerNetLib
{
	class TcpNetwork;
}

namespace ServerNetLib
{
	class ILog;
}

struct LobbyManagerConfig
{
	int MaxLobbyCount;
	int MaxLobbyUserCount;
	int MaxRoomCountByLobby;
	int MaxRoomUserCount;
};

struct LobbySmallInfo
{
	short Num;
	short UserCount;
};

class Lobby;

class LobbyManager
{
	using TcpNet = ServerNetLib::ITcpNetwork;
	using ILog = ServerNetLib::ILog;

public:
	LobbyManager();
	virtual ~LobbyManager();

	void Init(const LobbyManagerConfig config, TcpNet* pNetwork, ILog* pLogger);

	Lobby* GetLobby(short lobbyId);


public:
	void SendLobbyListInfo(const int sessionIndex);



private:
	ILog* m_pRefLogger;
	TcpNet* m_pRefNetwork;

	std::vector<Lobby> m_LobbyList;

};
