
#include <data_cfg_mgr.h>
#include <center_log.h>
#include "game_imple_table.h"
#include "stdafx.h"
#include "game_room.h"
#include "json/json.h"
#include "robot_mgr.h"
#include "common_logic.h"

using namespace std;
using namespace svrlib;
using namespace net;

namespace
{
	const static uint32 s_FreeTime			 = 1*1000;
    const static uint32 s_AddScoreTime       = 120*1000;
    const static uint32 s_ShowCardTime       = 5*1000;
	const static int64  s_lAreaMultiple[] = { 1,1,1,1,2,10,2 };
};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           //普通房
        {
            pTable = new CGameEveryColorTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛房
        {
            pTable = new CGameEveryColorTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人房
        {
            pTable = new CGameEveryColorTable(this,tableID,emTABLE_TYPE_PLAYER);
        }break;
    default:
        {
            assert(false);
            return NULL;
        }break;
    }
    return pTable;
}

CGameEveryColorTable::CGameEveryColorTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
	m_mpUserWinScoreFlower.clear();
	m_mpUserWinScoreBigSmall.clear();
	m_mpUserLostScoreFlower.clear();
	m_mpUserLostScoreBigSmall.clear();
	m_mpUserWinScoreToatl.clear();
	m_vecPlayers.clear();
	m_bInitTableSuccess = false;

	m_uGameCount = 0;
	m_iFlowerColorTax = 0;
	m_ibigSmallTax = 0;
	m_ibigSmallSwitch = 0;
	m_lJackpotFlower = 0;
	m_lFrontJackpotBigSmall = 0;
	m_lJackpotBigSmall = 0;
	m_lRandMaxScore = 0;
	m_lRandMinScore = 0;
	m_iSysWinPro = 0;
	m_iSysLostPro = 0;
	m_strPeriods = "";

	//夺宝数据
	m_uGameCountTen = 0;
	m_uGameCountHundred = 0;
	m_uSnatchCoinRobotWinPre = 0;
	m_uSnatchCoinStop = TRUE;
	ResetGameDataTen();
	ResetGameDataHundred();
}
CGameEveryColorTable::~CGameEveryColorTable()
{

}
bool    CGameEveryColorTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		LOG_DEBUG("player pointer is null");
		return false;
	}
	if (pPlayer->GetTable() != NULL)
	{
		LOG_DEBUG("player table is not null - uid:%d,ptable:%p", pPlayer->GetUID(), pPlayer->GetTable());
		return false;
	}
		
	if (IsFullTable())
	{
		LOG_DEBUG("game table limit - uid:%d,is_full:%d", pPlayer->GetUID(), IsFullTable());
		return false;
	}
	if (pPlayer->IsRobot())
	{
		if (m_mpLookers.size() >= 300)
		{
			LOG_DEBUG("robot is full - uid:%d,m_mpLookers.size:%d", pPlayer->GetUID(), m_mpLookers.size());
			return false;
		}
	}
    return true;
}
bool    CGameEveryColorTable::CanLeaveTable(CGamePlayer* pPlayer)
{
	if (GetGameState() != TABLE_STATE_FREE)
	{
		return false;
	}

    return true;
}

bool    CGameEveryColorTable::IsFullTable()
{
	if (m_mpLookers.size() >= 500)
	{
		return true;
	}	

	return false;
}

void    CGameEveryColorTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::everycolor_table_info* peverycolor = pInfo->mutable_everycolor();
	peverycolor->set_basescore(m_conf.baseScore);
	peverycolor->set_consume(m_conf.consume);
	peverycolor->set_feetype(m_conf.feeType);
	peverycolor->set_feevalue(m_conf.feeValue);
	peverycolor->set_table_state(GetGameState());
	peverycolor->set_add_score_time(s_AddScoreTime);
	peverycolor->set_show_card_time(s_ShowCardTime);
}

//配置桌子
bool    CGameEveryColorTable::Init()
{
    SetGameState(net::TABLE_STATE_FREE);
	m_gameStateTen = net::TABLE_STATE_FREE;
	m_gameStateHundred = net::TABLE_STATE_FREE;
	m_coolLogicTen.clearCool();
	m_coolLogicHundred.clearCool();

    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
	
	ReAnalysisParam();
	m_bInitTableSuccess = true;

	bool bReTen = false, bReHundred = false;
	bool bFlag = LoadSnatchCoinGameData(bReTen, bReHundred);
	m_uSnatchCoinStop = FALSE;

	InitBlingLogTen();
	InitBlingLogHundred();

	if (bReTen)
	{
		OnGameStartTen();
	}
	else
	{
		//继续上一局游戏
		WritePeriodsInfoTen();
		m_gameStateTen = net::TABLE_STATE_PLAY;
	}
	if (bReHundred)
	{
		OnGameStartHundred();
	}
	else
	{
		//继续上一局游戏
		WritePeriodsInfoHundred();
		m_gameStateHundred = net::TABLE_STATE_PLAY;
	}

	LOG_DEBUG("init_table - bReTen:%d,bReHundred:%d,", bReTen, bReHundred);

    return true;
}

bool CGameEveryColorTable::ReAnalysisParam()
{
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		//{"fcp":7000,"bsp":7000,"bss":1,"fcj":0,"bsj":0,"rmaxs":10000000,"rmins":-10000000,"swp":5000,"slp":5000}
		LOG_ERROR("reader json parse error - param:%s", param.c_str());
		return true;
	}
	
	int iFlowerColorY = 0;
	int iBigSmallY = 0;

	if (jvalue.isMember("fcp")) {
		m_iFlowerColorTax = jvalue["fcp"].asInt(); // 花色税率
	}
	if (jvalue.isMember("bsp")) {
		m_ibigSmallTax = jvalue["bsp"].asInt();	// 大小税率
	}
	if (jvalue.isMember("bss")) {
		m_ibigSmallSwitch = jvalue["bss"].asInt();	// 大小开关
	}

	if (jvalue.isMember("fcy")) {
		iFlowerColorY = jvalue["fcy"].asInt(); // 花色奖池开关
	}
	if (iFlowerColorY == 1) {
		if (jvalue.isMember("fcj")) {
			m_lJackpotFlower = jvalue["fcj"].asInt64(); // 花色奖池
		}
	}

	if (jvalue.isMember("bsy")) {
		iBigSmallY = jvalue["bsy"].asInt(); // 大小奖池开关
	}

	if (iBigSmallY == 1)
	{
		if (jvalue.isMember("bsj")) {
			m_lJackpotBigSmall = jvalue["bsj"].asInt64();	// 大小奖池
			m_lFrontJackpotBigSmall = m_lJackpotBigSmall;
		}
	}

	if (jvalue.isMember("rmaxs")) {
		m_lRandMaxScore = jvalue["rmaxs"].asInt64();	// 随机上限
	}
	if (jvalue.isMember("rmins")) {
		m_lRandMinScore = jvalue["rmins"].asInt64();	// 随机下线
	}
	if (jvalue.isMember("swp")) {
		m_iSysWinPro = jvalue["swp"].asInt();	// 吃币
	}
	if (jvalue.isMember("slp")) {
		m_iSysLostPro = jvalue["slp"].asInt();	// 吐币
	}

	int iIsUpdateChangeLostWinPro = 0;
	if (jvalue.isMember("clw")) {
		iIsUpdateChangeLostWinPro = jvalue["clw"].asInt(); // 大小奖池开关
	}
	if (iIsUpdateChangeLostWinPro == 1)
	{
		if (jvalue.isMember("lwp")) {
			m_uSysLostWinProChange = jvalue["lwp"].asInt();	// 吐币
		}
		if (jvalue.isMember("ujs")) {
			m_lUpdateJackpotScore = jvalue["ujs"].asInt64();	// 吐币
		}
	}

	if (jvalue.isMember("srp")) {
		m_uSnatchCoinRobotWinPre = jvalue["srp"].asInt();	// 夺宝机器人赢概率
	}

	LOG_ERROR("reader json parse success - roomid:%d,tableid:%d,m_iFlowerColorTax:%d,m_ibigSmallTax:%d,m_ibigSmallSwitch:%d,m_lJackpotFlower:%lld,m_lJackpotBigSmall:%lld,m_lRandMaxScore:%lld,m_lRandMinScore:%lld,m_iSysWinPro:%d,m_iSysLostPro:%d,iIsUpdateChangeLostWinPro:%d,m_uSysLostWinProChange:%d,m_lUpdateJackpotScore:%lld",
		m_pHostRoom->GetRoomID(), GetTableID(), m_iFlowerColorTax, m_ibigSmallTax, m_ibigSmallSwitch, m_lJackpotFlower, m_lJackpotBigSmall, m_lRandMaxScore, m_lRandMinScore, m_iSysWinPro, m_iSysLostPro, iIsUpdateChangeLostWinPro, m_uSysLostWinProChange, m_lUpdateJackpotScore);
	return true;
}

void    CGameEveryColorTable::ShutDown()
{

}
//复位桌子
void    CGameEveryColorTable::ResetTable()
{
    ResetGameData();

    SetGameState(TABLE_STATE_FREE);
    SendSeatInfoToClient();
}
void    CGameEveryColorTable::OnTimeTick()
{
	uint8 tableState = GetGameState();
    if(m_coolLogic.isTimeOut())
    {
        switch(tableState)
        {
        case TABLE_STATE_FREE:
            {
            	CheckRetireTableUser();
                if(IsCanStartGame())
				{
					SetGameState(TABLE_STATE_PLAY);
					m_coolLogic.beginCooling(s_AddScoreTime);
                    OnGameStart();
                }
            }break;
        case TABLE_STATE_PLAY:
            {
				SetGameState(TABLE_STATE_FREE);
				m_coolLogic.beginCooling(s_ShowCardTime);
				OnGameEnd(255, GER_NORMAL);
            }break;
        default:
            break;
        }
    }
	if (m_coolLogicTen.isTimeOut())
	{
		switch (m_gameStateTen)
		{
		case net::TABLE_STATE_GAME_END:
		{			
			OnGameStartTen();
		}break;
		default:
			break;
		}
	}
	if (m_coolLogicHundred.isTimeOut())
	{
		switch (m_gameStateHundred)
		{
		case net::TABLE_STATE_GAME_END:
		{
			OnGameStartHundred();
		}break;
		default:
			break;
		}
	}
	//LOG_DEBUG("m_uSnatchCoinStop:%d,m_gameStateTen:%d,size:%d,getPassTick:%lld", m_uSnatchCoinStop, m_gameStateTen, m_poolCardsTen.size(), m_coolLogicTen.getPassTick());

	if (m_uSnatchCoinStop == FALSE && m_gameStateTen == TABLE_STATE_PLAY)
	{
		OnRobotOperSnatchCoinTen();
	}
	if (m_uSnatchCoinStop == FALSE && m_gameStateHundred == TABLE_STATE_PLAY)
	{
		OnRobotOperSnatchCoinHundred();
	}
}

void    CGameEveryColorTable::OnRobotOper()
{
	vector<CGamePlayer*> robots;
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL || !pPlayer->IsRobot())
			continue;
		robots.push_back(pPlayer);
	}

	for (uint16 i = 0; i<robots.size(); ++i)
	{
		uint16 pos = g_RandGen.RandRange(0, robots.size() - 1);
		CGamePlayer* pPlayer = robots[pos];
		uint8 area = g_RandGen.RandRange(AreaIndex_Spade, AreaIndex_Small);
		int64 maxJetton = GetUserMaxJetton(pPlayer);
		if (maxJetton < 10)
			continue;
		int64 minJetton = GetUserMaxJetton(pPlayer) / 10;
		int64 jetton = g_RandGen.RandRange(minJetton, maxJetton);
		jetton = (jetton / 1000) * 1000;
		jetton = MAX(jetton, 1000);
		if (!OnUserPlaceJetton(pPlayer, area, jetton))
		{
			break;
		}			
	}
}

void    CGameEveryColorTable::GetAllRobotPlayer(vector<CGamePlayer*> & robots)
{
	robots.clear();
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL || !pPlayer->IsRobot())
		{
			continue;
		}
		robots.push_back(pPlayer);
	}

	LOG_DEBUG("robots.size:%d", robots.size());
}

void    CGameEveryColorTable::OnRobotSnatchCoin(BYTE cbType, BYTE cbCount, int iRobotCount, vector<CGamePlayer*> & robots)
{
	bool bRet = false;
	//LOG_DEBUG("1 cbType:%d,cbCount:%d,iRobotCount:%d,robots.size:%d,bRet:%d", cbType, cbCount, iRobotCount, robots.size(), bRet);

	for (uint16 i = 0; i < robots.size(); ++i)
	{
		uint16 pos = g_RandGen.RandRange(0, robots.size() - 1);
		CGamePlayer* pPlayer = robots[pos];
		bRet = OnUserSnatchCoin(pPlayer, cbType, cbCount);
		//LOG_DEBUG("2 cbType:%d,cbCount:%d,iRobotCount:%d,robots.size:%d,bRet:%d", cbType, cbCount, iRobotCount, robots.size(), bRet);
		iRobotCount--;
		if (iRobotCount <= 0)
		{
			return;
		}
	}
}

