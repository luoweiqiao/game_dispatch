
#ifndef _LOBBY_MGR_H__
#define _LOBBY_MGR_H__

#include "fundamental/noncopyable.h"
#include "svrlib.h"
#include <string.h>
#include "network/NetworkObject.h"
#include "network/protobuf_pkg.h"
#include "game_net_mgr.h"
#include "json/json.h"

using namespace std;
using namespace svrlib;
using namespace Network;

/**
 * 管理大厅服务器
 */
struct  stLobbyServer
{
	uint32 			svrID;
	uint32          svrType;
	uint32          gameType;
	NetworkObject*   pNetObj;
	uint32  		status;
	uint32          palyerNum;
	uint32          robotNum;
	stLobbyServer(){
		svrID 	 = 0;
		svrType  = 0;
		gameType = 0;
		pNetObj  = 0;
		status   = emSERVER_STATE_NORMAL;
		palyerNum = 0;
		robotNum = 0;
	}
};

class CLobbyMgr : public ITimerSink,public CProtobufMsgHanlde,public AutoDeleteSingleton<CLobbyMgr>
{
public:
	CLobbyMgr();
	~CLobbyMgr();

	virtual void OnTimer(uint8 eventID);

	bool	Init();
	void	ShutDown();
        
	bool   AddServer(NetworkObject* pNetObj,uint32 svrID,uint32 svrType,uint32 gameType);
	void   RemoveServer(NetworkObject* pNetObj);

public:
	void   NotifyRetireServer(vector<uint32> svrs);
	stLobbyServer * GetServerByNetObj(NetworkObject* pNetObj);
	void WriteSvrsInfo(Json::Value & value);
	void BroadcastInfo2Lobby(const google::protobuf::Message* msg,uint16 msg_type, uint32 srcSvrID, uint32 destSvrID);
	void CheckOtherLobbyServerLogin(const google::protobuf::Message* msg, uint16 msg_type, uint32 srcSvrID, uint32 uid);

	void BroadcastPHPMsg2Lobby(const string msg, uint16 type, int destSvrID);
	uint32 GetMasterSvrID();
private:
	typedef stl_hash_map<uint32,stLobbyServer> MAP_LOBBY;
	MAP_LOBBY	  m_lobbySvrs;// 大厅服务器
	//stLobbyServer m_lobbySvr;  
    CTimer*       m_pTimer;
};



class CHandleLobbyMsg : public IProtobufClientMsgRecvSink
{
public: 
	virtual int OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);	
protected:
	// 注册
    int  handle_msg_lobby_register(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len); 
    // 上报信息
    int  handle_msg_Online_Info(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
    // 广播
    int  handle_broadcast_msg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);

	int  handle_check_other_lobby_server(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);

};

#endif // _LOBBY_MGR_H__





