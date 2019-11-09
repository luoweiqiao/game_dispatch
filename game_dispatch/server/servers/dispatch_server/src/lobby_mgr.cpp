#include <data_cfg_mgr.h>
#include "lobby_mgr.h"
#include "dispatch_server_config.h"
#include "game_define.h"
#include "stdafx.h"
#include "server_mgr.h"


using namespace svrlib;
using namespace std;
using namespace Network;
using namespace net;

enum broadcast_type
{
	ALLBROADCAST = 1,     // 广播
	SINGLEBROADCAST = 2,  // 单播
};

CLobbyMgr::CLobbyMgr()
{
	m_lobbySvrs.clear();
}

CLobbyMgr::~CLobbyMgr()
{
}

bool CLobbyMgr::Init()
{
	SetMsgSinker(new CHandleLobbyMsg());
	return true;
}

bool  CLobbyMgr::AddServer(NetworkObject* pNetObj,uint32 svrID,uint32 svrType,uint32 gameType)
{
	MAP_LOBBY::iterator it_lobby = m_lobbySvrs.begin();
	for (; it_lobby != m_lobbySvrs.end(); ++it_lobby)
	{
		stLobbyServer& server = it_lobby->second;
		string		sip;
		uint16_t	port = 0;
		if (server.pNetObj != NULL)
		{
			port = server.pNetObj->GetPort();
			sip = server.pNetObj->GetSIP();
		}
		LOG_DEBUG("lobbyserver 1 - size:%d,sip:%s,port:%d,svrID:%d,svrType:%d,gameType:%d,status:%d,palyerNum:%d,robotNum:%d",
			m_lobbySvrs.size(), sip.c_str(), port, server.svrID, server.svrType, server.gameType,server.status, server.palyerNum, server.robotNum);
	}

	string		sip;
	uint16_t	port = 0;
	int			net_fd = 0;
	if (pNetObj != NULL)
	{
		port = pNetObj->GetPort();
		sip = pNetObj->GetSIP();
		net_fd = pNetObj->GetNetfd();
	}
	LOG_DEBUG("lobbyserver 2 - sip:%s,port:%d,svrID:%d,svrType:%d,gameType:%d net_fd:%d", sip.c_str(), port, svrID, svrType, gameType, net_fd);

	MAP_LOBBY::iterator iter = m_lobbySvrs.find(svrID);
	if(iter != m_lobbySvrs.end() && iter->second.pNetObj != NULL)
	{
		LOG_ERROR("GameSvr:%d already exist", svrID);
		return false;
	}

	stLobbyServer server;
	server.svrID = svrID;
	server.svrType = svrType;
	server.gameType = gameType;
	server.pNetObj = pNetObj;
	pNetObj->SetUID(svrID);
	if(true == CApplication::Instance().call<bool>("CheckIsMasterSvr", svrID))
	{
		server.status = emSERVER_STATE_RETIRE;
	}
	
	m_lobbySvrs.insert(make_pair(svrID,server));

	LOG_DEBUG("insert svrID:%d net_fd:%d", svrID, server.pNetObj->GetNetfd());

	stServerCfg* svrcfg = CDataCfgMgr::Instance().GetServerCfg(svrID);
	if(NULL == svrcfg)
	{
		CDataCfgMgr::Instance().Reload();
	}
	CRedisMgr::Instance().WriteSvrStatusInfo(svrID, server.palyerNum, server.robotNum, server.status);
	//通知gamesvr连接
	CServerMgr::Instance().NotifyGameSvrsHasNewLobby(svrID);
	return true;
}

void CLobbyMgr::ShutDown()
{
}

void CLobbyMgr::OnTimer(uint8 eventID)
{
	
}

void CLobbyMgr::RemoveServer(NetworkObject* pNetObj)
{
	MAP_LOBBY::iterator it = m_lobbySvrs.begin();
	for(;it != m_lobbySvrs.end();++it)
	{
		stLobbyServer& server = it->second;
		if(server.pNetObj == pNetObj)
		{   
			if(server.status == emSERVER_STATE_RETIRE)
			{
				CServerMgr::Instance().NotifyGameSvrRetire(server.svrID);
			}    
			m_lobbySvrs.erase(it);
			pNetObj->SetUID(0);
			CRedisMgr::Instance().DelSvrStatusInfo(server.svrID);

			break;
		}
	}
}

stLobbyServer * CLobbyMgr::GetServerByNetObj(NetworkObject* pNetObj)
{
	MAP_LOBBY::iterator it = m_lobbySvrs.begin();
	for(; it != m_lobbySvrs.end(); ++it)
	{
		if(pNetObj == it->second.pNetObj)
		{
			return &it->second;
		}
	}

	return NULL;
}

