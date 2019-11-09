

#include "gobal_event_mgr.h"
#include "stdafx.h"
#include "game_room_mgr.h"
#include "center_log.h"
#include "robot_mgr.h"

#include "game_net_mgr.h"
#include "lobby_mgr.h"
#include "svrlib.h"
#include <iostream>
#include "game_room_mgr.h"
#include "robot_mgr.h"
#include "game_server_config.h"
#include "utility/timeFunction.h"
#include "data_cfg_mgr.h"
#include "game_room.h"
#include "center_log.h"
#include "robot_mgr.h"
#include "dispatch_mgr.h"

using namespace svrlib;

namespace
{  

};
CGameSvrEventMgr::CGameSvrEventMgr()
{
	m_pReportTimer  = NULL;
	m_pRetireTimer  = NULL;
}
CGameSvrEventMgr::~CGameSvrEventMgr()
{

}
bool	CGameSvrEventMgr::Init()
{
    m_pReportTimer = CApplication::Instance().MallocTimer(this,emTIMER_EVENT_REPORT);
    m_pReportTimer->StartTimer(5*1000,5*1000);// 5秒

	return true;
}
void	CGameSvrEventMgr::ShutDown()
{
	CApplication::Instance().FreeTimer(m_pReportTimer);
	CApplication::Instance().FreeTimer(m_pRetireTimer);
}
void	CGameSvrEventMgr::ProcessTime()
{
	static uint64 uProcessTime = 0;

	uint64 uTime	= getSysTime();
	uint64 uTick	= getSystemTick64();
	if(!uProcessTime)
		uProcessTime = uTime;
	if(uTime == uProcessTime)
		return;
	bool bNewDay = (diffTimeDay(uProcessTime, uTime) != 0);
	if(bNewDay){
		//OnNewDay();
		
	}
	g_RandGen.Reset(uTick);
	CRobotMgr::Instance().OnTimeTick();

	uProcessTime = uTime;
}
void	CGameSvrEventMgr::OnTimer(uint8 eventID)
{
	switch(eventID)
	{
	case emTIMER_EVENT_REPORT:
		{
            ReportInfo2Lobby();   
        	ReportInfo2Dispatch();       
		}break;
	case emTIMER_EVENT_RETIRE:
		{
			DoRetire();
		}break; 
	default:
		break;
	}
}
// 通用初始化
bool    CGameSvrEventMgr::GameServerInit()    
{
	// db
	if(CDBMysqlMgr::Instance().Init(GameServerConfig::Instance().DBConf) == false)
	{
		LOG_ERROR("init mysqlmgr fail ");
		return false;
	}
	if(CDataCfgMgr::Instance().Init() == false) {
		LOG_ERROR("init datamgr fail ");
		return false;
	}
	if(!CGameNetMgr::Instance().Init())
	{
		LOG_ERROR("初始化网络失败");
		return false;
	}
	if(!CRedisMgr::Instance().Init(GameServerConfig::Instance().redisConf[0]))
    {
    	LOG_ERROR("redis初始化失败");
        return false;
    }	
	if(!CPlayerMgr::Instance().Init())
	{
		LOG_ERROR("playermgr init fail");
		return false;			
	} 
    if(!this->Init())
    {
        LOG_ERROR("global mgr init fail");
        return false;
    }
	if(!CCenterLogMgr::Instance().Init(CApplication::Instance().GetServerID())){
		LOG_ERROR("初始化日志Log失败");
		return false;
	}    
    if(CGameRoomMgr::Instance().Init() == false)
	{
		LOG_ERROR("初始化房间信息失败");
		return false;
	}
	if(CRobotMgr::Instance().Init() == false)
	{
		LOG_ERROR("初始化机器人管理器失败");
		return false;
	}    
	if(DispatchMgr::Instance().Init() == false)
	{
		LOG_ERROR("init dispatcher mgr failed");
		return false;
	}

    CRedisMgr::Instance().ClearPlayerOnlineSvrID(CApplication::Instance().GetServerID());
	CDBMysqlMgr::Instance().UpdateServerInfo();
    
    return true;
}
// 通用关闭
bool    CGameSvrEventMgr::GameServerShutDown() 
{
	CRobotMgr::Instance().ShutDown();
	CGameRoomMgr::Instance().ShutDown();
	CPlayerMgr::Instance().ShutDown();
	CRedisMgr::Instance().ShutDown();
	CGameNetMgr::Instance().ShutDown();

    CDBMysqlMgr::Instance().ShutDown();
    
    return true;
}
// 通用TICK
bool    CGameSvrEventMgr::GameServerTick()
{    
    CGameNetMgr::Instance().Update();	
    this->ProcessTime();
    CDBMysqlMgr::Instance().ProcessDBEvent(); 
    
    return true;       
}
void    CGameSvrEventMgr::ReportInfo2Lobby()
{
    net::msg_report_svr_info info;
    uint32 players = 0,robots = 0;
    CPlayerMgr::Instance().GetOnlines(players,robots);
    info.set_onlines(players);
    info.set_robots(robots);
    
    //CLobbyMgr::Instance().SendMsg2Client(&info,net::S2L_MSG_REPORT,0);
    CLobbyMgr::Instance().SendMsg2AllLobby(&info,net::S2L_MSG_REPORT);
}

