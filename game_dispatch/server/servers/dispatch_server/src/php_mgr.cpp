#include <center_log.h>
#include "php_mgr.h"
#include "stdafx.h"
#include "pb/msg_define.pb.h"
#include "json/json.h"

#include "server_mgr.h"
#include "lobby_mgr.h"

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
    switch(cmd)
    {
    HANDLE_PHP_FUNC(net::P2D_MSG_SVRLIST, handle_php_get_svrlist);
    HANDLE_PHP_FUNC(net::P2D_MSG_RETIRE_SERVER, handle_php_retire_server);
    HANDLE_PHP_FUNC(net::PHP_MSG_BROADCAST,handle_php_broadcast);
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ROBOT, handle_php_change_robot);
    HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ROBOT_BOSS_ACCVALUE, handle_php_change_robot_boss_accvalue);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_CONTROL_USER, handle_php_change_user_ctrl_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_CHANGE_ROOM_STOCK_CFG, handle_php_change_room_stock_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_LUCKY_INFO, handle_php_change_lucky_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_RESET_LUCK_CONFIG_INFO, handle_php_reset_lucky_cfg);
	HANDLE_PHP_FUNC(net::PHP_MSG_CONFIG_FISH_INFO, handle_php_change_fish_cfg);

    default:
	{
		handle_php_to_other_lobby_server(cmd, pNetObj, stream);

		break;
	}

    }
    return 0;
}

int	CHandlePHPMsg::handle_php_to_other_lobby_server(uint16 cmd,NetworkObject* pNetObj, CBufferStream& stream)
{
	if (cmd == net::PHP_MSG_NOTIFY_VIP_PROXY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_UNION_PAY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_WECHAT_PAY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_ALI_PAY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_OTHER_PAY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_QQ_PAY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_JD_PAY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_APPLE_PAY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_LARGE_ALI_PAY_RECHARGE ||
		cmd == net::PHP_MSG_NOTIFY_EXCLUSIVE_FLASH_RECHARGE)
	{
		string msg;
		stream.readString(msg);
		LOG_DEBUG("cmd:%d,msg:%s", cmd, msg.c_str());

		Json::Value  jrep;
		jrep["ret"] = 1;
		SendPHPMsg(pNetObj, jrep.toFastString(), cmd);
		
		net::msg_broadcast_info transMsg;
		transMsg.set_data(msg.data());
		transMsg.set_cmd(cmd);

		CLobbyMgr::Instance().BroadcastInfo2Lobby(&transMsg, net::D2L_MSG_BROADCAST, CApplication::Instance().GetServerID(), 0);
	}
	return 1;
}

int  CHandlePHPMsg::handle_php_retire_server(NetworkObject* pNetObj, CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
	LOG_DEBUG("msg:%s", msg.c_str());
    if (!reader.parse(msg, jvalue) || !jvalue.isMember("svrid"))
    {
        LOG_ERROR("json error msg:%s", msg.c_str());
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj, jrep.toFastString(), net::P2D_MSG_RETIRE_SERVER);
        return 0;
    }
    if (!jvalue["svrid"].isArray())
    {
        LOG_ERROR("json error msg:%s", msg.c_str());
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj, jrep.toFastString(), net::P2D_MSG_RETIRE_SERVER);
        return 0;
    }
    vector<uint32> svrids;
    for (uint32 i = 0; i < jvalue["svrid"].size(); ++i)
    {
        if (!jvalue["svrid"][i].isIntegral())
        {
            LOG_ERROR("svrid not integer");
            continue;
        }
        uint16 svrid = jvalue["svrid"][i].asUInt();
        svrids.push_back(svrid);
        LOG_DEBUG("retire svrID:%d", svrid);
    }
    CServerMgr::Instance().NotifyRetireServer(svrids);
    CLobbyMgr::Instance().NotifyRetireServer(svrids);

    jrep["ret"] = 1;
    SendPHPMsg(pNetObj, jrep.toFastString(), net::P2D_MSG_RETIRE_SERVER);
    return 0;
}

int  CHandlePHPMsg::handle_php_get_svrlist(NetworkObject* pNetObj, CBufferStream& stream)
{
    Json::Value value;
    CServerMgr::Instance().WriteSvrsInfo(value);
    CLobbyMgr::Instance().WriteSvrsInfo(value);
    LOG_ERROR("json string:%s",  value.toFastString().c_str());
    SendPHPMsg(pNetObj, value.toFastString(), net::P2D_MSG_SVRLIST);
    return 0; 
}