//退休大厅服务器
void CLobbyMgr::NotifyRetireServer(vector<uint32> svrs)
{
	std::vector<uint32>::iterator retireIter = svrs.begin();
	for(; retireIter != svrs.end(); ++retireIter)
	{
		MAP_LOBBY::iterator svrIter = m_lobbySvrs.find(*retireIter);
		if(svrIter != m_lobbySvrs.end())
		{
			//retire lobbysvr
			stLobbyServer &server = svrIter->second;
			if(server.status != emSERVER_STATE_NORMAL)
			{
				continue;
			}
			net::msg_retire_lobbysvr msg;
			LOG_DEBUG("retire lobbysvr:%d", server.svrID);
			server.status = emSERVER_STATE_RETIRE;
			SendProtobufMsg(server.pNetObj, &msg, net::D2L_MSG_RETIRE_LOBBYSVR, CApplication::Instance().GetServerID());
		}
		else
		{
			//retire gamesvr
			LOG_DEBUG("retire gamesvr:%d", *retireIter);
			MAP_LOBBY::iterator Iter = m_lobbySvrs.begin();
			for(; Iter != m_lobbySvrs.end(); ++Iter)
			{
				net::msg_retire_gamesvr msg;
				msg.set_svrid(*retireIter);

				SendProtobufMsg(Iter->second.pNetObj, &msg, net::D2S_MSG_RETIRE_GAMESVR, CApplication::Instance().GetServerID());
			}
		}	
	}

}

void CLobbyMgr::WriteSvrsInfo(Json::Value & value)
{
	MAP_LOBBY::iterator iter = m_lobbySvrs.begin();
	for(; iter != m_lobbySvrs.end(); ++iter)
	{
		stServerCfg * svrCfg = CDataCfgMgr::Instance().GetServerCfg(iter->first);
		if(!svrCfg)
		{
			LOG_ERROR("Get svr:%d stServerCfg error", iter->first);
			return;
		}
		stLobbyServer server = iter->second;
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

void CLobbyMgr::BroadcastInfo2Lobby(const google::protobuf::Message* msg,uint16 msg_type, uint32 srcSvrID, uint32 destSvrID)
{
	if(srcSvrID == destSvrID)
	{
		LOG_ERROR("srcSvr:%d == destSvr:%d",srcSvrID, destSvrID);
		return;
	}

	if(destSvrID == 0)
	{
		LOG_ERROR("size:%d", m_lobbySvrs.size());
		MAP_LOBBY::iterator iter = m_lobbySvrs.begin();
		for(; iter != m_lobbySvrs.end(); ++iter)
		{			
			stLobbyServer server = iter->second;
			LOG_ERROR("server.svrID:%d srcSvrID:%d", server.svrID, srcSvrID);
			if (server.svrID == srcSvrID)
			{
				continue;
			}

			SendProtobufMsg(server.pNetObj, msg, msg_type, CApplication::Instance().GetServerID());
		}
	}
	else
	{
		MAP_LOBBY::iterator iter = m_lobbySvrs.find(destSvrID);
		if(iter != m_lobbySvrs.end())
		{
			stLobbyServer server = iter->second;
			SendProtobufMsg(server.pNetObj, msg, msg_type, CApplication::Instance().GetServerID());
		}
	}
}

void CLobbyMgr::CheckOtherLobbyServerLogin(const google::protobuf::Message* msg, uint16 msg_type, uint32 srcSvrID, uint32 uid)
{
	uint32 uMasterSvrID = GetMasterSvrID();
	LOG_DEBUG("other_lobby_start - svrid:%d,uid:%d,uMasterSvrID:%d,lobby_count:%d", srcSvrID, uid, uMasterSvrID, m_lobbySvrs.size());

	MAP_LOBBY::iterator iter = m_lobbySvrs.begin();
	for (; iter != m_lobbySvrs.end(); ++iter)
	{
		stLobbyServer & server = iter->second;
		LOG_DEBUG("other_lobby_ing - msg_svrid:%d,uid:%d,svrID:%d,uMasterSvrID:%d", srcSvrID, uid, server.svrID, uMasterSvrID);
		if (server.svrID == srcSvrID || server.svrID == uMasterSvrID)
		{
			continue;
		}
		SendProtobufMsg(server.pNetObj, msg, msg_type, CApplication::Instance().GetServerID());
	}
}


void CLobbyMgr::BroadcastPHPMsg2Lobby(const string msg, uint16 type, int destSvrID)
{
	net::msg_broadcast_info transMsg;
	transMsg.set_data(msg);
 	transMsg.set_cmd(type);

	LOG_DEBUG("destSvrID:%d,type:%d,msg:%s", destSvrID, type, msg.c_str());

 	if(destSvrID == 0)
 	{
		MAP_LOBBY::iterator iter = m_lobbySvrs.begin();
		for (; iter != m_lobbySvrs.end(); ++iter)
		{
			stLobbyServer server = iter->second;
			LOG_DEBUG("svrID:%d,svrType:%d,gameType:%d,status:%d net_fd:%d", server.svrID, server.svrType, server.gameType, server.status, server.pNetObj->GetNetfd());
			SendProtobufMsg(server.pNetObj, &transMsg, net::D2L_MSG_BROADCAST, CApplication::Instance().GetServerID());
		}
 	}
 	else
 	{
 		MAP_LOBBY::iterator iter = m_lobbySvrs.find(destSvrID);
 		if(iter == m_lobbySvrs.end())
 		{
 			LOG_ERROR("cant find this lobbySvr:%d", destSvrID);
 			return;
 		}
 		stLobbyServer server = iter->second;
 		SendProtobufMsg(server.pNetObj, &transMsg, net::D2L_MSG_BROADCAST, CApplication::Instance().GetServerID());
 	}
	return;
}

uint32 CLobbyMgr::GetMasterSvrID()
{
	MAP_LOBBY::iterator iter = m_lobbySvrs.begin();
	for(; iter != m_lobbySvrs.end(); ++iter)
	{
		if(CApplication::Instance().call<bool>("CheckIsMasterSvr", iter->first))
		{
			return iter->first;
		}
	}
	return 0;
}

//----------------------------------------------------------------------
int CHandleLobbyMsg::OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head)
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
	HANDLE_SERVER_FUNC(net::LS2D_MSG_REGISTER, handle_msg_lobby_register);
    HANDLE_SERVER_FUNC(net::LS2D_MSG_REPORT_ONLINES, handle_msg_Online_Info);
    HANDLE_SERVER_FUNC(net::L2D_MSG_BROADCAST_OTHER_LOBBY, handle_broadcast_msg);
	HANDLE_SERVER_FUNC(net::LS2D_MSG_CHECK_OTHER_LOBBY_SERVER, handle_check_other_lobby_server);
	default:
        LOG_ERROR("cant handle this cmd[%d]", head->cmd);
		break;		
	}
	return 0;
}


