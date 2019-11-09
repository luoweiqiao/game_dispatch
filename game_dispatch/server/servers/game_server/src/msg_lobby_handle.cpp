
#include "data_cfg_mgr.h"
#include "stdafx.h"
#include "msg_lobby_handle.h"
#include "pb/msg_define.pb.h"
#include "center_log.h"
#include "gobal_event_mgr.h"
#include "robot_mgr.h"
#include "game_room.h"
#include "game_room_mgr.h"
#include "fish_info_mgr.h"

using namespace Network;
using namespace svrlib;
using namespace net;

int CHandleLobbyMsg::OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)	
{
    //LOG_DEBUG("收到大厅服务器消息:uin:%d--cmd:%d",head->uin,head->cmd);
#ifndef HANDLE_LOBBY_FUNC
#define HANDLE_LOBBY_FUNC(cmd,handle) \
	case cmd:\
	{ \
		handle(pNetObj,pkt_buf,buf_len,head);\
	}break;
#endif

	switch(head->cmd)
	{
    HANDLE_LOBBY_FUNC(net::L2S_MSG_REGISTER_REP,handle_msg_register_rep);
    HANDLE_LOBBY_FUNC(net::L2S_MSG_NOTIFY_NET_STATE,handle_msg_notify_net_state);
    HANDLE_LOBBY_FUNC(net::L2S_MSG_NOTIFY_CHANGE_PLAYERINFO,handle_msg_notify_change_playerinfo);
    HANDLE_LOBBY_FUNC(net::L2S_MSG_NOTIFY_STOP_SERVICE,handle_msg_notify_stop_server);
    HANDLE_LOBBY_FUNC(net::L2S_MSG_LEAVE_ROBOT,handle_msg_leave_robot);
    HANDLE_LOBBY_FUNC(net::L2S_MSG_FLUSH_CHANGE_ACC_DATA,handle_msg_flush_change_acc_data);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_UPDATE_CHANGE_ACC_DATA, handle_msg_update_change_acc_data);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_CHANGE_ROOM_PARAM, handle_msg_change_room_param);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_CONTORL_PLAYER, handle_msg_contorl_player);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_CONTORL_MULTI_PLAYER, handle_msg_contorl_multi_player);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_STOP_SNATCH_COIN, handle_msg_stop_snatch_coin);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_DICE_CONTROL_REQ, handle_msg_dice_game_control);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_MAJIANG_CONFIG_HAND_CARD, handle_msg_majiang_config_hand_card);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_UPDATE_SERVER_ROOM_ROBOT, handle_msg_update_server_room_robot);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_UPDATE_ROBOT_OBLINE_CFG, handle_msg_update_robot_online_cfg);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_DELETE_ROBOT_OBLINE_CFG, handle_msg_delete_robot_online_cfg);
    HANDLE_LOBBY_FUNC(net::L2S_MSG_ENTER_INTO_SVR,handle_msg_enter_svr);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_ENTER_INTO_EVERY_COLOR_SVR, handle_msg_enter_every_color_svr);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_ENTER_INTO_LEAVE_COLOR_SVR, handle_msg_leave_every_color_svr);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_STOP_CONTROL_PLAYER, handle_msg_stop_control_player);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_SYN_CTRL_USER_CFG, handle_msg_syn_ctrl_user_cfg);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_SYN_LUCKY_CFG, handle_msg_syn_lucky_cfg);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_RESET_LUCKY_CFG, handle_msg_reset_lucky_cfg);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_SYN_FISH_CFG, handle_msg_syn_fish_cfg);

    HANDLE_LOBBY_FUNC(net::C2S_MSG_TAKE_SAFEBOX,handle_msg_take_safebox);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_EXCHANGE_SCORE_REQ,handle_msg_exchange_score);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_SPEAK_BROADCAST_REQ,handle_msg_speak_broadcast);    
    HANDLE_LOBBY_FUNC(net::C2S_MSG_REQ_ROOMS_INFO,handle_msg_req_rooms_info);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_BACK_LOBBY,handle_msg_back_lobby);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_GOTO_SVR,handle_msg_goto_svr);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_ENTER_ROOM,handle_msg_enter_room);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_REQ_TABLE_LIST,handle_msg_req_table_list);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_REQ_CREATE_TABLE,handle_msg_req_create_table);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_RENEW_TABLE_REQ,handle_msg_renew_table);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_LEAVE_TABLE_REQ,handle_msg_leave_table_req);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_ENTER_TABLE_REQ,handle_msg_enter_table);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_TABLE_READY,handle_msg_table_ready);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_TABLE_CHAT,handle_msg_table_chat);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_TABLE_SET_AUTO,handle_msg_table_set_auto);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_FAST_JOIN_ROOM,handle_msg_fast_join_room);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_FAST_JOIN_TABLE,handle_msg_fast_join_table);
	HANDLE_LOBBY_FUNC(net::C2S_MSG_MASTER_JOIN_TABLE, handle_msg_master_join_table);
	HANDLE_LOBBY_FUNC(net::C2S_MSG_COMMON_JOIN_TABLE, handle_msg_common_join_table);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_QUERY_TABLE_LIST_REQ,handle_msg_query_table_list);
    HANDLE_LOBBY_FUNC(net::C2S_MSG_SITDOWN_STANDUP,handle_msg_sitdown_standup);
	HANDLE_LOBBY_FUNC(net::C2S_MSG_ITEMS_USER_REQ, handle_msg_items_user);
	HANDLE_LOBBY_FUNC(net::L2S_MSG_CHANGE_ROOM_STOCK_CFG, handle_msg_change_room_stock_cfg);
	
	default:
        {
            handle_msg_gameing_oper(pNetObj, pkt_buf, buf_len, head);

			//LOG_DEBUG("处理大厅服务器消息:uin:%d--cmd:%d", head->uin, head->cmd);
        }break;
	}
	return 0;
}

int	 CHandleLobbyMsg::handle_msg_register_rep(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
	net::msg_register_svr_rep msg;
	PARSE_MSG_FROM_ARRAY(msg);

	LOG_DEBUG("注册服务器返回:%d",msg.result());
    if(msg.result() == 1)
    {
        CLobbyMgr::Instance().RegisterRep(pNetObj->GetUID(), true);
    }else{
        CLobbyMgr::Instance().RegisterRep(pNetObj->GetUID(), false);
        LOG_ERROR("注册服务器失败:%d",CApplication::Instance().GetServerID());
    }

	return 0;
}
// 通知网络状态
int  CHandleLobbyMsg::handle_msg_notify_net_state(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_notify_net_state  msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid   = msg.uid();
    uint8  state = msg.state();

    LOG_DEBUG("notify net state - uid:%d,state:%d",uid,state);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer != NULL){
        pGamePlayer->SetNetState(state);
    }
    return 0;
}
// 通知修改玩家信息
int  CHandleLobbyMsg::handle_msg_notify_change_playerinfo(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_notify_change_playerinfo msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("通知修改玩家信息:%d,%s",msg.safebox(),msg.name().c_str());
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer != NULL){
        pGamePlayer->SetSafeBoxState(msg.safebox(),false);
        pGamePlayer->SetPlayerName(msg.name());
        pGamePlayer->FlushAccValue2Table();
    }
    return 0;
}
// 通知服务器维护
int  CHandleLobbyMsg::handle_msg_notify_stop_server(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_notify_stop_service msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("游戏服务器维护");
    CApplication::Instance().SetStatus(emSERVER_STATE_REPAIR);

    return 0;
}
// 机器人离开
int  CHandleLobbyMsg::handle_msg_leave_robot(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_leave_robot msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("机器人下线:%d",msg.uid());
    CGameRobot* pRobot = CRobotMgr::Instance().GetRobot(msg.uid());
    if(pRobot != NULL){
        pRobot->SetLeaveTime(getSysTime());
    }
    return 0;
}
int  CHandleLobbyMsg::handle_msg_flush_change_acc_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_flush_change_account_data msg;
    PARSE_MSG_FROM_ARRAY(msg);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;
    pGamePlayer->ChangeAccountValue(emACC_VALUE_DIAMOND,msg.diamond());
    pGamePlayer->ChangeAccountValue(emACC_VALUE_COIN,msg.coin());
    pGamePlayer->ChangeAccountValue(emACC_VALUE_SCORE,msg.score());
    pGamePlayer->ChangeAccountValue(emACC_VALUE_INGOT,msg.ingot());
    pGamePlayer->ChangeAccountValue(emACC_VALUE_CVALUE,msg.cvalue());
    pGamePlayer->ChangeAccountValue(emACC_VALUE_SAFECOIN,msg.safe_coin());
    
    return 0;
}

