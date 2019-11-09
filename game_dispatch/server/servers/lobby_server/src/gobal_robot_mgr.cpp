//
// Created by toney on 16/5/22.
//

#include <server_mgr.h>
#include "gobal_robot_mgr.h"
#include "stdafx.h"
#include "player.h"
#include "data_cfg_mgr.h"
#include "game_define.h"
#include "robot.h"

using namespace std;


CGobalRobotMgr::CGobalRobotMgr()
{
    m_checkLogin.beginCooling(20000);
	m_iRobotCityCount = 0;
	for (int i = 0; i < net::GAME_CATE_MAX_TYPE; i++)
	{
		m_bArrLoadAllDayRobotStatus[i] = false;
	}
}
CGobalRobotMgr::~CGobalRobotMgr()
{

}
bool	CGobalRobotMgr::Init()
{
    m_checkLogin.beginCooling(20000);
    LoadRobotPayPoolData();

    CDBMysqlMgr::Instance().ClearRobotLoginState();

    //CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadRobotOnlineCfg(m_mpRobotCfg);

	while (strcmp("", g_robot_city_tables[m_iRobotCityCount].c_str())!=0)
	{
		m_iRobotCityCount++;
	}
	m_iRobotLoadCount = 0;
    return true;
}
void	CGobalRobotMgr::ShutDown()
{

}
void	CGobalRobotMgr::OnTimeTick()
{
    if(m_checkLogin.isTimeOut())
	{
        CheckRobotLogin();
		if (m_iRobotLoadCount >= 20)
		{
			m_checkLogin.beginCooling(SECONDS_IN_MIN * 5 * 1000);
		}
		else
		{
			m_iRobotLoadCount++;
			m_checkLogin.beginCooling(SECONDS_IN_MIN * 1 * 1000);
		}
    }
}
// 尝试登陆一个机器人
bool    CGobalRobotMgr::LoginRobot(stRobotCfg & refCfg)
{
    CPlayer* pRobot = (CPlayer*)CPlayerMgr::Instance().GetPlayer(refCfg.uid);
	if (pRobot != NULL)
	{
		return false;
	}
    return AddRobot(refCfg);
}
bool    CGobalRobotMgr::AddRobot(stRobotCfg & refCfg)
{
	//LOG_DEBUG("robot login gameType:%d,uid:%d,richLv:%d", refCfg.gameType, refCfg.uid, refCfg.richLv);

	LOG_DEBUG("robot_login - batchID:%d,leveltype:%d,loginType:%d,uid:%d,scoreLv:%d,richLv:%d,gameType:%d",
		refCfg.batchID, refCfg.leveltype, refCfg.loginType, refCfg.uid, refCfg.scoreLv, refCfg.richLv, refCfg.gameType);

    CLobbyRobot* pRobot = new CLobbyRobot();
    pRobot->SetSession(NULL);
    pRobot->SetUID(refCfg.uid);
	pRobot->SetBatchID(refCfg.batchID);
	pRobot->SetLvScore(refCfg.scoreLv);
	pRobot->SetLvCoin(refCfg.richLv);
	int iRandRobotCityIndex = g_RandGen.RandRange(0, m_iRobotCityCount - 1);
	pRobot->SetCity(g_robot_city_tables[iRandRobotCityIndex]);
	//pRobot->SetCity("中国");

    CPlayerMgr::Instance().AddPlayer(pRobot);
    pRobot->OnLogin();
    pRobot->SetRobotCfg(refCfg);


	LOG_DEBUG("robot city info - uid:%d,m_iRobotCityCount:%d,iRandRobotCityIndex:%d,tables:%s,city:%s", refCfg.uid, m_iRobotCityCount, iRandRobotCityIndex, g_robot_city_tables[iRandRobotCityIndex].c_str(), pRobot->GetCity().c_str());

	//COUNT_OCCUPY_MEM_SZIE(this);

    return true;
}
// 检测新机器人上线
void    CGobalRobotMgr::CheckRobotLogin()
{
	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		return;
	}

    if(CApplication::Instance().GetStatus() == emSERVER_STATE_RETIRE)
    {
        return;
    }
    
    //因为访问同一个数据库 防止同一个机器人
    //加载到两个大厅 导致数据错乱
    if(!CApplication::Instance().call<bool>("CheckIsMasterSvr", CApplication::Instance().GetServerID()))
    {
        //LOG_ERROR("its not mastersvr");
        return;
    }
	tm local_time;
	uint64 uTime = getTime();
	getLocalTime(&local_time, uTime);

	map<uint32, vector<stRobotOnlineCfg>> & mpRobotCfg = CDataCfgMgr::Instance().GetRobotOnlineCfg();

	map<uint32, stRobotOnlineCfg>  mpRobotOnLineCount;
	uint32 allOnlineRobotCount = GetOnLineRobotCount(mpRobotOnLineCount);

	LOG_DEBUG("server_cfg_robot -allOnlineRobotCount:%d, mpRobotCfg.size:%d", allOnlineRobotCount, mpRobotCfg.size());
	for (auto iter_loop = mpRobotCfg.begin(); iter_loop != mpRobotCfg.end(); iter_loop++)
	{
		uint32 gameType = iter_loop->first;
		vector<stRobotOnlineCfg> & refVecCfg = iter_loop->second;
		if (refVecCfg.size() == 0)
		{
			LOG_DEBUG("not_cfg_robot - gameType:%d,size:%d", gameType, mpRobotCfg.size());
			continue;
		}
		bool bLoadRobotStatus = CheckLoadRobotStatus(gameType);

		if (bLoadRobotStatus == true)
		{
			bool bRetSubLoadIndex = true;//CDataCfgMgr::Instance().SubCfgRobotLoadindex(gameType);
			LOG_DEBUG("load_status_error - gameType:%d,bRetSubLoadIndex:%d,size:%d", gameType, bRetSubLoadIndex, mpRobotCfg.size());

			continue;
		}
		int minLoadIndex = refVecCfg[0].iLoadindex;
		uint32 curLoadIndex = 0;
		string strVecCfg;
		for (uint32 i = 0; i < refVecCfg.size(); i++)
		{
			auto & refCfg = refVecCfg[i];
			if (refCfg.iLoadindex < minLoadIndex)
			{
				minLoadIndex = refCfg.iLoadindex;
				curLoadIndex = i;
			}
			strVecCfg += CStringUtility::FormatToString("i:%d,batchID:%d,iLoadindex:%d ", i, refCfg.batchID, refCfg.iLoadindex);
		}

		stRobotOnlineCfg & refCfg = refVecCfg[curLoadIndex];
		refCfg.iLoadindex++;
		if (refCfg.iLoadindex >= 0x7FFFFFFF)
		{
			refCfg.iLoadindex = 0;
		}
		if (CServerMgr::Instance().IsOpenRobot(refCfg.gameType) == 0)
		{
			LOG_DEBUG("server_not_open_robot - batchID:%d,gameType:%d", refCfg.batchID, refCfg.gameType);
			continue;
		}
		LOG_DEBUG("load_robot - refVecCfg.size:%d,gameType:%d,batchID:%d,iLoadindex:%d,minLoadIndex:%d,curLoadIndex:%d,strVecCfg.c_str:%s",
			refVecCfg.size(), refCfg.gameType, refCfg.batchID, refCfg.iLoadindex, minLoadIndex, curLoadIndex, strVecCfg.c_str());
		for (uint8 lv = 0; lv<ROBOT_MAX_LEVEL; ++lv)
		{
			uint32 cfgRobotCount = refCfg.onlines[lv];
			LOG_DEBUG("start_load - gameType:%d,lv:%d,cfgRobotCount:%d,batchID:%d", refCfg.gameType, lv, cfgRobotCount, refCfg.batchID);
			if (cfgRobotCount == 0)
			{
				continue;
			}

			uint32 onlineRobotCount = 0;
			auto find_robotOnLineCount = mpRobotOnLineCount.find(refCfg.batchID);
			if (find_robotOnLineCount != mpRobotOnLineCount.end())
			{
				auto & tempRobotOnlineCfg = find_robotOnLineCount->second;
				onlineRobotCount = tempRobotOnlineCfg.onlines[lv];
			}

			LOG_DEBUG("sting_load - gameType:%d,lv:%d,cfgRobotCount:%d,onlineRobotCount:%d,batchID:%d", refCfg.gameType, lv, cfgRobotCount, onlineRobotCount, refCfg.batchID);
			if (onlineRobotCount >= cfgRobotCount)
			{
				continue;
			}
			uint32 needLoadRobotCount = cfgRobotCount - onlineRobotCount;
			LOG_DEBUG("sted_load - gameType:%d,lv:%d,needLoadRobotCount:%d,onlineRobotCount:%d,cfgRobotCount:%d,local_time.tm_wday:%d,batchID:%d,leveltype:%d",
				refCfg.gameType, lv, needLoadRobotCount, onlineRobotCount, cfgRobotCount, local_time.tm_wday, refCfg.batchID, refCfg.leveltype);

			bool bRetMysql = CDBMysqlMgr::Instance().AsyncLoadRobotLogin(refCfg.gameType, lv + 1, local_time.tm_wday + 1, needLoadRobotCount, refCfg.batchID, refCfg.loginType, refCfg.leveltype);
			if (bRetMysql == true)
			{
				CGobalRobotMgr::Instance().UpdateLoadRobotStatus(gameType, true);
			}
		}
	}
}

