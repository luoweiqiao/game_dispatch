//
// Created by toney on 16/4/5.
//

#ifndef SERVER_GAME_ROOM_MGR_H
#define SERVER_GAME_ROOM_MGR_H

#include "svrlib.h"
#include "game_room.h"
#include "pb/msg_define.pb.h"

class CGamePlayer;

class CGameRoomMgr : public AutoDeleteSingleton<CGameRoomMgr>
{
public:
    CGameRoomMgr();
    ~CGameRoomMgr();

    bool	Init();
    void	ShutDown();


    CGameRoom* GetRoom(uint32 roomID);

    void    SendRoomList2Client(uint32 uid);
    bool    FastJoinRoom(CGamePlayer* pPlayer,uint8 deal,uint8 consume);

    void    GetRoomList(uint8 deal,uint8 consume,vector<CGameRoom*>& rooms);
    void    GetAllRobotRoom(vector<CGameRoom*>& rooms);
    
    // 比较房间函数
    static  bool CompareRoomEnterLimit(CGameRoom* pRoom1,CGameRoom* pRoom2);
    static  bool CompareRoomNums(CGameRoom* pRoom1,CGameRoom* pRoom2);
    static  bool CompareRoomLessNums(CGameRoom* pRoom1,CGameRoom* pRoom2);
    
    void    GetAllRoomList(vector<CGameRoom*>& rooms);
private:
    typedef 	stl_hash_map<uint32, CGameRoom*>    MAP_ROOMS;
    MAP_ROOMS   m_mpRooms;


};


#endif //SERVER_GAME_ROOM_MGR_H