int  CHandlePHPMsg::handle_php_broadcast(NetworkObject* pNetObj, CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
	LOG_DEBUG("msg:%s", msg.c_str());

    Json::Reader reader;
    Json::Value  jvalue;
    Json::Value  jrep;
    if(!reader.parse(msg,jvalue) || !jvalue.isMember("uid") || !jvalue.isMember("msg"))
    {
        LOG_ERROR("parse json failed");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_BROADCAST);
        return 0;
    }
    if(!jvalue["uid"].isIntegral() || !jvalue["msg"].isString())
    {
        LOG_ERROR("json args type is invaild");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj,jrep.toFastString(),net::PHP_MSG_BROADCAST);
        return 0;
    }
    uint32 uid = jvalue["uid"].asUInt();
    int32 destSvrID = 0;
    if(uid != 0)
    {
        destSvrID = CRedisMgr::Instance().GetPlayerLobbySvrID(uid);
    }

	LOG_DEBUG("uid:%d,destSvrID:%d,msg:%s", uid, destSvrID, msg.c_str());

    CLobbyMgr::Instance().BroadcastPHPMsg2Lobby(msg, net::PHP_MSG_BROADCAST, destSvrID);
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_BROADCAST);
    return 0;
}

int  CHandlePHPMsg::handle_php_change_robot(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Value  jrep;
    uint32 masterSvrID = CLobbyMgr::Instance().GetMasterSvrID();

	LOG_DEBUG("msg:%s", msg.c_str());

    if(!masterSvrID)
    {
        LOG_ERROR("no find masterSvrID");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROBOT);
        return 0;
    }

    CLobbyMgr::Instance().BroadcastPHPMsg2Lobby(msg, net::PHP_MSG_BROADCAST, masterSvrID);
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROBOT);
    return 0;
}

void  CHandlePHPMsg::SendPHPMsg(NetworkObject* pNetObj,string jmsg,uint16 cmd)
{
    CBufferStream& sendStream = sendStream.buildStream();
    uint16 len = PHP_HEAD_LEN + jmsg.length();
    sendStream.write_(len);
    sendStream.write_(cmd);
    sendStream.writeString(jmsg);

    pNetObj->Send((char*)sendStream.getBuffer(),sendStream.getPosition());
    //LOG_DEBUG("回复php消息:%d--%d",cmd,sendStream.getPosition());
}

int  CHandlePHPMsg::handle_php_change_robot_boss_accvalue(NetworkObject* pNetObj,CBufferStream& stream)
{
    string msg;
    stream.readString(msg);
    Json::Value  jrep;
    uint32 masterSvrID = CLobbyMgr::Instance().GetMasterSvrID();
    if(!masterSvrID)
    {
        LOG_ERROR("no find masterSvrID");
        jrep["ret"] = 0;
        SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROBOT_BOSS_ACCVALUE);
        return 0;
    }
    LOG_DEBUG("change robot boss accvalue - masterSvrID:%d,msg:%s", masterSvrID, msg.c_str());
    CLobbyMgr::Instance().BroadcastPHPMsg2Lobby(msg, net::PHP_MSG_CHANGE_ROBOT_BOSS_ACCVALUE, masterSvrID);
    jrep["ret"] = 1;
    SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROBOT_BOSS_ACCVALUE);

    return 0;
}

int  CHandlePHPMsg::handle_php_change_user_ctrl_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Value  jrep;
	uint32 masterSvrID = 0;
	LOG_DEBUG("change user ctrl cfg - masterSvrID:%d,msg:%s", masterSvrID, msg.c_str());
	CLobbyMgr::Instance().BroadcastPHPMsg2Lobby(msg, net::PHP_MSG_CONFIG_CONTROL_USER, masterSvrID);
	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_CONTROL_USER);

	return 0;
}

