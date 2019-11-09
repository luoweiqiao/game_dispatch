//
// Created by toney on 16/5/18.
//

#include "robot_mgr.h"
#include "stdafx.h"
#include "game_room.h"
#include "game_room_mgr.h"
#include "data_cfg_mgr.h"
#include "common_logic.h"

namespace
{
    static const uint32 s_FreeRobotNum = 10;
    static const int32  s_RoomMaxRobot = 100;
};

CRobotMgr::CRobotMgr()
{
    m_isOpenRobot = false;
}
CRobotMgr::~CRobotMgr()
{
}
bool CRobotMgr::Init()
{
    m_DispatchCool.beginCooling(3000);
    m_isOpenRobot = CDataCfgMgr::Instance().GetCurSvrCfg().openRobot == 1 ? true : false;
    return true;
}
void CRobotMgr::ShutDown()
{

}
void CRobotMgr::OnTimeTick()
{
    CheckRobotBackPool();
    CheckRobotBackLobby();
    if(m_DispatchCool.isTimeOut()){
        DispatchRobotToTable();
        m_DispatchCool.beginCooling(10000);
    }
}
CGameRobot* CRobotMgr::GetRobot(uint32 uid)
{
    MAP_ROBOTS::iterator it = m_mpRobots.find(uid);
	if (it != m_mpRobots.end())
	{
		return it->second;
	}
    return NULL;
}
// 请求分配一个机器人
bool CRobotMgr::RequestOneRobot(CGameTable* pTable)
{
    if(m_mpRobots.empty() || !m_isOpenRobot)
        return false;
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
        return false;
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_RETIRE)
        return false;

    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    uint32 startPos = g_RandGen.RandUInt()%m_mpRobots.size();

	LOG_DEBUG("startPos_robot_start - startPos:%d,m_mpRobots.size:%d,roomid:%d,tableid:%d",
		startPos, m_mpRobots.size(), pTable->GetHostRoom()->GetRoomID(), pTable->GetTableID());
	int index = 0;
    for(;it != m_mpRobots.end();++it)
    {
        //if(startPos > 0){
        //    startPos--;
        //    continue;
        //}
        CGameRobot* pRobot = it->second;
		index++;
		LOG_DEBUG("get a robot start - uid:%d,index:%d,roomid:%d,tableid:%d", pRobot->GetUID(), index, pTable->GetHostRoom()->GetRoomID(), pTable->GetTableID());

        if(pRobot->GetRoom() != NULL || pRobot->GetTable() != NULL)
		{
			LOG_DEBUG("get a robot is have table - uid:%d,index:%d,roomid:%d,tableid:%d", pRobot->GetUID(), index, pTable->GetHostRoom()->GetRoomID(), pTable->GetTableID());

            continue;
        }
		bool bIsInRoom = pTable->GetHostRoom()->CanEnterRoom(pRobot);
		bool bIsInTable = false;
		if (bIsInRoom)
		{
			bIsInTable = pTable->CanEnterTable(pRobot);
		}
        if(!bIsInRoom || !bIsInTable)
		{
			LOG_DEBUG("get a robot donot in table - uid:%d,index:%d,roomid:%d,tableid:%d,bIsInRoom:%d,bIsInTable:%d",
				pRobot->GetUID(), index, pTable->GetHostRoom()->GetRoomID(), pTable->GetTableID(), bIsInRoom, bIsInTable);

            continue;
        }
        pTable->GetHostRoom()->EnterRoom(pRobot);
		if (pTable->EnterTable(pRobot))
		{
			LOG_DEBUG("get a robot succ - uid:%d,index:%d,roomid:%d,tableid:%d", pRobot->GetUID(), index, pTable->GetHostRoom()->GetRoomID(), pTable->GetTableID());

			return true;
		}
    }
    return false;
}

