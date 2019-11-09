
#include "redis_mgr.h"
#include "json/json.h"
#include "game_define.h"
#include <sstream>
#include <string>
#include "data_cfg_mgr.h"

using namespace std;
using namespace svrlib;

namespace
{
	const static char *s_key_onlinesvr		= "OLSVR";	 // 玩家在线服务器
	const static char *s_key_vipids			= "VIPIDS";	 // VIP玩家列表
	const static char *s_key_blacklist		= "FBD";	 // 封号列表
	const static char *s_key_loginkey		= "TMOLINE"; // 登陆KEY
    const static char *s_key_signinip       = "SIGNINIP";// 签到IP
    const static char *s_key_slog           = "SLOG";    // 服务器log
	const static char *s_key_blocker		= "BLOCKER"; // 屏蔽列表
    const static char *s_key_blockip		= "BLOCKIP"; // 屏蔽IP
    const static char *s_key_onlinelobby    = "OLLOBBY"; // 玩家大厅服务器ID信息
    const static char *s_key_svrsinfo       = "SVRINFO"; // 服务器的状态信息
	const static char *s_key_brc_control    = "BRCC";    // 百人场精准控制
};

CRedisMgr::CRedisMgr()
{
	m_svrID  = 0;
    m_pTimer = NULL;
}
CRedisMgr::~CRedisMgr()
{

}
void  CRedisMgr::OnTimer(uint8 eventID)
{
	CheckReconnectRedis();
}
bool	CRedisMgr::Init(stRedisConf& conf)
{
    m_pTimer = CApplication::Instance().MallocTimer(this,1);
    m_pTimer->StartTimer(5000,5000);
    
	m_svrID = CApplication::Instance().GetServerID();

	SetRedisConf(conf.redisHost.c_str(),conf.redisPort);
	ConnectRedis();
    
    return true;
}
void	CRedisMgr::ShutDown()
{
    if(m_pTimer){
        CApplication::Instance().FreeTimer(m_pTimer);
        m_pTimer = NULL;
    }
    
}
// 设置玩家在线服务器ID
void	CRedisMgr::SetPlayerOnlineSvrID(uint32 uid,uint16 svrID)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	if(svrID == 0){
		DelPlayerOnlineSvrID(uid);
		return;
	}
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HSET %s %d %d",s_key_onlinesvr,uid,svrID));
	FreeReply();
}
uint16  CRedisMgr::GetPlayerOnlineSvrID(uint32 uid)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return 0;
	}
	m_count++;
	uint16 svrID = 0;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HGET %s %d",s_key_onlinesvr,uid));
	if(NULL != m_reply && m_reply->str != NULL && m_reply->len > 0)
	{
		svrID = dAtoi(m_reply->str);
	}
	FreeReply();
	return svrID;
}
void 	CRedisMgr::DelPlayerOnlineSvrID(uint32 uid)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HDEL %s %d",s_key_onlinesvr,uid));
	FreeReply();
}
void 	CRedisMgr::ClearPlayerOnlineSvrID(uint16 svrID)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	vector<uint32> playerids;

	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HGETALL %s",s_key_onlinesvr));
	if(NULL != m_reply)
	{
		for(uint32 i=0;i<m_reply->elements;i+=2){
			redisReply* reply0 = m_reply->element[i];
			redisReply* reply1 = m_reply->element[i+1];
			uint32 uid = 0,cursvrid = 0;
			if(reply0->len > 0 && reply1->len > 0){
				uid = dAtoi(reply0->str);
				cursvrid = dAtoi(reply1->str);
				if(cursvrid == svrID || cursvrid == 0){
					playerids.push_back(uid);
				}
			}
		}
	}
	FreeReply();
	for(uint32 i=0;i<playerids.size();++i){
		DelPlayerOnlineSvrID(playerids[i]);
		//LOG_DEBUG("清理游戏服务器玩家ID:%d",playerids[i]);
	}
	playerids.clear();
}
// 保存玩家屏蔽玩家信息
void 	CRedisMgr::SavePlayerBlockers(uint32 uid,vector<uint32>& vecRef)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	if(vecRef.size() == 0)
		return;

	m_count++;
	Json::Value logValue;
	for(uint32 i=0;i<vecRef.size();++i){
		logValue.append(vecRef[i]);
	}
	LOG_DEBUG("保存黑名单:%s",logValue.toFastString().c_str());
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"SETEX %s%d %d %s",s_key_blocker,uid,SECONDS_IN_ONE_HOUR,logValue.toFastString().c_str()));
	FreeReply();
}
void 	CRedisMgr::LoadPlayerBlockers(uint32 uid,vector<uint32>& vecRef)
{
	m_count++;
	string str = GetString(CStringUtility::FormatToString("%s%d",s_key_blocker,uid).c_str());
	Json::Reader reader;
	Json::Value  jvalue;
	if(!reader.parse(str,jvalue))
	{
		return ;
	}
	for(uint32 i=0;i<jvalue.size();++i)
	{
		vecRef.push_back(jvalue[i].asUInt());
		LOG_DEBUG("加载黑名单:%d",vecRef[i]);
	}
}
void 	CRedisMgr::SavePlayerBlockerIPs(uint32 uid,vector<uint32>& vecRef)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	if(vecRef.size() == 0)
		return;

	m_count++;
	Json::Value logValue;
	for(uint32 i=0;i<vecRef.size();++i){
		logValue.append(vecRef[i]);
	}
	LOG_DEBUG("保存黑名单IP:%s",logValue.toFastString().c_str());
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"SETEX %s%d %d %s",s_key_blockip,uid,SECONDS_IN_ONE_HOUR,logValue.toFastString().c_str()));
	FreeReply();
}
void 	CRedisMgr::LoadPlayerBlockerIPs(uint32 uid,vector<uint32>& vecRef)
{
	m_count++;
	string str = GetString(CStringUtility::FormatToString("%s%d",s_key_blockip,uid).c_str());
	Json::Reader reader;
	Json::Value  jvalue;
	if(!reader.parse(str,jvalue))
	{
		return ;
	}
	for(uint32 i=0;i<jvalue.size();++i)
	{
		vecRef.push_back(jvalue[i].asUInt());
		LOG_DEBUG("加载黑名单IP:%d",vecRef[i]);
	}

}

