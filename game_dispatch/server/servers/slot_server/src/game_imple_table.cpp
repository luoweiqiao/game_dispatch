//
// Created by toney on 16/4/6.
//
#include <data_cfg_mgr.h>
#include <center_log.h>
#include "game_imple_table.h"
#include "stdafx.h"
#include "game_room.h"
#include "json/json.h"
#include "robot_mgr.h"

//using namespace std;
using namespace svrlib;
using namespace game_slot;
using namespace net;

namespace
{
    const static uint32  s_FreeTimes[MAXCOLUMN]   = {0,0,3,7,12};     // BONUS奖励免费次数  
	const static uint8   s_MaxLines               = 9; //最大线数
	const static uint32  s_MakeWheelsTime         = 30 * 60 * 1000; // 空闲时间
};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // 水果机
        {
            pTable = new CGameSlotTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // 比赛水果机
        {
            pTable = new CGameSlotTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // 私人水果机
        {
            pTable = new CGameSlotTable(this,tableID,emTABLE_TYPE_PLAYER);
        }break;
    default:
        {
            assert(false);
            return NULL;
        }break;
    }
    return pTable;
}
// 水果机桌子
CGameSlotTable::CGameSlotTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();

	m_bBroadcast = true;                             //游戏是否接收广播
	m_lBroadcastWinScore = 0;                     //发送广播需要赢金币数量
}

CGameSlotTable::~CGameSlotTable()
{
        
}
bool    CGameSlotTable::CanEnterTable(CGamePlayer* pPlayer)
{
    //if(pPlayer->GetTable() != NULL)
    //    return false;
    // 限额进入
    if(IsFullTable() || GetPlayerCurScore(pPlayer) < GetEnterMin()){
        return false;
    }
    return true;
}
bool    CGameSlotTable::CanLeaveTable(CGamePlayer* pPlayer)
{       
    return true;
}

bool CGameSlotTable::IsFullTable()
{
	if (m_mpLookers.size() >= 500)
		return true;

	return false;
}

void CGameSlotTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::slot_table_info* pslot = pInfo->mutable_slot();
	pslot->set_tableid(GetTableID());
	pslot->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
		pslot->set_is_passwd(1);
    }else{
		pslot->set_is_passwd(0);
    }
	pslot->set_hostname(m_conf.hostName);
	pslot->set_basescore(m_conf.baseScore);
	pslot->set_consume(m_conf.consume);
	pslot->set_entermin(m_conf.enterMin);
	pslot->set_duetime(m_conf.dueTime);
	pslot->set_feetype(m_conf.feeType);
	pslot->set_feevalue(m_conf.feeValue);
	pslot->set_seat_num(m_conf.seatNum);
	pslot->set_jackpot_score(m_lJackpotScore);//彩金
}

//template<typename ... Ts>
//auto fold_expression_sum(Ts ... ts)
//{
//	return (ts + ...);
//}

//配置桌子
bool CGameSlotTable::Init()
{
    SetGameState(net::TABLE_STATE_FREE);
	m_vecPlayers.resize(4);
	for (uint8 i = 0; i < 4; ++i)
	{
		m_vecPlayers[i].Reset();
	}
	m_bSaveRoomParam = false;
	InitSlotParam();
	ReAnalysisParam();

	SaveRoomParam(0);
	m_coolRobot.beginCooling(20 * 1000); //20 秒

	m_iArrMasterRandProCount[0] = 50;
	m_iArrMasterRandProCount[1] = 30;
	m_iArrMasterRandProCount[2] = 10;
	m_iArrMasterRandProCount[3] = 5;
	m_iArrMasterRandProCount[4] = 2;
	m_iArrMasterRandProCount[5] = 2;
	m_iArrMasterRandProCount[6] = 1;

	//std::vector<int> vecTest{ 1, 2, 3 };
	//for (const auto v : vecTest)
	//{
	//	LOG_DEBUG("v:%d", v);
	//}
	//int iSum{ fold_expression_sum(1, 2, 3) };
	//LOG_DEBUG("iSum:%d", iSum);
    return true;
}


//测试函数 打印当前
void CGameSlotTable::display()
{
	//打印
	vector<LinePosConfig>::iterator iter = m_oServerLinePos.begin();
	for (; iter != m_oServerLinePos.end(); iter++)
	{
		LOG_DEBUG("m_oServerLinePos--%d,--%d,--%d--%d--%d", iter->pos[0], iter->pos[1], iter->pos[2], iter->pos[3], iter->pos[4]);
	}

	//打印
	vector<PicRateConfig>::iterator iter1 = m_oPicRateConfig.begin();
	for (; iter1 != m_oPicRateConfig.end(); iter1++)
	{
		LOG_DEBUG("m_oPicRateConfig--%d,--%d,--%d--%d--%d", iter1->multiple[0], iter1->multiple[1], iter1->multiple[2], iter1->multiple[3], iter1->multiple[4]);
	}

	//打印
	LOG_DEBUG("m_JackpotBetRange=%lld,--%d,m_JackpotRangeScore=%d--%d", m_JackpotBetRange[0], m_JackpotBetRange[1], m_JackpotRangeScore[0], m_JackpotRangeScore[1]);
}

// 获取单个下注的是机器人还是玩家  add by har
void CGameSlotTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	LOG_DEBUG("IsRobotOrPlayerJetton  roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), pPlayer->GetUID());
	isAllPlayer = false;
	isAllRobot = false;
}

