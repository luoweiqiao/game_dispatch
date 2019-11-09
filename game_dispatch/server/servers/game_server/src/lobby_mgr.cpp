#include <data_cfg_mgr.h>
#include "lobby_mgr.h"
#include "game_server_config.h"
#include "game_define.h"
#include "stdafx.h"

#include "msg_lobby_handle.h"

using namespace svrlib;
using namespace std;
using namespace Network;
using namespace net;

void  CLobbyMgr::OnTimer(uint8 eventID)
{
	ReConnect();
}
bool	CLobbyMgr::Init(IProtobufClientMsgRecvSink* pMsgRecvSink)
{
	const stServerCfg& refCfg = CDataCfgMgr::Instance().GetCurSvrCfg();

	vector<stServerCfg> vecLobbyCfg;
	CDataCfgMgr::Instance().GetLobbySvrsCfg(refCfg.group,vecLobbyCfg);
	if(vecLobbyCfg.empty()){
		LOG_ERROR("大厅服务器配置未找到:group:%d",refCfg.group);
		return false;
	}
	for(uint32 i=0;i<vecLobbyCfg.size();++i)
    {
    	stServerCfg& lobbycfg = vecLobbyCfg[i];
    	CLobbyNetObj* pNetObj = new CLobbyNetObj();
    	pNetObj->SetUID(lobbycfg.svrid);
        pNetObj->SetSIP(lobbycfg.svrlanip);
        pNetObj->SetPort(lobbycfg.svrlanport);

        stLobbyServer lobbySvr;
    	lobbySvr.svrID = lobbycfg.svrid;
    	lobbySvr.isReconnecting = false;
    	lobbySvr.isRun 	= false;
    	lobbySvr.lobbyCfg  = lobbycfg;
    	lobbySvr.pNetObj   = pNetObj;    

		LOG_DEBUG("lobby_config_connect - svrid:%d,svrlanip:%s,svrlanport:%d", lobbycfg.svrid, lobbycfg.svrlanip.c_str(), lobbycfg.svrlanport);

        m_lobbySvrs.insert(make_pair(lobbySvr.svrID, lobbySvr));
    }             	

	SetMsgSinker(pMsgRecvSink);

    m_pTimer = CApplication::Instance().MallocTimer(this,1);
    m_pTimer->StartTimer(3000,3000);
	ReConnect();
	return true;
}
void	CLobbyMgr::Register(uint16 svrid)
{
    net::msg_register_svr msg;
    msg.set_svrid(CApplication::Instance().GetServerID());
    msg.set_game_type(CDataCfgMgr::Instance().GetCurSvrCfg().gameType);
    msg.set_game_subtype(CDataCfgMgr::Instance().GetCurSvrCfg().gameSubType);
    msg.set_robot(CDataCfgMgr::Instance().GetCurSvrCfg().openRobot);

    //SendMsg2Client(&msg,net::S2L_MSG_REGISTER,0);
	SendMsg2Lobby(&msg, net::S2L_MSG_REGISTER, svrid);
	LOG_DEBUG("注册游戏服务器:svrid:%d,game_type:%d,robot:%d",msg.svrid(),msg.game_type(), msg.robot());
}
bool 	CLobbyMgr::SendMsg2Client(const google::protobuf::Message* msg,uint16 msg_type,uint32 uin)
{
	CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uin);
	if(pPlayer)
	{
		NetworkObject* pNetObj = GetLobbySvrBySvrID(pPlayer->GetBelongLobbyID())->pNetObj;
		if (!pNetObj)
		{
			LOG_ERROR("Cant get player:%d belong lobby:%d netobj", pPlayer->GetUID(), pPlayer->GetBelongLobbyID());
			return false;
		}
		else
		{
			return SendProtobufMsg(pNetObj, msg, msg_type, uin);
		}
	}
	else
	{
		int lobbySvrID = CRedisMgr::Instance().GetPlayerLobbySvrID(uin);
		stLobbyServer * pSvr = GetLobbySvrBySvrID(lobbySvrID);
		if(pSvr)
		{
			return SendProtobufMsg(pSvr->pNetObj, msg, msg_type, uin);
		}
		else
		{
			LOG_ERROR("player not exist:%d",uin);
			return false;
		}
	}
	//return SendProtobufMsg(m_lobbySvr.pNetObj,msg,msg_type,uin);
}

