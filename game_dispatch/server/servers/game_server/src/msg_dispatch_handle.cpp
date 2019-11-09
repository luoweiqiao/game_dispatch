#include <data_cfg_mgr.h>
#include "stdafx.h"
#include "msg_dispatch_handle.h"
#include "pb/msg_define.pb.h"
#include "center_log.h"
#include "gobal_event_mgr.h"
#include "dispatch_mgr.h"
#include "game_room_mgr.h"

#include "lobby_mgr.h"

using namespace Network;
using namespace svrlib;
using namespace net;

int CHandleDispatchMsg::OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
	    LOG_DEBUG("recv dispatch msg uin:%d cmd:%d", head->uin, head->cmd);
#ifndef HANDLE_DISPATCH_FUNC
#define HANDLE_DISPATCH_FUNC(cmd,handle) \
	case cmd:\
    { \
    handle(pNetObj,pkt_buf,buf_len);\
}break;
#endif

	switch(head->cmd)
	{
	HANDLE_DISPATCH_FUNC(net::D2LS_MSG_REGISTER_REP, handle_msg_register_gamesvr_rep);
	HANDLE_DISPATCH_FUNC(net::D2S_MSG_NOTIFY_GAMESVRS_NEW_LOBBY, handle_msg_notify_new_lobby);
	HANDLE_DISPATCH_FUNC(net::D2S_MSG_RETIRE_GAMESVR, handle_msg_retire_gamesvr);
	HANDLE_DISPATCH_FUNC(net::D2L_MSG_RETIRE_LOBBYSVR, handle_msg_lobbysvr_retired);
	HANDLE_DISPATCH_FUNC(net::D2S_MSG_CHANGE_ROOM_STOCK_CFG, handle_msg_change_room_stock_cfg); // add by har
	}

	return 0;
}

int CHandleDispatchMsg::handle_msg_register_gamesvr_rep(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_register_dispatch_rep msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 result = msg.result(); 
	LOG_DEBUG("register dispatch rep: svrid[%d] result[%d]", pNetObj->GetUID(), result);
	bool rep = (result == 0) ? false : true;
	DispatchMgr::Instance().RegisterRep(pNetObj->GetUID(), rep);

	return 0;
}

int CHandleDispatchMsg::handle_msg_notify_new_lobby(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_notify_gamesvrs_new_lobby msg;
	PARSE_MSG_FROM_ARRAY(msg);

	int newlobbyId = msg.lobby_svrid();
	LOG_DEBUG("Connect to new lobbysvr:%d", newlobbyId);
	CLobbyMgr::Instance().ConnectNewLobby(newlobbyId);
	return 0;
}

int CHandleDispatchMsg::handle_msg_retire_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_retire_gamesvr msg;
	PARSE_MSG_FROM_ARRAY(msg);

	LOG_DEBUG("server retire !!!!!!!!");
	CApplication::Instance().SetStatus(emSERVER_STATE_RETIRE);
	CGameSvrEventMgr::Instance().StartRetire();

	return 0;
}

//有大厅退休
int CHandleDispatchMsg::handle_msg_lobbysvr_retired(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_retire_lobbysvr msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 lobbySvrID = msg.svrid();
	LOG_ERROR("lobby svr:%d has retired", lobbySvrID);

	CLobbyMgr::Instance().NotifyLobbyRetired(lobbySvrID);
	return 0;
}

// 修改房间库存配置 add by har
int CHandleDispatchMsg::handle_msg_change_room_stock_cfg(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len) {
	net::msg_change_room_stock_cfg msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 roomId = msg.roomid();
	stStockCfg st;
	st.stockMin = msg.stock_min();
	st.stockMax = msg.stock_max();
	st.stockConversionRate = msg.stock_conversion_rate();
	st.jackpotMin = msg.jackpot_min();
	st.jackpotMaxRate = msg.jackpot_max_rate();
	st.jackpotRate = msg.jackpot_rate();
	st.jackpotCoefficient = msg.jackpot_coefficient();
	st.jackpotExtractRate = msg.jackpot_extract_rate();
	st.stock = msg.add_stock();

	LOG_DEBUG("change room stock cfg - roomid:%d,stockMin:%d,stockMax:%d,stockConversionRate:%d,jackpotMin:%d,jackpotMaxRate:%d,jackpotRate:%d,jackpotCoefficient:%d,jackpotExtractRate:%d,stock:%lld",
		roomId, st.stockMin, st.stockMax, st.stockConversionRate, st.jackpotMin, st.jackpotMaxRate, st.jackpotRate, st.jackpotCoefficient, st.jackpotExtractRate, st.stock);
	CGameRoom *pRoom = CGameRoomMgr::Instance().GetRoom(roomId);
	if (pRoom == NULL) {
		LOG_ERROR("handle_msg_change_room_stock_cfg  room is not exist - roomid:%dx", roomId);
	} else
	    pRoom->ChangeRoomStockCfg(st);
	return 0;
}