int  CHandleLobbyMsg::handle_msg_update_change_acc_data(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_update_change_account_data msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CGamePlayer* pGamePlayer = GetGamePlayer(head);
	if (pGamePlayer == NULL)
		return 0;

	pGamePlayer->ChangeAccountValue(emACC_VALUE_DIAMOND, msg.diamond());
	pGamePlayer->ChangeAccountValue(emACC_VALUE_COIN, msg.coin());
	pGamePlayer->ChangeAccountValue(emACC_VALUE_SCORE, msg.score());
	pGamePlayer->ChangeAccountValue(emACC_VALUE_INGOT, msg.ingot());
	pGamePlayer->ChangeAccountValue(emACC_VALUE_CVALUE, msg.cvalue());
	pGamePlayer->ChangeAccountValue(emACC_VALUE_SAFECOIN, msg.safe_coin());

	CGameTable * pGameTable = pGamePlayer->GetTable();
	CGameRoom  * pGameRoom = pGamePlayer->GetRoom();

	LOG_DEBUG("update score 1 - uid:%d,cur_coin:%lld,coin:%lld,ptype:%d,stype:%d,pGameTable:%p,pGameRoom:%p", pGamePlayer->GetUID(), pGamePlayer->GetAccountValue(emACC_VALUE_COIN), msg.coin(), msg.oper_type(), msg.sub_type(), pGameTable, pGameRoom);

	if (pGameTable != NULL && pGameRoom != NULL)
	{
		pGameTable->UpdataScoreInGame(pGamePlayer->GetUID(), pGameRoom->GetGameType(), msg.coin());
		pGameTable->UpdateMaxJettonScore(pGamePlayer, msg.coin());
		pGameTable->BuyEnterScore(pGamePlayer, msg.coin());

		if (pGamePlayer->GetAccountValue(emACC_VALUE_COIN) > pGameRoom->GetEnterMin())
		{
			pGamePlayer->SetBankruptRecord(true);
		}

		LOG_DEBUG("update score 2 - uid:%d,coin:%lld,ptype:%d,stype:%d,roomid:%d,tableid:%d", pGamePlayer->GetUID(), msg.coin(), msg.oper_type(), msg.sub_type(), pGameRoom->GetRoomID(), pGameTable->GetTableID());

		//if (pGameRoom->GetGameType() == net::GAME_CATE_LAND || pGameRoom->GetGameType() == net::GAME_CATE_SHOWHAND || pGameRoom->GetGameType() == net::GAME_CATE_BULLFIGHT || pGameRoom->GetGameType() == net::GAME_CATE_TEXAS || pGameRoom->GetGameType() == net::GAME_CATE_ZAJINHUA || pGameRoom->GetGameType() == net::GAME_CATE_NIUNIU || pGameRoom->GetGameType() == net::GAME_CATE_BACCARAT || pGameRoom->GetGameType() == net::GAME_CATE_PAIJIU)
		//{

			//if (msg.oper_type() == emACCTRAN_OPER_TYPE_EVERY_COLOR_UPDATE_SCORE || msg.oper_type() == emACCTRAN_OPER_TYPE_BUY)
			//{
			//	//LOG_DEBUG("update score 2 - uid:%d,coin:%lld,ptype:%d,stype:%d,roomid:%d,tableid:%d", pGamePlayer->GetUID(), msg.coin(), msg.oper_type(), msg.sub_type(), pGameRoom->GetRoomID(), pGameTable->GetTableID());

			//	pGameTable->UpdataScoreInGame(pGamePlayer->GetUID(), pGameRoom->GetGameType(), msg.coin());
			//	pGameTable->UpdateMaxJettonScore(pGamePlayer, msg.coin());
			//}
		//}
		//if (pGameRoom->GetGameType() == net::GAME_CATE_EVERYCOLOR)
		//{
		//	pGameTable->SendPalyerLookerListToClient(pGamePlayer);
		//}
	}
	return 0;
}


// 改变房间param
int  CHandleLobbyMsg::handle_msg_change_room_param(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_change_room_param msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 gametype = msg.gametype();
	uint32 roomid = msg.roomid();
	string param = msg.param();
	LOG_DEBUG("change room  param - gametype:%d,roomid:%d,param:%s", gametype, roomid, param.c_str());
	
	CGameRoom* pRoom = CGameRoomMgr::Instance().GetRoom(roomid);
	if (pRoom == NULL) {
		LOG_DEBUG("room is not exist - gametype:%d,roomid:%d,param:%s", gametype, roomid, param.c_str());
		return 0;
	}
	int bfalg = pRoom->CheckRoommParambyGameType(gametype, param);
	if (bfalg != 0) {
		LOG_DEBUG("room param check error - gametype:%d,roomid:%d,bfalg:%d,param:%s", gametype, roomid, bfalg, param.c_str());
		return 0;
	}
	pRoom->SetRoomCfgParam(param);
	pRoom->ReAnalysisRoomParam();
	return 0;
}

// 控制玩家
int  CHandleLobbyMsg::handle_msg_contorl_player(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_contorl_player msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 gametype = msg.gametype();
	uint32 roomid = msg.roomid();
	uint32 uid = msg.uid();
	uint32 operatetype = msg.operatetype();
	uint32 gamecount = msg.gamecount();
	LOG_DEBUG("control player data - gametype:%d,roomid:%d,uid:%d,operatetype:%d,gamecount:%d", gametype, roomid, uid, operatetype, gamecount);

	CGameRoom* pRoom = CGameRoomMgr::Instance().GetRoom(roomid);
	if (pRoom == NULL) {
		LOG_DEBUG("room is not exist - gametype:%d,roomid:%d,uid:%d,operatetype:%d,gamecount:%d", gametype, roomid, uid, operatetype, gamecount);
		return 0;
	}
	if (gametype < net::GAME_CATE_MAX_TYPE)
	{
		pRoom->UpdateControlPlayer(uid, gamecount, operatetype);
	}
	
	return 0;
}

// 控制玩家
int  CHandleLobbyMsg::handle_msg_contorl_multi_player(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_contorl_multi_player msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 gametype = msg.gametype();
	uint32 roomid = msg.roomid();
	uint32 uid = msg.uid();
	uint32 operatetype = msg.operatetype();
	uint32 gamecount = msg.gamecount();
	uint64 gametime = msg.gametime();
	int64 totalscore = msg.totalscore();

	LOG_DEBUG("control player data - gametype:%d,roomid:%d,uid:%d,operatetype:%d,gamecount:%d,gametime:%lld,totalscore:%lld", gametype, roomid, uid, operatetype, gamecount, gametime, totalscore);

	CGameRoom* pRoom = CGameRoomMgr::Instance().GetRoom(roomid);
	if (pRoom == NULL) {
		LOG_DEBUG("room is not exist - gametype:%d,roomid:%d,uid:%d,operatetype:%d,gamecount:%d,gametime:%lld,totalscore:%lld", gametype, roomid, uid, operatetype, gamecount, gametime, totalscore);
		return 0;
	}
	if (gametype < net::GAME_CATE_MAX_TYPE)
	{
		pRoom->UpdateControlMultiPlayer(uid, gamecount, gametime, totalscore, operatetype);
	}

	return 0;
}