void    CGameEveryColorTable::OnRobotOperSnatchCoinTen()
{
	if (m_uSnatchCoinStop == TRUE)
	{
		return;
	}
	bool bFlag = g_RandGen.RandRatio(10000, PRO_DENO_10000);
	uint64 lCurTime = getTickCount64();
	vector<CGamePlayer*> robots;
	//LOG_DEBUG("m_uSnatchCoinStop:%d,size:%d", m_uSnatchCoinStop, m_poolCardsTen.size());

	if (m_poolCardsTen.size() >= 38)
	{
		int32 iSpaceMin = g_RandGen.RandRange(4, 5);
		uint64 lSpaceTime = iSpaceMin * 60 * 1000;
		BYTE cbType = SnatchCoinType_Ten;
		int iRobotCount = 2;
		BYTE cbCount = g_RandGen.RandRange(1, 5);
		//LOG_DEBUG("bFlag:%d,lCurTime:%lld,lTime:%lld", bFlag, lCurTime, m_lLastTimeRobotOperSnatchCoinTen );

		if (bFlag && lCurTime >= m_lLastTimeRobotOperSnatchCoinTen)
		{
			m_lLastTimeRobotOperSnatchCoinTen = lCurTime;
			GetAllRobotPlayer(robots);
			OnRobotSnatchCoin(cbType, cbCount, iRobotCount, robots);
		}
	}
	else if (m_poolCardsTen.size() >= 27)
	{
		int32 iSpaceMin = g_RandGen.RandRange(2, 3);
		uint64 lSpaceTime = iSpaceMin * 60 * 1000;
		BYTE cbType = SnatchCoinType_Ten;
		int iRobotCount = 1;
		BYTE cbCount = g_RandGen.RandRange(2, 7);		
		if (bFlag && lCurTime >= m_lLastTimeRobotOperSnatchCoinTen + lSpaceTime)
		{
			m_lLastTimeRobotOperSnatchCoinTen = lCurTime;
			GetAllRobotPlayer(robots);
			OnRobotSnatchCoin(cbType, cbCount, iRobotCount, robots);
		}
	}
	else if (m_poolCardsTen.size() >= 16)
	{
		int32 iSpaceMin = g_RandGen.RandRange(1, 2);
		uint64 lSpaceTime = iSpaceMin * 60 * 1000;
		BYTE cbType = SnatchCoinType_Ten;
		int iRobotCount = 1;
		BYTE cbCount = g_RandGen.RandRange(3, 8);
		if (bFlag && lCurTime >= m_lLastTimeRobotOperSnatchCoinTen + lSpaceTime)
		{
			m_lLastTimeRobotOperSnatchCoinTen = lCurTime;
			GetAllRobotPlayer(robots);
			OnRobotSnatchCoin(cbType, cbCount, iRobotCount, robots);
		}
	}
	else if (m_poolCardsTen.size() < 16)
	{
		uint64 lSpaceTime = 20 * 1000;
		BYTE cbType = SnatchCoinType_Ten;
		int iRobotCount = 1;
		BYTE cbCount = g_RandGen.RandRange(3, 8);
		if (bFlag && lCurTime >= m_lLastTimeRobotOperSnatchCoinTen + lSpaceTime)
		{
			m_lLastTimeRobotOperSnatchCoinTen = lCurTime;
			GetAllRobotPlayer(robots);
			OnRobotSnatchCoin(cbType, cbCount, iRobotCount, robots);
		}
	}
	else
	{
		return;
	}
}

void    CGameEveryColorTable::OnRobotOperSnatchCoinHundred()
{
	if (m_uSnatchCoinStop == TRUE)
	{
		return;
	}
	bool bFlag = g_RandGen.RandRatio(5000, PRO_DENO_10000);
	uint64 lCurTime = getTickCount64();
	vector<CGamePlayer*> robots;

	if (m_poolCardsHundred.size() >= 38)
	{

		int32 iSpaceMin = g_RandGen.RandRange(4, 5);
		uint64 lSpaceTime = iSpaceMin * 60 * 1000;
		BYTE cbType = SnatchCoinType_Hundred;
		int iRobotCount = 2;
		BYTE cbCount = g_RandGen.RandRange(1, 5);
		LOG_DEBUG("bFlag:%d,lCurTime:%lld,lTime:%lld", bFlag, lCurTime, m_lLastTimeRobotOperSnatchCoinHundred + lSpaceTime);

		if (bFlag && lCurTime >= m_lLastTimeRobotOperSnatchCoinHundred + lSpaceTime)
		{
			m_lLastTimeRobotOperSnatchCoinHundred = lCurTime;						
			GetAllRobotPlayer(robots);
			OnRobotSnatchCoin(cbType, cbCount, iRobotCount, robots);
		}
	}
	else if (m_poolCardsHundred.size() >= 27)
	{
		int32 iSpaceMin = g_RandGen.RandRange(2, 3);
		uint64 lSpaceTime = iSpaceMin * 60 * 1000;
		BYTE cbType = SnatchCoinType_Hundred;
		int iRobotCount = 1;
		BYTE cbCount = g_RandGen.RandRange(2, 7);
		if (bFlag && lCurTime >= m_lLastTimeRobotOperSnatchCoinHundred + lSpaceTime)
		{
			m_lLastTimeRobotOperSnatchCoinHundred = lCurTime;
			GetAllRobotPlayer(robots);
			OnRobotSnatchCoin(cbType, cbCount, iRobotCount, robots);
		}
	}
	else if (m_poolCardsHundred.size() >= 16)
	{
		int32 iSpaceMin = g_RandGen.RandRange(1, 2);
		uint64 lSpaceTime = iSpaceMin * 60 * 1000;
		BYTE cbType = SnatchCoinType_Hundred;
		int iRobotCount = 1;
		BYTE cbCount = g_RandGen.RandRange(3, 8);
		if (bFlag && lCurTime >= m_lLastTimeRobotOperSnatchCoinHundred + lSpaceTime)
		{
			m_lLastTimeRobotOperSnatchCoinHundred = lCurTime;
			GetAllRobotPlayer(robots);
			OnRobotSnatchCoin(cbType, cbCount, iRobotCount, robots);
		}
	}
	else if (m_poolCardsHundred.size() < 16)
	{
		uint64 lSpaceTime = 20 * 1000;
		BYTE cbType = SnatchCoinType_Hundred;
		int iRobotCount = 1;
		BYTE cbCount = g_RandGen.RandRange(3, 8);
		if (bFlag && lCurTime >= m_lLastTimeRobotOperSnatchCoinHundred + lSpaceTime)
		{
			m_lLastTimeRobotOperSnatchCoinHundred = lCurTime;
			GetAllRobotPlayer(robots);
			OnRobotSnatchCoin(cbType, cbCount, iRobotCount, robots);
		}
	}
	else
	{
		return;
	}
}

// 游戏消息
int     CGameEveryColorTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
	if (pPlayer == NULL || m_uSnatchCoinStop == TRUE)
	{
		uint32 tempuid = 0;
		if (pPlayer != NULL)
		{
			tempuid = pPlayer->GetUID();
		}
		LOG_DEBUG("recv player is null or server is stop - tempuid:%d,m_uSnatchCoinStop:%d", tempuid, m_uSnatchCoinStop);
		return 0;
	}
	uint32 uid = pPlayer->GetUID();
    LOG_DEBUG("recv game msg - uid:%d,cmdID:%d", uid,cmdID);

    switch(cmdID)
    {
    case net::C2S_MSG_EVERY_COLOR_PLACE_JETTON_REQ:// 加注
        {
            if(GetGameState() != TABLE_STATE_PLAY)
			{
                LOG_DEBUG("not is place jetton - uid:%d,gamestate:%d", pPlayer->GetUID(),GetGameState());
                return 0;
            }
            net::msg_everycolor_place_jetton_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
			OnUserPlaceJetton(pPlayer, msg.jetton_area(), msg.jetton_score());
			return 1;
        }break;
	case net::C2S_MSG_EVERY_COLOR_SNATCH_COIN_REQ://夺宝
		{
			if (GetGameState() != TABLE_STATE_PLAY)
			{
				LOG_DEBUG("not is place jetton - uid:%d,gamestate:%d", pPlayer->GetUID(), GetGameState());
				return 0;
			}
			net::msg_everycolor_snatch_coin_req msg;
			PARSE_MSG_FROM_ARRAY(msg);
			return OnUserSnatchCoin(pPlayer, msg.snatch_type(), msg.snatch_count());
		}break;
    default:
        return 0;
    }
    return 0;
}

// 游戏开始
bool    CGameEveryColorTable::OnGameStart()
{
	ResetGameData();

	m_uGameCount++;
	if (m_uGameCount == 10000)
	{
		m_uGameCount = 1;
	}
	tm	tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	getLocalTime(&tmTime, getSysTime());
	char szTempDate[32] = { 0 };
	m_strPeriods = CStringUtility::FormatToString("%.4d%.2d%.2d%04d", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday, m_uGameCount);
	
	LOG_DEBUG("game start - roomid:%d,tableid:%d,m_strPeriods:%s,m_uGameCount:%d", m_pHostRoom->GetRoomID(), GetTableID(), m_strPeriods.c_str(), m_uGameCount);


	//构造数据
    net::msg_everycolor_start_rep msg;
	msg.set_periods_num(m_strPeriods.c_str());
	msg.set_time_leave(m_coolLogic.getCoolTick());
    SendMsgToAll(&msg,net::S2C_MSG_EVERY_COLOR_GAME_START);
	
	InitBlingLog();
	WritePeriodsInfo();

    return true;
}

// 游戏开始
bool    CGameEveryColorTable::OnGameStartTen()
{
	if (m_uSnatchCoinStop == TRUE)
	{
		return false;
	}
	ResetGameDataTen();

	m_uGameCountTen++;
	if (m_uGameCountTen == 10000)
	{
		m_uGameCountTen = 1;
	}
	tm	tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	getLocalTime(&tmTime, getSysTime());
	char szTempDate[32] = { 0 };
	m_strPeriodsTen = CStringUtility::FormatToString("%.4d%.2d%.2d%04d", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday, m_uGameCountTen);

	LOG_DEBUG("game start - roomid:%d,tableid:%d,m_strPeriodsTen:%s,m_uGameCountTen:%d", m_pHostRoom->GetRoomID(), GetTableID(), m_strPeriodsTen.c_str(), m_uGameCountTen);

	InitRandCardTen();

	//构造数据
	net::msg_everycolor_snatch_coin_start_rep msg;
	msg.set_periods_num(m_strPeriodsTen.c_str());
	SendMsgToAll(&msg, net::S2C_MSG_EVERY_COLOR_TEN_GAME_START);

	InitBlingLogTen();
	WritePeriodsInfoTen();

	m_gameStateTen = net::TABLE_STATE_PLAY;

	return true;
}

// 游戏开始
bool    CGameEveryColorTable::OnGameStartHundred()
{
	if (m_uSnatchCoinStop == TRUE)
	{
		return false;
	}
	ResetGameDataHundred();

	m_uGameCountHundred++;
	if (m_uGameCountHundred == 10000)
	{
		m_uGameCountHundred = 1;
	}
	tm	tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	getLocalTime(&tmTime, getSysTime());
	char szTempDate[32] = { 0 };
	m_strPeriodsHundred = CStringUtility::FormatToString("%.4d%.2d%.2d%04d", tmTime.tm_year + 1900, tmTime.tm_mon + 1, tmTime.tm_mday, m_uGameCountHundred);

	LOG_DEBUG("game start - roomid:%d,tableid:%d,m_strPeriodsHundred:%s,m_uGameCountHundred:%d", m_pHostRoom->GetRoomID(), GetTableID(), m_strPeriodsHundred.c_str(), m_uGameCountHundred);

	InitRandCardHundred();

	//构造数据
	net::msg_everycolor_snatch_coin_start_rep msg;
	msg.set_periods_num(m_strPeriodsHundred.c_str());
	SendMsgToAll(&msg, net::S2C_MSG_EVERY_COLOR_HUNDRED_GAME_START);

	InitBlingLogHundred();
	WritePeriodsInfoHundred();

	m_gameStateHundred = net::TABLE_STATE_PLAY;
	return true;
}
// 牌局日志
void    CGameEveryColorTable::InitBlingLogTen()
{
	string chessid = CStringUtility::FormatToString("%d-%d-%d-%d", m_pHostRoom->GetGameType(), m_pHostRoom->GetRoomID(), GetTableID(), getSysTime());

	m_blingLogTen.Reset();
	m_blingLogTen.baseScore = m_conf.baseScore;
	m_blingLogTen.consume = m_conf.consume;
	m_blingLogTen.deal = m_conf.deal;
	m_blingLogTen.startTime = getSysTime();
	m_blingLogTen.gameType = m_pHostRoom->GetGameType();
	m_blingLogTen.roomType = m_pHostRoom->GetRoomType();
	m_blingLogTen.tableID = GetTableID();
	m_blingLogTen.chessid = chessid;
	m_blingLogTen.roomID = m_pHostRoom->GetRoomID();

	m_operLogTen.clear();
}

// 牌局日志
void    CGameEveryColorTable::InitBlingLogHundred()
{
	string chessid = CStringUtility::FormatToString("%d-%d-%d-%d", m_pHostRoom->GetGameType(), m_pHostRoom->GetRoomID(), GetTableID(), getSysTime());

	m_blingLogHundred.Reset();
	m_blingLogHundred.baseScore = m_conf.baseScore;
	m_blingLogHundred.consume = m_conf.consume;
	m_blingLogHundred.deal = m_conf.deal;
	m_blingLogHundred.startTime = getSysTime();
	m_blingLogHundred.gameType = m_pHostRoom->GetGameType();
	m_blingLogHundred.roomType = m_pHostRoom->GetRoomType();
	m_blingLogHundred.tableID = GetTableID();
	m_blingLogHundred.chessid = chessid;
	m_blingLogHundred.roomID = m_pHostRoom->GetRoomID();

	m_operLogHundred.clear();
}

