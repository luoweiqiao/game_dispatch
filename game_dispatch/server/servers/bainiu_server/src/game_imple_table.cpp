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
#include "robot_oper_mgr.h"

using namespace std;
using namespace svrlib;
using namespace net;
using namespace game_niuniu;

namespace
{
    const static uint32 s_FreeTime              = 3*1000;       // ����ʱ��
    const static uint32 s_PlaceJettonTime       = 10*1000;       // ��עʱ��
    const static uint32 s_DispatchTime          = 17*1000;      // ����ʱ��

	//const static uint32 s_SysLostWinProChange = 500;			// �Ա��±Ҹ��ʱ仯
	//const static int64  s_UpdateJackpotScore = 100000;			// ���½��ط���


    
};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // ��ţ
        {
            pTable = new CGameBaiNiuTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // ������ţ
        {
            pTable = new CGameBaiNiuTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // ˽�˷���ţ
        {
            pTable = new CGameBaiNiuTable(this,tableID,emTABLE_TYPE_PLAYER);
        }break;
    default:
        {
            assert(false);
            return NULL;
        }break;
    }
    return pTable;
}
// �����Ϸ����
CGameBaiNiuTable::CGameBaiNiuTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();

	//����ע��
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
    memset(m_playerJettonScore,0,sizeof(m_playerJettonScore));


	//������ע
	for(uint8 i=0;i<AREA_COUNT;++i){
        m_userJettonScore[i].clear();
    }
	//��ҳɼ�	
	m_mpUserWinScore.clear();
	//�˿���Ϣ
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));

	//ׯ����Ϣ
	m_pCurBanker            = NULL;
	m_wBankerTime           = 0;
	m_lBankerWinScore       = 0L;		

    m_robotBankerWinPro     = 0;
	m_robotBankerMaxCardPro = 0;
	m_tagNiuMultiple.Init();
	m_iMaxJettonRate = 1;
	m_bIsChairRobotAlreadyJetton = false;
	m_bIsRobotAlreadyJetton = false;
	m_chairRobotPlaceJetton.clear();
	m_RobotPlaceJetton.clear();
	m_tagControlPalyer.Init();
	m_lGameCount = 0;
	m_uBairenTotalCount = 0;
	m_vecAreaWinCount.clear();
	m_vecAreaLostCount.clear();
    return;
}
CGameBaiNiuTable::~CGameBaiNiuTable()
{

}
bool    CGameBaiNiuTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (pPlayer->GetTable() != NULL)
	{
		return false;
	}

    // �޶����
    if(IsFullTable() || GetPlayerCurScore(pPlayer) < GetEnterMin())
	{
        return false;
    }
	//if (pPlayer->IsRobot())
	//{
	//	if (m_pHostRoom != NULL)
	//	{
	//		if (GetPlayerCurScore(pPlayer) <= m_pHostRoom->GetRobotMinScore() || GetPlayerCurScore(pPlayer) >= m_pHostRoom->GetRobotMaxScore())
	//		{
	//			LOG_DEBUG("robot enter score - uid:%d,cur_score:%lld,roomid:%d,enter_score:%lld - %lld", pPlayer->GetUID(), GetPlayerCurScore(pPlayer), m_pHostRoom->GetRoomID(), m_pHostRoom->GetRobotMinScore(), m_pHostRoom->GetRobotMaxScore());

	//			return false;
	//		}
	//	}
	//}
    return true;
}
bool    CGameBaiNiuTable::CanLeaveTable(CGamePlayer* pPlayer)
{
    if(m_pCurBanker == pPlayer || IsSetJetton(pPlayer->GetUID()))
        return false;         
    if(IsInApplyList(pPlayer->GetUID()))
        return false;
    
    return true;
}
bool    CGameBaiNiuTable::CanSitDown(CGamePlayer* pPlayer,uint16 chairID)
{
    
    return CGameTable::CanSitDown(pPlayer,chairID);
}
bool    CGameBaiNiuTable::CanStandUp(CGamePlayer* pPlayer)
{
    return true;        
}    
bool    CGameBaiNiuTable::IsFullTable()
{
    if(m_mpLookers.size() >= 100)
        return true;
    
    return false;
}
void CGameBaiNiuTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::bainiu_table_info* pbainiu = pInfo->mutable_bainiu();
    pbainiu->set_tableid(GetTableID());
    pbainiu->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
        pbainiu->set_is_passwd(1);
    }else{
        pbainiu->set_is_passwd(0);
    }
    pbainiu->set_hostname(m_conf.hostName);
    pbainiu->set_basescore(m_conf.baseScore);
    pbainiu->set_consume(m_conf.consume);
    pbainiu->set_entermin(m_conf.enterMin);
    pbainiu->set_duetime(m_conf.dueTime);
    pbainiu->set_feetype(m_conf.feeType);
    pbainiu->set_feevalue(m_conf.feeValue);
    pbainiu->set_card_time(s_PlaceJettonTime);
    pbainiu->set_table_state(GetGameState());
    pbainiu->set_sitdown(m_pHostRoom->GetSitDown());
    pbainiu->set_apply_banker_condition(GetApplyBankerCondition());
    pbainiu->set_apply_banker_maxscore(GetApplyBankerConditionLimit());
    pbainiu->set_banker_max_time(m_BankerTimeLimit);
	pbainiu->set_max_jetton_rate(m_iMaxJettonRate);

}

//��������
bool CGameBaiNiuTable::Init()
{
	if (m_pHostRoom == NULL) {
		return false;
	}

    SetGameState(net::TABLE_STATE_NIUNIU_FREE);
	InitAreaSize(AREA_COUNT);
    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
    m_BankerTimeLimit = 3;
    m_BankerTimeLimit = CApplication::Instance().call<int>("bainiubankertime");
    //m_robotBankerWinPro = CApplication::Instance().call<int>("bainiubankerwin");
    
    m_robotApplySize = g_RandGen.RandRange(4, 8);//��������������
    m_robotChairSize = g_RandGen.RandRange(2, 4);//��������λ��

	ReAnalysisParam();
	CRobotOperMgr::Instance().PushTable(this);
	//LOG_DEBUG("main_push - ");
	SetMaxChairNum(GAME_PLAYER); // add by har
    return true;
}

bool CGameBaiNiuTable::ReAnalysisParam() {
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - roomid:%d,param:%s", m_pHostRoom->GetRoomID(),param.c_str());
		return true;
	}
	if (jvalue.isMember("n1")) {
		m_tagNiuMultiple.niu1 = jvalue["n1"].asInt();
	}
	if (jvalue.isMember("n2")) {
		m_tagNiuMultiple.niu2 = jvalue["n2"].asInt();
	}
	if (jvalue.isMember("n3")) {
		m_tagNiuMultiple.niu3 = jvalue["n3"].asInt();
	}
	if (jvalue.isMember("n4")) {
		m_tagNiuMultiple.niu4 = jvalue["n4"].asInt();
	}
	if (jvalue.isMember("n5")) {
		m_tagNiuMultiple.niu5 = jvalue["n5"].asInt();
	}
	if (jvalue.isMember("n6")) {
		m_tagNiuMultiple.niu6 = jvalue["n6"].asInt();
	}
	if (jvalue.isMember("n7")) {
		m_tagNiuMultiple.niu7 = jvalue["n7"].asInt();
	}
	if (jvalue.isMember("n8")) {
		m_tagNiuMultiple.niu8 = jvalue["n8"].asInt();
	}
	if (jvalue.isMember("n9")) {
		m_tagNiuMultiple.niu9 = jvalue["n9"].asInt();
	}
	if (jvalue.isMember("nn")) {
		m_tagNiuMultiple.niuniu = jvalue["nn"].asInt();
	}
	if (jvalue.isMember("bn")) {
		m_tagNiuMultiple.big5niu = jvalue["bn"].asInt();
	}
	if (jvalue.isMember("sn")) {
		m_tagNiuMultiple.small5niu = jvalue["sn"].asInt();
	}
	if (jvalue.isMember("bb")) {
		m_tagNiuMultiple.bomebome = jvalue["bb"].asInt();
	}
	if (jvalue.isMember("bbw")) {
		m_robotBankerWinPro = jvalue["bbw"].asInt();
	}
	if (jvalue.isMember("rmc")) {
		m_robotBankerMaxCardPro = jvalue["rmc"].asInt();
	}
	
	if (jvalue.isMember("mt")) {
		m_iMaxJettonRate = jvalue["mt"].asInt();
	}
	//if (jvalue.isMember("aps")) {
	//	m_lMaxPollScore = jvalue["aps"].asInt64();
	//}
	//if (jvalue.isMember("ips")) {
	//	m_lMinPollScore = jvalue["ips"].asInt64();
	//}
	//int iIsUpdateCurPollScore = 0;
	//if (jvalue.isMember("ucp")) {
	//	iIsUpdateCurPollScore = jvalue["ucp"].asInt();
	//}
	//if (iIsUpdateCurPollScore == 1)
	//{
	//	if (jvalue.isMember("cps")) {
	//		m_lCurPollScore = jvalue["cps"].asInt64();
	//		m_lFrontPollScore = m_lCurPollScore;
	//	}
	//}
	//if (jvalue.isMember("swp")) {
	//	m_uSysWinPro = jvalue["swp"].asInt();
	//}
	//if (jvalue.isMember("slp")) {
	//	m_uSysLostPro = jvalue["slp"].asInt();
	//}

	//int64 lDiffMaxScore = m_lCurPollScore - m_lMaxPollScore;
	//int64 lDiffMinScore = m_lMinPollScore - m_lCurPollScore;

	//if (lDiffMaxScore >= s_UpdateJackpotScore) // �������� �±����� �ԱҼ���
	//{
	//	do
	//	{
	//		if (m_uSysLostPro + s_SysLostWinProChange < PRO_DENO_10000)
	//		{
	//			m_uSysLostPro += s_SysLostWinProChange;
	//		}
	//		else if (m_uSysLostPro + s_SysLostWinProChange >= PRO_DENO_10000)
	//		{
	//			m_uSysLostPro = PRO_DENO_10000;
	//		}

	//		if (m_uSysWinPro > s_SysLostWinProChange)
	//		{
	//			m_uSysWinPro -= s_SysLostWinProChange;
	//		}
	//		else if (m_uSysWinPro <= s_SysLostWinProChange)
	//		{
	//			m_uSysWinPro = 0;
	//		}

	//		lDiffMaxScore -= s_UpdateJackpotScore;

	//	} while (lDiffMaxScore >= s_UpdateJackpotScore);
	//}

	//if (lDiffMinScore >= s_UpdateJackpotScore) // ���ؼ��� �Ա����� �±Ҽ���
	//{
	//	do
	//	{
	//		if (m_uSysWinPro + s_SysLostWinProChange < PRO_DENO_10000)
	//		{
	//			m_uSysWinPro += s_SysLostWinProChange;
	//		}
	//		else if (m_uSysWinPro + s_SysLostWinProChange >= PRO_DENO_10000)
	//		{
	//			m_uSysWinPro = PRO_DENO_10000;
	//		}

	//		if (m_uSysLostPro > s_SysLostWinProChange)
	//		{
	//			m_uSysLostPro -= s_SysLostWinProChange;
	//		}
	//		else if (m_uSysLostPro <= s_SysLostWinProChange)
	//		{
	//			m_uSysLostPro = 0;
	//		}

	//		lDiffMinScore -= s_UpdateJackpotScore;

	//	} while (lDiffMaxScore >= s_UpdateJackpotScore);
	//}


	LOG_DEBUG("reader json parse success - roomid:%d,tableid:%d,bomebome:%d,m_robotBankerWinPro:%d,m_robotBankerMaxCardPro:%d,m_iMaxJettonRate:%d", m_pHostRoom->GetRoomID(),GetTableID(),m_tagNiuMultiple.bomebome, m_robotBankerWinPro, m_robotBankerMaxCardPro, m_iMaxJettonRate);
	return true;
}


void CGameBaiNiuTable::ShutDown()
{
    //CalcBankerScore();
}
//��λ����
void CGameBaiNiuTable::ResetTable()
{
    ResetGameData();
}
void CGameBaiNiuTable::OnTimeTick()
{
	OnTableTick();

    uint8 tableState = GetGameState();
    if(m_coolLogic.isTimeOut())
    {
        switch(tableState)
        {
        case TABLE_STATE_NIUNIU_FREE:           // ����
            {                
                if(OnGameStart()){
					//InitChessID();
                    SetGameState(TABLE_STATE_NIUNIU_PLACE_JETTON);     

					m_brc_table_status = emTABLE_STATUS_START;
					m_brc_table_status_time = s_PlaceJettonTime;

					//ͬ��ˢ�°��˳����ƽ��������״̬��Ϣ
					OnBrcControlFlushTableStatus();

                }else{
                    m_coolLogic.beginCooling(s_FreeTime);
                }
            }break;
        case TABLE_STATE_NIUNIU_PLACE_JETTON:   // ��עʱ��
            {
                SetGameState(TABLE_STATE_NIUNIU_GAME_END);
                m_coolLogic.beginCooling(s_DispatchTime);
				DispatchTableCard();
				m_bIsChairRobotAlreadyJetton = false;
				m_bIsRobotAlreadyJetton = false;
                OnGameEnd(INVALID_CHAIR,GER_NORMAL);

				m_brc_table_status = emTABLE_STATUS_END;
				m_brc_table_status_time = 0;

				//ͬ��ˢ�°��˳����ƽ��������״̬��Ϣ
				OnBrcControlFlushTableStatus();

            }break;
        case TABLE_STATE_NIUNIU_GAME_END:       // ������Ϸ
            {
    			//�л�ׯ��
    			ClearTurnGameData();
    			ChangeBanker(false);
                SetGameState(TABLE_STATE_NIUNIU_FREE);
                m_coolLogic.beginCooling(s_FreeTime);
				CheckRetireTableUser();

				m_brc_table_status = emTABLE_STATUS_FREE;
				m_brc_table_status_time = 0;

				//ͬ��ˢ�°��˳����ƽ��������״̬��Ϣ
				OnBrcControlFlushTableStatus();

            }break;
        default:
            break;
        }
    }

	//LOG_DEBUG("on_time_tick_loop 1 - roomid:%d,tableid:%d,m_lGameCount:%lld,tableState:%d,m_chairRobotPlaceJetton.size:%d", GetRoomID(), GetTableID(), m_lGameCount, tableState,m_chairRobotPlaceJetton.size());

    if(tableState == TABLE_STATE_NIUNIU_PLACE_JETTON && m_coolLogic.getPassTick() > 0)
    {
		//LOG_DEBUG("on_time_tick_loop 2 - roomid:%d,tableid:%d,m_lGameCount:%lld,tableState:%d,m_chairRobotPlaceJetton.size:%d", GetRoomID(), GetTableID(), m_lGameCount, tableState, m_chairRobotPlaceJetton.size());

        //OnRobotOper();
		OnChairRobotJetton();
		OnRobotJetton();
		
    }
    
}

void CGameBaiNiuTable::OnRobotTick()
{
	uint8 tableState = GetGameState();
	int64 passtick = m_coolLogic.getPassTick();
	//LOG_DEBUG("on_time_tick_loop - roomid:%d,tableid:%d,m_lGameCount:%lld,tableState:%d,passtick:%lld", GetRoomID(), GetTableID(), m_lGameCount, tableState, passtick);

	if (tableState == net::TABLE_STATE_NIUNIU_PLACE_JETTON && m_coolLogic.getPassTick() > 500)
	{
		OnChairRobotPlaceJetton();
		OnRobotPlaceJetton();
	}
}

// ��Ϸ��Ϣ
int CGameBaiNiuTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
    LOG_DEBUG("table recv msg:%d--%d", pPlayer->GetUID(),cmdID);     
    switch(cmdID)
    {
    case net::C2S_MSG_BAINIU_PLACE_JETTON:  // �û���ע
        {
            if(GetGameState() != TABLE_STATE_NIUNIU_PLACE_JETTON){
                LOG_DEBUG("not jetton state can't jetton");
                return 0;
            }            
            net::msg_bainiu_place_jetton_req msg;
            PARSE_MSG_FROM_ARRAY(msg);                      
            return OnUserPlaceJetton(pPlayer,msg.jetton_area(),msg.jetton_score());
        }break;
    case net::C2S_MSG_BAINIU_APPLY_BANKER:  // ����ׯ��
        {
            net::msg_bainiu_apply_banker msg;
            PARSE_MSG_FROM_ARRAY(msg);
            if(msg.apply_oper() == 1){            
                return OnUserApplyBanker(pPlayer,msg.apply_score(),msg.auto_addscore());
            }else{
                return OnUserCancelBanker(pPlayer);
            }
        }break;
    case net::C2S_MSG_BAINIU_JUMP_APPLY_QUEUE:// ���
        {
            net::msg_bainiu_jump_apply_queue_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            
            return OnUserJumpApplyQueue(pPlayer);
        }break;    
	case net::C2S_MSG_BAINIU_CONTINUOUS_PRESSURE_REQ://
		{
			net::msg_player_continuous_pressure_jetton_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnUserContinuousPressure(pPlayer, msg);
		}break;
	case net::C2S_MSG_BRC_CONTROL_ENTER_TABLE_REQ://
		{
			net::msg_brc_control_user_enter_table_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnBrcControlEnterControlInterface(pPlayer);
		}break;
	case net::C2S_MSG_BRC_CONTROL_LEAVE_TABLE_REQ://
		{
			net::msg_brc_control_user_leave_table_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnBrcControlPlayerLeaveInterface(pPlayer);
		}break;
	case net::C2S_MSG_BRC_CONTROL_FORCE_LEAVE_BANKER_REQ://
		{
			net::msg_brc_control_force_leave_banker_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnBrcControlApplePlayer(pPlayer, msg.uid());
		}break;
	case net::C2S_MSG_BRC_CONTROL_AREA_INFO_REQ://
		{
			net::msg_brc_control_area_info_req msg;
			PARSE_MSG_FROM_ARRAY(msg);

			return OnBrcControlPlayerBetArea(pPlayer, msg);
		}break;
    default:
        return 0;
    }
    return 0;
}
//�û����߻�����
bool CGameBaiNiuTable::OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin)
{
    if(bConnected)//��������
    {
        if(isJoin)
        {
            pPlayer->SetPlayDisconnect(false);
            PlayerSetAuto(pPlayer,0);
            SendTableInfoToClient(pPlayer);
            SendSeatInfoToClient(pPlayer);
            if(m_mpLookers.find(pPlayer->GetUID()) != m_mpLookers.end()){
                NotifyPlayerJoin(pPlayer,true);
            }
            SendLookerListToClient(pPlayer);
            SendGameScene(pPlayer);
			SendPlayLog(pPlayer);
        }

		uint32 uid = 0;
		int64 lockScore = 0;
		if (pPlayer != NULL) {
			uid = pPlayer->GetUID();
			lockScore = m_ApplyUserScore[pPlayer->GetUID()];
		}
		int64 lCurScore = GetPlayerCurScore(pPlayer);
		LOG_DEBUG("uid:%d,isJoin:%d,lockScore:%lld,lCurScore:%lld", uid, isJoin, lockScore, lCurScore);

    }else{
        pPlayer->SetPlayDisconnect(true);
    }
    return true;
}
//�û�����
bool CGameBaiNiuTable::OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer)
{
    
    SendSeatInfoToClient();
    return true;
}
//�û�����
bool CGameBaiNiuTable::OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer)
{

    SendSeatInfoToClient();
    return true;
}
// ��Ϸ��ʼ
bool CGameBaiNiuTable::OnGameStart()
{
	//m_robotBankerWinPro = CApplication::Instance().call<int>("bainiubankerwin");

    //LOG_DEBUG("game start - roomid:%d,tableid:%d,m_robotBankerWinPro:%d",m_pHostRoom->GetRoomID(),GetTableID(), m_robotBankerWinPro);

    if(m_pCurBanker == NULL){
        //LOG_ERROR("the banker is null");
        CheckRobotApplyBanker();
        ChangeBanker(false);
        return false;
    }
    m_coolLogic.beginCooling(s_PlaceJettonTime);
    
    net::msg_bainiu_start_rep gameStart;
    gameStart.set_time_leave(m_coolLogic.getCoolTick());
    gameStart.set_banker_score(m_lBankerScore);
    gameStart.set_banker_id(GetBankerUID());
    gameStart.set_banker_buyin_score(m_lBankerBuyinScore);
    SendMsgToAll(&gameStart,net::S2C_MSG_BAINIU_START);   
	OnTableGameStart();
    OnRobotStandUp();
    return true;
}