// 停止夺宝
int  CHandleLobbyMsg::handle_msg_stop_snatch_coin(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_stop_snatch_coin msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 gametype = msg.gametype();
	uint32 roomid = msg.roomid();
	uint32 stop = msg.stop();
	LOG_DEBUG("change room  param - gametype:%d,roomid:%d,stop:%d", gametype, roomid, stop);

	if (gametype != net::GAME_CATE_EVERYCOLOR)
	{
		return 0;
	}
	CGameRoom* pRoom = CGameRoomMgr::Instance().GetRoom(roomid);
	if (pRoom == NULL) {
		LOG_DEBUG("room is not exist - gametype:%d,roomid:%d,stop:%d", gametype, roomid, stop);
		return 0;
	}

	pRoom->SetSnatchCoinState(stop);
	return 0;
}

int  CHandleLobbyMsg::handle_msg_dice_game_control(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_dice_control_req msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 gametype = msg.gametype();
	uint32 roomid = msg.roomid();
	uint32 udice[3] = { 0 };

	net::dice_control_req dice_control_req = msg.dice();

	for (int i = 0; i < dice_control_req.table_cards_size(); i++)
	{
		udice[i] = dice_control_req.table_cards(i);
	}

	if (gametype != net::GAME_CATE_DICE)
	{
		return 0;
	}
	CGameRoom* pRoom = CGameRoomMgr::Instance().GetRoom(roomid);
	if (pRoom == NULL)
	{
		return 0;
	}

	pRoom->DiceGameControlCard(gametype, roomid, udice);
	return 0;
}

int  CHandleLobbyMsg::handle_msg_majiang_config_hand_card(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_majiang_config_hand_card msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 gametype = msg.gametype();
	uint32 roomid = msg.roomid();
	string strHandCard = msg.hand_card();

	if (gametype != net::GAME_CATE_TWO_PEOPLE_MAJIANG)
	{
		return 0;
	}
	CGameRoom* pRoom = CGameRoomMgr::Instance().GetRoom(roomid);
	if (pRoom == NULL)
	{
		return 0;
	}

	pRoom->MajiangConfigHandCard(gametype, roomid, strHandCard);
	return 0;
}

int  CHandleLobbyMsg::handle_msg_update_server_room_robot(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_update_server_room_robot msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 gametype = msg.gametype();
	uint32 roomid = msg.roomid();
	uint32 robot = msg.robot();

	CGameRoom* pRoom = CGameRoomMgr::Instance().GetRoom(roomid);

	LOG_DEBUG("gametype:%d,robot:%d, roomid:%d, pRoom:%p", gametype, robot, roomid, pRoom);


	if (pRoom == NULL)
	{
		return 0;
	}

	pRoom->SetRobotCfg(robot);
	return 0;
}

int  CHandleLobbyMsg::handle_msg_update_robot_online_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_update_robot_online_cfg msg;
	PARSE_MSG_FROM_ARRAY(msg);
	stRobotOnlineCfg refCfg;

	refCfg.batchID = msg.batchid();
	refCfg.gameType = msg.gametype();
	refCfg.roomID = msg.roomid();
	refCfg.leveltype = msg.leveltype();
	refCfg.enterTime = msg.entertime();
	refCfg.leaveTime = msg.leavetime();

	for (uint8 lv = 0; lv < ROBOT_MAX_LEVEL && msg.onlines_size() == ROBOT_MAX_LEVEL; ++lv)
	{
		refCfg.onlines[lv] = msg.onlines(lv);
	}


	CDataCfgMgr::Instance().UpdateRobotOnlineCfg(refCfg);

	return 0;
}

int  CHandleLobbyMsg::handle_msg_delete_robot_online_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_delete_robot_online_cfg msg;
	PARSE_MSG_FROM_ARRAY(msg);
	stRobotOnlineCfg refCfg;

	refCfg.batchID = msg.batchid();
	refCfg.gameType = msg.gametype();

	CDataCfgMgr::Instance().DeleteRobotOnlineCfg(refCfg.gameType, refCfg.batchID);

	return 0;
}

// 进入游戏服务器
int  CHandleLobbyMsg::handle_msg_enter_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head) 
{
    net::msg_enter_into_game_svr msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid = head->uin;
    LOG_DEBUG("enter game server - uid:%d,player_type:%d",uid, msg.player_type());

    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer != NULL)
    {
        pGamePlayer->ReLogin();
        pGamePlayer->SetBelongLobbyID(pNetObj->GetUID());
        pGamePlayer->SetSession(pNetObj);
		pGamePlayer->SetCtrlFlag(msg.ctrl_flag());
    }else{
        if(msg.player_type() == PLAYER_TYPE_ONLINE) {
            pGamePlayer = new CGamePlayer(PLAYER_TYPE_ONLINE);
            pGamePlayer->SetPlayerGameData(msg);
            pGamePlayer->OnLogin();
            pGamePlayer->SetSession(pNetObj);
            pGamePlayer->SetBelongLobbyID(pNetObj->GetUID());
			pGamePlayer->SetCtrlFlag(msg.ctrl_flag());
            CPlayerMgr::Instance().AddPlayer(pGamePlayer);

        }else if(msg.player_type() == PLAYER_TYPE_ROBOT){
            CGameRobot* pRobot = new CGameRobot();
            pRobot->SetPlayerGameData(msg);
            pRobot->OnLogin();
            pRobot->SetSession(pNetObj);
            pRobot->SetBelongLobbyID(pNetObj->GetUID());
			pRobot->SetCtrlFlag(msg.ctrl_flag());
            CRobotMgr::Instance().AddRobot(pRobot);

            return 0;
        }
        else{
            LOG_ERROR("player type error - uid:%d,player_type:%d", uid,msg.player_type());
            return 0;
        }
    }

	net::msg_enter_gamesvr_rep msgrep;
    msgrep.set_result(1);
    msgrep.set_svrid(CDataCfgMgr::Instance().GetCurSvrCfg().svrid);
	msgrep.set_ctrl_flag(msg.ctrl_flag());

    pGamePlayer->SendMsgToClient(&msgrep,net::S2C_MSG_ENTER_SVR_REP);

    CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(pGamePlayer->GetUID(),CApplication::Instance().GetServerID(),0,pGamePlayer->GetPlayerType());
    
    CGameSvrEventMgr::Instance().ReportInfo2Lobby();
    return 0;
}