int64 CGameEveryColorTable::CalculateScore()
{
	m_mpUserWinScoreFlower.clear();
	m_mpUserWinScoreBigSmall.clear();
	m_mpUserLostScoreFlower.clear();
	m_mpUserLostScoreBigSmall.clear();
	m_mpUserWinScoreToatl.clear();
	m_mpUserWinScoreFee.clear();

	memset(m_areaHitPrize, false, sizeof(m_areaHitPrize));
	
	uint8 cbCardColor = GetCardColor(m_cbCardData);
	uint8 cbCardValue = GetCardValue(m_cbCardData);

	if (cbCardColor ==0x30 )	{
		m_areaHitPrize[AreaIndex_Spade] = true;
	}
	if (cbCardColor == 0x20) {
		m_areaHitPrize[AreaIndex_Heart] = true;
	}
	if (cbCardColor == 0x10) {
		m_areaHitPrize[AreaIndex_Club] = true;
	}
	if (cbCardColor == 0x00) {
		m_areaHitPrize[AreaIndex_Diamond] = true;
	}

	if (cbCardValue >7) {
		m_areaHitPrize[AreaIndex_Big] = true;
	}
	if (cbCardValue ==7) {
		m_areaHitPrize[AreaIndex_Seven] = true;
	}
	if (cbCardValue < 7) {
		m_areaHitPrize[AreaIndex_Small] = true;
	}

	int64 lBankerWinScore = 0;

	bool bIsHitFlower = false;
	bool bIsHitBigSmall = false;

	int64 lTempJackpotFlower = 0;
	int64 lTempJackpotBigSmall = 0;

	int64 lFlowerTotalJettonScore = 0;
	int64 lFlowerHitJettonScore = 0;

	for (int i = 0; i <=AreaIndex_Diamond; i++)
	{
		lFlowerTotalJettonScore += m_areaJettonScore[i];
		if (m_areaHitPrize[i])
		{
			lFlowerHitJettonScore += m_areaJettonScore[i];
		}
	}
	
	LOG_DEBUG("m_strPeriods:%s,m_cbCardData:0x%02X,cbCardColor:%d,cbCardValue:%d,m_lJackpotFlower:%lld,lFlowerTotalJettonScore:%lld,lFlowerHitJettonScore:%lld,m_areaHitPrize:%d %d %d %d %d %d %d", m_strPeriods.c_str(), m_cbCardData, cbCardColor, cbCardValue, m_lJackpotFlower,lFlowerTotalJettonScore, lFlowerHitJettonScore,m_areaHitPrize[0], m_areaHitPrize[1], m_areaHitPrize[2], m_areaHitPrize[3], m_areaHitPrize[4], m_areaHitPrize[5], m_areaHitPrize[6]);

	for (uint32 uIndex = 0; uIndex < m_userJettonSuccess.size(); uIndex++)
	{
		uint32 uid = m_userJettonSuccess[uIndex];
		for (int nAreaIndex = 0; nAreaIndex < EVERY_COLOR_AREA_COUNT; nAreaIndex++)
		{
			if (m_userJettonScore[nAreaIndex][uid] == 0)
			{
				continue;
			}
			int64 lTempWinScore = 0;
			int64 lAreaMultiple = 1;
			int64 lFeeScore = 0;
			if (m_areaHitPrize[nAreaIndex])
			{
				lAreaMultiple = s_lAreaMultiple[nAreaIndex];
				lTempWinScore = lAreaMultiple*m_userJettonScore[nAreaIndex][uid];

				
				if (nAreaIndex <= AreaIndex_Diamond)
				{
					bIsHitFlower = true;
					int64 lTempWinScoreCoin = m_lJackpotFlower + lFlowerTotalJettonScore;
					double fTempWinScorePer = double((double)lTempWinScoreCoin / (double)lFlowerHitJettonScore);
					lTempWinScore = fTempWinScorePer*lTempWinScore;

					//*((PRO_DENO_10000 - m_iFlowerColorTax) / PRO_DENO_10000);
					//double fRevenuePer = double(m_iFlowerColorTax / PRO_DENO_10000);
					lFeeScore = -(lTempWinScore * m_iFlowerColorTax / PRO_DENO_10000);
					lTempWinScore += lFeeScore;
					m_mpUserWinScoreFlower[uid] += lTempWinScore;

					LOG_DEBUG("cacl flower - m_strPeriods:%s,m_cbCardData:0x%02X,cbCardColor:%d,cbCardValue:%d,m_iFlowerColorTax:%d,lFeeScore:%lld,lFlowerTotalJettonScore:%lld,lFlowerHitJettonScore:%lld,lTempWinScore:%lld,lTempWinScoreCoin:%lld,fTempWinScorePer:%f,m_userJettonScore[%d][%d]:%lld,m_areaHitPrize:%d %d %d %d %d %d %d", m_strPeriods.c_str(), m_cbCardData, cbCardColor, cbCardValue, m_iFlowerColorTax, lFeeScore,lFlowerTotalJettonScore, lFlowerHitJettonScore, lTempWinScore, lTempWinScoreCoin, fTempWinScorePer, nAreaIndex, uid,m_userJettonScore[nAreaIndex][uid],m_areaHitPrize[0], m_areaHitPrize[1], m_areaHitPrize[2], m_areaHitPrize[3], m_areaHitPrize[4], m_areaHitPrize[5], m_areaHitPrize[6]);
				}
				else
				{
					bIsHitBigSmall = true;
					
					lTempJackpotBigSmall += (-(lTempWinScore - m_userJettonScore[nAreaIndex][uid]));
					//double fRevenuePer = double(m_ibigSmallTax / PRO_DENO_10000);
					lFeeScore = -(lTempWinScore * m_ibigSmallTax / PRO_DENO_10000);
					lTempWinScore += lFeeScore;
					m_mpUserWinScoreBigSmall[uid] += lTempWinScore;

					LOG_DEBUG("cacl bigsmall - m_strPeriods:%s,m_cbCardData:0x%02X,cbCardColor:%d,cbCardValue:%d,m_ibigSmallTax:%d,lFlowerTotalJettonScore:%lld,lFlowerHitJettonScore:%lld,lTempWinScore:%lld,lTempJackpotBigSmall:%lld,m_userJettonScore[%d][%d]:%lld,m_areaHitPrize:%d %d %d %d %d %d %d", m_strPeriods.c_str(), m_cbCardData, cbCardColor, cbCardValue, m_ibigSmallTax, lFlowerTotalJettonScore, lFlowerHitJettonScore, lTempWinScore, lTempJackpotBigSmall, nAreaIndex, uid, m_userJettonScore[nAreaIndex][uid], m_areaHitPrize[0], m_areaHitPrize[1], m_areaHitPrize[2], m_areaHitPrize[3], m_areaHitPrize[4], m_areaHitPrize[5], m_areaHitPrize[6]);
				}

				m_mpUserWinScoreToatl[uid] += lTempWinScore;
				m_mpUserWinScoreFee[uid] += (-lFeeScore);
				lBankerWinScore -= lTempWinScore;
			}
			else
			{
				int64 lTempLostScore = m_userJettonScore[nAreaIndex][uid];
				lBankerWinScore += lTempLostScore;
				if (nAreaIndex<= AreaIndex_Diamond)
				{
					lTempJackpotFlower += lTempLostScore;
					m_mpUserLostScoreFlower[uid] += lTempLostScore;
				}
				else
				{
					lTempJackpotBigSmall += lTempLostScore;
					m_mpUserLostScoreBigSmall[uid] += lTempLostScore;
				}
			}
			WriteAddScoreLog(uid, nAreaIndex, m_userJettonScore[nAreaIndex][uid]);
		}
	}
	m_lFrontJackpotBigSmall = m_lJackpotBigSmall;
	m_lJackpotFlower += lTempJackpotFlower;
	m_lJackpotBigSmall += lTempJackpotBigSmall;

	if (bIsHitFlower)
	{
		m_lJackpotFlower = 0;
	}
	LOG_DEBUG("cacl - m_strPeriods:%s,lTempJackpotFlower:%lld,lTempJackpotBigSmall:%lld,m_lJackpotFlower:%lld,m_lJackpotBigSmall:%lld,m_lFrontJackpotBigSmall:%lld",
		m_strPeriods.c_str(),lTempJackpotFlower, lTempJackpotBigSmall, m_lJackpotFlower, m_lJackpotBigSmall, m_lFrontJackpotBigSmall);

	return lBankerWinScore;
}

//游戏结束
bool    CGameEveryColorTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    LOG_DEBUG("game end:%d--%d",chairID,reason);

	switch(reason)
	{
	case GER_NORMAL:
		{
			//获取扑克
			GetOpenCard();
			//结算分数
			int64 lBankerWinScore = 0; CalculateScore();

			WriteCardDataInfo();
			

			LOG_DEBUG("m_strPeriods:%s,m_cbCardData:0x%02X,m_lJackpotBigSmall:%lld,m_lJackpotFlower:%lld,lBankerWinScore:%lld,m_areaJettonScore:%lld %lld %lld %lld %lld %lld %lld,m_areaHitPrize:%d %d %d %d %d %d %d", m_strPeriods.c_str(), m_cbCardData, m_lJackpotBigSmall, m_lJackpotFlower, lBankerWinScore, m_areaJettonScore[0], m_areaJettonScore[1], m_areaJettonScore[2], m_areaJettonScore[3], m_areaJettonScore[4], m_areaJettonScore[5], m_areaJettonScore[6], m_areaHitPrize[0], m_areaHitPrize[1], m_areaHitPrize[2], m_areaHitPrize[3], m_areaHitPrize[4], m_areaHitPrize[5], m_areaHitPrize[6]);

			for (uint32 uIndex = 0; uIndex < m_userJettonSuccess.size(); uIndex++)
			{
				uint32 uid = m_userJettonSuccess[uIndex];

				//int64 lUserScoreFree = CalcPlayerInfo(uid, m_mpUserWinScore[uid]);
				int64 lSubScore = m_mpUserWinScoreToatl[uid];
				int64 lPlayerJettonScore = 0;
				for (int iArea = 0; iArea < EVERY_COLOR_AREA_COUNT; iArea++)
				{
					lPlayerJettonScore += m_userJettonScore[iArea][uid];
				}
				int64 lPlayerWinScore = lSubScore - lPlayerJettonScore;
				FlushUserNoExistBlingLog(uid, lPlayerWinScore, m_mpUserWinScoreFee[uid]);
				LOG_DEBUG("user jetton total - lPlayerWinScore:%lld,m_strPeriods:%s,m_cbCardData:0x%02X,uid:%d,uIndex:%d,lSubScore:%lld,lPlayerJettonScore:%lld,m_userJettonScore:%lld %lld %lld %lld %lld %lld %lld", lPlayerWinScore,m_strPeriods.c_str(), m_cbCardData, uid, uIndex, lSubScore, lPlayerJettonScore, m_userJettonScore[0][uid], m_userJettonScore[1][uid], m_userJettonScore[2][uid], m_userJettonScore[3][uid], m_userJettonScore[4][uid], m_userJettonScore[5][uid], m_userJettonScore[6][uid]);
				if (lSubScore == 0) continue;
				ChangeScoreValueInGame(uid, lSubScore, emACCTRAN_OPER_TYPE_EVERY_COLOR_UPDATE_SCORE, m_pHostRoom->GetGameType(),m_chessid);

				string title = "时时彩获奖";
				string nickname = "系统提示";
				string content = CStringUtility::FormatToString("恭喜你在%s期的时时彩 - 押大小中获得%.2F筹码奖励，请注意查收。", m_strPeriods.c_str(), (lPlayerWinScore*0.01));
				CDBMysqlMgr::Instance().SendMail(0, uid, title, content, nickname);

			}

			LOG_DEBUG("1 - m_strPeriods:%s,m_lJackpotBigSmall:%lld,m_lRandMaxScore:%lld,m_lRandMinScore:%lld,m_lFrontJackpotBigSmall:%lld,m_iSysLostPro:%d,m_iSysWinPro:%d",
				m_strPeriods.c_str(), m_lJackpotBigSmall, m_lRandMaxScore, m_lRandMinScore, m_lFrontJackpotBigSmall, m_iSysLostPro, m_iSysWinPro);


			//吃币吐币概率变化
			if ((m_lJackpotBigSmall>m_lRandMaxScore || m_lJackpotBigSmall<m_lRandMinScore) && m_lUpdateJackpotScore>0)
			{
				LOG_DEBUG("1 - m_strPeriods:%s,m_lJackpotBigSmall:%lld,m_lRandMaxScore:%lld,m_lRandMinScore:%lld,m_lFrontJackpotBigSmall:%lld,m_iSysLostPro:%d,m_iSysWinPro:%d",
					m_strPeriods.c_str(), m_lJackpotBigSmall, m_lRandMaxScore, m_lRandMinScore, m_lFrontJackpotBigSmall, m_iSysLostPro, m_iSysWinPro);


				int64 lDiffScore =  m_lJackpotBigSmall - m_lFrontJackpotBigSmall;

				int iLoopCount = 0;

				if (m_lJackpotBigSmall>m_lRandMaxScore) //吐币
				{

					if (lDiffScore > 0 && lDiffScore >= m_lUpdateJackpotScore) //奖池分数增加 吐币增加
					{
						iLoopCount = 0;
						while (lDiffScore >= m_lUpdateJackpotScore)
						{
							iLoopCount++;
							if (iLoopCount > MAX_LOOP_COUNT)
							{
								LOG_DEBUG("loop error - lDiffScore:%lld,m_lUpdateJackpotScore：%lld", lDiffScore, m_lUpdateJackpotScore);
								break;
							}
							if (m_iSysLostPro + m_uSysLostWinProChange < PRO_DENO_10000)
							{
								m_iSysLostPro += m_uSysLostWinProChange;
							}
							else if (m_iSysLostPro + m_uSysLostWinProChange >= PRO_DENO_10000)
							{
								m_iSysLostPro = PRO_DENO_10000;
							}
							lDiffScore -= m_lUpdateJackpotScore;
						}
					}
					else if (lDiffScore < 0 && (-lDiffScore) >= m_lUpdateJackpotScore) //奖池分数减少 吐币减少
					{
						lDiffScore = (-lDiffScore);
						iLoopCount = 0;
						while (lDiffScore >= m_lUpdateJackpotScore)
						{
							iLoopCount++;
							if (iLoopCount > MAX_LOOP_COUNT)
							{
								LOG_DEBUG("loop error - lDiffScore:%lld,m_lUpdateJackpotScore：%lld", lDiffScore, m_lUpdateJackpotScore);
								break;
							}
							if (m_iSysLostPro > m_uSysLostWinProChange)
							{
								m_iSysLostPro -= m_uSysLostWinProChange;
							}
							else if (m_iSysLostPro <= m_uSysLostWinProChange)
							{
								m_iSysLostPro = 0;
							}
							lDiffScore -= m_lUpdateJackpotScore;
						}
					}
				}
				else if (m_lJackpotBigSmall<m_lRandMinScore) //吃币
				{
					if (lDiffScore < 0 && (-lDiffScore) >= m_lUpdateJackpotScore) // 奖池分数减少 吃币增加
					{
						lDiffScore = (-lDiffScore);
						iLoopCount = 0;
						while (lDiffScore >= m_lUpdateJackpotScore)
						{
							iLoopCount++;
							if (iLoopCount > MAX_LOOP_COUNT)
							{
								LOG_DEBUG("loop error - lDiffScore:%lld,m_lUpdateJackpotScore：%lld", lDiffScore, m_lUpdateJackpotScore);
								break;
							}
							if (m_iSysWinPro + m_uSysLostWinProChange < PRO_DENO_10000)
							{
								m_iSysWinPro += m_uSysLostWinProChange;
							}
							else if (m_iSysWinPro + m_uSysLostWinProChange >= PRO_DENO_10000)
							{
								m_iSysWinPro = PRO_DENO_10000;
							}
							lDiffScore -= m_lUpdateJackpotScore;
						}
					}
					else if (lDiffScore > 0 && lDiffScore >= m_lUpdateJackpotScore)// 奖池分数增加 吃币减少
					{
						iLoopCount = 0;
						while (lDiffScore >= m_lUpdateJackpotScore)
						{
							iLoopCount++;
							if (iLoopCount > MAX_LOOP_COUNT)
							{
								LOG_DEBUG("loop error - lDiffScore:%lld,m_lUpdateJackpotScore：%lld", lDiffScore, m_lUpdateJackpotScore);
								break;
							}
							if (m_iSysWinPro > m_uSysLostWinProChange)
							{
								m_iSysWinPro -= m_uSysLostWinProChange;
							}
							else if (m_iSysWinPro <= m_uSysLostWinProChange)
							{
								m_iSysWinPro = 0;
							}
							lDiffScore -= m_lUpdateJackpotScore;
						}
					}
				}
			}

			LOG_DEBUG("2 - m_strPeriods:%s,m_lJackpotBigSmall:%lld,m_lRandMaxScore:%lld,m_lRandMinScore:%lld,m_lFrontJackpotBigSmall:%lld,m_iSysLostPro:%d,m_iSysWinPro:%d",
				m_strPeriods.c_str(), m_lJackpotBigSmall, m_lRandMaxScore, m_lRandMinScore, m_lFrontJackpotBigSmall, m_iSysLostPro, m_iSysWinPro);


            net::msg_everycolor_game_end msg;
			msg.set_table_cards(m_cbCardData);
			msg.set_time_leave(m_coolLogic.getCoolTick());
			map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
			for (; it != m_mpLookers.end(); ++it)
			{
				CGamePlayer* pPlayer = it->second;
				if (pPlayer == NULL)continue;
				uint32 uid = pPlayer->GetUID();
				//if (!IsJettonSuccess(uid))continue;

				msg.set_win_score_flower(m_mpUserWinScoreFlower[uid]);
				msg.set_win_score_bigsmall(m_mpUserWinScoreBigSmall[uid]);

				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_EVERY_COLOR_GAME_END);				
			}
			WriteAreaInfo();
			SaveBlingLog();

		}break;
    default:
        break;
	}
	return false;
}