void CGameSlotTable::InitSlotParam()
{
	//初始化配置参数
	//线对应的点
	m_lFeeScore = 0;
	m_oServerLinePos.clear();
	m_oPicRateConfig.clear();
	m_oSysRepertoryRange.clear();
	m_VecJackpotWeigh.clear();
	//初始化
	m_JackpotBetRange[0] = 1000;
	m_JackpotBetRange[1] = 50000;
	m_JackpotRangeScore[0] = 25000000;
	m_JackpotRangeScore[1] = 250000000;
	uint32	tmpos1[MAXCOLUMN] = { 1,4,7,10,13 };
	uint32	tmpos2[MAXCOLUMN] = { 2,5,8,11,14 };
	uint32	tmpos3[MAXCOLUMN] = { 3,6,9,12,15 };
	uint32	tmpos4[MAXCOLUMN] = { 1,5,9,11,13 };
	uint32	tmpos5[MAXCOLUMN] = { 3,5,7,11,15 };
	uint32	tmpos6[MAXCOLUMN] = { 1,4,8,12,15 };
	uint32	tmpos7[MAXCOLUMN] = { 3,6,8,10,13 };
	uint32	tmpos8[MAXCOLUMN] = { 2,6,8,10,14 };
	uint32	tmpos9[MAXCOLUMN] = { 2,4,8,12,14 };
	LinePosConfig pcfg;
	pcfg.lineID = 0;
	memcpy(pcfg.pos, tmpos1, 20);
	m_oServerLinePos.push_back(pcfg);
	pcfg.lineID = 1;
	memcpy(pcfg.pos, tmpos2, 20);
	m_oServerLinePos.push_back(pcfg);
	pcfg.lineID = 2;
	memcpy(pcfg.pos, tmpos3, 20);
	m_oServerLinePos.push_back(pcfg);
	pcfg.lineID = 3;
	memcpy(pcfg.pos, tmpos4, 20);
	m_oServerLinePos.push_back(pcfg);
	pcfg.lineID = 4;
	memcpy(pcfg.pos, tmpos5, 20);
	m_oServerLinePos.push_back(pcfg);
	pcfg.lineID = 5;
	memcpy(pcfg.pos, tmpos6, 20);
	m_oServerLinePos.push_back(pcfg);
	pcfg.lineID = 6;
	memcpy(pcfg.pos, tmpos7, 20);
	m_oServerLinePos.push_back(pcfg);
	pcfg.lineID = 7;
	memcpy(pcfg.pos, tmpos8, 20);
	m_oServerLinePos.push_back(pcfg);
	pcfg.lineID = 8;
	memcpy(pcfg.pos, tmpos9, 20);
	m_oServerLinePos.push_back(pcfg);

	//打印
	vector<LinePosConfig>::iterator iter = m_oServerLinePos.begin();
	for (; iter != m_oServerLinePos.end(); iter++)
	{
		LOG_DEBUG("m_oServerLinePos--%d,--%d,--%d--%d--%d", iter->pos[0], iter->pos[1], iter->pos[2], iter->pos[3], iter->pos[4]);
	}
	//图片对应的赔率
	int32	tmpmultiple1[MAXCOLUMN] = { 0,1,3,10,75 };
	int32	tmpmultiple2[MAXCOLUMN] = { 0,0,3,10,85 };
	int32	tmpmultiple3[MAXCOLUMN] = { 0,0,15,40,250 };
	int32	tmpmultiple4[MAXCOLUMN] = { 0,0,25,50,400 };
	int32	tmpmultiple5[MAXCOLUMN] = { 0,0,30,70,550 };
	int32	tmpmultiple6[MAXCOLUMN] = { 0,0,35,80,650 };
	int32	tmpmultiple7[MAXCOLUMN] = { 0,0,45,100,800 };
	int32	tmpmultiple8[MAXCOLUMN] = { 0,0,75,175,1250 };
	int32	tmpmultiple9[MAXCOLUMN] = { 0,0,0,0,0 };
	int32	tmpmultiple10[MAXCOLUMN] = { 0,0,3,7,12 };
	int32	tmpmultiple11[MAXCOLUMN] = { 0,0,100,200,1750 };

	PicRateConfig    rcfg;
	rcfg.picId = 1;
	memcpy(rcfg.multiple, tmpmultiple1, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 2;
	memcpy(rcfg.multiple, tmpmultiple2, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 3;
	memcpy(rcfg.multiple, tmpmultiple3, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 4;
	memcpy(rcfg.multiple, tmpmultiple4, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 5;
	memcpy(rcfg.multiple, tmpmultiple5, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 6;
	memcpy(rcfg.multiple, tmpmultiple6, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 7;
	memcpy(rcfg.multiple, tmpmultiple7, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 8;
	memcpy(rcfg.multiple, tmpmultiple8, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 9;
	memcpy(rcfg.multiple, tmpmultiple9, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 10;
	memcpy(rcfg.multiple, tmpmultiple10, 20);
	m_oPicRateConfig.push_back(rcfg);
	rcfg.picId = 11;
	memcpy(rcfg.multiple, tmpmultiple11, 20);
	m_oPicRateConfig.push_back(rcfg);
	//打印
	vector<PicRateConfig>::iterator iter1 = m_oPicRateConfig.begin();
	for (; iter1 != m_oPicRateConfig.end(); iter1++)
	{
		LOG_DEBUG("m_oPicRateConfig--%d,--%d,--%d--%d--%d", iter1->multiple[0], iter1->multiple[1], iter1->multiple[2], iter1->multiple[3], iter1->multiple[4]);
	}
	int32 ReturnUserRate[2] = { -2000,2000 };  //个人返奖比例
	int32 ReturnSysRate[5] = { -1000,-500,0,500,1000 };  // 系统返奖比例
	uint32  omFreeTimes[MAXCOLUMN] = { 0,0,3,7,12 };
	memcpy(m_ReUserRate, ReturnUserRate, 2 * 2);  //个人返奖比例
	memcpy(m_ReSysRate, ReturnSysRate, 5 * 4);// 系统返奖比例
	m_lSysScore = 30000000; //系统库存
	m_lJackpotScore = 2000000;//彩金池子
	m_FeeRate = 500;         //抽水比例
	m_JackpotRate = 500;        //彩金比例
	m_oSysRepertoryRange.push_back(200000000);                        //
	m_oSysRepertoryRange.push_back(300000000);                        //系统库存区间配置
	m_oSysRepertoryRange.push_back(400000000);                        //系统库存区间配置
	m_oSysRepertoryRange.push_back(500000000);                        //系统库存区间配置

	m_gameLogic.SetSlotFeeRate(m_FeeRate);    // 抽水比例
	m_gameLogic.SetSlotJackpotRate(m_JackpotRate);
	m_gameLogic.SetBonusFreeTimes(omFreeTimes);
	m_gameLogic.SetReturnUserRate(ReturnUserRate);
	m_gameLogic.SetReturnSysRate(ReturnSysRate);

	m_gameLogic.SetServerLinePos(m_oServerLinePos);
	m_gameLogic.SetServerLinePos(m_oPicRateConfig);

	//=====================测试代码==================================
	uint32 tmpPicCfg[MAXCOLUMN][MAXWEIGHT] = {
		{ 10,43,88,58,72,38,28,25,10,10,0 },
		{ 10,43,88,58,72,38,28,25,10,10,0 },
		{ 10,43,88,58,72,38,28,25,10,11,0 },
		{ 10,43,88,58,72,38,28,25,10,12,0 },
		{ 10,43,88,58,72,38,28,25,10,13,0 }
	};
	uint32 mWeightPicCfg[55] = { 0 };
	memcpy(mWeightPicCfg, tmpPicCfg, sizeof(tmpPicCfg));
	m_gameLogic.SetWeightPicCfg(mWeightPicCfg);

	//彩金配置初始化
	m_nJackpotChangeRate = 10;                        //彩金库存变化率(万分比)
	m_nJackpotChangeUnit = 10000000;                  //彩金库存变化单位
	m_MinJackpotReviseRate = -500;                    //彩金库存最小修正率(万分比)
	m_MaxJackpotReviseRate = 500;                     //彩金库存最大修正率(万分比)
	m_VecJackpotWeigh.push_back(100);                   //彩金权重
	m_VecJackpotWeigh.push_back(50);                    //彩金权重
	m_VecJackpotWeigh.push_back(10);                    //彩金权重
	m_VecJackpotRatio.push_back(1000);                    //彩金比例
	m_VecJackpotRatio.push_back(2500);                    //彩金比例
	m_VecJackpotRatio.push_back(5000);                    //彩金比例
	m_lWinHandselGameCount = 0;
	m_uWinHandselPre = 0;

	m_vecUserJettonCellScore.clear();
}

bool CGameSlotTable::ReAnalysisParam() 
{
	//LOG_DEBUG("enter jCGameSlotTable::ReAnalysisParam");
	string param = m_pHostRoom->GetCfgParam();

	LOG_DEBUG("enter jCGameSlotTable::ReAnalysisParam - roomid:%d,tableid:%d,param :%s",GetRoomID(),GetTableID(), param.c_str());

	Json::Reader reader;
	Json::Value  jvalue;
	if (param.empty() || !reader.parse(param, jvalue))
	{
		LOG_ERROR("解析游戏参数json错误:%s", param.c_str());
		return true;
	}
	//清空数据
	//m_oServerLinePos.clear();
	//m_oPicRateConfig.clear();
	//m_oSysRepertoryRange.clear();
	//m_VecJackpotWeigh.clear();
	// && jvalue[""].isIntegral()
	if (jvalue.isMember("broadwincoin_num"))
	{
		m_lBroadcastWinScore = jvalue["broadwincoin_num"].asInt64();
	}
	if (jvalue.isMember("broadwincoin_switch"))
	{
		m_bBroadcast = (jvalue["broadwincoin_switch"].asInt() == 0 ? false : true);
	}
	//彩金比例
	if (jvalue.isMember("handselin"))
	{
		m_JackpotRate = jvalue["handselin"].asInt();
		m_gameLogic.SetSlotJackpotRate(m_JackpotRate);
	}
	//抽水比例
	if (jvalue.isMember("pump"))
	{
		m_FeeRate = jvalue["pump"].asInt();
		m_gameLogic.SetSlotFeeRate(m_FeeRate);    // 抽水比例
	}
	//下注额度
	if (jvalue.isMember("betquota"))
	{
		m_vecUserJettonCellScore.clear();

		string betquotaStr = jvalue["betquota"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (betquotaStr.empty() || !tmpreader.parse(betquotaStr, tmpjvalue))
		{
			LOG_ERROR("analysis betquota json error - betquota:%s", betquotaStr.c_str());
			return false;
		}
		if (tmpjvalue.size() ==0)
		{
			LOG_ERROR("analysis betquota json error - betquota:%s,size:%d", betquotaStr.c_str(), tmpjvalue.size());
			return false;
		}
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			m_vecUserJettonCellScore.push_back(tmpjvalue[i].asInt());
		}		
		for (uint32 i = 0; i < m_vecUserJettonCellScore.size(); i++)
		{
			LOG_ERROR("analysis betquota json success - betquota:%s,size:%d,cell:%d", betquotaStr.c_str(), m_vecUserJettonCellScore.size(),m_vecUserJettonCellScore[i]);
		}
	}
	//公共返奖调节
	if (jvalue.isMember("commonreturnaward"))
	{		
		string sysrepertoryrangeStr = jvalue["commonreturnaward"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (sysrepertoryrangeStr.empty() || !tmpreader.parse(sysrepertoryrangeStr, tmpjvalue))
		{
			LOG_ERROR("analysis commonreturnaward json error - commonreturnaward:%s", sysrepertoryrangeStr.c_str());
			return false;
		}
		if (tmpjvalue.size() < 5)
		{
			LOG_ERROR("analysis commonreturnaward json error - commonreturnaward:%s,size()=%d", sysrepertoryrangeStr.c_str(), tmpjvalue.size());
			return false;
		}
		int32 ReturnSysRate[5] = { 0 };  // 系统返奖比例
		memset(&ReturnSysRate, 0, sizeof(ReturnSysRate));
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			ReturnSysRate[i] = tmpjvalue[i].asInt();
		}
		m_gameLogic.SetReturnSysRate(ReturnSysRate);
	}
	//公共库存
	if (jvalue.isMember("commonrepertory"))
	{
		m_oSysRepertoryRange.clear();
		string commonrepertoryStr = jvalue["commonrepertory"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (commonrepertoryStr.empty() || !tmpreader.parse(commonrepertoryStr, tmpjvalue))
		{
			LOG_ERROR("analysis commonrepertory json error - commonrepertory:%s", commonrepertoryStr.c_str());
			return false;
		}
		if (tmpjvalue.size() < 4)
		{
			LOG_ERROR("analysis commonrepertory json error - commonrepertory:%s,size()=%d", commonrepertoryStr.c_str(),tmpjvalue.size());
			return false;
		}		
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			m_oSysRepertoryRange.push_back(tmpjvalue[i].asInt64());
		}
	}
	//个人返奖调节
	if (jvalue.isMember("personalreturnaward"))
	{
		string personalreturnawardStr = jvalue["personalreturnaward"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (personalreturnawardStr.empty() || !tmpreader.parse(personalreturnawardStr, tmpjvalue))
		{
			LOG_ERROR("analysis personalreturnaward json error - personalreturnaward:%s", personalreturnawardStr.c_str());
			return false;
		}
		if (tmpjvalue.size() > 2)
		{
			LOG_ERROR("analysis personalreturnaward json error - personalreturnaward:%s,size()=%d", personalreturnawardStr.c_str(), tmpjvalue.size());
			return false;
		}
		int32 ReturnUserRate[2] = { 0 };  //个人返奖比例
		memset(&ReturnUserRate, 0, sizeof(ReturnUserRate));
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			ReturnUserRate[i] = tmpjvalue[i].asInt();
		}
		m_gameLogic.SetReturnUserRate(ReturnUserRate);
	}

	if (jvalue.isMember("line"))
	{
		m_oServerLinePos.clear();
		string imgweightStr = jvalue["line"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (imgweightStr.empty() || !tmpreader.parse(imgweightStr, tmpjvalue))
		{
			LOG_ERROR("analysis line json error - line:%s", imgweightStr.c_str());
			return false;
		}
		if (tmpjvalue.size() != MAX_LINE)
		{
			LOG_ERROR("analysis line json error - line:%s,size()=%d", imgweightStr.c_str(), tmpjvalue.size());
			return false;
		}
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			Json::Value v = tmpjvalue[i];
			if (v.size() == MAXCOLUMN)
			{
				LinePosConfig pcfg;
				pcfg.lineID = i;
				for (uint32 k = 0; k < v.size(); ++k)
				{
					pcfg.pos[k] = v[k].asInt();
				}
				m_oServerLinePos.push_back(pcfg);
			}
		}
		//打印
		LOG_DEBUG("m_oServerLinePos--size%d,tmpjvalue=%d", m_oServerLinePos.size(), tmpjvalue.size());
		m_gameLogic.SetServerLinePos(m_oServerLinePos);
	}

	//bonus奖励免费转动次数
	if (jvalue.isMember("bonusfreetime"))
	{
		string bonusfreetimeStr = jvalue["bonusfreetime"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (bonusfreetimeStr.empty() || !tmpreader.parse(bonusfreetimeStr, tmpjvalue))
		{
			LOG_ERROR("analysis bonusfreetime json error - bonusfreetime:%s", bonusfreetimeStr.c_str());
			return false;
		}
		if (tmpjvalue.size() != MAXCOLUMN)
		{
			LOG_ERROR("analysis bonusfreetime json error - bonusfreetime:%s,size()=%d", bonusfreetimeStr.c_str(), tmpjvalue.size());
			return false;
		}
		uint32 omFreeTimes[MAXCOLUMN] = { 0 };  //个人返奖比例
		memset(&omFreeTimes, 0, sizeof(omFreeTimes));
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			omFreeTimes[i] = tmpjvalue[i].asInt();
		}
		m_gameLogic.SetBonusFreeTimes(omFreeTimes);
	}
	//彩金比例
	if (jvalue.isMember("handselweight") )
	{
		m_VecJackpotWeigh.clear();
		string handseloutStr = jvalue["handselweight"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (handseloutStr.empty() || !tmpreader.parse(handseloutStr, tmpjvalue))
		{
			LOG_ERROR("analysis handselweight json error - handselweight:%s", handseloutStr.c_str());
			return false;
		}
		if (tmpjvalue.size() < 3)
		{
			LOG_ERROR("analysis handselweight json error - handselweight:%s,size()=%d", handseloutStr.c_str(), tmpjvalue.size());
			return false;
		}
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			m_VecJackpotWeigh.push_back(tmpjvalue[i].asInt());
		}
	}
	//彩金比例
	if (jvalue.isMember("handselout") )
	{
		m_VecJackpotRatio.clear();
		string handselweightStr = jvalue["handselout"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (handselweightStr.empty() || !tmpreader.parse(handselweightStr, tmpjvalue))
		{
			LOG_ERROR("analysis handselout json error - handselout:%s", handselweightStr.c_str());
			return false;
		}
		if (tmpjvalue.size() < 3)
		{
			LOG_ERROR("analysis handselout json error - handselout:%s,size()=%d", handselweightStr.c_str(), tmpjvalue.size());
			return false;
		}
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			m_VecJackpotRatio.push_back(tmpjvalue[i].asInt());
		}
	}
	//各个图案的权重
	if (jvalue.isMember("imgweight") )
	{
		string imgweightStr = jvalue["imgweight"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (imgweightStr.empty() || !tmpreader.parse(imgweightStr, tmpjvalue))
		{
			LOG_ERROR("analysis sysweightStr json error - imgweight:%s", imgweightStr.c_str());
			return false;
		}
		if (tmpjvalue.size() != MAXCOLUMN )
		{
			LOG_ERROR("analysis imgweightStr json error - imgweightStr:%s,size()=%d", imgweightStr.c_str(), tmpjvalue.size());
			return false;
		}
		uint32 nlen = 0;
		uint32 mWeightPicCfg[55] = { 0 };
		memset(&mWeightPicCfg, 0, sizeof(mWeightPicCfg));
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			Json::Value v = tmpjvalue[i];		
			if (v.size() == 11)
			{
				for (uint32 k = 0; k < v.size(); ++k)
				{
					mWeightPicCfg[nlen++] = v[k].asInt();
				}
			}		
		}
		m_gameLogic.SetWeightPicCfg(mWeightPicCfg);
	}
	//水果倍数
	if (jvalue.isMember("fruitmul"))
	{
		m_oPicRateConfig.clear();
		string imgweightStr  = jvalue["fruitmul"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (imgweightStr.empty() || !tmpreader.parse(imgweightStr, tmpjvalue))
		{
			LOG_ERROR("analysis fruitmul json error - fruitmul:%s", imgweightStr.c_str());
			return false;
		}
		if (tmpjvalue.size() != PIC_SEVEN)
		{
			LOG_ERROR("analysis fruitmul json error - fruitmul:%s,size()=%d", imgweightStr.c_str(), tmpjvalue.size());
			return false;
		}
		for (uint32 i = 0; i < tmpjvalue.size(); ++i)
		{
			Json::Value v = tmpjvalue[i];
			if (v.size() == MAXCOLUMN)
			{			
				PicRateConfig pcfg;
				pcfg.picId = i;
				for (uint32 k = 0; k < v.size(); ++k)
				{
					pcfg.multiple[k] = v[k].asInt();
				}
				m_oPicRateConfig.push_back(pcfg);
			}
		}
		//打印
		m_gameLogic.SetServerLinePos(m_oPicRateConfig);
		LOG_DEBUG("m_oPicRateConfig--size=%d,tmpjvalue=%d", m_oPicRateConfig.size(), tmpjvalue.size());
	}

	//彩金库存变化率
	if (jvalue.isMember("handselchangerate") )
	{
		m_nJackpotChangeRate = jvalue["handselchangerate"].asInt();   //彩金库存变化率(万分比)
	}
	//彩金库存变化单位
	if (jvalue.isMember("handselchangeunit") )
	{
		m_nJackpotChangeUnit = jvalue["handselchangeunit"].asInt();    //彩金库存变化单位
	}
	//彩金最低修正率
	if (jvalue.isMember("handseladjratemin") )
	{
		m_MinJackpotReviseRate = jvalue["handseladjratemin"].asInt();  //彩金库存最小修正率(万分比)
	}
	//彩金最高修正率
	if (jvalue.isMember("handseladjratemax"))
	{
		m_MaxJackpotReviseRate = jvalue["handseladjratemax"].asInt();//彩金库存最大修正率(万分比)
	}
	//下注获取彩金界限
	if (jvalue.isMember("handselbetlimit") )
	{
		string imgweightStr = jvalue["handselbetlimit"].asString();
		Json::Reader tmpreader;
		Json::Value  tmpjvalue;
		if (imgweightStr.empty() || !tmpreader.parse(imgweightStr, tmpjvalue))
		{
			LOG_ERROR("analysis handselbetlimit json error - handselbetlimit:%s", imgweightStr.c_str());
			return false;
		}
		if (tmpjvalue.size() != 2)
		{
			LOG_ERROR("analysis handselbetlimit json error - handselbetlimit:%s,size()=%d", imgweightStr.c_str(), tmpjvalue.size());
			return false;
		}
		if (tmpjvalue.size()>0)
		{
			for (uint32 i = 0; i < tmpjvalue.size(); ++i)
			{
				Json::Value v = tmpjvalue[i];
				if (i == 0)
				{
					m_JackpotBetRange[0] = v[0].asInt64();
					m_JackpotBetRange[1] = v[1].asInt64();

				}
				else
				{
					m_JackpotRangeScore[0] = v[0].asInt64();
					m_JackpotRangeScore[1] = v[1].asInt64();
				}
			}
		}
	
		LOG_DEBUG("m_JackpotRangeScore=%lld,%lld,m_JackpotBetRange=%lld,%lld", m_JackpotRangeScore[0], m_JackpotRangeScore[1], m_JackpotBetRange[0], m_JackpotBetRange[1]);
	}
	//系统库存变化
	if (jvalue.isMember("commonNumber"))
	{
		m_lSysScore += jvalue["commonNumber"].asInt64();//系统库存变化
	}
	//彩金库存变化
	if (jvalue.isMember("handselNumber"))
	{
		m_lJackpotScore += jvalue["handselNumber"].asInt64();//彩金库存变化
	}

	if (jvalue.isMember("IsInitCommonNumber"))
	{
		if (jvalue["IsInitCommonNumber"].asInt64() == 1)
		{
			if (jvalue.isMember("InitCommonNumber"))
			{
				m_lSysScore = jvalue["InitCommonNumber"].asInt64();//系统库存初始
			}
		}
	}

	if (jvalue.isMember("IsInitHandselNumber"))
	{
		if (jvalue["IsInitHandselNumber"].asInt64() == 1)
		{
			if (jvalue.isMember("InitHandselNumber"))
			{
				m_lJackpotScore = jvalue["InitHandselNumber"].asInt64();//彩金库存初始
			}
		}
	}


	if (jvalue.isMember("IsSaveCommonNumber"))
	{
		if (jvalue["IsSaveCommonNumber"].asInt64() == 1)
		{
			if (jvalue.isMember("SaveCommonNumber"))
			{
				m_lSysScore = jvalue["SaveCommonNumber"].asInt64();//系统库存初始
			}
		}
	}

	if (jvalue.isMember("IsSaveHandselNumber"))
	{
		if (jvalue["IsSaveHandselNumber"].asInt64() == 1)
		{
			if (jvalue.isMember("SaveHandselNumber"))
			{
				m_lJackpotScore = jvalue["SaveHandselNumber"].asInt64();//彩金库存初始
			}
		}
	}


	LOG_DEBUG("m_lSysScore=%lld,m_lJackpotScore=%lld", m_lSysScore, m_lJackpotScore);

	if (jvalue.isMember("winHandselGameCount") )
	{
		m_lWinHandselGameCount = jvalue["winHandselGameCount"].asInt64();//获取彩金游戏局数

		LOG_DEBUG("success m_lWinHandselGameCount:%lld", m_lWinHandselGameCount);

	}
	else
	{
		LOG_DEBUG("failed m_lWinHandselGameCount:%lld", m_lWinHandselGameCount);

	}
	if (jvalue.isMember("winHandselPre") )
	{
		m_uWinHandselPre = jvalue["winHandselPre"].asInt64();//获取彩金游戏局数
	}
	


	//个人库存变化
	//if(jvalue.isMember("personNumber"))
	//{
	//	if(jvalue.isMember("personUid"))
	//	{
	//		int64 personNumber = jvalue["personNumber"].asInt64();
	//		uint32 uid         = jvalue["personUid"].asInt();
	//		CGamePlayer* pPlayer = (CGamePlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	//		if (pPlayer != NULL)
	//		{
	//			pPlayer->AsyncChangeStockScore(personNumber);
	//		}
	//	}
	//}
	m_gameLogic.MakeWheels();
	display();//打印日志
	return true;
}

void CGameSlotTable::ShutDown()
{

}
//复位桌子
void CGameSlotTable::ResetTable()
{
    ResetGameData();
    SetGameState(TABLE_STATE_FREE);
   // ResetPlayerReady();
    
}

void CGameSlotTable::OnTimeTick()
{
	OnTableTick();

    if(m_coolLogic.isTimeOut())
    {
		m_coolLogic.beginCooling(s_MakeWheelsTime);
		m_gameLogic.MakeWheels();
		m_lFeeScore = 0;//重置抽水库存
		//m_coolRobot.beginCooling(g_RandGen.RandRange(5000, 10000));
        uint8 tableState = GetGameState();
        switch(tableState)
        {
        case TABLE_STATE_FREE:
            {

            }break;
        case TABLE_STATE_PLAY:
            {

            }break;
        case TABLE_STATE_WAIT:
            {

            }break;
        default:
            break;
        }
    }
	
	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		SaveRoomParam(1);
	}

	//机器人代码测试
	if (m_coolRobot.isTimeOut())
	{
		OnRobotOper();
		//m_coolRobot.beginCooling(g_RandGen.RandRange(10 * 1000, 20 * 1000));
		m_coolRobot.beginCooling(60 * 1000);
	}
}

void CGameSlotTable::SaveRoomParam(int flag)
{
	if (m_bSaveRoomParam)
	{
		return;
	}
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;

	LOG_DEBUG("1 stopserver save status - m_lJackpotScore:%d,param:%s", m_lJackpotScore, param.c_str());

	if (param.empty() || !reader.parse(param, jvalue))
	{
		LOG_ERROR("json analysis error - param:%s", param.c_str());
		return;
	}
	jvalue["IsSaveCommonNumber"] = flag;
	jvalue["SaveCommonNumber"] = m_lSysScore;
	jvalue["IsSaveHandselNumber"] = flag;
	jvalue["SaveHandselNumber"] = m_lJackpotScore;

	string param2 = jvalue.toFastString();
	LOG_DEBUG("2 stopserver save status - m_lJackpotScore:%d,param:%s", m_lJackpotScore, param2.c_str());
	CDBMysqlMgr::Instance().UpdateGameRoomParam(param2, GetGameType(), GetRoomID());
	m_bSaveRoomParam = true;
}
// 游戏消息
int CGameSlotTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
	if (CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR)
	{
		LOG_DEBUG("server is repair");
		return 0;
	}
	if (pPlayer == NULL)
	{
		return 0;
	}
	uint32 uid = pPlayer->GetUID();
    uint16 chairID = GetChairID(pPlayer);
   // LOG_DEBUG("收到玩家消息 - uid:%d, %d--%d",uid,chairID,cmdID);
    switch(cmdID)
    {
    case net::C2S_MSG_SLOT_SPIN_REQ:// 开始转
        {
		    net::msg_slot_game_spin_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
			//InitBlingLog(pPlayer);
			return OnGameUserSpin(pPlayer,msg.linenum(), msg.betperline());
        }break;
    case net::C2S_MSG_SLOT_FREE_SPIN_REQ:// 免费转
        {
			//查找用户押注数据
			map<uint32, FruitUserData>::iterator iter = m_oFruitUserData.find(pPlayer->GetUID());
			if (iter != m_oFruitUserData.end())
			{
				if (iter->second.FreeTimes > 0)
				{				
					return OnGameUserFreeSpin(pPlayer);
				}
				else
				{
					iter->second.FreeTimes = 0; //当前玩家免费的次数
					iter->second.Free_winscore = 0; //当前玩家免费的次数
				}
			}
			return 0;
		}break;

	case net::C2S_MSG_SLOT_SEND_MASTER_CTRL_INFO_REQ:
		{
			bool bIsMaster = GetIsMasterUser(pPlayer);

			net::msg_send_slot_master_ctrl_info_req msg;
			PARSE_MSG_FROM_ARRAY(msg);
			net::msg_send_slot_master_ctrl_info_rep rep;
			rep.set_result(net::RESULT_CODE_FAIL);


			uint32 oper_type = msg.oper_type();
			net::msg_send_slot_master_ctrl_info ctrl_info = msg.info();

			tagSlotMasterCtrlInfo tagInfo;
			tagInfo.suid = uid;
			tagInfo.uid = ctrl_info.uid();
			tagInfo.lMinMultiple = ctrl_info.min_multiple();
			tagInfo.lMaxMultiple = ctrl_info.max_multiple();

			tagInfo.lRanMinMultiple = ctrl_info.ran_min_multiple();
			tagInfo.lRanMaxMultiple = ctrl_info.ran_max_multiple();

			tagInfo.iAllLostCount = ctrl_info.all_lost_count();
			tagInfo.iLostCount = tagInfo.iAllLostCount;

			tagInfo.iAllWinCount = ctrl_info.all_win_count();
			tagInfo.iWinCount = tagInfo.iAllWinCount;

			tagInfo.iFreeSpin = ctrl_info.free_spin();
			tagInfo.jackpotIndex = ctrl_info.jackpot_index();

			tagInfo.iAllPreCount = ctrl_info.all_pre_count();
			tagInfo.iPreCount = tagInfo.iAllPreCount;

			string strArrRandPro;
			if (oper_type == 2 && msg.rand_pro_size() == iMaxMasterRandProCount)
			{
				for (int i = 0; i < iMaxMasterRandProCount; i++)
				{
					m_iArrMasterRandProCount[i] = msg.rand_pro(i);
					strArrRandPro += CStringUtility::FormatToString("i:%d,p:%d ", i, m_iArrMasterRandProCount[i]);
				}
				rep.set_result(net::RESULT_CODE_SUCCESS);
			}
			int iResultCode = 0;
			if (bIsMaster && oper_type != 2)
			{
				auto iter_find = m_mpMasterCtrlInfo.find(tagInfo.uid);
				if (iter_find != m_mpMasterCtrlInfo.end())
				{
					iResultCode = 1;
					if (oper_type == 0)
					{
						m_mpMasterCtrlInfo.erase(iter_find);
						rep.set_result(net::RESULT_CODE_SUCCESS);
					}
					else if (oper_type == 1)
					{
						tagSlotMasterCtrlInfo & tempInfo = iter_find->second;
						tempInfo.suid = tagInfo.suid;
						tempInfo.lMinMultiple = tagInfo.lMinMultiple;
						tempInfo.lMaxMultiple = tagInfo.lMaxMultiple;
						tempInfo.lRanMinMultiple = tagInfo.lRanMinMultiple;
						tempInfo.lRanMaxMultiple = tagInfo.lRanMaxMultiple;

						tempInfo.iAllLostCount = tagInfo.iAllLostCount;
						tempInfo.iLostCount = tagInfo.iLostCount;
						tempInfo.iAllWinCount = tagInfo.iAllWinCount;
						tempInfo.iWinCount = tagInfo.iWinCount;
						tempInfo.iFreeSpin = tagInfo.iFreeSpin;
						tempInfo.jackpotIndex = tagInfo.jackpotIndex;
						tempInfo.iAllPreCount = tagInfo.iAllPreCount;
						tempInfo.iPreCount = tagInfo.iPreCount;
						rep.set_result(net::RESULT_CODE_SUCCESS);
					}
				}
				else
				{
					iResultCode = 2;
					if (oper_type == 1)
					{
						m_mpMasterCtrlInfo.insert(make_pair(tagInfo.uid, tagInfo));
						rep.set_result(net::RESULT_CODE_SUCCESS);
					}
				}
			}

			//for (int i = 0; i < iMaxMasterRandProCount; i++)
			//{
			//	rep.add_rand_pro(m_iArrMasterRandProCount[i]);
			//}

			LOG_DEBUG("send_master_ctrl_info - roomid:%d,tableid:%d,uid:%d,bIsMaster:%d,iResultCode:%d,oper_type:%d,rand_pro_size:%d,strArrRandPro:%s,tuid:%d,lMultiple:%lld - %lld,%lld - %lld,iLostCount:%d - %d,iWinCount:%d - %d,iFreeSpin:%d,jackpotIndex:%lld,iPreCount:%d - %d",
				GetRoomID(), GetTableID(), uid, bIsMaster, iResultCode, oper_type, msg.rand_pro_size(), strArrRandPro.c_str(), tagInfo.uid, tagInfo.lMinMultiple, tagInfo.lMaxMultiple, tagInfo.lRanMinMultiple, tagInfo.lRanMaxMultiple, tagInfo.iAllLostCount, tagInfo.iLostCount, tagInfo.iAllWinCount, tagInfo.iWinCount, tagInfo.iFreeSpin, tagInfo.jackpotIndex, tagInfo.iAllPreCount, tagInfo.iPreCount);

			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_SLOT_SEND_MASTER_CTRL_INFO_REP);

			if (rep.result() == net::RESULT_CODE_SUCCESS)
			{
				net::msg_show_slot_master_ctrl_info_rep boster;
				boster.set_suid(tagInfo.suid);
				boster.set_oper_type(oper_type);
				for (int i = 0; i < iMaxMasterRandProCount; i++)
				{
					boster.add_rand_pro(m_iArrMasterRandProCount[i]);
				}
				net::msg_send_slot_master_ctrl_info * pctrl_info = boster.mutable_info();
				pctrl_info->set_uid(tagInfo.uid);
				pctrl_info->set_min_multiple(tagInfo.lMinMultiple);
				pctrl_info->set_max_multiple(tagInfo.lMaxMultiple);
				pctrl_info->set_ran_min_multiple(tagInfo.lRanMinMultiple);
				pctrl_info->set_ran_max_multiple(tagInfo.lRanMaxMultiple);

				pctrl_info->set_all_lost_count(tagInfo.iAllLostCount);
				pctrl_info->set_lost_count(tagInfo.iLostCount);
				pctrl_info->set_all_win_count(tagInfo.iAllWinCount);
				pctrl_info->set_win_count(tagInfo.iWinCount);
				pctrl_info->set_free_spin(tagInfo.iFreeSpin);
				pctrl_info->set_jackpot_index(tagInfo.jackpotIndex);
				pctrl_info->set_all_pre_count(tagInfo.iAllPreCount);
				pctrl_info->set_pre_count(tagInfo.iPreCount);

				SendAllMasterUser(&boster, net::S2C_MSG_SLOT_SEND_MASTER_CTRL_INFO_BROADCAST);
			}
			return 0;
		}break;
	case net::C2S_MSG_SLOT_SHOW_MASTER_CTRL_INFO_REQ:
		{
			net::msg_show_slot_master_ctrl_info_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			bool bIsMaster = GetIsMasterUser(pPlayer);
			uint32 oper_uid = msg.uid();

			tagSlotMasterCtrlInfo tagInfo;
			int iResultCode = 0;
			if (bIsMaster)
			{
				auto iter_find = m_mpMasterCtrlInfo.find(oper_uid);
				if (iter_find != m_mpMasterCtrlInfo.end())
				{
					iResultCode = 1;
					tagInfo = iter_find->second;
				}
				else
				{
					iResultCode = 2;
					tagInfo.uid = oper_uid;
				}
			}

			LOG_DEBUG("show_master_ctrl_info - roomid:%d,tableid:%d,uid:%d,bIsMaster:%d,iResultCode:%d,tuid:%d,lMultiple:%lld - %lld,iLostCount:%d - %d,iWinCount:%d - %d,iFreeSpin:%d,jackpotIndex:%lld,iPreCount:%d - %d",
				GetRoomID(), GetTableID(), uid, bIsMaster, iResultCode, tagInfo.uid, tagInfo.lMinMultiple, tagInfo.lMaxMultiple, tagInfo.iAllLostCount, tagInfo.iLostCount, tagInfo.iAllWinCount, tagInfo.iWinCount, tagInfo.iFreeSpin, tagInfo.jackpotIndex, tagInfo.iAllPreCount, tagInfo.iPreCount);

			//SendMasterShowCtrlInfo(pPlayer, tagInfo);

			net::msg_show_slot_master_ctrl_info_rep rep;
			rep.set_suid(tagInfo.suid);
			for (int i = 0; i < iMaxMasterRandProCount; i++)
			{
				rep.add_rand_pro(m_iArrMasterRandProCount[i]);
			}
			net::msg_send_slot_master_ctrl_info * pctrl_info = rep.mutable_info();
			pctrl_info->set_uid(tagInfo.uid);
			pctrl_info->set_min_multiple(tagInfo.lMinMultiple);
			pctrl_info->set_max_multiple(tagInfo.lMaxMultiple);
			pctrl_info->set_ran_min_multiple(tagInfo.lRanMinMultiple);
			pctrl_info->set_ran_max_multiple(tagInfo.lRanMaxMultiple);

			pctrl_info->set_all_lost_count(tagInfo.iAllLostCount);
			pctrl_info->set_lost_count(tagInfo.iLostCount);
			pctrl_info->set_all_win_count(tagInfo.iAllWinCount);
			pctrl_info->set_win_count(tagInfo.iWinCount);
			pctrl_info->set_free_spin(tagInfo.iFreeSpin);
			pctrl_info->set_jackpot_index(tagInfo.jackpotIndex);
			pctrl_info->set_all_pre_count(tagInfo.iAllPreCount);
			pctrl_info->set_pre_count(tagInfo.iPreCount);

			LOG_DEBUG("roomid:%d,tableid:%d,suid:%d,tuid:%d,lMultiple:%lld - %lld,:%lld - %lld,iLostCount:%d - %d,iWinCount:%d - %d,iFreeSpin:%d,jackpotIndex:%lld,iPreCount:%d - %d",
				GetRoomID(), GetTableID(), pPlayer->GetUID(), tagInfo.uid, tagInfo.lMinMultiple, tagInfo.lMaxMultiple, tagInfo.lRanMinMultiple, tagInfo.lRanMaxMultiple, tagInfo.iAllLostCount, tagInfo.iLostCount, tagInfo.iWinCount, tagInfo.iWinCount, tagInfo.iFreeSpin, tagInfo.jackpotIndex, tagInfo.iAllPreCount, tagInfo.iPreCount);

			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_SLOT_SHOW_MASTER_CTRL_INFO_REP);
			return 0;
		}break;
	case net::C2S_MSG_SLOT_UPDATE_MASTER_SHOW_INFO_REQ:
		{
			net::msg_update_slot_master_show_info_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			LOG_DEBUG("update_show_master_info - roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), uid);
			SendMasterPlayerInfo(pPlayer);
			return 0;
		}break;
	case net::C2S_MSG_BRC_CONTROL_ENTER_TABLE_REQ://
		{
			net::msg_brc_control_user_enter_table_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			//bool ret = OnBrcControlPlayerEnterInterface(pPlayer);
			uint8 result = RESULT_CODE_SUCCESS;
			if (!pPlayer->GetCtrlFlag())
			{
				result = RESULT_CODE_FAIL;
			}
			else
			{
				auto iter_find_vec = std::find(m_vecMasterList.begin(), m_vecMasterList.end(), pPlayer->GetUID());
				if (iter_find_vec == m_vecMasterList.end())
				{
					m_vecMasterList.push_back(pPlayer->GetUID());
				}
			}

			LOG_DEBUG("enter_show_master_ctrl - roomid:%d,tableid:%d,uid:%d,result:%d", GetRoomID(), GetTableID(), uid, result);

			//返回结果消息
			net::msg_brc_control_user_enter_table_rep rep;
			rep.set_result(result);
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ENTER_TABLE_REP);

			return 0;
		}break;
	case net::C2S_MSG_BRC_CONTROL_LEAVE_TABLE_REQ://
		{
			net::msg_brc_control_user_leave_table_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			//return OnBrcControlPlayerLeaveInterface(pPlayer);

			auto iter_find_vec = std::find(m_vecMasterList.begin(), m_vecMasterList.end(), pPlayer->GetUID());
			if (iter_find_vec != m_vecMasterList.end())
			{
				m_vecMasterList.erase(iter_find_vec);
			}
			net::msg_brc_control_user_leave_table_rep rep;
			rep.set_result(RESULT_CODE_SUCCESS);

			LOG_DEBUG("leave_show_master_ctrl - roomid:%d,tableid:%d,uid:%d", GetRoomID(), GetTableID(), uid);


			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_LEAVE_TABLE_REP);

			return 0;
		}break;
	default:
		{
			return 0;
		}
	}
	return 0;
}

// 获取对应的图片集
void CGameSlotTable::GetSpinPics(CGamePlayer* pPlayer,uint32 nline, uint32 nbet, uint32 mGamePic[])
{
	//总押注
	//int64  nWinJackpotScore = 0;//赢得彩金数
	int64  m_lJetonScore = nline * nbet; //总押注
	uint32 randRate = g_RandGen.RandRange(1, MAX_RATIO);
	bool IsSysScore = false;//是否走系统库存
	//获取玩家库存
	int64 m_lPlayerScore = pPlayer->GetPlayerStockScore(GetGameType());
	//m_lSysScore +=  m_lJetonScore * (PRO_DENO_10000 - m_FeeRate - m_JackpotRate) / PRO_DENO_10000;
	//根据库存取图集
	if (m_lPlayerScore != 0)
	{
		//获取系统返奖调节图片矩阵
		if (m_lPlayerScore > 0)
		{
			if (m_lPlayerScore > m_lJetonScore)
			{
				int64 nWinPlayerScore = m_lJetonScore *  m_ReUserRate[1] / PRO_DENO_10000;
				pPlayer->AsyncChangeStockScore(GetGameType(),-nWinPlayerScore);// 修改游戏玩家个人库存并更新到数据库
				//m_lPlayerScore = m_lPlayerScore - m_lJetonScore *  m_ReUserRate[1] / MAX_WHEELS;//更新个人库存
				m_gameLogic.GetUserPicArray(randRate, nline, 1, mGamePic);//走玩家库存
				return;
			}
		}
		else
		{
			if (abs(m_lPlayerScore) > m_lJetonScore)
			{
				int64 nWinPlayerScore = m_lJetonScore * m_ReUserRate[0] / PRO_DENO_10000;//更新个人库存
				pPlayer->AsyncChangeStockScore(GetGameType(), nWinPlayerScore);// 修改游戏玩家个人库存并更新到数据库
				m_gameLogic.GetUserPicArray(randRate, nline, 0, mGamePic);//走玩家库存
				return;
			}
		}
	}

	//个人库存等于0 取公共库存
	//根据库存区间 获取不同的图集
	if (m_lSysScore < m_oSysRepertoryRange[0])
	{
		//更新系统库存
		m_gameLogic.GetSysPicArray(randRate, nline, 0, mGamePic);//走玩家库存
	}
	else if (m_lSysScore >= m_oSysRepertoryRange[0] && m_lSysScore < m_oSysRepertoryRange[1])
	{
		//更新系统库存
		m_gameLogic.GetSysPicArray(randRate, nline, 1, mGamePic);//走玩家库存
	}
	else if (m_lSysScore >= m_oSysRepertoryRange[1] && m_lSysScore < m_oSysRepertoryRange[2])
	{
		//更新系统库存
		m_gameLogic.GetSysPicArray(randRate, nline, 2, mGamePic);//走玩家库存
	}
	else if (m_lSysScore >= m_oSysRepertoryRange[2] && m_lSysScore < m_oSysRepertoryRange[3])
	{
		m_gameLogic.GetSysPicArray(randRate, nline, 3, mGamePic);//走玩家库存
	}
	else if (m_lSysScore >= m_oSysRepertoryRange[3])
	{
		m_gameLogic.GetSysPicArray(randRate, nline, 4, mGamePic);//走玩家库存

	}
	else
	{
		LOG_DEBUG("CGameSlotTable::GetSpinPics->m_lSysScore%lld,m_oSysRepertoryRange=%lld,%lld,%lld,%lld,%lld", m_lSysScore, m_oSysRepertoryRange[0], m_oSysRepertoryRange[1], m_oSysRepertoryRange[2], m_oSysRepertoryRange[3], m_oSysRepertoryRange[4]);
	}
	return;
}

// 获取有彩金生成概率
int64 CGameSlotTable::GetWinJackpotScore(uint32 nline, uint32 nbet, uint32  nIndex)
{
	int64  nChangeScore = 0;//彩金库存调节修正值
	//彩金判断修正率
	int64 nWinJackpotScore = 0;
	
	nChangeScore = ((m_lJackpotScore) / m_nJackpotChangeUnit + 1) * m_nJackpotChangeRate;

	if (nChangeScore != 0)
	{
		//彩金库存调节修正值范围
		if (nChangeScore > m_MaxJackpotReviseRate)
		{
			nChangeScore = m_MaxJackpotReviseRate;
		}
		else if (nChangeScore < m_MinJackpotReviseRate)
		{
			nChangeScore = m_MinJackpotReviseRate;
		}
	}
	//[800, 1000, 2000]
	//777   7777  77777
	nWinJackpotScore = m_lJackpotScore *  m_VecJackpotRatio[nIndex] / PRO_DENO_10000;
	//通过单线押注范围判断 可以获得彩金数范围
	if (nbet >= m_JackpotBetRange[0] && nbet < m_JackpotBetRange[1])
	{
		if (nWinJackpotScore >= m_JackpotRangeScore[0])
		{
			nWinJackpotScore = m_JackpotRangeScore[0];
		}
	}
	else if (nbet >= m_JackpotBetRange[1])
	{
		if (nWinJackpotScore >= m_JackpotRangeScore[1])
		{
			nWinJackpotScore = m_JackpotRangeScore[1];
		}
	}
	else if(nbet < m_JackpotBetRange[0])
	{
		nWinJackpotScore = 0;
	}

	//计算彩金的生成概率
	//LOG_DEBUG("CGameSlotTable::CGameSlotTable::GetJackpotRate nline=%d,nbet=%d, m_JackpotRate=%lld,nChangeScore =%lld,m_oPicRateConfig[PIC_SEVEN]=%d,nWinJackpotScore=%lld,m_JackpotBetRange:%lld,%lld", 
	//														  nline,   nbet,    m_JackpotRate,   nChangeScore,m_oPicRateConfig[PIC_SEVEN-1].multiple[nIndex+2], nWinJackpotScore, m_JackpotBetRange[0], m_JackpotBetRange[1]);
	
	//nHaveJackpotRate = (nline * nbet)* (m_JackpotRate + nChangeScore) * PRO_DENO_100 / (m_oPicRateConfig[PIC_SEVEN-1].multiple[nIndex+2] * nbet + nWinJackpotScore);

	//LOG_DEBUG("CGameSlotTable::CGameSlotTable::GetJackpotRate->nHaveJackpotRate=%lld,nWinJackpotScore=%lld,m_lJackpotScore=%lld,m_VecJackpotRatio[%d]=%d", nHaveJackpotRate, nWinJackpotScore, m_lJackpotScore, nIndex, m_VecJackpotRatio[nIndex]);
	return nWinJackpotScore;
}

//根据权重算法,获取拿到彩金库存比例
uint32 CGameSlotTable::GetWeightJackpotIndex()
{
	//根据权重表计算拿牌概率
	uint32   nWeightSum = 0; //权重总和
	for (uint8 i = 0; i < m_VecJackpotWeigh.size(); i++)
	{
		nWeightSum += m_VecJackpotWeigh[i];
	}
	uint32  nSectionNum = g_RandGen.RandUInt() % (nWeightSum * PRO_DENO_100);
	uint32   nCardIndex = 0;//根据权重获得能拿第几大的牌
	for (uint32 i = 0; i < m_VecJackpotWeigh.size(); i++)
	{
		if (nSectionNum <= (m_VecJackpotWeigh[i] * PRO_DENO_100))
		{
			nCardIndex = i;
			break;
		}
		else
		{
			nSectionNum -= (m_VecJackpotWeigh[i] * PRO_DENO_100);
		}
	}
	//不能超过最大值
	if (nCardIndex + 3 > MAXCOLUMN)
	{
		nCardIndex = 2;
	}
	return nCardIndex;
}

// 游戏开始摇一摇
bool CGameSlotTable::OnGameUserSpin(CGamePlayer* pPlayer, uint32 nline, uint32 nbet)
{
	//随机生成3 x 5 五组图案
	InitBlingLog(pPlayer);
	uint32 mGamePic[MAXROW][MAXCOLUMN];//图片数组
	uint32 tmpGamePic[MAXROW * MAXCOLUMN] = { 0 };
	memset(&mGamePic, 0, sizeof(mGamePic));
	
	bool bIsCorrectScore = false;

	for (uint32 i = 0; i < m_vecUserJettonCellScore.size(); i++)
	{
		LOG_ERROR("m_vecUserJettonCellScore - size:%d,cell:%d", m_vecUserJettonCellScore.size(), m_vecUserJettonCellScore[i]);

		if (m_vecUserJettonCellScore[i] == nbet)
		{
			bIsCorrectScore = true;
			break;
		}
	}

	LOG_DEBUG("uid:%d,nline:%d,nbet:%d,bIsCorrectScore:%d", pPlayer->GetUID(), nline, nbet, bIsCorrectScore);

	if (nline > MAX_LINE || nbet == 0 || nline == 0 || bIsCorrectScore==false)
	{
		net::msg_slot_game_error_rep msg;
		msg.set_result(net::RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_SLOT_ERROR_REP);

		LOG_DEBUG("jetton error - uid:%d,nline:%d,nbet:%d,bIsCorrectScore:%d", pPlayer->GetUID(), nline, nbet, bIsCorrectScore);

		return false;
	}
	int64 lJettonScore = (nline * nbet);
	if (TableJettonLimmit(pPlayer, lJettonScore, 0) == false)
	{

		LOG_DEBUG("table_jetton_limit - roomid:%d,tableid:%d,uid:%d,curScore:%lld,jettonmin:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), GetPlayerCurScore(pPlayer), GetJettonMin());
		net::msg_slot_game_error_rep msg;
		msg.set_result(net::RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_SLOT_ERROR_REP);
		return true;
	}
	int64 lUserScore = GetPlayerCurScore(pPlayer);//用户总金币
	if ((nline * nbet) > lUserScore)
	{
		net::msg_slot_game_error_rep msg;
		msg.set_result(net::RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_SLOT_ERROR_REP);
		return false;
	}
	FruitUserData UserData;
	UserData.CurLineNo = nline;   //当前押注的线号
	UserData.CurBetScore = nbet;     //当前押注的钱
	UserData.FreeTimes = 0;       //当前用户的免费次数
	UserData.Free_winscore = 0;    ////免费赢钱和

	//获取老虎机的图集
	int64  nWinJackpotScore = 0;//赢得彩金数
	
	GetSpinPics(pPlayer,nline, nbet, tmpGamePic); //获取图片

	bool bIsMasterCtrl = false;
	int out_hitIndex = 255;
	uint32 tmpMasterCtrlGamePic[MAXROW * MAXCOLUMN] = { 0 };
	bIsMasterCtrl = ProgressMasterCtrl(pPlayer, nline, nbet, tmpMasterCtrlGamePic, out_hitIndex);
	if (bIsMasterCtrl)
	{
		memcpy(tmpGamePic, tmpMasterCtrlGamePic, MAXROW * MAXCOLUMN * 4);
	}
	bool bIsFalgControl = false;
	if (bIsMasterCtrl == false)
	{
		uint32 tmpPalyerCtrlGamePic[MAXROW * MAXCOLUMN] = { 0 };
		bIsFalgControl = ProgressControlPalyer(pPlayer, nline, nbet, tmpPalyerCtrlGamePic);
		if (bIsFalgControl)
		{
			memcpy(tmpGamePic, tmpPalyerCtrlGamePic, MAXROW * MAXCOLUMN * 4);
		}
	}

	//ProgressControlPalyer(pPlayer, nline, nbet, tmpGamePic);

	memcpy(mGamePic, tmpGamePic, MAXROW * MAXCOLUMN * 4);	
	LOG_DEBUG("slot_image_  ------------------------------------------------------------------------------------------------------");
	int64 lPlayerGameCount = pPlayer->GetPlayerGameCount(GetGameType());
	LOG_DEBUG("slot_image_  uid:%d,nbet:%d,m_JackpotBetRange[0]:%d,m_lWinHandselGameCount:%lld,lPlayerGameCount:%lld,m_uWinHandselPre:%d,bIsMasterCtrl:%d,bIsFalgControl:%d",
		pPlayer->GetUID(), nbet, m_JackpotBetRange[0], m_lWinHandselGameCount, lPlayerGameCount, m_uWinHandselPre, bIsMasterCtrl, bIsFalgControl);

	//触发彩金判断
	if (bIsMasterCtrl == false && nbet >= m_JackpotBetRange[0] && GetWinHandselGameCount() <= lPlayerGameCount)
	{
		if (g_RandGen.RandRatio(m_uWinHandselPre, PRO_DENO_10000))
		{
			uint32  nIndex = 0;// GetWeightJackpotIndex();
			uint32 line = g_RandGen.RandRange(0, (nline - 1));
			uint32 uSevenCount = 3;//nIndex + 3;
			for (uint32 k = 0; k < uSevenCount; k++)
			{
				mGamePic[(m_oServerLinePos[line].pos[k] - 1) % MAXROW][(m_oServerLinePos[line].pos[k] - 1) / MAXROW] = PIC_SEVEN;
				LOG_DEBUG("slot_image_Jackpot -uid:%d,nline:%d,nbet:%d, mGamePic[%d][%d]=%d,nIndex=%d", pPlayer->GetUID(), nline, nbet,(m_oServerLinePos[line].pos[k] - 1) % MAXROW, (m_oServerLinePos[line].pos[k] - 1) / MAXROW, PIC_SEVEN, nIndex);
			}
			nWinJackpotScore = GetWinJackpotScore(nline, nbet, nIndex);
		}
	}

	// add by har
	bool isStockCtrl = false;
	if (!bIsMasterCtrl && !bIsFalgControl && nWinJackpotScore == 0)
		isStockCtrl = SetStockWinLose(pPlayer, nline, nbet, tmpGamePic);
	else
		memcpy(mGamePic, tmpGamePic, MAXROW * MAXCOLUMN * 4);
	if (isStockCtrl)
		memcpy(mGamePic, tmpGamePic, MAXROW * MAXCOLUMN * 4);
	// add by har end

	memcpy(tmpGamePic, mGamePic, MAXROW * MAXCOLUMN * 4);
	WritePicLog(tmpGamePic, MAXROW * MAXCOLUMN);//数据上报

	LOG_DEBUG("slot_image_ roomid:%d,tableid:%d ---------------------------------------------------------------------", GetRoomID(), GetTableID());
	LOG_DEBUG("slot_image_mGamePic - uid:%d,[%02d %02d %02d %02d %02d]", pPlayer->GetUID(), mGamePic[0][0], mGamePic[0][1], mGamePic[0][2], mGamePic[0][3], mGamePic[0][4]);
	LOG_DEBUG("slot_image_mGamePic - uid:%d,[%02d %02d %02d %02d %02d]", pPlayer->GetUID(), mGamePic[1][0], mGamePic[1][1], mGamePic[1][2], mGamePic[1][3], mGamePic[1][4]);
	LOG_DEBUG("slot_image_mGamePic - uid:%d,[%02d %02d %02d %02d %02d]", pPlayer->GetUID(), mGamePic[2][0], mGamePic[2][1], mGamePic[2][2], mGamePic[2][3], mGamePic[2][4]);

	LOG_DEBUG("solt_ctrl_score - roomid:%d,tableid:%d,uid:%d,isRobot:%d,bIsMasterCtrl:%d,bIsFalgControl:%d,nWinJackpotScore:%lld,lJackpotScore:%lld,isStockCtrl:%d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), pPlayer->IsRobot(), bIsMasterCtrl, bIsFalgControl, nWinJackpotScore, m_lJackpotScore, isStockCtrl);


	//if (nWinJackpotScore > 0)
	//{
	//	m_gameLogic.GetJackpotScoreByLine(nline, mGamePic);
	//}
    //触发彩金判断
	//if (m_lJackpotScore > m_MinJackpotScore && nbet >= 1000)
	//{
	//	//根据权重计算可以赢得的彩金数
	//	uint32  nIndex = GetWeightJackpotIndex();//随机可以获得777对应个数赔率索引号	
	//	int64  ntmpWinJackpotScore = 0;
	//	int64 tmpJackpotRate = GetJackpotRate(nline ,nbet, nIndex, ntmpWinJackpotScore);
	//	if(tmpJackpotRate >= g_RandGen.RandRange(1, PRO_DENO_100w)) //可以生成彩金
	//	{
	//		nWinJackpotScore = ntmpWinJackpotScore;
	//		uint32 line = g_RandGen.RandRange(0, (MAX_LINE - 1));
	//		for (uint32 k = 0; k < nIndex + 3; k++)
	//		{		
	//			mGamePic[(m_oServerLinePos[line].pos[k] - 1) % MAXROW][(m_oServerLinePos[line].pos[k] - 1) / MAXROW] = PIC_SEVEN;
	//		//	LOG_DEBUG("CGameSlotTable::OnGameUserSpin->mGamePic[%d][%d]=%d,nIndex=%d", (m_oServerLinePos[line].pos[k] - 1) % MAXROW, (m_oServerLinePos[line].pos[k] - 1) / MAXROW, PIC_SEVEN, nIndex);
	//		}
	//	}
	//}

	
	picLinkVecType picLinkVec; //押注结果
	int64 nTotalMultiple = 0;//总倍率

	//计算所有的线的倍率
	for (uint32 i = 0; i < nline; i++)
	{
		uint32 linePic[MAXCOLUMN] = { 0 };//顺序的线
		// 计算每条线的赔率
		for (uint32 k = 0; k < MAXCOLUMN; k++)
		{
			linePic[k] = mGamePic[(m_oServerLinePos[i].pos[k] - 1) % MAXROW][(m_oServerLinePos[i].pos[k] - 1) / MAXROW];
		}

		LOG_DEBUG("slot_image_linePic - uid:%d,[%02d %02d %02d %02d %02d]", pPlayer->GetUID(), linePic[0], linePic[1], linePic[2], linePic[3], linePic[4]);


		//统计线的结果
		picLinkinfo pLink;
		pLink.lineNo = i + 1;//线号
		//计算赔率

		uint32 	lineNo;  //线号
		uint32	picID;	//图片ID
		uint32 	linkNum; //连接数量

		int64 nMultipleTmp = GetLineMultiple(linePic, pLink);
		if (nMultipleTmp > 0 && pLink.picID != PIC_BONUS)
		{
			picLinkVec.push_back(pLink);
			nTotalMultiple += nMultipleTmp;

			LOG_DEBUG("slot_image_Multiple - uid:%d,lineNo:%d, picID:%d, linkNum:%d, nMultipleTmp:%d, nTotalMultiple:%d",
				pPlayer->GetUID(), pLink.lineNo, pLink.picID, pLink.linkNum, nMultipleTmp, nTotalMultiple);

		}
		//计算bonus-奖励
		if (pLink.picID == PIC_BONUS && pLink.linkNum <= MAXCOLUMN)
		{
			UserData.FreeTimes += s_FreeTimes[pLink.linkNum];
		}
	}
	UserData.bIsMasterCtrl = bIsMasterCtrl;
	AddFruitUserData(pPlayer->GetUID(), UserData);//保存用户记录
	//计算奖励
	int64 nWinScore = 0;
	nWinScore = nTotalMultiple * nbet;

	//数据上报
	m_operLog["totalmultiple"] = nTotalMultiple;//总赔率
	m_operLog["betscore"] = nbet;//线注
	m_operLog["linecount"] = nline;//线数
	m_operLog["totalmultiple"] = nTotalMultiple;//系统库存
	m_operLog["bonustimes"] = UserData.FreeTimes;//bonus次数
	//玩家输赢结算
	CalcPlayerInfo(pPlayer, nline * nbet, nWinScore, nWinJackpotScore);
	TatalLineUserBet(pPlayer, nline, nbet, nWinScore);
	//返回客户端结果
	net::msg_slot_game_spin_rep msg;
	net::pos_pics* ppics = msg.add_pics();
	//返回的图片
	for (uint32 i = 0; i < MAXCOLUMN; i++)
	{
		for (uint32 j = 0; j < MAXROW; j++)
		{
			ppics->add_pic(mGamePic[j][i]);//保存所有图片序号返回给客户端
		}
	//	LOG_DEBUG("CGameSlotTable::OnGameUserSpin->GamePic=%d,%d,%d", mGamePic[0][i], mGamePic[1][i], mGamePic[2][i]);
	}
	lUserScore = GetPlayerCurScore(pPlayer);//用户总金币	
	LOG_DEBUG("nWinScore=%d nTotalMultiple=%d,UserScore=%lld,nWinJackpotScore=%lld", nWinScore, nTotalMultiple, lUserScore, nWinJackpotScore);
	msg.set_freetimes(UserData.FreeTimes);//获得免费转次数
	msg.set_winscore(nWinScore);   //赢的金币
	msg.set_userscore(lUserScore);//用户总金币
	msg.set_winjackpot_score(nWinJackpotScore);//用户赢得彩金池
	msg.set_jackpot_score(m_lJackpotScore);//用户总彩金池
	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_SLOT_SPIN_REP);
	
	if (nWinJackpotScore > 0)
	{
		msg_slot_game_jackpot_score msg_jackpot_score;
		msg_jackpot_score.set_score(m_lJackpotScore);
		map<uint32, CGamePlayer*>::iterator it_JackpotScore = m_mpLookers.begin();
		for (; it_JackpotScore != m_mpLookers.end(); ++it_JackpotScore)
		{
			CGamePlayer* pPlayer = it_JackpotScore->second;
			if (pPlayer != NULL)
			{
				if (pPlayer->IsRobot() == false)
				{
					pPlayer->SendMsgToClient(&msg_jackpot_score, S2C_MSG_SLOT_JACKPOT_SCORE_REP);
				}
			}
		}
	}

	SendLookerListToClient();

	return true;
}

// 免费转
bool CGameSlotTable::OnGameUserFreeSpin(CGamePlayer* pPlayer)
{
	//测试代码
	//随机生成3 x 5 五组图案
	//AutoLock_T  alt(m_tl);
	InitBlingLog(pPlayer);
	uint32 mGamePic[MAXROW][MAXCOLUMN];//图片数组
	uint32 tmpGamePic[MAXROW * MAXCOLUMN] = { 0 };
	memset(&mGamePic, 0, sizeof(mGamePic));

	picLinkVecType picLinkVec; //押注结果
	int64 nTotalMultiple = 0;//总倍率
	int64  nWinJackpotScore = 0;//赢得彩金数
	//获取用户押注的数据
	map<uint32, FruitUserData>::iterator iter = m_oFruitUserData.find(pPlayer->GetUID());
	if (iter != m_oFruitUserData.end())
	{
		if (iter->second.FreeTimes == 0)
		{
			net::msg_slot_game_error_rep msg;
			msg.set_result(net::RESULT_CODE_FAIL);
			pPlayer->SendMsgToClient(&msg, net::S2C_MSG_SLOT_ERROR_REP);
			LOG_DEBUG("CGameSlotTable::OnGameUserFreeSpin return false,FreeTimes=[%d],", iter->second.FreeTimes);
			return false;			
		}
		iter->second.FreeTimes--;
	}
	//数据校验
	if (iter->second.CurLineNo == 0 || iter->second.CurBetScore == 0)
	{
		net::msg_slot_game_error_rep msg;
		msg.set_result(net::RESULT_CODE_FAIL);
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_SLOT_ERROR_REP);
		LOG_DEBUG("CGameSlotTable::OnGameUserFreeSpin->is errr CurLineNo[%d]CurBetScore=[%d],", iter->second.CurLineNo, iter->second.CurBetScore);
		return false;
	}
	//获取老虎机的图集
	GetSpinPics(pPlayer,iter->second.CurLineNo, iter->second.CurBetScore, tmpGamePic);

	if (iter->second.bIsMasterCtrl)
	{
		tagSlotMasterCtrlInfo tempInfo;
		GetRandMutail(tempInfo);
		uint32 tmpMasterCtrlGamePic[MAXROW * MAXCOLUMN] = { 0 };
		bool bIsControlPalyerFreeCount = SetControlPalyerFreeCount(pPlayer, iter->second.CurLineNo, iter->second.CurBetScore, tmpMasterCtrlGamePic, 0, tempInfo.lRanMinMultiple, tempInfo.lRanMaxMultiple);
		if (bIsControlPalyerFreeCount)
		{
			memcpy(tmpGamePic, tmpMasterCtrlGamePic, MAXROW * MAXCOLUMN * 4);
		}
	}

	memcpy(mGamePic, tmpGamePic, MAXROW * MAXCOLUMN * 4);
	//触发彩金判断
	//if (GetWinHandselGameCount() <= pPlayer->GetPlayerGameCount(GetGameType()))
	//{
	//	if (g_RandGen.RandRatio(m_uWinHandselPre, PRO_DENO_10000))
	//	{
	//		uint32  nIndex = GetWeightJackpotIndex();
	//		nWinJackpotScore = GetWinJackpotScore(iter->second.CurLineNo, iter->second.CurBetScore, nIndex);
	//		if (nWinJackpotScore > 0)
	//		{
	//			m_gameLogic.GetJackpotScoreByLine(iter->second.CurLineNo, mGamePic);
	//		}
	//	}
	//}

	//触发彩金判断
	if (iter->second.CurBetScore >= m_JackpotBetRange[0] && GetWinHandselGameCount() <= pPlayer->GetPlayerGameCount(GetGameType()))
	{
		if (g_RandGen.RandRatio(m_uWinHandselPre, PRO_DENO_10000))
		{
			uint32  nIndex = 0;// GetWeightJackpotIndex();
			uint32 line = g_RandGen.RandRange(0, (iter->second.CurLineNo - 1));
			uint32 uSevenCount = 3;// nIndex + 3;
			for (uint32 k = 0; k < uSevenCount; k++)
			{
				mGamePic[(m_oServerLinePos[line].pos[k] - 1) % MAXROW][(m_oServerLinePos[line].pos[k] - 1) / MAXROW] = PIC_SEVEN;
				LOG_DEBUG("slot_image_Jackpot -nline:%d,nbet:%d, mGamePic[%d][%d]=%d,nIndex=%d", iter->second.CurLineNo, iter->second.CurBetScore, (m_oServerLinePos[line].pos[k] - 1) % MAXROW, (m_oServerLinePos[line].pos[k] - 1) / MAXROW, PIC_SEVEN, nIndex);
			}
			nWinJackpotScore = GetWinJackpotScore(iter->second.CurLineNo, iter->second.CurBetScore, nIndex);
		}
	}

	
	//if(m_lJackpotScore > m_MinJackpotScore && iter->second.CurBetScore >= 1000)
	//{
	//	//根据权重计算可以赢得的彩金数
	//	uint32  nIndex = GetWeightJackpotIndex();//随机可以获得777对应个数赔率索引号
	//	int64  ntmpWinJackpotScore = 0;
	//	int64 tmpJackpotRate = GetJackpotRate(iter->second.CurLineNo , iter->second.CurBetScore, nIndex, ntmpWinJackpotScore);
	//	if(tmpJackpotRate >= g_RandGen.RandRange(1, PRO_DENO_100w))   //可以生成彩金
	//	{
	//		nWinJackpotScore = ntmpWinJackpotScore;
	//		uint32 line = g_RandGen.RandRange(0, (MAX_LINE - 1));
	//		for (uint32 k = 0; k < nIndex + 3; k++)
	//		{
	//			mGamePic[(m_oServerLinePos[line].pos[k] - 1) % MAXROW][(m_oServerLinePos[line].pos[k] - 1) / MAXROW] = PIC_SEVEN;
	//			//LOG_DEBUG("CGameSlotTable::OnGameUserFreeSpin->mGamePic[%d][%d]=%d,", k, line, PIC_SEVEN);
	//		}
	//	}
	//}


	memcpy(tmpGamePic, mGamePic, MAXROW * MAXCOLUMN * 4);
	WritePicLog(tmpGamePic, MAXROW * MAXCOLUMN);//数据上报
	//计算所有的线的倍率
	for (uint32 i = 0; i < iter->second.CurLineNo; i++)
	{
		uint32 linePic[MAXCOLUMN] = { 0 };//顺序的线
		//计算每条线的赔率
		for (int k = 0; k < MAXCOLUMN; k++)
		{
			linePic[k] = mGamePic[(m_oServerLinePos[i].pos[k] - 1) % MAXROW][(m_oServerLinePos[i].pos[k] - 1) / MAXROW];
		}
		//统计线的结果
		picLinkinfo pLink;
		pLink.lineNo = i + 1;//线号
		//计算赔率
		int64 nMultipleTmp = GetLineMultiple(linePic, pLink);
		if (nMultipleTmp > 0 && pLink.picID != PIC_BONUS)
		{
			picLinkVec.push_back(pLink);
			nTotalMultiple += nMultipleTmp;
		}
		//计算bonus-奖励
		if (pLink.picID == PIC_BONUS && pLink.linkNum <= MAXCOLUMN)
		{
			iter->second.FreeTimes += s_FreeTimes[pLink.linkNum];
		}
		//计算777-彩金
	}
	//结算输赢
	int64 nWinScore = nTotalMultiple * iter->second.CurBetScore;
	//返回客户端结果
	net::msg_slot_game_spin_rep msg;
	net::pos_pics* ppics = msg.add_pics();
	//返回的图片
	for (uint32 i = 0; i < MAXCOLUMN; i++)
	{
		for (uint32 j = 0; j < MAXROW; j++)
		{
			ppics->add_pic(mGamePic[j][i]);//保存所有图片序号返回给客户端
		}
	}
	//数据上报
	m_operLog["totalmultiple"] = nTotalMultiple;//总赔率
	m_operLog["betscore"] = iter->second.CurBetScore;//线注
	m_operLog["linecount"] = iter->second.CurLineNo;//线数
	m_operLog["totalmultiple"] = nTotalMultiple;//系统库存
	m_operLog["bonustimes"] = iter->second.FreeTimes;//bonus次数
	CalcPlayerInfo(pPlayer, 0, nWinScore, nWinJackpotScore);
	TatalLineUserBet(pPlayer, iter->second.CurLineNo, iter->second.CurBetScore, nWinScore);
	iter->second.Free_winscore += nWinScore;//免费赢钱和
	//返回消息	
	int64 lUserScore = GetPlayerCurScore(pPlayer);;//用户总金币
	LOG_DEBUG("nWinScore=%d nTotalMultiple=%d,UserScore=%lld,nWinJackpotScore=%lld,FreeTimes =%d", nWinScore, nTotalMultiple, lUserScore, nWinJackpotScore, iter->second.FreeTimes);
	msg.set_freetimes(iter->second.FreeTimes);//获得免费转次数	
	msg.set_winscore(nWinScore);   //赢的金币
	msg.set_userscore(lUserScore);//用户总金币
	msg.set_winjackpot_score(nWinJackpotScore);  //用户赢得彩金池
	msg.set_jackpot_score(m_lJackpotScore);//用户总彩金池
	msg.set_free_winscore(iter->second.Free_winscore);//免费赢钱和
	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_SLOT_FREE_SPIN_REP);
	return true;
}

//结算
int64   CGameSlotTable::CalcPlayerInfo(CGamePlayer* pPlayer, int64 lJettonScore, int64 nWinScore, int64 lJackpotScore)
{
	uint32 uid = pPlayer->GetUID();
	int64 CalcScore = nWinScore - lJettonScore;
	int64 lUserScore = GetPlayerCurScore(pPlayer);//用户总金币
	m_operLog["oldv"] = lUserScore;
	
	int64 m_lPlayerScore = pPlayer->GetPlayerStockScore(GetGameType());
	
	LOG_DEBUG("CGameSlotTable::CalcPlayerInfo:id=%d,CalcScore= %lld,m_lPlayerScore=%lld", uid, CalcScore,m_lPlayerScore);
	//玩家输赢结算	
	//库存处理
	//m_lSysScore += lJettonScore * m_FeeRate / PRO_DENO_10000;                            //普通库存
	m_lFeeScore     += lJettonScore * m_FeeRate / PRO_DENO_10000;                            //抽水库存
	
	m_lJackpotScore -= lJackpotScore;//减去中奖彩金

	if (CalcScore < 0)
	{
		m_lSysScore -= CalcScore;
		m_operLog["sysfee"] = 0;//系统抽水
	}
	else
	{
		m_lSysScore -= (CalcScore + lJackpotScore);

		//int64 lAddJackpotScore = CalcScore * m_JackpotRate / PRO_DENO_10000;	//彩金库存
		//m_lJackpotScore += lAddJackpotScore;
		//CalcScore -= lAddJackpotScore;

		//int64 lSubSysFee = CalcScore * m_FeeRate / PRO_DENO_10000;//系统抽水
		//m_operLog["sysfee"] = lSubSysFee;
		//CalcScore -= lSubSysFee;

		int64 lSumFree = m_JackpotRate + m_FeeRate;
		if (lSumFree != 0)
		{
			int64 lSubFee = CalcScore * (lSumFree) / PRO_DENO_10000;	//彩金库存
			CalcScore -= lSubFee;

			m_lJackpotScore += lSubFee * m_JackpotRate / lSumFree;
			m_operLog["sysfee"] = lSubFee * m_FeeRate / lSumFree;

		}
		else
		{
			m_operLog["sysfee"] = 0;
		}

	}
	int64 fee = CalcPlayerGameInfo(uid, CalcScore, lJackpotScore, false);
	lUserScore = GetPlayerCurScore(pPlayer);//用户总金币
	m_operLog["newv"] = lUserScore;
	//m_lSysScore     -= nWinScore;//更新系统库存
	m_operLog["winscore"]        = CalcScore + lJackpotScore;//输赢
	m_operLog["winjackpotscore"] = lJackpotScore;//赢彩金库存
	m_operLog["jackpotscore"]    = m_lJackpotScore;//彩金库存
	m_operLog["sysscore"]        = m_lSysScore;//系统库存
	m_operLog["playerscore"] = m_lPlayerScore;//个人库存
	//m_operLog["sysfee"]          = lJettonScore * m_FeeRate / PRO_DENO_10000;//系统抽水
	SaveBlingLog();//上报php
	BroadcasPlayerInfo(uid,1, nWinScore);
	BroadcasPlayerInfo(uid,2, lJackpotScore);

	LOG_DEBUG("cacl - uid:%d,m_lJackpotScore:%lld", pPlayer->GetUID(), m_lJackpotScore);
	if (!pPlayer->IsRobot()) {
		LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,uid:%d,CalcScore:%lld,lJackpotScore:%lld,lUserScore:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), CalcScore, lJackpotScore, lUserScore);
		SetIsAllRobotOrPlayerJetton(false);
		m_pHostRoom->UpdateStock(this, CalcScore + lJackpotScore); // add by har
	}
	return 0;
}

// 写入图片信息
void CGameSlotTable::WritePicLog(uint32 GamePic[], uint32 picCount)
{
	//Json::Value logValue;
	for (uint32 i = 0; i < picCount; ++i) 
	{
		m_operLog["pic"].append(GamePic[i]);
	}
	//m_operLog["wheel"].append(logValue);
}
//玩家进入或离开
void  CGameSlotTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
	uint32 uid = 0;
	if (pPlayer != NULL) {
		uid = pPlayer->GetUID();
	}
	LOG_DEBUG("player join - uid:%d,chairID:%d,isJoin:%d,looksize:%d,lCurScore:%lld", uid, chairID, isJoin, m_mpLookers.size(), GetPlayerCurScore(pPlayer));
	UpdateEnterScore(isJoin, pPlayer);
	//SendSeatInfoToClient();
	
    if(isJoin)
	{
		RefreshAllMasterPlayerUser(1, pPlayer);
        SendGameScene(pPlayer);
		//SendTableInfoToClient(pPlayer);
		//SendSeatInfoToClient(pPlayer);
    }
	else
	{
		RefreshAllMasterPlayerUser(2, pPlayer);
		if (pPlayer->IsRobot() == false)
		{
			auto iter_find_player_info = m_mpMasterPlayerInfo.find(pPlayer->GetUID());
			if (iter_find_player_info != m_mpMasterPlayerInfo.end())
			{
				m_mpMasterPlayerInfo.erase(iter_find_player_info);
			}
			auto iter_find_master_ctrl = m_mpMasterCtrlInfo.find(pPlayer->GetUID());
			if (iter_find_master_ctrl != m_mpMasterCtrlInfo.end())
			{
				m_mpMasterCtrlInfo.erase(iter_find_master_ctrl);
			}
		}
    }
	SendLookerListToClient();

	//清理用户数据
	DelFruitUserData(pPlayer->GetUID());

	CGameTable::OnPlayerJoin(isJoin, chairID, pPlayer);

	//LOG_DEBUG("player join - chairID:%d,uid:%d", chairID, pPlayer->GetUID());
}

// 发送场景信息(断线重连)
void   CGameSlotTable::SendGameScene(CGamePlayer* pPlayer)
{
    uint16 chairID = GetChairID(pPlayer);
    LOG_DEBUG("send game scene:%d", chairID);
	switch(m_gameState)
	{
	case net::TABLE_STATE_FREE: //空闲状态
    case net::TABLE_STATE_WAIT:
	case net::TABLE_STATE_PLAY:	//游戏状态
		{
			int64 lUserScore = GetPlayerCurScore(pPlayer);
			net::msg_slot_game_play_info msg;
			map<uint32, FruitUserData>::iterator iter = m_oFruitUserData.find(pPlayer->GetUID());
			if (iter != m_oFruitUserData.end())
			{
				msg.set_freetimes(iter->second.FreeTimes);//免费次数
			}
			else
			{
				msg.set_freetimes(0);
			}
			msg.set_game_status(m_gameState);
            msg.set_userscore(lUserScore);//用户金币
            msg.set_jackpot_score(m_lJackpotScore);//彩金
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_SLOT_GAME_PLAY_INFO);
			uint16 roomid = 0;
			if (m_pHostRoom != NULL) {
				roomid = m_pHostRoom->GetRoomID();
			}
			LOG_DEBUG("roomid:%d,tableid:%d,", roomid, GetTableID());
		}break;
	}    

}
//广播
int64    CGameSlotTable::BroadcasPlayerInfo(uint32 uid, uint16 actType,int64 winScore)
{
    //LOG_DEBUG("report game to lobby:%d  %lld", uid,winScore);
	int64 fee_value = 0;
	////发送推送广播
	if (winScore > 0 && m_lBroadcastWinScore <= winScore && m_bBroadcast)
	{
		stGameBroadcastLog BroadcastLog;
		BroadcastLog.uid = uid;
		BroadcastLog.actType = actType;
		BroadcastLog.gameType = GAME_CATE_FRUIT_MACHINE;
		BroadcastLog.score = winScore;
		SaveBroadcastLog(BroadcastLog);
	}
	return fee_value;
}
// 重置游戏数据
void    CGameSlotTable::ResetGameData()
{
	//游戏变量重置
}
// 写入出牌log
void    CGameSlotTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount)
{
	uint8 cardType = 0;// m_gameLogic.GetCardType(cardData, cardCount);
    Json::Value logValue;
    logValue["p"] = chairID;
    logValue["cardtype"] = cardType;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
    m_operLog["card"].append(logValue);
}
// 写入加注log
void CGameSlotTable::WriteAddScoreLog(uint16 chairID,int64 score)
{
    Json::Value logValue;
    logValue["p"]       = chairID;
    logValue["s"]       = score;
    logValue["cmp"]     = 0;
    logValue["cp"]       = 0;
    logValue["win"]      = 0;

    m_operLog["score"].append(logValue);
}