// 进入时时彩游戏服务器
int  CHandleLobbyMsg::handle_msg_enter_every_color_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_enter_into_game_svr msg;
	PARSE_MSG_FROM_ARRAY(msg);
	uint32 uid = head->uin;
	LOG_DEBUG("enter every_color game server - uid:%d", uid);

	CGamePlayer* pGamePlayer = GetGamePlayer(head);
	if (pGamePlayer != NULL)
	{
		pGamePlayer->ReLogin();
		pGamePlayer->SetSession(pNetObj);
		pGamePlayer->SetBelongLobbyID(pNetObj->GetUID());
	}
	else {
		if (msg.player_type() == PLAYER_TYPE_ONLINE) {
			pGamePlayer = new CGamePlayer(PLAYER_TYPE_ONLINE);
			pGamePlayer->SetPlayerGameData(msg);
			pGamePlayer->OnLogin();
			pGamePlayer->SetSession(pNetObj);
			pGamePlayer->SetBelongLobbyID(pNetObj->GetUID());
			CPlayerMgr::Instance().AddPlayer(pGamePlayer);

		}
		else if (msg.player_type() == PLAYER_TYPE_ROBOT) {
			CGameRobot* pRobot = new CGameRobot();
			pRobot->SetPlayerGameData(msg);
			pRobot->OnLogin();
			pRobot->SetSession(pNetObj);
			pRobot->SetBelongLobbyID(pNetObj->GetUID());
			CRobotMgr::Instance().AddRobot(pRobot);

			return 0;
		}
		else {
			LOG_ERROR("类型错误:%d", msg.player_type());
			return 0;
		}
	}

	net::msg_enter_every_color_gamesvr_rep msgrep;
	msgrep.set_result(1);
	msgrep.set_svrid(CDataCfgMgr::Instance().GetCurSvrCfg().svrid);

	pGamePlayer->SendMsgToClient(&msgrep, net::S2C_MSG_EVERY_COLOR_GAME_ENTER_REP);

	//CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(pGamePlayer->GetUID(), CApplication::Instance().GetServerID(), 0, pGamePlayer->GetPlayerType());

	CGameSvrEventMgr::Instance().ReportInfo2Lobby();

	//进入房间

	vector<CGameRoom*> vEveryColorRooms;
	CGameRoomMgr::Instance().GetRoomList(0, 0, vEveryColorRooms);
	LOG_DEBUG("get every color room - uid:%d,room_size:%d,svrID:%d", uid, vEveryColorRooms.size(), CDataCfgMgr::Instance().GetCurSvrCfg().svrid);
	if (vEveryColorRooms.size() != 1)
	{
		return 0;
	}
	CGameRoom* pEveryColorRooms = vEveryColorRooms[0];
	CGameRoom* pOldRoom = pGamePlayer->GetRoom();
	bool bEnterRoom = false;
	bool bEnterTable = false;

	if (pOldRoom != NULL && pOldRoom->GetRoomID() == pEveryColorRooms->GetRoomID())
	{
		bEnterRoom = pOldRoom->EnterEveryColorRoom(pGamePlayer);
		if (bEnterRoom)
		{
			bEnterTable = pOldRoom->FastJoinEveryColorTable(pGamePlayer);
		}
		LOG_DEBUG("is in room - uid:%d,roomID:%d,bEnterRoom:%d,bEnterTable:%d", uid, pEveryColorRooms->GetRoomID(), bEnterRoom, bEnterTable);

		return 0;
	}

	bEnterRoom = pEveryColorRooms->EnterEveryColorRoom(pGamePlayer);
	if (bEnterRoom)
	{
		bEnterTable = pEveryColorRooms->FastJoinEveryColorTable(pGamePlayer);
	}

	LOG_DEBUG("enter every color room - uid:%d,roomID:%d,bEnterRoom:%d,bEnterTable:%d", uid, pEveryColorRooms->GetRoomID(), bEnterRoom, bEnterTable);

	return 0;
}

// 离开时时彩游戏服务器
int  CHandleLobbyMsg::handle_msg_leave_every_color_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_leave_every_color_gamesvr_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	uint32 uid = head->uin;
	LOG_DEBUG("leave every color server - svrid:%d,uid:%d", msg.svrid(), uid);
	CGamePlayer* pGamePlayer = GetGamePlayer(head);
	uint8 bRet = net::RESULT_CODE_FAIL;
	if (pGamePlayer != NULL)
	{
		pGamePlayer->OnLoginOut(msg.svrid());
		CPlayerMgr::Instance().RemovePlayer(pGamePlayer);
		SAFE_DELETE(pGamePlayer);
		bRet = net::RESULT_CODE_SUCCESS;
	}
	net::msg_leave_every_color_gamesvr_rep rep;
	rep.set_result(bRet);
	rep.set_svrid(msg.svrid());
	CLobbyMgr::Instance().SendMsg2Client(&rep, net::S2C_MSG_EVERY_COLOR_GAME_LEAVE_REP, uid);

	return 0;
}

int  CHandleLobbyMsg::handle_msg_stop_control_player(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_stop_conctrl_player msg;
	PARSE_MSG_FROM_ARRAY(msg);
	uint32 uid = msg.uid();
	LOG_DEBUG("stop control player - uid:%d", uid);
	
	CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);

	if (pPlayer != NULL)
	{
		net::msg_control_player_back_lobby msgrep;
		msgrep.set_uid(uid);
		pPlayer->SendMsgToClient(&msgrep, net::S2C_MSG_NOTICE_CONTROL_PLAYER_BACK_LOBBY);
	}
	else
	{
		LOG_ERROR("get player info error - uid:%d", uid);
	}	
	return 0;
}

int  CHandleLobbyMsg::handle_msg_syn_ctrl_user_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_syn_ctrl_user_cfg msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 opertype = msg.opertype();

	tagUserControlCfg info;
	info.suid = msg.tag_suid();
	info.sdeviceid = msg.tag_sdeviceid();
	info.tuid = msg.tag_tuid();
	for (int i = 0; i < msg.tag_cgid_size(); i++)
	{
		info.cgid.insert(msg.tag_cgid(i));
	}
	info.skey = msg.tag_skey();

	if (opertype == 3)
	{
		set<uint32> set_suid;
		for (int i = 0; i < msg.vec_suid_size(); i++)
		{
			set_suid.insert(msg.vec_suid(i));
		}
		for (uint32 suid_tmp : set_suid)
		{
			CDataCfgMgr::Instance().UpdateUserControlInfo(suid_tmp, opertype, info);
		}
	}
	else
	{
		uint32 suid = msg.vec_suid(0);
		CDataCfgMgr::Instance().UpdateUserControlInfo(suid, opertype, info);
	}

	return 0;
}


