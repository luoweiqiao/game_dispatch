
#ifndef PHP_MGR_H_
#define PHP_MGR_H_

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

	int	handle_php_to_other_lobby_server(uint16 cmd, NetworkObject* pNetObj, CBufferStream& stream);

	int  handle_php_retire_server(NetworkObject* pNetObj, CBufferStream& stream);
	int  handle_php_get_svrlist(NetworkObject* pNetObj, CBufferStream& stream);
	// 广播消息
	int  handle_php_broadcast(NetworkObject* pNetObj, CBufferStream& stream);
	// php修改机器人配置
	int  handle_php_change_robot(NetworkObject* pNetObj,CBufferStream& stream);
	// 修改机器人鼻祖数值
	int	 handle_php_change_robot_boss_accvalue(NetworkObject* pNetObj,CBufferStream& stream);
	// PHP修改配置精准控制配置信息
	int	 handle_php_change_user_ctrl_cfg(NetworkObject* pNetObj, CBufferStream& stream);
	// 更改房间库存配置 add by har
	int handle_php_change_room_stock_cfg(NetworkObject *pNetObj, CBufferStream &stream);

	// PHP修改配置幸运值信息
	int	 handle_php_change_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream);
	// PHP重置配置幸运值信息
	int	 handle_php_reset_lucky_cfg(NetworkObject* pNetObj, CBufferStream& stream);
	// PHP重置配置捕鱼信息
	int	 handle_php_change_fish_cfg(NetworkObject* pNetObj, CBufferStream& stream);

protected:
    void SendPHPMsg(NetworkObject* pNetObj,string jmsg,uint16 cmd);
};


#endif 
