//
// Created by toney on 16/4/18.
//

#include <center_log.h>
#include <server_mgr.h>
#include "msg_php_handle.h"
#include "stdafx.h"
#include "lobby_server_config.h"
#include "player.h"
#include "pb/msg_define.pb.h"
#include "json/json.h"
#include "gobal_robot_mgr.h"

using namespace Network;
using namespace svrlib;

int CHandlePHPMsg::OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
#ifndef HANDLE_PHP_FUNC
#define HANDLE_PHP_FUNC(cmd,handle) \
	case cmd:\
	{ \
		handle(pNetObj,stream);\
	}break;
#endif
    uint16 lenght = 0;
    uint16 cmd    = 0;
    CBufferStream stream((char*)pkt_buf,buf_len);
    stream.read_(lenght);
    stream.read_(cmd);
	//if (cmd == 23)
	//{
	//	for (int i = 0; i < buf_len; i++)
	//	{
	//		LOG_DEBUG("test_php - %c ", pkt_buf[i]);
	//	}
	//	SendPHPMsg(pNetObj, "tttsucju", 23);
	//}
    switch(cmd)
    {
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_SAFEPWD,handle_php_change_safepwd);
    HANDLE_PHP_FUNC(net::PHP_MSG_BROADCAST,handle_php_broadcast);
    HANDLE_PHP_FUNC(net::PHP_MSG_SYS_NOTICE,handle_php_sys_notice);
    HANDLE_PHP_FUNC(net::PHP_MSG_KILL_PLAYER,handle_php_kill_player);
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ACCVALUE,handle_php_change_accvalue);
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_NAME,handle_php_change_name);
    HANDLE_PHP_FUNC(net::PHP_MSG_STOP_SERVICE,handle_php_stop_service);
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ROBOT,handle_php_change_robot);
	HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_VIP,handle_php_change_vip);
	HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ROOM_PARAM, handle_php_change_room_param);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONTROL_PLAYER, handle_php_control_player);
	HANDLE_PHP_FUNC(net::PHP_MSG_UPDATE_ACCVALUE_INGAME, handle_php_update_accvalue_ingame);
	HANDLE_PHP_FUNC(net::PHP_MSG_STOP_SNATCH_COIN, handle_php_stop_snatch_coin);
	HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_VIP_BROADCAST, handle_php_change_vip_broadcast);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONTROL_MULTI_PLAYER, handle_php_control_multi_player);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONTROL_DICE_GAME_CARD, handle_php_control_dice_game_card);
	HANDLE_PHP_FUNC(net::PHP_MSG_ONLINE_CONFIG_ROBOT, handle_php_online_config_robot);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_MAJIANG_CARD, handle_php_config_mahiang_card);
	HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ROOM_STOCK_CFG, handle_php_change_room_stock_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_LUCKY_INFO, handle_php_change_lucky_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_FISH_INFO, handle_php_change_fish_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_RESET_LUCK_CONFIG_INFO, handle_php_reset_lucky_cfg);

	default:
        break;
    }
    return 0;
}
/*------------------PHP消息---------------------------------*/
// PHP修改保险箱密码
int  CHandlePHPMsg::handle_php_change_safepwd(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  rep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("pwd"))
    {
        LOG_ERROR("解析连续登陆json错误");
        rep["ret"] = 0;
        SendPHPMsg(pNetObj,rep.toFastString(),net::PHP_MSG_CHANGE_SAFEPWD);
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["pwd"].isString())
    {
        LOG_ERROR("json参数类型错误");
        rep["ret"] = 0;
        SendPHPMsg(pNetObj,rep.toFastString(),net::PHP_MSG_CHANGE_SAFEPWD);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    string passwd = jvalue["pwd"].asString();
    LOG_DEBUG("php修改保险箱密码:%d-%s",uid,passwd.c_str());
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL){
        pPlayer->SetSafePasswd(passwd);
    }
    rep["ret"] = 1;
    SendPHPMsg(pNetObj,rep.toFastString(),net::PHP_MSG_CHANGE_SAFEPWD);
    return 0;
}
// PHP广播消息
int  CHandlePHPMsg::handle_php_broadcast(NetworkObject* pNetObj, CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("msg"))
    {
        LOG_ERROR("json data error");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_BROADCAST);
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["msg"].isString())
    {
        LOG_ERROR("json param error");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_BROADCAST);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    string broadCast = jvalue["msg"].asString();
    LOG_DEBUG("phpbroadcast - msg uid:%d,msg:%s",uid,broadCast.c_str());
    net::msg_php_broadcast_rep rep;
    rep.set_msg(broadCast);
    if(uid == 0)
	{
        CPlayerMgr::Instance().SendMsgToAll(&rep,net::S2C_MSG_PHP_BROADCAST);
    }
	else
	{
        CPlayer *pPlayer = (CPlayer *) CPlayerMgr::Instance().GetPlayer(uid);
		bool bAddRed = false;
		//uint32 uBroadcastCount = 0;
        if (pPlayer != NULL)
		{
            pPlayer->SendMsgToClient(&rep,net::S2C_MSG_PHP_BROADCAST);
        }
		//else
		//{
		//	//保存起来 等客户端登陆完成再发送 保证发给具体用户的时候 用户能够收到
		//	bAddRed = CPlayerMgr::Instance().AddBroadcast(uid, broadCast, uBroadcastCount);
		//}
		//LOG_DEBUG("phpbroadcast - uid:%d,pPlayer:%p,bAddRed:%d,uBroadcastCount:%d,msg:%s", uid, pPlayer, bAddRed, uBroadcastCount, broadCast.c_str());
    }
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_BROADCAST);
    return 0;
}
// PHP系统公告
int  CHandlePHPMsg::handle_php_sys_notice(NetworkObject* pNetObj, CBufferStream& stream)
{
    LOG_DEBUG("php系统公告");


    return 0;
}
// PHP踢出玩家
int  CHandlePHPMsg::handle_php_kill_player(NetworkObject* pNetObj, CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid"))
    {
        LOG_ERROR("解析json错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_KILL_PLAYER);
        return 0;
    }
    if(!jvalue["uid"].isIntegral())
    {
        LOG_ERROR("json参数类型错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_KILL_PLAYER);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    LOG_DEBUG("php踢出玩家:%d",uid);
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);	
    if(pPlayer != NULL)
    {		
        NetworkObject* pSession = pPlayer->GetSession();
        if(pSession){
            pSession->DestroyObj();
        }
        pPlayer->SetNeedRecover(true);
    }
	CDBMysqlMgr::Instance().DeletePlayerOnlineInfo(uid);
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_KILL_PLAYER);
    return 0;
}
// PHP修改玩家数值
int  CHandlePHPMsg::handle_php_change_accvalue(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
	LOG_DEBUG("json analysis begin - msg:%s", msg.c_str());

    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("diamond") || !jvalue.isMember("coin")
    || !jvalue.isMember("score") || !jvalue.isMember("ingot") || !jvalue.isMember("cvalue") || !jvalue.isMember("safecoin")
    || !jvalue.isMember("ptype") || !jvalue.isMember("sptype"))
    {
        LOG_ERROR("解析json错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_ACCVALUE);
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["diamond"].isIntegral() || !jvalue["coin"].isIntegral()
    || !jvalue["score"].isIntegral() || !jvalue["ingot"].isIntegral() || !jvalue["cvalue"].isIntegral()
    || !jvalue["safecoin"].isIntegral() || !jvalue["ptype"].isIntegral() || !jvalue["sptype"].isIntegral())
    {
        LOG_ERROR("json参数类型错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_ACCVALUE);
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
        if(pPlayer->IsInLobby() || !needInLobby)
        {
            bRet = pPlayer->AtomChangeAccountValue(oper_type, sub_type, diamond, coin, ingot, score, cvalue, safecoin);
            if(bRet){
                jrep["diamond"]     = pPlayer->GetAccountValue(emACC_VALUE_DIAMOND);
                jrep["coin"]        = pPlayer->GetAccountValue(emACC_VALUE_COIN);
                jrep["score"]       = pPlayer->GetAccountValue(emACC_VALUE_SCORE);
                jrep["ingot"]       = pPlayer->GetAccountValue(emACC_VALUE_INGOT);
                jrep["cvalue"]      = pPlayer->GetAccountValue(emACC_VALUE_CVALUE);
                jrep["safecoin"]    = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);

                pPlayer->UpdateAccValue2Client();
                pPlayer->FlushChangeAccData2GameSvr(diamond,coin,ingot,score,cvalue,safecoin);
				code = 0;
            }else{
                code=1;
            }
        }else{
            code=2;
        }
    }else{
        bRet = CCommonLogic::AtomChangeOfflineAccData(uid,oper_type,sub_type,diamond,coin,ingot,score,cvalue,safecoin);
        if(!bRet)code=3;
    }
    jrep["ret"]     = bRet ? 1 : 0;
    jrep["code"]    = code;

    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_ACCVALUE);
    return 0;
}
// 修改玩家名字
int   CHandlePHPMsg::handle_php_change_name(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue["uid"].isIntegral())
    {
        LOG_ERROR("解析连续登陆json错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_NAME);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer == NULL){
       jrep["ret"] = 1;
       SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_NAME);
       return 0;  
    }
    if(jvalue.isMember("name") && jvalue["name"].isString()){
        string name = jvalue["name"].asString();
        pPlayer->SetPlayerName(name);
    }    
    if(jvalue.isMember("sex") && jvalue["sex"].isIntegral()){
        uint32 sex = jvalue["sex"].asUInt();
        pPlayer->SetSex(sex);        
    }          
    pPlayer->NotifyChangePlayerInfo2GameSvr();
    
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_NAME);
    return 0;
}
// 停服
int  CHandlePHPMsg::handle_php_stop_service(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("svrid") || !jvalue.isMember("content")
       || !jvalue.isMember("btime") || !jvalue.isMember("etime"))
    {
        LOG_ERROR("解析停服json错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_STOP_SERVICE);
        return 0;
    }
    if(!jvalue["svrid"].isArray() || !jvalue["content"].isString()
       || !jvalue["btime"].isIntegral() || !jvalue["etime"].isIntegral())
    {
        LOG_ERROR("json参数类型错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_STOP_SERVICE);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    string content = jvalue["content"].asString();
    uint32 bTime   = jvalue["btime"].asUInt();
    uint32 eTime   = jvalue["etime"].asUInt();
    vector<uint16> svrids;
    for(uint32 i=0;i<jvalue["svrid"].size();++i){
        if(!jvalue["svrid"][i].isIntegral()){
            LOG_DEBUG("svrid 不是数字");
            continue;
        }
        uint16 svrid = jvalue["svrid"][i].asUInt();
        svrids.push_back(svrid);
        LOG_DEBUG("停服服务器ID:%d",svrid);
    }
    CServerMgr::Instance().NotifyStopService(bTime,eTime,svrids,content);

    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_STOP_SERVICE);
    return 0;
}
// 更改机器人配置
int   CHandlePHPMsg::handle_php_change_robot(NetworkObject* pNetObj,CBufferStream& stream)
{
    CGobalRobotMgr::Instance().ChangeAllRobotCfg();

    Json::Value  jrep;
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_ROBOT);
    return 0;
}
// 更改VIP
int   CHandlePHPMsg::handle_php_change_vip(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("vip"))
    {
        LOG_ERROR("解析连续登陆json错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_VIP);
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["vip"].isIntegral())
    {
        LOG_ERROR("json参数类型错误");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_VIP);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();    
    uint32 vip = jvalue["vip"].asUInt();
    LOG_DEBUG("php change vip:%d-%d",uid,vip);
    CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pPlayer != NULL){        
        pPlayer->SetVip(vip);        
    }
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_CHANGE_VIP);

	return 0;
}
// 更改房间param
int   CHandlePHPMsg::handle_php_change_room_param(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("param"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["param"].isString())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	string param = jvalue["param"].asString();
	LOG_DEBUG("php room  param - gametype:%d,roomid:%d,param:%s", gametype, roomid, param.c_str());
	CServerMgr::Instance().ChangeRoomParam(gametype, roomid, param.c_str());

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);

	return 0;
}

