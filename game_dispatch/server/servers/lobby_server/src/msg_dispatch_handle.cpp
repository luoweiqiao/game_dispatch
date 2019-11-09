#include <data_cfg_mgr.h>
#include "stdafx.h"
#include "msg_dispatch_handle.h"
#include "pb/msg_define.pb.h"
#include "center_log.h"
#include "gobal_event_mgr.h"
#include "json/json.h"
#include "gobal_robot_mgr.h"

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
		HANDLE_DISPATCH_FUNC(net::D2LS_MSG_REGISTER_REP, handle_msg_register_server_rep);
		HANDLE_DISPATCH_FUNC(net::D2L_MSG_RETIRE_LOBBYSVR, handle_msg_retire_lobbysvr);
		HANDLE_DISPATCH_FUNC(net::D2S_MSG_RETIRE_GAMESVR, handle_msg_retire_gamesvr);
		HANDLE_DISPATCH_FUNC(net::D2L_MSG_BROADCAST, handle_broadcast_msg);
		HANDLE_DISPATCH_FUNC(net::D2LS_MSG_CHECK_OTHER_LOBBY_SERVER, handle_check_other_lobby_server);

	}

	return 0;
}

int CHandleDispatchMsg::handle_msg_register_server_rep(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_register_dispatch_rep msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 result = msg.result(); 
	LOG_DEBUG("register dispatch rep: svrid[%d] result[%d]", pNetObj->GetUID(), result);
	bool rep = (result == 0) ? false : true;
	DispatchMgr::Instance().RegisterRep(pNetObj->GetUID(), rep);

	return 0;
}

int CHandleDispatchMsg::handle_msg_retire_lobbysvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_retire_lobbysvr msg;
	PARSE_MSG_FROM_ARRAY(msg);
	LOG_DEBUG("lobby server retire !!!!!!!!!!");
	CApplication::Instance().SetStatus(emSERVER_STATE_RETIRE);

	return 0;
}

int CHandleDispatchMsg::handle_msg_retire_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_retire_gamesvr msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 gameSvrID = msg.svrid();
	LOG_DEBUG("game server retired:%d", gameSvrID);

	CServerMgr::Instance().SetSvrStatus(gameSvrID, emSERVER_STATE_RETIRE);
	CServerMgr::Instance().SendSvrsInfoToAll();
	return 0;
}

int CHandleDispatchMsg::handle_check_other_lobby_server(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_check_other_lobby_server msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 lobbySvrID = msg.svrid();
	uint32 uid = msg.uid();
	uint32 curSvrID = CApplication::Instance().GetServerID();
	uint32 redisLobbyID = CRedisMgr::Instance().GetPlayerLobbySvrID(uid);
	CPlayer* pPlayer = NULL;

	LOG_DEBUG("repeat_login_check_start - uid:%d,lobbySvrID:%d,curSvrID:%d,redisLobbyID:%d", uid, lobbySvrID, curSvrID, redisLobbyID);
	if (lobbySvrID != curSvrID)
	{
		if (uid != 0)
		{
			pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
			if (pPlayer != NULL)
			{
				pPlayer->SetOtherRecover(true);
				uint16 playerInServerID = pPlayer->GetCurSvrID();
				
				NetworkObject *pOldSock = pPlayer->GetSession();
				if (pOldSock != NULL)
				{
					net::msg_notify_leave_rep leavemsg;
					leavemsg.set_result(net::RESULT_CODE_LOGIN_OTHER);
					SendProtobufMsg(pOldSock, &leavemsg, net::S2C_MSG_NOTIFY_LEAVE);
					//pOldSock->SetUID(0);
					//CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(GetUID(), 0, 0, pPlayer->GetPlayerType());
					pPlayer->SetNeedRecover(true);
					pOldSock->DestroyObj();
					pPlayer->SetSession(NULL);
					
					LOG_DEBUG("repeat_login_check_success - uid:%d,playerInServerID:%d,lobbySvrID:%d,curSvrID:%d,redisLobbyID:%d", uid, playerInServerID, lobbySvrID, curSvrID, redisLobbyID);
				}
				else
				{
					LOG_DEBUG("repeat_login_check_failed - uid:%d,playerInServerID:%d,lobbySvrID:%d,curSvrID:%d,redisLobbyID:%d,pOldSock:%p", uid, playerInServerID, lobbySvrID, curSvrID, redisLobbyID, pOldSock);
				}
			}
			else
			{
				LOG_DEBUG("repeat_login_check_failed - uid:%d,lobbySvrID:%d,curSvrID:%d,redisLobbyID:%d,pPlayer:%p", uid, lobbySvrID, curSvrID, redisLobbyID, pPlayer);
			}			
		}
	}
	return 0;
}