bool    CGameEveryColorTable::OnGameEndTen(uint16 chairID, uint8 reason)
{
	switch (reason)
	{
	case GER_NORMAL:
	{
		OpenLotterySnatchCoinTen();
		CalculateSnatchCoinTen();

		int64 lPlayerWinScore = 54000;
		int64 lFeeScore = -(lPlayerWinScore * 500 / PRO_DENO_10000);
		lPlayerWinScore += lFeeScore;

		FlushUserBlingLogByUidTen(m_PlayerWinSnatchCoinTen, lPlayerWinScore, lFeeScore);

		ChangeScoreValueInGame(m_PlayerWinSnatchCoinTen, lPlayerWinScore, emACCTRAN_OPER_TYPE_EVERY_COLOR_UPDATE_SCORE, m_pHostRoom->GetGameType(), m_blingLogTen.chessid);

		net::msg_everycolor_snatch_coin_game_end end;
		end.set_snatch_type(SnatchCoinType_Ten);
		end.set_table_cards(m_cbSnatchCoinCardDataTen);
		end.set_win_uid(m_PlayerWinSnatchCoinTen);
		end.set_win_score(lPlayerWinScore);
		end.set_periods_num(m_strPeriodsTen.c_str());
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			pPlayer->SendMsgToClient(&end, net::S2C_MSG_EVERY_COLOR_TEN_GAME_END);
		}

		string title = "时时彩获奖";
		string nickname = "系统提示";
		string content = CStringUtility::FormatToString("恭喜你在%s期的时时彩 - 十元夺宝中获得大奖，实际到账%.2F筹码，请注意查收。", end.periods_num().c_str(), (lPlayerWinScore*0.01));
		CDBMysqlMgr::Instance().SendMail(0, m_PlayerWinSnatchCoinTen, title, content, nickname);

		WriteCardDataInfoTen();
		SaveBlingLogTen();
	}break;
	default:
		break;
	}
	return false;
}

bool    CGameEveryColorTable::OnGameEndHundred(uint16 chairID, uint8 reason)
{
	switch (reason)
	{
	case GER_NORMAL:
	{
		OpenLotterySnatchCoinHundred();
		CalculateSnatchCoinHundred();

		int64 lPlayerWinScore = 540000;
		int64 lFeeScore = -(lPlayerWinScore * 500 / PRO_DENO_10000);
		lPlayerWinScore += lFeeScore;

		FlushUserBlingLogByUidHundred(m_PlayerWinSnatchCoinHundred, lPlayerWinScore, lFeeScore);

		ChangeScoreValueInGame(m_PlayerWinSnatchCoinHundred, lPlayerWinScore, emACCTRAN_OPER_TYPE_EVERY_COLOR_UPDATE_SCORE, m_pHostRoom->GetGameType(), m_blingLogHundred.chessid);

		net::msg_everycolor_snatch_coin_game_end end;
		end.set_snatch_type(SnatchCoinType_Hundred);
		end.set_table_cards(m_cbSnatchCoinCardDataHundred);
		end.set_win_uid(m_PlayerWinSnatchCoinHundred);
		end.set_win_score(lPlayerWinScore);
		end.set_periods_num(m_strPeriodsHundred.c_str());
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			pPlayer->SendMsgToClient(&end, net::S2C_MSG_EVERY_COLOR_HUNDRED_GAME_END);
		}

		string title = "时时彩获奖";
		string nickname = "系统提示";
		string content = CStringUtility::FormatToString("恭喜你在%s期的时时彩 - 百元夺宝中获得大奖，实际到账%.2F筹码，请注意查收。", end.periods_num().c_str(), (lPlayerWinScore*0.01));
		CDBMysqlMgr::Instance().SendMail(0, m_PlayerWinSnatchCoinHundred, title, content, nickname);

		WriteCardDataInfoHundred();
		SaveBlingLogHundred();

	}break;
	default:
		break;
	}
	return false;
}

void   CGameEveryColorTable::OpenLotterySnatchCoinTen()
{
	vector<BYTE> vecRobotCards;
	map<uint32, vector<BYTE>>::iterator it_SnatchCoinTen = m_userSnatchCoinTen.begin();
	for (; it_SnatchCoinTen != m_userSnatchCoinTen.end(); ++it_SnatchCoinTen)
	{
		uint32 tmpuid = it_SnatchCoinTen->first;
		CGamePlayer * pPlayer = GetPlayer(tmpuid);
		if (pPlayer != NULL && pPlayer->IsRobot())
		{
			for (uint32 i = 0; i < m_userSnatchCoinTen[tmpuid].size(); i++)
			{
				vecRobotCards.push_back(m_userSnatchCoinTen[tmpuid][i]);
			}
		}
	}

	double fRobotCardCount = (double)vecRobotCards.size();
	double fRobotConfigPre = (double)m_uSnatchCoinRobotWinPre;
	double fRobotWinPre = 1 / 54 * fRobotCardCount*(1 + (fRobotConfigPre / (double)PRO_DENO_10000));
	uint32 uRobotWinPre = fRobotWinPre * PRO_DENO_10000;

	bool bFlag = g_RandGen.RandRatio(uRobotWinPre, PRO_DENO_10000);



	if (bFlag)
	{
		uint16 pos = g_RandGen.RandRange(0, vecRobotCards.size() - 1);
		m_cbSnatchCoinCardDataTen = vecRobotCards[pos];
	}
	else
	{
		BYTE cbCardListData[EVERY_COLOR_SNATCH_COIN_CARD_COUNT];
		memcpy(cbCardListData, g_cbEveryColorCardListData, sizeof(g_cbEveryColorCardListData));
		BYTE cbPosition = g_RandGen.RandUInt() % (EVERY_COLOR_SNATCH_COIN_CARD_COUNT);
		m_cbSnatchCoinCardDataTen = cbCardListData[cbPosition];
	}
	LOG_DEBUG("fRobotCardCount:%f,fRobotConfigPre:%f,fRobotWinPre:%f,uRobotWinPre:%d,bFlag:%d,card:0x%02X",
		fRobotCardCount, fRobotConfigPre, fRobotWinPre, uRobotWinPre, bFlag, m_cbSnatchCoinCardDataTen);

	return;
}

void   CGameEveryColorTable::OpenLotterySnatchCoinHundred()
{
	vector<BYTE> vecRobotCards;
	map<uint32, vector<BYTE>>::iterator it_SnatchCoinHundred = m_userSnatchCoinHundred.begin();
	for (; it_SnatchCoinHundred != m_userSnatchCoinHundred.end(); ++it_SnatchCoinHundred)
	{
		uint32 tmpuid = it_SnatchCoinHundred->first;
		CGamePlayer * pPlayer = GetPlayer(tmpuid);
		if (pPlayer != NULL && pPlayer->IsRobot())
		{
			for (uint32 i = 0; i < m_userSnatchCoinHundred[tmpuid].size(); i++)
			{
				vecRobotCards.push_back(m_userSnatchCoinHundred[tmpuid][i]);
			}
		}
	}

	double fRobotCardCount = (double)vecRobotCards.size();
	double fRobotConfigPre = (double)m_uSnatchCoinRobotWinPre;
	double fRobotWinPre = 1 / 54 * fRobotCardCount*(1 + (fRobotConfigPre / (double)PRO_DENO_10000));
	uint32 uRobotWinPre = fRobotWinPre * PRO_DENO_10000;

	bool bFlag = g_RandGen.RandRatio(uRobotWinPre, PRO_DENO_10000);
	if (bFlag)
	{
		uint16 pos = g_RandGen.RandRange(0, vecRobotCards.size() - 1);
		m_cbSnatchCoinCardDataHundred = vecRobotCards[pos];
	}
	else
	{
		BYTE cbCardListData[EVERY_COLOR_SNATCH_COIN_CARD_COUNT];
		memcpy(cbCardListData, g_cbEveryColorCardListData, sizeof(g_cbEveryColorCardListData));
		BYTE cbPosition = g_RandGen.RandUInt() % (EVERY_COLOR_SNATCH_COIN_CARD_COUNT);
		m_cbSnatchCoinCardDataHundred = cbCardListData[cbPosition];
	}
	LOG_DEBUG("fRobotCardCount:%f,fRobotConfigPre:%f,fRobotWinPre:%f,uRobotWinPre:%d,bFlag:%d,card:0x%02X",
		fRobotCardCount, fRobotConfigPre, fRobotWinPre, uRobotWinPre, bFlag, m_cbSnatchCoinCardDataHundred);

	return;
}

void   CGameEveryColorTable::CalculateSnatchCoinTen()
{
	map<uint32, vector<BYTE>>::iterator it_SnatchCoinTen = m_userSnatchCoinTen.begin();
	for (; it_SnatchCoinTen != m_userSnatchCoinTen.end(); ++it_SnatchCoinTen)
	{
		uint32 tmpuid = it_SnatchCoinTen->first;

		//uSnatchCoinCountTen += m_userSnatchCoinTen[tmpuid].size();

		for (uint32 i = 0; i < m_userSnatchCoinTen[tmpuid].size(); i++)
		{
			if (m_userSnatchCoinTen[tmpuid][i] == m_cbSnatchCoinCardDataTen)
			{
				m_PlayerWinSnatchCoinTen = tmpuid;
				return;
			}
		}
	}
	//牌局出错 记录排查
}