// 控制玩家
int   CHandlePHPMsg::handle_php_control_player(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("uid") || !jvalue.isMember("operatetype") || !jvalue.isMember("gamecount"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_PLAYER);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["uid"].isIntegral() || !jvalue["operatetype"].isIntegral() || !jvalue["gamecount"].isIntegral())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_PLAYER);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	uint32 uid = jvalue["uid"].asUInt();
	uint32 operatetype = jvalue["operatetype"].asUInt();
	uint32 gamecount = jvalue["gamecount"].asUInt();
	
	LOG_DEBUG("control player data - gametype:%d,roomid:%d,uid:%d,operatetype:%d,gamecount:%d", gametype, roomid, uid, operatetype, gamecount);
	CServerMgr::Instance().ChangeContorlPlayer(gametype, roomid, uid, operatetype, gamecount);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_PLAYER);

	return 0;
}

// 控制玩家
int   CHandlePHPMsg::handle_php_control_multi_player(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	LOG_ERROR("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("uid") || !jvalue.isMember("operatetype") || !jvalue.isMember("gamecount") || !jvalue.isMember("gametime") || !jvalue.isMember("totalscore"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_MULTI_PLAYER);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["uid"].isArray() || !jvalue["operatetype"].isIntegral() || !jvalue["gamecount"].isIntegral() || !jvalue["gametime"].isIntegral() || !jvalue["totalscore"].isIntegral())
	{
		//LOG_ERROR("json param analysis error - uid:%d, msg:%s", jvalue["uid"].isObject(),msg.c_str());
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_MULTI_PLAYER);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	//uint32 uid = jvalue["uid"].asUInt();
	uint32 operatetype = jvalue["operatetype"].asUInt();
	uint32 gamecount = jvalue["gamecount"].asUInt();
	uint64 gametime = jvalue["gametime"].asUInt64();
	int64 totalscore = jvalue["totalscore"].asInt64();

	Json::Value  juids = jvalue["uid"];
	LOG_ERROR("json analysis - juids_size:%d", juids.size());

	if (juids.size() > 0)
	{
		for (uint32 i = 0; i < juids.size(); i++)
		{
			LOG_ERROR("json analysis - juids_size:%d,i:%d,isUint:%d", juids.size(), i, juids[i].isIntegral());
			if (!juids[i].isIntegral())
			{
				continue;
			}
			uint32 uid = juids[i].asUInt();

			LOG_DEBUG("control player data - gametype:%d,roomid:%d,uid:%d,operatetype:%d,gamecount:%d,gametime:%lld,totalscore:%lld", gametype, roomid, uid, operatetype, gamecount, gametime, totalscore);

			CServerMgr::Instance().ChangeContorlMultiPlayer(gametype, roomid, uid, operatetype, gamecount, gametime, totalscore);
		}
	}


	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_MULTI_PLAYER);

	return 0;
}