//获取玩家数
uint16  CGameSlotTable::GetPlayNum()
{
    //人数统计
    WORD wPlayerCount=0;
    return wPlayerCount;
}

//计算相连的个数
int64 CGameSlotTable::GetLineMultiple(uint32 GamePic[MAXCOLUMN], picLinkinfo &picLink)
{
	int64 nMultiple = 0;
	picLink.linkNum = 0;
	//判断首位是否是万能图片
	if (PIC_WILD == GamePic[0])
	{
		for (uint32 i = 1; i < MAXCOLUMN; i++)//计算相同的连线
		{
			//特殊符号
			if (0 == GamePic[i] || GamePic[i] == PIC_BONUS || GamePic[i] == PIC_SEVEN)
			{
				break;
			}
			if (GamePic[i] != PIC_WILD && GamePic[0] == PIC_WILD)
			{
				GamePic[0] = GamePic[i];
			}
			//统计个数
			if(GamePic[i] == GamePic[0] || GamePic[i] == PIC_WILD)
			{
				picLink.linkNum += 1;
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		//计算相同的连线
		for (uint32 i = 1; i < MAXCOLUMN; i++)//计算相同的连线
		{
			if (0 == GamePic[i] )
			{
				break;
			}
			//统计相连的个数
			if ( GamePic[0] == GamePic[i] || (GamePic[i] == PIC_WILD && PIC_BONUS != GamePic[0] && PIC_SEVEN != GamePic[0]))
			{
				picLink.linkNum += 1;
			}
			else
			{
				break;
			}		
		}
	}
	picLink.picID = GamePic[0];
	//获取赔率
	//LOG_DEBUG("CGameSlotTable::GetLineMultiple->GamePic[0]:%d,picLink.linkNum:%d,", GamePic[0], picLink.linkNum);
	if (picLink.linkNum >= MAXCOLUMN || GamePic[0] == 0)
	{
		return nMultiple;
	}
	nMultiple = m_oPicRateConfig[GamePic[0]-1].multiple[picLink.linkNum];//获取赔率
	//LOG_DEBUG("CGameSlotTable::GetLineMultiple->nMultiple=%d,", nMultiple);
	return	nMultiple;
}

// 更新用户押注数据
void CGameSlotTable::AddFruitUserData(uint32 uid, FruitUserData & UserData)
{
	map<uint32, FruitUserData>::iterator iter = m_oFruitUserData.find(uid);
	if (iter == m_oFruitUserData.end())
	{
		m_oFruitUserData[uid] = UserData;
	}
	else 
	{
		iter->second.CurLineNo     = UserData.CurLineNo;//当前玩家押注的线号
		iter->second.CurBetScore   = UserData.CurBetScore;//当前玩家押注的钱ID
		iter->second.FreeTimes     =  UserData.FreeTimes; //当前玩家免费的次数
		iter->second.Free_winscore = UserData.Free_winscore; //当前玩家免费的次数
		iter->second.bIsMasterCtrl = UserData.bIsMasterCtrl;
	}
	return;
}

// 删除用户押注数据
void CGameSlotTable::DelFruitUserData(uint32 uid)
{
	map<uint32, FruitUserData>::iterator iter = m_oFruitUserData.find(uid);
	if (iter != m_oFruitUserData.end())
	{
		m_oFruitUserData.erase(iter);
	}
	return;
}


void    CGameSlotTable::GetAllRobotPlayer(vector<CGamePlayer*> & robots)
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


uint32 CGameSlotTable::GetMinJettonScore()
{
	uint32 score = 100;
	for (uint32 i = 0; i < m_vecUserJettonCellScore.size(); i++)
	{
		if (m_vecUserJettonCellScore[i] < score)
		{
			score = m_vecUserJettonCellScore[i];
		}
	}
	if (m_vecUserJettonCellScore.size() >= 3)
	{
		uint32 iIndex = g_RandGen.RandRange(0, 2);
		if (iIndex > 0 && iIndex < m_vecUserJettonCellScore.size())
		{
			score = m_vecUserJettonCellScore[iIndex];
		}
	}
	return score;
}

//机器人操作
void CGameSlotTable::OnRobotOper()
{
	vector<CGamePlayer*> robots;
	GetAllRobotPlayer(robots);
	for (uint32 i = 0; i < robots.size(); i++)
	{
		CGamePlayer* pPlayer = robots[i];
		if (pPlayer == NULL || !pPlayer->IsRobot())
		{
			continue;
		}
		if (g_RandGen.RandRatio(5000, PRO_DENO_10000))
		{
			continue;
		}
		uint32 nline = g_RandGen.RandRange(1, 3);
		uint32 nbet = GetMinJettonScore();
		LOG_DEBUG("robot jetton - uid:%d,nbet:%d", pPlayer->GetUID(), nbet);
		bool bRet = OnGameUserSpin(pPlayer, nline, nbet);
		if (bRet)
		{
			return;
		}
	}

}

bool CGameSlotTable::SetControlPalyerLost(uint32 nline, uint32 mGamePic[])
{
	int iCount = 1000;
	for (int iIndex = 0; iIndex < iCount; iIndex++)
	{
		uint32 mCaclGamePic[MAXROW][MAXCOLUMN] = { 0 };
		m_gameLogic.GetRandPicArray(mCaclGamePic);

		int64 nTotalMultiple = 0;//总倍率

		for (uint32 i = 0; i < nline; i++)
		{
			uint32 linePic[MAXCOLUMN] = { 0 };
			for (uint32 k = 0; k < MAXCOLUMN; k++)
			{
				linePic[k] = mCaclGamePic[(m_oServerLinePos[i].pos[k] - 1) % MAXROW][(m_oServerLinePos[i].pos[k] - 1) / MAXROW];
			}

			//LOG_DEBUG("slot_image_linePic - uid:%d,[%02d %02d %02d %02d %02d]", pPlayer->GetUID(), linePic[0], linePic[1], linePic[2], linePic[3], linePic[4]);

			picLinkinfo pLink;
			pLink.lineNo = i + 1;//线号
			uint32 	lineNo;  //线号
			uint32	picID;	//图片ID
			uint32 	linkNum; //连接数量

			int64 nMultipleTmp = GetLineMultiple(linePic, pLink);
			if (nMultipleTmp > 0 && pLink.picID != PIC_BONUS)
			{
				nTotalMultiple += nMultipleTmp;
				//LOG_DEBUG("slot_image_Multiple - uid:%d,lineNo:%d, picID:%d, linkNum:%d, nMultipleTmp:%d, nTotalMultiple:%d",
				//	pPlayer->GetUID(), pLink.lineNo, pLink.picID, pLink.linkNum, nMultipleTmp, nTotalMultiple);
			}

		}
		if (nTotalMultiple <= 0)
		{
			memcpy(mGamePic, mCaclGamePic, MAXROW * MAXCOLUMN * 4);
			return true;
		}
	}
	return false;
}

bool CGameSlotTable::SetControlPalyerWin(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[], int64 &out_winscore)
{
	out_winscore = 0;
	int64 nTotalMultiple = 0;
	int64 lJettonScore = nbet * nline;
	int64 lWinScore = 0;
	int iCount = 1000;
	for (int iIndex = 0; iIndex < iCount; iIndex++)
	{
		uint32 mCaclGamePic[MAXROW][MAXCOLUMN] = { 0 };
		m_gameLogic.GetRandPicArray(mCaclGamePic);

		nTotalMultiple = 0;
		lWinScore = 0;

		for (uint32 i = 0; i < nline; i++)
		{
			uint32 linePic[MAXCOLUMN] = { 0 };
			for (uint32 k = 0; k < MAXCOLUMN; k++)
			{
				linePic[k] = mCaclGamePic[(m_oServerLinePos[i].pos[k] - 1) % MAXROW][(m_oServerLinePos[i].pos[k] - 1) / MAXROW];
			}

			//LOG_DEBUG("slot_image_linePic - uid:%d,[%02d %02d %02d %02d %02d]", pPlayer->GetUID(), linePic[0], linePic[1], linePic[2], linePic[3], linePic[4]);

			picLinkinfo pLink;
			pLink.lineNo = i + 1;//线号
			uint32 	lineNo;  //线号
			uint32	picID;	//图片ID
			uint32 	linkNum; //连接数量

			int64 nMultipleTmp = GetLineMultiple(linePic, pLink);
			if (nMultipleTmp > 0 && pLink.picID != PIC_BONUS)
			{
				nTotalMultiple += nMultipleTmp;
			}
		}
		lWinScore = nTotalMultiple * nbet;
		if (nTotalMultiple > 0 && nTotalMultiple < 60 && lWinScore > lJettonScore)
		{
			memcpy(mGamePic, mCaclGamePic, MAXROW * MAXCOLUMN * 4);
			out_winscore = lWinScore; // add by har
			LOG_DEBUG("uid:%d,lJettonScore:%lld, lWinScore:%lld, nTotalMultiple:%lld",
				pPlayer->GetUID(), lJettonScore, lWinScore, nTotalMultiple);

			return true;
		}
	}
	LOG_DEBUG("uid:%d,lJettonScore:%lld, lWinScore:%lld, nTotalMultiple:%lld",
		pPlayer->GetUID(), lJettonScore, lWinScore, nTotalMultiple);

	return false;
}


bool	CGameSlotTable::ProgressControlPalyer(CGamePlayer *pPlayer,uint32 nline, uint32 nbet, uint32 mGamePic[])
{
	if (pPlayer->IsRobot() == true)
	{
		return false;
	}
	bool bIsFalgControlMulti = false;
	tagControlMultiPalyer ControlMultiPalyer;
	auto it_player = m_mpControlMultiPalyer.find(pPlayer->GetUID());
	if (it_player != m_mpControlMultiPalyer.end())
	{
		ControlMultiPalyer = it_player->second;

		if (ControlMultiPalyer.type == GAME_CONTROL_MULTI_LOST)
		{
			bIsFalgControlMulti = SetControlPalyerLost(nline, mGamePic);
		}
		else if (ControlMultiPalyer.type == GAME_CONTROL_MULTI_WIN)
		{
			int64 winScore = 0;
			bIsFalgControlMulti = SetControlPalyerWin(pPlayer,nline, nbet, mGamePic, winScore);
		}
		else
		{
			bIsFalgControlMulti = SetControlPalyerLost(nline, mGamePic);
		}
	}

	LOG_DEBUG("uid:%d,nline:%d,bIsFalgControlMulti:%d,type:%d,ctime:%lld,stime:%lld,etime:%lld",
		pPlayer->GetUID(), nline, bIsFalgControlMulti, ControlMultiPalyer.type, ControlMultiPalyer.ctime, ControlMultiPalyer.stime, ControlMultiPalyer.etime);

	if (bIsFalgControlMulti)
	{
		return bIsFalgControlMulti;
	}

	bool bIsFalgControl = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;

	if (control_uid == pPlayer->GetUID() && control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		if (control_type == GAME_CONTROL_WIN)
		{
			int64 winScore = 0;
			bIsFalgControl = SetControlPalyerWin(pPlayer, nline, nbet, mGamePic, winScore);
		}
		if (control_type == GAME_CONTROL_LOST)
		{
			bIsFalgControl = SetControlPalyerLost(nline, mGamePic);
		}
		if (bIsFalgControl && m_tagControlPalyer.count>0)
		{
			//m_tagControlPalyer.count--;
			if (m_tagControlPalyer.count == 0)
			{
				m_tagControlPalyer.Init();
			}
			if (m_pHostRoom != NULL)
			{
				m_pHostRoom->SynControlPlayer(GetTableID(), m_tagControlPalyer.uid, -1, m_tagControlPalyer.type);
			}
		}
	}
	
	LOG_DEBUG("uid:%d,nline:%d,bIsFalgControl:%d,control_type:%d,game_count:%d,control_uid:%d",
		pPlayer->GetUID(), nline, bIsFalgControl, control_type, game_count, control_uid);

	return bIsFalgControl;

}

void CGameSlotTable::SendMasterPlayerInfo(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		return;
	}
	bool bIsMaster = GetIsMasterUser(pPlayer);
	if (bIsMaster)
	{
		net::msg_update_slot_master_show_info_rep rep;
		rep.set_type(1);
		map<uint32, CGamePlayer*>::iterator iter_loop = m_mpLookers.begin();
		for (; iter_loop != m_mpLookers.end(); ++iter_loop)
		{
			CGamePlayer* pTagPlayer = iter_loop->second;
			if (pTagPlayer == NULL || pTagPlayer->IsRobot())
			{
				continue;
			}
			//if (GetIsMasterUser(pPlayer))
			//{
			//	continue;
			//}
			net::msg_update_slot_master_show_info * pInfo = rep.add_info();

			tagSlotMasterShowPlayerInfo tagInfo;
			auto iter_find = m_mpMasterPlayerInfo.find(pTagPlayer->GetUID());
			if (iter_find != m_mpMasterPlayerInfo.end())
			{
				tagInfo = iter_find->second;
			}
			pInfo->set_uid(pTagPlayer->GetUID());
			pInfo->set_nickname(pTagPlayer->GetNickName());
			pInfo->set_attac_line(tagInfo.attac_line);
			pInfo->set_attac_score(tagInfo.attac_score);
			pInfo->set_lost_count(tagInfo.lost_count);
			pInfo->set_win_count(tagInfo.win_count);
			pInfo->set_cur_score(GetPlayerCurScore(pTagPlayer));
			pInfo->set_ismaster(GetIsMasterUser(pTagPlayer));
		}
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_SLOT_UPDATE_MASTER_SHOW_INFO_REP);
	}
}