void   CGameSvrEventMgr::StartRetire()
{
	if(CDataCfgMgr::Instance().GetCurSvrCfg().gameSubType == emSVR_TYPE_PRIVATE)
	{
		LOG_ERROR("private server retire not need do this");
		return;
	}
	if(CApplication::Instance().GetStatus() != emSERVER_STATE_RETIRE)
	{
		LOG_ERROR("server status not retire");
		return;
	}

	LOG_DEBUG("start retire gameserver");
	m_pRetireTimer = CApplication::Instance().MallocTimer(this,emTIMER_EVENT_RETIRE);
	m_pRetireTimer->StartTimer(10000, 10000); //5s

	// 踢出桌子上未开始游戏的玩家
	vector<CGameRoom*> vecRooms;
	CGameRoomMgr::Instance().GetAllRoomList(vecRooms);
	for(uint32 i = 0; i < vecRooms.size(); ++i)
	{
		CGameRoom * room = vecRooms[i];
		room->RetireAllTable();
	}

	// 踢出未在桌子上的机器人
	CRobotMgr::Instance().RetireFreeRobot();
}

void   CGameSvrEventMgr::DoRetire()
{	
	static uint32 retireCount = 0;
	if(retireCount > 90)
	{
		//退休15分钟后 强制自杀该进程
		LOG_ERROR("retire count more than 900s");
		exit(0);
	}
	uint32 playerNum = 0;
	uint32 robotNum = 0;
	CPlayerMgr::Instance().GetOnlines(playerNum, robotNum);
	if(playerNum == 0 && robotNum == 0)
	{
		LOG_ERROR("all palyers has LogOut");
		exit(0);
	}
	else
	{
		if(retireCount > 60)
		{
			//退休10分钟 还未退出的玩家
			vector<CPlayerBase*> Allplayers;
			CPlayerMgr::Instance().GetAllPlayers(Allplayers);
			for (uint32 i = 0; i < Allplayers.size(); ++i)
			{
				CPlayerBase * player =  Allplayers[i];
				LOG_ERROR("retire %dS not logout user:%d status:%d", retireCount, player->GetUID(), player->GetPlayerState());
			}
		}
	}
	retireCount++;
}

void   CGameSvrEventMgr::ReportInfo2Dispatch()
{
	net::msg_report_onlines msg;
	uint32 players = 0,robots = 0;
    CPlayerMgr::Instance().GetOnlines(players,robots);
    msg.set_player_size(players);
    msg.set_robot_size(robots);

    int svrID = CApplication::Instance().GetServerID();
    int status = CApplication::Instance().GetStatus();
    CRedisMgr::Instance().WriteSvrStatusInfo(svrID, players, robots, status);
    DispatchMgr::Instance().SendMsg2DispatchSvr(&msg, net::LS2D_MSG_REPORT_ONLINES, svrID);
}



















