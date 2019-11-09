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
	int handle_msg_register_server_rep(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 退休大厅
	int handle_msg_retire_lobbysvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 退休游戏
	int handle_msg_retire_gamesvr(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);
	// 处理其它大厅server通过dispatch转发过来的消息
	int handle_broadcast_msg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);

	int handle_check_other_lobby_server(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);

	
protected:
	// 同步其他大厅服玩家的数据(通过中心服转发)
	int handle_broadcast_sync_other_player_data(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);
	// 给玩家保险箱加钱(通过中心服转发)
	int handle_broadcast_give_safebox(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);
	// 玩家聊天信息转发(通过中心服转发)
	int handle_broadcast_chat_info_forward_rep(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);
	// php广播消息
	int handle_php_broadcast(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);
	// 修改机器人boss的数值
	int handle_php_change_robot_boss_accvalue(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);
	//vip代理充值
	int handle_php_notify_vip_proxy_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_notify_union_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_notify_wechat_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_notify_ali_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_notify_other_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_notify_qq_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_notify_wechat_scan_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_notify_jd_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_notify_apple_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_config_control_user(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);

	int handle_php_notify_large_ali_pay_recharge(NetworkObject* pNetObj, const char* pkt_buf, uint16 buf_len);
	// php修改专享闪付充值显示信息
	int handle_php_notify_exclusive_flash_recharge(NetworkObject *pNetObj, const char* pkt_buf, uint16 buf_len);

};

#endif