int CHandleDispatchMsg::handle_broadcast_msg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_broadcast_info msg;
	PARSE_MSG_FROM_ARRAY(msg);

	string bdData = msg.data();
	uint32 bdCmd = msg.cmd();

	const char * pData = bdData.c_str();
	uint16 uDataLen = bdData.size();

#ifndef HANDLE_DISPATCH_TRANS_FUNC
#define HANDLE_DISPATCH_TRANS_FUNC(bdCmd, handlebd)\
	case(bdCmd):\
	{ \
		handlebd(pNetObj, pData, uDataLen);\
}break;
#endif
	switch(bdCmd)
	{
		HANDLE_DISPATCH_TRANS_FUNC(net::LD2L_MSG_SYNC_OTHER_LOBBY_PLAYER_DATA, handle_broadcast_sync_other_player_data);
		HANDLE_DISPATCH_TRANS_FUNC(net::C2S_MSG_GIVE_SAFEBOX, handle_broadcast_give_safebox);
		HANDLE_DISPATCH_TRANS_FUNC(net::S2C_MSG_CHAT_INFO_FORWARD_REP, handle_broadcast_chat_info_forward_rep);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_BROADCAST, handle_php_broadcast);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_CHANGE_ROBOT_BOSS_ACCVALUE, handle_php_change_robot_boss_accvalue);

		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE, handle_php_notify_vip_proxy_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_UNION_PAY_RECHARGE, handle_php_notify_union_pay_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_WECHAT_PAY_RECHARGE, handle_php_notify_wechat_pay_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_ALI_PAY_RECHARGE, handle_php_notify_ali_pay_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_OTHER_PAY_RECHARGE, handle_php_notify_other_pay_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_QQ_PAY_RECHARGE, handle_php_notify_qq_pay_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE, handle_php_notify_wechat_scan_pay_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_JD_PAY_RECHARGE, handle_php_notify_jd_pay_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE, handle_php_notify_apple_pay_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_CONFIG_CONTROL_USER, handle_php_config_control_user);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_LARGE_ALI_PAY_RECHARGE, handle_php_notify_large_ali_pay_recharge);
		HANDLE_DISPATCH_TRANS_FUNC(net::PHP_MSG_NOTIFY_EXCLUSIVE_FLASH_RECHARGE, handle_php_notify_exclusive_flash_recharge);

	}
	return 0;
}

//********************************二级包处理****************************************************
	// 同步其他大厅服玩家的数据(通过中心服转发)
int CHandleDispatchMsg::handle_broadcast_sync_other_player_data(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len) {
	net::msg_sync_other_lobby_player_data msg;
	PARSE_MSG_FROM_ARRAY(msg); 
	CPlayerMgr::Instance().SetOtherLobbyPlayerData(msg.uid(), msg.name());
	return 0;
}

int CHandleDispatchMsg::handle_broadcast_give_safebox(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	net::msg_give_safebox_req msg;
	PARSE_MSG_FROM_ARRAY(msg);

	int64 gcoin = msg.give_coin();
    uint32 guid = msg.give_uid();
    uint32 ownuid = msg.own_uid();

    if (gcoin < 100 || guid == 0)// 最少送100
    {
    	LOG_ERROR("cant give safebox gcoin:%ld guid:%d", gcoin, guid);
        return 0;
    }

    CPlayer* pTarPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(guid);
    if(!pTarPlayer)
    {
    	LOG_ERROR("cant get user:%d", guid);
    	return 0;
    }

    pTarPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_GIVE, ownuid, 0, 0, 0, 0, 0, gcoin);
    pTarPlayer->UpdateAccValue2Client();

    return 0;
}

