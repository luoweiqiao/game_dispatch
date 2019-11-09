/*
* game_server.cpp
*
*  modify on: 2017-7-13
*  Author: kenny
*/
#include "game_define.h"
#include "framework/application.h"
#include "game_net_mgr.h"
#include "dbmysql_mgr.h"
#include "lobby_mgr.h"
#include "svrlib.h"
#include <iostream>
#include "stdafx.h"
#include "utility/timeFunction.h"
#include "data_cfg_mgr.h"
#include "dispatch_server_config.h"

#include "center_log.h"

#include <time.h>
using namespace svrlib;
using namespace std;

extern string   g_strConfFilename;

bool CApplication::Initialize()
{
	defLuaConfig(getLua());
		
	// 加载lua 配置   
    if(this->call<bool>("dispatch_config",m_uiServerID,&DispatchServerConfig::Instance()) == false)    
	{
		LOG_ERROR("load game_config fail ");
		return false;
	}    
	
	if(CDBMysqlMgr::Instance().Init(DispatchServerConfig::Instance().DBConf) == false)
	{
		LOG_ERROR("init mysqlmgr fail ");
		return false;
	}
	if(CDataCfgMgr::Instance().Init() == false) {
		LOG_ERROR("init datamgr fail ");
		return false;
	}
	if(!CGameNetMgr::Instance().Init())
	{
		LOG_ERROR("init net mgr failed");
		return false;
	}
	if(!CRedisMgr::Instance().Init(DispatchServerConfig::Instance().redisConf[0]))
    {
    	LOG_ERROR("redis init failed");
        return false;
    }
	if(!CLobbyMgr::Instance().Init())
	{
		LOG_ERROR("init lobbymgr fail");
		return false;
	}
	if(!CServerMgr::Instance().Init())
	{
		LOG_ERROR("Init game serverMgr fail");
		return false;
	}
	
	return true;
}

void  CApplication::ShutDown()
{
    CRedisMgr::Instance().ShutDown();
}

/**
* 本函数将在程序启动时和每次配置改变时被调用。
* 第一次调用将在Initialize()之前
*/
void CApplication::ConfigurationChanged()
{
    // 重加载配置
	LOG_ERROR("configuration changed");
    CDataCfgMgr::Instance().Reload();
}

void CApplication::Tick()
{	
    int64 tick1 = getTickCount64();
	int64 tick2 = 0;
	CGameNetMgr::Instance().Update();
	tick2 = getTickCount64();
	if((tick2-tick1)>100){
		LOG_ERROR("network cost time:%lld",tick2-tick1);
	}

    CDBMysqlMgr::Instance().ProcessDBEvent(); 
}

int main(int argc, char * argv[])
{
	return FrameworkMain(argc, argv);
}
