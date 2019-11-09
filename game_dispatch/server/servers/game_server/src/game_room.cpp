
#include <data_cfg_mgr.h>
#include "game_room.h"
#include "stdafx.h"
#include "game_table.h"

#include "game_server_config.h"

using namespace svrlib;
using namespace std;
using namespace net;

namespace
{
    const static uint8 s_MaxSendTable = 30;
}
CGameRoom::CGameRoom()
{
     m_playerNum = 0;        // 玩家人数
     m_showonline= 0;
     m_roomIndex = 0;        // 分配房间索引
     m_gameType  = 0;
     m_pFlushTimer = NULL;
	 m_tagJackpotScore.Init();
	 m_lPlayerMaxWinScore = 0;
	 m_curr_sys_time = getSysTime();
}
CGameRoom::~CGameRoom()
{
    m_pFlushTimer = NULL;
}
void  CGameRoom::OnTimer(uint8 eventID)
{
    switch(eventID)
    {
    case 1:
    {
        OnTimeTick();
    }break;
    default:
        break;
    }
}
bool    CGameRoom::Init(uint16 gameType)
{
    LOG_DEBUG("init game room：%d-%d-%d-%d, robot:%d,stockMin:%d,stockMax:%d,stockConversionRate:%d,stock:%d,jackpotMin:%d,jackpotMaxRate:%d,jackpotRate:%d,jackpotCoefficient:%d,jackpotExtractRate:%d,jackpot:%d,fee:%d",
		m_roomCfg.roomID,m_roomCfg.deal,m_roomCfg.consume,m_roomCfg.baseScore, m_roomCfg.robot, m_roomStockCfg.stockMin, 
		m_roomStockCfg.stockMax, m_roomStockCfg.stockConversionRate, m_roomStockCfg.stock, m_roomStockCfg.jackpotMin, 
		m_roomStockCfg.jackpotMaxRate, m_roomStockCfg.jackpotRate, m_roomStockCfg.jackpotCoefficient,
		m_roomStockCfg.jackpotExtractRate, m_roomStockCfg.jackpot, m_roomStockCfg.fee);
    m_gameType = gameType;
    if(GetRoomType() == emROOM_TYPE_PRIVATE)// 私人房加载私人房数据
    {
        vector<stPrivateTable> tables;
        bool bRet = CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_ACC).LoadPrivateTable(gameType,tables);
        if(bRet == false) {
            LOG_ERROR("load private room error:%d--%d",gameType,GetRoomID());
            return false;
        }
        for(auto &table : tables)
        {
            CGameTable* pTable = CreateTable(table.tableID);
            pTable->LoadPrivateTable(table);
            pTable->Init();
            pTable->ResetTable();
            m_mpTables.insert(make_pair(pTable->GetTableID(),pTable));
            LOG_DEBUG("load private room:%d",pTable->GetTableID());
        }
    }
    if(GetRoomType() == emROOM_TYPE_COMMON)// 普通房初始化桌子
    {
        for(uint32 i=0;i<m_roomCfg.tableNum;++i)
        {
            MallocTable();
        }
    }

    m_pFlushTimer = CApplication::Instance().MallocTimer(this,1);
    m_pFlushTimer->StartTimer(10,10);

	ReAnalysisParam();

	//捕鱼房间的初始化
	if (GetGameType() == net::GAME_CATE_FISHING)
	{
		if (CFishCfgMgr::Instance().Init() == false)
		{
			LOG_ERROR("初始化捕鱼管理器失败");
			return false;
		}
	}

    return true;
}
bool	CGameRoom::ReAnalysisParam() {

	string param = GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (param.empty() || !reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - param:%s", param.c_str());
		return true;
	}
	if (jvalue.isMember("aps") && jvalue["aps"].isIntegral()) {
		m_tagJackpotScore.lMaxPollScore = jvalue["aps"].asInt64();
	}
	if (jvalue.isMember("ips") && jvalue["ips"].isIntegral()) {
		m_tagJackpotScore.lMinPollScore = jvalue["ips"].asInt64();
	}
	int iIsUpdateCurPollScore = 0;
	if (jvalue.isMember("ucp") && jvalue["ucp"].isIntegral()) {
		iIsUpdateCurPollScore = jvalue["ucp"].asInt();
	}
	if (iIsUpdateCurPollScore == 1)
	{
		if (jvalue.isMember("cps") && jvalue["cps"].isIntegral()) {
			int64 lCurPollScore = jvalue["cps"].asInt64();
			m_tagJackpotScore.SetCurPoolScore(lCurPollScore);
		}
	}
	if (jvalue.isMember("swp") && jvalue["swp"].isIntegral()) {
		m_tagJackpotScore.uSysWinPro = jvalue["swp"].asInt();
	}
	if (jvalue.isMember("slp") && jvalue["slp"].isIntegral()) {
		m_tagJackpotScore.uSysLostPro = jvalue["slp"].asInt();
	}
	if (jvalue.isMember("lsc") && jvalue["lsc"].isIntegral()) {
		m_tagJackpotScore.uSysLostWinProChange = jvalue["lsc"].asInt();
	}
	if (jvalue.isMember("ujs") && jvalue["ujs"].isIntegral()) {
		m_tagJackpotScore.lUpdateJackpotScore = jvalue["ujs"].asInt64();
	}
	if (jvalue.isMember("ujc") && jvalue["ujc"].isIntegral()) {
		m_tagJackpotScore.iUserJackpotControl = jvalue["ujc"].asInt();
	}
	if (jvalue.isMember("mws") && jvalue["mws"].isIntegral()) {
		m_lPlayerMaxWinScore = jvalue["mws"].asInt();
	}

	if (jvalue.isMember("mis") && jvalue["mis"].isIntegral()) {
		m_MasterShowUserInfo.lMinScore = jvalue["mis"].asInt();
	}
	if (jvalue.isMember("mas") && jvalue["mas"].isIntegral()) {
		m_MasterShowUserInfo.lMaxScore = jvalue["mas"].asInt();
	}

	m_tagJackpotScore.ulLastRestTime = getSysTime();
	m_tagJackpotScore.tm_hour =  g_RandGen.RandRange(1, 3);
	m_tagJackpotScore.tm_min =  g_RandGen.RandRange(0, 59);


	LOG_ERROR("reader_success - roomid:%d,lMinScore:%d,lMaxScore:%d,m_lMaxPollScore:%lld,m_lMinPollScore:%lld,iIsUpdateCurPollScore:%d,m_lCurPollScore:%lld,m_uSysWinPro:%d,uSysLostPro:%d,uSysLostWinProChange:%d,lUpdateJackpotScore:%lld,iUserJackpotControl:%d,ulLastRestTime:%lld,tm_hour:%d,tm_min:%d,m_lPlayerMaxWinScore:%lld",
		GetRoomID(), m_MasterShowUserInfo.lMinScore, m_MasterShowUserInfo.lMaxScore,m_tagJackpotScore.lMaxPollScore, m_tagJackpotScore.lMinPollScore, iIsUpdateCurPollScore, m_tagJackpotScore.lCurPollScore, m_tagJackpotScore.uSysWinPro, m_tagJackpotScore.uSysLostPro,m_tagJackpotScore.uSysLostWinProChange, m_tagJackpotScore.lUpdateJackpotScore, m_tagJackpotScore.iUserJackpotControl, m_tagJackpotScore.ulLastRestTime, m_tagJackpotScore.tm_hour, m_tagJackpotScore.tm_min, m_lPlayerMaxWinScore);

	return true;
}

void    CGameRoom::ShutDown()
{
    CApplication::Instance().FreeTimer(m_pFlushTimer);
    for(auto &it : m_mpTables)
    {
        CGameTable* pTable = it.second;
        pTable->ShutDown();
    }
}
void    CGameRoom::OnTimeTick()
{
	uint64 curr_time = getSysTime();

	//如果当前房间为捕鱼或看牌抢庄，则需要设置定时器间隔为10ms,其它游戏间隔时间为1s
	if (curr_time <= m_curr_sys_time) {
		uint16 gameType = GetGameType();
		if (gameType != net::GAME_CATE_FISHING && gameType != net::GAME_CATE_ROBNIU)
			return;
	}
	
	//LOG_DEBUG("AAA - curr_time:%llu,m_curr_sys_time:%llu tick_time:%llu", curr_time, m_curr_sys_time, getTickCount64());

	m_curr_sys_time = curr_time;

	// add by har
	static uint64 uProcessTime = 0;
	uint64 uTime = getSysTime();
	if (uProcessTime == 0)
		uProcessTime = uTime;
	if (uTime != uProcessTime) { // 一秒一次
		if (diffTimeDay(uProcessTime, uTime) != 0) {
			LOG_DEBUG("CGameRoom::OnTimeTick new day roomid:%d,stock:%d,jackpot:%d,fee:%d,uProcessTime:%d,uTime:%d",
				GetRoomID(), m_roomStockCfg.stock, m_roomStockCfg.jackpot, m_roomStockCfg.fee, uProcessTime, uTime);
			m_roomStockCfg.fee = 0; // 跨天实时抽水清零
		}
		uProcessTime = uTime;
	}
	// add by har end
    for(auto &it : m_mpTables)
    {
        CGameTable* pTable = it.second;
        pTable->OnTimeTick();
    }
    if(m_coolMarry.isTimeOut()){
        MarryTable();
        m_coolMarry.beginCooling(1000);
    }
    if(m_coolRecover.isTimeOut()){
        CheckRecover();
        m_coolRecover.beginCooling(2000);
    }
    if(m_coolNewTable.isTimeOut()){
        CheckNewTable();
        m_coolNewTable.beginCooling(5000);
        CalcShowOnline();
    }

	//ResetJackpotScore();

}
bool    CGameRoom::EnterRoom(CGamePlayer* pGamePlayer)
{
	if (pGamePlayer == NULL) {
		LOG_DEBUG("pGamePlayer pointer is null");
		return false;
	}
	LOG_DEBUG("entering game room - uid:%d,m_showonline:%d,m_playerNum:%d", pGamePlayer->GetUID(), m_showonline, m_playerNum);

	if (pGamePlayer->GetRoom() != this && !CanEnterRoom(pGamePlayer)) {
		LOG_DEBUG("do not enter game room - uid:%d,m_showonline:%d,m_playerNum:%d,proom:%p,CanEnterRoom:%d", pGamePlayer->GetUID(), m_showonline, m_playerNum, pGamePlayer->GetRoom(),CanEnterRoom(pGamePlayer));
		return false;
	}
    if(pGamePlayer->GetRoom() != this) {
        pGamePlayer->SetRoom(this);
        pGamePlayer->SetAutoReady(false);
        m_playerNum++;
    }
    net::msg_enter_room_rep rep;
    rep.set_result(1);
    rep.set_cur_table(pGamePlayer->GetTableID());
    GetRoomInfo(rep.mutable_room());
    pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_ENTER_ROOM_REP);

    CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(pGamePlayer->GetUID(),CApplication::Instance().GetServerID(),GetRoomID(),pGamePlayer->GetPlayerType());
    CalcShowOnline();
	LOG_DEBUG("enter game room - uid:%d,roomid:%d,m_showonline:%d,m_playerNum:%d", pGamePlayer->GetUID(),GetRoomID(), m_showonline, m_playerNum);

    return true;
}

