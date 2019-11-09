//
// Created by toney on 16/4/18.
//

#ifndef SERVER_MSG_PHP_HANDLE_H
#define SERVER_MSG_PHP_HANDLE_H

#include "svrlib.h"
#include "network/NetworkObject.h"
#include "helper/bufferStream.h"
#include "packet/streampacket.h"

using namespace Network;

class CHandlePHPMsg : public AutoDeleteSingleton<CHandlePHPMsg>
{
public:
    virtual int OnRecvClientMsg(NetworkObject* pNetObj, const uint8* pkt_buf, uint16 buf_len);


protected:
    /*------------------PHP消息---------------------------------*/
    // PHP修改保险箱密码
    int  handle_php_change_safepwd(NetworkObject* pNetObj,CBufferStream& stream);
    // PHP广播消息
    int  handle_php_broadcast(NetworkObject* pNetObj,CBufferStream& stream);
    // PHP系统公告
    int  handle_php_sys_notice(NetworkObject* pNetObj,CBufferStream& stream);
    // PHP踢出玩家
    int  handle_php_kill_player(NetworkObject* pNetObj,CBufferStream& stream);
    // PHP修改玩家数值
    int  handle_php_change_accvalue(NetworkObject* pNetObj,CBufferStream& stream);
    // 修改玩家名字
    int  handle_php_change_name(NetworkObject* pNetObj,CBufferStream& stream);
    // 停服
    int  handle_php_stop_service(NetworkObject* pNetObj,CBufferStream& stream);
    // 更改机器人配置
    int  handle_php_change_robot(NetworkObject* pNetObj,CBufferStream& stream);
    // 更改VIP
    int  handle_php_change_vip(NetworkObject* pNetObj,CBufferStream& stream);
	// 更改房间param
	int  handle_php_change_room_param(NetworkObject* pNetObj, CBufferStream& stream);
	// 控制玩家
	int  handle_php_control_player(NetworkObject* pNetObj, CBufferStream& stream);
	int   handle_php_control_multi_player(NetworkObject* pNetObj, CBufferStream& stream);

	//更新游戏内金币
	int  handle_php_update_accvalue_ingame(NetworkObject* pNetObj, CBufferStream& stream);
	//停止时时彩
	int  handle_php_stop_snatch_coin(NetworkObject* pNetObj, CBufferStream& stream);
	//修改vip广播
	int  handle_php_change_vip_broadcast(NetworkObject* pNetObj, CBufferStream& stream);

	int handle_php_control_dice_game_card(NetworkObject* pNetObj, CBufferStream& stream);

	int handle_php_online_config_robot(NetworkObject* pNetObj, CBufferStream& stream);

	int handle_php_config_mahiang_card(NetworkObject* pNetObj, CBufferStream& stream);

	// 更改房间库存配置
	int handle_php_change_room_stock_cfg(NetworkObject *pNetObj, CBufferStream &stream);

	// PHP修改幸运值配置信息
	int	 handle_php_change_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream);

	// PHP重置幸运值配置信息---每天凌晨重置
	int	 handle_php_reset_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream);

	// PHP修改捕鱼配置信息
	int	 handle_php_change_fish_cfg(NetworkObject* pNetObj, CBufferStream& stream);

	void SendPHPMsg(NetworkObject* pNetObj,string jmsg,uint16 cmd);
protected:


};





















#endif //SERVER_MSG_PHP_HANDLE_H