// 设置币商UID
void 	CRedisMgr::AddVipPlayer(uint32 uid,int32 vip)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HSET %s %d %d",s_key_vipids,uid,vip));
	FreeReply();
}
void 	CRedisMgr::RemoveVipPlayer(uint32 uid)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HDEL %s %d",s_key_vipids,uid));
	FreeReply();
}
bool  	CRedisMgr::IsVipPlayer(uint32 uid)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return false;
	}
	m_count++;
	bool isExist = false;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HEXISTS %s %d",s_key_vipids,uid));
	if(NULL != m_reply){
		isExist = (m_reply->integer == 1) ? true : false;
	}
	FreeReply();
	return isExist;
}
// 签到设备限制
void    CRedisMgr::AddSignInDev(string dev,uint32 uid)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HSET %s %s %d",s_key_signinip,dev.c_str(),uid));
	FreeReply();    
}
void    CRedisMgr::ClearSignInDev()
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
    m_count++;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"DEL %s",s_key_signinip));
	FreeReply();
}
bool    CRedisMgr::IsHaveSignInDev(string dev)
{
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return false;
	}
	m_count++;
	bool isExist = false;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HEXISTS %s %s",s_key_signinip,dev.c_str()));
	if(NULL != m_reply){
		isExist = (m_reply->integer == 1) ? true : false;
	}
	FreeReply();
	return isExist;    
}
// 封号玩家
bool 	CRedisMgr::IsBlackList(uint32 uid)
{
	m_count++;
	if(IsExist(CStringUtility::FormatToString("%s%d",s_key_blacklist,uid).c_str()) != 0)
		return true;

	return false;
}
// 玩家登陆Key
string	CRedisMgr::GetPlayerLoginKey(uint32 uid)
{
	m_count++;
	return this->GetString(CStringUtility::FormatToString("%s%d",s_key_loginkey,uid).c_str());
}
void 	CRedisMgr::RenewalLoginKey(uint32 uid)
{
	m_count++;
	this->SetKeepAlive(CStringUtility::FormatToString("%s%d",s_key_loginkey,uid).c_str(),SECONDS_IN_ONE_DAY);
}
// 写入svrlog
void    CRedisMgr::WriteSvrLog(string logStr)
{
	LOG_DEBUG("logStr:%s", logStr.c_str());
	if(m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
    m_count++;
	//RedisCommand("rpush %s %s",s_key_slog,logStr.c_str());
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"rpush %s %s",s_key_slog,logStr.c_str()));
	FreeReply();
}