void CGameBaiNiuTable::AddGameCount()
{
	if (GetGameType() == net::GAME_CATE_BULLFIGHT)
	{
		m_uBairenTotalCount++;
	}
}


void CGameBaiNiuTable::InitAreaSize(uint32 count)
{
	if (GetGameType() == net::GAME_CATE_BULLFIGHT && count < 128)
	{
		m_vecAreaWinCount.resize(count);
		m_vecAreaLostCount.resize(count);

		for (uint32 i = 0; i < m_vecAreaWinCount.size(); i++)
		{
			m_vecAreaWinCount[i] = 0;
		}
		for (uint32 i = 0; i < m_vecAreaLostCount.size(); i++)
		{
			m_vecAreaLostCount[i] = 0;
		}
	}
}

void CGameBaiNiuTable::OnTablePushAreaWin(uint32 index, int win)
{
	if (index < m_vecAreaWinCount.size())
	{
		if (win == 1)
		{
			m_vecAreaWinCount[index] ++;
		}
		else
		{
			m_vecAreaLostCount[index] ++;
		}
	}
}

//��Ϸ����
bool CGameBaiNiuTable::OnGameEnd(uint16 chairID,uint8 reason)
{
    LOG_DEBUG("game end  roomid:%d,tableid:%d,m_wBankerTime:%d,reason:%d,chairID:%d", GetRoomID(), GetTableID(), m_wBankerTime, reason, chairID);
    switch(reason)
    {
    case GER_NORMAL:		//�������
        {
            InitBlingLog();
            WriteBankerInfo();
            AddPlayerToBlingLog();
            
			//�������
			int64 lBankerWinScore = CalculateScore();
            int64 bankerfee = CalcPlayerInfo(GetBankerUID(),lBankerWinScore,true);
			// add by har
			int64 robotWinScore = 0; // ��������Ӯ��
			int64 playerFees = 0;   // ����ܳ�ˮ
			if (m_pCurBanker) {
				if (m_pCurBanker->IsRobot())
					robotWinScore = lBankerWinScore;
				else
					playerFees = -bankerfee;
			} // add by har end
			lBankerWinScore += bankerfee;
            WriteMaxCardType(GetBankerUID(),m_cbTableCardType[MAX_SEAT_INDEX-1]);

			int64 playerAllWinScore = 0; // �����Ӯ��
			if (IsBankerRealPlayer())
				playerAllWinScore = lBankerWinScore; // add by har end

			//��������
			m_wBankerTime++;
			AddGameCount();

			//������Ϣ
            net::msg_bainiu_game_end msg;
            for(uint8 i=0;i<MAX_SEAT_INDEX;++i){
                net::msg_cards* pCards = msg.add_table_cards();
                for(uint8 j=0;j<5;++j){
                    pCards->add_cards(m_cbTableCardArray[i][j]);
                }
            }
            for(uint8 i=0;i<MAX_SEAT_INDEX;++i){
                msg.add_card_types(m_cbTableCardType[i]);                
            }
            for(uint8 i=0;i<AREA_COUNT;++i){
                msg.add_win_multiple(m_winMultiple[i]);
            }
			for (uint8 i = 0; i<AREA_COUNT; ++i)
			{
				OnTablePushAreaWin(i, m_winMultiple[i] > 0);
			}
			//msg.set_total_gamecount(m_uBairenTotalCount);
			//for (uint8 i = 0; i < AREA_COUNT; ++i)
			//{
			//	msg.add_area_wincount(m_vecAreaWinCount[i]);
			//	msg.add_area_lostcount(m_vecAreaLostCount[i]);
			//}

            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(lBankerWinScore);
			msg.set_banker_total_score(m_lBankerWinScore);
            msg.set_rand_card(CNiuNiuLogic::m_cbCardListData[g_RandGen.RandUInt()%CARD_COUNT]);
			msg.set_settle_accounts_type(m_cbBrankerSettleAccountsType);

			//LOG_DEBUG("on game end - roomid:%d,tableid:%d,m_cbTableCardType[%d %d %d %d %d]", m_pHostRoom->GetRoomID(),GetTableID(), m_cbTableCardType[0], m_cbTableCardType[1], m_cbTableCardType[2], m_cbTableCardType[3], m_cbTableCardType[4]);

			//LOG_DEBUG("on game end - roomid:%d,tableid:%d,brankeruid:%d,m_cbTableCardArray[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]", m_pHostRoom->GetRoomID(), GetTableID(), GetBankerUID(), m_cbTableCardArray[0][0], m_cbTableCardArray[0][1], m_cbTableCardArray[0][2], m_cbTableCardArray[0][3], m_cbTableCardArray[0][4], m_cbTableCardArray[1][0],m_cbTableCardArray[1][1], m_cbTableCardArray[1][2], m_cbTableCardArray[1][3], m_cbTableCardArray[1][4], m_cbTableCardArray[2][0], m_cbTableCardArray[2][1], m_cbTableCardArray[2][2], m_cbTableCardArray[2][3], m_cbTableCardArray[2][4], m_cbTableCardArray[3][0], m_cbTableCardArray[3][1], m_cbTableCardArray[3][2], m_cbTableCardArray[3][3], m_cbTableCardArray[3][4], m_cbTableCardArray[4][0], m_cbTableCardArray[4][1],m_cbTableCardArray[4][2],m_cbTableCardArray[4][3],m_cbTableCardArray[4][4]);

			int64 lRobotScoreResult = 0;
			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
					continue;
				if (pPlayer->IsRobot())
				{
					lRobotScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
				}
			}
			//�����Թ��߻���
			map<uint32, CGamePlayer*>::iterator it_win_robot_score = m_mpLookers.begin();
			for (; it_win_robot_score != m_mpLookers.end(); ++it_win_robot_score)
			{
				CGamePlayer* pPlayer = it_win_robot_score->second;
				if (pPlayer == NULL) continue;

				if (pPlayer->IsRobot())
				{
					lRobotScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
				}
			}
			if (lBankerWinScore> 0 && lRobotScoreResult < 0)
			{
				lRobotScoreResult = -lRobotScoreResult;
				int64 fee = -(lRobotScoreResult * m_conf.feeValue / PRO_DENO_10000);
				lRobotScoreResult += fee;
				lRobotScoreResult = -lRobotScoreResult;
			}


			//������λ����
			//for( WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex )
			//{
			//	CGamePlayer *pPlayer = GetPlayer(wUserIndex);
			//	if(pPlayer == NULL)
   //                 continue;
			//	int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
			//	lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
			//	m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
			//	msg.set_user_score(lUserScoreFree);
   //             //pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_GAME_END);
			//}

			//���ͻ���
			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				//���óɼ�
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
				{
					msg.add_player_score(0);
				}
				else
				{
					int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
					// add by har
					if (pPlayer->IsRobot())
						robotWinScore += m_mpUserWinScore[pPlayer->GetUID()];
					else
						playerFees -= lUserScoreFree; // add by har end
					lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
					m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
					//uint32 uid = GetPlayerID(wUserIndex);
					msg.add_player_score(m_mpUserWinScore[pPlayer->GetUID()]);
					if (!pPlayer->IsRobot())
						playerAllWinScore += lUserScoreFree; // add by har end

				}
			}

			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
					continue;
				msg.set_user_score(m_mpUserWinScore[pPlayer->GetUID()]);
				pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_GAME_END);

				//��׼����ͳ��
				OnBrcControlSetResultInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);

			}


            //�����Թ��߻���
            map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
            for(;it != m_mpLookers.end();++it)
            {
                CGamePlayer* pPlayer = it->second;
                if(pPlayer == NULL)continue;

				int64 lUserScoreFree = CalcPlayerInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
				// add by har
				if (pPlayer->IsRobot())
					robotWinScore += m_mpUserWinScore[pPlayer->GetUID()];
				else
					playerFees -= lUserScoreFree; // add by har end
				lUserScoreFree = lUserScoreFree + m_mpUserWinScore[pPlayer->GetUID()];
				m_mpUserWinScore[pPlayer->GetUID()] = lUserScoreFree;
				msg.set_user_score(lUserScoreFree);
				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BAINIU_GAME_END);

				//��׼����ͳ��
				OnBrcControlSetResultInfo(pPlayer->GetUID(), m_mpUserWinScore[pPlayer->GetUID()]);
				if (!pPlayer->IsRobot())
					playerAllWinScore += lUserScoreFree; // add by har end

			}

			//����ͳ��
			int64 lPlayerScoreResult = 0;
			for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
			{
				CGamePlayer *pPlayer = GetPlayer(wUserIndex);
				if (pPlayer == NULL)
					continue;
				if (!pPlayer->IsRobot())
				{
					lPlayerScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
				}
			}
			map<uint32, CGamePlayer*>::iterator it_lookers = m_mpLookers.begin();
			for (; it_lookers != m_mpLookers.end(); ++it_lookers)
			{
				CGamePlayer* pPlayer = it_lookers->second;
				if (pPlayer == NULL)continue;
				if (!pPlayer->IsRobot())
				{
					lPlayerScoreResult += m_mpUserWinScore[pPlayer->GetUID()];
				}
			}

			if (m_pCurBanker != NULL && m_pCurBanker->IsRobot() && m_pHostRoom != NULL && lPlayerScoreResult != 0)
			{
				m_pHostRoom->UpdateJackpotScore(-lPlayerScoreResult);
			}
			if (m_pCurBanker != NULL && !m_pCurBanker->IsRobot() && m_pHostRoom != NULL && lRobotScoreResult != 0)
			{
				m_pHostRoom->UpdateJackpotScore(lRobotScoreResult);
			}

			LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,lPlayerScoreResult:%lld,lRobotScoreResult:%lld,robotWinScore:%lld,playerFees:%lld",
				m_pHostRoom->GetRoomID(), GetTableID(), lPlayerScoreResult, lRobotScoreResult, robotWinScore, playerFees);

			//�����ǰׯ��Ϊ��ʵ��ң���Ҫ���¾�׼����ͳ��
			if (m_pCurBanker && !m_pCurBanker->IsRobot())
			{
				OnBrcControlSetResultInfo(GetBankerUID(), lBankerWinScore);
			}

			m_mpUserWinScore[GetBankerUID()] = 0;   
            SaveBlingLog();
            CheckRobotCancelBanker();

			//ͬ������������ݵ��ض�
			OnBrcFlushSendAllPlayerInfo();
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // add by har
			OnTableGameEnd();

			return true;
        }break;    
    case GER_DISMISS:		//��Ϸ��ɢ
        {
            LOG_ERROR("ǿ����Ϸ��ɢ");
            for(uint8 i=0;i<GAME_PLAYER;++i)
            {
                if(m_vecPlayers[i].pPlayer != NULL) {
                    LeaveTable(m_vecPlayers[i].pPlayer);
                }
            }
            ResetTable();
            return true;
        }break;
    case GER_NETWORK_ERROR:		//�û�ǿ��
    case GER_USER_LEAVE:
    default:
        break;
    }
    //�������
    assert(false);
    return false;
}



//��ҽ�����뿪
void  CGameBaiNiuTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
	uint32 uid = 0;
	if (pPlayer!=NULL) {
		uid = pPlayer->GetUID();
	}
	int64 lCurScore = GetPlayerCurScore(pPlayer);
    //LOG_DEBUG("PlayerJoin -  uid:%d,isJoin:%d,chairID:%d,lCurScore:%lld", uid, isJoin,chairID, lCurScore);
	LOG_DEBUG("PlayerJoin -  uid:%d,isJoin:%d,chairID:%d,lCurScore:%lld", uid, isJoin, chairID, GetPlayerCurScore(pPlayer));
	UpdateEnterScore(isJoin, pPlayer);

    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);
    if(isJoin){
        SendApplyUser(pPlayer);
        SendGameScene(pPlayer);
        SendPlayLog(pPlayer);
    }else{
        OnUserCancelBanker(pPlayer);        
        RemoveApplyBanker(pPlayer->GetUID());
        for(uint8 i=0;i<AREA_COUNT;++i){
            m_userJettonScore[i].erase(pPlayer->GetUID());
        }
    }        

	//ˢ�¿��ƽ�����������
	if (!pPlayer->IsRobot())
	{
		OnBrcFlushSendAllPlayerInfo();
	}
}
// ���ͳ�����Ϣ(��������)
void    CGameBaiNiuTable::SendGameScene(CGamePlayer* pPlayer)
{
	int64 lCurScore = GetPlayerCurScore(pPlayer);
    LOG_DEBUG("send game scene - uid:%d,m_gameState:%d,lCurScore:%lld", pPlayer->GetUID(), m_gameState,lCurScore);
    switch(m_gameState)
    {
    case net::TABLE_STATE_NIUNIU_FREE:          // ����״̬
        {
            net::msg_bainiu_game_info_free_rep msg;
            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_banker_id(GetBankerUID());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(m_lBankerWinScore);
            msg.set_banker_score(m_lBankerScore);
            msg.set_banker_buyin_score(m_lBankerBuyinScore);
           
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_GAME_FREE_INFO);
        }break;
    case net::TABLE_STATE_NIUNIU_PLACE_JETTON:  // ��Ϸ״̬
    case net::TABLE_STATE_NIUNIU_GAME_END:      // ����״̬
        {
            net::msg_bainiu_game_info_play_rep msg;
            for(uint8 i=0;i<AREA_COUNT;++i){
                msg.add_all_jetton_score(m_allJettonScore[i]);
            }            
            msg.set_banker_id(GetBankerUID());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(m_lBankerWinScore);
            msg.set_banker_score(m_lBankerScore);
            msg.set_banker_buyin_score(m_lBankerBuyinScore);
            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_game_status(m_gameState);
            for(uint8 i=0;i<AREA_COUNT;++i){
                msg.add_self_jetton_score(m_userJettonScore[i][pPlayer->GetUID()]);
            }       
			//msg.set_max_jetton_rate(m_iMaxJettonRate);
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_GAME_PLAY_INFO);     

			//ˢ�����п��ƽ�����ϢЭ��---���ڶ��������Ĵ���
			if (pPlayer->GetCtrlFlag())
			{
				//ˢ�°��˳�����״̬
				m_brc_table_status_time = m_coolLogic.getCoolTick();
				OnBrcControlFlushTableStatus(pPlayer);

				//���Ϳ���������Ϣ
				OnBrcControlFlushAreaInfo(pPlayer);
				//����������ʵ����б�
				OnBrcControlSendAllPlayerInfo(pPlayer);
				//���ͻ���������ע��Ϣ
				OnBrcControlSendAllRobotTotalBetInfo();
				//������ʵ�������ע��Ϣ
				OnBrcControlSendAllPlayerTotalBetInfo();
				//����������ׯ����б�
				OnBrcControlFlushAppleList();
			}
        }break;
    default:
        break;                    
    }
    SendLookerListToClient(pPlayer);
    SendApplyUser(pPlayer);
	SendFrontJettonInfo(pPlayer);
}

// ��ȡ������ע���ǻ����˻������  add by har
void CGameBaiNiuTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	for (uint16 wAreaIndex = 0; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
		if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
			continue;
		if (pPlayer->IsRobot())
			isAllPlayer = false;
		else
		    isAllRobot = false;
		return;
	}
}