// 更新游戏内金币
int   CHandlePHPMsg::handle_php_update_accvalue_ingame(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("diamond") || !jvalue.isMember("coin")
		|| !jvalue.isMember("score") || !jvalue.isMember("ingot") || !jvalue.isMember("cvalue") || !jvalue.isMember("safecoin")
		|| !jvalue.isMember("ptype") || !jvalue.isMember("sptype"))
	{
		LOG_ERROR("analysis json error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_UPDATE_ACCVALUE_INGAME);
		return 0;
	}
	if (!jvalue["uid"].isIntegral() || !jvalue["diamond"].isIntegral() || !jvalue["coin"].isIntegral()
		|| !jvalue["score"].isIntegral() || !jvalue["ingot"].isIntegral() || !jvalue["cvalue"].isIntegral()
		|| !jvalue["safecoin"].isIntegral() || !jvalue["ptype"].isIntegral() || !jvalue["sptype"].isIntegral())
	{
		LOG_ERROR("json param error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_UPDATE_ACCVALUE_INGAME);
		return 0;
	}
	uint32 uid = jvalue["uid"].asUInt();
	uint32 oper_type = jvalue["ptype"].asUInt();
	uint32 sub_type = jvalue["sptype"].asUInt();
	int64  diamond = jvalue["diamond"].asInt64();
	int64  coin = jvalue["coin"].asInt64();
	int64  score = jvalue["score"].asInt64();
	int64  ingot = jvalue["ingot"].asInt64();
	int64  cvalue = jvalue["cvalue"].asInt64();
	int64  safecoin = jvalue["safecoin"].asInt64();
	LOG_DEBUG("1 update_accvalue_ingame data - uid:%d,oper_type:%d,sub_type:%d,diamond:%lld,coin:%lld,score:%lld,ingot:%lld,cvalue:%lld,safecoin:%lld", uid, oper_type, sub_type, diamond, coin, score, ingot, cvalue, safecoin);
	bool bRet = false;
	int  code = 0;
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);

	LOG_DEBUG("2 update_accvalue_ingame status - uid:%d,oper_type:%d,sub_type:%d,pPlayer:%p", uid, oper_type, sub_type, pPlayer);

	if (pPlayer != NULL)
	{
		if (oper_type == emACCTRAN_OPER_TYPE_BUY)
		{
			pPlayer->AddRecharge(coin);
			pPlayer->SendVipBroadCast();
		}
		//bool needInLobby = true;
		//if (diamond >= 0 && coin >= 0 && score >= 0 && ingot >= 0 && cvalue >= 0 && safecoin >= 0)
		//{
		//	needInLobby = false;
		//}
		bool needInLobby = false;
		if (coin < 0 || score < 0) {
			needInLobby = true;
		}
		LOG_DEBUG("3 update_accvalue_ingame status - uid:%d,oper_type:%d,sub_type:%d,IsInLobby:%d,needInLobby:%d,curSvrID:%d", uid, oper_type, sub_type, pPlayer->IsInLobby(), needInLobby, pPlayer->GetCurSvrID());
		if (pPlayer->IsInLobby() || !needInLobby)
		{
			bRet = pPlayer->AtomChangeAccountValue(oper_type, sub_type, diamond, coin, ingot, score, cvalue, safecoin);
			LOG_DEBUG("4 update_accvalue_ingame status - uid:%d,oper_type:%d,sub_type:%d,bRet:%d,", uid, oper_type, sub_type, bRet);

			if (bRet) {
				jrep["diamond"] = pPlayer->GetAccountValue(emACC_VALUE_DIAMOND);
				jrep["coin"] = pPlayer->GetAccountValue(emACC_VALUE_COIN);
				jrep["score"] = pPlayer->GetAccountValue(emACC_VALUE_SCORE);
				jrep["ingot"] = pPlayer->GetAccountValue(emACC_VALUE_INGOT);
				jrep["cvalue"] = pPlayer->GetAccountValue(emACC_VALUE_CVALUE);
				jrep["safecoin"] = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);

				pPlayer->UpdateAccValue2Client();
				uint32 temp_sub_type = 0;
				stGServer* pServer = CServerMgr::Instance().GetServerBySvrID(pPlayer->GetCurSvrID());
				if (pServer != NULL)
				{
					temp_sub_type = pServer->gameType;
				}
				pPlayer->UpDateChangeAccData2GameSvr(diamond, coin, ingot, score, cvalue, safecoin, oper_type, temp_sub_type);

				code = 0;
			}
			else {
				code = 1;
			}
		}
		else {
			code = 2;
		}
	}
	else {
		bRet = CCommonLogic::AtomChangeOfflineAccData(uid, oper_type, sub_type, diamond, coin, ingot, score, cvalue, safecoin);
		LOG_DEBUG("5 update_accvalue_ingame status player is null - uid:%d,oper_type:%d,sub_type:%d,bRet:%d,", uid, oper_type, sub_type, bRet);
		if (!bRet)code = 3;
	}
	jrep["ret"] = bRet ? 1 : 0;
	jrep["code"] = code;

	LOG_DEBUG("6 update_accvalue_ingame status - uid:%d,oper_type:%d,bRet:%d,code:%d", uid, oper_type, bRet, code);

	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_UPDATE_ACCVALUE_INGAME);
	return 0;
}

