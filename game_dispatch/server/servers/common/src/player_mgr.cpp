

#include "player_mgr.h"
#include "svrlib.h"

using namespace svrlib;
using namespace std;
using namespace Network;

CPlayerMgr::CPlayerMgr()
{
    m_pFlushTimer   = NULL;
    m_pRecoverTimer = NULL;
}
CPlayerMgr::~CPlayerMgr()
{
    m_pFlushTimer   = NULL;
    m_pRecoverTimer = NULL;
}
void  CPlayerMgr::OnTimer(uint8 eventID)
{
    switch(eventID)
    {
    case 1:
        {
            OnTimeTick();
        }break;
    case 2:
        {
            CheckRecoverPlayer();
        }break;
    default:
        break;
    }
}
bool  CPlayerMgr::Init()
{
    m_pFlushTimer = CApplication::Instance().MallocTimer(this,1);
    m_pFlushTimer->StartTimer(1000,1000); // 1秒
    m_pRecoverTimer = CApplication::Instance().MallocTimer(this,2);
    m_pRecoverTimer->StartTimer(1000,1000);

    return true;
}
void  CPlayerMgr::ShutDown()
{
    CApplication::Instance().FreeTimer(m_pFlushTimer);
}
void  CPlayerMgr::OnTimeTick()
{
    static uint64 uProcessTime = 0;
    uint64 uTime	= getSysTime();
    if(!uProcessTime)
        uProcessTime = uTime;

    bool bNewDay = (diffTimeDay(uProcessTime, uTime) != 0);
    MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
    for(;Iter != m_mpPlayers.end();++Iter)
    {
        CPlayerBase* pPlayer = Iter->second;
        pPlayer->OnTimeTick(uTime,bNewDay);
    }
    uProcessTime = uTime;
}
bool  CPlayerMgr::IsOnline(uint32 uid)
{
    MAPPLAYERS::iterator Iter = m_mpPlayers.find(uid);
    if(Iter != m_mpPlayers.end()){
        return true;       
    }
    return false;
}
CPlayerBase*  CPlayerMgr::GetPlayer(uint32 dwUin)
{
    MAPPLAYERS::iterator Iter = m_mpPlayers.find(dwUin);
    if(Iter != m_mpPlayers.end())
    {
        return  Iter->second;
    }
    return NULL;
}
bool  CPlayerMgr::AddPlayer(CPlayerBase* pPlayer)
{
    if(IsOnline(pPlayer->GetUID()))
    {
    	LOG_ERROR("addplayer is error - uid:%d", pPlayer->GetUID());
        return false;
    }
    m_mpPlayers.insert(make_pair(pPlayer->GetUID(),pPlayer));	
    return true;
}

bool  CPlayerMgr::RemovePlayer(CPlayerBase* pPlayer)
{
	m_mpPlayers.erase(pPlayer->GetUID());	
    return true;
}
void  CPlayerMgr::SendMsgToAll(const google::protobuf::Message* msg,uint16 msg_type)
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
    for(;Iter != m_mpPlayers.end();++Iter)
    {
        CPlayerBase* pPlayer = Iter->second;
		if(pPlayer->IsPlaying())
		{
            pPlayer->SendMsgToClient(msg,msg_type);
		}		
    }
}
void  CPlayerMgr::SendMsgToPlayer(const google::protobuf::Message* msg,uint16 msg_type,uint32 uid)
{
	CPlayerBase* pPlayer = GetPlayer(uid);
	if(pPlayer != NULL)
	{
		pPlayer->SendMsgToClient(msg,msg_type);
		return;
	}
}
uint32	CPlayerMgr::GetOnlines(uint32& players,uint32& robots)
{
    players = 0;
    robots  = 0;
    MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
    for(;Iter != m_mpPlayers.end();++Iter)
    {
        CPlayerBase* pPlayer = Iter->second;
        if(pPlayer->GetPlayerType() == PLAYER_TYPE_ONLINE){
            players++;
        }else{
            robots++;
        }
    }
    return m_mpPlayers.size();
}
void  CPlayerMgr::GetAllPlayers(vector<CPlayerBase*>& refVec) 
{
    MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
    for(;Iter != m_mpPlayers.end();++Iter)
    {
        CPlayerBase* pPlayer = Iter->second;
        refVec.push_back(pPlayer);		
    }
}
void  CPlayerMgr::CheckRecoverPlayer()
{
	int iRecoverCount = 0;
    vector<CPlayerBase*> vecPlayers;
    CPlayerMgr::Instance().GetAllPlayers(vecPlayers);
    for(uint32 i=0;i<vecPlayers.size();++i)
    {
        CPlayerBase* pPlayer = vecPlayers[i];
        if(pPlayer->NeedRecover())
        {
            LOG_DEBUG("recover player - uid:%d",pPlayer->GetUID());
			iRecoverCount++;
            pPlayer->OnLoginOut();
            RemovePlayer(pPlayer);
            SAFE_DELETE(pPlayer);
        }
    }
    vecPlayers.clear();

	uint32 uPlayers = 0, uRobots = 0;
	uint32 uPlayerCount = CPlayerMgr::Instance().GetOnlines(uPlayers, uRobots);

	if (iRecoverCount > 0)
	{
		LOG_DEBUG("palyer occupy men - iRecoverCount:%d,uPlayerCount:%d,uPlayers:%d,uRobots:%d", iRecoverCount, uPlayerCount, uPlayers, uRobots);
	}
}

