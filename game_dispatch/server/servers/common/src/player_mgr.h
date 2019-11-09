

#ifndef __PLAYER_MGR_H__
#define __PLAYER_MGR_H__


#include "player_base.h"
#include <map>
#include "svrlib.h"

#include <string>
#include "utility/stl_hash_map.h"


// 在线玩家管理器
using namespace svrlib;
using namespace std;
using namespace Network;


struct SPlayerData { // 离线玩家数据
	string name = ""; // 昵称
};

class CPlayerMgr : public ITimerSink,public AutoDeleteSingleton<CPlayerMgr>
{		
public:
    CPlayerMgr();
    ~CPlayerMgr();

    virtual void OnTimer(uint8 eventID);

    bool        Init();
	void		ShutDown();
	void        OnTimeTick();

    bool            IsOnline(uint32 uid);
    CPlayerBase*    GetPlayer(uint32 uid);
	
    bool        AddPlayer(CPlayerBase* pPlayer);
    bool        RemovePlayer(CPlayerBase* pPlayer);

	void		SendMsgToAll(const google::protobuf::Message* msg,uint16 msg_type);
	void		SendMsgToPlayer(const google::protobuf::Message* msg,uint16 msg_type,uint32 uid);

	uint32		GetOnlines(uint32& players,uint32& robots);
    
    void        GetAllPlayers(vector<CPlayerBase*>& refVec);

	stl_hash_map<uint32, CPlayerBase*> & GetAllPlayers() { return m_mpPlayers; }

    void        CheckRecoverPlayer();

	bool		SendVipBroadCast();

	bool		SendVipProxyRecharge();
	bool		SendVipAliAccRecharge();
	bool		SendUnionPayRecharge();
	bool		SendWeChatPayRecharge();
	bool		SendAliPayRecharge();
	bool		SendOtherPayRecharge();
	bool		SendQQPayRecharge();
	bool		SendWeChatScanPayRecharge();
	bool		SendJDPayRecharge();
	bool		SendApplePayRecharge();
	bool		SendLargeAliPayRecharge();
	void SendExclusiveFlashRecharge();
	// 设置其他大厅玩家的数据
	// name : 玩家昵称
	void SetOtherLobbyPlayerData(uint32 uid, const string& name);
	// 获取其他大厅服登陆的玩家数据
	SPlayerData* GetOtherLobbyPlayerData(uint32 uid);

private:
	typedef 	stl_hash_map<uint32, CPlayerBase*> 		 	MAPPLAYERS;		
    MAPPLAYERS  m_mpPlayers;
    CTimer*     m_pFlushTimer;
    CTimer*     m_pRecoverTimer;
	unordered_map<uint32, SPlayerData> m_umpOtherLobbyPlayerData; // 保存离线玩家数据(昵称等)
};




#endif // __PLAYER_MGR_H__