int64    CGameBaiNiuTable::CalcPlayerInfo(uint32 uid,int64 winScore,bool isBanker)
{
    if(winScore == 0)
        return 0;
    //LOG_DEBUG("report to lobby:%d  %lld",uid,winScore);
	int64 fee = CalcPlayerGameInfo(uid, winScore, 0, true);
    if(isBanker){// ׯ�ҳ��Ӽ�����Ӧ����,������Ŀ��ƽ
        m_lBankerWinScore    += fee;
        //��ǰ����
        m_lBankerScore       += fee;
    }
	return fee;
}
// ������Ϸ����
void    CGameBaiNiuTable::ResetGameData()
{
	//����ע��
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
    memset(m_playerJettonScore,0,sizeof(m_playerJettonScore));
    memset(m_winMultiple,0,sizeof(m_winMultiple));

	//������ע
	for(uint8 i=0;i<AREA_COUNT;++i){
	    m_userJettonScore[i].clear();
	}
	//��ҳɼ�	
	m_mpUserWinScore.clear();       
	//�˿���Ϣ
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));
	//ׯ����Ϣ
	m_pCurBanker            = NULL;
	m_wBankerTime           = 0;
	m_lBankerWinScore       = 0L;		


    m_bankerAutoAddScore    = 0;                   //�Զ�����
    m_needLeaveBanker       = false;               //�뿪ׯλ
        
    m_wBankerTime = 0;							//��ׯ����
    m_wBankerWinTime = 0;                       //ʤ������
	m_lBankerScore = 0;							//ׯ�һ���
	m_lBankerWinScore = 0;						//�ۼƳɼ�

    m_lBankerBuyinScore = 0;                    //ׯ�Ҵ���
    m_lBankerInitBuyinScore = 0;                //ׯ�ҳ�ʼ����
    m_lBankerWinMaxScore = 0;                   //ׯ�������Ӯ
    m_lBankerWinMinScore = 0;                   //ׯ�������Ӯ

}
void    CGameBaiNiuTable::ClearTurnGameData()
{
	//����ע��
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
	memset(m_playerJettonScore, 0, sizeof(m_playerJettonScore));

	//������ע
	for(uint8 i=0;i<AREA_COUNT;++i){
	    m_userJettonScore[i].clear();
	}
	//��ҳɼ�	
	m_mpUserWinScore.clear();       
	//�˿���Ϣ
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));     
}
// д�����log
void    CGameBaiNiuTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int32 mulip)
{
    uint8 cardType = m_GameLogic.GetCardType(cardData,cardCount);
    Json::Value logValue;
    logValue["p"]       = chairID;
    logValue["m"]       = mulip;
    logValue["cardtype"] = cardType;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
    m_operLog["card"].append(logValue);
}
// д���עlog
void    CGameBaiNiuTable::WriteAddScoreLog(uint32 uid,uint8 area,int64 score)
{
    if(score == 0)
        return;
    Json::Value logValue;
    logValue["uid"]  = uid;
    logValue["p"]    = area;
    logValue["s"]    = score;

    m_operLog["op"].append(logValue);
}
// д���������
void 	CGameBaiNiuTable::WriteMaxCardType(uint32 uid,uint8 cardType)
{
    Json::Value logValue;
    logValue["uid"]  = uid;
    logValue["mt"]   = cardType;
    m_operLog["maxcard"].append(logValue);
}
// д��ׯ����Ϣ
void    CGameBaiNiuTable::WriteBankerInfo()
{
	tagJackpotScore tmpJackpotScore;
	if (m_pHostRoom != NULL)
	{
		tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
	}
    m_operLog["banker"] = GetBankerUID();
	m_operLog["cps"] = tmpJackpotScore.lCurPollScore;
	m_operLog["swp"] = tmpJackpotScore.uSysWinPro;
	m_operLog["slp"] = tmpJackpotScore.uSysLostPro;

}
//��ע�¼�
bool    CGameBaiNiuTable:: OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore)
{
    //LOG_DEBUG("player place jetton:%d--%d--%lld",pPlayer->GetUID(),cbJettonArea,lJettonScore);
    //Ч�����
	if(cbJettonArea > ID_HUANG_MEN || lJettonScore <= 0){
        
        LOG_DEBUG("jetton is error:%d--%lld",cbJettonArea,lJettonScore);
		return false;
	}
	if(GetGameState() != net::TABLE_STATE_NIUNIU_PLACE_JETTON){
        net::msg_bainiu_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_FAIL);
        
		//������Ϣ
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_PLACE_JETTON_REP);        
		return true;
	}
	//ׯ���ж�
	if(pPlayer->GetUID() == GetBankerUID()){
        LOG_DEBUG("the banker can't jetton");
		return true;
	}

	//��������
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
	{
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}

	if (TableJettonLimmit(pPlayer, lJettonScore, lJettonCount) == false)
	{
		//bPlaceJettonSuccess = false;
		LOG_DEBUG("table_jetton_limit - roomid:%d,tableid:%d,uid:%d,cbJettonArea:%d,lJettonScore:%lld,curScore:%lld,jettonmin:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), cbJettonArea, lJettonScore, GetPlayerCurScore(pPlayer), GetJettonMin());

		net::msg_bainiu_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//������Ϣ
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BAINIU_PLACE_JETTON_REP);
		return true;
	}
	//��һ���
	int64 lUserScore = GetPlayerCurScore(pPlayer);

	//�Ϸ�У��
	if(lUserScore < lJettonCount + lJettonScore) 
    {
        LOG_DEBUG("the jetton more than you have");
        return true;
	}
	//�ɹ���ʶ
	bool bPlaceJettonSuccess=true;
	//�Ϸ���֤
	if(GetUserMaxJetton(pPlayer, cbJettonArea) >= lJettonScore)
	{
		//������ע
		m_allJettonScore[cbJettonArea] += lJettonScore;
        if(!pPlayer->IsRobot()){
            m_playerJettonScore[cbJettonArea] += lJettonScore;
        }
		m_userJettonScore[cbJettonArea][pPlayer->GetUID()] += lJettonScore;			
	}else{
	    LOG_DEBUG("the jetton more than limit");
		bPlaceJettonSuccess = false;
	}
	if(bPlaceJettonSuccess)
	{
		RecordPlayerBaiRenJettonInfo(pPlayer, cbJettonArea, lJettonScore);

		//OnAddPlayerJetton(pPlayer->GetUID(), lJettonScore);

        net::msg_bainiu_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_SUCCESS);        
		//������Ϣ
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_PLACE_JETTON_REP);

        net::msg_bainiu_place_jetton_broadcast broad;
        broad.set_uid(pPlayer->GetUID());
        broad.set_jetton_area(cbJettonArea);
        broad.set_jetton_score(lJettonScore);
        broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);
        
        SendMsgToAll(&broad,net::S2C_MSG_BAINIU_PLACE_JETTON_BROADCAST);

		//ˢ�°��˳����ƽ������ע��Ϣ
		OnBrcControlBetDeal(pPlayer);
	}
	else
	{
        net::msg_bainiu_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_FAIL);
        
		//������Ϣ
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_PLACE_JETTON_REP);
        
	}    
    return true;
}
bool    CGameBaiNiuTable::OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg)
{
	//LOG_DEBUG("player place jetton:%d--%d--%lld",pPlayer->GetUID(),cbJettonArea,lJettonScore);
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,branker:%d,GetGameState:%d,info_size:%d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), msg.info_size());

	net::msg_player_continuous_pressure_jetton_rep rep;
	rep.set_result(net::RESULT_CODE_FAIL);
	if (msg.info_size() == 0 || GetGameState() != net::TABLE_STATE_NIUNIU_PLACE_JETTON || pPlayer->GetUID() == GetBankerUID())
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BAINIU_CONTINUOUS_PRESSURE_REP);
		return false;
	}
	//Ч�����
	int64 lTotailScore = 0;
	for (int i = 0; i < msg.info_size(); i++)
	{
		net::bairen_jetton_info info = msg.info(i);

		if (info.score() <= 0 || info.area()>ID_HUANG_MEN)
		{
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BAINIU_CONTINUOUS_PRESSURE_REP);
			return false;
		}
		lTotailScore += info.score();
	}

	//��������
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex)
	{
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}

	//��һ���
	int64 lUserScore = GetPlayerCurScore(pPlayer);
	//�Ϸ�У��
	//if (lUserScore < lJettonCount + lTotailScore + GetJettonMin())
	if (GetJettonMin() > GetEnterScore(pPlayer) || (lUserScore < lJettonCount + lTotailScore))
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BAINIU_CONTINUOUS_PRESSURE_REP);

		LOG_DEBUG("the jetton more than you have - uid:%d", pPlayer->GetUID());

		return true;
	}
	//�ɹ���ʶ
	//bool bPlaceJettonSuccess = true;

	//for (int i = 0; i < msg.info_size(); i++)
	//{
	//	net::bairen_jetton_info info = msg.info(i);

	//	if (info.score() <= 0 || info.area()>ID_HUANG_MEN)
	//	{
	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BAINIU_CONTINUOUS_PRESSURE_REP);
	//		return false;
	//	}
	//	if (GetUserMaxJetton(pPlayer, info.area()) < info.score())
	//	{
	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BAINIU_CONTINUOUS_PRESSURE_REP);
	//		return false;
	//	}
	//}

	int64 lUserMaxHettonScore = GetUserMaxJetton(pPlayer, 0);
	if (lUserMaxHettonScore < lTotailScore)
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BAINIU_CONTINUOUS_PRESSURE_REP);

		LOG_DEBUG("error_pressu - uid:%d,lUserMaxHettonScore:%lld,lUserScore:%lld,lJettonCount:%lld,, lTotailScore:%lld,, GetJettonMin:%lld",
			pPlayer->GetUID(), lUserMaxHettonScore, lUserScore, lJettonCount, lTotailScore, GetJettonMin());

		return false;
	}

	for (int i = 0; i < msg.info_size(); i++)
	{
		net::bairen_jetton_info info = msg.info(i);

		m_allJettonScore[info.area()] += info.score();
		if (!pPlayer->IsRobot())
		{
			m_playerJettonScore[info.area()] += info.score();
		}
		m_userJettonScore[info.area()][pPlayer->GetUID()] += info.score();

		RecordPlayerBaiRenJettonInfo(pPlayer, info.area(), info.score());

		BYTE cbJettonArea = info.area();
		int64 lJettonScore = info.score();

		net::msg_bainiu_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_SUCCESS);
		//������Ϣ
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BAINIU_PLACE_JETTON_REP);

		net::msg_bainiu_place_jetton_broadcast broad;
		broad.set_uid(pPlayer->GetUID());
		broad.set_jetton_area(cbJettonArea);
		broad.set_jetton_score(lJettonScore);
		broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

		SendMsgToAll(&broad, net::S2C_MSG_BAINIU_PLACE_JETTON_BROADCAST);
	}

	rep.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BAINIU_CONTINUOUS_PRESSURE_REP);

	//ˢ�°��˳����ƽ������ע��Ϣ
	OnBrcControlBetDeal(pPlayer);

	return true;
}

//����ׯ��
bool    CGameBaiNiuTable::OnUserApplyBanker(CGamePlayer* pPlayer,int64 bankerScore,uint8 autoAddScore)
{
	int64 lCurScore = GetPlayerCurScore(pPlayer);
    LOG_DEBUG("player  begin apply banker - uid:%d,bankerScore:%lld,lCurScore:%lld",pPlayer->GetUID(),bankerScore, lCurScore);
    //�������
    net::msg_bainiu_apply_banker_rep msg;
    msg.set_apply_oper(1);
    msg.set_buyin_score(bankerScore);
    
    if(m_pCurBanker == pPlayer){
        LOG_DEBUG("you is the banker");
        msg.set_result(net::RESULT_CODE_ERROR_STATE);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_APPLY_BANKER); 
        return false;
    }
    if(IsSetJetton(pPlayer->GetUID())){// ��ע������ׯ
        msg.set_result(net::RESULT_CODE_ERROR_STATE);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_APPLY_BANKER);
        return false;
    }    
    //�Ϸ��ж�
	int64 lUserScore = GetPlayerCurScore(pPlayer);
    if(bankerScore > lUserScore){
        LOG_DEBUG("you not have more score:%d",pPlayer->GetUID());
        msg.set_result(net::RESULT_CODE_ERROR_PARAM);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_APPLY_BANKER);    
        return false;    
    }
    
	if(bankerScore < GetApplyBankerCondition() || bankerScore > GetApplyBankerConditionLimit()){
		LOG_DEBUG("you score less than condition %lld--%lld��faild",GetApplyBankerCondition(),GetApplyBankerConditionLimit());
        msg.set_result(net::RESULT_CODE_ERROR_PARAM);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_APPLY_BANKER);  
		return false;
	}

	//�����ж�
	for(uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx)
	{
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if(id == pPlayer->GetUID())
		{
			LOG_DEBUG("you is in apply list");
            msg.set_result(net::RESULT_CODE_ERROR_STATE);    
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_APPLY_BANKER);
			return false;
		}
	}

	//������Ϣ 
	m_ApplyUserArray.push_back(pPlayer);
    m_mpApplyUserInfo[pPlayer->GetUID()] = autoAddScore;
    LockApplyScore(pPlayer,bankerScore);        

    msg.set_result(net::RESULT_CODE_SUCCESS);    
    pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_APPLY_BANKER);
	//�л��ж�
	if(GetGameState() == net::TABLE_STATE_NIUNIU_FREE && m_ApplyUserArray.size() == 1)
	{
		ChangeBanker(false);
	}
    FlushApplyUserSort();
    SendApplyUser();

	//ˢ�¿��ƽ������ׯ�б�
	OnBrcControlFlushAppleList();

	int64 lCurScoreFinish = GetPlayerCurScore(pPlayer);
	LOG_DEBUG("player finish apply banker - uid:%d,bankerScore:%lld,lCurScore:%lld", pPlayer->GetUID(), bankerScore, lCurScoreFinish);

    return true;
}
bool    CGameBaiNiuTable::OnUserJumpApplyQueue(CGamePlayer* pPlayer)
{
    LOG_DEBUG("player jump queue:%d",pPlayer->GetUID());
    int64 cost = CDataCfgMgr::Instance().GetJumpQueueCost();  
    net::msg_bainiu_jump_apply_queue_rep msg;
    if(pPlayer->GetAccountValue(emACC_VALUE_COIN) < cost){
        LOG_DEBUG("the jump cost can't pay:%lld",cost);
        msg.set_result(net::RESULT_CODE_CION_ERROR);
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_JUMP_APPLY_QUEUE);
        return false;
    }    
    //�����ж�
	for(uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx)
	{
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if(id == pPlayer->GetUID())
		{
            if(nUserIdx == 0){
               LOG_DEBUG("you is the first queue");
               return false; 
            }
            m_ApplyUserArray.erase(m_ApplyUserArray.begin()+nUserIdx);
            m_ApplyUserArray.insert(m_ApplyUserArray.begin(),pPlayer);                      
            
            SendApplyUser(NULL);
            msg.set_result(net::RESULT_CODE_SUCCESS);
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_JUMP_APPLY_QUEUE);

			//ˢ�¿��ƽ������ׯ�б�
			OnBrcControlFlushAppleList();

            cost = -cost;
            pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_JUMPQUEUE,0,0,cost,0,0,0,0);
			return true;
		}
	}      
    
    return false;
}
//ȡ������
bool    CGameBaiNiuTable::OnUserCancelBanker(CGamePlayer* pPlayer)
{
    LOG_DEBUG("cance banker:%d",pPlayer->GetUID());

    net::msg_bainiu_apply_banker_rep msg;
    msg.set_apply_oper(0);
    msg.set_result(net::RESULT_CODE_SUCCESS);

    //ǰ���ֲ�����ׯ
    if(pPlayer->GetUID() == GetBankerUID() && m_wBankerTime < 3)
        return false;
    //��ǰׯ�� 
	if(pPlayer->GetUID() == GetBankerUID() && GetGameState() != net::TABLE_STATE_NIUNIU_FREE)
	{
		//������Ϣ
		LOG_DEBUG("the game is run,you can't cance banker");
        m_needLeaveBanker = true;
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_APPLY_BANKER);            
		return true;
	}
	//�����ж�
	for(WORD i=0; i<m_ApplyUserArray.size(); ++i)
	{
		//��ȡ���
		CGamePlayer *pTmp = m_ApplyUserArray[i];		     

		//��������
		if(pTmp != pPlayer) continue;

		//ɾ�����
		RemoveApplyBanker(pPlayer->GetUID());       
		if(m_pCurBanker != pPlayer)
		{
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_APPLY_BANKER);			
		}
		else if(m_pCurBanker == pPlayer)
		{
			//�л�ׯ�� 
			ChangeBanker(true);
		}
        SendApplyUser();
		return true;
	}
	return false;
}

bool CGameBaiNiuTable::GetCardSortIndex(uint8 uArSortIndex[])
{
	BYTE cbTableCard[MAX_SEAT_INDEX][5] = { {0} };
	memcpy(cbTableCard, m_cbTableCardArray, sizeof(m_cbTableCardArray));

	uint8 uArSortCardIndex[MAX_SEAT_INDEX] = { 0,1,2,3,4 };

	for (uint8 i = 0; i < MAX_SEAT_INDEX; i++)
	{
		for (uint8 j = 0; j < MAX_SEAT_INDEX; j++)
		{
			if (i == j)
			{
				continue;
			}
			BYTE mup;
			bool bIsSwapCardIndex = (m_GameLogic.CompareCard(cbTableCard[j], 5, cbTableCard[i], 5, mup) == 1);
			if (bIsSwapCardIndex)
			{
				uint8 tmp[5];
				memcpy(tmp, cbTableCard[j], 5);
				memcpy(cbTableCard[j], cbTableCard[i], 5);
				memcpy(cbTableCard[i], tmp, 5);

				uint8 uTempIndex = uArSortCardIndex[i];
				uArSortCardIndex[i] = uArSortCardIndex[j];
				uArSortCardIndex[j] = uTempIndex;
			}
		}
	}

	for (int i = 0; i < MAX_SEAT_INDEX; i++)
	{
		uArSortIndex[i] = uArSortCardIndex[i];
	}

	return true;
}

bool CGameBaiNiuTable::GetJettonSortIndex(uint32 uid, uint8 uArSortIndex[])
{
	int64 lArJettonScore[AREA_COUNT] = {0};
	for (int i = 0; i < AREA_COUNT; i++)
	{
		lArJettonScore[i] = m_userJettonScore[i][uid];
	}

	uint8 uArSortJettonIndex[AREA_COUNT] = { 0,1,2,3 };

	for (uint8 i = 0; i < AREA_COUNT; i++)
	{
		for (uint8 j = 0; j < AREA_COUNT; j++)
		{
			if (i == j)
			{
				continue;
			}
			if (lArJettonScore[j]<lArJettonScore[i])
			{
				int64 lTempScore = lArJettonScore[i];
				lArJettonScore[i] = lArJettonScore[j];
				lArJettonScore[j] = lTempScore;

				uint8 uTempIndex = uArSortJettonIndex[i];
				uArSortJettonIndex[i] = uArSortJettonIndex[j];
				uArSortJettonIndex[j] = uTempIndex;
			}
		}
	}
	for (int i = 0; i < AREA_COUNT; i++)
	{
		uArSortIndex[i] = uArSortJettonIndex[i];
	}

	return true;
}
bool CGameBaiNiuTable::GetPlayerGameRest(uint32 uid)
{
	bool static bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang;
	BYTE TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple;
	TianMultiple = 1;
	DiMultiple = 1;
	TianXuanltiple = 1;
	HuangMultiple = 1;
	DeduceWinner(bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple);

	BYTE  cbMultiple[] = { 1, 1, 1, 1 };
	cbMultiple[ID_TIAN_MEN] = TianMultiple;
	cbMultiple[ID_DI_MEN] = DiMultiple;
	cbMultiple[ID_XUAN_MEN] = TianXuanltiple;
	cbMultiple[ID_HUANG_MEN] = HuangMultiple;

	int64 control_win_score = 0;
	int64 control_lose_score = 0;
	int64 control_result_score = 0;

	//ʤ����ʶ
	bool static bWinFlag[AREA_COUNT];
	bWinFlag[ID_TIAN_MEN] = bWinTianMen;
	bWinFlag[ID_DI_MEN] = bWinDiMen;
	bWinFlag[ID_XUAN_MEN] = bWinXuanMen;
	bWinFlag[ID_HUANG_MEN] = bWinHuang;

	for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
	{
		if (m_userJettonScore[wAreaIndex][uid] == 0)
			continue;
		if (true == bWinFlag[wAreaIndex])// Ӯ��
		{
			control_win_score += (m_userJettonScore[wAreaIndex][uid] * cbMultiple[wAreaIndex]);

		}
		else// ����
		{
			control_lose_score -= m_userJettonScore[wAreaIndex][uid] * cbMultiple[wAreaIndex];
		}
	}

	control_result_score = control_win_score + control_lose_score;

	if (control_result_score > 0) {
		return true;
	}

	return false;
}