int  CHandleLobbyMsg::handle_msg_lobby_register(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_register_dispatch msg;
	PARSE_MSG_FROM_ARRAY(msg);

	uint32 svrid = msg.svrid();
	uint32 svrType = msg.svr_type();
	uint32 gametype = msg.game_type();

	LOG_ERROR("lobbySvr register svrid[%d] svr_type[%d] gametype[%d]", svrid, svrType, gametype);
	bool ret = CLobbyMgr::Instance().AddServer(pNetObj, svrid, svrType, gametype);
	uint32 registerResult = 1;
	if(!ret)
	{
		registerResult = 0;
		LOG_ERROR("register lobbysvr error");
	}

	net::msg_register_dispatch_rep repMsg;
	repMsg.set_result(registerResult);
	SendProtobufMsg(pNetObj, &repMsg, net::D2LS_MSG_REGISTER_REP, CApplication::Instance().GetServerID());

	return 0;
}

int  CHandleLobbyMsg::handle_msg_Online_Info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_report_onlines msg;
	PARSE_MSG_FROM_ARRAY(msg);

	stLobbyServer * server = CLobbyMgr::Instance().GetServerByNetObj(pNetObj);
	if(server)
	{
		server->palyerNum = msg.player_size();
		server->robotNum = msg.robot_size();
	}
	else
	{
		LOG_ERROR("cant find lobby server by pNetObj:%d", pNetObj->GetUID());
	}

	return 0;
}

int  CHandleLobbyMsg::handle_broadcast_msg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_broadcast_other_lobby msg;
	PARSE_MSG_FROM_ARRAY(msg);

	net::msg_broadcast_info transMsg;
	transMsg.set_data(msg.data());
 	transMsg.set_cmd(msg.cmd());

 	uint32 broadType = msg.type();
	if(broadType == ALLBROADCAST)
	{
		//广播
		CLobbyMgr::Instance().BroadcastInfo2Lobby(&transMsg, net::D2L_MSG_BROADCAST, pNetObj->GetUID(), 0);
		LOG_DEBUG("Trans broadcast Msg:%d", msg.cmd());
	}
	else if(broadType == SINGLEBROADCAST)
	{
		//单播
		uint32 destSvrid = msg.svrid();
		CLobbyMgr::Instance().BroadcastInfo2Lobby(&transMsg, net::D2L_MSG_BROADCAST, pNetObj->GetUID(), destSvrid);
		LOG_DEBUG("Trans broadcast Msg:%d to svr:%d", msg.cmd(), destSvrid);
	}

	return 0;
}

int  CHandleLobbyMsg::handle_check_other_lobby_server(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len)
{
	net::msg_check_other_lobby_server msg;
	PARSE_MSG_FROM_ARRAY(msg);

	net::msg_check_other_lobby_server transMsg;
	transMsg.set_svrid(msg.svrid());
	transMsg.set_uid(msg.uid());

	LOG_DEBUG("exe_lobby_send - svrid:%d,uid:%d", msg.svrid(), msg.uid());

	CLobbyMgr::Instance().CheckOtherLobbyServerLogin(&transMsg, net::D2LS_MSG_CHECK_OTHER_LOBBY_SERVER, msg.svrid(), msg.uid());

	return 0;
}