// 提取保险箱
int  CHandleLobbyMsg::handle_msg_take_safebox(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_take_safebox_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("游戏服提取保险箱:%d--%lld",head->uin,msg.take_coin());
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    net::msg_take_safebox_rep rep;
    rep.set_take_coin(msg.take_coin());
    rep.set_result(0);
    if(pGamePlayer == NULL || pGamePlayer->IsInGamePlaying() || pGamePlayer->GetSafeBoxState() != 1){
        rep.set_result(net::RESULT_CODE_GAMEING);
        pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_TAKE_SAFEBOX_REP);
        return 0;
    }
    if(pGamePlayer->TakeSafeBox(msg.take_coin())){
        rep.set_result(net::RESULT_CODE_SUCCESS);
    }else{
        rep.set_result(net::RESULT_CODE_CION_ERROR);
    }
    pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_TAKE_SAFEBOX_REP);

    pGamePlayer->FlushAccValue2Table();

    return 0;
}
// 兑换积分
int  CHandleLobbyMsg::handle_msg_exchange_score(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    LOG_DEBUG("兑换积分");
    net::msg_exchange_score_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;
    net::msg_exchange_score_rep rep;
    rep.set_exchange_id(msg.exchange_id());
    rep.set_exchange_type(msg.exchange_type());

    if(pGamePlayer->IsInGamePlaying())
    {
        rep.set_result(net::RESULT_CODE_GAMEING);
        pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_EXCHANGE_SCORE_REP);
        return 0;
    }
    uint8 bRet = pGamePlayer->ExchangeScore(msg.exchange_id(),msg.exchange_type());
    rep.set_result(bRet);
    pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_EXCHANGE_SCORE_REP);

    pGamePlayer->FlushAccValue2Table();

    return 0;
}
// 发送喇叭
int  CHandleLobbyMsg::handle_msg_speak_broadcast(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    LOG_DEBUG("发送喇叭");
    net::msg_speak_broadcast_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    CGamePlayer* pPlayer = GetGamePlayer(head);
    if(pPlayer == NULL)
        return 0;
    net::msg_speak_oper_rep operrep; 
    LOG_DEBUG("发送喇叭:%d--%s",pPlayer->GetUID(),msg.msg().c_str());
    if(pPlayer->IsInGamePlaying()){
        LOG_DEBUG("游戏中不能发送喇叭");
        operrep.set_result(net::RESULT_CODE_GAMEING);
        pPlayer->SendMsgToClient(&operrep,net::S2C_MSG_SPEAK_OPER_REP);
        return 0;
    }
  
    int64 cost = CDataCfgMgr::Instance().GetSpeakCost();
    if(pPlayer->GetAccountValue(emACC_VALUE_COIN) < cost){
        LOG_DEBUG("财富币不够:%lld",cost);
        operrep.set_result(net::RESULT_CODE_CION_ERROR);
        pPlayer->SendMsgToClient(&operrep,net::S2C_MSG_SPEAK_OPER_REP);
        return 0;
    }
    pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_SPEAK,0,0,-cost,0,0,0,0);
    operrep.set_result(net::RESULT_CODE_SUCCESS);
    pPlayer->SendMsgToClient(&operrep,net::S2C_MSG_SPEAK_OPER_REP); 
    
    net::msg_speak_broadcast_rep repmsg;
    repmsg.set_send_id(pPlayer->GetUID());
    repmsg.set_send_name(pPlayer->GetPlayerName());
    repmsg.set_msg(msg.msg());
                
    //CLobbyMgr::Instance().SendMsg2Client(&repmsg,net::S2C_MSG_SPEAK_BROADCAST_REP,0);    
    stLobbyServer * pSvr = CLobbyMgr::Instance().GetLobbySvrBypNetObj(pNetObj);
    if(!pSvr)
    {
        LOG_ERROR("cont find server by netobj");
        return 0;
    }
    CLobbyMgr::Instance().SendMsg2Client(&repmsg, net::S2C_MSG_SPEAK_BROADCAST_REP, pSvr->svrID);
    return 0;
}
// 请求房间列表信息
int  CHandleLobbyMsg::handle_msg_req_rooms_info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_rooms_info_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid = head->uin;
    LOG_DEBUG("请求房间列表信息:%d",uid);
    CGameRoomMgr::Instance().SendRoomList2Client(uid);

    return 0;
}
// 返回大厅
int  CHandleLobbyMsg::handle_msg_back_lobby(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_back_lobby_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid = head->uin;
    LOG_DEBUG("请求返回大厅 - uid:%d",uid);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer != NULL)
    {
        if(!pGamePlayer->CanBackLobby())
        {
            net::msg_back_lobby_rep rep;
            rep.set_result(0);
            pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_BACK_LOBBY_REP);
            LOG_DEBUG("游戏状态中不能返回大厅");
            return 0;
        }else{
			LOG_ERROR("OnLoginOut - uid:%d", uid);
            pGamePlayer->OnLoginOut();
            CPlayerMgr::Instance().RemovePlayer(pGamePlayer);
            SAFE_DELETE(pGamePlayer);
        }
        return 0;
    }
    LOG_DEBUG("发送返回大厅消息 - uid:%d",uid);
    net::msg_back_lobby_rep rep;
    rep.set_result(1);
    CLobbyMgr::Instance().SendMsg2Client(&rep,net::S2C_MSG_BACK_LOBBY_REP,uid);
    return 0;
}
// 跳转游戏服务器
int  CHandleLobbyMsg::handle_msg_goto_svr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_goto_gamesvr_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid = head->uin;
    LOG_DEBUG("请求跳转游戏服务器 - uid:%d,svrid:%d",uid,msg.svrid());
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    uint8 bRet = net::RESULT_CODE_FAIL;
    if(pGamePlayer != NULL)
    {
        if(!pGamePlayer->CanBackLobby()){
            LOG_DEBUG("游戏状态中不能跳转游戏服务器 - uid:%d,svrid:%d", uid, msg.svrid());
        }else{
			LOG_DEBUG("请求跳转游戏服务器成功 - uid:%d,svrid:%d", uid, msg.svrid());
            pGamePlayer->OnLoginOut(msg.svrid());
            CPlayerMgr::Instance().RemovePlayer(pGamePlayer);
            SAFE_DELETE(pGamePlayer);
            bRet = net::RESULT_CODE_SUCCESS;
        }
    }
    net::msg_goto_gamesvr_rep rep;
    rep.set_result(bRet);
    rep.set_svrid(msg.svrid());
    CLobbyMgr::Instance().SendMsg2Client(&rep,net::S2C_MSG_GOTO_SVR_REP,uid);
    return 0;
}
// 进入房间
int  CHandleLobbyMsg::handle_msg_enter_room(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_enter_room_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid     = head->uin;
    uint16 roomID  = msg.room_id();
    LOG_DEBUG("请求进入房间:%d-%d",uid,roomID);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL){
        LOG_DEBUG("请求进入房间的玩家不存在");
        return 0;
    }
    net::msg_enter_room_rep rep;
    CGameRoom* pOldRoom = pGamePlayer->GetRoom();
    if(pOldRoom != NULL && pOldRoom->GetRoomID() == roomID)
    {
        LOG_DEBUG("已经在房间中了:%d",roomID);
        pOldRoom->EnterRoom(pGamePlayer);
        return 0;
    }
    if(!pGamePlayer->CanBackLobby()) {
        LOG_DEBUG("已经在座位上了不能换房:%d",uid);
        rep.set_result(0);
        pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_ENTER_ROOM_REP);
        return 0;
    }

    if(pOldRoom != NULL){
        pOldRoom->LeaveRoom(pGamePlayer);
    }
    CGameRoom* pNewRoom = CGameRoomMgr::Instance().GetRoom(roomID);
    if(pNewRoom == NULL || !pNewRoom->CanEnterRoom(pGamePlayer)) {
        LOG_DEBUG("房间不存在或者不能进入:uid:%d--room:%d",uid,roomID);
        rep.set_result(0);
        pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_ENTER_ROOM_REP);
        return 0;
    }
    pNewRoom->EnterRoom(pGamePlayer);

    return 0;
}
// 请求桌子列表
int  CHandleLobbyMsg::handle_msg_req_table_list(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_table_list_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid = head->uin;
    LOG_DEBUG("请求桌子列表:%d",uid);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
       return 0;

    if(pGamePlayer->GetRoom() == NULL)
    {
        LOG_DEBUG("玩家没有进入房间:%d",uid);
        return 0;
    }
    uint8   seachType = msg.seach_type();
    uint32  tableID   = msg.table_id();

    pGamePlayer->GetRoom()->SendTableListToPlayer(pGamePlayer,tableID);
    return 0;
}
// 创建桌子
int  CHandleLobbyMsg::handle_msg_req_create_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_create_table_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid = head->uin;
    LOG_DEBUG("请求创建桌子:%d",uid);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;
    if (CApplication::Instance().GetStatus() == emSERVER_STATE_RETIRE)
    {
        LOG_ERROR("retire server cant create table !!!");
        return 0;
    }
    CGameRoom* pRoom = pGamePlayer->GetRoom();
    if(pRoom == NULL || pRoom->GetRoomType() != emROOM_TYPE_PRIVATE)
    {
        LOG_DEBUG("不在房间中或者不能创建房间:%d-->%d",uid,pGamePlayer->GetRoomID());
        return 0;
    }
    stTableConf conf;
    conf.tableName = msg.table_name();
    conf.baseScore = msg.base_score();
    conf.consume   = net::ROOM_CONSUME_TYPE_COIN;
    conf.deal      = msg.deal();
    conf.enterMin  = CDataCfgMgr::Instance().CalcEnterMin(conf.baseScore,conf.deal);
    conf.hostID    = pGamePlayer->GetUID();
    conf.hostName  = pGamePlayer->GetPlayerName();
    conf.passwd    = msg.passwd();
    conf.dueTime   = getSysTime() + msg.open_days() * SECONDS_IN_ONE_DAY;
    conf.feeType   = msg.fee_type();
    conf.feeValue  = msg.fee_value();
    conf.isShow    = msg.is_show();

    uint8 bRet = net::RESULT_CODE_SUCCESS;
    // 检测配置是否正常，扣费
    if(CDataCfgMgr::Instance().CheckBaseScore(conf.deal,conf.baseScore) == false){
        LOG_DEBUG("底分配置错误");
        bRet = net::RESULT_CODE_ERROR_PARAM;
    }
    if(CDataCfgMgr::Instance().CheckFee(conf.feeType,conf.feeValue) == false){
        LOG_ERROR("台费配置错误");
        bRet = net::RESULT_CODE_ERROR_PARAM;
    }
    int64 coin = CDataCfgMgr::Instance().GetLandPRoomRice(msg.open_days());
    if(coin > pGamePlayer->GetAccountValue(emACC_VALUE_COIN)){
        LOG_ERROR("开房费用不足");
        bRet = net::RESULT_CODE_CION_ERROR;
    }
    net::msg_create_table_rep rep;
    rep.set_result(bRet);
    rep.set_table_id(0);
    if(bRet == net::RESULT_CODE_SUCCESS){
        CGameTable* pTable = pRoom->CreatePlayerTable(pGamePlayer,conf);
        if(pTable == NULL){
            rep.set_result(net::RESULT_CODE_FAIL);
            LOG_DEBUG("创建桌子失败");
        }else{
            rep.set_table_id(pTable->GetTableID());
            pGamePlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_PROOM,pTable->GetTableID(),0,-coin,0,0,0,0);
        }
    }
    pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_CREATE_TABLE_REP);

    return 0;
}
// 续费桌子
int  CHandleLobbyMsg::handle_msg_renew_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_renew_table_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("请求续费桌子");
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;
    CGameRoom* pRoom = pGamePlayer->GetRoom();
    if(pRoom == NULL || pRoom->GetRoomType() != emROOM_TYPE_PRIVATE)
    {
        LOG_DEBUG("不在房间中或者不能创建房间:%d-->%d",pGamePlayer->GetUID(),pGamePlayer->GetRoomID());
        return 0;
    }
    uint8 bRet = net::RESULT_CODE_FAIL;
    int64 coin = CDataCfgMgr::Instance().GetLandPRoomRice(msg.renew_days());
    if(coin > pGamePlayer->GetAccountValue(emACC_VALUE_COIN)){
        LOG_ERROR("开房费用不足");
        bRet = net::RESULT_CODE_CION_ERROR;
    }
    CGameTable* pTable = pRoom->GetTable(msg.table_id());
    if(pTable == NULL)
    {
        LOG_ERROR("续费的桌子不存在:%d",msg.table_id());
        bRet = net::RESULT_CODE_NOT_TABLE;
    }else{
        if(pTable->GetHostID() != pGamePlayer->GetUID())
        {
            LOG_ERROR("不属于你的桌子:%d--%d",pTable->GetTableID(),pGamePlayer->GetUID());
            bRet = net::RESULT_CODE_NOT_OWER;
        }else{
            pGamePlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_PROOM,pTable->GetTableID(),0,-coin,0,0,0,0);
            pTable->RenewPrivateTable(msg.renew_days());
            pTable->UpdatePrivateTableDuetime();
            bRet = net::RESULT_CODE_SUCCESS;
        }
    }

    net::msg_renew_table_rep rep;
    rep.set_renew_days(msg.renew_days());
    rep.set_table_id(msg.table_id());
    rep.set_result(bRet);
    pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_RENEW_TABLE_REP);

    return 0;
}
// 离开桌子
int  CHandleLobbyMsg::handle_msg_leave_table_req(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_leave_table_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid = head->uin;
    LOG_DEBUG("请求离开桌子:%d",uid);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;
    CGameTable* pTable    = pGamePlayer->GetTable();
    if(pTable == NULL){
        LOG_DEBUG("不在桌子上:%d",uid);
        return 0;
    }
    CGameRoom* pRoom = pGamePlayer->GetRoom();
    net::msg_leave_table_rep rep;
    if(pTable->CanLeaveTable(pGamePlayer))
    {
        pTable->LeaveTable(pGamePlayer);
        rep.set_result(1);
        if(pRoom != NULL && pRoom->GetRoomType() == emROOM_TYPE_COMMON){
            pRoom->LeaveRoom(pGamePlayer);
        }        
    }else{
        rep.set_result(0);
    }
    pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_LEAVE_TABLE_REP);

    return 0;
}
// 进入桌子
int  CHandleLobbyMsg::handle_msg_enter_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_enter_table_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid = head->uin;
    LOG_DEBUG("请求进入桌子:%d-->%d",uid,msg.table_id());
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;
    CGameRoom* pRoom = pGamePlayer->GetRoom();
    if(pRoom == NULL)
    {
        LOG_DEBUG("换桌操作不在房间中:%d",uid);
        return 0;
    }
    uint8 bRet = pRoom->EnterTable(pGamePlayer,msg.table_id(),msg.passwd());
    if(msg.table_id() != 0 && bRet == RESULT_CODE_SUCCESS)// 特殊处理，私人房进入成功不返还
        return 0;

    net::msg_enter_table_rep rep;
    rep.set_result(bRet);
    rep.set_table_id(msg.table_id());
    pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_ENTER_TABLE);
    return 0;
}
// 桌子准备
int  CHandleLobbyMsg::handle_msg_table_ready(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR){
        LOG_DEBUG("维护状态,直接掉线切服");
        return 0;
    }
    net::msg_table_ready_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("桌子准备");
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;

    CGameTable* pTable = pGamePlayer->GetTable();
    if(pTable != NULL){
        pTable->PlayerReady(pGamePlayer);
        pGamePlayer->SetAutoReady(false);
    }
    return 0;
}
// 桌子聊天
int  CHandleLobbyMsg::handle_msg_table_chat(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_table_chat_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("桌子聊天");
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;

    CGameTable* pTable = pGamePlayer->GetTable();
    if(pTable != NULL)
    {
        net::msg_table_chat_rep rep;
        rep.set_uid(pGamePlayer->GetUID());
        rep.set_chat_msg(msg.chat_msg());
        pTable->SendMsgToAll(&rep,net::S2C_MSG_TABLE_CHAT);
    }
    return 0;
}
// 桌子设置托管
int  CHandleLobbyMsg::handle_msg_table_set_auto(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_table_set_auto_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("桌子设置托管:%d",msg.auto_type());
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;

    CGameTable* pTable = pGamePlayer->GetTable();
    if(pTable != NULL){
        pTable->PlayerSetAuto(pGamePlayer,msg.auto_type());
    }
    return 0;
}
// 快速开始
int  CHandleLobbyMsg::handle_msg_fast_join_room(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_fast_join_room_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("快速加入房间");
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
	if (pGamePlayer == NULL)
	{
		LOG_DEBUG("pGamePlayer:%p,uid:%d", pGamePlayer, head->uin);
		return 0;
	}
    net::msg_fast_join_room_rep rep;
    uint8 bRet = CGameRoomMgr::Instance().FastJoinRoom(pGamePlayer,msg.deal(),msg.consume());
    rep.set_result(bRet);
    pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_FAST_JOIN_ROOM_REP);
	LOG_DEBUG("rep - uid:%d,result:%d", pGamePlayer->GetUID(), rep.result());
    return 0;
}
// 快速换桌
int  CHandleLobbyMsg::handle_msg_fast_join_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    //LOG_DEBUG("快速换桌");
    net::msg_fast_join_table_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
	if (pGamePlayer == NULL)
	{
		//LOG_DEBUG("pGamePlayer:%p,uid:%d", pGamePlayer, head->uin);		
		return 0;
	}
    net::msg_fast_join_table_rep rep;
    rep.set_result(0);
    CGameRoom* pRoom = pGamePlayer->GetRoom();
    if(pRoom != NULL)
    {
        if(pRoom->FastJoinTable(pGamePlayer))
		{
            rep.set_result(1);
        }
    }
    LOG_DEBUG("rep - uid:%d,result:%d", pGamePlayer->GetUID(),rep.result());
    pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_FAST_JOIN_TABLE_REP);
    return 0;
}