// 更改房间库存配置 add by har
int CHandlePHPMsg::handle_php_change_room_stock_cfg(NetworkObject *pNetObj, CBufferStream &stream) {
	string msg;
	stream.readString(msg);
	Json::Reader reader;
	Json::Value  jvalue;
	Json::Value  jrep;
	if (!reader.parse(msg, jvalue) || !jvalue.isMember("gametype") || !jvalue.isMember("roomid") || !jvalue.isMember("stock_min") ||
		!jvalue.isMember("stock_max") || !jvalue.isMember("stock_conversion_rate") || !jvalue.isMember("jackpot_min") ||
		!jvalue.isMember("jackpot_max_rate") || !jvalue.isMember("jackpot_rate") || !jvalue.isMember("jackpot_coefficient") ||
		!jvalue.isMember("jackpot_extract_rate") || !jvalue.isMember("add_stock") || !jvalue["gametype"].isIntegral() || 
		!jvalue["roomid"].isIntegral() || !jvalue["stock_min"].isIntegral() || !jvalue["stock_max"].isIntegral() || 
		!jvalue["stock_conversion_rate"].isIntegral() || !jvalue["jackpot_min"].isIntegral() || !jvalue["jackpot_max_rate"].isIntegral() ||
		!jvalue["jackpot_rate"].isIntegral() || !jvalue["jackpot_coefficient"].isIntegral() || !jvalue["jackpot_extract_rate"].isIntegral() || !jvalue["add_stock"].isIntegral())
	{
		LOG_ERROR("handle_php_change_room_stock_cfg json analysis error - msg:%s", msg.c_str());
		jrep["ret"] = 0;
		SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);
		return 0;
	}
	stStockCfg st;
	uint32 gametype = jvalue["gametype"].asUInt();
	st.roomID = jvalue["roomid"].asUInt();
	st.stockMin = jvalue["stock_min"].asInt64();
	st.stockMax = jvalue["stock_max"].asInt64();
	st.stockConversionRate = jvalue["stock_conversion_rate"].asInt();
	st.jackpotMin = jvalue["jackpot_min"].asInt64();
	st.jackpotMaxRate = jvalue["jackpot_max_rate"].asInt();
	st.jackpotRate = jvalue["jackpot_rate"].asInt();
	st.jackpotCoefficient = jvalue["jackpot_coefficient"].asInt();
	st.jackpotExtractRate = jvalue["jackpot_extract_rate"].asInt();
	st.stock = jvalue["add_stock"].asInt64();
	LOG_DEBUG("handle_php_change_room_stock_cfg  param - gametype:%d,roomid:%d,stockMin:%d,stockMax:%d,stockonvCersionRate:%d,jackpotMin:%d,jackpotMaxRate:%d,jackpotRate:%d,jackpotCoefficient:%d,jackpotExtractRate:%d,stock:%lld",
		gametype, st.roomID, st.stockMin, st.stockMax, st.stockConversionRate, st.jackpotMin, st.jackpotMaxRate,
		st.jackpotRate, st.jackpotCoefficient, st.jackpotExtractRate, st.stock);

	if (!CServerMgr::Instance().NotifyGameSvrsChangeRoomStockCfg(gametype, st)) {
		jrep["ret"] = 0;
		LOG_ERROR("handle_php_change_room_stock_cfg game server not exist - msg:%s", msg.c_str());
	} else
		jrep["ret"] = 1;

	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CHANGE_ROOM_PARAM);

	return 0;
}

int  CHandlePHPMsg::handle_php_change_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Value  jrep;
	uint32 masterSvrID = 0;
	LOG_DEBUG("change lucky cfg - masterSvrID:%d,msg:%s", masterSvrID, msg.c_str());
	CLobbyMgr::Instance().BroadcastPHPMsg2Lobby(msg, net::PHP_MSG_CONFIG_LUCKY_INFO, masterSvrID);
	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_LUCKY_INFO);

	return 0;
}

int  CHandlePHPMsg::handle_php_reset_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Value  jrep;
	uint32 masterSvrID = 0;
	LOG_DEBUG("reset lucky cfg - masterSvrID:%d,msg:%s", masterSvrID, msg.c_str());
	CLobbyMgr::Instance().BroadcastPHPMsg2Lobby(msg, net::PHP_MSG_RESET_LUCK_CONFIG_INFO, masterSvrID);
	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_RESET_LUCK_CONFIG_INFO);

	return 0;
}

int  CHandlePHPMsg::handle_php_change_fish_cfg(NetworkObject* pNetObj, CBufferStream& stream)
{
	string msg;
	stream.readString(msg);
	Json::Value  jrep;
	uint32 masterSvrID = 0;
	LOG_DEBUG("change fish cfg - masterSvrID:%d,msg:%s", masterSvrID, msg.c_str());
	CLobbyMgr::Instance().BroadcastPHPMsg2Lobby(msg, net::PHP_MSG_CONFIG_FISH_INFO, masterSvrID);
	jrep["ret"] = 1;
	SendPHPMsg(pNetObj, jrep.toFastString(), net::PHP_MSG_CONFIG_FISH_INFO);

	return 0;
}