bool CGameBaiNiuTable::SetControlPlayerWin(uint32 control_uid)
{
	int  iControlPlayerJessonAreaCount = 0;
	bool bArControlPlayerJesson[AREA_COUNT] = {0};
	
	for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
	{
		if (m_userJettonScore[wAreaIndex][control_uid] == 0)
			continue;
		bArControlPlayerJesson[wAreaIndex] = true;
		iControlPlayerJessonAreaCount++;
	}
	if (iControlPlayerJessonAreaCount == 0)
	{
		return false;
	}

	uint8 uArSortJettonIndex[AREA_COUNT] = { 0 };
	GetJettonSortIndex(control_uid,uArSortJettonIndex);

	LOG_DEBUG("control player jetton sort - control_uid:%d,uArSortJettonIndex:%d %d %d %d", control_uid, uArSortJettonIndex[0], uArSortJettonIndex[1], uArSortJettonIndex[2], uArSortJettonIndex[3]);

	uint8 uArSortCardIndex[MAX_SEAT_INDEX] = {0};
	
	if (iControlPlayerJessonAreaCount==1)
	{
		GetCardSortIndex(uArSortCardIndex);

		if (uArSortCardIndex[1] != 0)
		{
			uint8 tmp[5];
			memcpy(tmp, m_cbTableCardArray[0], 5);
			memcpy(m_cbTableCardArray[0], m_cbTableCardArray[uArSortCardIndex[1]], 5);
			memcpy(m_cbTableCardArray[uArSortCardIndex[1]], tmp, 5);
		}

		GetCardSortIndex(uArSortCardIndex);

		uint8 cbJettonAreaCardIndex = uArSortJettonIndex[0] + 1;
		if (cbJettonAreaCardIndex != uArSortCardIndex[0])
		{
			uint8 tmp[5];
			memcpy(tmp, m_cbTableCardArray[cbJettonAreaCardIndex], 5);
			memcpy(m_cbTableCardArray[cbJettonAreaCardIndex], m_cbTableCardArray[uArSortCardIndex[0]], 5);
			memcpy(m_cbTableCardArray[uArSortCardIndex[0]], tmp, 5);
		}

		//for (uint8 cbAreaIndex = 0; cbAreaIndex < AREA_COUNT; ++cbAreaIndex)
		//{
		//	uint8 cbJettonAreaCardIndex = cbAreaIndex + 1;
		//	if (bArControlPlayerJesson[cbAreaIndex] && cbJettonAreaCardIndex != uArSortCardIndex[0])
		//	{
		//		uint8 tmp[5];
		//		memcpy(tmp, m_cbTableCardArray[cbJettonAreaCardIndex], 5);
		//		memcpy(m_cbTableCardArray[cbJettonAreaCardIndex], m_cbTableCardArray[uArSortCardIndex[0]], 5);
		//		memcpy(m_cbTableCardArray[uArSortCardIndex[0]], tmp, 5);
		//		//return true;
		//	}
		//}
		return true;
	}
	else if (iControlPlayerJessonAreaCount == 2)
	{
		GetCardSortIndex(uArSortCardIndex);

		if (uArSortCardIndex[1] != 0)
		{
			uint8 tmp[5];
			memcpy(tmp, m_cbTableCardArray[0], 5);
			memcpy(m_cbTableCardArray[0], m_cbTableCardArray[uArSortCardIndex[1]], 5);
			memcpy(m_cbTableCardArray[uArSortCardIndex[1]], tmp, 5);
		}

		GetCardSortIndex(uArSortCardIndex);

		uint8 cbJettonAreaCardIndex = uArSortJettonIndex[0] + 1;

		if (cbJettonAreaCardIndex != uArSortCardIndex[0])
		{
			uint8 tmp[5];
			memcpy(tmp, m_cbTableCardArray[cbJettonAreaCardIndex], 5);
			memcpy(m_cbTableCardArray[cbJettonAreaCardIndex], m_cbTableCardArray[uArSortCardIndex[0]], 5);
			memcpy(m_cbTableCardArray[uArSortCardIndex[0]], tmp, 5);
		}

		if (GetPlayerGameRest(control_uid))
		{
			return true;
		}
		else
		{
			GetCardSortIndex(uArSortCardIndex);

			GetCardSortIndex(uArSortCardIndex);
			if (uArSortCardIndex[2] != 0)
			{
				uint8 tmp[5];
				memcpy(tmp, m_cbTableCardArray[0], 5);
				memcpy(m_cbTableCardArray[0], m_cbTableCardArray[uArSortCardIndex[2]], 5);
				memcpy(m_cbTableCardArray[uArSortCardIndex[2]], tmp, 5);
			}
			GetCardSortIndex(uArSortCardIndex);
			uint8 cbJettonAreaCardIndex = uArSortJettonIndex[0] + 1;
			if (cbJettonAreaCardIndex != uArSortCardIndex[0])
			{
				uint8 tmp[5];
				memcpy(tmp, m_cbTableCardArray[cbJettonAreaCardIndex], 5);
				memcpy(m_cbTableCardArray[cbJettonAreaCardIndex], m_cbTableCardArray[uArSortCardIndex[0]], 5);
				memcpy(m_cbTableCardArray[uArSortCardIndex[0]], tmp, 5);
			}
			GetCardSortIndex(uArSortCardIndex);
			cbJettonAreaCardIndex = uArSortJettonIndex[1] + 1;
			if (cbJettonAreaCardIndex != uArSortCardIndex[1])
			{
				uint8 tmp[5];
				memcpy(tmp, m_cbTableCardArray[cbJettonAreaCardIndex], 5);
				memcpy(m_cbTableCardArray[cbJettonAreaCardIndex], m_cbTableCardArray[uArSortCardIndex[1]], 5);
				memcpy(m_cbTableCardArray[uArSortCardIndex[1]], tmp, 5);
			}
			return true;
		}
	}
	else
	{
		BYTE    cbTableCard[CARD_COUNT];

		uint32 brankeruid = GetBankerUID();

		int64 control_win_score = 0;
		int64 control_lose_score = 0;
		int64 control_result_score = 0;

		int irount_count = 1000;
		int iRountIndex = 0;

		for (; iRountIndex < irount_count; iRountIndex++)
		{
			bool static bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang;
			BYTE TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple;
			TianMultiple = 1;
			DiMultiple = 1;
			TianXuanltiple = 1;
			HuangMultiple = 1;
			DeduceWinner(bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple);

			BYTE  cbMultiple[] = { 1, 1, 1, 1 };
			cbMultiple[ID_TIAN_MEN] = TianMultiple;
			cbMultiple[ID_DI_MEN] = DiMultiple;
			cbMultiple[ID_XUAN_MEN] = TianXuanltiple;
			cbMultiple[ID_HUANG_MEN] = HuangMultiple;

			control_win_score = 0;
			control_lose_score = 0;
			control_result_score = 0;

			//ʤ����ʶ
			bool static bWinFlag[AREA_COUNT];
			bWinFlag[ID_TIAN_MEN] = bWinTianMen;
			bWinFlag[ID_DI_MEN] = bWinDiMen;
			bWinFlag[ID_XUAN_MEN] = bWinXuanMen;
			bWinFlag[ID_HUANG_MEN] = bWinHuang;

			for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][control_uid] == 0)
					continue;
				if (true == bWinFlag[wAreaIndex])// Ӯ��
				{
					control_win_score += (m_userJettonScore[wAreaIndex][control_uid] * cbMultiple[wAreaIndex]);

				}
				else// ����
				{
					control_lose_score -= m_userJettonScore[wAreaIndex][control_uid] * cbMultiple[wAreaIndex];
				}
			}

			control_result_score = control_win_score + control_lose_score;
			if (control_result_score > 0) {
				break;
			}
			else {
				//����ϴ��
				m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
				//�����˿�
				memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
			}
		}

		//LOG_DEBUG("player win - roomid:%d,tableid:%d,brankeruid:%d,control_result_score:%lld,iRountIndex:%d", m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, control_result_score, iRountIndex);

		if (control_result_score > 0) {
			return true;
		}
	}

	return false;
}

bool CGameBaiNiuTable::SetControlPlayerLost(uint32 control_uid)
{
	BYTE    cbTableCard[CARD_COUNT];

	uint32 brankeruid = GetBankerUID();

	int64 control_win_score = 0;
	int64 control_lose_score = 0;
	int64 control_result_score = 0;

	int irount_count = 1000;
	int iRountIndex = 0;

	for (; iRountIndex < irount_count; iRountIndex++)
	{
		bool static bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang;
		BYTE TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple;
		TianMultiple = 1;
		DiMultiple = 1;
		TianXuanltiple = 1;
		HuangMultiple = 1;
		DeduceWinner(bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple);

		BYTE  cbMultiple[] = { 1, 1, 1, 1 };
		cbMultiple[ID_TIAN_MEN] = TianMultiple;
		cbMultiple[ID_DI_MEN] = DiMultiple;
		cbMultiple[ID_XUAN_MEN] = TianXuanltiple;
		cbMultiple[ID_HUANG_MEN] = HuangMultiple;

		control_win_score = 0;
		control_lose_score = 0;
		control_result_score = 0;

		//ʤ����ʶ
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_TIAN_MEN] = bWinTianMen;
		bWinFlag[ID_DI_MEN] = bWinDiMen;
		bWinFlag[ID_XUAN_MEN] = bWinXuanMen;
		bWinFlag[ID_HUANG_MEN] = bWinHuang;

		for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
		{
			if (m_userJettonScore[wAreaIndex][control_uid] == 0)
				continue;
			if (true == bWinFlag[wAreaIndex])// Ӯ��
			{
				control_win_score += (m_userJettonScore[wAreaIndex][control_uid] * cbMultiple[wAreaIndex]);

			}
			else// ����
			{
				control_lose_score -= m_userJettonScore[wAreaIndex][control_uid] * cbMultiple[wAreaIndex];
			}
		}

		control_result_score = control_win_score + control_lose_score;
		if (control_result_score < 0) {
			break;
		}
		else {
			//����ϴ��
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//�����˿�
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	LOG_DEBUG("player lost - roomid:%d,tableid:%d,brankeruid:%d,control_result_score:%lld,iRountIndex:%d", m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, control_result_score, iRountIndex);

	if (control_result_score < 0) {
		return true;
	}

	return false;
}

bool CGameBaiNiuTable::SetLeisurePlayerWin()
{
	BYTE    cbTableCard[CARD_COUNT];

	uint32 brankeruid = GetBankerUID();

	int64 control_win_score = 0;
	int64 control_lose_score = 0;
	int64 control_result_score = 0;

	int irount_count = 1000;
	int iRountIndex = 0;

	for (; iRountIndex < irount_count; iRountIndex++)
	{
		bool static bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang;
		BYTE TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple;
		TianMultiple = 1;
		DiMultiple = 1;
		TianXuanltiple = 1;
		HuangMultiple = 1;
		DeduceWinner(bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple);

		BYTE  cbMultiple[] = { 1, 1, 1, 1 };
		cbMultiple[ID_TIAN_MEN] = TianMultiple;
		cbMultiple[ID_DI_MEN] = DiMultiple;
		cbMultiple[ID_XUAN_MEN] = TianXuanltiple;
		cbMultiple[ID_HUANG_MEN] = HuangMultiple;

		control_win_score = 0;
		control_lose_score = 0;
		control_result_score = 0;

		//ʤ����ʶ
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_TIAN_MEN] = bWinTianMen;
		bWinFlag[ID_DI_MEN] = bWinDiMen;
		bWinFlag[ID_XUAN_MEN] = bWinXuanMen;
		bWinFlag[ID_HUANG_MEN] = bWinHuang;

		//������λ����
		for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			uint8 maxCardType = CT_POINT;
			if (pPlayer == NULL)continue;
			if(pPlayer->IsRobot())continue;
			for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
					continue;
				int64 scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);
				if (true == bWinFlag[wAreaIndex])// Ӯ�� 
				{
					control_win_score += scoreWin;
				}
				else// ����
				{
					control_lose_score -= scoreWin;
				}
			}
		}
		//�����Թ��߻���
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			uint8 maxCardType = CT_POINT;
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			int64 lResultScore = 0;
			for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
					continue;
				int64 scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);

				if (true == bWinFlag[wAreaIndex])// Ӯ��
				{
					control_win_score += scoreWin;
				}
				else// ����
				{
					control_lose_score -= scoreWin;
				}
			}
		}

		control_result_score = control_win_score + control_lose_score;
		if (control_result_score > 0) {
			break;
		}
		else {
			//����ϴ��
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//�����˿�
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	//LOG_DEBUG("player win - roomid:%d,tableid:%d,brankeruid:%d,control_result_score:%lld,iRountIndex:%d", m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, control_result_score, iRountIndex);

	if (control_result_score > 0) {
		return true;
	}

	return false;
}

bool CGameBaiNiuTable::SetLeisurePlayerLost()
{
	BYTE    cbTableCard[CARD_COUNT];

	uint32 brankeruid = GetBankerUID();

	int64 control_win_score = 0;
	int64 control_lose_score = 0;
	int64 control_result_score = 0;

	int irount_count = 1000;
	int iRountIndex = 0;

	for (; iRountIndex < irount_count; iRountIndex++)
	{
		bool static bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang;
		BYTE TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple;
		TianMultiple = 1;
		DiMultiple = 1;
		TianXuanltiple = 1;
		HuangMultiple = 1;
		DeduceWinner(bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple);

		BYTE  cbMultiple[] = { 1, 1, 1, 1 };
		cbMultiple[ID_TIAN_MEN] = TianMultiple;
		cbMultiple[ID_DI_MEN] = DiMultiple;
		cbMultiple[ID_XUAN_MEN] = TianXuanltiple;
		cbMultiple[ID_HUANG_MEN] = HuangMultiple;

		control_win_score = 0;
		control_lose_score = 0;
		control_result_score = 0;

		//ʤ����ʶ
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_TIAN_MEN] = bWinTianMen;
		bWinFlag[ID_DI_MEN] = bWinDiMen;
		bWinFlag[ID_XUAN_MEN] = bWinXuanMen;
		bWinFlag[ID_HUANG_MEN] = bWinHuang;

		//������λ����
		for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			uint8 maxCardType = CT_POINT;
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
					continue;
				int64 scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);
				if (true == bWinFlag[wAreaIndex])// Ӯ�� 
				{
					control_win_score += scoreWin;
				}
				else// ����
				{
					control_lose_score -= scoreWin;
				}
			}
		}
		//�����Թ��߻���
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			uint8 maxCardType = CT_POINT;
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			int64 lResultScore = 0;
			for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
					continue;
				int64 scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);

				if (true == bWinFlag[wAreaIndex])// Ӯ��
				{
					control_win_score += scoreWin;
				}
				else// ����
				{
					control_lose_score -= scoreWin;
				}
			}
		}

		control_result_score = control_win_score + control_lose_score;
		if (control_result_score < 0) {
			break;
		}
		else {
			//����ϴ��
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//�����˿�
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}

	//LOG_DEBUG("player win - roomid:%d,tableid:%d,brankeruid:%d,control_result_score:%lld,iRountIndex:%d", m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, control_result_score, iRountIndex);

	if (control_result_score < 0) {
		return true;
	}

	return false;
}


//���û�����ׯ��ӮǮ
bool    CGameBaiNiuTable::SetRobotBankerWin(bool bBrankerIsRobot)
{
	/*
	if (bBrankerIsRobot)
	{
		BYTE    cbTableCard[CARD_COUNT];

		uint32 brankeruid = GetBankerUID();

		int64 lBankerWinScore = 0;

		int irount_count = 1000;
		int iRountIndex = 0;

		for (; iRountIndex < irount_count; iRountIndex++)
		{
			bool static bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang;
			BYTE TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple;
			TianMultiple = 1;
			DiMultiple = 1;
			TianXuanltiple = 1;
			HuangMultiple = 1;
			DeduceWinner(bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple);

			BYTE  cbMultiple[] = { 1, 1, 1, 1 };
			cbMultiple[ID_TIAN_MEN] = TianMultiple;
			cbMultiple[ID_DI_MEN] = DiMultiple;
			cbMultiple[ID_XUAN_MEN] = TianXuanltiple;
			cbMultiple[ID_HUANG_MEN] = HuangMultiple;

			lBankerWinScore = 0;

			//ʤ����ʶ
			bool static bWinFlag[AREA_COUNT];
			bWinFlag[ID_TIAN_MEN] = bWinTianMen;
			bWinFlag[ID_DI_MEN] = bWinDiMen;
			bWinFlag[ID_XUAN_MEN] = bWinXuanMen;
			bWinFlag[ID_HUANG_MEN] = bWinHuang;

			//������λ����
			for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
			{
				//��ȡ�û�
				CGamePlayer * pPlayer = GetPlayer(wChairID);
				if (pPlayer == NULL)continue;
				if(pPlayer->IsRobot())continue;
				for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
				{
					if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
						continue;
					if (true == bWinFlag[wAreaIndex])// Ӯ�� 
					{
						lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);
					}
					else// ����
					{
						lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex];
					}
				}
			}
			//�����Թ��߻���
			map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
			for (; it != m_mpLookers.end(); ++it)
			{
				CGamePlayer* pPlayer = it->second;
				if (pPlayer == NULL)continue;
				if (pPlayer->IsRobot())continue;
				for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
				{
					if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
						continue;
					if (true == bWinFlag[wAreaIndex])// Ӯ��
					{
						lBankerWinScore -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);
					}
					else// ����
					{
						lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex];
					}
				}
			}
			if (lBankerWinScore > 0) {
				break;
			}
			else {
				//����ϴ��
				m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
				//�����˿�
				memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
			}
		}

		LOG_DEBUG("robot win - roomid:%d,tableid:%d,brankeruid:%d,lBankerWinScore:%lld,iRountIndex:%d", m_pHostRoom->GetRoomID(), GetTableID(), brankeruid, lBankerWinScore, iRountIndex);

		if (lBankerWinScore > 0) {
			return true;
		}

		return false;	
	}
	*/

	
	if (bBrankerIsRobot)//ׯλʤ�ʿ���
	{
		uint8 uMaxCardIndex = 0;
		uint8 uSenCardIndex = 0;

		bool bIsMaxCard = g_RandGen.RandRatio(m_robotBankerMaxCardPro, PRO_DENO_10000);

		if (bIsMaxCard)
		{
			for (uint8 i = 0; i <= ID_HUANG_MEN; ++i)
			{
				uint8 seat = i + 1;
				uint8 mup = 0;
				if (m_GameLogic.CompareCard(m_cbTableCardArray[0], 5, m_cbTableCardArray[seat], 5, mup) == 1)
				{
					uint8 tmp[5];
					memcpy(tmp, m_cbTableCardArray[0], 5);
					memcpy(m_cbTableCardArray[0], m_cbTableCardArray[seat], 5);
					memcpy(m_cbTableCardArray[seat], tmp, 5);
				}
			}
		}
		else
		{
			//BYTE cbTableCardArray[MAX_SEAT_INDEX][5];
			//memcpy(cbTableCardArray, m_cbTableCardArray, sizeof(cbTableCardArray));
			uMaxCardIndex = 1;
			uSenCardIndex = 0;
			uint8 mup = 0;
			if (m_GameLogic.CompareCard(m_cbTableCardArray[1], 5, m_cbTableCardArray[0], 5, mup) == 1)
			{				
				uSenCardIndex = 1;
				uMaxCardIndex = 0;
			}

			for (uint8 i = 2; i < MAX_SEAT_INDEX; ++i)
			{
				uint8 seat = i;
				bool bIsMaxCard = (m_GameLogic.CompareCard(m_cbTableCardArray[uMaxCardIndex], 5, m_cbTableCardArray[seat], 5, mup) == 1);
				bool bIsSenCard = (m_GameLogic.CompareCard(m_cbTableCardArray[uSenCardIndex], 5, m_cbTableCardArray[seat], 5, mup) == 1);
				if (bIsMaxCard == true && bIsSenCard == true)
				{
					uSenCardIndex = uMaxCardIndex;
					uMaxCardIndex = seat;
				}
				if (bIsMaxCard == false && bIsSenCard == true)
				{
					uSenCardIndex = seat;
				}
				if (bIsMaxCard == true && bIsSenCard == false)
				{
					uSenCardIndex = uMaxCardIndex;
					uMaxCardIndex = seat;
				}
				if (bIsMaxCard == false && bIsSenCard == false)
				{

				}
			}
			if (uSenCardIndex != 0)
			{
				uint8 tmp[5];
				memcpy(tmp, m_cbTableCardArray[0], 5);
				memcpy(m_cbTableCardArray[0], m_cbTableCardArray[uSenCardIndex], 5);
				memcpy(m_cbTableCardArray[uSenCardIndex], tmp, 5);
			}
		}
		//LOG_DEBUG("robot win - roomid:%d,tableid:%d,brankeruid:%d,uMaxCardIndex:%d,uSenCardIndex:%d,m_cbTableCardArray[0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X - 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X]", m_pHostRoom->GetRoomID(), GetTableID(), GetBankerUID(), uMaxCardIndex, uSenCardIndex, m_cbTableCardArray[0][0], m_cbTableCardArray[0][1], m_cbTableCardArray[0][2], m_cbTableCardArray[0][3], m_cbTableCardArray[0][4], m_cbTableCardArray[1][0], m_cbTableCardArray[1][1], m_cbTableCardArray[1][2], m_cbTableCardArray[1][3], m_cbTableCardArray[1][4], m_cbTableCardArray[2][0], m_cbTableCardArray[2][1], m_cbTableCardArray[2][2], m_cbTableCardArray[2][3], m_cbTableCardArray[2][4], m_cbTableCardArray[3][0], m_cbTableCardArray[3][1], m_cbTableCardArray[3][2], m_cbTableCardArray[3][3], m_cbTableCardArray[3][4], m_cbTableCardArray[4][0], m_cbTableCardArray[4][1], m_cbTableCardArray[4][2], m_cbTableCardArray[4][3], m_cbTableCardArray[4][4]);

	}
	else
	{
		for (uint8 i = 0; i <= ID_HUANG_MEN; ++i)
		{
			uint8 seat = i + 1;
			uint8 mup = 0;
			if (m_GameLogic.CompareCard(m_cbTableCardArray[0], 5, m_cbTableCardArray[seat], 5, mup) != 1)
			{
				uint8 tmp[5];
				memcpy(tmp, m_cbTableCardArray[0], 5);
				memcpy(m_cbTableCardArray[0], m_cbTableCardArray[seat], 5);
				memcpy(m_cbTableCardArray[seat], tmp, 5);
			}
		}
	}
	return true;
}