int  CHandleLobbyMsg::handle_msg_master_join_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_master_join_table_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CGamePlayer* pGamePlayer = GetGamePlayer(head);
	if (pGamePlayer == NULL)
	{
		LOG_DEBUG("pGamePlayer:%p,uid:%d", pGamePlayer, head->uin);		
		return 0;
	}
	net::msg_master_join_table_rep rep;
	rep.set_result(0);
	uint32 result_leave = 0;
	uint32 result_enter = 0;
	uint32 result_ctrl = 0;
	map<uint32, tagUserControlCfg> mpCfgInfo;
	CDataCfgMgr::Instance().GetUserControlCfg(mpCfgInfo);
	auto find_iter = mpCfgInfo.find(pGamePlayer->GetUID());
	tagUserControlCfg tagCtrlCfg;
	uint32 gametype = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
	CGamePlayer * pTagPlayer = NULL;
	CGameRoom * pTagRoom = NULL;
	CGameTable * pTagTable = NULL;

	CGameRoom * pMasterRoom = pGamePlayer->GetRoom();
	CGameTable * pMasterTable = pGamePlayer->GetTable();
	
	bool bMasterNeedLeave = true;
	bool bMasterLeaveFlag = true;

	LOG_DEBUG("rep 1 - gametype:%d,suid:%d,result:%d,tuid:%d,pTagRoom:%p,pTagTable:%p,pMasterRoom:%p,pMasterTable:%p,bMasterNeedLeave:%d,bMasterLeaveFlag:%d,result_leave:%d,result_enter:%d",
		gametype,pGamePlayer->GetUID(), rep.result(), tagCtrlCfg.tuid, pTagRoom, pTagTable, pMasterRoom, pMasterTable, bMasterNeedLeave, bMasterLeaveFlag, result_leave, result_enter);

	if (find_iter != mpCfgInfo.end())
	{
		tagCtrlCfg = find_iter->second;
		auto find_set = tagCtrlCfg.cgid.find(gametype);
		if (find_set != tagCtrlCfg.cgid.end())
		{
			pTagPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(tagCtrlCfg.tuid);
			if (pTagPlayer != NULL)
			{
				pTagRoom = pTagPlayer->GetRoom();
				pTagTable = pTagPlayer->GetTable();
				if (pMasterRoom != NULL && pMasterTable != NULL && pTagRoom != NULL && pTagTable != NULL)
				{
					// 和目标的桌子一样就不需要离开
					if (pMasterRoom->GetRoomID() == pTagRoom->GetRoomID() && pMasterTable->GetTableID() == pTagTable->GetTableID())
					{
						rep.set_result(1);
						result_leave = 1;
						bMasterNeedLeave = false;
					}
				}
				// 目标的桌子不为空、需要离开桌子  才离开自己的桌子
				if (pTagRoom != NULL && pTagTable != NULL && bMasterNeedLeave)
				{
					if (pMasterTable != NULL)
					{
						if (pMasterTable->CanLeaveTable(pGamePlayer) && pMasterTable->LeaveTable(pGamePlayer))
						{
							result_leave = 2;
						}
						else
						{
							result_leave = 3;
							bMasterLeaveFlag = false;
						}
					}
					pMasterRoom = pGamePlayer->GetRoom();
					if (pMasterRoom != NULL)
					{
						if (pMasterRoom->CanLeaveRoom(pGamePlayer) && pMasterRoom->LeaveRoom(pGamePlayer))
						{
							result_leave = 4;
						}
						else
						{
							result_leave = 5;
							bMasterLeaveFlag = false;
						}
					}
				}

				// 目标不为空并且离开成功开始进入玩家的桌子
				if (pTagRoom != NULL && pTagTable != NULL && bMasterLeaveFlag)
				{
					if (pTagRoom->CanEnterRoom(pGamePlayer) && pTagRoom->EnterRoom(pGamePlayer))
					{
						if (pTagTable->CanEnterTable(pGamePlayer) && pTagTable->EnterTable(pGamePlayer))
						{
							rep.set_result(1);
							result_enter = 6;
						}
						else
						{
							result_enter = 7;
						}
					}
					else
					{
						result_enter = 8;
					}
				}
			}
			else
			{
				result_ctrl = 1;
			}
		}
		else
		{
			result_ctrl = 2;
		}
	}
	else
	{
		result_ctrl = 3;
	}

	uint32 uMasterRoomID = 255;
	uint32 uMasterTableID = 255;
	uint32 uTagRoomID = 255;
	uint32 uTagTableID = 255;
	
	if (pGamePlayer->GetRoom()!=NULL)
	{
		uMasterRoomID = pGamePlayer->GetRoom()->GetRoomID();
	}
	if (pGamePlayer->GetTable() != NULL)
	{
		uMasterTableID = pGamePlayer->GetTable()->GetTableID();
	}
	if (pTagRoom != NULL)
	{
		uTagRoomID = pTagRoom->GetRoomID();
	}
	if (pTagTable != NULL)
	{
		uTagTableID = pTagTable->GetTableID();
	}

	LOG_DEBUG("rep 2 - suid:%d,result:%d,tuid:%d,pTagRoom:%p,pTagTable:%p,pMasterRoom:%p,pMasterTable:%p,bMasterNeedLeave:%d,bMasterLeaveFlag:%d,result_leave:%d,result_enter:%d,result_ctrl:%d,uMasterRoomID:%d,uMasterTableID:%d,uTagRoomID:%d,uTagTableID:%d",
		pGamePlayer->GetUID(), rep.result(), tagCtrlCfg.tuid, pTagRoom, pTagTable, pMasterRoom, pMasterTable, bMasterNeedLeave, bMasterLeaveFlag, result_leave, result_enter, result_ctrl, uMasterRoomID, uMasterTableID, uTagRoomID, uTagTableID);

	pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_MASTER_JOIN_TABLE_REP);
	return 0;
}