void   CGameEveryColorTable::CalculateSnatchCoinHundred()
{
	map<uint32, vector<BYTE>>::iterator it_SnatchCoinHundred = m_userSnatchCoinHundred.begin();
	for (; it_SnatchCoinHundred != m_userSnatchCoinHundred.end(); ++it_SnatchCoinHundred)
	{
		uint32 tmpuid = it_SnatchCoinHundred->first;

		//uSnatchCoinCountTen += m_userSnatchCoinTen[tmpuid].size();

		for (uint32 i = 0; i < m_userSnatchCoinHundred[tmpuid].size(); i++)
		{
			if (m_userSnatchCoinHundred[tmpuid][i] == m_cbSnatchCoinCardDataHundred)
			{
				m_PlayerWinSnatchCoinHundred = tmpuid;
				return;
			}
		}
	}
	//牌局出错 记录排查
}


bool CGameEveryColorTable::GetOpenCard()
{
	BYTE cbArCardColor[] = { 0x00,0x10,0x20,0x30 };
	BYTE cbArCardValue[] = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D };
	BYTE cbPosition = g_RandGen.RandRange(0, 3);
	BYTE cbCardColor = cbArCardColor[cbPosition];
	BYTE cbCardValue = 0;

	int64 lScoreBig = 0;
	int64 lScoreSmall = 0;

	lScoreBig = m_areaJettonScore[AreaIndex_Big];
	lScoreSmall = m_areaJettonScore[AreaIndex_Small];

	//for (uint32 uIndex = 0; uIndex < m_userJettonSuccess.size(); uIndex++)
	//{
	//	uint32 uid = m_userJettonSuccess[uIndex];
	//	if (m_userJettonScore[AreaIndex_Big][uid] == 0 && m_userJettonScore[AreaIndex_Small][uid] == 0)
	//	{
	//		continue;
	//	}
	//	lScoreBig += m_userJettonScore[AreaIndex_Big][uid];
	//	lScoreSmall += m_userJettonScore[AreaIndex_Small][uid];
	//}
	bool bSysWinPro = g_RandGen.RandRatio(m_iSysWinPro, PRO_DENO_10000);
	bool bSysLostPro = g_RandGen.RandRatio(m_iSysLostPro, PRO_DENO_10000);

	if (m_ibigSmallSwitch == 0 || lScoreBig == lScoreSmall) //大小开关关闭 和 大小下注相等 随机开大小
	{
		cbPosition = g_RandGen.RandRange(0, 12);
		cbCardValue = cbArCardValue[cbPosition];
	}
	else
	{
		if (m_lJackpotBigSmall > m_lRandMinScore && m_lJackpotBigSmall < m_lRandMaxScore)
		{
			cbPosition = g_RandGen.RandRange(0, 12);
			cbCardValue = cbArCardValue[cbPosition];
		}
		else
		{
			if (m_lJackpotBigSmall < m_lRandMinScore && bSysWinPro)
			{
				//吃币
				if (lScoreBig > lScoreSmall)
				{
					//开小
					cbPosition = g_RandGen.RandRange(0, 5);
					cbCardValue = cbArCardValue[cbPosition];
				}
				else
				{
					//开大
					cbPosition = g_RandGen.RandRange(7, 12);
					cbCardValue = cbArCardValue[cbPosition];
				}
			}
			if (m_lJackpotBigSmall > m_lRandMaxScore && bSysLostPro)
			{
				//吐币
				if (lScoreBig > lScoreSmall)
				{
					//开大
					cbPosition = g_RandGen.RandRange(7, 12);
					cbCardValue = cbArCardValue[cbPosition];
				}
				else
				{
					//开小
					cbPosition = g_RandGen.RandRange(0, 5);
					cbCardValue = cbArCardValue[cbPosition];
				}
			}
		}
	}

	if (cbCardValue == 0 || cbCardValue < 0x01 || cbCardValue>0x0D)
	{
		cbPosition = g_RandGen.RandRange(0, 12);
		cbCardValue = cbArCardValue[cbPosition];	
	}

	m_cbCardData = cbCardColor + cbCardValue;

	LOG_DEBUG("get card - m_strPeriods:%s,cbPosition:%02d,m_cbCardData:0X%02x,cbCardValue:0X%02x,cbCardColor:0X%02x,lScoreBig:%lld,lScoreSmall:%lld,bSysWinPro:%d,bSysLostPro:%d,m_iSysWinPro:%d,m_iSysLostPro:%d,m_lJackpotBigSmall:%lld,m_lRandMinScore:%lld,m_lRandMaxScore:%lld",
		m_strPeriods.c_str(), cbPosition,m_cbCardData, cbCardValue, cbCardColor, lScoreBig, lScoreSmall, bSysWinPro, bSysLostPro, m_iSysWinPro, m_iSysLostPro, m_lJackpotBigSmall, m_lRandMinScore, m_lRandMaxScore);


	return true;
}

//用户同意
bool    CGameEveryColorTable::OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer)
{
    return true;
}
//玩家进入或离开
void    CGameEveryColorTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
	if (pPlayer == NULL){
		return;
	}
	LOG_DEBUG("isJoin:%d,chairID:%d,uid:%d,m_mpLookers.size:%d", isJoin, chairID, pPlayer->GetUID(), m_mpLookers.size());
    if(isJoin){
		AddEveryColorLooker(pPlayer);
        SendGameScene(pPlayer);
    }else{
		RemoveEveryColorLooker(pPlayer);
    }
}
// 发送场景信息(断线重连)
void    CGameEveryColorTable::SendGameScene(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		LOG_DEBUG("send game scene pPlayer is null.");
		return;
	}
	uint32 uid = pPlayer->GetUID();

	switch(m_gameState)
	{
	//case net::TABLE_STATE_FREE: //空闲状态
	//	{
 //           net::msg_everycolor_game_info_free_rep msg;
	//		msg.set_periods_num(m_strPeriods.c_str());
 //           msg.set_time_leave(m_coolLogic.getCoolTick());

 //           pPlayer->SendMsgToClient(&msg,net::S2C_MSG_EVERY_COLOR_GAME_FREE_INFO);

	//	}break;
	case net::TABLE_STATE_FREE:
    case net::TABLE_STATE_CALL:
    case net::TABLE_STATE_WAIT:
	case net::TABLE_STATE_PLAY:	//游戏状态
		{
            net::msg_everycolor_game_info_play_rep msg;
			msg.set_periods_num(m_strPeriods.c_str());
            msg.set_time_leave(m_coolLogic.getCoolTick());
			msg.set_table_cards(m_cbCardData);
			msg.set_game_status(GetGameState());

			for (int i = 0; i < EVERY_COLOR_AREA_COUNT; i++) {
				msg.add_self_jetton_score(m_userJettonScore[i][uid]);
			}

			//LOG_DEBUG("m_strPeriods:%s,uid:%d,m_userJettonScore:%lld %lld %lld %lld %lld %lld %lld",m_strPeriods uid, m_userJettonScore[0][uid], m_userJettonScore[1][uid], m_userJettonScore[2][uid], m_userJettonScore[3][uid], m_userJettonScore[4][uid], m_userJettonScore[5][uid], m_userJettonScore[6][uid]);
			
			for (int i = 0; i < EVERY_COLOR_AREA_COUNT; i++) {
				msg.add_total_jetton_score(m_areaJettonScore[i]);
			}

			if (GetGameState()==net::TABLE_STATE_FREE)
			{
				map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
				for (; it != m_mpLookers.end(); ++it)
				{
					CGamePlayer* pPlayer = it->second;
					if (pPlayer == NULL)continue;
					uint32 tempuid = pPlayer->GetUID();
					
					//if (!IsJettonSuccess(uid))continue;
					if (uid == tempuid)
					{
						msg.set_win_score_flower(m_mpUserWinScoreFlower[uid]);
						msg.set_win_score_bigsmall(m_mpUserWinScoreBigSmall[uid]);
						
						break;
					}
				}
			}
			for (int i = 0; i < getArrayLen(s_lAreaMultiple); i++)
			{
				msg.add_area_multiple(s_lAreaMultiple[i]);
			}
			msg.set_basescore(m_conf.baseScore);

			msg.set_snatch_coin_stop(m_uSnatchCoinStop);
			msg.set_periods_num_ten(m_strPeriodsTen.c_str());
			msg.set_periods_num_hundred(m_strPeriodsHundred.c_str());
			msg.set_game_status_ten(m_gameStateTen);
			msg.set_game_status_hundred(m_gameStateHundred);
			msg.set_residue_card_ten(m_poolCardsTen.size());
			msg.set_residue_card_hundred(m_poolCardsHundred.size());

			LOG_DEBUG("uid:%d,size:%d %d,residue:%d %d",
				uid, m_poolCardsTen.size(), m_poolCardsHundred.size(), msg.residue_card_ten(),msg.residue_card_hundred());

			auto it_player_ten = m_userSnatchCoinTen.find(uid);
			if (it_player_ten != m_userSnatchCoinTen.end())
			{
				for (uint32 i = 0; i < m_userSnatchCoinTen[uid].size(); i++)
				{
					msg.add_card_data_ten(m_userSnatchCoinTen[uid][i]);
				}
			}
			auto it_player_hundred = m_userSnatchCoinHundred.find(uid);
			if (it_player_hundred != m_userSnatchCoinHundred.end())
			{
				for (uint32 i = 0; i < m_userSnatchCoinHundred[uid].size(); i++)
				{
					msg.add_card_data_hundred(m_userSnatchCoinHundred[uid][i]);
				}
			}


			msg.set_time_leave_ten(m_coolLogicTen.getCoolTick());
			msg.set_time_leave_hundred(m_coolLogicHundred.getCoolTick());
			msg.set_win_uid_ten(m_PlayerWinSnatchCoinTen);
			msg.set_win_uid_hundred(m_PlayerWinSnatchCoinHundred);
			msg.set_win_score_ten(m_WinScoreSnatchCoinTen);
			msg.set_win_score_hundred(m_WinScoreSnatchCoinHundred);

            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_EVERY_COLOR_GAME_PLAY_INFO);
		}break;
	}    

}
// 重置游戏数据
void    CGameEveryColorTable::ResetGameData()
{
	m_userJettonSuccess.clear();
	for (int i = 0; i < EVERY_COLOR_AREA_COUNT; i++)
	{
		m_userJettonScore[i].clear();
		m_areaJettonScore[i] = 0;
	}
	memset(m_areaHitPrize,false,sizeof(m_areaHitPrize));
	m_mpUserWinScoreFlower.clear();
	m_mpUserWinScoreBigSmall.clear();
	m_mpUserLostScoreFlower.clear();
	m_mpUserLostScoreBigSmall.clear();
	m_mpUserWinScoreToatl.clear();
	m_mpUserWinScoreFee.clear();
	m_cbCardData = 0;
}

void    CGameEveryColorTable::ResetGameDataTen()
{
	m_poolCardsTen.clear();
	m_cbSnatchCoinCardDataTen = 0;
	m_userSnatchCoinTen.clear();
	m_PlayerWinSnatchCoinTen = 0;
	m_WinScoreSnatchCoinTen = 0;
	m_lLastTimeRobotOperSnatchCoinTen = getTickCount64();
}

void    CGameEveryColorTable::ResetGameDataHundred()
{	
	m_poolCardsHundred.clear();
	m_cbSnatchCoinCardDataHundred = 0;
	m_userSnatchCoinHundred.clear();
	m_PlayerWinSnatchCoinHundred = 0;
	m_WinScoreSnatchCoinHundred = 0;
	m_lLastTimeRobotOperSnatchCoinHundred = getTickCount64() + 2 * 60 * 1000;
}

// 是否能够开始游戏
bool    CGameEveryColorTable::IsCanStartGame()
{
    return m_bInitTableSuccess;
}

//最大下注
int64   CGameEveryColorTable::GetUserMaxJetton(CGamePlayer* pPlayer)
{
	int iTimer = 50;
	uint32 uid = pPlayer->GetUID();

	//已下注额
	int64 lNowJetton = 0;
	for (int nAreaIndex = 0; nAreaIndex < EVERY_COLOR_AREA_COUNT; ++nAreaIndex) {
		lNowJetton += m_userJettonScore[nAreaIndex][uid];
	}

	//个人限制
	int64 lMeMaxScore = (GetPlayerCurScore(pPlayer) - lNowJetton*iTimer) / iTimer;

	//非零限制
	lMeMaxScore = MAX(lMeMaxScore, 0);

	//LOG_DEBUG("user max jetton - uid:%d,curScore:%d,lMeMaxScore:%d", uid, GetPlayerCurScore(pPlayer), lMeMaxScore);

	return (lMeMaxScore);
}