bool    CGameRoom::EnterEveryColorRoom(CGamePlayer* pGamePlayer)
{
	if (pGamePlayer->GetRoom() != this && !CanEnterRoom(pGamePlayer))
		return false;
	if (pGamePlayer->GetRoom() != this) {
		pGamePlayer->SetRoom(this);
		pGamePlayer->SetAutoReady(false);
		m_playerNum++;
	}

	//CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(pGamePlayer->GetUID(), CApplication::Instance().GetServerID(), GetRoomID(), pGamePlayer->GetPlayerType());
	CalcShowOnline();
	return true;
}

bool    CGameRoom::LeaveRoom(CGamePlayer* pGamePlayer)
{
	if (pGamePlayer == NULL) {
		return false;
	}
    if(pGamePlayer->GetRoom() != this){
        LOG_ERROR("not the same room - uid:%d",pGamePlayer->GetUID());
        return false;
    }

    auto pTable = pGamePlayer->GetTable();
    if(pTable != NULL)
    {
        if(!pTable->CanLeaveTable(pGamePlayer))
            return false;
        pTable->LeaveTable(pGamePlayer);
    }
    LeaveMarry(pGamePlayer);
    pGamePlayer->SetRoom(NULL);
    pGamePlayer->SetAutoReady(false);
    m_playerNum--;
	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType != net::GAME_CATE_EVERYCOLOR)
	{
		CDBMysqlMgr::Instance().UpdatePlayerOnlineInfo(pGamePlayer->GetUID(), CApplication::Instance().GetServerID(), 0, pGamePlayer->GetPlayerType());
	}
    CalcShowOnline();
    return true;
}
uint8   CGameRoom::EnterTable(CGamePlayer* pGamePlayer,uint32 tableID,string passwd)
{
	uint32 tuid = 0;
	if (pGamePlayer == NULL)
	{
		tuid = pGamePlayer->GetUID();
	}	
    if(pGamePlayer->GetRoom() != this)
	{
        //LOG_ERROR("not the same room:%d",pGamePlayer->GetUID());
		LOG_DEBUG("a - uid:%d,roomid:%d,tableID:%d", tuid, GetRoomID(), tableID);
        return net::RESULT_CODE_FAIL;
    }
    CGameTable* pOldTable = pGamePlayer->GetTable();
	LOG_DEBUG("enter_table - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
    if(GetRoomType() == emROOM_TYPE_PRIVATE)
    {
        // 特殊处理
        net::msg_enter_table_rep rep;
        rep.set_result(RESULT_CODE_SUCCESS);
        rep.set_table_id(tableID);
        if(pOldTable != NULL)
        {
            if(pOldTable->GetTableID() != tableID)
			{
				LOG_DEBUG("b - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
                return net::RESULT_CODE_FAIL;
            }
			else
			{
                //LOG_DEBUG("backe table")
				LOG_DEBUG("c - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
                pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_ENTER_TABLE);
                pOldTable->OnActionUserNetState(pGamePlayer, true);
                return net::RESULT_CODE_SUCCESS;
            }
        }
        CGameTable* pNewTable = GetTable(tableID);
        if(pNewTable == NULL)
		{
            //LOG_DEBUG("the table is not have:%d",tableID);
			LOG_DEBUG("d - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
            return net::RESULT_CODE_NOT_TABLE;
        }
        if(pNewTable->IsRightPasswd(passwd) == false)
		{
            //LOG_DEBUG("passwd is error");
			LOG_DEBUG("e - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
            return net::RESULT_CODE_PASSWD_ERROR;
        }
        if(!pNewTable->CanEnterTable(pGamePlayer))
		{
            //LOG_DEBUG("cant join table");
			LOG_DEBUG("f - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
            return net::RESULT_CODE_TABLE_FULL;
        }
        pGamePlayer->SendMsgToClient(&rep,net::S2C_MSG_ENTER_TABLE);
        if(pNewTable->EnterTable(pGamePlayer))
		{
			LOG_DEBUG("g - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
            return net::RESULT_CODE_SUCCESS;
        }
		LOG_DEBUG("h - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
        return net::RESULT_CODE_FAIL;
    }
    else
    {
        if(pOldTable != NULL)
        {
            if (pOldTable->GetTableID() == tableID)
			{
                //LOG_DEBUG("back table")
				LOG_DEBUG("1 success_table - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
                pOldTable->OnActionUserNetState(pGamePlayer, true);
                return net::RESULT_CODE_SUCCESS;
            }
            if (!pOldTable->CanLeaveTable(pGamePlayer)) 
			{
                //LOG_DEBUG("can't leave table:%d", pGamePlayer->GetUID());
				LOG_DEBUG("2 - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
                return net::RESULT_CODE_FAIL;
            }
            if (!pOldTable->LeaveTable(pGamePlayer))
			{
                //LOG_DEBUG("leave table faild:%d", pGamePlayer->GetUID());
				LOG_DEBUG("3 - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
                return net::RESULT_CODE_FAIL;
            }
        }
        if(IsNeedMarry())
		{
			LOG_DEBUG("4 success_table - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
            JoinMarry(pGamePlayer,0);
			return net::RESULT_CODE_SUCCESS;
        }
		else
		{
            CGameTable* pNewTable = GetTable(tableID);
            if(pNewTable == NULL || !pNewTable->CanEnterTable(pGamePlayer))
            {
                if(FastJoinTable(pGamePlayer))
				{
					LOG_DEBUG("5 success_table - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
                    return net::RESULT_CODE_SUCCESS;
                }
				else
				{
					LOG_DEBUG("6 - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
                    //LOG_DEBUG("can't join the table");
                    return net::RESULT_CODE_TABLE_FULL;
                }
            }
            if(pNewTable->EnterTable(pGamePlayer))
			{
				LOG_DEBUG("7 success_table - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
                return net::RESULT_CODE_SUCCESS;
            }
			LOG_DEBUG("8 - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
            return net::RESULT_CODE_FAIL;
        }
		LOG_DEBUG("9 success_table - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
        return net::RESULT_CODE_SUCCESS;
    }
	LOG_DEBUG("10 - uid:%d,roomid:%d,roomtype:%d,tableid:%d", tuid, GetRoomID(), GetRoomType(), tableID);
    //LOG_DEBUG("join table faild:%d-->%d",pGamePlayer->GetUID(),tableID);
    return net::RESULT_CODE_FAIL;
}

bool    CGameRoom::CommonJoinTable(CGamePlayer* pGamePlayer)
{
	if (pGamePlayer == NULL)
	{
		return false;
	}
	if (pGamePlayer->GetRoom() != this)
	{
		LOG_ERROR("not the same room - uid:%d", pGamePlayer->GetUID());
		return false;
	}
	if (GetRoomType() != emROOM_TYPE_COMMON)
		return false;
	CGameTable* pOldTable = pGamePlayer->GetTable();
	bool bEnterTable = false;
	bool bCanLeaveTable = false;
	bool bLeaveTable = false;
	bool bIsJoinMarry = false;
	uint32 tableID = 255;
	uint32 enter_code = 0;
	if (pOldTable == NULL)
	{
		if (IsNeedMarry())
		{
			JoinMarry(pGamePlayer, 0);
			bIsJoinMarry = true;
			//return true;
		}
		else
		{
			bEnterTable = JoinNoFullTable(pGamePlayer, 0);
			//return bEnterTable;
		}
	}
	else
	{
		tableID = pOldTable->GetTableID();
		enter_code = EnterTable(pGamePlayer, tableID, "");
		if (enter_code == net::RESULT_CODE_SUCCESS)
		{
			bEnterTable = true;
		}
	}
	LOG_ERROR("uid:%d,pOldTable:%p,bEnterTable:%d,bCanLeaveTable:%d,bLeaveTable:%d,bIsJoinMarry:%d,tableID:%d,enter_code:%d",
		pGamePlayer->GetUID(), pOldTable, bEnterTable, bCanLeaveTable, bLeaveTable, bIsJoinMarry, tableID, enter_code);

	if (bIsJoinMarry)
	{
		return bIsJoinMarry;
	}	
	return bEnterTable;
}

bool    CGameRoom::FastJoinTable(CGamePlayer* pGamePlayer)
{
	if (pGamePlayer == NULL)
	{
		return false;
	}
    if(pGamePlayer->GetRoom() != this){
        LOG_ERROR("not the same room - uid:%d",pGamePlayer->GetUID());
        return false;
    }
    if(GetRoomType() != emROOM_TYPE_COMMON)
        return false;
    CGameTable* pOldTable = pGamePlayer->GetTable();
	bool bEnterTable = false;
	bool bCanLeaveTable = false;
	bool bLeaveTable = false;
    if(pOldTable == NULL)
    {
        if(IsNeedMarry())
		{
            JoinMarry(pGamePlayer,0);
            return true;
        }
		else
		{
			bEnterTable = JoinNoFullTable(pGamePlayer, 0);
			return bEnterTable;
        }
    }
	else
	{
		bCanLeaveTable = pOldTable->CanLeaveTable(pGamePlayer);
		if (bCanLeaveTable)
		{
			bLeaveTable = pOldTable->LeaveTable(pGamePlayer);
		}
        if(bCanLeaveTable &&  bLeaveTable)
        {
            if(IsNeedMarry()) {
                JoinMarry(pGamePlayer,pOldTable->GetTableID());
                return true;
            }else{
				bEnterTable = JoinNoFullTable(pGamePlayer,pOldTable->GetTableID());
				return bEnterTable;
            }
        }
    }
	LOG_ERROR("uid:%d,pOldTable:%p,bEnterTable:%d,bCanLeaveTable:%d,bLeaveTable:%d -  can't join.", pGamePlayer->GetUID(), pOldTable, bEnterTable, bCanLeaveTable, bLeaveTable);
    return false;
}

bool    CGameRoom::FastJoinEveryColorTable(CGamePlayer* pGamePlayer)
{
	if (pGamePlayer == NULL || pGamePlayer->GetRoom() != this)
	{
		uint32 uid = 0;
		if (pGamePlayer != NULL)
		{
			uid = pGamePlayer->GetUID();
		}
		LOG_ERROR("not the same room - uid:%d", uid);
		return false;
	}
	if (GetRoomType() != emROOM_TYPE_COMMON)
	{
		return false;
	}
		
	CGameTable* pOldTable = pGamePlayer->GetTable();
	if (pOldTable!=NULL && pOldTable->CanLeaveTable(pGamePlayer) && pOldTable->LeaveTable(pGamePlayer))
	{
		return false;
	}
	vector<CGameTable*> readyTables;
	for(auto &it : m_mpTables)
	{
		CGameTable* pTable = it.second;
		if (pTable->CanEnterTable(pGamePlayer))
		{
			readyTables.push_back(pTable);
		}
	}

	sort(readyTables.begin(), readyTables.end(), CompareTableMarry);
	for(uint32 i = 0; i<readyTables.size(); ++i) {
		CGameTable* pTable = readyTables[i];
		if (pTable->EnterEveryColorTable(pGamePlayer)) {
			return true;
		}
	}

	LOG_ERROR("leave old table fail and can't join uid:%d,size:%d", pGamePlayer->GetUID(), readyTables.size());
	return false;
}

bool    CGameRoom::IsNeedMarry()
{
    return m_roomCfg.marry == 1;
}
bool    CGameRoom::CanEnterRoom(CGamePlayer* pGamePlayer)
{
    if(pGamePlayer->GetRoom() != NULL && pGamePlayer->GetRoom() != this)
    {
		LOG_DEBUG("haved join the other room - uid:%d", pGamePlayer->GetUID());
        return false;
    }
    if(GetRoomType() == emROOM_TYPE_COMMON)
    {
        if(m_roomCfg.limitEnter != 0)
        {
            if(GetPlayerCurScore(pGamePlayer) < GetEnterMin() || GetPlayerCurScore(pGamePlayer) > GetEnterMax())
            {
                LOG_DEBUG("less score - uid:%d,cur_score:%lld,roomid:%d,enter_min_score:%lld", pGamePlayer->GetUID(),GetPlayerCurScore(pGamePlayer), GetRoomID(), GetEnterMin());
                return false;
            }
			if (GetGameType()== net::GAME_CATE_BULLFIGHT)
			{
				if (pGamePlayer->IsRobot())
				{
					if (GetPlayerCurScore(pGamePlayer) >= GetRobotMaxScore()) {
						return false;
					}
				}
			}
        }
    }
	LOG_DEBUG("player can_enter_room - uid:%d,roomid:%d,m_showonline:%d,m_playerNum:%d", pGamePlayer->GetUID(),GetRoomID(), m_showonline, m_playerNum);

    return true;
}
bool    CGameRoom::CanLeaveRoom(CGamePlayer* pGamePlayer)
{
    CGameTable* pTable = pGamePlayer->GetTable();
    if(pTable != NULL && !pTable->CanLeaveTable(pGamePlayer))
        return false;

    return true;
}
int64   CGameRoom::GetPlayerCurScore(CGamePlayer* pPlayer)
{
    uint8 consumeType = GetConsume();
    switch(consumeType)
    {
    case net::ROOM_CONSUME_TYPE_SCORE:
        {
            return pPlayer->GetAccountValue(emACC_VALUE_SCORE);
        }break;
    case net::ROOM_CONSUME_TYPE_COIN:
        {
            return pPlayer->GetAccountValue(emACC_VALUE_COIN);
        }break;
    default:
        break;
    }
    return 0;
}
void    CGameRoom::GetRoomInfo(net::room_info* pRoom)
{
    pRoom->set_id(GetRoomID());
    pRoom->set_consume(m_roomCfg.consume);
    pRoom->set_deal(m_roomCfg.deal);
    pRoom->set_enter_min(m_roomCfg.enter_min);
    pRoom->set_enter_max(m_roomCfg.enter_max);
    pRoom->set_player_num(m_showonline);
    pRoom->set_basescore(m_roomCfg.baseScore);
    pRoom->set_show_type(m_roomCfg.showType);
    pRoom->set_show_pic(m_roomCfg.showPic);
	pRoom->set_jetton_min(m_roomCfg.jettonMinScore);

}
void    CGameRoom::SendTableListToPlayer(CGamePlayer* pGamePlayer,uint32 tableID)
{
    LOG_DEBUG("SendTableList to player");
    if(GetRoomType() != emROOM_TYPE_PRIVATE) {
        LOG_DEBUG("not private don't send tablelist");
        return;
    }
    uint8 tableCount = 0;
    net::msg_table_list_rep msg;
    if(tableID != 0){
        CGameTable* pTable = GetTable(tableID);
        if(pTable != NULL) {
            net::table_face_info *pInfo = msg.add_tables();
            pTable->GetTableFaceInfo(pInfo);
        }
        pGamePlayer->SendMsgToClient(&msg,net::S2C_MSG_TABLE_LIST);
        return;
    }
    vector<CGameTable*> tables;
    for(auto &it : m_mpTables)
    {
        CGameTable* pTable = it.second;
        // 过滤条件
        if(!pTable->IsShow() && pTable->GetHostID() != pGamePlayer->GetUID())
            continue;

        tables.push_back(pTable);
    }
    // 排序
    sort(tables.begin(),tables.end(),CompareTableList);

    for(uint32 i=0;i<tables.size() && i<s_MaxSendTable;++i){
        CGameTable* pTable = tables[i];
        net::table_face_info* pInfo = msg.add_tables();
        pTable->GetTableFaceInfo(pInfo);
    }
    pGamePlayer->SendMsgToClient(&msg,net::S2C_MSG_TABLE_LIST);
}
void    CGameRoom::QueryTableListToPlayer(CGamePlayer* pGamePlayer,uint32 start,uint32 end)
{
    uint8 tableCount = 0;
    net::msg_query_table_list_rep msg;
    vector<CGameTable*> tables;
    MAP_TABLE::iterator it = m_mpTables.begin();
    for(;it != m_mpTables.end();++it)
    {
        CGameTable* pTable = it->second;
        tables.push_back(pTable);
    }
    // 排序
    msg.set_table_num(tables.size());
    sort(tables.begin(),tables.end(),CompareTableID);
    for(uint32 i=start;i<tables.size() && i <= end && tableCount < s_MaxSendTable;++i)
    {
        CGameTable* pTable = tables[i];
        net::table_face_info* pInfo = msg.add_tables();
        pTable->GetTableFaceInfo(pInfo);
        tableCount++;
    }
    pGamePlayer->SendMsgToClient(&msg,net::S2C_MSG_QUERY_TABLE_LIST_REP);
}
// 继承房间配置信息
void    CGameRoom::InheritRoomCfg(CGameTable* pTable)
{
    stTableConf conf;
    conf.baseScore = GetBaseScore();
    conf.consume   = GetConsume();
    conf.deal      = GetDeal();
    conf.enterMin  = GetEnterMin();
    conf.enterMax  = GetEnterMax();
    conf.feeType   = m_roomCfg.feeType;
    conf.feeValue  = m_roomCfg.feeValue;
    conf.seatNum   = m_roomCfg.seatNum;

    pTable->SetTableConf(conf);
}
CGameTable* CGameRoom::MallocTable()
{
    CGameTable* pTable = NULL;
    if(!m_freeTable.empty())
    {
        pTable = m_freeTable.front();
        m_freeTable.pop();
    }else{
        uint32 tableID = ++m_roomIndex;
        pTable = CreateTable(tableID);
        InheritRoomCfg(pTable);
        pTable->Init();
    }
    pTable->ResetTable();
    m_mpTables.insert(make_pair(pTable->GetTableID(),pTable));
    LOG_DEBUG("room:%d malloctable:%d",GetRoomID(),pTable->GetTableID());
    return pTable;
}
void       CGameRoom::FreeTable(CGameTable* pTable)
{
    LOG_DEBUG("room:%d freetable:%d",GetRoomID(),pTable->GetTableID());
    m_mpTables.erase(pTable->GetTableID());
    if(GetRoomType() != emROOM_TYPE_PRIVATE) {
        m_freeTable.push(pTable);
    }else{
        SAFE_DELETE(pTable);
    }
}
CGameTable* CGameRoom::GetTable(uint32 tableID)
{
    auto it = m_mpTables.find(tableID);
    if(it != m_mpTables.end()){
        return it->second;
    }
    return NULL;
}
CGameTable* CGameRoom::CreatePlayerTable(CGamePlayer* pPlayer,stTableConf& cfg)
{
    if(GetRoomType() != emROOM_TYPE_PRIVATE){
        return NULL;
    }
    if(pPlayer->GetTable() != NULL){
        return NULL;
    }
    CGameTable* pTable = CreateTable(0);
    InheritRoomCfg(pTable);
    pTable->SetTableConf(cfg);
    pTable->Init();
    if(!pTable->CreatePrivateTable()){
        LOG_ERROR("create private table oper faild");
        SAFE_DELETE(pTable);
        return NULL;
    }
    pTable->ResetTable();
    m_mpTables.insert(make_pair(pTable->GetTableID(),pTable));
    if(!pTable->EnterTable(pPlayer)){
        LOG_DEBUG("can't join the self create table:%d",pPlayer->GetUID());
    }
    return pTable;
}
// 检测回收空桌子
void    CGameRoom::CheckRecover()
{
    vector<CGameTable*> vecRecovers;
    for(auto &it : m_mpTables)
    {
        CGameTable* pTable = it.second;
        if(pTable->NeedRecover()){
            vecRecovers.push_back(pTable);
        }
    }
    for(uint32 i=0;i<vecRecovers.size();++i){
        FreeTable(vecRecovers[i]);
    }
}
// 检测是否需要生成新桌子
void    CGameRoom::CheckNewTable()
{
	if (GetRoomType() != emROOM_TYPE_COMMON || IsNeedMarry()) {
		return;
	}
	uint16 gameType = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
	if (CCommonLogic::IsBaiRenGame(gameType)) {
		uint32 num = 0;
		for (auto &it : m_mpTables)
		{
			CGameTable* pTable = it.second;
			if (pTable->IsFullTable())
				num++;
		}
		if (num == m_mpTables.size()) {
			MallocTable();
		}
	}
	else {
		if (GetFreeTableNum() < 10) {
			LOG_DEBUG("not free table and create table");
			MallocTable();
		}
	}
}
// 匹配桌子
void    CGameRoom::MarryTable()
{
    if(GetRoomType() != emROOM_TYPE_PRIVATE && IsNeedMarry())// 不是私人房
    {
        CGameTable* pTable = NULL;
        while(!m_marryPlayers.empty())
        {
            auto it = m_marryPlayers.begin();
            CGamePlayer* pPlayer = it->first;
            if(JoinNoFullTable(pPlayer,it->second)){
                LeaveMarry(pPlayer);
                continue;
            }
            if(pTable == NULL || pTable->IsFullTable()){
                pTable = MallocTable();
            }
            if(pTable->EnterTable(pPlayer)){
                LeaveMarry(pPlayer);
            }else{
                LeaveRoom(pPlayer);
            }
        }
    }
}
// 进入空闲桌子
bool    CGameRoom::JoinNoFullTable(CGamePlayer* pPlayer,uint32 excludeID)
{
	if (pPlayer == NULL) {
		return false;
	}
    vector<CGameTable*> readyTables;
    for(auto &it : m_mpTables)
    {
        CGameTable* pTable = it.second;
        if(pTable->GetTableID() != excludeID && pTable->CanEnterTable(pPlayer))
        {
			readyTables.push_back(pTable);
        }
    }
    sort(readyTables.begin(),readyTables.end(),CompareTableMarry);
    for(uint32 i=0;i<readyTables.size();++i){
        CGameTable* pTable = readyTables[i];
        if(pTable->EnterTable(pPlayer)){
            return true;
        }
    }
    LOG_ERROR("not table join - uid:%d,GameType:%d,roomid:%d,m_mpTables.size:%d,excludeID:%d,readyTables_size:%d",
		pPlayer->GetUID(),GetGameType(),GetRoomID(),m_mpTables.size(), excludeID,readyTables.size());
    return false;
}
// 加入匹配
void    CGameRoom::JoinMarry(CGamePlayer* pPlayer,uint32 excludeID)
{
    if(GetRoomType() == emROOM_TYPE_PRIVATE)
        return;
    if(IsJoinMarry(pPlayer))
        return;
    pPlayer->SetAutoReady(true);
    m_marryPlayers.insert(make_pair(pPlayer,excludeID));
}
void    CGameRoom::LeaveMarry(CGamePlayer* pPlayer)
{
    if(GetRoomType() == emROOM_TYPE_PRIVATE)
        return;

    m_marryPlayers.erase(pPlayer);
}
bool    CGameRoom::IsJoinMarry(CGamePlayer* pPlayer)
{
    return (m_marryPlayers.find(pPlayer) != m_marryPlayers.end()); 
}
uint32  CGameRoom::GetFreeTableNum()
{
    uint32 num = m_freeTable.size();
    for(auto &it : m_mpTables)
    {
        CGameTable* pTable = it.second;
        if(!pTable->IsFullTable())
            num++;
    }
    
    return num;
}
// 获取所有桌子
void    CGameRoom::GetAllFreeTable(vector<CGameTable*>& tables)
{
    for(auto &it : m_mpTables)
    {
        CGameTable* pTable = it.second;
        if(pTable->GetGameState() == TABLE_STATE_FREE
           && !pTable->IsFullTable())
        {
            tables.push_back(pTable);
        }
    }
}
// 比较桌子函数(私人桌列表)
bool    CGameRoom::CompareTableList(CGameTable* pTable1,CGameTable* pTable2)
{
    if(pTable1->GetOnlinePlayerNum() == pTable2->GetOnlinePlayerNum())
    {
        if(pTable1->GetPlayerNum() == pTable2->GetPlayerNum()){
            return pTable1->GetTableID() < pTable2->GetTableID();
        }    
        return pTable1->GetPlayerNum() > pTable2->GetPlayerNum();
    }
    return pTable1->GetOnlinePlayerNum() > pTable2->GetOnlinePlayerNum();
}
// 桌子排队比较函数(匹配)
bool    CGameRoom::CompareTableMarry(CGameTable* pTable1,CGameTable* pTable2)
{
    if(pTable1->GetReadyNum() == pTable2->GetReadyNum())
    {
        if(pTable1->GetPlayerNum() == pTable2->GetPlayerNum()){
            return pTable1->GetTableID() < pTable2->GetTableID();
        }
        return pTable1->GetPlayerNum() > pTable2->GetPlayerNum();
    }
    return pTable1->GetReadyNum() > pTable2->GetReadyNum();
}
// 桌子ID排序
bool    CGameRoom::CompareTableID(CGameTable* pTable1,CGameTable* pTable2)
{
    return pTable1->GetTableID() < pTable2->GetTableID();
}

void    CGameRoom::CalcShowOnline()
{
    if(m_roomCfg.showonline == 0){
        m_showonline = m_playerNum;
    }else{
        m_showonline = m_playerNum*m_roomCfg.showonline + g_RandGen.RandUInt()%100;
    }
}

int CGameRoom::CheckRoommParambyGameType(uint32 gametype, string param) {

	LOG_DEBUG("check room param - gametype:%d, param:%s", gametype, param.c_str());

	bool bflag = 0 /*true*/; // modify by har
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - gametype:%d, param:%s", gametype, param.c_str());
		return bflag /*true*/;
	}
	if (gametype == net::GAME_CATE_BULLFIGHT) {
		if (!jvalue.isMember("n1")) { bflag = 300 /*false*/; }
		if (!jvalue.isMember("n2")) { bflag = 301 /*false*/; }
		if (!jvalue.isMember("n3")) { bflag = 302 /*false*/; }
		if (!jvalue.isMember("n4")) { bflag = 303 /*false*/; }
		if (!jvalue.isMember("n5")) { bflag = 304 /*false*/; }
		if (!jvalue.isMember("n6")) { bflag = 305 /*false*/; }
		if (!jvalue.isMember("n7")) { bflag = 306 /*false*/; }
		if (!jvalue.isMember("n8")) { bflag = 307 /*false*/; }
		if (!jvalue.isMember("n9")) { bflag = 308 /*false*/; }
		if (!jvalue.isMember("nn")) { bflag = 309 /*false*/; }
		if (!jvalue.isMember("bn")) { bflag = 310 /*false*/; }
		if (!jvalue.isMember("sn")) { bflag = 311 /*false*/; }
		if (!jvalue.isMember("bb")) { bflag = 312 /*false*/; }
		if (!jvalue.isMember("bbw")) { bflag = 313 /*false*/; }
		if (!jvalue.isMember("rmc")) { bflag = 314 /*false*/; }
		if (!jvalue.isMember("mt")) { bflag = 315 /*false*/; }
		if (!jvalue.isMember("aps")) { bflag = 316 /*false*/; }
		if (!jvalue.isMember("ips")) { bflag = 317 /*false*/; }
		if (!jvalue.isMember("ucp")) { bflag = 318 /*false*/; }
		if (!jvalue.isMember("cps")) { bflag = 319 /*false*/; }
		if (!jvalue.isMember("swp")) { bflag = 320 /*false*/; }
		if (!jvalue.isMember("slp")) { bflag = 321 /*false*/; }
		if (!jvalue.isMember("lsc")) { bflag = 322 /*false*/; }
		if (!jvalue.isMember("ujs")) { bflag = 323 /*false*/; }
		if (!jvalue.isMember("ujc")) { bflag = 324 /*false*/; }

		if (bflag == 0) {
			if (!jvalue["n1"].isIntegral()) { bflag = 350 /*false*/; }
			if (!jvalue["n2"].isIntegral()) { bflag = 351 /*false*/; }
			if (!jvalue["n3"].isIntegral()) { bflag = 352 /*false*/; }
			if (!jvalue["n4"].isIntegral()) { bflag = 353 /*false*/; }
			if (!jvalue["n5"].isIntegral()) { bflag = 354 /*false*/; }
			if (!jvalue["n6"].isIntegral()) { bflag = 355 /*false*/; }
			if (!jvalue["n7"].isIntegral()) { bflag = 356 /*false*/; }
			if (!jvalue["n8"].isIntegral()) { bflag = 357 /*false*/; }
			if (!jvalue["n9"].isIntegral()) { bflag = 358 /*false*/; }
			if (!jvalue["nn"].isIntegral()) { bflag = 359 /*false*/; }
			if (!jvalue["bn"].isIntegral()) { bflag = 360 /*false*/; }
			if (!jvalue["sn"].isIntegral()) { bflag = 361 /*false*/; }
			if (!jvalue["bb"].isIntegral()) { bflag = 362 /*false*/; }
			if (!jvalue["bbw"].isIntegral()) { bflag = 363 /*false*/; }
			if (!jvalue["rmc"].isIntegral()) { bflag = 364 /*false*/; }
			if (!jvalue["mt"].isIntegral()) { bflag = 365 /*false*/; }
			if (!jvalue["aps"].isIntegral()) { bflag = 366 /*false*/; }
			if (!jvalue["ips"].isIntegral()) { bflag = 367 /*false*/; }
			if (!jvalue["ucp"].isIntegral()) { bflag = 368 /*false*/; }
			if (!jvalue["cps"].isIntegral()) { bflag = 369 /*false*/; }
			if (!jvalue["swp"].isIntegral()) { bflag = 370 /*false*/; }
			if (!jvalue["slp"].isIntegral()) { bflag = 371 /*false*/; }
			if (!jvalue["lsc"].isIntegral()) { bflag = 372 /*false*/; }
			if (!jvalue["ujs"].isIntegral()) { bflag = 373 /*false*/; }
			if (!jvalue["ujc"].isIntegral()) { bflag = 374 /*false*/; }

		}

	}
	else if (gametype == net::GAME_CATE_SHOWHAND) {
		if (!jvalue.isMember("bbw")) { bflag = 200 /*false*/; }
		if (!jvalue.isMember("aps")) { bflag = 201 /*false*/; }
		if (!jvalue.isMember("ips")) { bflag = 202 /*false*/; }
		if (!jvalue.isMember("ucp")) { bflag = 203 /*false*/; }
		if (!jvalue.isMember("cps")) { bflag = 204 /*false*/; }
		if (!jvalue.isMember("swp")) { bflag = 205 /*false*/; }
		if (!jvalue.isMember("slp")) { bflag = 206 /*false*/; }
		if (!jvalue.isMember("lsc")) { bflag = 207 /*false*/; }
		if (!jvalue.isMember("ujs")) { bflag = 208 /*false*/; }
		if (!jvalue.isMember("ujc")) { bflag = 209 /*false*/; }
		if (!jvalue.isMember("mws")) { bflag = 210 /*false*/; }
		if (!jvalue.isMember("pr0")) { bflag = 211 /*false*/; }
		if (!jvalue.isMember("pr1")) { bflag = 212 /*false*/; }
		if (!jvalue.isMember("pr2")) { bflag = 213 /*false*/; }
		if (!jvalue.isMember("pr3")) { bflag = 214 /*false*/; }
		if (!jvalue.isMember("pr4")) { bflag = 215 /*false*/; }
		if (!jvalue.isMember("pr5")) { bflag = 216 /*false*/; }
		if (!jvalue.isMember("pr6")) { bflag = 217 /*false*/; }
		if (!jvalue.isMember("pr7")) { bflag = 218 /*false*/; }
		if (!jvalue.isMember("pr8")) { bflag = 219 /*false*/; }
		if (bflag == 0) {
			if (!jvalue["bbw"].isIntegral()) { bflag = 250 /*false*/; }
			if (!jvalue["aps"].isIntegral()) { bflag = 251 /*false*/; }
			if (!jvalue["ips"].isIntegral()) { bflag = 252 /*false*/; }
			if (!jvalue["ucp"].isIntegral()) { bflag = 253 /*false*/; }
			if (!jvalue["cps"].isIntegral()) { bflag = 254 /*false*/; }
			if (!jvalue["swp"].isIntegral()) { bflag = 255 /*false*/; }
			if (!jvalue["slp"].isIntegral()) { bflag = 256 /*false*/; }
			if (!jvalue["lsc"].isIntegral()) { bflag = 257 /*false*/; }
			if (!jvalue["ujs"].isIntegral()) { bflag = 258 /*false*/; }
			if (!jvalue["ujc"].isIntegral()) { bflag = 259 /*false*/; }
			if (!jvalue["mws"].isIntegral()) { bflag = 260 /*false*/; }
			if (!jvalue["pr0"].isIntegral()) { bflag = 261 /*false*/; }
			if (!jvalue["pr1"].isIntegral()) { bflag = 262 /*false*/; }
			if (!jvalue["pr2"].isIntegral()) { bflag = 263 /*false*/; }
			if (!jvalue["pr3"].isIntegral()) { bflag = 264 /*false*/; }
			if (!jvalue["pr4"].isIntegral()) { bflag = 265 /*false*/; }
			if (!jvalue["pr5"].isIntegral()) { bflag = 266 /*false*/; }
			if (!jvalue["pr6"].isIntegral()) { bflag = 267 /*false*/; }
			if (!jvalue["pr7"].isIntegral()) { bflag = 268 /*false*/; }
			if (!jvalue["pr8"].isIntegral()) { bflag = 269 /*false*/; }
		}
	}
	else if (gametype == net::GAME_CATE_NIUNIU) {
		//if (!jvalue.isMember("bbw")) { bflag = false; }
		//if (!jvalue.isMember("aps")) { bflag = false; }
		//if (!jvalue.isMember("ips")) { bflag = false; }
		//if (!jvalue.isMember("ucp")) { bflag = false; }
		//if (!jvalue.isMember("cps")) { bflag = false; }
		//if (!jvalue.isMember("swp")) { bflag = false; }
		//if (!jvalue.isMember("slp")) { bflag = false; }
		//if (!jvalue.isMember("lsc")) { bflag = false; }
		//if (!jvalue.isMember("ujs")) { bflag = false; }
		//if (!jvalue.isMember("ujc")) { bflag = false; }

		//if (bflag) {
		//	if (!jvalue["bbw"].isIntegral()) { bflag = false; }
		//	if (!jvalue["aps"].isIntegral()) { bflag = false; }
		//	if (!jvalue["ips"].isIntegral()) { bflag = false; }
		//	if (!jvalue["ucp"].isIntegral()) { bflag = false; }
		//	if (!jvalue["cps"].isIntegral()) { bflag = false; }
		//	if (!jvalue["swp"].isIntegral()) { bflag = false; }
		//	if (!jvalue["slp"].isIntegral()) { bflag = false; }
		//	if (!jvalue["lsc"].isIntegral()) { bflag = false; }
		//	if (!jvalue["ujs"].isIntegral()) { bflag = false; }
		//	if (!jvalue["ujc"].isIntegral()) { bflag = false; }
		//}
	}
	else if (gametype == net::GAME_CATE_PAIJIU) {

	    if (!jvalue.isMember("bbw")) { bflag = 900 /*false*/; }
	    if (!jvalue.isMember("pbl")) { bflag = 901 /*false*/; }
	    if (!jvalue.isMember("aps")) { bflag = 902 /*false*/; }
	    if (!jvalue.isMember("ips")) { bflag = 903 /*false*/; }
	    if (!jvalue.isMember("ucp")) { bflag = 904 /*false*/; }
	    if (!jvalue.isMember("cps")) { bflag = 905 /*false*/; }
	    if (!jvalue.isMember("swp")) { bflag = 906 /*false*/; }
	    if (!jvalue.isMember("slp")) { bflag = 907 /*false*/; }
	    if (!jvalue.isMember("lsc")) { bflag = 908 /*false*/; }
	    if (!jvalue.isMember("ujs")) { bflag = 909 /*false*/; }
	    if (!jvalue.isMember("ujc")) { bflag = 910 /*false*/; }

		if (bflag == 0) {
			if (!jvalue["bbw"].isIntegral()) { bflag = 950 /*false*/; }
			if (!jvalue["pbl"].isIntegral()) { bflag = 951 /*false*/; }
			if (!jvalue["aps"].isIntegral()) { bflag = 952 /*false*/; }
			if (!jvalue["ips"].isIntegral()) { bflag = 953 /*false*/; }
			if (!jvalue["ucp"].isIntegral()) { bflag = 954 /*false*/; }
			if (!jvalue["cps"].isIntegral()) { bflag = 955 /*false*/; }
			if (!jvalue["swp"].isIntegral()) { bflag = 956 /*false*/; }
			if (!jvalue["slp"].isIntegral()) { bflag = 957 /*false*/; }
			if (!jvalue["lsc"].isIntegral()) { bflag = 958 /*false*/; }
			if (!jvalue["ujs"].isIntegral()) { bflag = 959 /*false*/; }
			if (!jvalue["ujc"].isIntegral()) { bflag = 960 /*false*/; }
		}
	}
	else if (gametype == net::GAME_CATE_BACCARAT) {

	    if (!jvalue.isMember("pbl")) { bflag = 700 /*false*/; }
	    if (!jvalue.isMember("aps")) { bflag = 701 /*false*/; }
	    if (!jvalue.isMember("ips")) { bflag = 702 /*false*/; }
	    if (!jvalue.isMember("ucp")) { bflag = 703 /*false*/; }
	    if (!jvalue.isMember("cps")) { bflag = 704 /*false*/; }
	    if (!jvalue.isMember("usp")) { bflag = 705 /*false*/; }
	    if (!jvalue.isMember("swp")) { bflag = 706 /*false*/; }
	    if (!jvalue.isMember("slp")) { bflag = 707 /*false*/; }
	    if (!jvalue.isMember("lsc")) { bflag = 708 /*false*/; }
	    if (!jvalue.isMember("ujs")) { bflag = 709 /*false*/; }
	    if (!jvalue.isMember("ujc")) { bflag = 710 /*false*/; }

		if (bflag == 0) {
			if (!jvalue["pbl"].isIntegral()) { bflag = 750 /*false*/; }
			if (!jvalue["aps"].isIntegral()) { bflag = 751 /*false*/; }
			if (!jvalue["ips"].isIntegral()) { bflag = 752 /*false*/; }
			if (!jvalue["ucp"].isIntegral()) { bflag = 753 /*false*/; }
			if (!jvalue["cps"].isIntegral()) { bflag = 754 /*false*/; }
			if (!jvalue["usp"].isIntegral()) { bflag = 755 /*false*/; }
			if (!jvalue["swp"].isIntegral()) { bflag = 756 /*false*/; }
			if (!jvalue["slp"].isIntegral()) { bflag = 757 /*false*/; }
			if (!jvalue["lsc"].isIntegral()) { bflag = 758 /*false*/; }
			if (!jvalue["ujs"].isIntegral()) { bflag = 759 /*false*/; }
			if (!jvalue["ujc"].isIntegral()) { bflag = 760 /*false*/; }
		}
	}
	else if (gametype == net::GAME_CATE_ZAJINHUA) {
	    if (!jvalue.isMember("aps")) { bflag = 500 /*false*/; }
	    if (!jvalue.isMember("ips")) { bflag = 501 /*false*/; }
	    if (!jvalue.isMember("ucp")) { bflag = 502 /*false*/; }
	    if (!jvalue.isMember("cps")) { bflag = 503 /*false*/; }
	    if (!jvalue.isMember("swp")) { bflag = 504 /*false*/; }
	    if (!jvalue.isMember("slp")) { bflag = 505 /*false*/; }
	    if (!jvalue.isMember("lsc")) { bflag = 506 /*false*/; }
	    if (!jvalue.isMember("ujs")) { bflag = 507 /*false*/; }
	    if (!jvalue.isMember("ujc")) { bflag = 508 /*false*/; }
	    if (!jvalue.isMember("pr0")) { bflag = 509 /*false*/; }
	    if (!jvalue.isMember("pr1")) { bflag = 510 /*false*/; }
	    if (!jvalue.isMember("pr2")) { bflag = 511 /*false*/; }
	    if (!jvalue.isMember("pr3")) { bflag = 512 /*false*/; }
	    if (!jvalue.isMember("pr4")) { bflag = 513 /*false*/; }
	    if (!jvalue.isMember("pr5")) { bflag = 514 /*false*/; }
	    if (!jvalue.isMember("bbw")) { bflag = 515 /*false*/; }
	    if (bflag == 0) {
		    if (!jvalue["aps"].isIntegral()) { bflag = 550 /*false*/; }
		    if (!jvalue["ips"].isIntegral()) { bflag = 551 /*false*/; }
		    if (!jvalue["ucp"].isIntegral()) { bflag = 552 /*false*/; }
		    if (!jvalue["cps"].isIntegral()) { bflag = 553 /*false*/; }
		    if (!jvalue["swp"].isIntegral()) { bflag = 554 /*false*/; }
		    if (!jvalue["slp"].isIntegral()) { bflag = 555 /*false*/; }
		    if (!jvalue["lsc"].isIntegral()) { bflag = 556 /*false*/; }
		    if (!jvalue["ujs"].isIntegral()) { bflag = 557 /*false*/; }
		    if (!jvalue["ujc"].isIntegral()) { bflag = 558 /*false*/; }
		    if (!jvalue["pr0"].isIntegral()) { bflag = 559 /*false*/; }
		    if (!jvalue["pr1"].isIntegral()) { bflag = 560 /*false*/; }
		    if (!jvalue["pr2"].isIntegral()) { bflag = 561 /*false*/; }
		    if (!jvalue["pr3"].isIntegral()) { bflag = 562 /*false*/; }
		    if (!jvalue["pr4"].isIntegral()) { bflag = 563 /*false*/; }
		    if (!jvalue["pr5"].isIntegral()) { bflag = 564 /*false*/; }
		    if (!jvalue["bbw"].isIntegral()) { bflag = 565 /*false*/; }
	    }
	}
	else if (gametype == net::GAME_CATE_TEXAS) {

	    if (!jvalue.isMember("rmc")) { bflag = 400 /*false*/; }
	    if (!jvalue.isMember("mws")) { bflag = 401 /*false*/; }
	    if (!jvalue.isMember("pr0")) { bflag = 402 /*false*/; }
	    //if (!jvalue.isMember("pr1")) { bflag = 403 /*false*/; }
	    //if (!jvalue.isMember("pr2")) { bflag = 404 /*false*/; }
	    if (!jvalue.isMember("pr3")) { bflag = 405 /*false*/; }
	    if (!jvalue.isMember("pr4")) { bflag = 406 /*false*/; }
	    if (!jvalue.isMember("pr5")) { bflag = 407 /*false*/; }
	    if (!jvalue.isMember("pr6")) { bflag = 408 /*false*/; }
	    if (!jvalue.isMember("pr7")) { bflag = 409 /*false*/; }
	    if (!jvalue.isMember("pr8")) { bflag = 410 /*false*/; }
	    if (!jvalue.isMember("pr9")) { bflag = 411 /*false*/; }
	    if (bflag == 0) {
		    if (!jvalue["rmc"].isIntegral()) { bflag = 450 /*false*/; }
		    if (!jvalue["mws"].isIntegral()) { bflag = 451 /*false*/; }
		    if (!jvalue["pr0"].isIntegral()) { bflag = 452 /*false*/; }
		    //if (!jvalue["pr1"].isIntegral()) { bflag = 453 /*false*/; }
		    //if (!jvalue["pr2"].isIntegral()) { bflag = 454 /*false*/; }
		    if (!jvalue["pr3"].isIntegral()) { bflag = 455 /*false*/; }
		    if (!jvalue["pr4"].isIntegral()) { bflag = 456 /*false*/; }
		    if (!jvalue["pr5"].isIntegral()) { bflag = 457 /*false*/; }
		    if (!jvalue["pr6"].isIntegral()) { bflag = 458 /*false*/; }
		    if (!jvalue["pr7"].isIntegral()) { bflag = 459 /*false*/; }
		    if (!jvalue["pr8"].isIntegral()) { bflag = 460 /*false*/; }
		    if (!jvalue["pr9"].isIntegral()) { bflag = 461 /*false*/; }
	    }
	}
	else if (gametype == net::GAME_CATE_EVERYCOLOR) {
	    if (!jvalue.isMember("fcp")) { bflag = 1000 /*false*/; }
	    if (!jvalue.isMember("bsp")) { bflag = 1001 /*false*/; }
	    if (!jvalue.isMember("bss")) { bflag = 1002 /*false*/; }
	    if (!jvalue.isMember("fcy")) { bflag = 1003 /*false*/; }
	    if (!jvalue.isMember("fcj")) { bflag = 1004 /*false*/; }
	    if (!jvalue.isMember("bsy")) { bflag = 1005 /*false*/; }
	    if (!jvalue.isMember("bsj")) { bflag = 1006 /*false*/; }
	    if (!jvalue.isMember("rmaxs")) { bflag = 1007 /*false*/; }
	    if (!jvalue.isMember("rmins")) { bflag = 1008 /*false*/; }
	    if (!jvalue.isMember("swp")) { bflag = 1009 /*false*/; }
	    if (!jvalue.isMember("slp")) { bflag = 1010 /*false*/; }
	    if (!jvalue.isMember("clw")) { bflag = 1011 /*false*/; }
	    if (!jvalue.isMember("lwp")) { bflag = 1012 /*false*/; }
	    if (!jvalue.isMember("ujs")) { bflag = 1013 /*false*/; }
	    if (!jvalue.isMember("srp")) { bflag = 1014 /*false*/; }
		if (bflag == 0) {
			if (!jvalue["fcp"].isIntegral()) { bflag = 1050 /*false*/; }
			if (!jvalue["bsp"].isIntegral()) { bflag = 1051 /*false*/; }
			if (!jvalue["bss"].isIntegral()) { bflag = 1052 /*false*/; }
			if (!jvalue["fcy"].isIntegral()) { bflag = 1053 /*false*/; }
			if (!jvalue["fcj"].isIntegral()) { bflag = 1054 /*false*/; }
			if (!jvalue["bsy"].isIntegral()) { bflag = 1055 /*false*/; }
			if (!jvalue["bsj"].isIntegral()) { bflag = 1056 /*false*/; }
			if (!jvalue["rmaxs"].isIntegral()) { bflag = 1057 /*false*/; }
			if (!jvalue["rmins"].isIntegral()) { bflag = 1058 /*false*/; }
			if (!jvalue["swp"].isIntegral()) { bflag = 1059 /*false*/; }
			if (!jvalue["slp"].isIntegral()) { bflag = 1060 /*false*/; }
			if (!jvalue["clw"].isIntegral()) { bflag = 1061 /*false*/; }
			if (!jvalue["lwp"].isIntegral()) { bflag = 1062 /*false*/; }
			if (!jvalue["ujs"].isIntegral()) { bflag = 1063 /*false*/; }
			if (!jvalue["srp"].isIntegral()) { bflag = 1064 /*false*/; }
		}
	}
	else if (gametype == net::GAME_CATE_FRUIT_MACHINE)
	{
		bflag = true;
	}
	else if (gametype == net::GAME_CATE_WAR)
	{
	    if (!jvalue.isMember("pr0")) { bflag = 1400 /*false*/; }
	    if (!jvalue.isMember("pr1")) { bflag = 1401 /*false*/; }
	    if (!jvalue.isMember("pr2")) { bflag = 1402 /*false*/; }
	    if (!jvalue.isMember("pr3")) { bflag = 1403 /*false*/; }
	    if (!jvalue.isMember("pr4")) { bflag = 1404 /*false*/; }
	    if (!jvalue.isMember("pr5")) { bflag = 1405 /*false*/; }
	    if (!jvalue.isMember("sbw")) { bflag = 1406 /*false*/; }
	    if (bflag == 0)
	    {
		    if (!jvalue["pr0"].isIntegral()) { bflag = 1450 /*false*/; }
		    if (!jvalue["pr1"].isIntegral()) { bflag = 1451 /*false*/; }
		    if (!jvalue["pr2"].isIntegral()) { bflag = 1452 /*false*/; }
		    if (!jvalue["pr3"].isIntegral()) { bflag = 1453 /*false*/; }
		    if (!jvalue["pr4"].isIntegral()) { bflag = 1454 /*false*/; }
		    if (!jvalue["pr5"].isIntegral()) { bflag = 1455 /*false*/; }
		    if (!jvalue["sbw"].isIntegral()) { bflag = 1456 /*false*/; }
	    }
	}
	else if (gametype == net::GAME_CATE_LAND)
	{
	    if (!jvalue.isMember("isaddblockers")) { bflag = 100 /*false*/; }
	    if (!jvalue.isMember("bbw")) { bflag = 101 /*false*/; }
	    if (bflag == 0)
	    {
		    if (!jvalue["isaddblockers"].isIntegral()) { bflag = 150 /*false*/; }
		    if (!jvalue["bbw"].isIntegral()) { bflag = 151 /*false*/; }
	    }
	}
	else if (gametype == net::GAME_CATE_TWO_PEOPLE_MAJIANG)
	{
	    if (!jvalue.isMember("pr0")) { bflag = 1200 /*false*/; }
	    if (!jvalue.isMember("pr1")) { bflag = 1201 /*false*/; }
	    if (!jvalue.isMember("pr2")) { bflag = 1202 /*false*/; }
	    if (!jvalue.isMember("pr3")) { bflag = 1203 /*false*/; }
	    if (!jvalue.isMember("pr4")) { bflag = 1204 /*false*/; }
	    if (!jvalue.isMember("pr5")) { bflag = 1205 /*false*/; }
	    if (!jvalue.isMember("pr6")) { bflag = 1206 /*false*/; }
	    if (!jvalue.isMember("pr7")) { bflag = 1207 /*false*/; }
	    if (!jvalue.isMember("pr8")) { bflag = 1208 /*false*/; }
	    if (!jvalue.isMember("zmp")) { bflag = 1209 /*false*/; }
	    if (bflag == 0)
	    {
		    if (!jvalue["pr0"].isIntegral()) { bflag = 1250 /*false*/; }
		    if (!jvalue["pr1"].isIntegral()) { bflag = 1251 /*false*/; }
		    if (!jvalue["pr2"].isIntegral()) { bflag = 1252 /*false*/; }
		    if (!jvalue["pr3"].isIntegral()) { bflag = 1253 /*false*/; }
		    if (!jvalue["pr4"].isIntegral()) { bflag = 1254 /*false*/; }
		    if (!jvalue["pr5"].isIntegral()) { bflag = 1255 /*false*/; }
		    if (!jvalue["pr6"].isIntegral()) { bflag = 1256 /*false*/; }
		    if (!jvalue["pr7"].isIntegral()) { bflag = 1257 /*false*/; }
		    if (!jvalue["zmp"].isIntegral()) { bflag = 1259 /*false*/; }
	    }
	}
	else if (gametype == net::GAME_CATE_FIGHT)
	{
	    if (!jvalue.isMember("pr0")) { bflag = 1500 /*false*/; }
	    if (!jvalue.isMember("pr1")) { bflag = 1501 /*false*/; }
	    if (!jvalue.isMember("pr2")) { bflag = 1502 /*false*/; }
	    if (!jvalue.isMember("sbw")) { bflag = 1503 /*false*/; }
	    if (bflag == 0)
	    {
		    if (!jvalue["pr0"].isIntegral()) { bflag = 1550 /*false*/; }
		    if (!jvalue["pr1"].isIntegral()) { bflag = 1551 /*false*/; }
		    if (!jvalue["pr2"].isIntegral()) { bflag = 1552 /*false*/; }
		    if (!jvalue["sbw"].isIntegral()) { bflag = 1553 /*false*/; }
	    }
	}
	else if (gametype == net::GAME_CATE_ROBNIU)
	{
	}
	else if (gametype == net::GAME_CATE_FISHING)
	{
	}
	else {
		bflag = 1 /*false*/;
	}
	return bflag;
}

void CGameRoom::ReAnalysisRoomParam() {
	ReAnalysisParam();

	for (auto &it : m_mpTables)
	{
		CGameTable* pTable = it.second;
		if (pTable != NULL)
		{
			pTable->ReAnalysisParam();
		}		
	}
}

void CGameRoom::UpdateControlPlayer(uint32 uid, uint32 gamecount,uint32 operatetype) {
	for (auto &it : m_mpTables)
	{
		CGameTable* pTable = it.second;
		if (pTable != NULL)
		{
			pTable->UpdateControlPlayer(uid, gamecount, operatetype);
		}
	}
}
void CGameRoom::UpdateControlMultiPlayer(uint32 uid, uint32 gamecount, uint64 gametime, int64 totalscore, uint32 operatetype) {
	for (auto &it : m_mpTables)
	{
		CGameTable* pTable = it.second;
		if (pTable != NULL)
		{
			pTable->UpdateControlMultiPlayer(uid, gamecount, gametime, totalscore, operatetype);
		}
	}
}
bool CGameRoom::SetSnatchCoinState(uint32 stop) {

	for (auto &it : m_mpTables)
	{
		CGameTable* pTable = it.second;
		if (pTable != NULL)
		{
			pTable->SetSnatchCoinState(stop);
		}		
	}
	return true;
}

bool CGameRoom::DiceGameControlCard(uint32 gametype, uint32 roomid, uint32 udice[])
{
	for (auto &it : m_mpTables)
	{
		CGameTable* pTable = it.second;
		if (pTable != NULL)
		{
			pTable->DiceGameControlCard(gametype, roomid, udice);
		}
	}
	return true;
}

bool CGameRoom::MajiangConfigHandCard(uint32 gametype, uint32 roomid, string strHandCard)
{
	for (auto &it : m_mpTables)
	{
		CGameTable* pTable = it.second;
		if (pTable != NULL)
		{
			pTable->MajiangConfigHandCard(gametype, roomid, strHandCard);
		}
	}
	return true;
}

bool CGameRoom::SynControlPlayer(uint32 tableID,uint32 uid, int32 gamecount, uint32 operatetype)
{
	for (auto &it : m_mpTables)
	{
		CGameTable* pTable = it.second;
		if (pTable != NULL /*&& pTable->GetTableID() != tableID*/)
		{
			pTable->SynControlPlayer(uid, gamecount, operatetype);
		}
	}
	return true;
}

void CGameRoom::UpdateJackpotScore(int64 lScore)
{
	if (lScore == 0 || m_tagJackpotScore.iUserJackpotControl != 1 || m_tagJackpotScore.lUpdateJackpotScore <= 0)
	{
		return;
	}

	LOG_DEBUG("1 lScore:%lld,lCurPollScore:%lld,lDiffPollScore:%lld,uSysLostPro:%d,uSysWinPro:%d",lScore, m_tagJackpotScore.lCurPollScore, m_tagJackpotScore.lDiffPollScore, m_tagJackpotScore.uSysLostPro, m_tagJackpotScore.uSysWinPro);

	m_tagJackpotScore.UpdateCurPoolScore(lScore);

	if (m_tagJackpotScore.lCurPollScore>m_tagJackpotScore.lMinPollScore && m_tagJackpotScore.lCurPollScore<m_tagJackpotScore.lMaxPollScore)
	{
		LOG_DEBUG("2 lScore:%lld,lCurPollScore:%lld,lDiffPollScore:%lld,uSysLostPro:%d,uSysWinPro:%d", lScore, m_tagJackpotScore.lCurPollScore, m_tagJackpotScore.lDiffPollScore, m_tagJackpotScore.uSysLostPro, m_tagJackpotScore.uSysWinPro);

		return;
	}
	int iLoopCount = 0;
	if (lScore>0 && m_tagJackpotScore.lDiffPollScore>=0)
	{
		int64 lChangerScore = lScore + m_tagJackpotScore.lDiffPollScore; // 奖池增加 吐币增加 吃币减少
		iLoopCount = 0;
		while (lChangerScore >= m_tagJackpotScore.lUpdateJackpotScore)
		{
			iLoopCount++;
			if (iLoopCount > MAX_LOOP_COUNT)
			{
				LOG_DEBUG("loop error - lChangerScore:%lld,lUpdateJackpotScore：%lld", lChangerScore, m_tagJackpotScore.lUpdateJackpotScore);
				break;
			}
			if (m_tagJackpotScore.uSysLostPro + m_tagJackpotScore.uSysLostWinProChange < PRO_DENO_10000)
			{
				m_tagJackpotScore.uSysLostPro += m_tagJackpotScore.uSysLostWinProChange;
			}
			else if (m_tagJackpotScore.uSysLostPro + m_tagJackpotScore.uSysLostWinProChange >= PRO_DENO_10000)
			{
				m_tagJackpotScore.uSysLostPro = PRO_DENO_10000;
			}

			if (m_tagJackpotScore.uSysWinPro > m_tagJackpotScore.uSysLostWinProChange)
			{
				m_tagJackpotScore.uSysWinPro -= m_tagJackpotScore.uSysLostWinProChange;
			}
			else if (m_tagJackpotScore.uSysWinPro <= m_tagJackpotScore.uSysLostWinProChange)
			{
				m_tagJackpotScore.uSysWinPro = 0;
			}
			lChangerScore -= m_tagJackpotScore.lUpdateJackpotScore;
		}
		m_tagJackpotScore.lDiffPollScore = lChangerScore;
	}
	else if (lScore>0 && m_tagJackpotScore.lDiffPollScore<0)
	{
		int64 lChangerScore = lScore + m_tagJackpotScore.lDiffPollScore;
		if (lChangerScore > 0) // 奖池增加 吐币增加 吃币减少
		{
			iLoopCount = 0;
			while (lChangerScore >= m_tagJackpotScore.lUpdateJackpotScore)
			{
				iLoopCount++;
				if (iLoopCount > MAX_LOOP_COUNT)
				{
					LOG_DEBUG("loop error - lChangerScore:%lld,lUpdateJackpotScore：%lld", lChangerScore, m_tagJackpotScore.lUpdateJackpotScore);
					break;
				}
				if (m_tagJackpotScore.uSysLostPro + m_tagJackpotScore.uSysLostWinProChange < PRO_DENO_10000)
				{
					m_tagJackpotScore.uSysLostPro += m_tagJackpotScore.uSysLostWinProChange;
				}
				else if (m_tagJackpotScore.uSysLostPro + m_tagJackpotScore.uSysLostWinProChange >= PRO_DENO_10000)
				{
					m_tagJackpotScore.uSysLostPro = PRO_DENO_10000;
				}

				if (m_tagJackpotScore.uSysWinPro > m_tagJackpotScore.uSysLostWinProChange)
				{
					m_tagJackpotScore.uSysWinPro -= m_tagJackpotScore.uSysLostWinProChange;
				}
				else if (m_tagJackpotScore.uSysWinPro <= m_tagJackpotScore.uSysLostWinProChange)
				{
					m_tagJackpotScore.uSysWinPro = 0;
				}
				lChangerScore -= m_tagJackpotScore.lUpdateJackpotScore;
			}
			m_tagJackpotScore.lDiffPollScore = lChangerScore;
		}
		else if (lChangerScore <= 0)
		{
			m_tagJackpotScore.lDiffPollScore = lChangerScore;

			//LOG_DEBUG("3 lScore:%lld,lCurPollScore:%lld,lDiffPollScore:%lld,uSysLostPro:%d,uSysWinPro:%d", lScore, m_tagJackpotScore.lCurPollScore, m_tagJackpotScore.lDiffPollScore, m_tagJackpotScore.uSysLostPro, m_tagJackpotScore.uSysWinPro);

			//return;
		}

	}
	else if (lScore<0 && m_tagJackpotScore.lDiffPollScore >= 0)
	{
		int64 lChangerScore = lScore + m_tagJackpotScore.lDiffPollScore;
		if (lChangerScore<0) // 奖池减少 吃币增加 吐币减少
		{
			lChangerScore = (-lChangerScore);

			//LOG_DEBUG("44 lScore:%lld,lCurPollScore:%lld,lDiffPollScore:%lld,uSysLostPro:%d,uSysWinPro:%d,lChangerScore:%lld", lScore, m_tagJackpotScore.lCurPollScore, m_tagJackpotScore.lDiffPollScore, m_tagJackpotScore.uSysLostPro, m_tagJackpotScore.uSysWinPro, lChangerScore);
			iLoopCount = 0;
			while (lChangerScore >= m_tagJackpotScore.lUpdateJackpotScore)
			{
				iLoopCount++;
				if (iLoopCount > MAX_LOOP_COUNT)
				{
					LOG_DEBUG("loop error - lChangerScore:%lld,lUpdateJackpotScore：%lld", lChangerScore, m_tagJackpotScore.lUpdateJackpotScore);
					break;
				}
				if (m_tagJackpotScore.uSysWinPro + m_tagJackpotScore.uSysLostWinProChange < PRO_DENO_10000)
				{
					m_tagJackpotScore.uSysWinPro += m_tagJackpotScore.uSysLostWinProChange;
				}
				else if (m_tagJackpotScore.uSysWinPro + m_tagJackpotScore.uSysLostWinProChange >= PRO_DENO_10000)
				{
					m_tagJackpotScore.uSysWinPro = PRO_DENO_10000;
				}

				if (m_tagJackpotScore.uSysLostPro > m_tagJackpotScore.uSysLostWinProChange)
				{
					m_tagJackpotScore.uSysLostPro -= m_tagJackpotScore.uSysLostWinProChange;
				}
				else if (m_tagJackpotScore.uSysLostPro <= m_tagJackpotScore.uSysLostWinProChange)
				{
					m_tagJackpotScore.uSysLostPro = 0;
				}

				lChangerScore -= m_tagJackpotScore.lUpdateJackpotScore;
			}

			m_tagJackpotScore.lDiffPollScore = (-lChangerScore);

			//LOG_DEBUG("444 lScore:%lld,lCurPollScore:%lld,lDiffPollScore:%lld,uSysLostPro:%d,uSysWinPro:%d,lChangerScore:%lld", lScore, m_tagJackpotScore.lCurPollScore, m_tagJackpotScore.lDiffPollScore, m_tagJackpotScore.uSysLostPro, m_tagJackpotScore.uSysWinPro, lChangerScore);
			//return;
		}
		else if (lChangerScore >= 0)
		{
			m_tagJackpotScore.lDiffPollScore = lChangerScore;

			//LOG_DEBUG("4 lScore:%lld,lCurPollScore:%lld,lDiffPollScore:%lld,uSysLostPro:%d,uSysWinPro:%d", lScore, m_tagJackpotScore.lCurPollScore, m_tagJackpotScore.lDiffPollScore, m_tagJackpotScore.uSysLostPro, m_tagJackpotScore.uSysWinPro);

			//return;
		}
	}
	else if (lScore<0 && m_tagJackpotScore.lDiffPollScore<0)
	{
		int64 lChangerScore = -(lScore + m_tagJackpotScore.lDiffPollScore); // 奖池减少 吃币增加 吐币减少
		iLoopCount = 0;
		while (lChangerScore >= m_tagJackpotScore.lUpdateJackpotScore)
		{
			iLoopCount++;
			if (iLoopCount > MAX_LOOP_COUNT)
			{
				LOG_DEBUG("loop error - lChangerScore:%lld,lUpdateJackpotScore：%lld", lChangerScore, m_tagJackpotScore.lUpdateJackpotScore);
				break;
			}
			if (m_tagJackpotScore.uSysWinPro + m_tagJackpotScore.uSysLostWinProChange < PRO_DENO_10000)
			{
				m_tagJackpotScore.uSysWinPro += m_tagJackpotScore.uSysLostWinProChange;
			}
			else if (m_tagJackpotScore.uSysWinPro + m_tagJackpotScore.uSysLostWinProChange >= PRO_DENO_10000)
			{
				m_tagJackpotScore.uSysWinPro = PRO_DENO_10000;
			}

			if (m_tagJackpotScore.uSysLostPro > m_tagJackpotScore.uSysLostWinProChange)
			{
				m_tagJackpotScore.uSysLostPro -= m_tagJackpotScore.uSysLostWinProChange;
			}
			else if (m_tagJackpotScore.uSysLostPro <= m_tagJackpotScore.uSysLostWinProChange)
			{
				m_tagJackpotScore.uSysLostPro = 0;
			}

			lChangerScore -= m_tagJackpotScore.lUpdateJackpotScore;
		}
		m_tagJackpotScore.lDiffPollScore = -lChangerScore;
	}
	LOG_DEBUG("5 lScore:%lld,lCurPollScore:%lld,lDiffPollScore:%lld,uSysLostPro:%d,uSysWinPro:%d", lScore, m_tagJackpotScore.lCurPollScore, m_tagJackpotScore.lDiffPollScore, m_tagJackpotScore.uSysLostPro, m_tagJackpotScore.uSysWinPro);

}

void CGameRoom::ResetJackpotScore()
{
	//bool bNewDay = (diffTimeDay(m_tagJackpotScore.ulLastRestTime, uTime) != 0);
	//if (!bNewDay)
	//{
	//	return;
	//}

	uint64 uProcessTime = getSysTime();
	static tm	tm_date;
	memset(&tm_date, 0, sizeof(tm_date));
	getLocalTime(&tm_date, uProcessTime);

	if (m_tagJackpotScore.tm_hour != tm_date.tm_hour || m_tagJackpotScore.tm_min > tm_date.tm_min)
	{
		return;
	}

	string param = GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - param:%s", param.c_str());
		return;
	}

	int iIsUpdateCurPollScore = 0;
	if (jvalue.isMember("ucp")) {
		iIsUpdateCurPollScore = jvalue["ucp"].asInt();
	}
	//if (iIsUpdateCurPollScore == 1)
	//{
		if (jvalue.isMember("cps")) {
			int64 lCurPollScore = jvalue["cps"].asInt64();
			m_tagJackpotScore.SetCurPoolScore(lCurPollScore);
		}
	//}
	if (jvalue.isMember("swp")) {
		m_tagJackpotScore.uSysWinPro = jvalue["swp"].asInt();
	}
	if (jvalue.isMember("slp")) {
		m_tagJackpotScore.uSysLostPro = jvalue["slp"].asInt();
	}

	m_tagJackpotScore.ulLastRestTime = getSysTime();
	m_tagJackpotScore.tm_hour = g_RandGen.RandRange(1, 3);
	m_tagJackpotScore.tm_min = g_RandGen.RandRange(0, 59);

	LOG_ERROR("reader json parse success - roomid:%d,m_lMaxPollScore:%lld,m_lMinPollScore:%lld,iIsUpdateCurPollScore:%d,m_lCurPollScore:%lld,m_uSysWinPro:%d,uSysLostPro:%d,uSysLostWinProChange:%d,lUpdateJackpotScore:%lld,iUserJackpotControl:%d,ulLastRestTime:%lld,tm_hour:%d,tm_min:%d",
		GetRoomID(), m_tagJackpotScore.lMaxPollScore, m_tagJackpotScore.lMinPollScore, iIsUpdateCurPollScore, m_tagJackpotScore.lCurPollScore, m_tagJackpotScore.uSysWinPro, m_tagJackpotScore.uSysLostPro, m_tagJackpotScore.uSysLostWinProChange, m_tagJackpotScore.lUpdateJackpotScore, m_tagJackpotScore.iUserJackpotControl, m_tagJackpotScore.ulLastRestTime, m_tagJackpotScore.tm_hour, m_tagJackpotScore.tm_min);

}

void    CGameRoom::RetireAllTable()
{
    MAP_TABLE::iterator it = m_mpTables.begin();
    for (; it != m_mpTables.end(); ++it)
    {
        CGameTable* pTable = it->second;
        pTable->CheckRetireTableUser();
    }
}

// 修改房间库存配置 add by har
void CGameRoom::ChangeRoomStockCfg(stStockCfg &cfg) {
	if (cfg.stockConversionRate < 0 || cfg.stockConversionRate > PRO_DENO_10000 || cfg.jackpotMin < 1 ||
		cfg.jackpotMaxRate < 0 || cfg.jackpotMaxRate > PRO_DENO_10000 || cfg.jackpotRate < 0 || cfg.jackpotRate > PRO_DENO_10000 ||
		cfg.jackpotCoefficient < 1 || cfg.jackpotExtractRate < 0 || cfg.jackpotExtractRate > PRO_DENO_10000 ||
		cfg.playerWinRate < 0 || cfg.playerWinRate > PRO_DENO_10000) {
		LOG_ERROR("ChangeRoomStockCfg  param error  roomid:%d", GetRoomID());
		return;
	}
	m_roomStockCfg.stockMax = cfg.stockMax;
	m_roomStockCfg.stockConversionRate = cfg.stockConversionRate;
	m_roomStockCfg.jackpotMin = cfg.jackpotMin;
	m_roomStockCfg.jackpotMaxRate = cfg.jackpotMaxRate;
	m_roomStockCfg.jackpotRate = cfg.jackpotRate;
	m_roomStockCfg.jackpotCoefficient = cfg.jackpotCoefficient;
	m_roomStockCfg.jackpotExtractRate = cfg.jackpotExtractRate;
	m_roomStockCfg.killPointsLine = cfg.killPointsLine;
	m_roomStockCfg.playerWinRate = cfg.playerWinRate;
	int64 oldStock = m_roomStockCfg.stock;
	m_roomStockCfg.stock = oldStock + cfg.stock;
	int64 oldJackpot = m_roomStockCfg.jackpot;
	m_roomStockCfg.jackpot = oldJackpot + cfg.jackpot;
	LOG_DEBUG("ChangeRoomStockCfg  roomid:%d,add_stock:%lld,oldStock:%lld,stock:%lld,oldJackpot:%lld,jackpot:%lld,jackpotCoefficient:%d",
		GetRoomID(), cfg.stock, oldStock, m_roomStockCfg.stock, oldJackpot, m_roomStockCfg.jackpot, m_roomStockCfg.jackpotCoefficient);
}

// 是否触发库存变牌  add by har
int64 CGameRoom::IsStockChangeCard(CGameTable *pTable) {
	bool isAllRobotOrPlayerJetton = pTable->IsAllRobotOrPlayerJetton();
	if (isAllRobotOrPlayerJetton)
		return 0;

	uint32 tableId = pTable->GetTableID();
	int64 jackpotSub = labs(m_roomStockCfg.jackpot - m_roomStockCfg.jackpotMin);
	if (m_roomStockCfg.jackpot < m_roomStockCfg.killPointsLine) {
		if (m_roomStockCfg.jackpot < 0) {
			LOG_WARNING("IsStockChangeCard1 jackpot<0 roomid:%d,tableid:%d,jackpot:%lld,killPointsLine:%lld", GetRoomID(), tableId,
				m_roomStockCfg.jackpot, m_roomStockCfg.killPointsLine);
			return -1;
		}
		int64 oldPlayerWinRate = m_roomStockCfg.playerWinRate - PRO_DENO_10000 * jackpotSub / m_roomStockCfg.jackpotCoefficient;
		int64 playerWinRate = oldPlayerWinRate;
		if (playerWinRate < 0)
			playerWinRate = 0;
		else if (playerWinRate > PRO_DENO_10000)
			playerWinRate = PRO_DENO_10000;
		bool bRet = g_RandGen.RandRatio(playerWinRate, PRO_DENO_10000);
		LOG_DEBUG("IsStockChangeCard2 roomid:%d,tableid:%d,bRet:%d,playerWinRate:%lld,oldPlayerWinRate:%lld,jackpot:%lld,jackpotMin:%lld",
			GetRoomID(), tableId, bRet, playerWinRate, oldPlayerWinRate, m_roomStockCfg.jackpot, m_roomStockCfg.jackpotMin);
		if (bRet)
			return 0;
		return -1;
	}

	if (m_roomStockCfg.jackpot < m_roomStockCfg.jackpotMin)
		return 0;

	//int jackpotRate = m_roomStockCfg.jackpotRate + PRO_DENO_10000 * jackpotSub / m_roomStockCfg.jackpotCoefficient; //计算结果如果大于int的范围，则jackpotRate的值被赋予0！
	int64 jackpotRate = m_roomStockCfg.jackpotRate + PRO_DENO_10000 * jackpotSub / m_roomStockCfg.jackpotCoefficient;
	if (jackpotRate < 0)
		jackpotRate = 0;
	else if (jackpotRate > PRO_DENO_10000)
		jackpotRate = PRO_DENO_10000;
	bool bRet2 = g_RandGen.RandRatio(jackpotRate, PRO_DENO_10000);
	LOG_DEBUG("IsStockChangeCard3 roomid:%d,tableid:%d,bRet2:%d,jackpotRate:%lld,jackpotMaxRate:%d,jackpotSub:%lld",
		GetRoomID(), tableId, bRet2, jackpotRate, m_roomStockCfg.jackpotMaxRate, jackpotSub);
	if (bRet2)
		return 0.0001 * jackpotSub * m_roomStockCfg.jackpotMaxRate;

	return 0;
}

// 每局结束,更新库存信息 add by har
void CGameRoom::UpdateStock(CGameTable *pTable, int64 playerWinScore) {
	if (playerWinScore == 0)
		return;
	uint32 tableId = pTable->GetTableID();
	if (pTable->GetIsAllRobotOrPlayerJetton()) {
		LOG_DEBUG("CGameRoom::UpdateStock isAllRobotOrPlayerJetton roomid:%d,tableid:%d,playerWinScore:%lld", GetRoomID(), tableId, playerWinScore);
		return;
	}
	int64 addStock = 0;
	int64 addJackpot = 0;
	if (playerWinScore < 0) {
		int64 robotWinScore = -playerWinScore;
		addJackpot = 0.0001 * robotWinScore * m_roomStockCfg.jackpotExtractRate;
		addStock = robotWinScore - addJackpot;
	} else
		addJackpot = -playerWinScore;

	int64 addCan = 0;
	int64 canInventory = m_roomStockCfg.stock + addStock - m_roomStockCfg.stockMax;
	if (canInventory > 0) {
		addCan = 0.0001 * canInventory * m_roomStockCfg.stockConversionRate;
		addJackpot += addCan;
		addStock -= addCan;
	}

	if (addStock == 0 && addJackpot == 0)
		return;

	m_roomStockCfg.stock += addStock;
	m_roomStockCfg.jackpot += addJackpot;
	LOG_DEBUG("CGameRoom::UpdateStock2  roomid:%d,tableid:%d,playerWinScore:%lld,addStock:%lld,addJackpot:%lld,stock:%lld,jackpot:%lld,canInventory:%lld,addCan:%lld,pTable:%p",
		GetRoomID(), tableId, playerWinScore, addStock, addJackpot, m_roomStockCfg.stock, m_roomStockCfg.jackpot, canInventory, addCan, pTable);
	CDBMysqlMgr::Instance().UpdateStock(GetGameType(), GetRoomID(), addStock, addJackpot);
}