#ifndef DISPATCH_MGR_H_
#define DISPATCH_MGR_H_

#include "fundamental/noncopyable.h"
#include "svrlib.h"
#include <string.h>
#include "network/NetworkObject.h"
#include "network/protobuf_pkg.h"
#include "game_net_mgr.h"

struct stDispatchServer
{
	uint16 			svrID;
	DispatchNetObj* pNetObj;
	bool  			isRun;
	bool  			isReconnecting;
	stServerCfg 	dispatchCfg;
	stDispatchServer(){
		svrID 	= 0;
		pNetObj = 0;
		isRun   = false;
		isReconnecting = false;
	}
};

class DispatchMgr : public ITimerSink,public CProtobufMsgHanlde,public AutoDeleteSingleton<DispatchMgr>
{
public:
	virtual void  OnTimer(uint8 eventID);
	
	bool	Init();

	void	Register(uint16 svrid);
	void    RegisterRep(uint16 svrid, bool rep);
	
	bool    SendMsg2DispatchSvr(const google::protobuf::Message* msg, uint16 msg_type, uint16 svrid);    

    void	OnCloseClient(DispatchNetObj* pNetObj);
	void	ReConnect();
private:
	//typedef stl_hash_map<uint32,stDispatchServer> MAP_LOBBY;
	//MAP_LOBBY	  m_DispatchSvrs;// 大厅服务器
	stDispatchServer m_DispatchSvr;  
    CTimer*       m_pTimer;
};

#endif