// 检测筹码是否正确
bool    CGameEveryColorTable::CheckJetton(CGamePlayer* pPlayer,int64 lScore)
{
	uint32 uid = pPlayer->GetUID();
	int64 lJettonScore = 0;
	//for (int nAreaIndex = 0; nAreaIndex < EVERY_COLOR_AREA_COUNT; ++nAreaIndex)
	//	lJettonScore += m_userJettonScore[nAreaIndex][uid];
	int64 lUserScore = GetPlayerCurScore(pPlayer);
	if (lUserScore < lScore)
	{
		LOG_DEBUG("the jetton more than you have - uid:%d,lUserScore:%lld,lJettonScore:%lld,lScore:%lld", uid, lUserScore, lJettonScore, lScore);
		return false;
	}
	//if (GetUserMaxJetton(pPlayer)<lScore)
	//{
	//	return false;
	//}
    return true;
}
//加注事件
bool    CGameEveryColorTable::OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore)
{
	uint32 uid = pPlayer->GetUID();

    net::msg_everycolor_place_jetton_rep rep;
	rep.set_jetton_area(cbJettonArea);
    rep.set_jetton_score(lJettonScore);

	bool bCheckJetton = CheckJetton(pPlayer, lJettonScore);
	LOG_DEBUG("1 - uid:%d,cbJettonArea:%d,lJettonScore:%lld,bCheckJetton:%d,GetGameState:%d", uid, cbJettonArea, lJettonScore, bCheckJetton, GetGameState());

    if(!bCheckJetton || cbJettonArea>= EVERY_COLOR_AREA_COUNT || lJettonScore<=0 || GetGameState() != net::TABLE_STATE_PLAY) {
        rep.set_result(RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_EVERY_COLOR_PLACE_JETTON_REP);
        return false;
    }

	m_areaJettonScore[cbJettonArea] += lJettonScore;
	m_userJettonScore[cbJettonArea][uid] += lJettonScore;

    rep.set_result(net::RESULT_CODE_SUCCESS);

	for (int i = 0; i < EVERY_COLOR_AREA_COUNT; i++) {
		rep.add_self_jetton_score(m_userJettonScore[i][uid]);
	}
	
	//LOG_DEBUG("2 - uid:%d,cbJettonArea:%d,lJettonScore:%lld,bCheckJetton:%d,GetGameState:%d", uid, cbJettonArea, lJettonScore, bCheckJetton, GetGameState());

	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_EVERY_COLOR_PLACE_JETTON_REP);

	//LOG_DEBUG("3 - uid:%d,cbJettonArea:%d,lJettonScore:%lld,bCheckJetton:%d,GetGameState:%d", uid, cbJettonArea, lJettonScore, bCheckJetton, GetGameState());

    net::msg_everycolor_place_jetton_broadcast broad;
	broad.set_uid(uid);
	broad.set_jetton_area(cbJettonArea);
    broad.set_jetton_score(lJettonScore);
	//broad.set_total_jetton_score(m_areaJettonScore[cbJettonArea]);

	for (int i = 0; i < EVERY_COLOR_AREA_COUNT; i++) {
		broad.add_total_jetton_score(m_areaJettonScore[i]);
	}

	//LOG_DEBUG("4 - uid:%d,cbJettonArea:%d,lJettonScore:%lld,bCheckJetton:%d,GetGameState:%d", uid, cbJettonArea, lJettonScore, bCheckJetton, GetGameState());

    SendMsgToAll(&broad,net::S2C_MSG_EVERY_COLOR_PLACE_JETTON_BROADCAST);

	//LOG_DEBUG("5 - uid:%d,cbJettonArea:%d,lJettonScore:%lld,bCheckJetton:%d,GetGameState:%d", uid, cbJettonArea, lJettonScore, bCheckJetton, GetGameState());

	bool bFlag = IsExistJettonSuccess(uid);
	if (!bFlag)
	{
		AddUserBlingLog(pPlayer);
		m_userJettonSuccess.push_back(uid);
	}

	//下注成功直接扣钱
	int64 lSubScore = -lJettonScore;
	//LOG_DEBUG("6 - uid:%d,cbJettonArea:%d,lJettonScore:%lld,bCheckJetton:%d,GetGameState:%d", uid, cbJettonArea, lJettonScore, bCheckJetton, GetGameState());

	ChangeScoreValueInGame(uid, lSubScore, emACCTRAN_OPER_TYPE_EVERY_COLOR_UPDATE_SCORE, m_pHostRoom->GetGameType(),m_chessid);
	FlushUserBlingLog(pPlayer, 0);
	LOG_DEBUG("7 - uid:%d,cbJettonArea:%d,lJettonScore:%lld,bCheckJetton:%d,GetGameState:%d", uid, cbJettonArea, lJettonScore, bCheckJetton, GetGameState());
	return true;
}

//夺宝事件
bool    CGameEveryColorTable::OnUserSnatchCoin(CGamePlayer* pPlayer, BYTE cbType, BYTE cbCount)
{
	uint32 uid = pPlayer->GetUID();

	net::msg_everycolor_snatch_coin_rep rep;
	net::msg_everycolor_snatch_coin_broadcast broad;

	rep.set_snatch_type(cbType);
	rep.set_snatch_count(cbCount);
	int64 lSnatchScore = 0;
	if (cbType<=SnatchCoinType_Hundred)
	{
		if (cbType == SnatchCoinType_Ten)
		{
			if (cbCount <= m_poolCardsTen.size())
			{
				lSnatchScore = cbCount * 1000;
			}
			else
			{
				rep.set_result(2);
				pPlayer->SendMsgToClient(&rep, net::S2C_MSG_EVERY_COLOR_SNATCH_COIN_REP);
			}
		}
		else if (cbType == SnatchCoinType_Hundred)
		{
			if (cbCount <= m_poolCardsHundred.size())
			{
				lSnatchScore = cbCount * 10000;
			}
			else
			{
				rep.set_result(2);
				pPlayer->SendMsgToClient(&rep, net::S2C_MSG_EVERY_COLOR_SNATCH_COIN_REP);
			}
		}
	}
	bool bCheckJetton = CheckJetton(pPlayer, lSnatchScore);

	if (!bCheckJetton)
	{
		rep.set_result(3);
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_EVERY_COLOR_SNATCH_COIN_REP);
	}

	LOG_DEBUG("uid:%d,m_uSnatchCoinStop:%d,cbType:%d,cbCount:%d,lSnatchScore:%lld,bCheckJetton:%d,GetGameState:%d,card_size:%d, %d",
		uid, m_uSnatchCoinStop, cbType, cbCount, lSnatchScore, bCheckJetton, GetGameState(), m_poolCardsTen.size(), m_poolCardsHundred.size());


	if (m_uSnatchCoinStop == TRUE || !bCheckJetton || cbType > SnatchCoinType_Hundred || cbCount <= 0 || lSnatchScore <= 0 || GetGameState() != net::TABLE_STATE_PLAY)
	{
		rep.set_result(RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_EVERY_COLOR_SNATCH_COIN_REP);
		return false;
	}

	if (cbType == SnatchCoinType_Ten && cbCount <= m_poolCardsTen.size())
	{
		//if (m_poolCardsTen.size()<10)
		//{
		//	SetSnatchCoinState(1);
		//	return false;
		//}

		bool bIsExist = IsExistSnatchCoinTenSuccess(uid);
		if (bIsExist == false)
		{
			AddUserBlingLogTen(pPlayer);
		}
		for (BYTE i = 0; i < cbCount; i++)
		{
			BYTE cbCardData = PopCardFromPoolTen();
			LOG_DEBUG("PopCardFromPoolTen - uid:%d,bIsExist:%d,size:%d,cbCardData:0x%02X", uid, bIsExist, m_poolCardsTen.size(), cbCardData);
			m_userSnatchCoinTen[uid].push_back(cbCardData);
		}

		for (uint32 i = 0; i < m_userSnatchCoinTen[uid].size(); i++)
		{
			rep.add_self_snatch_card(m_userSnatchCoinTen[uid][i]);
		}
		rep.set_self_total_count(m_userSnatchCoinTen[uid].size());
		rep.set_residue_card_count(m_poolCardsTen.size());

		uint32 uSnatchCoinCountTen = 0;
		map<uint32, vector<BYTE>>::iterator it_SnatchCoinTen = m_userSnatchCoinTen.begin();
		for (; it_SnatchCoinTen != m_userSnatchCoinTen.end(); ++it_SnatchCoinTen)
		{
			uint32 tmpuid = it_SnatchCoinTen->first;
			uSnatchCoinCountTen += m_userSnatchCoinTen[tmpuid].size();

			for (uint32 i = 0; i < m_userSnatchCoinTen[tmpuid].size(); i++)
			{
				broad.add_total_snatch_card(m_userSnatchCoinTen[tmpuid][i]);
			}
		}
		broad.set_total_snatch_count(uSnatchCoinCountTen);
	}
	else if (cbType == SnatchCoinType_Hundred && cbCount <= m_poolCardsHundred.size())
	{
		if (IsExistSnatchCoinHundredSuccess(uid) == false)
		{
			AddUserBlingLogHundred(pPlayer);
		}
		for (BYTE i = 0; i < cbCount; i++)
		{
			BYTE cbCardData = PopCardFromPoolHundred();
			LOG_DEBUG("PopCardFromPoolHundred - uid:%d,size:%d,cbCardData:0x%02X", uid, m_poolCardsHundred.size(), cbCardData);
			m_userSnatchCoinHundred[uid].push_back(cbCardData);
		}

		for (uint32 i = 0; i < m_userSnatchCoinHundred[uid].size(); i++)
		{
			rep.add_self_snatch_card(m_userSnatchCoinHundred[uid][i]);
		}
		rep.set_self_total_count(m_userSnatchCoinHundred[uid].size());
		rep.set_residue_card_count(m_poolCardsHundred.size());

		uint32 uSnatchCoinCountHundred = 0;
		map<uint32, vector<BYTE>>::iterator it_SnatchCoinHundred = m_userSnatchCoinHundred.begin();
		for (; it_SnatchCoinHundred != m_userSnatchCoinHundred.end(); ++it_SnatchCoinHundred)
		{
			uint32 tmpuid = it_SnatchCoinHundred->first;
			uSnatchCoinCountHundred += m_userSnatchCoinHundred[tmpuid].size();

			for (uint32 i = 0; i < m_userSnatchCoinHundred[tmpuid].size(); i++)
			{
				broad.add_total_snatch_card(m_userSnatchCoinHundred[tmpuid][i]);
			}
		}
		broad.set_total_snatch_count(uSnatchCoinCountHundred);
	}
	rep.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_EVERY_COLOR_SNATCH_COIN_REP);

	
	broad.set_uid(uid);
	broad.set_snatch_type(cbType);
	broad.set_snatch_count(cbCount);
	SendMsgToAll(&broad, net::S2C_MSG_EVERY_COLOR_SNATCH_COIN_BROADCAST);

	LOG_DEBUG("jutten success - uid:%d,m_uSnatchCoinStop:%d,cbType:%d,cbCount:%d,lSnatchScore:%lld,bCheckJetton:%d,GetGameState:%d,card_size:%d, %d",
		uid, m_uSnatchCoinStop, cbType, cbCount, lSnatchScore, bCheckJetton, GetGameState(), m_poolCardsTen.size(), m_poolCardsHundred.size());


	//下注成功直接扣钱
	int64 lSubScore = -lSnatchScore;
	
	if (cbType == SnatchCoinType_Ten)
	{
		ChangeScoreValueInGame(uid, lSubScore, emACCTRAN_OPER_TYPE_EVERY_COLOR_UPDATE_SCORE, m_pHostRoom->GetGameType(), m_blingLogTen.chessid);


		FlushUserBlingLogTen(pPlayer);

		if (m_poolCardsTen.size() == 0)
		{
			OnGameEndTen(255, GER_NORMAL);
			m_gameStateTen = net::TABLE_STATE_GAME_END;
			m_coolLogicTen.beginCooling(s_ShowCardTime);
		}
	}
	else if (cbType == SnatchCoinType_Hundred)
	{
		ChangeScoreValueInGame(uid, lSubScore, emACCTRAN_OPER_TYPE_EVERY_COLOR_UPDATE_SCORE, m_pHostRoom->GetGameType(), m_blingLogHundred.chessid);
		
		FlushUserBlingLogHundred(pPlayer);

		if (m_poolCardsHundred.size() == 0)
		{
			OnGameEndHundred(255, GER_NORMAL);
			m_gameStateHundred = net::TABLE_STATE_GAME_END;
			m_coolLogicHundred.beginCooling(s_ShowCardTime);
		}
	}
	return true;
}

bool CGameEveryColorTable::IsExistJettonSuccess(uint32 uid)
{
	for (uint32 uIndex = 0; uIndex < m_userJettonSuccess.size(); uIndex++)
	{
		if (m_userJettonSuccess[uIndex] == uid)
		{
			return true;
		}
	}
	return false;
}

bool CGameEveryColorTable::IsExistSnatchCoinTenSuccess(uint32 uid)
{
	map<uint32, vector<BYTE>>::iterator it_SnatchCoinTen = m_userSnatchCoinTen.begin();

	LOG_DEBUG("1 size:%d,uid:%d", m_userSnatchCoinTen.size(), uid);

	for (; it_SnatchCoinTen != m_userSnatchCoinTen.end(); ++it_SnatchCoinTen)
	{
		uint32 tmpuid = it_SnatchCoinTen->first;
		vector<BYTE> & tempvec = it_SnatchCoinTen->second;
		LOG_DEBUG("1 tmpuid:%d,uid:%d:tempvec:%d", tmpuid, uid,tempvec.size());
		if (tmpuid == uid && tempvec.size()>0)
		{
			return true;
		}
	}
	LOG_DEBUG("2 uid:%d", uid);

	return false;
}

