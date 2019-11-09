
#include "server_mgr.h"
#include "pb/msg_define.pb.h"
#include "stdafx.h"

using namespace svrlib;
using namespace Network;

CServerMgr::CServerMgr()
{
	m_mpServers.clear();
}
CServerMgr::~CServerMgr()
{

}

bool CServerMgr::Init()
{
	SetMsgSinker(new CHandleServerMsg());
	return true;
}


void  CServerMgr::ShutDown()
{
}

void CServerMgr::OnTimer(uint8 eventID)
{

}

bool  CServerMgr::AddServer(NetworkObject* pNetObj, uint32 svrID, uint32 svrType, uint32 gameType)
{
	MAP_SERVERS::iterator iter = m_mpServers.find(svrID);
	if(iter != m_mpServers.end() && iter->second.pNetObj != NULL)
	{
		LOG_ERROR("GameSvr:%d already exist", svrID);
		return false;
	}

	stGServer server;
	server.svrID = svrID;
	server.svrType = svrType;
	server.gameType = gameType;
	server.pNetObj = pNetObj;
	pNetObj->SetUID(svrID);

	m_mpServers.insert(make_pair(svrID, server));
	stServerCfg* svrcfg = CDataCfgMgr::Instance().GetServerCfg(svrID);
	if(NULL == svrcfg)
	{
		CDataCfgMgr::Instance().Reload();
	}
	CRedisMgr::Instance().WriteSvrStatusInfo(svrID, server.palyerNum, server.robotNum, server.status);
	return true;
}

void   CServerMgr::RemoveServer(NetworkObject* pNetObj)
{
	MAP_SERVERS::iterator iter = m_mpServers.begin();
	for(; iter != m_mpServers.end(); ++iter)
	{
		stGServer &server = iter->second;
		if(server.pNetObj == pNetObj)
		{
			m_mpServers.erase(iter);
			pNetObj->SetUID(0);
			CRedisMgr::Instance().DelSvrStatusInfo(server.svrID);
			break;
		}
	}
}

stGServer * CServerMgr::GetServer(uint32 svrID)
{
	MAP_SERVERS::iterator iter = m_mpServers.find(svrID);
	if(iter != m_mpServers.end())
	{
		return &iter->second;
	}

	return NULL;
}

stGServer * CServerMgr::GetServerByNetObj(NetworkObject* pNetObj)
{
	MAP_SERVERS::iterator iter = m_mpServers.begin();
	for(; iter != m_mpServers.end(); ++iter)
	{
		if(iter->second.pNetObj == pNetObj)
		{
			return &iter->second;
		}
	}

	return NULL;
}

//退休游戏服务器
void CServerMgr::NotifyRetireServer(vector<uint32> svrs)
{
	std::vector<uint32>::iterator vecIter = svrs.begin();
	for(; vecIter != svrs.end(); ++vecIter)
	{
		MAP_SERVERS::iterator svrIter = m_mpServers.find(*vecIter);
		if(svrIter != m_mpServers.end())
		{
			stGServer &server = svrIter->second;
			if(server.status != emSERVER_STATE_NORMAL)
			{
				continue;
			}
			net::msg_retire_gamesvr msg;
			LOG_DEBUG("retire server:%d", server.svrID);
			server.status = emSERVER_STATE_RETIRE; //退休
			SendProtobufMsg(server.pNetObj, &msg, net::D2S_MSG_RETIRE_GAMESVR, CApplication::Instance().GetServerID());
		}
	}
}

//通知gameserver连接新大厅
void   CServerMgr::NotifyGameSvrsHasNewLobby(uint32 svrid)
{
	net::msg_notify_gamesvrs_new_lobby msg;
	msg.set_lobby_svrid(svrid);

	MAP_SERVERS::iterator iter = m_mpServers.begin();
	for(; iter != m_mpServers.end(); ++iter)
	{
		stGServer server = iter->second;
		if(server.status == emSERVER_STATE_NORMAL)
		{
			SendProtobufMsg(server.pNetObj, &msg, net::D2S_MSG_NOTIFY_GAMESVRS_NEW_LOBBY, CApplication::Instance().GetServerID());
		}
	}
}

//通知所有子游戏有大厅服务器退休
void CServerMgr::NotifyGameSvrRetire(uint32 svrID)
{
	net::msg_retire_lobbysvr msg;
	msg.set_svrid(svrID);
	MAP_SERVERS::iterator iter = m_mpServers.begin();
	for(; iter != m_mpServers.end(); ++iter)
	{
		stGServer server = iter->second;
		if(server.status == emSERVER_STATE_NORMAL)
		{
			SendProtobufMsg(server.pNetObj, &msg, net::D2L_MSG_RETIRE_LOBBYSVR, CApplication::Instance().GetServerID());
		}
	}
}