void	CLobbyMgr::OnCloseClient(CLobbyNetObj* pNetObj)
{	
	LOG_ERROR("lobby OnClose:%d",pNetObj->GetUID());
	stLobbyServer* pSvr = GetLobbySvrBySvrID(pNetObj->GetUID());
	if (pSvr == NULL)
    {
        LOG_ERROR("lobby:%d cfg not exist", pNetObj->GetUID());
        return;
    }    
	pSvr->isRun = false;
}

void	CLobbyMgr::ReConnect()
{
	MAP_LOBBY::iterator it = m_lobbySvrs.begin();
    for(;it != m_lobbySvrs.end();++it)
    {
        stLobbyServer* pSvr = &it->second;
    	if(pSvr->isRun || pSvr->isReconnecting)
        {
    		continue;
    	}
    	
        if (pSvr->pNetObj->Connect() == 0)
        {
            Register(pSvr->svrID);
            pSvr->isRun = true;

            LOG_DEBUG("connect lobby success ip:%s port:%d fd:%d", pSvr->pNetObj->GetSIP().c_str(), pSvr->lobbyCfg.svrlanport, pSvr->pNetObj->GetNetfd());
        }        
        else
        {
            LOG_ERROR("connect lobby failed ip:%s port:%d", pSvr->pNetObj->GetSIP().c_str(), pSvr->lobbyCfg.svrlanport);
        }
    }
}
// 请求大厅修改数值
void	CLobbyMgr::NotifyLobbyChangeAccValue(uint32 uid,int32 operType,int32 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safeCoin,const string& chessid)
{
	net::msg_notify_change_account_data msg;
	msg.set_uid(uid);
	msg.set_diamond(diamond);
	msg.set_coin(coin);
	msg.set_ingot(ingot);
	msg.set_score(score);
	msg.set_cvalue(cvalue);
	msg.set_safe_coin(safeCoin);
	msg.set_oper_type(operType);
	msg.set_sub_type(subType);
	msg.set_chessid(chessid);

	SendMsg2Client(&msg,S2L_MSG_NOTIFY_CHANGE_ACCOUNT_DATA,uid);
}

void	CLobbyMgr::UpDateLobbyChangeAccValue(uint32 uid, int32 operType, int32 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safeCoin, const string& chessid)
{
	net::msg_update_lobby_change_account_data msg;
	msg.set_uid(uid);
	msg.set_diamond(diamond);
	msg.set_coin(coin);
	msg.set_ingot(ingot);
	msg.set_score(score);
	msg.set_cvalue(cvalue);
	msg.set_safe_coin(safeCoin);
	msg.set_oper_type(operType);
	msg.set_sub_type(subType);
	msg.set_chessid(chessid);

	SendMsg2Client(&msg, S2L_MSG_UPDATE_LOBBY_CHANGE_ACCOUNT_DATA, uid);
}

void	CLobbyMgr::NotifyUpDateLobbyChangeAccValue(uint32 uid, int32 operType, int32 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safeCoin, const string& chessid)
{
	net::msg_update_lobby_change_account_data msg;
	msg.set_uid(uid);
	msg.set_diamond(diamond);
	msg.set_coin(coin);
	msg.set_ingot(ingot);
	msg.set_score(score);
	msg.set_cvalue(cvalue);
	msg.set_safe_coin(safeCoin);
	msg.set_oper_type(operType);
	msg.set_sub_type(subType);
	msg.set_chessid(chessid);

	SendMsg2Client(&msg, S2L_MSG_NOTIFY_UPDATE_LOBBY_CHANGE_ACCOUNT_DATA, uid);
}