bool CGameEveryColorTable::IsExistSnatchCoinHundredSuccess(uint32 uid)
{
	map<uint32, vector<BYTE>>::iterator it_SnatchCoinHundred = m_userSnatchCoinHundred.begin();
	for (; it_SnatchCoinHundred != m_userSnatchCoinHundred.end(); ++it_SnatchCoinHundred)
	{
		uint32 tmpuid = it_SnatchCoinHundred->first;
		vector<BYTE> & tempvec = it_SnatchCoinHundred->second;
		if (tmpuid == uid && tempvec.size()>0)
		{
			return true;
		}
	}
	return false;
}

void    CGameEveryColorTable::AddUserBlingLogTen(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)return;

	for (uint32 i = 0; i<m_blingLogTen.users.size(); ++i) {
		if (m_blingLogTen.users[i].uid == pPlayer->GetUID()) {
			return;
		}
	}
	stBlingUser user;
	user.uid = pPlayer->GetUID();
	user.oldValue = GetPlayerCurScore(pPlayer);
	user.safeCoin = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
	user.chairid = GetChairID(pPlayer);

	LOG_DEBUG("uid:%d", user.uid);

	m_blingLogTen.users.push_back(user);
}

void    CGameEveryColorTable::AddUserBlingLogHundred(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)return;

	for (uint32 i = 0; i<m_blingLogHundred.users.size(); ++i)
	{
		if (m_blingLogHundred.users[i].uid == pPlayer->GetUID())
		{
			return;
		}
	}

	stBlingUser user;
	user.uid = pPlayer->GetUID();
	user.oldValue = GetPlayerCurScore(pPlayer);
	user.safeCoin = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
	user.chairid = GetChairID(pPlayer);

	LOG_DEBUG("uid:%d", user.uid);

	m_blingLogHundred.users.push_back(user);
}

void    CGameEveryColorTable::FlushUserBlingLogTen(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)return;
	for (uint32 i = 0; i<m_blingLogTen.users.size(); ++i)
	{
		if (m_blingLogTen.users[i].uid == pPlayer->GetUID())
		{
			m_blingLogTen.users[i].win = 0;
			m_blingLogTen.users[i].newValue = GetPlayerCurScore(pPlayer);
			m_blingLogTen.users[i].safeCoin = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
			return;
		}
	}
}

void    CGameEveryColorTable::FlushUserBlingLogHundred(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)return;
	for (uint32 i = 0; i<m_blingLogHundred.users.size(); ++i)
	{
		if (m_blingLogHundred.users[i].uid == pPlayer->GetUID())
		{
			m_blingLogHundred.users[i].win = 0;
			m_blingLogHundred.users[i].newValue = GetPlayerCurScore(pPlayer);
			m_blingLogHundred.users[i].safeCoin = pPlayer->GetAccountValue(emACC_VALUE_SAFECOIN);
			return;
		}
	}
}

void    CGameEveryColorTable::FlushUserBlingLogByUidTen(uint32 uid, int64 winScore, int64 fee)
{
	if (uid == 0)return;
	for (uint32 i = 0; i<m_blingLogTen.users.size(); ++i)
	{
		if (m_blingLogTen.users[i].uid == uid)
		{
			m_blingLogTen.users[i].win = winScore;
			m_blingLogTen.users[i].newValue = m_blingLogTen.users[i].newValue + winScore;
			m_blingLogTen.users[i].fee += fee;
			return;
		}
	}
}

void    CGameEveryColorTable::FlushUserBlingLogByUidHundred(uint32 uid, int64 winScore, int64 fee)
{
	if (uid == 0)return;
	for (uint32 i = 0; i<m_blingLogHundred.users.size(); ++i)
	{
		if (m_blingLogHundred.users[i].uid == uid)
		{
			m_blingLogHundred.users[i].win = winScore;
			m_blingLogHundred.users[i].newValue = m_blingLogHundred.users[i].newValue + winScore;
			m_blingLogHundred.users[i].fee += fee;
			return;
		}
	}
}

void    CGameEveryColorTable::SaveBlingLogTen()
{
	m_blingLogTen.endTime = getSysTime();
	m_blingLogTen.operLog << m_operLogTen.toFastString();
	CCenterLogMgr::Instance().WriteGameBlingLog(m_blingLogTen);
}

void    CGameEveryColorTable::SaveBlingLogHundred()
{
	m_blingLogHundred.endTime = getSysTime();
	m_blingLogHundred.operLog << m_operLogHundred.toFastString();
	CCenterLogMgr::Instance().WriteGameBlingLog(m_blingLogHundred);
}

void    CGameEveryColorTable::WriteAddScoreLog(uint32 uid, uint8 area, int64 score)
{
	if (score == 0)
	{
		return;
	}		
	Json::Value logValue;
	logValue["uid"] = uid;
	logValue["p"] = area;
	logValue["s"] = score;

	m_operLog["op"].append(logValue);
}

void    CGameEveryColorTable::WriteCardDataInfo()
{
	m_operLog["card"] = m_cbCardData;
	m_operLog["type"] = SnatchCoinType_Jetton;
}

void    CGameEveryColorTable::WritePeriodsInfo()
{
	m_operLog["periods"] = m_strPeriods.c_str();
}

void    CGameEveryColorTable::WritePeriodsInfoTen()
{
	m_operLogTen["periods"] = m_strPeriodsTen.c_str();
}

void    CGameEveryColorTable::WritePeriodsInfoHundred()
{
	m_operLogHundred["periods"] = m_strPeriodsHundred.c_str();
}

void    CGameEveryColorTable::WriteCardDataInfoTen()
{
	m_operLogTen["card"] = m_cbSnatchCoinCardDataTen;
	m_operLogTen["type"] = SnatchCoinType_Ten;

	map<uint32, vector<BYTE>>::iterator it_SnatchCoinTen = m_userSnatchCoinTen.begin();
	for (; it_SnatchCoinTen != m_userSnatchCoinTen.end(); ++it_SnatchCoinTen)
	{
		uint32 tmpuid = it_SnatchCoinTen->first;
		vector<BYTE> & tempvec = it_SnatchCoinTen->second;
		if (tempvec.size()>0)
		{
			Json::Value logValue;
			logValue["uid"] = tmpuid;
			for (uint32 i = 0; i < tempvec.size(); i++)
			{
				logValue["uc"].append(tempvec[i]);
			}
			m_operLogTen["ucs"].append(logValue);
		}
	}
}

void    CGameEveryColorTable::WriteCardDataInfoHundred()
{
	m_operLogHundred["card"] = m_cbSnatchCoinCardDataHundred;
	m_operLogHundred["type"] = SnatchCoinType_Hundred;

	
	map<uint32, vector<BYTE>>::iterator it_SnatchCoinHundred = m_userSnatchCoinHundred.begin();
	for (; it_SnatchCoinHundred != m_userSnatchCoinHundred.end(); ++it_SnatchCoinHundred)
	{
		uint32 tmpuid = it_SnatchCoinHundred->first;
		vector<BYTE> & tempvec = it_SnatchCoinHundred->second;
		if (tempvec.size()>0)
		{
			Json::Value logValue;
			logValue["uid"] = tmpuid;
			for (uint32 i = 0; i < tempvec.size(); i++)
			{
				logValue["uc"].append(tempvec[i]);
			}
			m_operLogHundred["ucs"].append(logValue);
		}
	}
}

void    CGameEveryColorTable::WriteAreaInfo()
{
	Json::Value logValue;
	
	for (int nAreaIndex = 0; nAreaIndex < EVERY_COLOR_AREA_COUNT; nAreaIndex++)
	{
		string str_area_score = CStringUtility::FormatToString("sa%d", nAreaIndex);
		string str_area_prize = CStringUtility::FormatToString("ha%d", nAreaIndex);

		logValue[str_area_score] = m_areaJettonScore[nAreaIndex];
		if (m_areaHitPrize[nAreaIndex])
		{
			logValue[str_area_prize] = 1;
		}
		else
		{
			logValue[str_area_prize] = 0;
		}
	}
	m_operLog["area"].append(logValue);
	m_operLog["jfc"] = m_lJackpotFlower;
	m_operLog["jbs"] = m_lJackpotBigSmall;
	m_operLog["swp"] = m_iSysWinPro;
	m_operLog["slp"] = m_iSysLostPro;
}

//初始化洗牌
void    CGameEveryColorTable::InitRandCardTen()
{
	BYTE cbCardListData[EVERY_COLOR_SNATCH_COIN_CARD_COUNT];
	memcpy(cbCardListData, g_cbEveryColorCardListData, sizeof(g_cbEveryColorCardListData));

	//BYTE cbPosition = g_RandGen.RandUInt() % (EVERY_COLOR_SNATCH_COIN_CARD_COUNT);
	//m_cbCardData = cbCardListData[cbPosition];

	m_poolCardsTen.clear();
	for (uint32 i = 0; i< sizeof(cbCardListData); ++i)
	{
		m_poolCardsTen.push_back(cbCardListData[i]);
	}
	RandPoolCardTen();
	//LOG_DEBUG("初始化牌池:牌数:%d", m_poolCards.size());
}

void    CGameEveryColorTable::InitRandCardHundred()
{
	BYTE cbCardListData[EVERY_COLOR_SNATCH_COIN_CARD_COUNT];
	memcpy(cbCardListData, g_cbEveryColorCardListData, sizeof(g_cbEveryColorCardListData));

	//BYTE cbPosition = g_RandGen.RandUInt() % (EVERY_COLOR_SNATCH_COIN_CARD_COUNT);
	//m_cbCardData = cbCardListData[cbPosition];

	m_poolCardsHundred.clear();
	for (uint32 i = 0; i< sizeof(cbCardListData); ++i)
	{
		m_poolCardsHundred.push_back(cbCardListData[i]);
	}
	RandPoolCardHundred();
	//LOG_DEBUG("初始化牌池:牌数:%d", m_poolCards.size());
}


//牌池洗牌
void 	CGameEveryColorTable::RandPoolCardTen()
{
	for (uint32 i = 0; i<m_poolCardsTen.size(); ++i)
	{
		std::swap(m_poolCardsTen[i], m_poolCardsTen[g_RandGen.RandUInt() % m_poolCardsTen.size()]);
	}
}

void 	CGameEveryColorTable::RandPoolCardHundred()
{
	for (uint32 i = 0; i<m_poolCardsHundred.size(); ++i)
	{
		std::swap(m_poolCardsHundred[i], m_poolCardsHundred[g_RandGen.RandUInt() % m_poolCardsHundred.size()]);
	}
}

//摸一张牌
BYTE    CGameEveryColorTable::PopCardFromPoolTen()
{
	BYTE card = 0;
	if (m_poolCardsTen.size() > 0) {
		card = m_poolCardsTen[0];
		m_poolCardsTen.erase(m_poolCardsTen.begin());
	}
	return card;
}
//摸一张牌
BYTE    CGameEveryColorTable::PopCardFromPoolHundred()
{
	BYTE card = 0;
	if (m_poolCardsHundred.size() > 0) {
		card = m_poolCardsHundred[0];
		m_poolCardsHundred.erase(m_poolCardsHundred.begin());
	}
	return card;
}

void    CGameEveryColorTable::SendSnatchCoinGameState()
{
	//构造数据
	net::msg_everycolor_snatch_coin_state msg;
	msg.set_stop_state(m_uSnatchCoinStop);
	SendMsgToAll(&msg, net::S2C_MSG_EVERY_COLOR_SNATCH_COIN_STATE);
}

bool CGameEveryColorTable::SetSnatchCoinState(uint32 stop)
{
	if (stop >= 2)
	{
		LOG_ERROR("failed - roomid:%d,tableid:%d,stop:%d", m_pHostRoom->GetRoomID(), GetTableID(), stop);
		return false;
	}
	//LOG_ERROR("success - roomid:%d,tableid:%d,stop:%d", m_pHostRoom->GetRoomID(), GetTableID(), stop);

	if (stop == 1)
	{
		SaveSnatchCoinGameData();
	}
	else if (stop == 0)
	{
		bool bReTen = false, bReHundred = false;
		bool bFlag = LoadSnatchCoinGameData(bReTen, bReHundred);
		if (bReTen)
		{
			OnGameStartTen();
		}
		else
		{
			//继续上一局游戏
			InitBlingLogTen();
			WritePeriodsInfoTen();
			m_gameStateTen = net::TABLE_STATE_PLAY;
		}
		if (bReHundred)
		{
			OnGameStartHundred();
		}
		else
		{
			//继续上一局游戏
			InitBlingLogHundred();
			WritePeriodsInfoHundred();

			m_gameStateHundred = net::TABLE_STATE_PLAY;
		}
	}

	m_uSnatchCoinStop = stop;
	SendSnatchCoinGameState();

	LOG_ERROR("success - roomid:%d,tableid:%d,stop:%d", m_pHostRoom->GetRoomID(), GetTableID(), stop);
	return true;
}