int CHandlePHPMsg::handle_php_stop_snatch_coin(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("stop"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_STOP_SNATCH_COIN);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["stop"].isIntegral())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_STOP_SNATCH_COIN);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	uint32 stop = jvalue["stop"].asUInt();
	LOG_DEBUG("php room  param - gametype:%d,roomid:%d,stop:%d", gametype, roomid, stop);
	CServerMgr::Instance().StopSnatchCoin(gametype, roomid, stop);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_STOP_SNATCH_COIN);

	return 0;
}


int CHandlePHPMsg::handle_php_change_vip_broadcast(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("viphorn"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_VIP_BROADCAST);
		return 0;
	}
	if (!jvalue["viphorn"].isObject())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_VIP_BROADCAST);
		return 0;
	}
	Json::Value  jvalueBroadCast = jvalue["viphorn"];
	string strBroadCast = jvalueBroadCast.toFastString();

	LOG_DEBUG("php strBroadCast  param - strBroadCast:%s", strBroadCast.c_str());

	CDataCfgMgr::Instance().VipBroadCastAnalysis(true,strBroadCast);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_VIP_BROADCAST);

	return 0;
}

int   CHandlePHPMsg::handle_php_control_dice_game_card(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("dice"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_DICE_GAME_CARD);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["dice"].isArray())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_DICE_GAME_CARD);
		return 0;
	}
	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	Json::Value  jdice = jvalue["dice"];
	LOG_DEBUG("json analysis - gametype:%d,roomid:%d,jdice:%d", gametype, roomid, jdice.size());

	uint32 uDice[3] = { 0 };
	bool bIsSendDiceServer = true;
	if (jdice.size() == 3)
	{
		for (uint32 i = 0; i < jdice.size(); i++)
		{
			LOG_DEBUG("json analysis - jdice_size:%d,i:%d,isUint:%d", jdice.size(), i, jdice[i].isIntegral());
			if (!jdice[i].isIntegral())
			{
				continue;
			}
			uDice[i] = jdice[i].asUInt();
			if (uDice[i] <= 0 || uDice[i] >= 7)
			{
				bIsSendDiceServer = false;
				break;
			}
		}
	}
	else
	{
		bIsSendDiceServer = false;
	}
	if (bIsSendDiceServer)
	{
		CServerMgr::Instance().ContorlDiceGameCard(gametype, roomid, uDice);
	}


	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONTROL_DICE_GAME_CARD);

	return 0;
}

