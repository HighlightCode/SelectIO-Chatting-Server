#include "../Common/Packet.h"
#include "TcpNetwork.h"
#include "../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "Lobby.h"
#include "Game.h"
#include "Room.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

ERROR_CODE PacketProcess::RoomEnter(PacketInfo packetInfo)
{
	CHECK_START
		auto reqPkt = (NCommon::PktRoomEnterReq*)packetInfo.pRefData;
	NCommon::PktRoomEnterRes resPkt;

	auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		//CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	if (pUser->IsCurDomainInLobby() == false) {
		//CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	if (pLobby == nullptr) {
		//CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
	}

	Room* pRoom = nullptr;

	// ���� ����� ����� ���� �����
	if (reqPkt->IsCreate)
	{
		pRoom = pLobby->CreateRoom();
		if (pRoom == nullptr) {
			//CHECK_ERROR(ERROR_CODE::ROOM_ENTER_EMPTY_ROOM);
		}

		auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
		if (ret != ERROR_CODE::NONE) {
			//CHECK_ERROR(ret);
		}
	}
	else
	{
		pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
		if (pRoom == nullptr) {
			//CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}
	}


	auto enterRet = pRoom->EnterUser(pUser);
	if (enterRet != ERROR_CODE::NONE) {
		CHECK_ERROR(enterRet);
	}

	// ���� ������ �뿡 ���Դٰ� �����Ѵ�.
	pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

	// �κ� ������ �������� �˸���
	pLobby->NotifyLobbyLeaveUserInfo(pUser);

	// �κ� �� ������ �뺸�Ѵ�.
	pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

	// �뿡 �� ���� ���Դٰ� �˸���
	pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());

	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
	return __result;
}

ERROR_CODE PacketProcess::RoomLeave(PacketInfo packetInfo)
{
	CHECK_START
		NCommon::PktRoomLeaveRes resPkt;

	auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		//CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);
	auto userIndex = pUser->GetIndex();

	if (pUser->IsCurDomainInRoom() == false) {
		//CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	if (pLobby == nullptr) {
		//CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
	}

	auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
	if (pRoom == nullptr) {
		//CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
	}

	auto leaveRet = pRoom->LeaveUser(userIndex);
	if (leaveRet != ERROR_CODE::NONE) {
		//CHECK_ERROR(leaveRet);
	}

	// ���� ������ �κ�� ����
	pUser->EnterLobby(lobbyIndex);

	// �뿡 ������ �������� �뺸
	pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

	// �κ� ���ο� ������ �������� �뺸
	pLobby->NotifyLobbyEnterUserInfo(pUser);

	// �κ� �ٲ� �� ������ �뺸
	pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
	return __result;
}

ERROR_CODE PacketProcess::RoomChat(PacketInfo packetInfo)
{
	CHECK_START
		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
	NCommon::PktRoomChatRes resPkt;

	auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		//CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	if (pUser->IsCurDomainInRoom() == false) {
		//CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	if (pLobby == nullptr) {
		//CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
	}

	auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
	if (pRoom == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
	}

	pRoom->NotifyChat(pUser->GetSessioIndex(), pUser->GetID().c_str(), reqPkt->Msg);

	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
	return __result;
}

ERROR_CODE PacketProcess::RoomMasterGameStart(PacketInfo packetInfo)
{
	CHECK_START
		NCommon::PktRoomMaterGameStartRes resPkt;

	auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		//CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	if (pUser->IsCurDomainInRoom() == false) {
		//CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	if (pLobby == nullptr) {
		//CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_LOBBY_INDEX);
	}

	auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
	if (pRoom == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_ROOM_INDEX);
	}

	// ������ �´��� Ȯ��
	if (pRoom->IsMaster(pUser->GetIndex()) == false) {
		CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_MASTER);
	}

	// ���� �ο��� 2���ΰ�?
	if (pRoom->GetUserCount() != 2) {
		CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_USER_COUNT);
	}

	// ���� ���°� ������ ���ϴ� ������?
	if (pRoom->GetGameObj()->GetState() != GameState::NONE) {
		CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_GAME_STATE);
	}

	// ���� ���� ���� ����
	pRoom->GetGameObj()->SetState(GameState::STARTTING);

	//TODO: �κ��� �������� ���� ���� ���� �뺸

	// ���� �ٸ� �������� ������ ���� ���� ��û�� ������ �˸���
	pRoom->SendToAllUser((short)PACKET_ID::ROOM_MASTER_GAME_START_NTF,
		0,
		nullptr,
		pUser->GetIndex());

	// ��û�ڿ��� �亯�� ������.
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_MASTER_GAME_START_RES, sizeof(resPkt), (char*)&resPkt);
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_MASTER_GAME_START_RES, sizeof(resPkt), (char*)&resPkt);
	return __result;
}

ERROR_CODE PacketProcess::RoomGameStart(PacketInfo packetInfo)
{
	CHECK_START
		NCommon::PktRoomGameStartRes resPkt;

	auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
	auto errorCode = std::get<0>(pUserRet);

	if (errorCode != ERROR_CODE::NONE) {
		//CHECK_ERROR(errorCode);
	}

	auto pUser = std::get<1>(pUserRet);

	if (pUser->IsCurDomainInRoom() == false) {
		//CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_DOMAIN);
	}

	auto lobbyIndex = pUser->GetLobbyIndex();
	auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
	if (pLobby == nullptr) {
		//CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_LOBBY_INDEX);
	}

	auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
	if (pRoom == nullptr) {
		CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_ROOM_INDEX);
	}

	// ���� ���°� ������ ���ϴ� ������?
	if (pRoom->GetGameObj()->GetState() != GameState::STARTTING) {
		CHECK_ERROR(ERROR_CODE::ROOM_MASTER_GAME_START_INVALID_GAME_STATE);
	}

	//TODO: �̹� ���� ���� ��û�� �ߴ°�?

	//TODO: �濡�� ���� ���� ��û�� ���� ����Ʈ�� ���

	// ���� �ٸ� �������� ���� ���� ��û�� ������ �˸���

	// ��û�ڿ��� �亯�� ������.
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_GAME_START_RES, sizeof(resPkt), (char*)&resPkt);


	// ���� ���� �����Ѱ�?
	// �����̸� ���� ���� ���� GameState::ING
	// ���� ���� ��Ŷ ������
	// ���� ���� ���� �κ� �˸���
	// ������ ���� ���� �ð� ����
	return ERROR_CODE::NONE;

CHECK_ERR:
	resPkt.SetError(__result);
	m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_GAME_START_RES, sizeof(resPkt), (char*)&resPkt);
	return __result;
}