bool    CGameBaiNiuTable::SetPlayerBrankerWin()
{
	for (uint8 i = 0; i <= ID_HUANG_MEN; ++i)
	{
		uint8 seat = i + 1;
		uint8 mup = 0;
		if (m_GameLogic.CompareCard(m_cbTableCardArray[0], 5, m_cbTableCardArray[seat], 5, mup) == 1)
		{
			uint8 tmp[5];
			memcpy(tmp, m_cbTableCardArray[0], 5);
			memcpy(m_cbTableCardArray[0], m_cbTableCardArray[seat], 5);
			memcpy(m_cbTableCardArray[seat], tmp, 5);
		}
	}

	return true;
}

bool    CGameBaiNiuTable::SetPlayerBrankerLost()
{
	for (uint8 i = 0; i < ID_HUANG_MEN; ++i)
	{
		uint8 seat = i + 1;
		uint8 mup = 0;
		if (m_GameLogic.CompareCard(m_cbTableCardArray[0], 5, m_cbTableCardArray[seat], 5, mup) != 1)
		{
			uint8 tmp[5];
			memcpy(tmp, m_cbTableCardArray[0], 5);
			memcpy(m_cbTableCardArray[0], m_cbTableCardArray[seat], 5);
			memcpy(m_cbTableCardArray[seat], tmp, 5);
		}
	}
	return true;
}

//���û�����ׯ����Ǯ
void    CGameBaiNiuTable::SetRobotBankerLose()
{
	//��һ����С���Ƹ�ׯ�һ�����
	if (m_pCurBanker != NULL && m_pCurBanker->IsRobot()) {
		for (uint8 i = 0; i<ID_HUANG_MEN; ++i) {
			uint8 seat = i + 1;
			uint8 mup = 0;
			if (m_GameLogic.CompareCard(m_cbTableCardArray[0], 5, m_cbTableCardArray[seat], 5, mup) != 1)
			{
				uint8 tmp[5];
				memcpy(tmp, m_cbTableCardArray[0], 5);
				memcpy(m_cbTableCardArray[0], m_cbTableCardArray[seat], 5);
				memcpy(m_cbTableCardArray[seat], tmp, 5);
			}
		}
	}
}

// ���ÿ����Ӯ  add by har
bool CGameBaiNiuTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	int64 bankWin = 0;
	int64 playerAllWinScore = 0;
	int i = 0;
	// ѭ����ֱ���ҵ����������������
	while (true) {
		bankWin = 0;
		playerAllWinScore = GetBankerAndPlayerWinScore(m_cbTableCardArray, bankWin);
		if (CheckStockChange(stockChange, playerAllWinScore, i)) {
			LOG_DEBUG("SetStockWinLose suc  roomid:%d,tableid:%d,stockChange:%d,i:%d,playerAllWinScore:%d,bankWin:%d,IsBankerRealPlayer:%d",
				GetRoomID(), GetTableID(), stockChange, i, playerAllWinScore, bankWin, IsBankerRealPlayer());
			return true;
		}
		if (++i > 999)
			break;
		//����ϴ��
		m_GameLogic.RandCardList(m_cbTableCardArray[0], sizeof(m_cbTableCardArray) / sizeof(m_cbTableCardArray[0][0]));
	}
	LOG_ERROR("SetStockWinLose fail! roomid:%d,tableid:%d,playerAllWinScore:%d,bankWin:%d,stockChange:%d", GetRoomID(), GetTableID(), playerAllWinScore, bankWin, stockChange);
	return false;
}

// ��ȡĳ����ҵ�Ӯ��  add by har
int64 CGameBaiNiuTable::GetSinglePlayerWinScore(CGamePlayer *pPlayer, bool bWinFlag[AREA_COUNT], uint8 cbMultiple[], int64 &lBankerWinScore) {
	if (pPlayer == NULL)
		return 0;
	uint32 playerUid = pPlayer->GetUID();
	int64 playerScoreWin = 0; // �������Ӯ��
	for (uint16 wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
		if (m_userJettonScore[wAreaIndex][playerUid] == 0)
			continue;
		int64 scoreWin = m_userJettonScore[wAreaIndex][playerUid] * cbMultiple[wAreaIndex];
		if (true == bWinFlag[wAreaIndex]) // Ӯ��
			playerScoreWin += scoreWin;
		else // ����
			playerScoreWin -= scoreWin;
	}
	lBankerWinScore -= playerScoreWin;
	if (pPlayer->IsRobot())
		return 0;
	return playerScoreWin;
}

// ��ȡׯ�Һͷǻ��������Ӯ����� add by har
int64 CGameBaiNiuTable::GetBankerAndPlayerWinScore(uint8 cbTableCardArray[][5], int64 &lBankerWinScore) {
	//�ƶ������Ӯ
	bool static bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang;
	uint8 TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple;
	TianMultiple = 1;
	DiMultiple = 1;
	TianXuanltiple = 1;
	HuangMultiple = 1;
	DeduceWinnerDeal(bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple, cbTableCardArray);

	uint8 cbMultiple[] = { 1, 1, 1, 1 };
	cbMultiple[ID_TIAN_MEN] = TianMultiple;
	cbMultiple[ID_DI_MEN] = DiMultiple;
	cbMultiple[ID_XUAN_MEN] = TianXuanltiple;
	cbMultiple[ID_HUANG_MEN] = HuangMultiple;

	int64 playerAllWinScore = 0; // �ǻ����������Ӯ��

	//ʤ����ʶ
	bool static bWinFlag[AREA_COUNT];
	bWinFlag[ID_TIAN_MEN] = bWinTianMen;
	bWinFlag[ID_DI_MEN] = bWinDiMen;
	bWinFlag[ID_XUAN_MEN] = bWinXuanMen;
	bWinFlag[ID_HUANG_MEN] = bWinHuang;

	//������λ����
	for (uint16 wChairID = 0; wChairID < GAME_PLAYER; ++wChairID)
		playerAllWinScore += GetSinglePlayerWinScore(GetPlayer(wChairID), bWinFlag, cbMultiple, lBankerWinScore);
		//��ȡ�û�
		/*CGamePlayer *pPlayer = GetPlayer(wChairID);
		if (pPlayer == NULL)
			continue;
		uint32 playerUid = pPlayer->GetUID();
		int64 playerScoreWin = 0; // �������Ӯ��
		for (uint16 wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex) {
			if (m_userJettonScore[wAreaIndex][playerUid] == 0)
				continue;
			int64 scoreWin = m_userJettonScore[wAreaIndex][playerUid] * cbMultiple[wAreaIndex];
			if (true == bWinFlag[wAreaIndex]) { // Ӯ��
				lBankerWinScore -= scoreWin;
				playerScoreWin += scoreWin;
			} else { // ����
				lBankerWinScore += scoreWin;
				playerScoreWin -= scoreWin;
			}
		}
		if (!pPlayer->IsRobot())
			playerAllWinScore += playerScoreWin;*/

	//�����Թ��߻���
	for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it)
		playerAllWinScore += GetSinglePlayerWinScore(it->second, bWinFlag, cbMultiple, lBankerWinScore);

	if (IsBankerRealPlayer())
		playerAllWinScore += lBankerWinScore;

	return playerAllWinScore;
}

//�����˿�
bool    CGameBaiNiuTable::DispatchTableCard()
{
    //����ϴ��
    m_GameLogic.RandCardList(m_cbTableCardArray[0],sizeof(m_cbTableCardArray)/sizeof(m_cbTableCardArray[0][0]));

    //if(g_RandGen.RandRatio(m_robotBankerWinPro,PRO_DENO_10000)){
    //    SetRobotBankerWin();
    //}
	bool bAreaCtrl = false;
	bool bBrankerIsRobot = false;
	bool bBrankerIsControl = false;
	bool bIsControlPlayerIsJetton = false;
	bool bIsFalgControl = false;

	bAreaCtrl = OnBrcAreaControl();

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;

	bool needRobotChangeCard = g_RandGen.RandRatio(m_robotBankerWinPro, PRO_DENO_10000);

	if (!bAreaCtrl && m_pCurBanker != NULL)
	{
		bBrankerIsRobot = m_pCurBanker->IsRobot();
		if (control_uid == m_pCurBanker->GetUID())
		{
			bBrankerIsControl = true;
		}
	}

	if (!bAreaCtrl && bBrankerIsControl && game_count>0 && (control_type == GAME_CONTROL_WIN || control_type == GAME_CONTROL_LOST))
	{
		if (control_type == GAME_CONTROL_WIN)
		{
			bIsFalgControl = SetPlayerBrankerWin();
		}
		if (control_type == GAME_CONTROL_LOST)
		{
			bIsFalgControl = SetPlayerBrankerLost();
		}
		if (bIsFalgControl && m_tagControlPalyer.count>0)
		{
			/*m_tagControlPalyer.count--;
			if (m_tagControlPalyer.count == 0)
			{
				m_tagControlPalyer.Init();
			}*/
			if (m_pHostRoom != NULL)
			{
				m_pHostRoom->SynControlPlayer(GetTableID(), m_tagControlPalyer.uid, -1, m_tagControlPalyer.type);
			}
		}
	}

	
	if (!bAreaCtrl && !bBrankerIsControl && control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		for (uint8 i = 0; i < AREA_COUNT; ++i)
		{
			if (m_userJettonScore[i][control_uid] > 0)
			{
				bIsControlPlayerIsJetton = true;
				break;
			}
		}
	}

	if (!bAreaCtrl && bIsControlPlayerIsJetton && game_count>0 && (control_type == GAME_CONTROL_WIN || control_type == GAME_CONTROL_LOST))
	{
		if (control_type == GAME_CONTROL_WIN)
		{
			bIsFalgControl = SetControlPlayerWin(control_uid);
		}
		if (control_type == GAME_CONTROL_LOST)
		{
			bIsFalgControl = SetControlPlayerLost(control_uid);
		}
		if (bIsFalgControl && m_tagControlPalyer.count>0)
		{
			/*m_tagControlPalyer.count--;
			if (m_tagControlPalyer.count==0)
			{
				m_tagControlPalyer.Init();
			}*/
			if (m_pHostRoom != NULL)
			{
				m_pHostRoom->SynControlPlayer(GetTableID(), m_tagControlPalyer.uid, -1, m_tagControlPalyer.type);
			}
		}
	}

	//bool bIsPoolScoreControl = false;
	//uint32  uSysWinPro = m_uSysWinPro;
	//uint32  uSysLostPro = m_uSysLostPro;
	//bool bIsSysWinPro = g_RandGen.RandRatio(uSysWinPro, PRO_DENO_10000);
	//bool bIsSysLostPro = g_RandGen.RandRatio(uSysLostPro, PRO_DENO_10000);

	tagJackpotScore tmpJackpotScore;
	if (m_pHostRoom != NULL)
	{
		tmpJackpotScore = m_pHostRoom->GetJackpotScoreInfo();
	}
	bool bIsPoolScoreControl = false;
	bool bIsSysWinPro = g_RandGen.RandRatio(tmpJackpotScore.uSysWinPro, PRO_DENO_10000);
	bool bIsSysLostPro = g_RandGen.RandRatio(tmpJackpotScore.uSysLostPro, PRO_DENO_10000);


	if (!bAreaCtrl && !bBrankerIsControl && !bIsControlPlayerIsJetton && tmpJackpotScore.iUserJackpotControl == 1 && tmpJackpotScore.lCurPollScore>tmpJackpotScore.lMaxPollScore && bIsSysLostPro) // �±�
	{
		bIsPoolScoreControl = true;
		if (bBrankerIsRobot)
		{
			SetLeisurePlayerWin();
		}
		else
		{
			SetPlayerBrankerWin();
		}
	}
	if (!bAreaCtrl && !bBrankerIsControl && !bIsControlPlayerIsJetton && tmpJackpotScore.iUserJackpotControl == 1 && tmpJackpotScore.lCurPollScore<tmpJackpotScore.lMinPollScore && bIsSysWinPro) // �Ա�
	{
		bIsPoolScoreControl = true;
		if (bBrankerIsRobot)
		{
			SetPlayerBrankerWin();
		}
		else
		{
			//SetLeisurePlayerLost();
			SetPlayerBrankerLost();
		}
	}

	// add by har
	bool bStockControl = false;
	if (!bAreaCtrl && !bBrankerIsControl && !bIsControlPlayerIsJetton && !bIsPoolScoreControl) {
		bStockControl = SetStockWinLose();
	} // add by har end

	if (!bAreaCtrl && !bBrankerIsControl && !bIsControlPlayerIsJetton && !bIsPoolScoreControl && !bStockControl && needRobotChangeCard && IsUserPlaceJetton())
	{
		SetRobotBankerWin(bBrankerIsRobot);
	}
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	LOG_DEBUG("robot win - roomid:%d,tableid:%d, bAreaCtrl:%d, bBrankerIsRobot:%d,m_robotBankerWinPro:%d,needRobotChangeCard:%d,bIsControlPlayerIsJetton:%d,control_uid:%d,control_type:%d,game_count:%d,bIsFalgControl:%d,bStockControl:%d,bIsDispatchTableCardStock:%d",
		m_pHostRoom->GetRoomID(), GetTableID(), bAreaCtrl, bBrankerIsRobot, m_robotBankerWinPro, needRobotChangeCard, bIsControlPlayerIsJetton, control_uid, control_type, game_count, bIsFalgControl, bStockControl, GetIsAllRobotOrPlayerJetton());
		
    /*else{// ��ע�������ʹ�С����
        WORD maxAreaIndex = ID_TIAN_MEN;
        for(WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
        {
            if(m_playerJettonScore[wAreaIndex] > m_playerJettonScore[maxAreaIndex]) {
                maxAreaIndex = wAreaIndex;
                continue;
            }
        }
        //����
        for(uint8 i=0;i<ID_HUANG_MEN;++i)
        {
            uint8 seat = i+1;
            uint8 mup = 0;
            if(m_GameLogic.CompareCard(m_cbTableCardArray[maxAreaIndex+1],5,m_cbTableCardArray[seat],5,mup) != 1)
            {
                uint8 tmp[5];
                memcpy(tmp,m_cbTableCardArray[seat],5);
                memcpy(m_cbTableCardArray[maxAreaIndex+1],m_cbTableCardArray[seat],5);
                memcpy(m_cbTableCardArray[seat],tmp,5);
            }
        }
    }*/

    //test
//    if(g_RandGen.RandRatio(50,100)){
//        SetRobotBankerLose();
//    }

 /*
    if(g_RandGen.RandRatio(10,20)){// test
    	BYTE temp1[5]={0x07,0x17,0x07,0x37,0x08};
    	BYTE temp2[5]={0x01,0x02,0x11,0x12,0x21};
    	BYTE temp3[]={ 0x1C,0x0C,0x2B,0x2C,0x1B};
    	BYTE temp4[]={ 0x27,0x02,0x22,0x0D,0x07};
    	BYTE temp5[]={ 0x16,0x14,0x1C,0x3B,0x3A};
    	memcpy(&m_cbTableCardArray[0][0], temp1, sizeof(m_cbTableCardArray[0]));
    	memcpy(&m_cbTableCardArray[1][0], temp2, sizeof(m_cbTableCardArray[1]));
    memcpy(&m_cbTableCardArray[2][0], temp3, sizeof(m_cbTableCardArray[2]));
    memcpy(&m_cbTableCardArray[3][0], temp4, sizeof(m_cbTableCardArray[3]));
    memcpy(&m_cbTableCardArray[4][0], temp5, sizeof(m_cbTableCardArray[3]));
}*/
    // test toney
    if(false){
        BYTE temp1[] = {0x0D, 0x1D, 0x2D, 0x3D, 0x01};// ը��
        BYTE temp2[] = {0x01, 0x02, 0x12, 0x04, 0x11};//
        BYTE temp3[] = {0x0B, 0x1B, 0x2B, 0x2C, 0x0C};
        for (uint16 i = 0; i < MAX_SEAT_INDEX; ++i)
        {
            uint8 testType = g_RandGen.RandUInt() % 3;
            if(testType == 0) {
                memcpy(&m_cbTableCardArray[i][0], temp1, sizeof(m_cbTableCardArray[i]));
            }else if(testType == 1){
                memcpy(&m_cbTableCardArray[i][0], temp2, sizeof(m_cbTableCardArray[i]));
            }else if(testType == 2){
                memcpy(&m_cbTableCardArray[i][0], temp3, sizeof(m_cbTableCardArray[i]));
            }
        }
    }
    return true;
}
//����ׯ��
void    CGameBaiNiuTable::SendApplyUser(CGamePlayer* pPlayer)
{
    net::msg_bainiu_apply_list msg;
   	for(uint32 nUserIdx=0; nUserIdx<m_ApplyUserArray.size(); ++nUserIdx)
	{
		CGamePlayer *pTmp = m_ApplyUserArray[nUserIdx];
		//ׯ���ж�
		if(pTmp == m_pCurBanker) 
            continue;        
        msg.add_player_ids(pTmp->GetUID());
        msg.add_apply_score(m_ApplyUserScore[pTmp->GetUID()]);
	}
    LOG_DEBUG("����ׯ���б�:%d",msg.player_ids_size());
    if(pPlayer){
       pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_APPLY_LIST);
    }else{
       SendMsgToAll(&msg,net::S2C_MSG_BAINIU_APPLY_LIST);
    }    
}
//����ׯ��
void    CGameBaiNiuTable::FlushApplyUserSort()
{
    if(m_ApplyUserArray.size() > 1)
    {
        for(uint32 i = 0; i < m_ApplyUserArray.size(); ++i)
        {
            for(uint32 j=i+1;j < m_ApplyUserArray.size();++j)
            {
                if(i != j && CompareApplyBankers(m_ApplyUserArray[i], m_ApplyUserArray[j]))
                {
                    CGamePlayer* pTmp = m_ApplyUserArray[i];
                    m_ApplyUserArray[i] = m_ApplyUserArray[j];
                    m_ApplyUserArray[j] = pTmp;
                }
            }
        }
    }
}
//����ׯ��
bool    CGameBaiNiuTable::ChangeBanker(bool bCancelCurrentBanker)
{
   	//�л���ʶ
	bool bChangeBanker = false;

	//ȡ����ǰ
	if(bCancelCurrentBanker)
	{
        CalcBankerScore();
		TakeTurns();
		bChangeBanker = true;
	}
	//��ׯ�ж�
	else if(m_pCurBanker != NULL)
	{
        //�Զ�����
        AutoAddBankerScore();                   
		//�����ж�
		if(m_needLeaveBanker || GetBankerTimeLimit() <= m_wBankerTime || m_lBankerScore < GetApplyBankerCondition())
		{				
            LOG_DEBUG("the timesout or the score less,you down banker:%d-%lld",m_wBankerTime,m_lBankerScore);
            CalcBankerScore();
			TakeTurns();	
			bChangeBanker       = true;
		}		
	}
	//ϵͳ��ׯ
	else if(m_pCurBanker == NULL && m_ApplyUserArray.size() != 0)
	{
		//�ֻ��ж�
		TakeTurns();
		bChangeBanker = true;
	}
	//�л��ж�
	if(bChangeBanker)
	{
		//���ñ���
		m_wBankerTime       = 0;
		m_lBankerWinScore   = 0;

		//������Ϣ
        net::msg_bainiu_change_banker msg;
        msg.set_banker_user(GetBankerUID());
        msg.set_banker_score(m_lBankerScore);
        
        SendMsgToAll(&msg,net::S2C_MSG_BAINIU_CHANGE_BANKER);

        SendApplyUser(NULL);
	}

	return bChangeBanker; 
}
//�ֻ��ж�
void    CGameBaiNiuTable::TakeTurns()
{
    vector<uint32> delIDs;
	for(uint32 i = 0; i < m_ApplyUserArray.size(); i++)
	{
		if(GetGameState() == net::TABLE_STATE_NIUNIU_FREE)
		{
			//��ȡ����
			CGamePlayer *pPlayer = m_ApplyUserArray[i];
            if(pPlayer->GetNetState() == 0){
                delIDs.push_back(pPlayer->GetUID());
                continue;
            }            
			if(m_ApplyUserScore[pPlayer->GetUID()] >= GetApplyBankerCondition())
			{
				m_pCurBanker            = m_ApplyUserArray[i];
                m_lBankerScore          = m_ApplyUserScore[pPlayer->GetUID()];
                m_lBankerBuyinScore     = m_lBankerScore;      //ׯ�Ҵ���
                m_lBankerInitBuyinScore = m_lBankerBuyinScore;
                
                m_bankerAutoAddScore    = m_mpApplyUserInfo[pPlayer->GetUID()];//�Զ�����
                m_needLeaveBanker       = false;
                
                RemoveApplyBanker(pPlayer->GetUID());
                StandUpBankerSeat(pPlayer); 

                m_BankerTimeLimit = CApplication::Instance().call<int>("bainiubankertime");
				break;
			}		
		}
	}
    for(uint16 i=0;i<delIDs.size();++i){
        RemoveApplyBanker(delIDs[i]);
    }    
}
//����ׯ��
void    CGameBaiNiuTable::CalcBankerScore() 
{
    if(m_pCurBanker == NULL)
        return;
    net::msg_bainiu_banker_calc_rep msg;
    msg.set_banker_time(m_wBankerTime);
    msg.set_win_count(m_wBankerWinTime);
    msg.set_buyin_score(m_lBankerBuyinScore);
    msg.set_win_score(m_lBankerWinScore);
    msg.set_win_max(m_lBankerWinMaxScore);
    msg.set_win_min(m_lBankerWinMinScore);

    m_pCurBanker->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_BANKER_CALC);
    
    int64 score = m_lBankerWinScore;
    int32 pro = 0;
    switch(m_wBankerTime)
    {
    case 3:
        pro = 8;
        break;
    case 4:
        pro = 6;
        break;
    case 5:
        pro = 5;
        break;
    default:
        break;
    }
    if(score > 200 && pro > 0)
    {
        int64 decScore = score*pro/PRO_DENO_100;
        LOG_DEBUG("��ǰ��ׯ�۷�:%lld",decScore);
        m_pCurBanker->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_FEE,2,0,-decScore,0,0,0,0);
    }

    LOG_DEBUG("the turn the banker win:%lld,rest:%lld,buyin:%lld",score,m_lBankerScore,m_lBankerBuyinScore);
    RemoveApplyBanker(m_pCurBanker->GetUID());

    //����ׯ��
	m_pCurBanker = NULL;     
    m_robotApplySize = g_RandGen.RandRange(4, 8);//��������������
    m_robotChairSize = g_RandGen.RandRange(5, 7);//��������λ��
    
    ResetGameData();    
}
//�Զ�����
void    CGameBaiNiuTable::AutoAddBankerScore()
{
    //��ׯ�ж�
	if(m_pCurBanker == NULL || m_bankerAutoAddScore == 0 || m_needLeaveBanker || GetBankerTimeLimit() <= m_wBankerTime)
        return;
	
	//int64 lBankerScore = m_lBankerScore;
	////�ж�Ǯ�Ƿ�
	//if(lBankerScore >= m_lBankerInitBuyinScore)
 //       return;
 //   int64 diffScore   = m_lBankerInitBuyinScore - lBankerScore;
 //   int64 canAddScore = GetPlayerCurScore(m_pCurBanker) - m_lBankerBuyinScore;
 //   if(canAddScore < diffScore){
 //       diffScore = canAddScore;
 //   }
 //   if((m_lBankerScore + diffScore) < GetApplyBankerCondition())
 //       return;
 //   
 //   m_lBankerBuyinScore += diffScore;
 //   m_lBankerScore      += diffScore;
 //   
 //   net::msg_bainiu_add_bankerscore_rep msg;
 //   msg.set_buyin_score(diffScore);
	int64 diffScore = m_lBankerInitBuyinScore - m_lBankerScore;
	int64 canAddScore = GetPlayerCurScore(m_pCurBanker) - m_lBankerScore;

	LOG_DEBUG("1 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,curScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, GetPlayerCurScore(m_pCurBanker), m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

	//�ж�Ǯ�Ƿ�
	if (diffScore <= 0)
	{
		return;
	}
	if (canAddScore <= 0)
	{
		return;
	}
	if (canAddScore < diffScore)
	{
		diffScore = canAddScore;
	}
	if ((m_lBankerBuyinScore + diffScore) < GetApplyBankerCondition())
	{
		return;
	}

	m_lBankerBuyinScore += diffScore;
	m_lBankerScore += diffScore;

	net::msg_paijiu_add_bankerscore_rep msg;
	msg.set_buyin_score(diffScore);

	LOG_DEBUG("5 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,curScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, GetPlayerCurScore(m_pCurBanker), m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

    m_pCurBanker->SendMsgToClient(&msg,net::S2C_MSG_BAINIU_ADD_BANKER_SCORE);             
}
//������Ϸ��¼
void  CGameBaiNiuTable::SendPlayLog(CGamePlayer* pPlayer)
{
    net::msg_bainiu_play_log_rep msg;
    for(uint16 i=0;i<m_vecRecord.size();++i)
    {
        net::bainiu_play_log* plog = msg.add_logs();
        bainiuGameRecord& record = m_vecRecord[i];
        for(uint16 j=0;j<AREA_COUNT;j++){
            plog->add_seats_win(record.wins[j]);
        }
    }
    LOG_DEBUG("�����ƾּ�¼:%d",msg.logs_size());
    if(pPlayer != NULL) {
        pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BAINIU_PLAY_LOG);
    }else{
        SendMsgToAll(&msg,net::S2C_MSG_BAINIU_PLAY_LOG);
    }
}
//�����ע
int64   CGameBaiNiuTable::GetUserMaxJetton(CGamePlayer* pPlayer, BYTE cbJettonArea)
{
	int iTimer = m_iMaxJettonRate;
	//����ע��
	int64 lNowJetton = 0;
	for(int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex){ 
        lNowJetton += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}
	//ׯ�ҽ��
	int64 lBankerScore = 2147483647;
	if(m_pCurBanker != NULL)
		lBankerScore = m_lBankerScore;
	for(int nAreaIndex = 0; nAreaIndex < AREA_COUNT; ++nAreaIndex) 
        lBankerScore -= m_allJettonScore[nAreaIndex]*iTimer;

	//��������
	int64 lMeMaxScore = (GetPlayerCurScore(pPlayer) - lNowJetton*iTimer)/iTimer;

	//ׯ������
	lMeMaxScore = min(lMeMaxScore,lBankerScore/iTimer);

	//��������
	lMeMaxScore = MAX(lMeMaxScore, 0);

	return (lMeMaxScore);
}
uint32  CGameBaiNiuTable::GetBankerUID()
{
    if(m_pCurBanker)
        return m_pCurBanker->GetUID();
    return 0;
}
//void    CGameBaiNiuTable::RemoveApplyBanker(uint32 uid)
//{
//    LOG_DEBUG("remove apply banker - uid:%d",uid);
//    for(uint32 i=0; i<m_ApplyUserArray.size(); ++i)
//	{
//		//��������
//		CGamePlayer* pPlayer = m_ApplyUserArray[i];
//		if(pPlayer->GetUID() == uid){
//    		//ɾ�����
//    		m_ApplyUserArray.erase(m_ApplyUserArray.begin()+i);
//            m_mpApplyUserInfo.erase(uid);
//            UnLockApplyScore(pPlayer);
//    		break;
//		}
//	}
//    FlushApplyUserSort();
//}
bool    CGameBaiNiuTable::LockApplyScore(CGamePlayer* pPlayer,int64 score)
{
	if (pPlayer == NULL) {
		return false;
	}

    if(GetPlayerCurScore(pPlayer) < score)
        return false;
	LOG_DEBUG("1 uid:%d,lCurScore:%lld,score:%lld", pPlayer->GetUID(), GetPlayerCurScore(pPlayer), score);
    ChangePlayerCurScore(pPlayer,-score);
	LOG_DEBUG("2 uid:%d,lCurScore:%lld,score:%lld", pPlayer->GetUID(), GetPlayerCurScore(pPlayer), score);
    m_ApplyUserScore[pPlayer->GetUID()] = score;

    return true;
}
bool    CGameBaiNiuTable::UnLockApplyScore(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL) {
		return false;
	}

    int64 lockScore = m_ApplyUserScore[pPlayer->GetUID()];
    ChangePlayerCurScore(pPlayer,lockScore);
    
    return true;
}    
//ׯ��վ��
void    CGameBaiNiuTable::StandUpBankerSeat(CGamePlayer* pPlayer)
{
    for(uint8 i=0;i < m_vecPlayers.size();++i)
    {
        if(m_vecPlayers[i].pPlayer == pPlayer)
        {
            m_vecPlayers[i].Reset();            
            LOG_DEBUG("standup banker seat:room:%d--tb:%d,chairID:%d,uid:%d",m_pHostRoom->GetRoomID(),GetTableID(),i,pPlayer->GetUID());
            AddLooker(pPlayer);
            OnActionUserStandUp(i,pPlayer);
        }
    }
}
bool    CGameBaiNiuTable::IsSetJetton(uint32 uid)
{
    //if(TABLE_STATE_NIUNIU_PLACE_JETTON != GetGameState())
    //    return false;
	if (TABLE_STATE_NIUNIU_GAME_END == GetGameState())
	{
		return false;
	}
	

    for(uint8 i=0;i<AREA_COUNT;++i){
        if(m_userJettonScore[i][uid] > 0)
            return true;
    }

	for (uint32 i = 0; i < m_chairRobotPlaceJetton.size(); i++)
	{
		uint32 temp_uid = 0;
		if (m_chairRobotPlaceJetton[i].pPlayer)
		{
			temp_uid = m_chairRobotPlaceJetton[i].pPlayer->GetUID();
		}
		if (uid == temp_uid) {
			return true;
		}
	}
	for (uint32 i = 0; i < m_RobotPlaceJetton.size(); i++)
	{
		uint32 temp_uid = 0;
		if (m_RobotPlaceJetton[i].pPlayer)
		{
			temp_uid = m_RobotPlaceJetton[i].pPlayer->GetUID();
		}
		if (uid == temp_uid) {
			return true;
		}
	}
    return false;    
}    
bool    CGameBaiNiuTable::IsInApplyList(uint32 uid)
{
	//�����ж�
	for(uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx)
	{
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if(id == uid){
			return true;
		}
	}
    return false;        
}