//PHP_MSG_ONLINE_CONFIG_ROBOT = 37;
// 添加总账号
//{"action":"update_acount", "gametype" : 13}
// 修改游戏机器配置
//{"action":"update_game", "gametype" : 13, "svrid" : 131, "robot" : 1}
// 修改游戏房间机器配置
//{"action":"update_room",  "gametype" : 13, "roomid" : 1, "robot" : 1}
// 更新机器上线配置
//{"action":"update_count", "id" : 1, "gametype" : 13, "roomid" : 1,  "leveltype" : 1,  "online" : [0, 0, 0, 20, 15, 12, 4, 8, 12, 0, 0, 0, 0, 0, 0, 0, 0]}
// 删除机器人批次
//{"action":"delete_bacthid",  "gametype" : 13, "id" : 1}

int CHandlePHPMsg::handle_php_online_config_robot(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue) || !jvalue.isMember("action"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
		return 0;
	}
	string straction = jvalue["action"].asString();
	if (straction == "update_acount")
	{
		if (jvalue.isMember("gametype") && jvalue["gametype"].isIntegral())
		{
			int gametype = jvalue["gametype"].asInt();
			CGobalRobotMgr::Instance().AddRobotPayPoolData(gametype);
		}
	}
	else if (straction == "update_game")
	{
		if (jvalue.isMember("gametype") && jvalue["gametype"].isIntegral() &&
			jvalue.isMember("svrid") && jvalue["svrid"].isIntegral() &&
			jvalue.isMember("robot") && jvalue["robot"].isIntegral())
		{
			int gametype = jvalue["gametype"].asInt();
			int svrid = jvalue["svrid"].asInt();
			int robot = jvalue["robot"].asInt();

			LOG_DEBUG("gametype:%d,svrid:%d, robot:%d", gametype, svrid, robot);

			CServerMgr::Instance().UpdateOpenRobot(svrid, robot);
		}
	}
	else if (straction == "update_room")
	{
		if (jvalue.isMember("robot") && jvalue["robot"].isIntegral() &&
			jvalue.isMember("gametype") && jvalue["gametype"].isIntegral() &&
			jvalue.isMember("roomid") && jvalue["roomid"].isIntegral())
		{
			int gametype = jvalue["gametype"].asInt();
			//int svrid = jvalue["svrid"].asInt();
			int roomid = jvalue["roomid"].asInt();
			int robot = jvalue["robot"].asInt();

			LOG_DEBUG("gametype:%d,roomid:%d, robot:%d", gametype, roomid, robot);

			CServerMgr::Instance().UpdateServerRoomRobot(gametype, roomid, robot);
		}
	}
	else if (straction == "delete_bacthid")
	{
		if (jvalue.isMember("gametype") && jvalue["gametype"].isIntegral() &&
			jvalue.isMember("id") && jvalue["id"].isIntegral())
		{
			int gametype = jvalue["gametype"].asInt();
			int batchid = jvalue["id"].asInt();

			bool bIsDelete = CDataCfgMgr::Instance().DeleteRobotOnlineCfg(gametype, batchid);

			LOG_DEBUG("bIsDelete:%d,gametype:%d,batchid:%d", bIsDelete, gametype, batchid);

			if (bIsDelete)
			{
				CServerMgr::Instance().DeleteRobotOnlineCfg(gametype, batchid);
			}
		}
	}
	else if (straction == "update_count")
	{
		if (jvalue.isMember("gametype") && jvalue["gametype"].isIntegral() &&
			jvalue.isMember("roomid") && jvalue["roomid"].isIntegral() &&
			jvalue.isMember("leveltype") && jvalue["leveltype"].isIntegral() &&
			jvalue.isMember("id") && jvalue["id"].isIntegral() &&
			jvalue.isMember("online") && jvalue["online"].isArray())
		{
			stRobotOnlineCfg refCfg;
			refCfg.batchID = jvalue["id"].asInt();
			refCfg.gameType = jvalue["gametype"].asInt();
			refCfg.roomID = jvalue["roomid"].asInt();
			refCfg.leveltype = jvalue["leveltype"].asInt();

			Json::Value jonline = jvalue["online"];

			if (ROBOT_MAX_LEVEL != jonline.size())
			{
				LOG_ERROR("json analysis error - msg:%s", msg.c_str());
				jrep["ret"] = 0;
				SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
				return 0;
			}

			for (uint32 i = 0; i < jonline.size(); i++)
			{
				LOG_DEBUG("json analysis - jonline:%d,i:%d,isIntegral:%d", jonline.size(), i, jonline[i].isIntegral());
				if (!jonline[i].isIntegral())
				{
					continue;
				}
				int count = jonline[i].asInt();
				if (count >= 0)
				{
					refCfg.onlines[i] = count;
				}
			}
			CDataCfgMgr::Instance().UpdateRobotOnlineCfg(refCfg);
			CServerMgr::Instance().UpdateRobotOnlineCfg(refCfg);
		}
	}
	else
	{
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);
		return 0;
	}

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_ONLINE_CONFIG_ROBOT);

	return 0;
}

