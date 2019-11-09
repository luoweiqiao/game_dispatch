#include <data_cfg_mgr.h>
#include "dispatch_mgr.h"
#include "game_server_config.h"
#include "game_define.h"
#include "stdafx.h"

#include "msg_dispatch_handle.h"

using namespace svrlib;
using namespace std;
using namespace Network;
using namespace net;

void  DispatchMgr::OnTimer(uint8 eventID)
{
	ReConnect();
}

bool DispatchMgr::Init()
{
	const stServerCfg& refCfg = CDataCfgMgr::Instance().GetCurSvrCfg();

	vector<stServerCfg> vecDispatchCfg;
	CDataCfgMgr::Instance().GetDispatchSvrsCfg(refCfg.group, vecDispatchCfg);
	if (vecDispatchCfg.empty())
	{
		LOG_ERROR("lobby cfg not find group:%d", refCfg.group);
		return false;
	}

	stServerCfg& dispatchcfg = vecDispatchCfg[0];
	DispatchNetObj* pNetObj = new DispatchNetObj();
	pNetObj->SetUID(dispatchcfg.svrid);
	pNetObj->SetSIP(dispatchcfg.svrlanip);
	pNetObj->SetPort(dispatchcfg.svrlanport);

	//stDispatchServer dispatchSvr;
	m_DispatchSvr.svrID = dispatchcfg.svrid;
	m_DispatchSvr.isReconnecting = false;
	m_DispatchSvr.isRun 	= false;
	m_DispatchSvr.dispatchCfg  = dispatchcfg;
	m_DispatchSvr.pNetObj   = pNetObj;

	SetMsgSinker(new CHandleDispatchMsg());

	m_pTimer = CApplication::Instance().MallocTimer(this, 1);
	m_pTimer->StartTimer(3000, 3000);
	ReConnect();
	return true;
}

void  DispatchMgr::Register(uint16 svrid)
{
	net::msg_register_dispatch msg;
	msg.set_svrid(CApplication::Instance().GetServerID());
	msg.set_svr_type(CDataCfgMgr::Instance().GetCurSvrCfg().svrType);
	msg.set_game_type(CDataCfgMgr::Instance().GetCurSvrCfg().gameType);

	SendMsg2DispatchSvr(&msg, net::LS2D_MSG_REGISTER, svrid);
	LOG_DEBUG("register to dispatch server:%d-->svr_type:%d-->game_type:%d   destSvrID:%d", msg.svrid(), msg.svr_type(), msg.game_type(), svrid);
}

void   DispatchMgr::RegisterRep(uint16 svrid, bool rep)
{
	LOG_DEBUG("register recv dispatch svrid:%d ret:%d", svrid, rep);
    m_DispatchSvr.isRun = rep;
}

bool  DispatchMgr::SendMsg2DispatchSvr(const google::protobuf::Message* msg, uint16 msg_type, uint16 svrid)
{
	return SendProtobufMsg(m_DispatchSvr.pNetObj, msg, msg_type, static_cast<uint32>(svrid));
}


void  DispatchMgr::OnCloseClient(DispatchNetObj* pNetObj)
{
	LOG_ERROR("Dispatch OnClose:%d", pNetObj->GetUID());
    if (pNetObj != m_DispatchSvr.pNetObj)
    {
        LOG_ERROR("Dispatch:%d cfg not exist", pNetObj->GetUID());
        return;
    }    
	m_DispatchSvr.isRun = false;
}


void  DispatchMgr::ReConnect()
{
	if (m_DispatchSvr.isRun || m_DispatchSvr.isReconnecting)
	{
		return;
	}

	if (m_DispatchSvr.pNetObj->Connect() == 0)
	{
		Register(m_DispatchSvr.svrID);
		m_DispatchSvr.isRun = true;

		LOG_DEBUG("connect dispatch success ip:%s port:%d fd:%d", m_DispatchSvr.pNetObj->GetSIP().c_str(), m_DispatchSvr.dispatchCfg.svrlanport, m_DispatchSvr.pNetObj->GetNetfd());
	}
	else
	{
		LOG_ERROR("connect dispatch failed ip:%s port:%d", m_DispatchSvr.pNetObj->GetSIP().c_str(), m_DispatchSvr.dispatchCfg.svrlanport);
	}
}