void CGameSlotTable::SendMasterShowCtrlInfo(CGamePlayer * pPlayer, tagSlotMasterCtrlInfo tagInfo)
{
	if (pPlayer == NULL)
	{
		return;
	}
	net::msg_show_slot_master_ctrl_info_rep rep;
	rep.set_suid(tagInfo.suid);
	for (int i = 0; i < iMaxMasterRandProCount; i++)
	{
		rep.add_rand_pro(m_iArrMasterRandProCount[i]);
	}
	net::msg_send_slot_master_ctrl_info * pctrl_info = rep.mutable_info();
	pctrl_info->set_uid(tagInfo.uid);
	pctrl_info->set_min_multiple(tagInfo.lMinMultiple);
	pctrl_info->set_max_multiple(tagInfo.lMaxMultiple);
	pctrl_info->set_ran_min_multiple(tagInfo.lRanMinMultiple);
	pctrl_info->set_ran_max_multiple(tagInfo.lRanMaxMultiple);

	pctrl_info->set_all_lost_count(tagInfo.iAllLostCount);
	pctrl_info->set_lost_count(tagInfo.iLostCount);
	pctrl_info->set_all_win_count(tagInfo.iAllWinCount);
	pctrl_info->set_win_count(tagInfo.iWinCount);
	pctrl_info->set_free_spin(tagInfo.iFreeSpin);
	pctrl_info->set_jackpot_index(tagInfo.jackpotIndex);
	pctrl_info->set_all_pre_count(tagInfo.iAllPreCount);
	pctrl_info->set_pre_count(tagInfo.iPreCount);

	LOG_DEBUG("roomid:%d,tableid:%d,suid:%d,tuid:%d,lMultiple:%lld - %lld,:%lld - %lld,iLostCount:%d - %d,iWinCount:%d - %d,iFreeSpin:%d,jackpotIndex:%lld,iPreCount:%d - %d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), tagInfo.uid, tagInfo.lMinMultiple, tagInfo.lMaxMultiple, tagInfo.lRanMinMultiple, tagInfo.lRanMaxMultiple, tagInfo.iAllLostCount, tagInfo.iLostCount, tagInfo.iWinCount, tagInfo.iWinCount, tagInfo.iFreeSpin, tagInfo.jackpotIndex, tagInfo.iAllPreCount, tagInfo.iPreCount);

	SendAllMasterUser(&rep, net::S2C_MSG_SLOT_SHOW_MASTER_CTRL_INFO_REP);
}

void CGameSlotTable::SendAllMasterUser(const google::protobuf::Message* msg, uint32 msg_type)
{
	map<uint32, CGamePlayer*>::iterator iter_loop_Master = m_mpLookers.begin();
	for (; iter_loop_Master != m_mpLookers.end(); ++iter_loop_Master)
	{
		CGamePlayer* pTagPlayer = iter_loop_Master->second;
		if (pTagPlayer == NULL || pTagPlayer->IsRobot())
		{
			continue;
		}
		auto iter_find_vec = std::find(m_vecMasterList.begin(), m_vecMasterList.end(), pTagPlayer->GetUID());
		if (iter_find_vec == m_vecMasterList.end())
		{
			continue;
		}
		if (GetIsMasterUser(pTagPlayer))
		{
			LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,msg_type:%d", GetRoomID(), GetTableID(), pTagPlayer->GetUID(), msg_type);
			pTagPlayer->SendMsgToClient(msg, msg_type);
		}
	}
}

void CGameSlotTable::RefreshAllMasterPlayerUser(uint32 type, CGamePlayer* pPlayer)
{
	if (pPlayer == NULL)
	{
		return;
	}
	if (pPlayer->IsRobot())
	{
		return;
	}
	net::msg_update_slot_master_show_info_rep rep;
	rep.set_type(type);
	net::msg_update_slot_master_show_info * pInfo = rep.add_info();
	tagSlotMasterShowPlayerInfo tagInfo;
	auto iter_find_send = m_mpMasterPlayerInfo.find(pPlayer->GetUID());
	if (iter_find_send != m_mpMasterPlayerInfo.end())
	{
		tagInfo = iter_find_send->second;
	}
	pInfo->set_uid(pPlayer->GetUID());
	pInfo->set_nickname(pPlayer->GetNickName());
	pInfo->set_attac_line(tagInfo.attac_line);
	pInfo->set_attac_score(tagInfo.attac_score);
	pInfo->set_lost_count(tagInfo.lost_count);
	pInfo->set_win_count(tagInfo.win_count);
	pInfo->set_cur_score(GetPlayerCurScore(pPlayer));
	pInfo->set_ismaster(GetIsMasterUser(pPlayer));
	SendAllMasterUser(&rep, net::S2C_MSG_SLOT_UPDATE_MASTER_SHOW_INFO_REP);
}

void CGameSlotTable::GetRandMutail(tagSlotMasterCtrlInfo & tempInfo)
{
	string strArrRandPro;
	int iSumValue = 0;
	int iRandValue = 1;
	int iProIndex = 0;
	int iArrRandPro[iMaxMasterRandProCount] = { 50,30,10,5,2,2,1 };
	tagSlotMasterCtrlInfo tagArrCtrlPro[iMaxMasterRandProCount];
	tagArrCtrlPro[0].lRanMinMultiple = 1;	tagArrCtrlPro[0].lRanMaxMultiple = 2;
	tagArrCtrlPro[1].lRanMinMultiple = 2;	tagArrCtrlPro[1].lRanMaxMultiple = 4;
	tagArrCtrlPro[2].lRanMinMultiple = 5;	tagArrCtrlPro[2].lRanMaxMultiple = 10;
	tagArrCtrlPro[3].lRanMinMultiple = 11;	tagArrCtrlPro[3].lRanMaxMultiple = 20;
	tagArrCtrlPro[4].lRanMinMultiple = 21;	tagArrCtrlPro[4].lRanMaxMultiple = 29;
	tagArrCtrlPro[5].lRanMinMultiple = 29;	tagArrCtrlPro[5].lRanMaxMultiple = 39;
	tagArrCtrlPro[6].lRanMinMultiple = 39;	tagArrCtrlPro[6].lRanMaxMultiple = 50;
	for (int i = 0; i < iMaxMasterRandProCount; i++)
	{
		strArrRandPro += CStringUtility::FormatToString("i:%d,p:%d ", i, m_iArrMasterRandProCount[i]);
		iArrRandPro[i] = m_iArrMasterRandProCount[i];
		iSumValue += iArrRandPro[i];
	}

	iProIndex = 0;
	if (iSumValue > 0)
	{
		iRandValue = g_RandGen.GetRandi(1, iSumValue);
		for (; iProIndex < iMaxMasterRandProCount; iProIndex++)
		{
			if (iArrRandPro[iProIndex] == 0)
			{
				continue;
			}
			if (iRandValue <= iArrRandPro[iProIndex])
			{
				break;
			}
			else
			{
				iRandValue -= iArrRandPro[iProIndex];
			}
		}
	}
	if (iProIndex >= iMaxMasterRandProCount)
	{
		iProIndex = 0;
	}
	tempInfo.lRanMinMultiple = tagArrCtrlPro[iProIndex].lRanMinMultiple;
	tempInfo.lRanMaxMultiple = tagArrCtrlPro[iProIndex].lRanMaxMultiple;

	LOG_DEBUG("roomid:%d,tableid:%d,strArrRandPro:%s,iSumValue:%d,iRandValue:%d,iProIndex:%d,lMultiple:%lld - %lld,%lld",
		GetRoomID(), GetTableID(), strArrRandPro.c_str(), iSumValue, iRandValue, iProIndex, tempInfo.lRanMinMultiple, tempInfo.lRanMaxMultiple);

}

bool CGameSlotTable::SetControlPalyerFreeCount(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[], int freeCount, int64 lMinWinMultiple, int64 lMaxWinMultiple)
{
	int64 nTotalMultiple = 0;
	int64 lJettonScore = nbet * nline;
	int64 lWinScore = 0;
	int iCaclFreeCount = 0;
	int iCount = 1024;
	int iIndex = 0;

	uint32 mTempCaclGamePic[MAXROW][MAXCOLUMN] = { 0 };

	for (; iIndex < iCount; iIndex++)
	{
		uint32 mCaclGamePic[MAXROW][MAXCOLUMN] = { 0 };
		if (freeCount == 0)
		{
			//m_gameLogic.GetRandPicArray(mCaclGamePic);
			if (nline == 1)
			{
				for (uint32 j = 0; j < MAXCOLUMN; j++)
				{
					for (uint32 k = 0; k < MAXROW; k++)
					{
						uint32 nIndex = m_gameLogic.GetMultipleSpinWeightIndex(j, nline, lMinWinMultiple, lMaxWinMultiple);
						mCaclGamePic[k][j] = nIndex + 1;
					}
				}
			}
			else
			{
				m_gameLogic.GetMultiplePicArray(nline, lMinWinMultiple, lMaxWinMultiple, mCaclGamePic);
			}
		}
		else
		{
			// 生成有免费次数的图片集
			for (uint32 j = 0; j < MAXCOLUMN; j++)
			{
				for (uint32 k = 0; k < MAXROW; k++)
				{
					uint32 nIndex = m_gameLogic.GetFreeSpinWeightIndex(j);
					mCaclGamePic[k][j] = nIndex + 1;
				}
			}
		}

		nTotalMultiple = 0;
		lWinScore = 0;
		iCaclFreeCount = 0;
		for (uint32 i = 0; i < nline; i++)
		{
			uint32 linePic[MAXCOLUMN] = { 0 };
			for (uint32 k = 0; k < MAXCOLUMN; k++)
			{
				linePic[k] = mCaclGamePic[(m_oServerLinePos[i].pos[k] - 1) % MAXROW][(m_oServerLinePos[i].pos[k] - 1) / MAXROW];
			}
			picLinkinfo pLink;
			pLink.lineNo = i + 1;//线号
			int64 nMultipleTmp = GetLineMultiple(linePic, pLink);
			if (nMultipleTmp > 0 && pLink.picID != PIC_BONUS)
			{
				nTotalMultiple += nMultipleTmp;
			}
			//计算bonus-奖励
			if (pLink.picID == PIC_BONUS && pLink.linkNum <= MAXCOLUMN)
			{
				iCaclFreeCount += s_FreeTimes[pLink.linkNum];
			}

			//LOG_DEBUG("slot_image_linePic - uid:%d,iCaclFreeCount:%d,nTotalMultiple:%lld,nMultipleTmp:%lld,line:%d,[%02d %02d %02d %02d %02d]", pPlayer->GetUID(), iCaclFreeCount, nTotalMultiple, nMultipleTmp,i, linePic[0], linePic[1], linePic[2], linePic[3], linePic[4]);
		}

		lWinScore = nTotalMultiple * nbet;
		bool bIsSuccessMultiple = false;
		//if (lMinWinMultiple == 0 && lMaxWinMultiple == 0)
		//{
		//	bIsSuccessMultiple = true;
		//}
		if (lMinWinMultiple > 0 && lMaxWinMultiple > 0 && nTotalMultiple > 0 && nTotalMultiple >= lMinWinMultiple && nTotalMultiple <= lMaxWinMultiple)
		{
			bIsSuccessMultiple = true;
		}

		//LOG_DEBUG("slot_image_ roomid:%d,tableid:%d ---------------------------------------------------------------------", GetRoomID(), GetTableID());
		//LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mCaclGamePic[0][0], mCaclGamePic[0][1], mCaclGamePic[0][2], mCaclGamePic[0][3], mCaclGamePic[0][4]);
		//LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mCaclGamePic[1][0], mCaclGamePic[1][1], mCaclGamePic[1][2], mCaclGamePic[1][3], mCaclGamePic[1][4]);
		//LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mCaclGamePic[2][0], mCaclGamePic[2][1], mCaclGamePic[2][2], mCaclGamePic[2][3], mCaclGamePic[2][4]);

		//LOG_DEBUG("1_ctrl_free - roomid:%d,tableid:%d,iIndex:%d,uid:%d,nline:%d,nbet:%d,lJettonScore:%lld, lWinScore:%lld, nTotalMultiple:%lld,lMinWinMultiple:%lld,lMaxWinMultiple:%lld,freeCount:%d,iCaclFreeCount:%d,",
		//	GetRoomID(), GetTableID(), iIndex, pPlayer->GetUID(), nline, nbet, lJettonScore, lWinScore, nTotalMultiple, lMinWinMultiple, lMaxWinMultiple, freeCount, iCaclFreeCount);

		if (freeCount != 0)
		{
			//if (iCaclFreeCount == freeCount && bIsSuccessMultiple && lWinScore > lJettonScore)
			if (iCaclFreeCount == freeCount)
			{
				memcpy(mGamePic, mCaclGamePic, MAXROW * MAXCOLUMN * 4);

				LOG_DEBUG("slot_image_ roomid:%d,tableid:%d ---------------------------------------------------------------------", GetRoomID(), GetTableID());
				LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mCaclGamePic[0][0], mCaclGamePic[0][1], mCaclGamePic[0][2], mCaclGamePic[0][3], mCaclGamePic[0][4]);
				LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mCaclGamePic[1][0], mCaclGamePic[1][1], mCaclGamePic[1][2], mCaclGamePic[1][3], mCaclGamePic[1][4]);
				LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mCaclGamePic[2][0], mCaclGamePic[2][1], mCaclGamePic[2][2], mCaclGamePic[2][3], mCaclGamePic[2][4]);

				LOG_DEBUG("2_ctrl_free - roomid:%d,tableid:%d,iIndex:%d,uid:%d,nline:%d,nbet:%d,lJettonScore:%lld, lWinScore:%lld, nTotalMultiple:%lld,lMinWinMultiple:%lld,lMaxWinMultiple:%lld,freeCount:%d,iCaclFreeCount:%d,",
					GetRoomID(), GetTableID(), iIndex, pPlayer->GetUID(), nline, nbet, lJettonScore, lWinScore, nTotalMultiple, lMinWinMultiple, lMaxWinMultiple, freeCount, iCaclFreeCount);

				return true;
			}
		}
		else
		{
			lWinScore = nTotalMultiple * nbet;
			//if (bIsSuccessMultiple && lWinScore > lJettonScore)

			if (bIsSuccessMultiple)
			{
				memcpy(mGamePic, mCaclGamePic, MAXROW * MAXCOLUMN * 4);

				LOG_DEBUG("slot_image_ roomid:%d,tableid:%d ---------------------------------------------------------------------", GetRoomID(), GetTableID());
				LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mCaclGamePic[0][0], mCaclGamePic[0][1], mCaclGamePic[0][2], mCaclGamePic[0][3], mCaclGamePic[0][4]);
				LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mCaclGamePic[1][0], mCaclGamePic[1][1], mCaclGamePic[1][2], mCaclGamePic[1][3], mCaclGamePic[1][4]);
				LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mCaclGamePic[2][0], mCaclGamePic[2][1], mCaclGamePic[2][2], mCaclGamePic[2][3], mCaclGamePic[2][4]);

				LOG_DEBUG("3_ctrl_free - roomid:%d,tableid:%d,iIndex:%d,uid:%d,nline:%d,nbet:%d,lJettonScore:%lld, lWinScore:%lld, nTotalMultiple:%lld,lMinWinMultiple:%lld,lMaxWinMultiple:%lld,freeCount:%d,iCaclFreeCount:%d,",
					GetRoomID(), GetTableID(), iIndex, pPlayer->GetUID(), nline, nbet, lJettonScore, lWinScore, nTotalMultiple, lMinWinMultiple, lMaxWinMultiple, freeCount, iCaclFreeCount);

				return true;
			}
		}

		memcpy(mTempCaclGamePic, mCaclGamePic, MAXROW * MAXCOLUMN * 4);
	}

	LOG_DEBUG("slot_image_ roomid:%d,tableid:%d ---------------------------------------------------------------------", GetRoomID(), GetTableID());
	LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mTempCaclGamePic[0][0], mTempCaclGamePic[0][1], mTempCaclGamePic[0][2], mTempCaclGamePic[0][3], mTempCaclGamePic[0][4]);
	LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mTempCaclGamePic[1][0], mTempCaclGamePic[1][1], mTempCaclGamePic[1][2], mTempCaclGamePic[1][3], mTempCaclGamePic[1][4]);
	LOG_DEBUG("slot_image_mGamePic - roomid:%d,tableid:%d,uid:%d,[%02d %02d %02d %02d %02d]", GetRoomID(), GetTableID(), pPlayer->GetUID(), mTempCaclGamePic[2][0], mTempCaclGamePic[2][1], mTempCaclGamePic[2][2], mTempCaclGamePic[2][3], mTempCaclGamePic[2][4]);

	LOG_DEBUG("4_ctrl_free - roomid:%d,tableid:%d,iIndex:%d,uid:%d,nline:%d,nbet:%d,lJettonScore:%lld, lWinScore:%lld, nTotalMultiple:%lld,lMinWinMultiple:%lld,lMaxWinMultiple:%lld,freeCount:%d,iCaclFreeCount:%d,",
		GetRoomID(), GetTableID(), iIndex, pPlayer->GetUID(), nline, nbet, lJettonScore, lWinScore, nTotalMultiple, lMinWinMultiple, lMaxWinMultiple, freeCount, iCaclFreeCount);

	return false;
}