// {"gametype":12, "roomid" : 1,"handcard":[[0x37,0x05],[0x37,0x37]]}
int CHandlePHPMsg::handle_php_config_mahiang_card(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_MAJIANG_CARD);
		return 0;
	}

	if (!jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("handcard"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_MAJIANG_CARD);
		return 0;
	}
	if (!jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() || !jvalue["handcard"].isArray())
	{
		LOG_ERROR("json param analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_MAJIANG_CARD);
		return 0;
	}

	uint32 gametype = jvalue["gametype"].asUInt();
	uint32 roomid = jvalue["roomid"].asUInt();
	string strHandCard = jvalue["handcard"].toFastString();
	LOG_DEBUG("json analysis - gametype:%d,roomid:%d,strHandCard:%s", gametype, roomid, strHandCard.c_str());
	CServerMgr::Instance().ConfigMajiangHandCard(gametype, roomid, strHandCard);
	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_MAJIANG_CARD);

	return 0;
}

// 更改房间库存配置 add by har
int CHandlePHPMsg::handle_php_change_room_stock_cfg(NetworkObject *pNetObj, CBufferStream &stream) {
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") ||
		!jvalue.isMember("stock_max") || !jvalue.isMember("stock_conversion_rate") || !jvalue.isMember("jackpot_min") ||
		!jvalue.isMember("jackpot_max_rate") || !jvalue.isMember("jackpot_rate") || !jvalue.isMember("jackpot_coefficient") ||
		!jvalue.isMember("jackpot_extract_rate") || !jvalue.isMember("add_stock") || !jvalue.isMember("kill_points_line") ||
		!jvalue.isMember("player_win_rate") || !jvalue.isMember("add_jackpot") || !jvalue["gametype"].isIntegral() || !jvalue["roomid"].isIntegral() ||
		!jvalue["stock_max"].isIntegral() || !jvalue["stock_conversion_rate"].isIntegral() ||
		!jvalue["jackpot_min"].isIntegral() || !jvalue["jackpot_max_rate"].isIntegral() || !jvalue["jackpot_rate"].isIntegral() ||
		!jvalue["jackpot_coefficient"].isIntegral() || !jvalue["jackpot_extract_rate"].isIntegral() || !jvalue["add_stock"].isIntegral() ||
		!jvalue["kill_points_line"].isIntegral() || !jvalue["player_win_rate"].isIntegral() || !jvalue["add_jackpot"].isIntegral())
	{
		LOG_ERROR("handle_php_change_room_stock_cfg json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);
		return 0;
	}
	stStockCfg st;
	uint32 gametype = jvalue["gametype"].asUInt();
	st.roomID = jvalue["roomid"].asUInt();
	st.stockMax = jvalue["stock_max"].asInt64();
	st.stockConversionRate = jvalue["stock_conversion_rate"].asInt();
	st.jackpotMin = jvalue["jackpot_min"].asInt64();
	st.jackpotMaxRate = jvalue["jackpot_max_rate"].asInt();
	st.jackpotRate = jvalue["jackpot_rate"].asInt();
	st.jackpotCoefficient = jvalue["jackpot_coefficient"].asInt();
	st.jackpotExtractRate = jvalue["jackpot_extract_rate"].asInt();
	st.stock = jvalue["add_stock"].asInt64();
	st.killPointsLine = jvalue["kill_points_line"].asInt64();
	st.playerWinRate = jvalue["player_win_rate"].asInt();
	st.jackpot = jvalue["add_jackpot"].asInt64();
	LOG_DEBUG("handle_php_change_room_stock_cfg  param - gametype:%d,roomid:%d,stockMax:%d,stockonvCersionRate:%d,jackpotMin:%d,jackpotMaxRate:%d,jackpotRate:%d,jackpotCoefficient:%d,jackpotExtractRate:%d,stock:%lld,killPointsLine:%lld,playerWinRate:%d,jackpot:%lld",
		gametype, st.roomID, st.stockMax, st.stockConversionRate, st.jackpotMin, st.jackpotMaxRate, st.jackpotRate,
		st.jackpotCoefficient, st.jackpotExtractRate, st.stock, st.killPointsLine, st.playerWinRate, st.jackpot);

	if (!CServerMgr::Instance().NotifyGameSvrsChangeRoomStockCfg(gametype, st)) {
		jrep["ret"] = 0;
		LOG_ERROR("handle_php_change_room_stock_cfg game server not exist - msg:%s", msg.c_str());
	}
	else
		jrep["ret"] = 1;

	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);

	return 0;
}