uint32  CGobalRobotMgr::GetOnLineRobotCount(map<uint32, stRobotOnlineCfg> & mpBatchCount)
{
	//vector<CPlayerBase*> allPlayers;
	//CPlayerMgr::Instance().GetAllPlayers(allPlayers);
	uint32 count = 0;
	mpBatchCount.clear();
	stl_hash_map<uint32, CPlayerBase*> & mpPlayers = CPlayerMgr::Instance().GetAllPlayers();
	for (auto iter = mpPlayers.begin(); iter != mpPlayers.end(); ++iter)
	{
		if (iter->second == NULL)
		{
			continue;
		}
		if (iter->second->IsRobot())
		{
			count++;
			CLobbyRobot* pRobot = (CLobbyRobot*)iter->second;
			if (pRobot != NULL && pRobot->GetCfgLevel() > 0 && pRobot->GetCfgLevel() <= ROBOT_MAX_LEVEL)
			{
				int  cfgLevel = pRobot->GetCfgLevel();
				cfgLevel -= 1;
				if (cfgLevel >= 0 && cfgLevel < ROBOT_MAX_LEVEL)
				{
					auto iter_find = mpBatchCount.find(pRobot->GetBatchID());
					if (iter_find != mpBatchCount.end())
					{
						auto & tempRobotOnlineCfg = iter_find->second;
						tempRobotOnlineCfg.onlines[cfgLevel]++;
					}
					else
					{
						stRobotOnlineCfg tempRobotOnlineCfg;
						tempRobotOnlineCfg.onlines[cfgLevel]++;
						mpBatchCount.insert(make_pair(pRobot->GetBatchID(), tempRobotOnlineCfg));
					}
				}
			}
		}
	}
	return count;
}