bool CGameSlotTable::ProgressMasterCtrl(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[], int & out_hitIndex)
{
	if (pPlayer->IsRobot())
	{
		return false;
	}
	out_hitIndex = 255;
	bool bIsMasterCtrl = false;
	int iLoopIndex = 0;
	int iLoopCount = 1;
	auto iter_find = m_mpMasterCtrlInfo.find(pPlayer->GetUID());
	tagSlotMasterCtrlInfo tagTempInfo;
	if (iter_find != m_mpMasterCtrlInfo.end())
	{
		tagSlotMasterCtrlInfo & tempInfo = iter_find->second;
		tagTempInfo = tempInfo;
		if (tempInfo.iPreCount > 0)
		{
			if (tempInfo.iLostCount <= 0 && tempInfo.iWinCount > 0)
			{
				tagSlotMasterCtrlInfo tempTagInfo;
				GetRandMutail(tempTagInfo);
				bool bIsWinCount = false;
				for (iLoopIndex = 0; iLoopIndex < iLoopCount; iLoopIndex++)
				{
					bool bIsControlPalyerFreeCount = SetControlPalyerFreeCount(pPlayer, nline, nbet, mGamePic, 0, tempTagInfo.lRanMinMultiple, tempTagInfo.lRanMaxMultiple);
					if (bIsControlPalyerFreeCount)
					{
						bIsWinCount = true;
						break;
					}
				}
				if (bIsWinCount)
				{
					bIsMasterCtrl = true;
					tempInfo.iWinCount--;
					if (tempInfo.iWinCount <= 0)
					{
						tempInfo.iPreCount--;
						if (tempInfo.iPreCount <= 0)
						{
							//m_mpMasterCtrlInfo.erase(iter_find);
						}
						else
						{
							tempInfo.iWinCount = tempInfo.iAllWinCount;
						}
					}
				}
			}
			else if (tempInfo.iLostCount > 0 && tempInfo.iWinCount <= 0)
			{
				bool bIsControlPalyerLost = SetControlPalyerLost(nline, mGamePic);
				if (bIsControlPalyerLost)
				{
					bIsMasterCtrl = true;
					tempInfo.iLostCount--;
					if (tempInfo.iLostCount <= 0)
					{
						tempInfo.iPreCount--;
						if (tempInfo.iPreCount <= 0)
						{
							//m_mpMasterCtrlInfo.erase(iter_find);
						}
						else
						{
							tempInfo.iLostCount = tempInfo.iAllLostCount;
						}
					}
				}
			}
			else if (tempInfo.iLostCount == 0 && tempInfo.iWinCount == 0)
			{
				bool bIsFreeSpin = false;
				bool bIsMultiple = false;
				if (tempInfo.iFreeSpin == 3 || tempInfo.iFreeSpin == 7 || tempInfo.iFreeSpin == 12)
				{
					for (iLoopIndex = 0; iLoopIndex < iLoopCount; iLoopIndex++)
					{
						bool bIsControlPalyerFreeCount = SetControlPalyerFreeCount(pPlayer, nline, nbet, mGamePic, tempInfo.iFreeSpin, tempInfo.lRanMinMultiple, tempInfo.lRanMaxMultiple);
						if (bIsControlPalyerFreeCount)
						{
							bIsFreeSpin = true;
							break;
						}
					}
				}
				else if (tempInfo.lMinMultiple != 0, tempInfo.lMaxMultiple != 0)
				{
					for (iLoopIndex = 0; iLoopIndex < iLoopCount; iLoopIndex++)
					{
						bool bIsControlPalyerFreeCount = SetControlPalyerFreeCount(pPlayer, nline, nbet, mGamePic, tempInfo.iFreeSpin, tempInfo.lMinMultiple, tempInfo.lMaxMultiple);
						if (bIsControlPalyerFreeCount)
						{
							bIsMultiple = true;
							break;
						}
					}
				}
				if (bIsFreeSpin || bIsMultiple)
				{
					bIsMasterCtrl = true;
					tempInfo.iPreCount--;
					if (tempInfo.iPreCount <= 0)
					{
						tempInfo.iFreeSpin = 0;
					}
				}
			}
			else if (tempInfo.iLostCount < 0 && tempInfo.iWinCount < 0)
			{

			}
			else
			{

			}
			if (tempInfo.iLostCount < 0 || tempInfo.iWinCount < 0 || tempInfo.iPreCount <= 0)
			{
				m_mpMasterCtrlInfo.erase(iter_find);
			}
		}
		else
		{
			m_mpMasterCtrlInfo.erase(iter_find);
		}

		LOG_DEBUG("1_ctrl_info - roomid:%d,tableid:%d,uid:%d,bIsMasterCtrl:%d,tuid:%d,lMultiple:%lld - %lld,%lld - %lld,iLostCount:%d - %d,iWinCount:%d - %d,iFreeSpin:%d,jackpotIndex:%lld,iPreCount:%d - %d",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), bIsMasterCtrl, tempInfo.uid, tempInfo.lMinMultiple, tempInfo.lMaxMultiple, tempInfo.lRanMinMultiple, tempInfo.lRanMaxMultiple, tempInfo.iAllLostCount, tempInfo.iLostCount, tempInfo.iAllWinCount, tempInfo.iWinCount, tempInfo.iFreeSpin, tempInfo.jackpotIndex, tempInfo.iAllPreCount, tempInfo.iPreCount);

	}

	LOG_DEBUG("2_ctrl_info - roomid:%d,tableid:%d,uid:%d,bIsMasterCtrl:%d,tuid:%d,lMultiple:%lld - %lld,%lld - %lld,iLostCount:%d - %d,iWinCount:%d - %d,iFreeSpin:%d,jackpotIndex:%lld,iPreCount:%d - %d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), bIsMasterCtrl, tagTempInfo.uid, tagTempInfo.lMinMultiple, tagTempInfo.lMaxMultiple, tagTempInfo.lRanMinMultiple, tagTempInfo.lRanMaxMultiple, tagTempInfo.iAllLostCount, tagTempInfo.iLostCount, tagTempInfo.iAllWinCount, tagTempInfo.iWinCount, tagTempInfo.iFreeSpin, tagTempInfo.jackpotIndex, tagTempInfo.iAllPreCount, tagTempInfo.iPreCount);

	if (bIsMasterCtrl)
	{
		tagSlotMasterCtrlInfo tagInfo;
		int iResultCode = 0;
		auto iter_find = m_mpMasterCtrlInfo.find(pPlayer->GetUID());
		if (iter_find != m_mpMasterCtrlInfo.end())
		{
			iResultCode = 1;
			tagInfo = iter_find->second;
		}
		else
		{
			iResultCode = 2;
			tagInfo.uid = pPlayer->GetUID();
		}



		LOG_DEBUG("3_ctrl_info roomid:%d,tableid:%d,uid:%d,iResultCode:%d,tuid:%d,lMultiple:%lld - %lld,%lld - %lld,iLostCount:%d - %d,iWinCount:%d - %d,iFreeSpin:%d,jackpotIndex:%lld,iPreCount:%d - %d",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), iResultCode, tagInfo.uid, tagInfo.lMinMultiple, tagInfo.lMaxMultiple, tagInfo.lRanMinMultiple, tagInfo.lRanMaxMultiple, tagInfo.iAllLostCount, tagInfo.iLostCount, tagInfo.iAllWinCount, tagInfo.iWinCount, tagInfo.iFreeSpin, tagInfo.jackpotIndex, tagInfo.iAllPreCount, tagInfo.iPreCount);

		map<uint32, CGamePlayer*>::iterator iter_loop_Master = m_mpLookers.begin();
		for (; iter_loop_Master != m_mpLookers.end(); ++iter_loop_Master)
		{
			CGamePlayer* pTagPlayer = iter_loop_Master->second;
			if (pTagPlayer == NULL || pTagPlayer->IsRobot())
			{
				continue;
			}
			auto iter_find_vec = std::find(m_vecMasterList.begin(), m_vecMasterList.end(), pTagPlayer->GetUID());
			if (iter_find_vec == m_vecMasterList.end())
			{
				continue;
			}
			if (GetIsMasterUser(pTagPlayer))
			{
				SendMasterShowCtrlInfo(pTagPlayer, tagInfo);
			}
		}
	}
	return bIsMasterCtrl;
}