bool CGameEveryColorTable::SaveSnatchCoinGameData()
{
	uint32 uSysUid = ROBOT_MGR_ID + net::GAME_CATE_EVERYCOLOR;

	LOG_DEBUG("m_userSnatchCoinTen.size:%d,m_userSnatchCoinHundred.size:%d", m_userSnatchCoinTen.size(), m_userSnatchCoinHundred.size());

	//十元夺宝
	if (m_userSnatchCoinTen.size() > 0)
	{
		map<uint32, vector<BYTE>>::iterator it_SnatchCoinTen = m_userSnatchCoinTen.begin();

		LOG_DEBUG("m_userSnatchCoinTen.size:%d,m_userSnatchCoinHundred.size:%d,it_SnatchCoinTen:%p,end:%p", m_userSnatchCoinTen.size(), m_userSnatchCoinHundred.size(), it_SnatchCoinTen, m_userSnatchCoinTen.end());

		for (; it_SnatchCoinTen != m_userSnatchCoinTen.end(); ++it_SnatchCoinTen)
		{
			uint32 tmpuid = it_SnatchCoinTen->first;
			vector<BYTE> & vecCard = it_SnatchCoinTen->second;
			
			LOG_DEBUG("m_userSnatchCoin - tmpuid:%d,vecCard.size:%d", tmpuid, vecCard.size());

			Json::Value cardValue;
			for (uint32 i = 0; i < vecCard.size(); i++)
			{
				LOG_DEBUG("log - tmpuid:%d,vecCard:0x%02X", tmpuid, vecCard[i]);
				cardValue["card"].append(vecCard[i]);
			}
			LOG_DEBUG("m_userSnatchCoin - tmpuid:%d,vecCard.size:%d,cardValue:%s", tmpuid, vecCard.size(), cardValue.toFastString().c_str());
			string strcard = cardValue.toFastString();
			CDBMysqlMgr::Instance().AsyncSaveSnatchCoinGameData(tmpuid, SnatchCoinType_Ten, strcard);
		}

		Json::Value SnatchCoinValueTen;
		SnatchCoinValueTen["gc"] = m_uGameCountTen;
		SnatchCoinValueTen["pd"] = m_strPeriodsTen;
		for (uint32 i = 0; i < m_poolCardsTen.size(); i++)
		{
			SnatchCoinValueTen["cd"].append(m_poolCardsTen[i]);
		}
		string strdata_ten = SnatchCoinValueTen.toFastString();
		CDBMysqlMgr::Instance().AsyncSaveSnatchCoinGameData(uSysUid, SnatchCoinType_Ten, strdata_ten);

		Json::Value logValueTen;
		for (uint32 i = 0; i<m_blingLogTen.users.size(); ++i)
		{
			stBlingUser& user = m_blingLogTen.users[i];
			Json::Value juser;
			juser["uid"] = user.uid;
			juser["oldv"] = user.oldValue;
			juser["newv"] = user.newValue;
			juser["safebox"] = user.safeCoin;
			juser["win"] = user.win;
			juser["chair"] = user.chairid;
			logValueTen["uids"].append(juser);
		}
		string strlog_ten = logValueTen.toFastString();
		CDBMysqlMgr::Instance().AsyncSaveSnatchCoinGameData(uSysUid + 1, SnatchCoinType_Ten, strlog_ten);
	}

	//百元夺宝
	if (m_userSnatchCoinHundred.size()>0)
	{
		map<uint32, vector<BYTE>>::iterator it_SnatchCoinHundred = m_userSnatchCoinHundred.begin();
		for (; it_SnatchCoinHundred != m_userSnatchCoinHundred.end(); ++it_SnatchCoinHundred)
		{
			uint32 tmpuid = it_SnatchCoinHundred->first;
			vector<BYTE> & vecCard = it_SnatchCoinHundred->second;
			LOG_DEBUG("m_userSnatchCoin - tmpuid:%d,vecCard.size:%d", tmpuid, vecCard.size());

			Json::Value cardValue;
			for (uint32 i = 0; i < vecCard.size(); i++)
			{
				cardValue["card"].append(vecCard[i]);
			}
			string strcard = cardValue.toFastString();
			LOG_DEBUG("m_userSnatchCoin - tmpuid:%d,vecCard.size:%d,cardValue:%s", tmpuid, vecCard.size(), cardValue.toFastString().c_str());
			CDBMysqlMgr::Instance().AsyncSaveSnatchCoinGameData(tmpuid, SnatchCoinType_Hundred, strcard);
		}

		Json::Value SnatchCoinValueHundred;
		SnatchCoinValueHundred["gc"] = m_uGameCountHundred;
		SnatchCoinValueHundred["pd"] = m_strPeriodsHundred;
		for (uint32 i = 0; i < m_poolCardsHundred.size(); i++)
		{
			SnatchCoinValueHundred["cd"].append(m_poolCardsHundred[i]);
		}
		string strdata_hundred = SnatchCoinValueHundred.toFastString();
		CDBMysqlMgr::Instance().AsyncSaveSnatchCoinGameData(uSysUid, SnatchCoinType_Hundred, strdata_hundred);


		Json::Value logValueHundred;
		for (uint32 i = 0; i<m_blingLogHundred.users.size(); ++i)
		{
			stBlingUser& user = m_blingLogHundred.users[i];
			Json::Value juser;
			juser["uid"] = user.uid;
			juser["oldv"] = user.oldValue;
			juser["newv"] = user.newValue;
			juser["safebox"] = user.safeCoin;
			juser["win"] = user.win;
			juser["chair"] = user.chairid;
			logValueHundred["uids"].append(juser);
		}
		string strlog_hundred = logValueHundred.toFastString();
		CDBMysqlMgr::Instance().AsyncSaveSnatchCoinGameData(uSysUid + 1, SnatchCoinType_Hundred, strlog_hundred);

	}
	
	return true;
}

bool CGameEveryColorTable::LoadSnatchCoinGameData(bool & bReTen,bool & bReHundred)
{
	bReTen = true;
	bReHundred = true;

	vector<tagSnatchCoinGameData> vecGameData;
	bool bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_ACC).SynLoadSnatchCoinGameData(vecGameData);
	LOG_DEBUG("bRet:%d,size:%d", bRet, vecGameData.size());
	if (!bRet || vecGameData.size() == 0)
	{
		return false;
	}
	uint32 uMinSysUid = ROBOT_MGR_ID + net::GAME_CATE_EVERYCOLOR;
	uint32 uMaxSysUid = ROBOT_MGR_ID + net::GAME_CATE_EVERYCOLOR + net::GAME_CATE_MAX_TYPE;
	for (uint32 i = 0; i < vecGameData.size(); i++)
	{
		Json::Value  jvalue;
		Json::Reader reader;

		uint32 uid = vecGameData[i].uid;
		uint32 type = vecGameData[i].type;
		string card = vecGameData[i].card;
		if (uid == 0 || type == 3 || card.empty())
		{
			LOG_DEBUG("vecGameData.size:%d,i:%d,uid:%d,type:%d,card:%s", vecGameData.size(),i, uid, type, card.c_str());
			return false;
		}
		if (type == SnatchCoinType_Ten)
		{
			bReTen = false;
			if (uid == uMinSysUid)
			{
				bool bFlag = reader.parse(card, jvalue);
				if (!bFlag)
				{
					continue;
				}
				if (jvalue.isMember("gc"))
				{
					m_uGameCountTen = jvalue["gc"].asUInt();
				}
				else
				{
					continue;
				}
				if (jvalue.isMember("pd"))
				{
					m_strPeriodsTen = jvalue["pd"].asString();
				}
				else
				{
					continue;
				}
				if (jvalue.isMember("cd"))
				{
					Json::Value syscard = jvalue["cd"];
					LOG_DEBUG("ten - size:%d,m_poolCardsTen:%s", syscard.size(), syscard.toFastString().c_str());
					for (uint32 j = 0; j < syscard.size(); j++)
					{
						m_poolCardsTen.push_back(syscard[j].asUInt());
					}
				}
				else
				{
					continue;
				}
			}
			else if (uid == uMinSysUid + 1)
			{
				bool bFlag = reader.parse(card, jvalue);
				if (!bFlag)
				{
					continue;
				}
				Json::Value juids = jvalue["uids"];
				LOG_DEBUG("jvalue.size:%d,str1:%s - str2:%s,juids.size:%d,juids.str:%s ", jvalue.size(), jvalue.toFastString().c_str(), jvalue.toStyledString().c_str(), juids.size(), juids.toStyledString().c_str());
				for (uint32 j = 0; j < juids.size(); j++)
				{
					stBlingUser user;
					Json::Value juser = juids[j];
					user.uid = juser["uid"].asUInt();
					user.oldValue = juser["oldv"].asInt64();
					user.newValue = juser["newv"].asInt64();
					user.safeCoin = juser["safebox"].asInt64();
					user.win = juser["win"].asInt64();
					user.chairid = juser["chair"].asUInt();
					m_blingLogTen.users.push_back(user);
				}
			}
			else if (uid < uMinSysUid)
			{
				bool bFlag = reader.parse(card, jvalue);
				Json::Value jcard = jvalue["card"];
				LOG_DEBUG("bFlag:%d,jvalue.size:%d,str1:%s - str2:%s,jcard.size:%d,jcard.str:%s ", bFlag,jvalue.size(), jvalue.toFastString().c_str(), jvalue.toStyledString().c_str(), jcard.size(), jcard.toStyledString().c_str());

				if (!bFlag)
				{
					continue;
				}
				for (uint32 j = 0; j<jcard.size(); ++j)
				{
					m_userSnatchCoinTen[uid].push_back(jcard[j].asUInt());
				}
			}

		}
		else if (type == SnatchCoinType_Hundred)
		{
			bReHundred = false;

			if (uid == uMinSysUid)
			{
				bool bFlag = reader.parse(card, jvalue);
				if (!bFlag)
				{
					continue;
				}
				if (jvalue.isMember("gc"))
				{
					m_uGameCountHundred = jvalue["gc"].asUInt();
				}
				else
				{
					continue;
				}
				if (jvalue.isMember("pd"))
				{
					m_strPeriodsHundred = jvalue["pd"].asString();
				}
				else
				{
				}
				if (jvalue.isMember("cd"))
				{
					Json::Value syscard = jvalue["cd"];
					LOG_DEBUG("hundred - size:%d,m_poolCardsHundred:%s", syscard.size(), syscard.toFastString().c_str());
					for (uint32 j = 0; j < syscard.size(); j++)
					{
						m_poolCardsHundred.push_back(syscard[j].asUInt());
					}
				}
				else
				{
					continue;
				}
			}
			else if (uid == uMinSysUid + 1)
			{
				bool bFlag = reader.parse(card, jvalue);
				if (!bFlag)
				{
					continue;
				}
				Json::Value juids = jvalue["uids"];
				LOG_DEBUG("jvalue.size:%d,str1:%s - str2:%s,juids.size:%d,juids.str:%s ", jvalue.size(), jvalue.toFastString().c_str(), jvalue.toStyledString().c_str(), juids.size(), juids.toStyledString().c_str());
				for (uint32 j = 0; j < jvalue.size(); j++)
				{
					stBlingUser user;
					Json::Value juser = juids[j];
					user.uid = juser["uid"].asUInt();
					user.oldValue = juser["oldv"].asInt64();
					user.newValue = juser["newv"].asInt64();
					user.safeCoin = juser["safebox"].asInt64();
					user.win = juser["win"].asInt64();
					user.chairid = juser["chair"].asUInt();
					m_blingLogHundred.users.push_back(user);
				}
			}
			else if (uid < uMinSysUid)
			{
				bool bFlag = reader.parse(card, jvalue);
				Json::Value jcard = jvalue["card"];
				LOG_DEBUG("bFlag:%d,jvalue.size:%d,str1:%s - str2:%s,jcard.size:%d,jcard.str:%s ", bFlag, jvalue.size(), jvalue.toFastString().c_str(), jvalue.toStyledString().c_str(), jcard.size(), jcard.toStyledString().c_str());

				if (!bFlag)
				{
					continue;
				}
				for (uint32 j = 0; j<jcard.size(); ++j)
				{
					m_userSnatchCoinHundred[uid].push_back(jcard[j].asUInt());
				}
			}
		}
	}

	LOG_DEBUG("m_userSnatchCoin.size:%d,size:%d,m_uGameCount:%d %d,m_strPeriods:%s - %s,",
		m_userSnatchCoinTen.size(), m_userSnatchCoinHundred.size(), m_uGameCountTen, m_uGameCountHundred, m_strPeriodsTen.c_str(),m_strPeriodsHundred.c_str());


	//十元夺宝
	map<uint32, vector<BYTE>>::iterator it_SnatchCoinTen = m_userSnatchCoinTen.begin();
	for (; it_SnatchCoinTen != m_userSnatchCoinTen.end(); ++it_SnatchCoinTen)
	{
		uint32 tmpuid = it_SnatchCoinTen->first;
		vector<BYTE> & vecCard = it_SnatchCoinTen->second;
		LOG_DEBUG("m_userSnatchCoin - tmpuid:%d,vecCard.size:%d", tmpuid, vecCard.size());
		for (uint32 i = 0; i < vecCard.size(); i++)
		{
			LOG_DEBUG("ten - i:%d,tmpuid:%d,vecCard:0x%02X", i,tmpuid, vecCard[i]);
		}
	}

	//百元夺宝
	map<uint32, vector<BYTE>>::iterator it_SnatchCoinHundred = m_userSnatchCoinHundred.begin();
	for (; it_SnatchCoinHundred != m_userSnatchCoinHundred.end(); ++it_SnatchCoinHundred)
	{
		uint32 tmpuid = it_SnatchCoinHundred->first;
		vector<BYTE> & vecCard = it_SnatchCoinHundred->second;
		LOG_DEBUG("m_userSnatchCoin - tmpuid:%d,vecCard.size:%d", tmpuid, vecCard.size());
		for (uint32 i = 0; i < vecCard.size(); i++)
		{
			LOG_DEBUG("hundred - i:%dtmpuid:%d,vecCard:0x%02X", i,tmpuid, vecCard[i]);
		}
	}

	CDBMysqlMgr::Instance().ClearSnatchCoinGameData();

	LOG_DEBUG("load SnatchCoinGameData success - bReTen:%d,bReHundred:%d,", bReTen,bReHundred);

	return true;
}