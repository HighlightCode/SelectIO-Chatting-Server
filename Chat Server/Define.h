#pragma once


/*-------------------
*	SERVER CONFIG
--------------------*/

namespace ServerNetLib 
{
	struct ServerConfig
	{
		unsigned short Port;
		int BackLogCount;

		int MaxClientCount;
		int ExtraClientCount;

		short MaxClientSockOptRecvBufferSize;
		short MaxClientSockOptSendBufferSize;
		short MaxClientRecvBufferSize;
		short MaxClientSendBufferSize;

		bool isLoginCheck;

		int MaxLobbyCount;
		int MaxLobbyUserCount;
		int MaxRoomCountByLobby;
		int MaxRoomUserCount;
	};

	const int MAX_IP_LEN = 32; // maximum length of IP add
	const int MAX_PACKET_BODY_SIZE; // maximum packet size

	struct RecvPacketInfo
	{
		int SessionIndex = 0;
		short PacketId = 0;
		short PacketBodySize = 0;
		char* pRefData = nullptr;
	};
}