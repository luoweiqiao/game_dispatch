#ifndef SERVER_MGR_H_
#define SERVER_MGR_H_

#include <vector>
#include "svrlib.h"
#include "game_define.h"
#include "json/json.h"

using namespace std;
using namespace svrlib;
using namespace Network;

struct  stGServer
{
	uint32 svrID;
	uint32 svrType;
	uint32 gameType;
	uint32  status;
	NetworkObject* pNetObj;
	uint32 palyerNum;
	uint32 robotNum; 
	stGServer()
	{
		svrID = 0;
		svrType = 0;
		gameType = 0;
		status = emSERVER_STATE_NORMAL;
		pNetObj = NULL;
		palyerNum = 0;
		robotNum = 0;
	}	
};

struct stStockCfg; // 前向声明  add by har

class CServerMgr : public ITimerSink,public CProtobufMsgHanlde,public AutoDeleteSingleton<CServerMgr>
{
public:
	CServerMgr();
	~CServerMgr();

	virtual void OnTimer(uint8 eventID);

	bool	Init();
	void	ShutDown();
        
	bool   AddServer(NetworkObject* pNetObj,uint32 svrID,uint32 svrType,uint32 gameType);
	void   RemoveServer(NetworkObject* pNetObj);
public:
	void NotifyRetireServer(vector<uint32> svrs);
	void   NotifyGameSvrsHasNewLobby(uint32 svrid);
	stGServer * GetServer(uint32 svrID);
	stGServer * GetServerByNetObj(NetworkObject* pNetObj);
	void WriteSvrsInfo(Json::Value & value);

	uint32 GetGameSvrSzie() {return m_mpServers.size();}
	void 	NotifyGameSvrRetire(uint32 svrID);
	// 通知游戏服修改房间库存配置  add by har
	bool NotifyGameSvrsChangeRoomStockCfg(uint32 gameType, stStockCfg &st);
private:
	typedef stl_hash_map<uint32,stGServer>  MAP_SERVERS;
	MAP_SERVERS		m_mpServers;
};


class CHandleServerMsg : public IProtobufClientMsgRecvSink
{
public:
	virtual int OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
protected:
	int  handle_msg_gameSvr_register(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len); 
	int  handle_msg_Online_Info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
};


#endif