void  CHandlePHPMsg::SendPHPMsg(NetworkObject* pNetObj,string jmsg,uint16 cmd)
{
    CBufferStream& sendStream = sendStream.buildStream();
    uint16 len = PHP_HEAD_LEN + jmsg.length();
    sendStream.write_(len);
    sendStream.write_(cmd);
    sendStream.writeString(jmsg);

    pNetObj->Send(sendStream.getBuffer(),sendStream.getPosition());
    //LOG_DEBUG("回复php消息:%d--%d",cmd,sendStream.getPosition());
}

int  CHandlePHPMsg::handle_php_change_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_LUCKY_INFO);
		return 0;
	}

	uint32 uid = 0;
	if (!jvalue.isMember("uid"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_LUCKY_INFO);
		return 0;
	}
	else
	{
		uid = jvalue["uid"].asUInt();
	}

	LOG_DEBUG("update player lucky info - uid:%d", uid);

	msg_syn_lucky_cfg synMsg;
	synMsg.set_uid(uid);
	CServerMgr::Instance().SynLuckyCfg(&synMsg);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_LUCKY_INFO);
	return 0;
}

int  CHandlePHPMsg::handle_php_change_fish_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	if (!reader.parse(msg, jvalue))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}

	uint32 id = 0;
	if (!jvalue.isMember("id"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}
	else
	{
		id = jvalue["id"].asUInt();
	}

	uint32 prize_min = 0;
	if (!jvalue.isMember("prize_min"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}
	else
	{
		prize_min = jvalue["prize_min"].asUInt();
	}

	uint32 prize_max = 0;
	if (!jvalue.isMember("prize_max"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}
	else
	{
		prize_max = jvalue["prize_max"].asUInt();
	}

	uint32 kill_rate = 0;
	if (!jvalue.isMember("kill_rate"))
	{
		LOG_ERROR("json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
		return 0;
	}
	else
	{
		kill_rate = jvalue["kill_rate"].asUInt();
	}

	LOG_DEBUG("update fish config info - id:%d prize_min:%d prize_max:%d kill_rate:%d", id, prize_min, prize_max, kill_rate);

	msg_syn_fish_cfg synMsg;
	synMsg.set_id(id);
	synMsg.set_prize_min(prize_min);
	synMsg.set_prize_max(prize_max);
	synMsg.set_kill_rate(kill_rate);
	CServerMgr::Instance().SynFishCfg(&synMsg);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);
	return 0;
}

int  CHandlePHPMsg::handle_php_reset_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;

	LOG_DEBUG("json analysis - msg:%s", msg.c_str());

	msg_reset_lucky_cfg synMsg;
	synMsg.set_uid(0);
	CServerMgr::Instance().ResetLuckyCfg(&synMsg);

	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_RESET_LUCK_CONFIG_INFO);
	return 0;
}
