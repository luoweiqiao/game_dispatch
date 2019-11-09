//
// Created by toney on 16/4/5.
//
#include "game_room_mgr.h"
#include "stdafx.h"
#include "data_cfg_mgr.h"

using namespace std;
using namespace svrlib;

CGameRoomMgr::CGameRoomMgr()
{

}
CGameRoomMgr::~CGameRoomMgr()
{

}

bool	CGameRoomMgr::Init()
{
    // 初始化房间信息
    uint16 gameType     = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
    uint8  gameSubType  = CDataCfgMgr::Instance().GetCurSvrCfg().gameSubType;

    vector<stRoomCfg> vecRooms;
    bool bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadRoomCfg(gameType,gameSubType,vecRooms);
    if(!bRet) return false;
	// add by har
	unordered_map<uint16, stStockCfg> umIRooms;
	bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadRoomStockCfg(gameType, umIRooms);
	if (!bRet) return false; // add by har end
    for(stRoomCfg &cfg : vecRooms)
    {
        CGameRoom* pRoom = new CGameRoom();
        pRoom->SetRoomCfg(cfg);
		unordered_map<uint16, stStockCfg>::iterator it = umIRooms.find(cfg.roomID);
		if (it != umIRooms.end())
			pRoom->SetRoomStockCfg(it->second); // add by har
        if(pRoom->Init(gameType) == false)
        {
            LOG_ERROR("房间配置错误:%d",cfg.roomID);
            return false;
        }
        m_mpRooms.insert(make_pair(pRoom->GetRoomID(),pRoom));
    }
    CDBMysqlMgr::Instance().ClearPlayerOnlineInfo(CApplication::Instance().GetServerID());
    return true;
}
void	CGameRoomMgr::ShutDown()
{
    for(auto &it : m_mpRooms)
    {
        CGameRoom* pRoom = it.second;
        pRoom->ShutDown();
        SAFE_DELETE(pRoom);
    }
    m_mpRooms.clear();
}
CGameRoom* CGameRoomMgr::GetRoom(uint32 roomID)
{
    CGameRoom* pRoom = NULL;
    auto it = m_mpRooms.find(roomID);
    if(it != m_mpRooms.end()){
        pRoom = it->second;
    }
    return pRoom;
}
void    CGameRoomMgr::SendRoomList2Client(uint32 uid)
{
    CGamePlayer* pGamePlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
    if(pGamePlayer == NULL)
        return;
    net::msg_rooms_info_rep roominfo;
    roominfo.set_cur_roomid(pGamePlayer->GetRoomID());
    for(auto &it : m_mpRooms)
    {
        net::room_info* pRoom = roominfo.add_rooms();
        CGameRoom* pGameRoom = it.second;
        pGameRoom->GetRoomInfo(pRoom);
		LOG_DEBUG("send room - uid:%d,cur_roomid:%d,rooms_size:%d,roomid:%d - %d,basescore:%lld - %lld,jetton_min:%lld - %lld",
			uid, roominfo.cur_roomid(), roominfo.rooms_size(), pRoom->id(), pGameRoom->GetRoomID(), pRoom->basescore(), pGameRoom->GetBaseScore(),pRoom->jetton_min(),pGameRoom->GetJettonMin());
    }
    pGamePlayer->SendMsgToClient(&roominfo,net::S2C_MSG_ROOMS_INFO);
    LOG_DEBUG("send room uid:%d,cur_roomid:%d,list:%d", uid, roominfo.cur_roomid(),roominfo.rooms_size());
}
bool    CGameRoomMgr::FastJoinRoom(CGamePlayer* pPlayer,uint8 deal,uint8 consume)
{
    CGameRoom* pOldRoom = pPlayer->GetRoom();
    if(pOldRoom != NULL) {
        if(!pOldRoom->CanLeaveRoom(pPlayer)) {
            return false;
        }
        if(!pOldRoom->LeaveRoom(pPlayer)){
            return false;
        }
    }
    vector<CGameRoom*> rooms;
    GetRoomList(deal,consume,rooms);
    for(auto pRoom : rooms)
    {
        if(pRoom->GetPlayerNum() > 0 && pRoom->CanEnterRoom(pPlayer))
        {
            pRoom->EnterRoom(pPlayer);
            pRoom->FastJoinTable(pPlayer);
            return true;
        }
    }
    for(auto pRoom : rooms)
    {
        if(pRoom->CanEnterRoom(pPlayer))
        {
            pRoom->EnterRoom(pPlayer);
            pRoom->FastJoinTable(pPlayer);            
            return true;
        }
    }    
    return false;
}
void    CGameRoomMgr::GetRoomList(uint8 deal,uint8 consume,vector<CGameRoom*>& rooms)
{
    for(auto &it : m_mpRooms)
    {
        CGameRoom* pGameRoom = it.second;
        //if(pGameRoom->GetDeal() == deal && pGameRoom->GetConsume() == consume){
            rooms.push_back(pGameRoom);
        //}
    }
    sort(rooms.begin(),rooms.end(),CompareRoomEnterLimit);
}
void    CGameRoomMgr::GetAllRobotRoom(vector<CGameRoom*>& rooms)
{
    for(auto &it : m_mpRooms)
    {
        CGameRoom* pGameRoom = it.second;
        if(pGameRoom->GetRobotCfg() != 0){        
            rooms.push_back(pGameRoom);
        }
    }
    //sort(rooms.begin(),rooms.end(),CompareRoomLessNums);
}
bool    CGameRoomMgr::CompareRoomEnterLimit(CGameRoom* pRoom1,CGameRoom* pRoom2)
{
    return pRoom1->GetEnterMin() > pRoom2->GetEnterMin();
}
// 比较房间函数
bool    CGameRoomMgr::CompareRoomNums(CGameRoom* pRoom1,CGameRoom* pRoom2)
{
    return pRoom1->GetPlayerNum() > pRoom2->GetPlayerNum();
}
bool    CGameRoomMgr::CompareRoomLessNums(CGameRoom* pRoom1,CGameRoom* pRoom2)
{
    if(pRoom1->GetConsume() == pRoom2->GetConsume()) {
        return pRoom1->GetPlayerNum() < pRoom2->GetPlayerNum();
    }
    return pRoom1->GetConsume() > pRoom2->GetConsume();
}

void    CGameRoomMgr::GetAllRoomList(vector<CGameRoom*>& rooms)
{
    for(auto &it : m_mpRooms)
    {
        CGameRoom* pGameRoom = it.second;
        {
            rooms.push_back(pGameRoom);
        }
    }
    sort(rooms.begin(),rooms.end(),CompareRoomEnterLimit);
}