bool CGobalRobotMgr::UpdateLoadRobotStatus(uint32 gameType, bool flag)
{
	bool ret = false;
	if (gameType >= GAME_CATE_LAND && gameType < net::GAME_CATE_MAX_TYPE)
	{
		ret = true;
		m_bArrLoadAllDayRobotStatus[gameType] = flag;
	}
	return ret;
}

bool CGobalRobotMgr::CheckLoadRobotStatus(uint32 gameType)
{
	bool ret = true;
	if (gameType >= GAME_CATE_LAND && gameType < net::GAME_CATE_MAX_TYPE)
	{
		ret = m_bArrLoadAllDayRobotStatus[gameType];
	}
	return ret;
}

// 加载机器人充值池数据
bool    CGobalRobotMgr::LoadRobotPayPoolData()
{
    LOG_DEBUG("robotmgr login:%d",ROBOT_MGR_ID);

    for(uint32 i=0;i<net::GAME_CATE_MAX_TYPE;++i)
    {
        if(!CCommonLogic::IsOpenGame(i))
            continue;
        
        uint32 uid = ROBOT_MGR_ID + i;
        CPlayer* pRobot = new CLobbyRobot();
        pRobot->SetSession(NULL);
        pRobot->SetUID(uid);
        CPlayerMgr::Instance().AddPlayer(pRobot);
        pRobot->OnLogin();
    }
    return true;
}

bool    CGobalRobotMgr::AddRobotPayPoolData(uint16 gameType)
{
	LOG_DEBUG("robotmgr - ROBOT_MGR_ID:%d,gameType:%d", ROBOT_MGR_ID, gameType);
	if (gameType > GAME_CATE_MAX_TYPE || gameType <= 0)
	{
		return false;
	}
	uint32 robotUid = ROBOT_MGR_ID + gameType;
	bool bIs_robot_online = CPlayerMgr::Instance().IsOnline(robotUid);
	LOG_DEBUG("robotmgr - robotUid:%d,bIs_robot_online:%d", robotUid, bIs_robot_online);
	if (bIs_robot_online)
	{
		return false;
	}
	else
	{
		CPlayer* pRobot = new CLobbyRobot();
		pRobot->SetSession(NULL);
		pRobot->SetUID(robotUid);
		CPlayerMgr::Instance().AddPlayer(pRobot);
		pRobot->OnLogin();
	}
	return true;
}