bool  CPlayerMgr::SendVipBroadCast()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendVipBroadCast();
		}
	}
	return true;
}


bool  CPlayerMgr::SendVipProxyRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendVipProxyRecharge();
		}
	}
	return true;
}


bool  CPlayerMgr::SendVipAliAccRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying() && pPlayer->IsRobot() == false)
		{
			pPlayer->SendVipAliAccRecharge();
		}
	}
	return true;
}


bool  CPlayerMgr::SendUnionPayRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendUnionPayRecharge();
		}
	}
	return true;
}

bool  CPlayerMgr::SendWeChatPayRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendWeChatPayRecharge();
		}
	}
	return true;
}

bool  CPlayerMgr::SendAliPayRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendAliPayRecharge();
		}
	}
	return true;
}
bool  CPlayerMgr::SendOtherPayRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendOtherPayRecharge();
		}
	}
	return true;
}
bool  CPlayerMgr::SendQQPayRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendQQPayRecharge();
		}
	}
	return true;
}
bool  CPlayerMgr::SendWeChatScanPayRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendWeChatScanPayRecharge();
		}
	}
	return true;
}
bool  CPlayerMgr::SendJDPayRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendJDPayRecharge();
		}
	}
	return true;
}

bool  CPlayerMgr::SendApplePayRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying())
		{
			pPlayer->SendApplePayRecharge();
		}
	}
	return true;
}

bool  CPlayerMgr::SendLargeAliPayRecharge()
{
	MAPPLAYERS::iterator Iter = m_mpPlayers.begin();
	for (; Iter != m_mpPlayers.end(); ++Iter)
	{
		CPlayerBase* pPlayer = Iter->second;
		if (pPlayer->IsPlaying() && pPlayer->IsRobot() == false)
		{
			pPlayer->SendLargeAliPayRecharge();
		}
	}
	return true;
}

void CPlayerMgr::SendExclusiveFlashRecharge() {
	for (stl_hash_map<uint32, CPlayerBase*>::iterator iter = m_mpPlayers.begin(); iter != m_mpPlayers.end(); ++iter)
		iter->second->SendExclusiveFlashRecharge();
}

// 设置其他大厅玩家的数据
void CPlayerMgr::SetOtherLobbyPlayerData(uint32 uid, const string& name) {
	unordered_map<uint32, SPlayerData>::iterator iter = m_umpOtherLobbyPlayerData.find(uid);
	if (iter == m_umpOtherLobbyPlayerData.end()) {
		SPlayerData s;
		s.name = name;
		m_umpOtherLobbyPlayerData.insert(make_pair(uid, s));
		LOG_DEBUG("1 uid:%d,name:%s,size:%lld", uid, name.c_str(), m_umpOtherLobbyPlayerData.size());
		return;
	}
	iter->second.name = name;
	LOG_DEBUG("2 uid:%d,name:%s,size:%lld", uid, name.c_str(), m_umpOtherLobbyPlayerData.size());
}

// 获取其他大厅服登陆的玩家数据
SPlayerData* CPlayerMgr::GetOtherLobbyPlayerData(uint32 uid) {
	unordered_map<uint32, SPlayerData>::iterator iter = m_umpOtherLobbyPlayerData.find(uid);
	if (iter == m_umpOtherLobbyPlayerData.end())
		return NULL;
	return &iter->second;
}