// 通知游戏服修改房间库存配置  add by har
bool CServerMgr::NotifyGameSvrsChangeRoomStockCfg(uint32 gameType, stStockCfg &st) {
	bool bRet = false;
	net::msg_change_room_stock_cfg msg;
	msg.set_roomid(st.roomID);
	msg.set_stock_min(st.stockMin);
	msg.set_stock_max(st.stockMax);
	msg.set_stock_conversion_rate(st.stockConversionRate);
	msg.set_jackpot_min(st.jackpotMin);
	msg.set_jackpot_max_rate(st.jackpotMaxRate);
	msg.set_jackpot_rate(st.jackpotRate);
	msg.set_jackpot_coefficient(st.jackpotCoefficient);
	msg.set_jackpot_extract_rate(st.jackpotExtractRate);
	msg.set_add_stock(st.stock);
	for (MAP_SERVERS::iterator iter = m_mpServers.begin(); iter != m_mpServers.end(); ++iter) {
		stGServer& server = iter->second;
		if (server.status == emSERVER_STATE_NORMAL && server.gameType == gameType) {
			bRet = true;
			SendProtobufMsg(server.pNetObj, &msg, net::D2S_MSG_CHANGE_ROOM_STOCK_CFG, CApplication::Instance().GetServerID());
		}
	}
	return bRet;
}

void CServerMgr::WriteSvrsInfo(Json::Value & value)
{
	MAP_SERVERS::iterator iter = m_mpServers.begin();
	for(; iter != m_mpServers.end(); ++iter)
	{
		stServerCfg * svrCfg = CDataCfgMgr::Instance().GetServerCfg(iter->first);
		if(!svrCfg)
		{
			LOG_ERROR("Get svr:%d stServerCfg error", iter->first);
			return;
		}
		stGServer server = iter->second;
		Json::Value item;
		item["svrid"] = server.svrID;
		item["status"] = server.status;
		item["players"] = server.palyerNum;
		item["robots"] = server.robotNum;
		item["svr_type"] = svrCfg->svrType;
		item["svrport"] = svrCfg->svrport;
		item["svrip"] = svrCfg->svrip.c_str();

		value["svrinfo"].append(item);
	}
}

//---------------------------------------------------------------------------------------------------
int CHandleServerMsg::OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)	
{
#ifndef HANDLE_SERVER_FUNC
#define HANDLE_SERVER_FUNC(cmd,handle) \
	case cmd:\
	{ \
		handle(pNetObj,pkt_buf,buf_len);\
	}break;
#endif

	switch(head->cmd)
	{
    HANDLE_SERVER_FUNC(net::LS2D_MSG_REGISTER, handle_msg_gameSvr_register);
    HANDLE_SERVER_FUNC(net::LS2D_MSG_REPORT_ONLINES, handle_msg_Online_Info);
    
	default:
       	LOG_ERROR("cant handle cmd:%d", head->cmd);
		break;		
	}
	return 0;
}

int  CHandleServerMsg::handle_msg_gameSvr_register(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_register_dispatch msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 svrid = msg.svrid();
	uint32 svrType = msg.svr_type();
	uint32 gametype = msg.game_type();

	LOG_ERROR("gameSvr register svrid[%d] svr_type[%d] gametype[%d]", svrid, svrType, gametype);
	bool ret = CServerMgr::Instance().AddServer(pNetObj, svrid, svrType, gametype);
	uint32 registerResult = 1;
	if(!ret)
	{
		registerResult = 0;
		LOG_ERROR("register svr fail:%d", svrid);
	}

	net::msg_register_dispatch_rep repMsg;
	repMsg.set_result(registerResult);
	SendProtobufMsg(pNetObj, &repMsg, net::D2LS_MSG_REGISTER_REP, CApplication::Instance().GetServerID());

	return 0;
}

int  CHandleServerMsg::handle_msg_Online_Info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_report_onlines msg;
	PARSE_MSG_FROM_ARRAY(msg);

	stGServer * server = CServerMgr::Instance().GetServerByNetObj(pNetObj);
	if(server)
	{
		server->palyerNum = msg.player_size();
		server->robotNum = msg.robot_size();
	}
	else
	{
		LOG_ERROR("cant find game server by pNetObj:%d", pNetObj->GetUID());
	}

	return 0;
}