// 机器人充值存入积分财富币
bool    CGobalRobotMgr::StoreScoreCoin(CLobbyRobot* pPlayer,int64 score,int64 coin)
{
    CPlayer* pRobotMgr = GetRobotMgr(pPlayer->GetGameType());
    if(pRobotMgr == NULL){
        return false;
    }
    if(!pPlayer->CanChangeAccountValue(emACC_VALUE_SCORE,-score) || !pPlayer->CanChangeAccountValue(emACC_VALUE_COIN,-coin)){
        return false;
    }
    if(!pRobotMgr->CanChangeAccountValue(emACC_VALUE_SCORE,score) || !pRobotMgr->CanChangeAccountValue(emACC_VALUE_COIN,coin)){
        return false;
    }
    pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTSTORE,0,0,-coin,0,-score,0,0);
    pRobotMgr->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTSTORE,1,0,coin,0,score,0,0);
    LOG_DEBUG("robot store uid:%d-score:%lld-coin:%lld",pPlayer->GetUID(),score,coin);
    
    return true;
}
// 机器人充值钻石
bool    CGobalRobotMgr::PayDiamond(CLobbyRobot* pPlayer,int64 diamond)
{
    CPlayer* pRobotMgr = GetRobotMgr(pPlayer->GetGameType());
    if(pRobotMgr == NULL){
        return false;
    }
    if(!pRobotMgr->CanChangeAccountValue(emACC_VALUE_DIAMOND,-diamond)){
        LOG_DEBUG("总账号%d钻石不足，不能充值:%d",pRobotMgr->GetUID(),pRobotMgr->GetAccountValue(emACC_VALUE_DIAMOND));
        return false;
    }
    pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTPAY,0,diamond,0,0,0,0,0);
    pRobotMgr->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTPAY,1,-diamond,0,0,0,0,0);
    LOG_DEBUG("机器人:%d 充值钻石:%lld",pPlayer->GetUID(),diamond);
    return true;
}
// 兑换积分
bool    CGobalRobotMgr::TakeScoreCoin(CLobbyRobot* pPlayer,int64 score,int64 coin)
{
    CPlayer* pRobotMgr = GetRobotMgr(pPlayer->GetGameType());
    if(pRobotMgr == NULL){
        return false;
    }
    if(!pRobotMgr->CanChangeAccountValue(emACC_VALUE_SCORE,-score)){
        LOG_DEBUG("机器人总账号:%d积分不足:%lld",pRobotMgr->GetUID(),pRobotMgr->GetAccountValue(emACC_VALUE_SCORE));
        return false;
    }
    if(!pRobotMgr->CanChangeAccountValue(emACC_VALUE_COIN,-coin)){
        LOG_DEBUG("机器人总账号:%d财富币不足:%lld",pRobotMgr->GetUID(),pRobotMgr->GetAccountValue(emACC_VALUE_COIN));
        return false;
    }
    pRobotMgr->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_ROBOTPAY,1,0,-coin,0,-score,0,0);
    LOG_DEBUG("机器人兑换积分财富币:%lld -- %lld",score,coin);

    return true;
}
// 获得机器人管理员
CPlayer* CGobalRobotMgr::GetRobotMgr(uint16 gameType)
{
    uint32 mgrID = ROBOT_MGR_ID + gameType;
    CPlayer* pRobotMgr = (CPlayer*)CPlayerMgr::Instance().GetPlayer(mgrID);
    if(pRobotMgr == NULL || !pRobotMgr->IsPlaying()){
        LOG_ERROR("机器人总账号未登录或未加载数据成功:%d",mgrID);
        return NULL;
    }    
    return pRobotMgr;
}
// 重新加载机器人数据
void    CGobalRobotMgr::ChangeAllRobotCfg()
{
    vector<CPlayerBase*> allPlayers;
    CPlayerMgr::Instance().GetAllPlayers(allPlayers);
    for(uint32 i=0;i<allPlayers.size();++i){
        if(allPlayers[i]->IsRobot()){
            CLobbyRobot* pRobot = (CLobbyRobot*)allPlayers[i];
            pRobot->SetNeedLoginOut(true);

            net::msg_leave_robot msg;
            msg.set_uid(pRobot->GetUID());
            pRobot->SendMsgToGameSvr(&msg,net::L2S_MSG_LEAVE_ROBOT);
        }
    }
}
// 获得在线机器人数量
uint32  CGobalRobotMgr::GetRobotNum(uint16 gameType,uint8 level)
{
    uint32 count = 0;
    vector<CPlayerBase*> allPlayers;
    CPlayerMgr::Instance().GetAllPlayers(allPlayers);
    for(uint32 i=0;i<allPlayers.size();++i)
    {
        if(allPlayers[i]->IsRobot())
        {
            CLobbyRobot* pRobot = (CLobbyRobot*)allPlayers[i];
            if(pRobot->GetGameType() == gameType)
            {
                if(pRobot->GetCfgLevel() == level)
                    count++;
            }
        }
    }
    return count;
}