// 玩家聊天信息转发
int CHandleDispatchMsg::handle_broadcast_chat_info_forward_rep(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len) {
	net::msg_chat_info_forward msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 duid = msg.duid();

	CPlayer* pTarPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(duid);
	if (pTarPlayer == NULL) {
		LOG_ERROR("cant get user:%d", duid);
		return 0;
	}

	pTarPlayer->SendMsgToClient(&msg, net::S2C_MSG_CHAT_INFO_FORWARD_REP);
	LOG_DEBUG("duid:%d,fromid:%d,mtype:%d,messagStr:%s,uiTime:%lld", duid, msg.fromid(),
		msg.mtype(), msg.messagstr().c_str(), msg.time());

	return 0;
}

int CHandleDispatchMsg::handle_php_broadcast(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
    msg.append(pkt_buf, buf_len);
    Json::Reader reader;
    Json::Value  jvalue;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("msg"))
    {
        LOG_ERROR("parse json failed");
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["msg"].isString())
    {
        LOG_ERROR("json args type is invaild");
        return 0;
    }
    
    uint32 uid = jvalue["uid"].asUInt();
    string broadCast = jvalue["msg"].asString();
    //LOG_DEBUG("php广播消息:%d-%s",uid,broadCast.c_str());
    net::msg_php_broadcast_rep rep;
    rep.set_msg(broadCast);
    if(uid == 0){
        CPlayerMgr::Instance().SendMsgToAll(&rep,net::S2C_MSG_PHP_BROADCAST);
    }else{
        CPlayer *pPlayer = (CPlayer *) CPlayerMgr::Instance().GetPlayer(uid);
        if (pPlayer != NULL) {
            pPlayer->SendMsgToClient(&rep,net::S2C_MSG_PHP_BROADCAST);
        }
    }

    return 0;
}

int CHandleDispatchMsg::handle_php_change_robot_boss_accvalue(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
    msg.append(pkt_buf, buf_len);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
	LOG_DEBUG("json analysis begin - msg:%s", msg.c_str());

    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("diamond") || !jvalue.isMember("coin")
    || !jvalue.isMember("score") || !jvalue.isMember("ingot") || !jvalue.isMember("cvalue") || !jvalue.isMember("safecoin")
    || !jvalue.isMember("ptype") || !jvalue.isMember("sptype"))
    {
        LOG_ERROR("analyze json error");
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["diamond"].isIntegral() || !jvalue["coin"].isIntegral()
    || !jvalue["score"].isIntegral() || !jvalue["ingot"].isIntegral() || !jvalue["cvalue"].isIntegral()
    || !jvalue["safecoin"].isIntegral() || !jvalue["ptype"].isIntegral() || !jvalue["sptype"].isIntegral())
    {
        LOG_ERROR("json param type error");
        return 0;
    }
    uint32 uid       = jvalue["uid"].asUInt();
    uint32 oper_type = jvalue["ptype"].asUInt();
    uint32 sub_type  = jvalue["sptype"].asUInt();
    int64  diamond   = jvalue["diamond"].asInt64();
    int64  coin      = jvalue["coin"].asInt64();
    int64  score     = jvalue["score"].asInt64();
    int64  ingot     = jvalue["ingot"].asInt64();
    int64  cvalue    = jvalue["cvalue"].asInt64();
    int64  safecoin  = jvalue["safecoin"].asInt64();
    LOG_DEBUG("php_change_value uid:%d,oper_type:%d,sub_type:%d,diamond %lld- coin %lld- score %lld- ingot %lld-cvalue %lld,safecoin:%lld", uid, oper_type, sub_type,diamond,coin,score,ingot,cvalue,safecoin);
    bool bRet = false;
    int  code = 0;
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL)
    {
        bool needInLobby = true;
        if(diamond >=0 && coin >=0 && score >= 0 && ingot >= 0 && cvalue >=0 && safecoin >=0){
            needInLobby = false;
        }
        if(pPlayer->IsInLobby())
        {
            bRet = pPlayer->AtomChangeAccountValue(oper_type, sub_type, diamond, coin, ingot, score, cvalue, safecoin);
            if(bRet)
            {
                /*
                jrep["diamond"]     = pPlayer->GetAccountValue(emACC_VALUE_DIAMOND);
                jrep["coin"]        = pPlayer->GetAccountValue(emACC_VALUE_COIN);
                jrep["score"]       = pPlayer->GetAccountValue(emACC_VALUE_SCORE);
                jrep["ingot"]       = pPlayer->GetAccountValue(emACC_VALUE_INGOT);
                jrep["cvalue"]      = pPlayer->GetAccountValue(emACC_VALUE_CVALUE);
                jrep["safecoin"]    = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
                */
                LOG_DEBUG("change robot boss:%d success", uid);
            }else
            {
                LOG_ERROR("change robot boss:%d error", uid);
            }
        }
        else
        {
            LOG_ERROR("ROBOT BOSS NOT IN LOBBY:%d ?????",uid);
        }
    }
    else
    {
        bRet = CCommonLogic::AtomChangeOfflineAccData(uid,oper_type,sub_type,diamond,coin,ingot,score,cvalue,safecoin);
    	if(!bRet)    
    	{
    		LOG_ERROR("change robot boss:%d error", uid);
    	}
    }

    return 0;
}