bool CRobotMgr::RequestXRobot(int count, CGameTable* pTable)
{
	if (m_mpRobots.empty() || !m_isOpenRobot || count <= 0 || pTable == NULL)
	{
		return false;
	}

	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		return false;
	}

	MAP_ROBOTS::iterator it = m_mpRobots.begin();
	int robotIndex = 0;
	for (; it != m_mpRobots.end(); ++it)
	{
		robotIndex++;
		CGameRobot* pRobot = it->second;
		if (pRobot == NULL)
		{
			continue;
		}
		if (pRobot->GetRoom() != NULL || pRobot->GetTable() != NULL)
		{
			continue;
		}
		if (!pTable->GetHostRoom()->CanEnterRoom(pRobot) || !pTable->CanEnterTable(pRobot))
		{
			continue;
		}
		//LOG_DEBUG("get a robot - uid:%d", pRobot->GetUID());
		bool bEnterRoom = pTable->GetHostRoom()->EnterRoom(pRobot);
		bool bEnterTable = false;
		if (bEnterRoom)
		{
			bEnterTable = pTable->EnterTable(pRobot);
		}
		LOG_DEBUG("get_x_robot - robotIndex:%d,bEnterRoom:%d,bEnterTable:%d,m_mpRobots.size:%d,count:%d,roomid:%d,tableid:%d,ruid:%d",
			robotIndex, bEnterRoom, bEnterTable, m_mpRobots.size(), count, pTable->GetRoomID(), pTable->GetTableID(), pRobot->GetUID());

		if (bEnterTable)
		{
			count--;
			if (count <= 0)
			{
				return true;
			}
		}
	}

	LOG_DEBUG("get_xe_robot - robotIndex:%d,m_mpRobots.size:%d,count:%d,roomid:%d,tableid:%d",
		robotIndex, m_mpRobots.size(), count, pTable->GetRoomID(), pTable->GetTableID());

	return false;
}