//设置玩家的大厅服务器ID
void    CRedisMgr::SetPlayerLobbySvrID(uint32 uid, uint16 lobbySvrID)
{
	if (m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	if(lobbySvrID == 0)
	{
		DelPlayerOnlineSvrID(uid);
		return;
	}
	//添加gamesvrid信息
	int onlineGameSvrID = GetPlayerOnlineSvrID(uid);
	stringstream instr;
	instr << lobbySvrID << " " << onlineGameSvrID;
	if(!IsConnect())
	{
		LOG_ERROR("connect break");
		return;
	}
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HSET %s %d %s",s_key_onlinelobby,uid,instr.str().c_str()));
	FreeReply();
}

uint16  CRedisMgr::GetPlayerLobbySvrID(uint32 uid)
{
	if (m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return 0;
	}
	m_count++;
	uint16 svrID = 0;
	if(!IsConnect())
	{
		LOG_ERROR("connect break");
		return 0;
	}
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HGET %s %d",s_key_onlinelobby,uid));
	if(NULL != m_reply && m_reply->str != NULL && m_reply->len > 0)
	{
		stringstream outstr(m_reply->str);
		outstr >> svrID; 
	}
	FreeReply();
	return svrID;
}

void	CRedisMgr::DelPlayerLobbySvrID(uint32 uid)
{
	if (m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	if(!IsConnect())
	{
		LOG_ERROR("connect break");
		return ;
	}
	uint16 redisLobbyID = CApplication::Instance().GetServerID();
	uint16 playerLobbyID = GetPlayerLobbySvrID(uid);
	bool bIsSameLobby = (redisLobbyID !=0 && redisLobbyID == playerLobbyID);
	if (bIsSameLobby)
	{
		m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "HDEL %s %d", s_key_onlinelobby, uid));
		FreeReply();
		LOG_ERROR("delete_lobbyid_succes - uid:%d,redisLobbyID:%d, playerLobbyID:%d, bIsSameLobby:%d", uid, redisLobbyID, playerLobbyID, bIsSameLobby);
	}
	else
	{
		LOG_ERROR("delete_lobbyid_failed - uid:%d,redisLobbyID:%d, playerLobbyID:%d, bIsSameLobby:%d", uid,redisLobbyID, playerLobbyID, bIsSameLobby);
	}
}

void	CRedisMgr::ClearPlayerOnlineLobbySvrID(uint16 lobbySvrID)
{
	if (m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	vector<uint32> playerids;
	if(!IsConnect())
	{
		LOG_ERROR("connect break");
		return ;
	}
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HGETALL %s",s_key_onlinelobby));
	if(NULL != m_reply)
	{
		for(uint32 i=0;i<m_reply->elements;i+=2){
			redisReply* reply0 = m_reply->element[i];
			redisReply* reply1 = m_reply->element[i+1];
			uint32 uid = 0,cursvrid = 0;
			if(reply0->len > 0 && reply1->len > 0){
				uid = dAtoi(reply0->str);
				cursvrid = dAtoi(reply1->str);
				if(cursvrid == lobbySvrID || cursvrid == 0){
					playerids.push_back(uid);
				}
			}
		}
	}
	FreeReply();
	for(uint32 i=0;i<playerids.size();++i){
		DelPlayerLobbySvrID(playerids[i]);
		//LOG_DEBUG("清理玩家的大厅服务ID:%d",playerids[i]);
	}
	playerids.clear();
}

void  CRedisMgr::UpdatePlayerOnlineGameSvrID(uint32 uid, uint16 gameSvrID)
{
	if (m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}
	m_count++;
	int currLobbySvrID = GetPlayerLobbySvrID(uid);
	if(!currLobbySvrID)
	{
		//LOG_DEBUG("currLobbySvrID is 0 OR This user is robot, do not update");
		return;
	}

	stringstream instr;
	instr << currLobbySvrID << " " << gameSvrID;

	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis,"HSET %s %d %s",s_key_onlinelobby,uid,instr.str().c_str()));
	FreeReply();

}