/*
{
	"vipproxyrecharge": {
		"data": {
			"action": "vip_wechat",
			"wcinfo": [{
				"wx_account": "weixin001",
				"wx_title": "\u6d4b\u8bd5\u6807\u9898001001001001001001",
				"sort_id": 1,
				"showtime": "",
				"vip": "[1,3,8,9,12,0]",
				"low_amount": 80
			}, {
				"wx_account": "weixin004",
				"wx_title": "\u5fae\u4fe1\u6807\u9898004",
				"sort_id": 10,
				"showtime": "",
				"vip": "[10,12,13,14,4,2,3]",
				"low_amount": 50
			}]
		}
	}
}

{"vipalipayrecharge":{"data":{"action":"vip_proxy","recharge":0,"status":0}}}

	"vipalipayrecharge": {
		"data": {
			"action": "vip_aliacc",
			"wcinfo": [{
				"wx_account": "weixin001",
				"wx_title": "\u6d4b\u8bd5\u6807\u9898001001001001001001",
				"sort_id": 1,
				"showtime": "",
				"vip": "[1,3,8,9,12,0]",
				"low_amount": 80
			}, {
				"wx_account": "weixin004",
				"wx_title": "\u5fae\u4fe1\u6807\u9898004",
				"sort_id": 10,
				"showtime": "",
				"vip": "[10,12,13,14,4,2,3]",
				"low_amount": 50
			}]
		}
	}
}

{"vipalipayrecharge":{"data":{"action":"vipalipay","recharge":0,"status":0}}}


*/


int CHandleDispatchMsg::handle_php_notify_vip_proxy_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (reader.parse(msg, jvalue) && jvalue.isMember("vipproxyrecharge") && jvalue["vipproxyrecharge"].isObject())
	{
		Json::Value  jvalueVipProxyRecharge = jvalue["vipproxyrecharge"];
		string strjvalueVipProxyRecharge = jvalueVipProxyRecharge.toFastString();

		LOG_DEBUG("php strjvalueVipProxyRecharge  param - strjvalueVipProxyRecharge:%s", strjvalueVipProxyRecharge.c_str());

		CDataCfgMgr::Instance().VipProxyRechargeAnalysis(true, strjvalueVipProxyRecharge);
		jrep["ret"] = 1;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE);
		return 0;
	}
	else if (reader.parse(msg, jvalue) && jvalue.isMember("vipalipayrecharge") && jvalue["vipalipayrecharge"].isObject())
	{
		Json::Value  jvalueVipRecharge = jvalue["vipalipayrecharge"];
		string strjvalueVipRecharge = jvalueVipRecharge.toFastString();

		LOG_DEBUG("php strjvalueVipRecharge param - str:%s", strjvalueVipRecharge.c_str());

		CDataCfgMgr::Instance().VipAliAccRechargeAnalysis(true, strjvalueVipRecharge);
		jrep["ret"] = 1;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE);
		return 0;
	}

	LOG_ERROR("json analysis error - msg:%s", msg.c_str());
	jrep["ret"] = 0;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE);
	return 0;
}


int CHandleDispatchMsg::handle_php_notify_union_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("unionpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UNION_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["unionpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UNION_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueUnionpayRecharge = jvalue["unionpayrecharge"];
	string strjvalueUnionpayRecharge = jvalueUnionpayRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueUnionpayRecharge:%s", strjvalueUnionpayRecharge.c_str());

	CDataCfgMgr::Instance().UnionPayRechargeAnalysis(true, strjvalueUnionpayRecharge);

	jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_UNION_PAY_RECHARGE);

	return 0;
}