void CGameSlotTable::TatalLineUserBet(CGamePlayer* pPlayer, uint32 nline, uint32 nbet, int64 lWinScore)
{
	if (pPlayer == NULL)
	{
		return;
	}

	if (pPlayer->IsRobot() == false)
	{
		auto iter_find = m_mpMasterPlayerInfo.find(pPlayer->GetUID());
		if (iter_find != m_mpMasterPlayerInfo.end())
		{
			tagSlotMasterShowPlayerInfo & tagInfo = iter_find->second;
			tagInfo.attac_line = nline;
			tagInfo.attac_score = nbet;
			if (lWinScore > 0)
			{
				tagInfo.win_count++;
			}
			else
			{
				tagInfo.lost_count++;
			}
		}
		else
		{
			tagSlotMasterShowPlayerInfo tagInfo;
			tagInfo.uid = pPlayer->GetUID();
			tagInfo.attac_line = nline;
			tagInfo.attac_score = nbet;
			if (lWinScore > 0)
			{
				tagInfo.win_count++;
			}
			else
			{
				tagInfo.lost_count++;
			}
			m_mpMasterPlayerInfo.insert(make_pair(tagInfo.uid, tagInfo));
		}
		//
		net::msg_update_slot_master_show_info_rep rep;
		rep.set_type(0);
		net::msg_update_slot_master_show_info * pInfo = rep.add_info();
		tagSlotMasterShowPlayerInfo tagInfo;
		auto iter_find_send = m_mpMasterPlayerInfo.find(pPlayer->GetUID());
		if (iter_find_send != m_mpMasterPlayerInfo.end())
		{
			tagInfo = iter_find_send->second;
		}
		pInfo->set_uid(pPlayer->GetUID());
		pInfo->set_nickname(pPlayer->GetNickName());
		pInfo->set_attac_line(tagInfo.attac_line);
		pInfo->set_attac_score(tagInfo.attac_score);
		pInfo->set_lost_count(tagInfo.lost_count);
		pInfo->set_win_count(tagInfo.win_count);
		pInfo->set_cur_score(GetPlayerCurScore(pPlayer));
		pInfo->set_ismaster(GetIsMasterUser(pPlayer));
		SendAllMasterUser(&rep, net::S2C_MSG_SLOT_UPDATE_MASTER_SHOW_INFO_REP);
	}
}