int  CHandleLobbyMsg::handle_msg_common_join_table(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_common_join_table_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	CGamePlayer* pGamePlayer = GetGamePlayer(head);
	if (pGamePlayer == NULL)
	{
		return 0;
	}
	uint32 gametype = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
	if (gametype != net::GAME_CATE_TWO_PEOPLE_MAJIANG)
	{
		LOG_DEBUG("enter_table - uid:%d,gametype:%d", pGamePlayer->GetUID(), gametype);
		return 0;
	}
	LOG_DEBUG("enter_table - uid:%d", pGamePlayer->GetUID());
	net::msg_common_join_table_rep rep;
	rep.set_result(0);
	CGameRoom* pRoom = pGamePlayer->GetRoom();
	if (pRoom != NULL)
	{
		if (pRoom->CommonJoinTable(pGamePlayer))
		{
			rep.set_result(1);
		}
	}

	LOG_DEBUG("enter_table_rep - uid:%d,result:%d", pGamePlayer->GetUID(), rep.result());
	pGamePlayer->SendMsgToClient(&rep, net::S2C_MSG_COMMON_JOIN_TABLE_REP);
	return 0;
}
// 查看桌子信息
int  CHandleLobbyMsg::handle_msg_query_table_list(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{
    net::msg_query_table_list_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    uint32 uid = head->uin;
    LOG_DEBUG("查看桌子信息:%d",uid);
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;

    if(pGamePlayer->GetRoom() == NULL)
    {
        LOG_DEBUG("玩家没有进入房间:%d",uid);
        return 0;
    }
    uint32 startID = msg.startid();
    uint32 endID   = msg.endid();
    pGamePlayer->GetRoom()->QueryTableListToPlayer(pGamePlayer,startID,endID);

    return 0;
}
// 站立做起
int  CHandleLobbyMsg::handle_msg_sitdown_standup(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
{  
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR){
        LOG_DEBUG("维护状态,直接掉线切服");
        return 0;
    }
    net::msg_sitdown_standup_req msg;
    PARSE_MSG_FROM_ARRAY(msg);
    LOG_DEBUG("站立坐下");
    CGamePlayer* pGamePlayer = GetGamePlayer(head);
    if(pGamePlayer == NULL)
        return 0;
	LOG_DEBUG("uid:%d", pGamePlayer->GetUID());

	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_TWO_PEOPLE_MAJIANG)
	{
		return 0;
	}
    CGameTable* pTable = pGamePlayer->GetTable();
    if(pTable != NULL)
    {
        net::msg_sitdown_standup_rep rep;
        
        if(pTable->PlayerSitDownStandUp(pGamePlayer,msg.oper_id(),msg.chair_id())){
            rep.set_result(net::RESULT_CODE_SUCCESS);
        }else{
            rep.set_result(net::RESULT_CODE_FAIL);
        }
        
        rep.set_oper_id(msg.oper_id());
        rep.set_chair_id(msg.chair_id());        
        pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_SITDOWN_STANDUP);
    }
    return 0;
}

