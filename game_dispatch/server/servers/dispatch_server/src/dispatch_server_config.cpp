

#include "dispatch_server_config.h"

using namespace svrlib;
using namespace std;



// ����Lua����

void	defLuaConfig(lua_State* pL)
{
	if(pL == NULL)
	{
		LOG_CRITIC("defLuaConfig lua_state point is NULL ");
		return;
	}
	defLuaBaseConfig(pL);	
	
	lua_tinker::class_add<DispatchServerConfig>(pL,"DispatchServerConfig");
	lua_tinker::class_def<DispatchServerConfig>(pL,"SetNeedPassWD" 	,		&DispatchServerConfig::SetNeedPassWD	);
	lua_tinker::class_def<DispatchServerConfig>(pL,"GetRedisConf" 	,			&DispatchServerConfig::GetRedisConf	);
	lua_tinker::class_def<DispatchServerConfig>(pL,"GetDBConf" 		,		&DispatchServerConfig::GetDBConf		); 	

	LOG_DEBUG("����Lua����");

}




