void    CLobbyMgr::RegisterRep(uint16 svrid, bool rep)
{    
    LOG_DEBUG("register recv lobby_svrid:%d ret:%d", svrid, rep);
    stLobbyServer* pSvr = GetLobbySvrBySvrID(svrid);
    if(!pSvr)
    {
    	LOG_ERROR("cant find this svr:%d",svrid);
    	return;
    }
    pSvr->isRun = rep;
}

bool    CLobbyMgr::SendMsg2Lobby(const google::protobuf::Message* msg,uint16 msg_type,uint16 svrid)
{
    stLobbyServer* pSvr = GetLobbySvrBySvrID(svrid);
    if(pSvr == NULL)
        return false;
    return SendProtobufMsg(pSvr->pNetObj,msg,msg_type,static_cast<uint32>(pSvr->svrID));         
}

void    CLobbyMgr::SendMsg2AllLobby(const google::protobuf::Message* msg, uint16 msg_type)
{
	MAP_LOBBY::iterator iter = m_lobbySvrs.begin();
	for(; iter != m_lobbySvrs.end(); ++iter)
	{
		stLobbyServer svr = iter->second;
		SendProtobufMsg(svr.pNetObj, msg, msg_type, static_cast<uint32>(svr.svrID));
	}
}

void  CLobbyMgr::ConnectNewLobby(uint32 svrid)
{
	MAP_LOBBY::iterator iter = m_lobbySvrs.find(svrid);
	if(iter != m_lobbySvrs.end())
	{
		LOG_ERROR("already connected lobby:%d", svrid);
		return;
	}

	CDataCfgMgr::Instance().Reload();
	const stServerCfg * lobbycfg = CDataCfgMgr::Instance().GetServerCfg(svrid);
	if(NULL == lobbycfg)
	{
		LOG_ERROR("Cant get lobby:%d cfg !!!", svrid);
		return;
	}
	CLobbyNetObj* pNetObj = new CLobbyNetObj();
    pNetObj->SetUID(lobbycfg->svrid);
    pNetObj->SetSIP(lobbycfg->svrlanip);
    pNetObj->SetPort(lobbycfg->svrlanport);

	stLobbyServer lobbySvr;
	lobbySvr.svrID = lobbycfg->svrid;
	lobbySvr.pNetObj = pNetObj;
	lobbySvr.lobbyCfg = *lobbycfg;
	lobbySvr.isReconnecting = false;
    lobbySvr.isRun 	= false;

    m_lobbySvrs.insert(make_pair(lobbySvr.svrID, lobbySvr));
    ReConnect();
	
}

void	CLobbyMgr::NotifyLobbyRetired(uint32 svrid)
{
	stLobbyServer* pSvr = GetLobbySvrBySvrID(svrid);
    if(!pSvr)
    {
    	LOG_ERROR("cant find this svr:%d", svrid);
    	return ;
    }

    if(pSvr->isRun)
    {
    	LOG_ERROR("the lobby:%d is running", svrid);
    	return ;
    }

    MAP_LOBBY::iterator iter = m_lobbySvrs.find(svrid);
	if(iter == m_lobbySvrs.end())
	{
		LOG_ERROR("cant find lobby:%d", svrid);
		return;
	}

	LOG_ERROR("lobby:%d retired", svrid);
}

stLobbyServer*  CLobbyMgr::GetLobbySvrBySvrID(uint16 svrid)
{
   return m_lobbySvrs.find_(svrid);    
}

stLobbyServer * CLobbyMgr::GetLobbySvrBypNetObj(NetworkObject* pNetObj)
{
	MAP_LOBBY::iterator iter = m_lobbySvrs.begin();
	for(; iter != m_lobbySvrs.end(); ++iter)
	{
		if(iter->second.pNetObj == pNetObj)
		{
			return &iter->second;
		}
	}

	return NULL;
}