bool    CRobotMgr::AddRobot(CGameRobot* pRobot)
{
	if (GetRobot(pRobot->GetUID()) != NULL)
	{
		return false;
	}

    m_mpRobots.insert(make_pair(pRobot->GetUID(),pRobot));
    LOG_DEBUG("add robot uid:%d,m_mpRobots.size:%d",pRobot->GetUID(),m_mpRobots.size());
    CPlayerMgr::Instance().AddPlayer(pRobot);

    return true;
}
bool    CRobotMgr::RemoveRobot(CGameRobot* pRobot)
{
    uint32 uid = pRobot->GetUID();
    pRobot->OnLoginOut();
    m_mpRobots.erase(pRobot->GetUID());
    CPlayerMgr::Instance().RemovePlayer(pRobot);
    SAFE_DELETE(pRobot);

    LOG_DEBUG("remove robot - uid:%d,m_mpRobots.size:%d",uid,m_mpRobots.size());
    return true;
}
// 空闲机器人数量
uint32  CRobotMgr::GetFreeRobotNum()
{
    uint32 num = 0;
    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    for(;it != m_mpRobots.end();++it)
    {
        CGameRobot* pRobot = it->second;
        if(pRobot->GetRoom() != NULL || pRobot->GetTable() != NULL){
            continue;
        }
        num++;
    }
    return num;
}
void    CRobotMgr::GetFreeRobot(vector<CGameRobot*>& robots)
{
    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    for(;it != m_mpRobots.end();++it)
    {
        CGameRobot* pRobot = it->second;
        if(pRobot->GetRoom() != NULL || pRobot->GetTable() != NULL){
            continue;
        }
		//LOG_DEBUG("push free robot - uid:%d,robots.size:%d", pRobot->GetUID(), robots.size());
        robots.push_back(pRobot);
    }
}
// 派发机器人占桌
void    CRobotMgr::DispatchRobotToTable()
{
	if (m_isOpenRobot == false || CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR || CApplication::Instance().GetStatus() == emSERVER_STATE_RETIRE)
	{
		return;
	}
	uint16 gameType = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
	if (gameType == net::GAME_CATE_TWO_PEOPLE_MAJIANG || gameType == net::GAME_CATE_LAND)
	{
		return;
	}
    vector<CGameRobot*> robots;
    GetFreeRobot(robots);
    uint32 disNum = m_mpRobots.size()/4;

    if(!CCommonLogic::IsBaiRenGame(gameType)){
        disNum = MAX(s_FreeRobotNum,disNum);
        if (robots.size() <= disNum) {
            //LOG_DEBUG("no more robots - robots.size:%d,disNum:%d,gameType:%d", robots.size(),disNum, gameType);
            return;
        }
    }else{
        disNum = 0;
    }
    //robots.erase(robots.begin(),robots.begin()+disNum);

	vector<CGameRoom*> rooms;
	CGameRoomMgr::Instance().GetAllRobotRoom(rooms);
	if (rooms.size() == 0)
	{
		LOG_DEBUG("no_room_enter - robots.size:%d,rooms.size:%d,disNum:%d", robots.size(), rooms.size(), disNum);
		return;
	}

    while(robots.size() > 0)
    {
        CGameRoom* pRoom = NULL;
        if(rooms.size() > 1)
		{
            pRoom = rooms[g_RandGen.RandRange(0, rooms.size() - 1)];
        }
		else
		{
            pRoom = rooms[0];
        }
        if(pRoom->GetPlayerNum() > s_RoomMaxRobot || pRoom->IsNeedMarry())
		{
            break;
        }
        CGameRobot* pGamePlayer = robots[0];
		robots.erase(robots.begin());
		if (pGamePlayer == NULL)
		{
			continue;
		}
        bool benter_room = pRoom->EnterRoom(pGamePlayer);
        if(benter_room)
        {
            if(pRoom->FastJoinTable(pGamePlayer)){
                //LOG_DEBUG("enter game table success - uid:%d,robots.size:%d,allocNum:%d,m:%d,cur_score:%lld,roomid:%d", pGamePlayer->GetUID(), robots.size(), allocNum, m, pGamePlayer->GetAccountValue(emACC_VALUE_COIN), pRoom->GetRoomID());
            }else{
                pRoom->LeaveRoom(pGamePlayer);
                //LOG_DEBUG("leave game room - uid:%d,robots.size:%d,allocNum:%d,m:%d,cur_score:%lld,roomid:%d", pGamePlayer->GetUID(), robots.size(), allocNum, m, pGamePlayer->GetAccountValue(emACC_VALUE_COIN), pRoom->GetRoomID());
            }
        }
        else {
            //LOG_DEBUG("not enter game room - uid:%d,robots.size:%d,allocNum:%d,m:%d,cur_score:%lld,roomid:%d", pGamePlayer->GetUID(), robots.size(), allocNum,m, pGamePlayer->GetAccountValue(emACC_VALUE_COIN), pRoom->GetRoomID());
        }
    }
}
void    CRobotMgr::CheckRobotBackPool()
{
    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    for(;it != m_mpRobots.end();++it)
    {
        CGameRobot* pRobot = it->second;
        if(pRobot->NeedBackPool())
        {
            pRobot->BackPool();
            continue;
        }
    }
}
void    CRobotMgr::CheckRobotBackLobby()
{
    vector<CGameRobot*> vecRobots;    
    MAP_ROBOTS::iterator it = m_mpRobots.begin();
    for(;it != m_mpRobots.end();++it)
    {
        CGameRobot* pRobot = it->second;
        if(pRobot->NeedBackLobby())
        {            
            vecRobots.push_back(pRobot);
            continue;
        }
    }
    for(uint32 i=0;i<vecRobots.size();++i)
    {
        RemoveRobot(vecRobots[i]);
    }
    vecRobots.clear();
}

void    CRobotMgr::RetireFreeRobot()
{
    CheckRobotBackLobby();
}










