//����÷�
int64   CGameBaiNiuTable::CalculateScore()
{
	//�ƶ����
	bool static bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang;
	BYTE TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple;
	TianMultiple = 1;
	DiMultiple = 1;
	TianXuanltiple = 1;
	HuangMultiple = 1;
	DeduceWinner(bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple);

	LOG_DEBUG("win:%d-%d-%d-%d,multiple:%d-%d-%d-%d", bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple);

	BYTE  cbMultiple[] = { 1, 1, 1, 1 };
	cbMultiple[ID_TIAN_MEN] = TianMultiple;
	cbMultiple[ID_DI_MEN] = DiMultiple;
	cbMultiple[ID_XUAN_MEN] = TianXuanltiple;
	cbMultiple[ID_HUANG_MEN] = HuangMultiple;

	m_winMultiple[ID_TIAN_MEN] = bWinTianMen ? TianMultiple : -TianMultiple;
	m_winMultiple[ID_DI_MEN] = bWinDiMen ? DiMultiple : -DiMultiple;
	m_winMultiple[ID_XUAN_MEN] = bWinXuanMen ? TianXuanltiple : -TianXuanltiple;
	m_winMultiple[ID_HUANG_MEN] = bWinHuang ? HuangMultiple : -HuangMultiple;

	//��Ϸ��¼
	for (uint32 i = 0; i < AREA_COUNT; ++i)
	{
		m_record.wins[i] = m_winMultiple[i]>0 ? 1 : 0;
	}
	m_vecRecord.push_back(m_record);
	if (m_vecRecord.size() > 10) {//�������10��
		m_vecRecord.erase(m_vecRecord.begin());
	}

	for (uint8 i = 0; i < MAX_SEAT_INDEX; ++i) {
		m_cbTableCardType[i] = m_GameLogic.GetCardType(m_cbTableCardArray[i], 5);
		WriteOutCardLog(i, m_cbTableCardArray[i], 5, m_winMultiple[i]);
	}

	//ׯ������
	int64 lBankerWinScore = 0;

	//��ҳɼ�
	m_mpUserWinScore.clear();
	map<uint32, int64> mpUserLostScore;
	mpUserLostScore.clear();
	
	//ʤ����ʶ
	bool static bWinFlag[AREA_COUNT];
	bWinFlag[ID_TIAN_MEN] = bWinTianMen;
	bWinFlag[ID_DI_MEN] = bWinDiMen;
	bWinFlag[ID_XUAN_MEN] = bWinXuanMen;
	bWinFlag[ID_HUANG_MEN] = bWinHuang;

	bool bIsUserPlaceJetton = false;
	for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < ID_HUANG_MEN; ++wAreaIndex)
	{
		if (m_allJettonScore[wAreaIndex]>0)
		{
			bIsUserPlaceJetton = true;
		}
	}
	if (!bWinTianMen && !bWinDiMen && !bWinXuanMen && !bWinHuang && bIsUserPlaceJetton)
	{
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_TAKE_ALL;
	}
	else if (bWinTianMen && bWinDiMen && bWinXuanMen && bWinHuang && bIsUserPlaceJetton)
	{
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_COMPENSATION;
	}
	else
	{
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_NULL;
	}

	//������λ����
	for(WORD wChairID=0; wChairID<GAME_PLAYER; wChairID++)
	{
		//��ȡ�û�
		CGamePlayer * pPlayer = GetPlayer(wChairID);
        uint8 maxCardType = CT_POINT;
		if(pPlayer == NULL)continue;
		//int64 lResultScore = 0;
		for(WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
		{
            if(m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
                continue;
			int64 scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);
			if(true == bWinFlag[wAreaIndex])// Ӯ�� 
			{
				m_mpUserWinScore[pPlayer->GetUID()]    += scoreWin;
				lBankerWinScore -= scoreWin;
				//lResultScore -= scoreWin;
			}
			else// ����
			{
				mpUserLostScore[pPlayer->GetUID()] -= scoreWin;
				lBankerWinScore += m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex];
				//lResultScore += scoreWin;
			}
			
            WriteAddScoreLog(pPlayer->GetUID(),wAreaIndex,m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
            maxCardType = maxCardType < m_cbTableCardType[wAreaIndex+1] ? m_cbTableCardType[wAreaIndex+1] : maxCardType;
		}
		//�ܵķ���
		m_mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
        mpUserLostScore[pPlayer->GetUID()] = 0;
        WriteMaxCardType(pPlayer->GetUID(),maxCardType);
	}
    //�����Թ��߻���
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        uint8 maxCardType = CT_POINT;
        if(pPlayer == NULL)continue;
		//int64 lResultScore = 0;
		for(WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
        {
            if(m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
                continue;
			int64 scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);

            if(true == bWinFlag[wAreaIndex])// Ӯ��
            {
                m_mpUserWinScore[pPlayer->GetUID()]    += scoreWin;
                lBankerWinScore -= scoreWin;
				//lResultScore -= scoreWin;
            }
            else// ����
            {
                mpUserLostScore[pPlayer->GetUID()] -= scoreWin;
                lBankerWinScore += scoreWin;
				//lResultScore += scoreWin;
            }
			
            WriteAddScoreLog(pPlayer->GetUID(),wAreaIndex,m_userJettonScore[wAreaIndex][pPlayer->GetUID()]);
            maxCardType = maxCardType < m_cbTableCardType[wAreaIndex+1] ? m_cbTableCardType[wAreaIndex+1] : maxCardType;
        }
		//�ܵķ���
		m_mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
        mpUserLostScore[pPlayer->GetUID()] = 0;
        WriteMaxCardType(pPlayer->GetUID(),maxCardType);
    }  
	//�ۼƻ���
	m_lBankerWinScore    += lBankerWinScore;
	//��ǰ����
    m_lBankerScore       += lBankerWinScore;
    if(lBankerWinScore > 0)m_wBankerWinTime++;
    m_lBankerWinMaxScore = MAX(lBankerWinScore,m_lBankerWinMaxScore);
    m_lBankerWinMinScore = MIN(lBankerWinScore,m_lBankerWinMinScore);
    
	return lBankerWinScore;    
}

bool CGameBaiNiuTable::IsUserPlaceJetton()
{
	for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
	{
		CGamePlayer * pPlayer = GetPlayer(wChairID);
		if (pPlayer == NULL)continue;
		if (pPlayer->IsRobot())continue;
		for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
		{
			if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()]> 0)
			{
				return true;
			}
		}
	}
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL)continue;
		if (pPlayer->IsRobot())continue;
		for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
		{
			if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] > 0)
			{
				return true;
			}
		}
	}
	return false;
}

// �ƶ�Ӯ�Ҵ��� add by har
void CGameBaiNiuTable::DeduceWinnerDeal(bool &bWinTian, bool &bWinDi, bool &bWinXuan, bool &bWinHuan, uint8 &TianMultiple, uint8 &diMultiple, uint8 &TianXuanltiple, uint8 &HuangMultiple, uint8 cbTableCardArray[][5]) {
	//��С�Ƚ�
	bWinTian = m_GameLogic.CompareCard(cbTableCardArray[0], 5, cbTableCardArray[ID_TIAN_MEN + 1], 5, TianMultiple, 0, &m_tagNiuMultiple) == 1 ? true : false;
	bWinDi = m_GameLogic.CompareCard(cbTableCardArray[0], 5, cbTableCardArray[ID_DI_MEN + 1], 5, diMultiple, 0, &m_tagNiuMultiple) == 1 ? true : false;
	bWinXuan = m_GameLogic.CompareCard(cbTableCardArray[0], 5, cbTableCardArray[ID_XUAN_MEN + 1], 5, TianXuanltiple, 0, &m_tagNiuMultiple) == 1 ? true : false;
	bWinHuan = m_GameLogic.CompareCard(cbTableCardArray[0], 5, cbTableCardArray[ID_HUANG_MEN + 1], 5, HuangMultiple, 0, &m_tagNiuMultiple) == 1 ? true : false;
}