int CHandleDispatchMsg::handle_php_notify_wechat_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("wechatpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["wechatpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["wechatpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().WeChatPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_PAY_RECHARGE);

	return 0;
}

int CHandleDispatchMsg::handle_php_notify_ali_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("alipayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_ALI_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["alipayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_ALI_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["alipayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().AliPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_ALI_PAY_RECHARGE);

	return 0;
}

int CHandleDispatchMsg::handle_php_notify_other_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("otherpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_OTHER_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["otherpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_OTHER_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["otherpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().OtherPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_OTHER_PAY_RECHARGE);

	return 0;
}

int CHandleDispatchMsg::handle_php_notify_qq_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("qqpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_QQ_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["qqpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_QQ_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["qqpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().QQPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_QQ_PAY_RECHARGE);

	return 0;
}

int CHandleDispatchMsg::handle_php_notify_wechat_scan_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("wcscanpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["wcscanpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["wcscanpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().WeChatScanPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE);

	return 0;
}

int CHandleDispatchMsg::handle_php_notify_jd_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("jdpayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_JD_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["jdpayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_JD_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["jdpayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().JDPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_JD_PAY_RECHARGE);

	return 0;
}

int CHandleDispatchMsg::handle_php_notify_apple_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("applepayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["applepayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["applepayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().ApplePayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);

	return 0;
}

// {"opertype":1,"suid":1,"sdeviceid":"abc","tuid" : 1,"cgid":[1,2,3],"skey":"abc"}
int CHandleDispatchMsg::handle_php_config_control_user(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		//jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_CONTROL_USER);
		return 0;
	}

	if (!jvalue.isMember("opertype") || !jvalue.isMember("suid"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		//jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_CONTROL_USER);
		return 0;
	}
	uint32 opertype = jvalue["opertype"].asUInt();
	uint32 suid = 0;
	set<uint32> set_suid;

	//如果为删除类型，则需要按数组类型解析
	if (opertype == 3)
	{
		string suid_tmp = jvalue["suid"].toFastString();

		//解析用户ID
		if (suid_tmp.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(suid_tmp, jvalue))
			{
				LOG_ERROR("json param analysis error - msg:%s", suid_tmp.c_str());				
				return 0;
			}
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				set_suid.insert(jvalue[i].asUInt());
			}
		}
	}
	else
	{
		suid = jvalue["suid"].asUInt();
	}	
	tagUserControlCfg info;
	info.suid = suid;

	if (jvalue.isMember("sdeviceid"))
	{
		info.sdeviceid = jvalue["sdeviceid"].asString();
	}

	if (jvalue.isMember("skey"))
	{
		info.skey = jvalue["skey"].asString();
	}

	if (jvalue.isMember("tuid"))
	{
		info.tuid = jvalue["tuid"].asUInt();
	}
	string cgid_tmp;
	if (jvalue.isMember("cgid"))
	{
		cgid_tmp = jvalue["cgid"].toFastString();
		//解析游戏ID
		if (cgid_tmp.size() > 0)
		{
			Json::Reader reader;
			Json::Value  jvalue;
			if (!reader.parse(cgid_tmp, jvalue))
			{
				LOG_ERROR("json param analysis error - msg:%s", cgid_tmp.c_str());
				//jrep["ret"] = 0;
				//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_CONTROL_USER);
				return 0;
			}
			LOG_DEBUG("cgid size:%d", jvalue.size());
			for (uint32 i = 0; i < jvalue.size(); ++i)
			{
				info.cgid.insert(jvalue[i].asUInt());
				LOG_DEBUG("value:%d info.cgid size:%d", jvalue[i].asUInt(), info.cgid.size());
			}
		}
	}
	LOG_DEBUG("opertype:%d suid:%d tuid:%d cgid:%s skey:%s sdeviceid:%s", opertype, suid, info.tuid, cgid_tmp.c_str(), info.skey.c_str(), info.sdeviceid.c_str());
	//jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_CONTROL_USER);

	//对于修改和删除类型，需要在线的控制玩家的行为	
	map<uint32, tagUserControlCfg> mpCfgInfo;
	CDataCfgMgr::Instance().GetUserControlCfg(mpCfgInfo);

	bool isNoticeFlag = false;  //是否通知玩家返回大厅，由于配置发生变化
	uint16 curSvrID = 0;		//当前玩家所在的SVRID

	//修改控制玩家配置
	if (opertype == 2)
	{
		//判断配置是否存在
		auto iter_player = mpCfgInfo.find(suid);
		if (iter_player != mpCfgInfo.end())
		{
			//如果当前玩家处于某个游戏中时，需要判断当前控制游戏是否发生改变
			info.skey = iter_player->second.skey;
			CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(suid);
			if (pPlayer != NULL && pPlayer->GetCurSvrID() != 0)
			{
				//设备码是否发生改变
				if (!isNoticeFlag && info.sdeviceid != iter_player->second.sdeviceid)
				{
					isNoticeFlag = true;
				}
				
				//tuid值是否发生改变
				if (!isNoticeFlag && info.tuid != iter_player->second.tuid)
				{
					isNoticeFlag = true;
				}
				stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(pPlayer->GetCurSvrID());
				if (!isNoticeFlag && pServer != NULL)
				{
					//判断所在游戏ID是否在新的游戏配置列表中
					auto iter_gametype = info.cgid.find(pServer->gameType);
					if (iter_gametype == info.cgid.end())
					{
						isNoticeFlag = true;
					}
				}
				curSvrID = pPlayer->GetCurSvrID();

				//如果当前控制玩家正在某个游戏中，则需要中断控制
				if (isNoticeFlag && curSvrID != 0)
				{
					CServerMgr::Instance().StopContrlPlayer(curSvrID, suid);
				}
			}
		}
	}

	//删除控制玩家配置
	if (opertype == 3)
	{
		for (uint32 suid_tmp : set_suid)
		{    		
			auto iter_player = mpCfgInfo.find(suid_tmp);
			if (iter_player != mpCfgInfo.end())
			{
				//如果当前玩家处于某个游戏中时，需要判断当前控制游戏是否发生改变
				CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(suid_tmp);
				if (pPlayer != NULL && pPlayer->GetCurSvrID() != 0)
				{
					isNoticeFlag = true;
					curSvrID = pPlayer->GetCurSvrID();
					CServerMgr::Instance().StopContrlPlayer(curSvrID, suid_tmp);
				}
			}
		}
	}
	
	//更新配置信息
	if (opertype == 3)
	{
		for (uint32 suid_tmp : set_suid)
		{
			CDataCfgMgr::Instance().UpdateUserControlInfo(suid_tmp, opertype, info);
		}
	}
	else
	{
		CDataCfgMgr::Instance().UpdateUserControlInfo(suid, opertype, info);
	}

	msg_syn_ctrl_user_cfg synMsg;
	if (opertype == 3)
	{
		for (uint32 suid_tmp : set_suid)
		{
			synMsg.add_vec_suid(suid_tmp);

		}
	}
	else
	{
		synMsg.add_vec_suid(suid);
	}
	synMsg.set_opertype(opertype);

	synMsg.set_tag_suid(info.suid);
	synMsg.set_tag_sdeviceid(info.sdeviceid);
	synMsg.set_tag_tuid(info.tuid);
	for (uint32 gid : info.cgid)
	{
		synMsg.add_tag_cgid(gid);
	}
	synMsg.set_tag_skey(info.skey);

	CServerMgr::Instance().SynCtrlUserCfg(&synMsg);

	return 0;
}

int CHandleDispatchMsg::handle_php_notify_large_ali_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len)
{
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("bigalipayrecharge"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);
		return 0;
	}
	if (!jvalue["bigalipayrecharge"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["bigalipayrecharge"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().LargeAliPayRechargeAnalysis(true, strjvalueRecharge);

	jrep["ret"] = 1;
	//SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE);

	return 0;
}

// php修改专享闪付充值显示信息
int CHandleDispatchMsg::handle_php_notify_exclusive_flash_recharge(NetworkObject *pNetObj, const char* pkt_buf, uint16 buf_len) {
	string msg;
	msg.append(pkt_buf, buf_len);
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("exclusiveshanpay")) {
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		return 0;
	}
	if (!jvalue["exclusiveshanpay"].isObject()) {
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		return 0;
	}

	Json::Value  jvalueRecharge = jvalue["exclusiveshanpay"];
	string strjvalueRecharge = jvalueRecharge.toFastString();

	LOG_DEBUG("php_jvalueRecharge  param - jvalueRecharge:%s", strjvalueRecharge.c_str());

	CDataCfgMgr::Instance().ExclusiveFlashRechargeAnalysis(true, strjvalueRecharge);

	return 0;
}