int  CHandleLobbyMsg::handle_msg_items_user(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR) {
		LOG_DEBUG("维护状态,直接掉线切服");
		return 0;
	}
	net::msg_items_user_req msg;
	PARSE_MSG_FROM_ARRAY(msg);
	//LOG_DEBUG("站立坐下");
	CGamePlayer* pGamePlayer = GetGamePlayer(head);
	if (pGamePlayer == NULL)
		return 0;

	//CGamePlayer* pBeGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(msg.beuid());

	msg_items_user_rep repmsg;


	CGameTable* pTable = pGamePlayer->GetTable();
	if (pTable != NULL)
	{
		repmsg.set_mechair_id(pTable->GetChairID(pGamePlayer));
		//repmsg.set_bechair_id(pTable->GetChairID(pBeGamePlayer));
		repmsg.set_bechair_id(msg.bechair_id());
		repmsg.set_item_id(msg.item_id());
		repmsg.set_item_count(msg.item_count());
		
		pTable->SendMsgToAll(&repmsg, net::S2C_MSG_ITEMS_USER_REP);
	}
	return 0;
}

// 修改房间库存配置 add by har
int CHandleLobbyMsg::handle_msg_change_room_stock_cfg(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len, PACKETHEAD* head) {
	net::msg_change_room_stock_cfg msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 roomId = msg.roomid();
	stStockCfg st;
	st.stockMax = msg.stock_max();
	st.stockConversionRate = msg.stock_conversion_rate();
	st.jackpotMin = msg.jackpot_min();
	st.jackpotMaxRate = msg.jackpot_max_rate();
	st.jackpotRate = msg.jackpot_rate();
	st.jackpotCoefficient = msg.jackpot_coefficient();
	st.jackpotExtractRate = msg.jackpot_extract_rate();
	st.stock = msg.add_stock();
	st.killPointsLine = msg.kill_points_line();
	st.playerWinRate = msg.player_win_rate();
	st.jackpot = msg.add_jackpot();

	LOG_DEBUG("change room stock cfg - roomid:%d,stockMax:%d,stockConversionRate:%d,jackpotMin:%d,jackpotMaxRate:%d,jackpotRate:%d,jackpotCoefficient:%d,jackpotExtractRate:%d,stock:%lld,killPointsLine:%lld,playerWinRate:%d,jackpot:%lld",
		roomId, st.stockMax, st.stockConversionRate, st.jackpotMin, st.jackpotMaxRate, st.jackpotRate, st.jackpotCoefficient, st.jackpotExtractRate, st.stock, st.killPointsLine, st.playerWinRate, st.jackpot);
	CGameRoom *pRoom = CGameRoomMgr::Instance().GetRoom(roomId);
	if (pRoom == NULL) {
		LOG_ERROR("handle_msg_change_room_stock_cfg  room is not exist - roomid:%d", roomId);
	}
	else
		pRoom->ChangeRoomStockCfg(st);
	return 0;
}

CGamePlayer* CHandleLobbyMsg::GetGamePlayer(PACKETHEAD* head)
{
    CGamePlayer* pPlayer =  (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(head->uin);
	uint32 uid = 0;
	if (head != NULL)
	{
		uid = head->uin;
	}
    if(pPlayer == NULL)
	{
        //LOG_DEBUG("游戏玩家不存在:%d",head->uin);
		LOG_DEBUG("plyaer_is_not - pPlayer:%p,uid:%d", pPlayer, uid);
    }
	else
	{
        pPlayer->ResetHeart();
    }
    return pPlayer;
}

int  CHandleLobbyMsg::handle_msg_syn_lucky_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_syn_lucky_cfg msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 uid = msg.uid();
	CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	if (pPlayer == NULL)
	{
		LOG_DEBUG("玩家:%d 没有在该游戏中", uid);
	}
	else
	{
		pPlayer->GetLuckyCfg();
	}
	return 0;
}

int  CHandleLobbyMsg::handle_msg_reset_lucky_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	LOG_DEBUG("reset all player lucky info.");

	net::msg_reset_lucky_cfg msg;
	PARSE_MSG_FROM_ARRAY(msg);

	vector<CPlayerBase*> vecPlayers;
	CPlayerMgr::Instance().GetAllPlayers(vecPlayers);
	for (uint32 i = 0; i < vecPlayers.size(); ++i)
	{
		CGamePlayer* pPlayer = (CGamePlayer*)vecPlayers[i];
		if (pPlayer != NULL)
		{
			pPlayer->ResetLuckyInfo();
			LOG_DEBUG("reset online player lucky cfg. uid:%d", pPlayer->GetUID());
		}
	}
	vecPlayers.clear();
	return 0;
}

int  CHandleLobbyMsg::handle_msg_syn_fish_cfg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len, PACKETHEAD* head)
{
	net::msg_syn_fish_cfg msg;
	PARSE_MSG_FROM_ARRAY(msg);

	CFishCfgMgr::Instance().UpdateFishCfg(msg.id(), msg.prize_min(), msg.prize_max(), msg.kill_rate());
	return 0;
}