//�ƶ�Ӯ��
void    CGameBaiNiuTable::DeduceWinner(bool &bWinTian, bool &bWinDi, bool &bWinXuan,bool &bWinHuan,BYTE &TianMultiple,BYTE &diMultiple,BYTE &TianXuanltiple,BYTE &HuangMultiple )
{
	//��С�Ƚ�
	DeduceWinnerDeal(bWinTian, bWinDi, bWinXuan, bWinHuan, TianMultiple, diMultiple, TianXuanltiple, HuangMultiple, m_cbTableCardArray);
  
}
//��������
int64   CGameBaiNiuTable::GetApplyBankerCondition()
{
    return GetBaseScore();
}
int64   CGameBaiNiuTable::GetApplyBankerConditionLimit()
{
    return GetBaseScore()*20;
}
//��������
int32   CGameBaiNiuTable::GetBankerTimeLimit()
{
    return m_BankerTimeLimit;
}
//����ׯ�Ҷ�������
bool	CGameBaiNiuTable::CompareApplyBankers(CGamePlayer* pBanker1,CGamePlayer* pBanker2)
{
    if(m_ApplyUserScore[pBanker1->GetUID()] < m_ApplyUserScore[pBanker2->GetUID()])
        return true;

    return false;
}
void    CGameBaiNiuTable::OnRobotOper()
{
    //LOG_DEBUG("robot place jetton");
	/*
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
            continue;
        uint8 area = g_RandGen.RandRange(ID_TIAN_MEN,ID_HUANG_MEN);
        if(g_RandGen.RandRatio(30,PRO_DENO_100))
            continue;
		//int64 minJetton = GetUserMaxJetton(pPlayer,area)/10;
		//if (minJetton > 100)
		//{
		//	minJetton = (minJetton / 100) * 100;
		//}
		//else
		//{
		//	continue;
		//}
		int64 minJetton = GetRobotJettonScore(pPlayer, area);
		if (minJetton == 0)
		{
			continue;
		}
        if(!OnUserPlaceJetton(pPlayer,area,minJetton))
            break;
    }
	*/
	/*
    for(uint32 i=0;i<GAME_PLAYER;++i)
    {
        CGamePlayer* pPlayer = GetPlayer(i);
        if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
            continue;
		int iJettonCount = g_RandGen.RandRange(1, 9);
		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
		{
			uint8 area = g_RandGen.RandRange(ID_TIAN_MEN, ID_HUANG_MEN);
			if (g_RandGen.RandRatio(50, PRO_DENO_100))
				continue;
			//int64 minJetton = GetUserMaxJetton(pPlayer,area)/3;
			//if (minJetton > 100)
			//{
			//	minJetton = (minJetton / 100) * 100;
			//}
			//else
			//{
			//	continue;
			//}
			int64 minJetton = GetRobotJettonScore(pPlayer, area);
			if (minJetton == 0)
			{
				continue;
			}
			if (!OnUserPlaceJetton(pPlayer, area, minJetton))
				break;
		}
    }
	*/

}




bool CGameBaiNiuTable::OnChairRobotJetton()
{
	if (m_bIsChairRobotAlreadyJetton)
	{
		return false;
	}
	m_bIsChairRobotAlreadyJetton = true;
	m_chairRobotPlaceJetton.clear();
	for (uint32 i = 0; i < GAME_PLAYER; ++i)
	{
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		{
			continue;
		}
		int iJettonCount = g_RandGen.RandRange(5, 9);
		uint8 cbJettonArea = g_RandGen.RandRange(ID_TIAN_MEN, ID_HUANG_MEN);
		int64 lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
		if (lUserRealJetton == 0)
		{
			continue;
		}
		int iJettonTypeCount = g_RandGen.RandRange(1, 2);
		int iJettonStartCount = g_RandGen.RandRange(2, 3);
		int64 lOldRealJetton = -1;
		int64 iJettonOldTime = -1;
		
		bool bIsContinuouslyJetton = false;
		int iPreRatio = g_RandGen.RandRange(5, 10);
		if (g_RandGen.RandRatio(iPreRatio, PRO_DENO_100))
		{
			bIsContinuouslyJetton = true;

			if (lUserRealJetton == 100 || lUserRealJetton == 1000)
			{
				iJettonCount = g_RandGen.RandRange(5, 18);
			}
		}
		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
		{
			//if (g_RandGen.RandRatio(95, PRO_DENO_100))
			//{
			//	continue;
			//}

			if (bIsContinuouslyJetton == false)
			{
				cbJettonArea = g_RandGen.RandRange(ID_TIAN_MEN, ID_HUANG_MEN);

				lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
				if (lOldRealJetton == -1)
				{
					lOldRealJetton = lUserRealJetton;
				}
				if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 1)
				{
					lUserRealJetton = lOldRealJetton;
				}
				if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 2 && iJettonStartCount == iIndex)
				{
					lUserRealJetton = lUserRealJetton;
					lOldRealJetton = lUserRealJetton;
				}
				else
				{
					lUserRealJetton = lOldRealJetton;
				}
			}
			if (lUserRealJetton == 0)
			{
				continue;
			}

			tagRobotPlaceJetton robotPlaceJetton;
			robotPlaceJetton.uid = pPlayer->GetUID();
			robotPlaceJetton.pPlayer = pPlayer;
			int64 uRemainTime = m_coolLogic.getCoolTick();
			int64 passtick = m_coolLogic.getPassTick();
			int64 uMaxDelayTime = s_PlaceJettonTime;

			if (iJettonOldTime >= uMaxDelayTime - 500)
			{
				iJettonOldTime = g_RandGen.RandRange(100, 5000);
			}
			if (iJettonOldTime == -1)
			{
				robotPlaceJetton.time = g_RandGen.RandRange(100, uMaxDelayTime - 500);
				iJettonOldTime = robotPlaceJetton.time;
				if (bIsContinuouslyJetton == true)
				{
					robotPlaceJetton.time = g_RandGen.RandRange(100, 4000);
					iJettonOldTime = robotPlaceJetton.time;
				}
			}
			else
			{
				robotPlaceJetton.time = g_RandGen.RandRange(100, uMaxDelayTime - 500);
				iJettonOldTime = robotPlaceJetton.time;			
			}

			if (bIsContinuouslyJetton == true)
			{
				robotPlaceJetton.time = iJettonOldTime + 100;
				iJettonOldTime = robotPlaceJetton.time;
			}

			if (robotPlaceJetton.time <= 0 || robotPlaceJetton.time > uMaxDelayTime - 500)
			{
				continue;
			}
			robotPlaceJetton.area = cbJettonArea;
			robotPlaceJetton.jetton = lUserRealJetton;
			robotPlaceJetton.bflag = false;
			m_chairRobotPlaceJetton.push_back(robotPlaceJetton);
		}
	}
	LOG_DEBUG("chair_robot_jetton - roomid:%d,tableid:%d,m_chairRobotPlaceJetton.size:%d", GetRoomID(), GetTableID(), m_chairRobotPlaceJetton.size());

	return true;
}

void	CGameBaiNiuTable::OnChairRobotPlaceJetton()
{
	if (m_chairRobotPlaceJetton.size() == 0)
	{
		return;
	}
	vector<tagRobotPlaceJetton>	vecRobotPlaceJetton;
	for (uint32 i = 0; i < m_chairRobotPlaceJetton.size(); i++)
	{
		if (m_chairRobotPlaceJetton.size() == 0)
		{
			return;
		}
		tagRobotPlaceJetton robotPlaceJetton = m_chairRobotPlaceJetton[i];
		CGamePlayer * pPlayer = robotPlaceJetton.pPlayer;

		if (pPlayer == NULL)
		{
			continue;
		}
		if (m_chairRobotPlaceJetton.size() == 0)
		{
			return;
		}
		int64 passtick = m_coolLogic.getPassTick();
		int64 uMaxDelayTime = s_PlaceJettonTime;
		//passtick = passtick / 1000;
		bool bflag = false;
		bool bIsInTable = false;
		bool bIsJetton = false;
		for (uint32 i = 0; i < vecRobotPlaceJetton.size(); i++)
		{
			if (vecRobotPlaceJetton[i].pPlayer != NULL && vecRobotPlaceJetton[i].pPlayer->GetUID() == robotPlaceJetton.pPlayer->GetUID())
			{
				bIsJetton = true;
			}
		}

		if (passtick > robotPlaceJetton.time && uMaxDelayTime - passtick > 800 && m_chairRobotPlaceJetton[i].bflag == false && bIsJetton == false)
		{
			bIsInTable = IsInTableRobot(robotPlaceJetton.uid, robotPlaceJetton.pPlayer);
			if (bIsInTable)
			{
				bflag = OnUserPlaceJetton(robotPlaceJetton.pPlayer, robotPlaceJetton.area, robotPlaceJetton.jetton);
			}
			m_chairRobotPlaceJetton[i].bflag = bflag;
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
			//LOG_DEBUG("chair_robot_jetton - roomid:%d,tableid:%d,uid:%d,time:%d,passtick:%d,bIsInTable:%d,bIsJetton:%d,bflag:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), robotPlaceJetton.time, passtick, bIsInTable, bIsJetton, bflag);
		}
		if (m_chairRobotPlaceJetton[i].bflag == true)
		{
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
		}
	}

	for (uint32 i = 0; i < vecRobotPlaceJetton.size(); i++)
	{
		auto iter_begin = m_chairRobotPlaceJetton.begin();
		for (; iter_begin != m_chairRobotPlaceJetton.end(); iter_begin++)
		{
			if (iter_begin->bflag == true)
			{
				m_chairRobotPlaceJetton.erase(iter_begin);
				break;
			}
		}
	}
}

bool CGameBaiNiuTable::OnRobotJetton()
{
	//return false;
	if (m_bIsRobotAlreadyJetton)
	{
		return false;
	}
	m_bIsRobotAlreadyJetton = true;
	m_RobotPlaceJetton.clear();
	int64 iJettonOldTime = -1;
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* pPlayer = it->second;
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
			continue;
		if (g_RandGen.RandRatio(50, PRO_DENO_100))
		{
			continue;
		}
		int iJettonCount = g_RandGen.RandRange(1, 6);
		uint8 cbJettonArea = g_RandGen.RandRange(ID_TIAN_MEN, ID_HUANG_MEN);
		int64 lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
		if (lUserRealJetton == 0)
		{
			continue;
		}
		int iJettonTypeCount = g_RandGen.RandRange(1, 2);
		int iJettonStartCount = g_RandGen.RandRange(2, 3);
		int64 lOldRealJetton = -1;
		
		bool bIsContinuouslyJetton = false;
		int iPreRatio = g_RandGen.RandRange(5, 10);
		if (g_RandGen.RandRatio(iPreRatio, PRO_DENO_100))
		{
			bIsContinuouslyJetton = true;

			if (lUserRealJetton == 100 || lUserRealJetton == 1000)
			{
				iJettonCount = g_RandGen.RandRange(5, 18);
			}
		}

		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
		{
			if (bIsContinuouslyJetton == false)
			{
				cbJettonArea = g_RandGen.RandRange(ID_TIAN_MEN, ID_HUANG_MEN);

				lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
				if (lOldRealJetton == -1)
				{
					lOldRealJetton = lUserRealJetton;
				}
				if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 1)
				{
					lUserRealJetton = lOldRealJetton;
				}
				if (lOldRealJetton != lUserRealJetton && iJettonTypeCount == 2 && iJettonStartCount == iIndex)
				{
					lUserRealJetton = lUserRealJetton;
					lOldRealJetton = lUserRealJetton;
				}
				else
				{
					lUserRealJetton = lOldRealJetton;
				}
			}
			if (lUserRealJetton == 0)
			{
				continue;
			}

			tagRobotPlaceJetton robotPlaceJetton;
			robotPlaceJetton.uid = pPlayer->GetUID();
			robotPlaceJetton.pPlayer = pPlayer;
			int64 uRemainTime = m_coolLogic.getCoolTick();

			int64 passtick = m_coolLogic.getPassTick();
			int64 uMaxDelayTime = s_PlaceJettonTime;

			if (iJettonOldTime >= uMaxDelayTime - 500)
			{
				iJettonOldTime = g_RandGen.RandRange(100, 5000);
			}
			if (iJettonOldTime == -1)
			{
				robotPlaceJetton.time = g_RandGen.RandRange(100, 5000);
				iJettonOldTime = robotPlaceJetton.time;
			}
			else
			{
				robotPlaceJetton.time = iJettonOldTime + g_RandGen.RandRange(100, 2000);
				iJettonOldTime = robotPlaceJetton.time;
			}
			if (bIsContinuouslyJetton == true)
			{
				robotPlaceJetton.time = iJettonOldTime + 100;
				iJettonOldTime = robotPlaceJetton.time;
			}

			if (robotPlaceJetton.time <= 0 || robotPlaceJetton.time > uMaxDelayTime - 500)
			{
				continue;
			}

			robotPlaceJetton.area = cbJettonArea;
			robotPlaceJetton.jetton = lUserRealJetton;
			robotPlaceJetton.bflag = false;
			m_RobotPlaceJetton.push_back(robotPlaceJetton);
		}
	}

	return true;
}


void	CGameBaiNiuTable::OnRobotPlaceJetton()
{
	if (m_RobotPlaceJetton.size() == 0)
	{
		return;
	}
	vector<tagRobotPlaceJetton>	vecRobotPlaceJetton;

	for (uint32 i = 0; i < m_RobotPlaceJetton.size(); i++)
	{
		if (m_RobotPlaceJetton.size() == 0)
		{
			return;
		}
		tagRobotPlaceJetton robotPlaceJetton = m_RobotPlaceJetton[i];
		CGamePlayer * pPlayer = robotPlaceJetton.pPlayer;

		if (pPlayer == NULL) {
			continue;
		}
		if (m_RobotPlaceJetton.size() == 0)
		{
			return;
		}
		int64 passtick = m_coolLogic.getPassTick();
		int64 uMaxDelayTime = s_PlaceJettonTime;
		//passtick = passtick / 1000;
		bool bflag = false;
		bool bIsInTable = false;
		bool bIsJetton = false;
		for (uint32 i = 0; i < vecRobotPlaceJetton.size(); i++)
		{
			if (vecRobotPlaceJetton[i].pPlayer != NULL && vecRobotPlaceJetton[i].pPlayer->GetUID() == robotPlaceJetton.pPlayer->GetUID())
			{
				bIsJetton = true;
			}
		}
		if (robotPlaceJetton.time <= passtick && uMaxDelayTime - passtick > 500 && m_RobotPlaceJetton[i].bflag == false && bIsJetton == false)
		{
			bIsInTable = IsInTableRobot(robotPlaceJetton.uid, robotPlaceJetton.pPlayer);
			if (bIsInTable)
			{
				bflag = OnUserPlaceJetton(robotPlaceJetton.pPlayer, robotPlaceJetton.area, robotPlaceJetton.jetton);
			}
			m_RobotPlaceJetton[i].bflag = bflag;
			vecRobotPlaceJetton.push_back(robotPlaceJetton);
			//LOG_DEBUG("look_robot_jetton - roomid:%d,tableid:%d,uid:%d,time:%d,passtick:%d,bIsInTable:%d,bIsJetton:%d,bflag:%d", GetRoomID(), GetTableID(), pPlayer->GetUID(), robotPlaceJetton.time, passtick, bIsInTable, bIsJetton, bflag);
		}
	}

	for (uint32 i = 0; i < vecRobotPlaceJetton.size(); i++)
	{
		auto iter_begin = m_RobotPlaceJetton.begin();
		for (; iter_begin != m_RobotPlaceJetton.end(); iter_begin++)
		{
			if (iter_begin->bflag == true)
			{
				m_RobotPlaceJetton.erase(iter_begin);
				break;
			}
		}
	}
}



bool    CGameBaiNiuTable::IsInTableRobot(uint32 uid, CGamePlayer * pPlayer)
{
	for (uint32 i = 0; i<GAME_PLAYER; ++i)
	{
		if (pPlayer != NULL && pPlayer == GetPlayer(i) && pPlayer->GetUID() == uid)
		{
			return true;
		}
	}

	auto iter_player = m_mpLookers.find(uid);
	if (iter_player != m_mpLookers.end())
	{
		if (pPlayer != NULL && pPlayer == iter_player->second && pPlayer->GetUID() == iter_player->first)
		{
			return true;
		}
	}

	return false;
}

int64 CGameBaiNiuTable::GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area)
{
	int64 lUserRealJetton = 100;
	int64 lUserMinJetton = 100;
	int64 lUserMaxJetton = GetUserMaxJetton(pPlayer, area);
	int64 lUserCurJetton = GetPlayerCurScore(pPlayer);
	//LOG_DEBUG("uid:%d,lUserMinJetton:%lld,lUserMaxJetton:%lld,lUserRealJetton:%lld", pPlayer->GetUID(), lUserMinJetton, lUserMaxJetton, lUserRealJetton);
	
	if (lUserCurJetton < 2000)
	{
		lUserRealJetton = 100;
	}
	else if (lUserCurJetton>=2000 && lUserCurJetton < 50000)
	{
		if (g_RandGen.RandRatio(77, PRO_DENO_100))
		{
			lUserRealJetton = 100;
		}
		else if (g_RandGen.RandRatio(15, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else
		{
			lUserRealJetton = 5000;
		}
	}
	else if (lUserCurJetton>=50000 && lUserCurJetton < 200000)
	{
		if (g_RandGen.RandRatio(60, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(17, PRO_DENO_100))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(20, PRO_DENO_100))
		{
			lUserRealJetton = 10000;
		}
		else
		{
			lUserRealJetton = 50000;
		}
	}
	else if (lUserCurJetton>= 200000 && lUserCurJetton < 2000000)
	{
		if (g_RandGen.RandRatio(3470, PRO_DENO_10000))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(2500, PRO_DENO_10000))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(3500, PRO_DENO_10000))
		{
			lUserRealJetton = 10000;
		}
		else if (g_RandGen.RandRatio(450, PRO_DENO_10000))
		{
			lUserRealJetton = 50000;
		}
		else
		{
			lUserRealJetton = 200000;
		}
	}
	else if (lUserCurJetton >= 2000000)
	{
		if (g_RandGen.RandRatio(6300, PRO_DENO_10000))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(3000, PRO_DENO_10000))
		{
			lUserRealJetton = 10000;
		}
		else if (g_RandGen.RandRatio(550, PRO_DENO_10000))
		{
			lUserRealJetton = 50000;
		}
		else
		{
			lUserRealJetton = 200000;
		}
	}
	else
	{
		lUserRealJetton = 100;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 200000)
	{
		lUserRealJetton = 50000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 50000)
	{
		lUserRealJetton = 10000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 10000)
	{
		lUserRealJetton = 5000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 5000)
	{
		lUserRealJetton = 1000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 1000)
	{
		lUserRealJetton = 100;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 100)
	{
		lUserRealJetton = 0;
	}
	if (lUserRealJetton < lUserMinJetton)
	{
		lUserRealJetton = 0;
	}
	return lUserRealJetton;
}

void    CGameBaiNiuTable::OnRobotStandUp()
{
    // ����һ������λ�����    
    vector<uint16> emptyChairs;
    vector<uint16> robotChairs;
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        CGamePlayer* pPlayer = GetPlayer(i);
        if(pPlayer == NULL){
            emptyChairs.push_back(i);
            continue;
        }
        if(pPlayer->IsRobot()){
            robotChairs.push_back(i);
        }        
    }
    
    if(GetChairPlayerNum() > m_robotChairSize && robotChairs.size() > 0)// ������վ��
    {
        uint16 chairID = robotChairs[g_RandGen.RandUInt()%robotChairs.size()];
        CGamePlayer* pPlayer = GetPlayer(chairID);
        if(pPlayer != NULL && pPlayer->IsRobot() && CanStandUp(pPlayer))
        {
            PlayerSitDownStandUp(pPlayer,false,chairID);
            return;
        }                          
    }
    if(GetChairPlayerNum() < m_robotChairSize && emptyChairs.size() > 0)//����������
    {
        map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
        for(;it != m_mpLookers.end();++it)
        {
            CGamePlayer* pPlayer = it->second;
            if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
                continue;

            uint16 chairID = emptyChairs[g_RandGen.RandUInt()%emptyChairs.size()];
            if(CanSitDown(pPlayer,chairID)){
                PlayerSitDownStandUp(pPlayer,true,chairID);
                return;
            }      
        }            
    }
}
void 	CGameBaiNiuTable::CheckRobotCancelBanker()
{
    if(m_pCurBanker != NULL && m_pCurBanker->IsRobot())
    {
        if(m_wBankerTime > 3 && m_lBankerWinScore > m_lBankerBuyinScore/2)
        {
            if(g_RandGen.RandRatio(65,100)){
                OnUserCancelBanker(m_pCurBanker);
            }
        }
    }

}

void    CGameBaiNiuTable::GetAllRobotPlayer(vector<CGamePlayer*> & robots)
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

	for (WORD wUserIndex = 0; wUserIndex < GAME_PLAYER; ++wUserIndex)
	{
		CGamePlayer *pPlayer = GetPlayer(wUserIndex);
		if (pPlayer == NULL || !pPlayer->IsRobot())
		{
			continue;
		}
		robots.push_back(pPlayer);
	}

	//LOG_DEBUG("robots.size:%d", robots.size());
}


