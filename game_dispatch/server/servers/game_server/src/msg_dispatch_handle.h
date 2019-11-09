#ifndef MSG_DISPATCH_HANDLE_H_
#define MSG_DISPATCH_HANDLE_H_

#include "network/protobuf_pkg.h"


using namespace Network;

class CGamePlayer;

class CHandleDispatchMsg : public IProtobufClientMsgRecvSink
{
public:
	virtual int OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len,PACKETHEAD* head);
protected:
	int handle_msg_register_gamesvr_rep(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int handle_msg_notify_new_lobby(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	int handle_msg_retire_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	//有大厅已经退休
	int handle_msg_lobbysvr_retired(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 修改房间库存配置 add by har
	int handle_msg_change_room_stock_cfg(NetworkObject *pNetObj, const uint8 *pkt_buf, uint16 buf_len);
};

#endif