bool    CRedisMgr::WriteSvrStatusInfo(int svrID, int players, int robots, int status)
{
	if (m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return false;
	}
	m_count++;
	Json::Value val;
	val["svrID"] = svrID;
	val["players"] = players;
	val["robots"] = robots;
	val["status"] = status;

	stServerCfg * svrCfg = CDataCfgMgr::Instance().GetServerCfg(svrID);
	if(!svrCfg)
	{
		LOG_ERROR("Get svr:%d config faild", svrID);
		return false;
	}
	val["svr_type"] = svrCfg->svrType;
	val["svrport"] = svrCfg->svrport;
	val["svrip"] = svrCfg->svrip;
	val["name"] = svrCfg->svrName;
	
	std::string valStr = val.toFastString();
	if(!IsConnect())
	{
		LOG_ERROR("redis connect break");
		return false;
	}
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "HSET %s %d %s", s_key_svrsinfo, svrID, valStr.c_str()));
    FreeReply();

    return true;
}

bool	CRedisMgr::DelSvrStatusInfo(int svrID)
{
	if (m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return false;
	}
	m_count++;
	if(!IsConnect())
	{
		LOG_ERROR("connect break");
		return false;
	}
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "HDEL %s %d", s_key_svrsinfo, svrID));
	FreeReply();

	return true;
}

// 获取当前玩家的百人场的输赢记录
void 	CRedisMgr::GetBrcPlayerResultInfo(uint32 uid, uint16 gid, tagPlayerResultInfo &pri_info)
{
	if (m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}

	//当前库切换到3号库
	SelectDB(3);

	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "HGETALL %s_%d_%d", s_key_brc_control, uid, gid));
	if (NULL != m_reply)
	{
		for (uint32 i = 0; i < m_reply->elements; i += 2)
		{
			redisReply* reply0 = m_reply->element[i];
			redisReply* reply1 = m_reply->element[i + 1];
			if (reply0->len > 0)
			{
				if (strcmp(reply0->str, "total_win_coin") == 0)
				{
					pri_info.total_win_coin = dAtol(reply1->str);
					continue;
				}
				if (strcmp(reply0->str, "day_win_coin") == 0)
				{
					pri_info.day_win_coin = dAtol(reply1->str);
					continue;
				}
				if (strcmp(reply0->str, "last_update_time") == 0)
				{
					pri_info.last_update_time = dAtol(reply1->str);
					continue;
				}				
			}
		}
	}

	FreeReply();

	LOG_DEBUG("Get BRCC Info. uid:%d gid:%d total_win_coin:%d day_win_coin:%d last_update_time:%d", uid, gid, pri_info.total_win_coin, pri_info.day_win_coin, pri_info.last_update_time);

	//当前库切换到原来库
	SelectDB(0);
}

// 设置当前玩家的百人场的输赢记录
void    CRedisMgr::SetBrcPlayerResultInfo(uint32 uid, uint16 gid, tagPlayerResultInfo pri_info)
{
	if (m_redis == NULL)
	{
		LOG_ERROR("m_redis pointer is null.");
		return;
	}

	//当前库切换到3号库
	SelectDB(3);

	m_count++;
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "HSET %s_%d_%d %s %lld", s_key_brc_control, uid, gid, "total_win_coin", pri_info.total_win_coin));
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "HSET %s_%d_%d %s %lld", s_key_brc_control, uid, gid, "day_win_coin", pri_info.day_win_coin));
	m_reply = reinterpret_cast<redisReply*>(redisCommand(m_redis, "HSET %s_%d_%d %s %llu", s_key_brc_control, uid, gid, "last_update_time", pri_info.last_update_time));
	FreeReply();

	//当前库切换到原来库
	SelectDB(0);

}

