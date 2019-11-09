
#include "gobal_event_mgr.h"
#include "stdafx.h"
#include "player.h"
#include "center_log.h"
#include "gobal_robot_mgr.h"

using namespace svrlib;

namespace
{  

};
CGobalEventMgr::CGobalEventMgr()
{
	m_pReportTimer  = NULL;
}
CGobalEventMgr::~CGobalEventMgr()
{

}
bool	CGobalEventMgr::Init()
{
	m_pReportTimer = CApplication::Instance().MallocTimer(this,emTIMER_EVENT_REPORT);
    m_pReportTimer->StartTimer(5*1000,5*1000);// 5秒
	return true;
}
void	CGobalEventMgr::ShutDown()
{
	CApplication::Instance().FreeTimer(m_pReportTimer);  
}
void	CGobalEventMgr::ProcessTime()
{
	static uint64 uProcessTime = 0;

	uint64 uTime	= getSysTime();
	uint64 uTick	= getSystemTick64();
	if(!uProcessTime)
		uProcessTime = uTime;
	if(uTime == uProcessTime)
		return;// 一秒一次

	bool bNewDay = (diffTimeDay(uProcessTime,uTime) != 0);
	if(bNewDay){
		OnNewDay();
	}
	uProcessTime = uTime;
	g_RandGen.Reset(uTick);
	
	CGobalRobotMgr::Instance().OnTimeTick();
}
void	CGobalEventMgr::OnTimer(uint8 eventID)
{
	switch(eventID)
	{
	case emTIMER_EVENT_RECOVER:
		{
			
		}break;
	case emTIMER_EVENT_REPORT:
		{
			ReportInfo2Dispatch();
		} break;
	default:
		break;
	}
}

void	CGobalEventMgr::OnNewDay()
{
	bool bNewWeek 	= false;
	bool bNewMonth	= false;

	// 新的一天
	tm local_time;        
	uint64 uTime	= getTime();
	getLocalTime( &local_time, uTime );   

    CRedisMgr::Instance().ClearSignInDev();
    
	// 跨周        0-6
	if( local_time.tm_wday == 0 )            
   	{
   		bNewWeek = true;
		OnNewWeek();
	}
	// 跨月        1-31
	if( local_time.tm_mday == 1 )		 	
	{
		bNewMonth = true;
		OnNewMonth();
	}    	
}
void	CGobalEventMgr::OnNewWeek()
{
	
}
void	CGobalEventMgr::OnNewMonth()
{
	
}
void 	CGobalEventMgr::SaveAllPlayerAndLoginOut()
{
	vector<CPlayerBase*> vecPlayers;
	CPlayerMgr::Instance().GetAllPlayers(vecPlayers);
	for(uint32 i=0;i<vecPlayers.size();++i)
	{
		CPlayer* pPlayer = (CPlayer*)vecPlayers[i];
        if(!pPlayer->IsRobot())
		{
			LOG_ERROR("OnLoginOut - uid:%d", pPlayer->GetUID());
		    pPlayer->OnLoginOut();
        }
		CPlayerMgr::Instance().RemovePlayer(pPlayer);
		SAFE_DELETE(pPlayer);
	}
	vecPlayers.clear();
	//COUNT_OCCUPY_MEM_SZIE(this);
}
void    CGobalEventMgr::AddSpeak(net::msg_speak_broadcast_rep& msg)
{
    m_speakList.push_back(msg);
    while(m_speakList.size() > 20){
        m_speakList.pop_front();        
    }
	//COUNT_OCCUPY_MEM_SZIE(this);
}
void    CGobalEventMgr::SendSpeakListToPlayer(CPlayer* pPlayer)
{
    net::msg_send_history_speak msg;
    list<net::msg_speak_broadcast_rep>::iterator it = m_speakList.begin();
    for(;it != m_speakList.end();++it){
        net::msg_speak_broadcast_rep* rep = msg.add_msgs();
        *rep = *it;
    }
    pPlayer->SendMsgToClient(&msg,net::S2C_MSG_SEND_HISTORY_SPEAK);
}


void   CGobalEventMgr::ReportInfo2Dispatch()
{
	net::msg_report_onlines msg;
	uint32 players = 0,robots = 0;
    CPlayerMgr::Instance().GetOnlines(players,robots);
    msg.set_player_size(players);
    msg.set_robot_size(robots);

    int svrID = CApplication::Instance().GetServerID();
    int status = CApplication::Instance().GetStatus();
    if(true == CApplication::Instance().call<bool>("CheckIsMasterSvr", CApplication::Instance().GetServerID()))
    {
    	LOG_DEBUG("svr:%d real status:%d",CApplication::Instance().GetServerID(), status);
    	status = emSERVER_STATE_RETIRE;
    }
    CRedisMgr::Instance().WriteSvrStatusInfo(svrID, players, robots, status);
    DispatchMgr::Instance().SendMsg2DispatchSvr(&msg, net::LS2D_MSG_REPORT_ONLINES, svrID);
}

