// 获取一次随机图组赢分  add by har
int64 CGameSlotTable::GetRandomWinScore(uint32 nline, uint32 nbet, uint32 mGamePic[]) {
	int64 nTotalMultiple = 0;

	uint32 mCaclGamePic[MAXROW][MAXCOLUMN] = { 0 };
	m_gameLogic.GetRandPicArray(mCaclGamePic);
	for (uint32 i = 0; i < nline; ++i) {
		uint32 linePic[MAXCOLUMN] = { 0 };
		for (uint32 k = 0; k < MAXCOLUMN; ++k)
			linePic[k] = mCaclGamePic[(m_oServerLinePos[i].pos[k] - 1) % MAXROW][(m_oServerLinePos[i].pos[k] - 1) / MAXROW];

		picLinkinfo pLink;
		pLink.lineNo = i + 1;//线号
		int64 nMultipleTmp = GetLineMultiple(linePic, pLink);
		if (nMultipleTmp > 0 && pLink.picID != PIC_BONUS)
			nTotalMultiple += nMultipleTmp;
	}
	memcpy(mGamePic, mCaclGamePic, MAXROW * MAXCOLUMN * 4);
	return nTotalMultiple * nbet;
}

// 设置库存输赢  add by har
bool CGameSlotTable::SetStockWinLose(CGamePlayer *pPlayer, uint32 nline, uint32 nbet, uint32 mGamePic[]) {
	if (pPlayer->IsRobot())
		return false;

	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	int64 betScore = nline * nbet;
	int64 winScore = 0;
	if (stockChange == -1) {
		for (int i = 0; i < 1000; ++i) {
			winScore = GetRandomWinScore(nline, nbet, mGamePic);
			if (winScore - betScore < 1) {
				LOG_DEBUG("SetStockWinLose suc1 roomid:%d,tableid:%d,stockChange:%lld,uid:%d,nline:%d,nbet:%d,winScore:%lld,betScore:%lld,i:%d",
					GetRoomID(), GetTableID(), stockChange, pPlayer->GetUID(), nline, nbet, winScore, betScore, i);
				return true;
			}
		}
		LOG_ERROR("SetStockWinLose fail1 roomid:%d,tableid:%d,stockChange:%lld,uid:%d,nline:%d,nbet:%d,winScore:%lld,betScore:%lld",
			GetRoomID(), GetTableID(), stockChange, pPlayer->GetUID(), nline, nbet, winScore, betScore);
		return false;
	}

	if (stockChange == -2)
		for (int i = 0; i < 1000; ++i) {
			SetControlPalyerWin(pPlayer, nline, nbet, mGamePic, winScore);
			if (winScore - betScore > -1) {
				LOG_DEBUG("SetStockWinLose suc2 roomid:%d,tableid:%d,stockChange:%lld,uid:%d,nline:%d,nbet:%d,i:%d,winScore:%lld,betScore:%lld",
					GetRoomID(), GetTableID(), stockChange, pPlayer->GetUID(), nline, nbet, i, winScore, betScore);
				return true;
			}
		}
	else {
		if (stockChange < 3 * betScore) {
			LOG_ERROR("SetStockWinLose fail12 roomid:%d,tableid:%d,stockChange:%lld,uid:%d,nline:%d,nbet:%d,betScore:%lld",
				GetRoomID(), GetTableID(), stockChange, pPlayer->GetUID(), nline, nbet, betScore);
			return false;
		}

		for (int i = 0; i < 1000; ++i) {
			SetControlPalyerWin(pPlayer, nline, nbet, mGamePic, winScore);
			int64 realWinScore = winScore - betScore;
			if (realWinScore > -1 && realWinScore <= stockChange) {
				LOG_DEBUG("SetStockWinLose suc3 roomid:%d,tableid:%d,stockChange:%lld,uid:%d,nline:%d,nbet:%d,i:%d,winScore:%lld,betScore:%lld,realWinScore:%lld",
					GetRoomID(), GetTableID(), stockChange, pPlayer->GetUID(), nline, nbet, i, winScore, betScore, realWinScore);
				return true;
			}
		}
	}

	LOG_ERROR("SetStockWinLose fail3 roomid:%d,tableid:%d,stockChange:%lld,uid:%d,nline:%d,nbet:%d,winScore:%lld,betScore:%lld",
		GetRoomID(), GetTableID(), stockChange, pPlayer->GetUID(), nline, nbet, winScore, betScore);
	return false;
}