void    CGameBaiNiuTable::CheckRobotApplyBanker()
{   
	if (m_pCurBanker != NULL || m_ApplyUserArray.size() >= m_robotApplySize)
	{
		return;
	}
	uint32 roomid = 255;
	if (m_pHostRoom != NULL)
	{
		roomid = m_pHostRoom->GetRoomID();
	}
	vector<CGamePlayer*> robots;
	GetAllRobotPlayer(robots);

    LOG_DEBUG("robot apply banker - roomid:%d,tableid:%d,robots.size:%d, --------------------------------", roomid, GetTableID(), robots.size());

    //map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (uint32 uIndex = 0; uIndex < robots.size(); uIndex ++)
    {
		CGamePlayer* pPlayer = robots[uIndex];
		if (pPlayer == NULL || !pPlayer->IsRobot())
		{
			continue;
		}
        int64 curScore = GetPlayerCurScore(pPlayer);



		LOG_DEBUG("robot_ApplyBanker - roomid:%d,tableid:%d,uid:%d,curScore:%lld,GetApplyBankerCondition:%lld,m_ApplyUserArray.size:%d,", roomid,GetTableID(), pPlayer->GetUID(), curScore, GetApplyBankerCondition(), m_ApplyUserArray.size());

		if (curScore < GetApplyBankerCondition())
		{
			continue;
		}
        int64 buyinScore = GetApplyBankerCondition()*2;

        if(curScore < buyinScore)
        {
            buyinScore = curScore;
            buyinScore = (buyinScore/10000)*10000;
            OnUserApplyBanker(pPlayer,buyinScore,0);

        }
		else
		{
            buyinScore = g_RandGen.RandRange(buyinScore,curScore);
            buyinScore = (buyinScore/10000)*10000;

            OnUserApplyBanker(pPlayer,buyinScore,1);     
        }
		if (m_ApplyUserArray.size() > m_robotApplySize)
		{
			break;
		}
    }        
}

void    CGameBaiNiuTable::AddPlayerToBlingLog()
{
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL)
            continue;
        for(uint8 i=0;i<AREA_COUNT;++i){
            if(m_userJettonScore[i][pPlayer->GetUID()] > 0){
                AddUserBlingLog(pPlayer);
                break;
            }
        }           
    }
    AddUserBlingLog(m_pCurBanker);
}
//test 
void    CGameBaiNiuTable::TestMultiple()
{
    static bool isCalc = false;
    if(isCalc)return;

    isCalc = true;
    uint32 count = 1000000;
    map<uint32,uint32> mpType;
    map<uint32,uint32> mpMultip;
    for(uint32 k=0;k<count;++k)
    {
        DispatchTableCard();        
        for(uint8 i=0;i<5;++i)
        {            
            uint32 type = m_GameLogic.GetCardType(m_cbTableCardArray[i],5);                      
            mpType[type] = mpType[type] + 1;            
        }
        for(uint8 i=0;i<4;++i)
        {
            uint8 multip = 0;
            m_GameLogic.CompareCard(m_cbTableCardArray[0],5,m_cbTableCardArray[i+1],5,multip);
            mpMultip[multip] = mpMultip[multip] + 1;
        }
    }
    map<uint32,uint32>::iterator it = mpType.begin();
    LOG_DEBUG("�������ͷֲ�����:");
    for(;it != mpType.end();++it)
    {
        LOG_DEBUG("����:%d--����:%d",it->first,(it->second*2000)/count);        
    }
    it = mpMultip.begin();
    LOG_DEBUG("���Ա����ֲ�����:");
    uint32 multip = 0;
    for(;it != mpMultip.end();++it)
    {
        LOG_DEBUG("����:%d--����:%d",it->first,(it->second*2500)/count);
        multip += (it->first*it->second);
    }   
    LOG_DEBUG("��������:%d",(multip*10000)/(count*4));
        
}

void CGameBaiNiuTable::OnNewDay()
{
	m_uBairenTotalCount = 0;
	for (uint32 i = 0; i < m_vecAreaWinCount.size(); i++)
	{
		m_vecAreaWinCount[i] = 0;
	}
	for (uint32 i = 0; i < m_vecAreaLostCount.size(); i++)
	{
		m_vecAreaLostCount[i] = 0;
	}
}

void CGameBaiNiuTable::OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer)
{
	if (!pPlayer)
	{
		return;
	}
	LOG_DEBUG("send brc control all true player info list uid:%d.", pPlayer->GetUID());

	net::msg_brc_control_all_player_bet_info rep;

	//������λ���
	for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
	{
		//��ȡ�û�
		CGamePlayer * tmp_pPlayer = GetPlayer(wChairID);
		if (tmp_pPlayer == NULL)	continue;
		if (tmp_pPlayer->IsRobot())   continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		//ͳ����Ϣ
		auto iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end())
		{
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//��ע��Ϣ	
		uint64 total_bet = 0;
		for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}

	//�����Թ����
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* tmp_pPlayer = it->second;
		if (tmp_pPlayer == NULL)	continue;
		if (tmp_pPlayer->IsRobot())   continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		//ͳ����Ϣ
		auto iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end())
		{
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//��ע��Ϣ	
		uint64 total_bet = 0;
		for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ALL_PLAYER_BET_INFO);
}

void CGameBaiNiuTable::OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer)
{
	if (!pPlayer)
	{
		return;
	}

	if (pPlayer->IsRobot())   return;

	uint32 uid = pPlayer->GetUID();
	LOG_DEBUG("notice brc control single true player bet info uid:%d.", uid);

	net::msg_brc_control_single_player_bet_info rep;

	//������λ���
	for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
	{
		//��ȡ�û�
		CGamePlayer * tmp_pPlayer = GetPlayer(wChairID);
		if (tmp_pPlayer == NULL) continue;
		if (uid == tmp_pPlayer->GetUID())
		{
			net::brc_control_player_bet_info *info = rep.mutable_player_bet_info();
			info->set_uid(uid);
			info->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
			info->set_name(pPlayer->GetPlayerName());

			//ͳ����Ϣ
			auto iter = m_mpPlayerResultInfo.find(uid);
			if (iter != m_mpPlayerResultInfo.end())
			{
				info->set_curr_day_win(iter->second.day_win_coin);
				info->set_win_number(iter->second.win);
				info->set_lose_number(iter->second.lose);
				info->set_total_win(iter->second.total_win_coin);
			}
			//��ע��Ϣ	
			uint64 total_bet = 0;
			for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
			{
				info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
				total_bet += m_userJettonScore[wAreaIndex][uid];
			}
			info->set_total_bet(total_bet);
			info->set_ismaster(IsBrcControlPlayer(uid));
			break;
		}
	}

	//�����Թ����
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* tmp_pPlayer = it->second;
		if (tmp_pPlayer == NULL)continue;
		if (uid == tmp_pPlayer->GetUID())
		{
			net::brc_control_player_bet_info *info = rep.mutable_player_bet_info();
			info->set_uid(uid);
			info->set_coin(pPlayer->GetAccountValue(emACC_VALUE_COIN));
			info->set_name(pPlayer->GetPlayerName());

			//ͳ����Ϣ
			auto iter = m_mpPlayerResultInfo.find(uid);
			if (iter != m_mpPlayerResultInfo.end())
			{
				info->set_curr_day_win(iter->second.day_win_coin);
				info->set_win_number(iter->second.win);
				info->set_lose_number(iter->second.lose);
				info->set_total_win(iter->second.total_win_coin);
			}
			//��ע��Ϣ	
			uint64 total_bet = 0;
			for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
			{
				info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
				total_bet += m_userJettonScore[wAreaIndex][uid];
			}
			info->set_total_bet(total_bet);
			info->set_ismaster(IsBrcControlPlayer(uid));
			break;
		}
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_SINGLE_PLAYER_BET_INFO);
	}
}

void CGameBaiNiuTable::OnBrcControlSendAllRobotTotalBetInfo()
{
	LOG_DEBUG("notice brc control all robot totol bet info.");

	net::msg_brc_control_total_robot_bet_info rep;
	for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
	{
		rep.add_area_info(m_allJettonScore[wAreaIndex] - m_playerJettonScore[wAreaIndex]);
		LOG_DEBUG("wAreaIndex:%d m_allJettonScore[%d]:%lld m_playerJettonScore[d]:%lld", wAreaIndex, wAreaIndex, m_allJettonScore[wAreaIndex], wAreaIndex, m_playerJettonScore[wAreaIndex]);
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_ROBOT_BET_INFO);
	}
}

void CGameBaiNiuTable::OnBrcControlSendAllPlayerTotalBetInfo()
{
	LOG_DEBUG("notice brc control all player totol bet info.");

	net::msg_brc_control_total_player_bet_info rep;
	for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
	{
		rep.add_area_info(m_playerJettonScore[wAreaIndex]);
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_PLAYER_BET_INFO);
	}
}

bool CGameBaiNiuTable::OnBrcControlEnterControlInterface(CGamePlayer* pPlayer)
{
	if (!pPlayer)
		return false;

	LOG_DEBUG("brc control enter control interface. uid:%d", pPlayer->GetUID());

	bool ret = OnBrcControlPlayerEnterInterface(pPlayer);
	if (ret)
	{
		//ˢ�°��˳�����״̬
		m_brc_table_status_time = m_coolLogic.getCoolTick();
		OnBrcControlFlushTableStatus(pPlayer);

		//����������ʵ����б�
		OnBrcControlSendAllPlayerInfo(pPlayer);
		//���ͻ���������ע��Ϣ
		OnBrcControlSendAllRobotTotalBetInfo();
		//������ʵ�������ע��Ϣ
		OnBrcControlSendAllPlayerTotalBetInfo();
		//����������ׯ����б�
		OnBrcControlFlushAppleList();
		
		return true;
	}
	return false;
}

void CGameBaiNiuTable::OnBrcControlBetDeal(CGamePlayer* pPlayer)
{
	if (!pPlayer)
		return;

	LOG_DEBUG("brc control bet deal. uid:%d", pPlayer->GetUID());
	if (pPlayer->IsRobot())
	{
		//���ͻ���������ע��Ϣ
		OnBrcControlSendAllRobotTotalBetInfo();
	}
	else
	{
		//֪ͨ���������ע��Ϣ
		OnBrcControlNoticeSinglePlayerInfo(pPlayer);
		//������ʵ�������ע��Ϣ
		OnBrcControlSendAllPlayerTotalBetInfo();
	}
}

bool CGameBaiNiuTable::OnBrcAreaControl()
{
	LOG_DEBUG("brc area control.");

	if (m_real_control_uid == 0)
	{
		LOG_DEBUG("brc area control the control uid is zero.");
		return false;
	}

	//��ȡ��ǰ��������
	uint8 ctrl_area_a = AREA_MAX;	//A ���� ׯӮ/ׯ��

	bool ctrl_area_b = false;
	set<uint8> ctrl_area_b_list;	//B ���� ��/��/��/�� ֧�ֶ��
		
	for (uint8 i = 0; i < AREA_MAX; ++i)
	{
		if (m_req_control_area[i] == 1)
		{
			if (i == AREA_TIAN_MEN || i == AREA_DI_MEN || i == AREA_XUAN_MEN || i == AREA_HUANG_MEN)	//B �������
			{
				ctrl_area_b_list.insert(i);
				ctrl_area_b = true;
			}
			else    //A �������
			{
				ctrl_area_a = i;
			}
		}
	}

	if (!ctrl_area_b && ctrl_area_a == AREA_MAX)
	{
		LOG_DEBUG("brc area control the ctrl_area is none.");
		return false;
	}

	//�жϵ�ǰִ�еĿ�����A������B����
	if (ctrl_area_a != AREA_MAX)
	{
		return OnBrcAreaControlForA(ctrl_area_a);
	}

	if (ctrl_area_b && ctrl_area_b_list.size() <= AREA_COUNT)
	{
		return OnBrcAreaControlForB(ctrl_area_b_list);
	}

	return false;
}

void CGameBaiNiuTable::OnBrcFlushSendAllPlayerInfo()
{
	LOG_DEBUG("send brc flush all true player info list.");

	net::msg_brc_control_all_player_bet_info rep;

	//������λ���
	for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
	{
		//��ȡ�û�
		CGamePlayer * tmp_pPlayer = GetPlayer(wChairID);
		if (tmp_pPlayer == NULL)	continue;
		if (tmp_pPlayer->IsRobot())   continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		//ͳ����Ϣ
		auto iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end())
		{
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//��ע��Ϣ	
		uint64 total_bet = 0;
		for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}

	//�����Թ����
	map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
	for (; it != m_mpLookers.end(); ++it)
	{
		CGamePlayer* tmp_pPlayer = it->second;
		if (tmp_pPlayer == NULL)	continue;
		if (tmp_pPlayer->IsRobot())   continue;
		uint32 uid = tmp_pPlayer->GetUID();

		net::brc_control_player_bet_info *info = rep.add_player_bet_list();
		info->set_uid(uid);
		info->set_coin(tmp_pPlayer->GetAccountValue(emACC_VALUE_COIN));
		info->set_name(tmp_pPlayer->GetPlayerName());

		//ͳ����Ϣ
		auto iter = m_mpPlayerResultInfo.find(uid);
		if (iter != m_mpPlayerResultInfo.end())
		{
			info->set_curr_day_win(iter->second.day_win_coin);
			info->set_win_number(iter->second.win);
			info->set_lose_number(iter->second.lose);
			info->set_total_win(iter->second.total_win_coin);
		}
		//��ע��Ϣ	
		uint64 total_bet = 0;
		for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex < AREA_COUNT; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ALL_PLAYER_BET_INFO);
	}
}

bool CGameBaiNiuTable::OnBrcAreaControlForB(set<uint8> &area_list)
{
	uint8 area_size = area_list.size();
	LOG_DEBUG("brc area control for B. area_size:%d", area_size);

	for (uint8 area_id : area_list)
	{
		LOG_DEBUG("brc area control for B. id:%d", area_id);
	}
	
	//�����ƴӴ�Сλ��˳�� 
	uint8 uArSortCardIndex[MAX_SEAT_INDEX] = { 0 };
	GetCardSortIndex(uArSortCardIndex);

	BYTE cbTableCard[MAX_SEAT_INDEX][5] = { {0} };
	
	//���������û��� ���þ�ΪӮ������Ϊ��
	uint8 front = 0;
	uint8 back = 0;
	uint8 banker = area_size;
	for (uint8 i= ID_TIAN_MEN;i< AREA_COUNT;i++)
	{		
		bool isfind = false;
		set<uint8>::iterator iter;
		iter = area_list.find(i);
		if (iter != area_list.end())
		{
			isfind = true;
		}
		else
		{
			isfind = false;
		}

		//���ΪӮ����ȡ����
		if (isfind)
		{
			memcpy(cbTableCard[i + 1], m_cbTableCardArray[uArSortCardIndex[front]], 5);
			front++;
		}
		else   //���Ϊ�䣬��ȡС��
		{
			memcpy(cbTableCard[i + 1], m_cbTableCardArray[uArSortCardIndex[MAX_SEAT_INDEX- back -1]], 5);
			back++;
		}		
	}

	//����ׯ�ҵ���
	memcpy(cbTableCard[0], m_cbTableCardArray[uArSortCardIndex[banker]], 5);

	//���ݿ��ƽ������
	memcpy(m_cbTableCardArray, cbTableCard, sizeof(m_cbTableCardArray));

	LOG_DEBUG("brc area control for B. front:%d back:%d banker:%d", front, back, banker);

	return true;
}

bool CGameBaiNiuTable::OnBrcAreaControlForA(uint8 ctrl_area_a)
{
	LOG_DEBUG("brc area control for A. ctrl_area_b:%d", ctrl_area_a);

	//ׯӮ
	if (ctrl_area_a == AREA_BANK)
	{
		LOG_DEBUG("get area ctrl A is success - roomid:%d,tableid:%d,ctrl_area_b:%d", m_pHostRoom->GetRoomID(), GetTableID(), ctrl_area_a);
		return SetControlBankerScore(true);
	}

	//��Ӯ
	if (ctrl_area_a == AREA_XIAN)
	{
		LOG_DEBUG("get area ctrl A is success - roomid:%d,tableid:%d,ctrl_area_b:%d", m_pHostRoom->GetRoomID(), GetTableID(), ctrl_area_a);
		return SetControlBankerScore(false);
	}	
	return true;
}

bool CGameBaiNiuTable::SetControlBankerScore(bool isWin)
{
	LOG_DEBUG("Set Control Banker Score - isWin:%d", isWin);

	BYTE    cbTableCard[CARD_COUNT];

	uint32 bankeruid = GetBankerUID();

	int64 banker_score = 0;

	int irount_count = 1000;
	int iRountIndex = 0;

	for (; iRountIndex < irount_count; iRountIndex++)
	{
		bool static bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang;
		BYTE TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple;
		TianMultiple = 1;
		DiMultiple = 1;
		TianXuanltiple = 1;
		HuangMultiple = 1;
		DeduceWinner(bWinTianMen, bWinDiMen, bWinXuanMen, bWinHuang, TianMultiple, DiMultiple, TianXuanltiple, HuangMultiple);

		BYTE  cbMultiple[] = { 1, 1, 1, 1 };
		cbMultiple[ID_TIAN_MEN] = TianMultiple;
		cbMultiple[ID_DI_MEN] = DiMultiple;
		cbMultiple[ID_XUAN_MEN] = TianXuanltiple;
		cbMultiple[ID_HUANG_MEN] = HuangMultiple;

		//ʤ����ʶ
		bool static bWinFlag[AREA_COUNT];
		bWinFlag[ID_TIAN_MEN] = bWinTianMen;
		bWinFlag[ID_DI_MEN] = bWinDiMen;
		bWinFlag[ID_XUAN_MEN] = bWinXuanMen;
		bWinFlag[ID_HUANG_MEN] = bWinHuang;

		//������λ����
		for (WORD wChairID = 0; wChairID < GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
					continue;
				if (true == bWinFlag[wAreaIndex])// Ӯ��
				{
					banker_score -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);

				}
				else// ����
				{
					banker_score += m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex];
				}
			}
		}

		//�����Թ��߻���
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			for (WORD wAreaIndex = ID_TIAN_MEN; wAreaIndex <= ID_HUANG_MEN; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
					continue;
				if (true == bWinFlag[wAreaIndex])// Ӯ��
				{
					banker_score -= (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex]);

				}
				else// ����
				{
					banker_score += m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * cbMultiple[wAreaIndex];
				}
			}
		}

		if ((isWin && banker_score > 0) || (!isWin && banker_score < 0)) 
		{
			break;
		}
		else {
			//����ϴ��
			m_GameLogic.RandCardList(cbTableCard, getArrayLen(cbTableCard));
			//�����˿�
			memcpy(&m_cbTableCardArray[0][0], cbTableCard, sizeof(m_cbTableCardArray));
		}
	}
	LOG_DEBUG("set banker win or lose - roomid:%d,tableid:%d,bankeruid:%d,banker_score:%lld,isWin:%d", m_pHostRoom->GetRoomID(), GetTableID(), bankeruid, banker_score, isWin);
	if (iRountIndex >= irount_count)
	{
		return false;
	}
	return true;
}

void CGameBaiNiuTable::OnNotityForceApplyUser(CGamePlayer* pPlayer)
{
	LOG_DEBUG("Notity Force Apply uid:%d.", pPlayer->GetUID());

	net::msg_bainiu_apply_banker_rep msg;
	msg.set_apply_oper(0);
	msg.set_result(net::RESULT_CODE_SUCCESS);

	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BAINIU_APPLY_BANKER);
}