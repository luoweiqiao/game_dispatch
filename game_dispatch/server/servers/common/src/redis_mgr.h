
#ifndef REDIS_MGR_H__
#define REDIS_MGR_H__

#include "dbmysql/hiredisop.h"
#include "svrlib.h"
#include "config/config.h"
#include <string>
#include "db_struct_define.h"

using namespace std;
using namespace svrlib;

class CRedisMgr : public ITimerSink,public CHiredisOp,public AutoDeleteSingleton<CRedisMgr>
{
public:
	CRedisMgr();
	~CRedisMgr();

	virtual void OnTimer(uint8 eventID);
	
	bool	Init(stRedisConf& conf);
	void	ShutDown();
	
	// 设置玩家在线服务器ID
	void	SetPlayerOnlineSvrID(uint32 uid,uint16 svrID);
	uint16  GetPlayerOnlineSvrID(uint32 uid);
	void 	DelPlayerOnlineSvrID(uint32 uid);
	void 	ClearPlayerOnlineSvrID(uint16 svrID);
	// 保存玩家屏蔽玩家信息
	void 	SavePlayerBlockers(uint32 uid,vector<uint32>& vecRef);
	void 	LoadPlayerBlockers(uint32 uid,vector<uint32>& vecRef);
	void 	SavePlayerBlockerIPs(uint32 uid,vector<uint32>& vecRef);
	void 	LoadPlayerBlockerIPs(uint32 uid,vector<uint32>& vecRef);

	// 设置币商UID
	void 	AddVipPlayer(uint32 uid,int32 vip);
	void 	RemoveVipPlayer(uint32 uid);
	bool  	IsVipPlayer(uint32 uid);
    // 签到设备限制
    void    AddSignInDev(string dev,uint32 uid);
    void    ClearSignInDev();
    bool    IsHaveSignInDev(string dev);
    
	// 封号玩家
	bool 	IsBlackList(uint32 uid);
	// 玩家登陆Key
	string	GetPlayerLoginKey(uint32 uid);
	void 	RenewalLoginKey(uint32 uid);

	uint32	GetOperCount(){ return m_count; }
	void 	ClearOperCount(){ m_count = 0; }

    // 写入svrlog
    void    WriteSvrLog(string logStr);

    // 设置玩家的lobbys_server的ID
	void    SetPlayerLobbySvrID(uint32 uid, uint16 lobbySvrID);
	uint16  GetPlayerLobbySvrID(uint32 uid);
	void	DelPlayerLobbySvrID(uint32 uid);
	void	ClearPlayerOnlineLobbySvrID(uint16 lobbySvrID);
	void    UpdatePlayerOnlineGameSvrID(uint32 uid, uint16 gameSvrID);
    
	//! 服务器状态信息
    bool    WriteSvrStatusInfo(int svrID, int players, int robots, int status);
    bool	DelSvrStatusInfo(int svrID); 

	// 百人场精准控制信息
	void    GetBrcPlayerResultInfo(uint32 uid, uint16 gid, tagPlayerResultInfo	&pri_info);
	void    SetBrcPlayerResultInfo(uint32 uid, uint16 gid, tagPlayerResultInfo	pri_info);
    
private:
	uint16  m_svrID;
    CTimer* m_pTimer;
    char	m_szKey[128];
	uint32  m_count;

};

#endif // REDIS_MGR_H__

























































































































































