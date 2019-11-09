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
using namespace game_baccarat;

namespace
{
    const static uint32 s_FreeTime              = 5*1000;       // ����ʱ��
    const static uint32 s_PlaceJettonTime       = 15*1000;      // ��עʱ��    
    const static uint32 s_DispatchTime          = 20*1000;      // ����ʱ��

	const static float  s_Multiple[] = { 1.0, 8.0, 1.0, 11.0, 11.0, 12.0, 1.5, 0.5 };//������multiple
	const static int32  s_MultipleTime[] = { 1,8,1,11,11,12,1,2 };//������multiple
    
	//const static uint32 s_SysLostWinProChange	= 500;			// �Ա��±Ҹ��ʱ仯
	//const static int64  s_UpdateJackpotScore    = 100000;		// ���½��ط���
};

CGameTable* CGameRoom::CreateTable(uint32 tableID)
{
    CGameTable* pTable = NULL;
    switch(m_roomCfg.roomType)
    {
    case emROOM_TYPE_COMMON:           // �ټ���
        {
            pTable = new CGameBaccaratTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_MATCH:            // �����ټ���
        {
            pTable = new CGameBaccaratTable(this,tableID,emTABLE_TYPE_SYSTEM);
        }break;
    case emROOM_TYPE_PRIVATE:          // ˽�˷��ټ���
        {
            pTable = new CGameBaccaratTable(this,tableID,emTABLE_TYPE_PLAYER);
        }break;
    default:
        {
            assert(false);
            return NULL;
        }break;
    }
    return pTable;
}

CGameBaccaratTable::CGameBaccaratTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType)
:CGameTable(pRoom,tableID,tableType)
{
    m_vecPlayers.clear();

	//����ע��
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
    memset(m_playerJettonScore,0,sizeof(m_playerJettonScore));

	//������ע
	for(uint8 i=0;i<AREA_MAX;++i){
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
	m_lBankerShowScore		= 0L;
    m_poolCards.clear();
	m_imake_card_count = 0;
	m_inotmake_card_count = 0;
	m_inotmake_card_round = 0;
	m_imax_notmake_round = 18;

	m_playerBankerLosePro = 0;
	//m_lMaxPollScore = 0;
	//m_lMinPollScore = 0;
	//m_lCurPollScore = 0;
	//m_lFrontPollScore = 0;
	//m_uSysWinPro = 0;
	//m_uSysLostPro = 0;

	m_lGameRount = 0;
	
	m_tagControlPalyer.Init();

	m_bIsChairRobotAlreadyJetton = false;
	m_bIsRobotAlreadyJetton = false;
	m_chairRobotPlaceJetton.clear();
	m_RobotPlaceJetton.clear();

    return;
}
CGameBaccaratTable::~CGameBaccaratTable()
{

}
bool    CGameBaccaratTable::CanEnterTable(CGamePlayer* pPlayer)
{
	if (pPlayer == NULL) {
		LOG_DEBUG("player pointer is null");
		return false;
	}
	if (pPlayer->GetTable() != NULL) {
		LOG_DEBUG("player table is not null - uid:%d,ptable:%p", pPlayer->GetUID(), pPlayer->GetTable());
		return false;
	}
    // �޶����
    if(IsFullTable() || GetPlayerCurScore(pPlayer) < GetEnterMin()){
		LOG_DEBUG("game table limit - uid:%d,is_full:%d,cur_score:%lld,min_score:%lld", pPlayer->GetUID(), IsFullTable(), GetPlayerCurScore(pPlayer), GetEnterMin());
        return false;
    }
    return true;
}
bool    CGameBaccaratTable::CanLeaveTable(CGamePlayer* pPlayer)
{
    if(m_pCurBanker == pPlayer || IsSetJetton(pPlayer->GetUID()))
        return false;
    if(IsInApplyList(pPlayer->GetUID()))
        return false;
    
    return true;
}
bool    CGameBaccaratTable::CanSitDown(CGamePlayer* pPlayer,uint16 chairID)
{
    return CGameTable::CanSitDown(pPlayer,chairID);
}
bool    CGameBaccaratTable::CanStandUp(CGamePlayer* pPlayer)
{
    return true;
}
bool    CGameBaccaratTable::IsFullTable()
{
    if(m_mpLookers.size() >= 100)
        return true;
    
    return false;
}
void CGameBaccaratTable::GetTableFaceInfo(net::table_face_info* pInfo)
{
    net::baccarat_table_info* pbaccarat = pInfo->mutable_baccarat();
    pbaccarat->set_tableid(GetTableID());
    pbaccarat->set_tablename(m_conf.tableName);
    if(m_conf.passwd.length() > 1){
        pbaccarat->set_is_passwd(1);
    }else{
        pbaccarat->set_is_passwd(0);
    }
    pbaccarat->set_hostname(m_conf.hostName);
    pbaccarat->set_basescore(m_conf.baseScore);
    pbaccarat->set_consume(m_conf.consume);
    pbaccarat->set_entermin(m_conf.enterMin);
    pbaccarat->set_duetime(m_conf.dueTime);
    pbaccarat->set_feetype(m_conf.feeType);
    pbaccarat->set_feevalue(m_conf.feeValue);
    pbaccarat->set_card_time(s_PlaceJettonTime);
    pbaccarat->set_table_state(GetGameState());
    pbaccarat->set_sitdown(m_pHostRoom->GetSitDown());
    pbaccarat->set_apply_banker_condition(GetApplyBankerCondition());
    pbaccarat->set_apply_banker_maxscore(GetApplyBankerConditionLimit());
    pbaccarat->set_banker_max_time(m_BankerTimeLimit);
    
}

//��������
bool CGameBaccaratTable::Init()
{
    SetGameState(net::TABLE_STATE_NIUNIU_FREE);

    m_vecPlayers.resize(GAME_PLAYER);
    for(uint8 i=0;i<GAME_PLAYER;++i)
    {
        m_vecPlayers[i].Reset();
    }
    m_BankerTimeLimit = 3;
    m_BankerTimeLimit = CApplication::Instance().call<int>("bainiubankertime");

    m_robotApplySize = g_RandGen.RandRange(4, 8);//��������������
    m_robotChairSize = g_RandGen.RandRange(3, 6);//��������λ��
	
	ReAnalysisParam();
	CRobotOperMgr::Instance().PushTable(this);
	SetMaxChairNum(GAME_PLAYER); // add by har
    return true;
}

bool CGameBaccaratTable::ReAnalysisParam() {
	string param = m_pHostRoom->GetCfgParam();
	Json::Reader reader;
	Json::Value  jvalue;
	if (!reader.parse(param, jvalue))
	{
		LOG_ERROR("reader json parse error - param:%s", param.c_str());
		return true;
	}
	if (jvalue.isMember("pbl")) {
		m_playerBankerLosePro = jvalue["pbl"].asInt();
	}
	if (jvalue.isMember("mnr")) {
		m_imax_notmake_round = jvalue["mnr"].asInt();
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
	//int iIsUpdateSysPro = 0;
	//if (jvalue.isMember("usp")) {
	//	iIsUpdateSysPro = jvalue["usp"].asInt();
	//}
	//if (iIsUpdateSysPro == 1)
	//{
	//	if (jvalue.isMember("swp")) {
	//		m_uSysWinPro = jvalue["swp"].asInt();
	//	}
	//	if (jvalue.isMember("slp")) {
	//		m_uSysLostPro = jvalue["slp"].asInt();
	//	}
	//}



	//int64 lDiffMaxScore = m_lCurPollScore - m_lMaxPollScore;
	//int64 lDiffMinScore = m_lMinPollScore - m_lCurPollScore;

	//LOG_ERROR("1 - roomid:%d,tableid:%d,m_playerBankerLosePro:%d,m_lMaxPollScore:%lld,m_lMinPollScore:%lld,iIsUpdateCurPollScore:%d,m_lCurPollScore:%lld,m_uSysWinPro:%d,m_uSysLostPro:%d,lDiffMaxScore:%lld,lDiffMinScore:%lld",
	//	m_pHostRoom->GetRoomID(), GetTableID(), m_playerBankerLosePro, m_lMaxPollScore, m_lMinPollScore, iIsUpdateCurPollScore, m_lCurPollScore, m_uSysWinPro, m_uSysLostPro, lDiffMaxScore, lDiffMinScore);


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

	//	} while(lDiffMaxScore >= s_UpdateJackpotScore);
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

	//LOG_ERROR("2 - roomid:%d,tableid:%d,m_playerBankerLosePro:%d,m_lMaxPollScore:%lld,m_lMinPollScore:%lld,iIsUpdateCurPollScore:%d,m_lCurPollScore:%lld,m_uSysWinPro:%d,m_uSysLostPro:%d,lDiffMaxScore:%lld,lDiffMinScore:%lld,m_imax_notmake_round:%d",
	//	m_pHostRoom->GetRoomID(), GetTableID(), m_playerBankerLosePro, m_lMaxPollScore, m_lMinPollScore, iIsUpdateCurPollScore, m_lCurPollScore, m_uSysWinPro, m_uSysLostPro, lDiffMaxScore, lDiffMinScore, m_imax_notmake_round);
	
	LOG_ERROR("2 - roomid:%d,tableid:%d,m_playerBankerLosePro:%d,m_imax_notmake_round:%d",
		m_pHostRoom->GetRoomID(), GetTableID(), m_playerBankerLosePro, m_imax_notmake_round);


	return true;
}



void CGameBaccaratTable::ShutDown()
{

}
//��λ����
void CGameBaccaratTable::ResetTable()
{
    ResetGameData();
}
void CGameBaccaratTable::OnTimeTick()
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
					InitChessID();
                    SetGameState(TABLE_STATE_NIUNIU_PLACE_JETTON);
					m_chairRobotPlaceJetton.clear();

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

				m_brc_table_status = emTABLE_STATUS_FREE;
				m_brc_table_status_time = 0;
                
				//ͬ��ˢ�°��˳����ƽ��������״̬��Ϣ
				OnBrcControlFlushTableStatus();
            }break;
        default:
            break;
        }
    }
    if(tableState == TABLE_STATE_NIUNIU_PLACE_JETTON && m_coolLogic.getPassTick() > 1000)
    {
		OnChairRobotJetton();
		OnRobotJetton();
    }      	
}

void CGameBaccaratTable::OnRobotTick()
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
int CGameBaccaratTable::OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
    LOG_DEBUG("table recv msg:%d--%d", pPlayer->GetUID(),cmdID);     
    switch(cmdID)
    {
    case net::C2S_MSG_BACCARAT_PLACE_JETTON:  // �û���ע
        {
            if(GetGameState() != TABLE_STATE_NIUNIU_PLACE_JETTON){
                LOG_DEBUG("not jetton state can't jetton");
                return 0;
            }            
            net::msg_baccarat_place_jetton_req msg;
            PARSE_MSG_FROM_ARRAY(msg);                      
            return OnUserPlaceJetton(pPlayer,msg.jetton_area(),msg.jetton_score());
        }break;
    case net::C2S_MSG_BACCARAT_APPLY_BANKER:  // ����ׯ��
        {
            net::msg_baccarat_apply_banker msg;
            PARSE_MSG_FROM_ARRAY(msg);
            if(msg.apply_oper() == 1){
                return OnUserApplyBanker(pPlayer,msg.apply_score(),msg.auto_addscore());
            }else{
                return OnUserCancelBanker(pPlayer);
            }
        }break;
    case net::C2S_MSG_BACCARAT_JUMP_APPLY_QUEUE:// ���
        {
            net::msg_baccarat_jump_apply_queue_req msg;
            PARSE_MSG_FROM_ARRAY(msg);
            
            return OnUserJumpApplyQueue(pPlayer);
        }break;
	case net::C2S_MSG_BACCARAT_CONTINUOUS_PRESSURE_REQ://
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
bool CGameBaccaratTable::OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin)
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
		//LockApplyScore(pPlayer, lockScore);
		//int64 lafterScore = GetPlayerCurScore(pPlayer);
		LOG_DEBUG("uid:%d,isJoin:%d,lockScore:%lld,lCurScore:%lld", uid, isJoin, lockScore, lCurScore);

    }else{
        pPlayer->SetPlayDisconnect(true);
    }
    return true;
}
//�û�����
bool CGameBaccaratTable::OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer)
{
    
    SendSeatInfoToClient();
    return true;
}
//�û�����
bool CGameBaccaratTable::OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer)
{

    SendSeatInfoToClient();
    return true;
}
// ��Ϸ��ʼ
bool CGameBaccaratTable::OnGameStart()
{
    LOG_DEBUG("game start:%d-%d",m_pHostRoom->GetRoomID(),GetTableID());
    if(m_pCurBanker == NULL){
        LOG_ERROR("the banker is null");
        CheckRobotApplyBanker();
        ChangeBanker(false);
        return false;
    }
    m_coolLogic.beginCooling(s_PlaceJettonTime);
    
    net::msg_baccarat_start_rep gameStart;
    gameStart.set_time_leave(m_coolLogic.getCoolTick());
    gameStart.set_banker_score(m_lBankerScore);
    gameStart.set_banker_id(GetBankerUID());
    gameStart.set_banker_buyin_score(m_lBankerBuyinScore);
 
    SendMsgToAll(&gameStart,net::S2C_MSG_BACCARAT_START);
	OnTableGameStart();
    OnRobotStandUp();
    return true;
}
//��Ϸ����
bool CGameBaccaratTable::OnGameEnd(uint16 chairID,uint8 reason)
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
			int64 playerAllWinScore = 0; // �����Ӯ��
			if (IsBankerRealPlayer())
				playerAllWinScore = lBankerWinScore;
			// add by har
			int64 robotWinScore = 0; // ��������Ӯ��
			int64 playerFees = 0;   // ����ܳ�ˮ
			if (m_pCurBanker) {
				if (m_pCurBanker->IsRobot())
					robotWinScore = lBankerWinScore;
				else
					playerFees = -bankerfee;
			} // add by har end
			LOG_DEBUG("breanker_score - roomid:%d,tableid:%d,uid:%d,m_lBankerScore:%lld,curScore:%lld,m_wBankerTime:%d,robotWinScore:%lld,playerFees:%lld,lBankerWinScore:%lld,bankerfee:%lld,m_cbWinArea:%d-%d-%d-%d-%d-%d-%d-%d,m_cbTableCardArray:%d-%d-%d-%d-%d-%d",
				GetRoomID(), GetTableID(), GetBankerUID(), m_lBankerScore, GetPlayerCurScore(m_pCurBanker), m_wBankerTime, robotWinScore,
				playerFees, lBankerWinScore, bankerfee, m_cbWinArea[0], m_cbWinArea[1], m_cbWinArea[2], m_cbWinArea[3], m_cbWinArea[4],
				m_cbWinArea[5], m_cbWinArea[6], m_cbWinArea[7], m_cbTableCardArray[0][0], m_cbTableCardArray[0][1], m_cbTableCardArray[0][2], m_cbTableCardArray[1][0], m_cbTableCardArray[1][1], m_cbTableCardArray[1][2]);
			lBankerWinScore += bankerfee;
			//��������
			m_wBankerTime++;		


			//������Ϣ
            net::msg_baccarat_game_end msg;
            for(uint8 i=0;i<MAX_SEAT_INDEX;++i){
                net::msg_cards* pCards = msg.add_table_cards();
				//LOG_DEBUG("table card count - tableid:%d,i:%d,m_cbCardCount:%d", GetTableID(), i, m_cbCardCount[i]);

                for(uint8 j=0;j<m_cbCardCount[i];++j){
					//LOG_DEBUG("table card data - tableid:%d,i:%d,m_cbCardCount:%d,j:%d,card_data:0x%02X", GetTableID(), i, m_cbCardCount[i], j, m_cbTableCardArray[i][j]);

                    pCards->add_cards(m_cbTableCardArray[i][j]);
                }
            }
            for(uint8 i=0;i<MAX_SEAT_INDEX;++i){
                msg.add_card_types(m_cbTableCardType[i]);
            }
            for(uint8 i=0;i<AREA_MAX;++i){
                msg.add_win_types(m_cbWinArea[i]);
            }

            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(lBankerWinScore);
            msg.set_banker_total_score(m_lBankerWinScore);                                 
            msg.set_remain_card(m_poolCards.size());
			msg.set_settle_accounts_type(m_cbBrankerSettleAccountsType);
			//}
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
				if (pPlayer == NULL)continue;

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
				pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BACCARAT_GAME_END);

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
                pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_GAME_END);

				//��׼����ͳ��
				OnBrcControlSetResultInfo(pPlayer->GetUID(), lUserScoreFree);
				if (!pPlayer->IsRobot())
					playerAllWinScore += lUserScoreFree; // add by har end

				//LOG_DEBUG("Calc Player Info looker - tableid:%d,uid:%d,time_leave:%d,banker_time:%d,banker_win_score:%lld,banker_total_score:%lld,remain_card:%d,user_score:%lld,m_mpUserWinScore[%lld]",
				//	GetTableID(), pPlayer->GetUID(), msg.time_leave(), msg.banker_time(), msg.banker_win_score(), msg.banker_total_score(), msg.remain_card(), msg.user_score(), m_mpUserWinScore[pPlayer->GetUID()]);
            }

			//�����ǰׯ��Ϊ��ʵ��ң���Ҫ���¾�׼����ͳ��
			if (m_pCurBanker && !m_pCurBanker->IsRobot())
			{
				LOG_DEBUG("set banker result info  roomid:%d,tableid:%d,banker uid:%lld,lBankerWinScore:%lld", m_pHostRoom->GetRoomID(), GetTableID(), GetBankerUID(), lBankerWinScore);
				OnBrcControlSetResultInfo(GetBankerUID(), lBankerWinScore);
			}

			m_mpUserWinScore[GetBankerUID()] = 0;   
            SaveBlingLog();


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

			//int64 lCurPollScore = m_lCurPollScore;

			if (m_pCurBanker != NULL && m_pCurBanker->IsRobot())
			{
				//m_lCurPollScore -= lPlayerScoreResult;
				if (m_pHostRoom != NULL && lPlayerScoreResult != 0)
				{
					m_pHostRoom->UpdateJackpotScore(-lPlayerScoreResult);
				}
			}
			if (m_pCurBanker != NULL && !m_pCurBanker->IsRobot())
			{
				//m_lCurPollScore += lRobotScoreResult;
				if (m_pHostRoom != NULL && lRobotScoreResult != 0)
				{
					m_pHostRoom->UpdateJackpotScore(lRobotScoreResult);
				}
			}
			

			LOG_DEBUG("OnGameEnd2 roomid:%d,tableid:%d,lPlayerScoreResult:%lld,lRobotScoreResult:%lld,robotWinScore:%lld,playerFees:%lld,m_allJettonScore:%d-%d-%d-%d-%d-%d-%d-%d--%d",
				m_pHostRoom->GetRoomID(), GetTableID(), lPlayerScoreResult, lRobotScoreResult, robotWinScore, playerFees, 
				m_allJettonScore[0], m_allJettonScore[1], m_allJettonScore[2], m_allJettonScore[3], m_allJettonScore[4], m_allJettonScore[5], m_allJettonScore[6], m_allJettonScore[7],
				m_allJettonScore[0] + m_allJettonScore[1] + m_allJettonScore[2] + m_allJettonScore[3] + m_allJettonScore[4] + m_allJettonScore[5] + m_allJettonScore[6] + m_allJettonScore[7]);
			m_pHostRoom->UpdateStock(this, playerAllWinScore); // add by har
			//ͬ������������ݵ��ض�
			OnBrcFlushSendAllPlayerInfo();

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
void  CGameBaccaratTable::OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer)
{
    LOG_DEBUG("player join:%d--%d",chairID,isJoin);
    CGameTable::OnPlayerJoin(isJoin,chairID,pPlayer);
    if(isJoin){
        SendApplyUser(pPlayer);
        SendGameScene(pPlayer);
        SendPlayLog(pPlayer);
    }else{
        OnUserCancelBanker(pPlayer);        
        RemoveApplyBanker(pPlayer->GetUID());
        for(uint8 i=0;i<AREA_MAX;++i){
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
void    CGameBaccaratTable::SendGameScene(CGamePlayer* pPlayer)
{
    LOG_DEBUG("send game scene:%d", pPlayer->GetUID());
    switch(m_gameState)
    {
    case net::TABLE_STATE_NIUNIU_FREE:          // ����״̬
        {
            net::msg_baccarat_game_info_free_rep msg;
            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_banker_id(GetBankerUID());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(m_lBankerWinScore);
            msg.set_banker_score(m_lBankerScore);
            msg.set_banker_buyin_score(m_lBankerBuyinScore);
			//msg.set_banker_show_score(m_lBankerShowScore);
           
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_GAME_FREE_INFO);
        }break;
    case net::TABLE_STATE_NIUNIU_PLACE_JETTON:  // ��Ϸ״̬
    case net::TABLE_STATE_NIUNIU_GAME_END:      // ����״̬
        {
            net::msg_baccarat_game_info_play_rep msg;
            for(uint8 i=0;i<AREA_MAX;++i){
                msg.add_all_jetton_score(m_allJettonScore[i]);
            }            
            msg.set_banker_id(GetBankerUID());
            msg.set_banker_time(m_wBankerTime);
            msg.set_banker_win_score(m_lBankerWinScore);
            msg.set_banker_score(m_lBankerScore);
            msg.set_banker_buyin_score(m_lBankerBuyinScore);
            msg.set_time_leave(m_coolLogic.getCoolTick());
            msg.set_game_status(m_gameState);
            for(uint8 i=0;i<AREA_MAX;++i){
                msg.add_self_jetton_score(m_userJettonScore[i][pPlayer->GetUID()]);
            }       
			//msg.set_banker_show_score(m_lBankerShowScore);
			if (GetBankerUID() == pPlayer->GetUID() && m_needLeaveBanker)
			{
				msg.set_need_leave_banker(1);
			}
			else
			{
				msg.set_need_leave_banker(0);
			}
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_GAME_PLAY_INFO);

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
void CGameBaccaratTable::IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {
	for (uint16 wAreaIndex = 0; wAreaIndex < AREA_MAX; ++wAreaIndex) {
		if (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] == 0)
			continue;
		if (pPlayer->IsRobot())
			isAllPlayer = false;
		else
		    isAllRobot = false;
		return;
	}
}

int64    CGameBaccaratTable::CalcPlayerInfo(uint32 uid,int64 winScore,bool isBanker)
{
    if(winScore == 0)
        return 0;
    LOG_DEBUG("report to lobby - uid:%d,winScore:%lld",uid,winScore);
	int64 fee = CalcPlayerGameInfo(uid, winScore, 0, true);
	if (isBanker) {// ׯ�ҳ��Ӽ�����Ӧ����,������Ŀ��ƽ
		m_lBankerWinScore += fee;
		//��ǰ����
		m_lBankerScore += fee;
	}

	LOG_DEBUG("calc player score alfter - uid:%d,winScore:%lld,fee:%lld,isBanker:%d", uid, winScore, fee, isBanker);

	return fee;

}
// ������Ϸ����
void    CGameBaccaratTable::ResetGameData()
{
	//����ע��
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
    memset(m_playerJettonScore,0,sizeof(m_playerJettonScore));
	//������ע
	for(uint8 i=0;i<AREA_MAX;++i){
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
void    CGameBaccaratTable::ClearTurnGameData()
{
	//����ע��
	memset(m_allJettonScore,0,sizeof(m_allJettonScore));
	memset(m_playerJettonScore, 0, sizeof(m_playerJettonScore));
	
	//������ע
	for(uint8 i=0;i<AREA_MAX;++i){
	    m_userJettonScore[i].clear();
	}
	//��ҳɼ�	
	m_mpUserWinScore.clear();       
	//�˿���Ϣ
	memset(m_cbTableCardArray,0,sizeof(m_cbTableCardArray));     
}
// д�����log
void    CGameBaccaratTable::WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int32 mulip)
{
    Json::Value logValue;
    logValue["p"]       = chairID;
    logValue["m"]       = mulip;
    for(uint32 i=0;i<cardCount;++i){
        logValue["c"].append(cardData[i]);
    }
    m_operLog["card"].append(logValue);
}
// д���עlog
void    CGameBaccaratTable::WriteAddScoreLog(uint32 uid,uint8 area,int64 score,int64 win)
{
    if(score == 0)
        return;
    
    Json::Value logValue;
    logValue["uid"]  = uid;
    logValue["p"]    = area;
    logValue["s"]    = score;
    logValue["win"]  = win;
    
    m_operLog["op"].append(logValue);
}
// д��ׯ����Ϣ
void    CGameBaccaratTable::WriteBankerInfo()
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
bool    CGameBaccaratTable::OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore)
{
	if (pPlayer == NULL) {
		return false;
	}
    LOG_DEBUG("player place jetton- uid:%d--%d--%lld",pPlayer->GetUID(),cbJettonArea,lJettonScore);
    //Ч�����
	if(cbJettonArea >= AREA_MAX || lJettonScore==0){
        
        LOG_DEBUG("jetton is error - uid:%d - %d--%lld", pPlayer->GetUID(),cbJettonArea,lJettonScore);
		return false;
	}
	if(GetGameState() != net::TABLE_STATE_NIUNIU_PLACE_JETTON){
        net::msg_baccarat_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_FAIL);
        
		//������Ϣ
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_PLACE_JETTON_REP);
		return false;
	}
	//ׯ���ж�
	if(pPlayer->GetUID() == GetBankerUID()){
        LOG_DEBUG("the banker can't jetton- uid:%d", pPlayer->GetUID());
		return false;
	}

	//��������
	int64 lJettonCount = 0L;
	for(int nAreaIndex = 0; nAreaIndex < AREA_MAX; ++nAreaIndex)
        lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];

	//��һ���
	int64 lUserScore = GetPlayerCurScore(pPlayer);

	//�Ϸ�У��
	if(lUserScore < lJettonCount + lJettonScore) 
    {
        LOG_DEBUG("the jetton more than you have - uid:%d", pPlayer->GetUID());
        return true;
	}
	if (TableJettonLimmit(pPlayer, lJettonScore, lJettonCount) == false)
	{
		//bPlaceJettonSuccess = false;
		LOG_DEBUG("table_jetton_limit - roomid:%d,tableid:%d,uid:%d,curScore:%lld,jettonmin:%lld",
			GetRoomID(), GetTableID(), pPlayer->GetUID(), GetPlayerCurScore(pPlayer), GetJettonMin());
		net::msg_baccarat_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_FAIL);

		//������Ϣ
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BACCARAT_PLACE_JETTON_REP);
		return true;
	}
	//�ɹ���ʶ
	bool bPlaceJettonSuccess=true;
	//�Ϸ���֤
	if(GetUserMaxJetton(pPlayer, cbJettonArea) >= lJettonScore)
	{
		//������ע
		m_allJettonScore[cbJettonArea] += lJettonScore;
		m_userJettonScore[cbJettonArea][pPlayer->GetUID()] += lJettonScore;
        if(!pPlayer->IsRobot()){
            m_playerJettonScore[cbJettonArea] += lJettonScore;
        }
	}else{
	    LOG_DEBUG("the jetton more than limit - uid:%d", pPlayer->GetUID());
		bPlaceJettonSuccess = false;
	}
	if(bPlaceJettonSuccess)
	{
		//ˢ�°��˳����ƽ������ע��Ϣ
		OnBrcControlBetDeal(pPlayer);

		RecordPlayerBaiRenJettonInfo(pPlayer, cbJettonArea, lJettonScore);
		OnAddPlayerJetton(pPlayer->GetUID(), lJettonScore);

        net::msg_baccarat_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_SUCCESS);        
		//������Ϣ
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_PLACE_JETTON_REP);

        net::msg_baccarat_place_jetton_broadcast broad;
        broad.set_uid(pPlayer->GetUID());
        broad.set_jetton_area(cbJettonArea);
        broad.set_jetton_score(lJettonScore);
        broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);
        
        SendMsgToAll(&broad,net::S2C_MSG_BACCARAT_PLACE_JETTON_BROADCAST);
        
	}
	else
	{
        net::msg_baccarat_place_jetton_rep msg;
        msg.set_jetton_area(cbJettonArea);
        msg.set_jetton_score(lJettonScore);
        msg.set_result(net::RESULT_CODE_FAIL);
        
		//������Ϣ
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_PLACE_JETTON_REP);
		return false;
        
	}    
    return true;
}
//����ׯ��
bool    CGameBaccaratTable::OnUserApplyBanker(CGamePlayer* pPlayer,int64 bankerScore,uint8 autoAddScore)
{
	if (pPlayer == NULL) {
		LOG_DEBUG("player apply banker - pPlayer is null");
		return false;
	}
	LOG_DEBUG("player apply banker - uid:%d,autoAddScore:%d,score:%lld", pPlayer->GetUID(), autoAddScore, bankerScore);
        //�������
    net::msg_baccarat_apply_banker_rep msg;
    msg.set_apply_oper(1);
    msg.set_buyin_score(bankerScore);
    
    if(m_pCurBanker == pPlayer){
        LOG_DEBUG("you is the banker - uid:", pPlayer->GetUID());
        msg.set_result(net::RESULT_CODE_ERROR_STATE);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_APPLY_BANKER);
        return false;
    }
    if(IsSetJetton(pPlayer->GetUID())){// ��ע������ׯ
		LOG_DEBUG("you is jettoning - uid:", pPlayer->GetUID());
        msg.set_result(net::RESULT_CODE_ERROR_STATE);
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_APPLY_BANKER);
        return false;
    }    
    //�Ϸ��ж�
	int64 lUserScore = GetPlayerCurScore(pPlayer);
    if(bankerScore > lUserScore){
        LOG_DEBUG("you not have more score - uid:%d,lUserScore:%lld,bankerScore:%lld",pPlayer->GetUID(),lUserScore,bankerScore);
        msg.set_result(net::RESULT_CODE_ERROR_PARAM);    
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_APPLY_BANKER);
        return false;    
    }
    
	if(bankerScore < GetApplyBankerCondition() || bankerScore > GetApplyBankerConditionLimit()){
		LOG_DEBUG("you score less than condition - uid:%d,bankerScore:%lld,ApplyBankerCondition:%lld,ApplyBankerConditionLimit:%lld - faild", pPlayer->GetUID(), bankerScore,GetApplyBankerCondition(),GetApplyBankerConditionLimit());
        msg.set_result(net::RESULT_CODE_ERROR_PARAM);
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_APPLY_BANKER);
		return false;
	}

	//�����ж�
	for(uint32 nUserIdx = 0; nUserIdx < m_ApplyUserArray.size(); ++nUserIdx)
	{
		uint32 id = m_ApplyUserArray[nUserIdx]->GetUID();
		if(id == pPlayer->GetUID())
		{
			LOG_DEBUG("you is in apply list - uid:%d", pPlayer->GetUID());
            msg.set_result(net::RESULT_CODE_ERROR_STATE);    
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_APPLY_BANKER);
			return false;
		}
	}

	//������Ϣ 
	m_ApplyUserArray.push_back(pPlayer);
    m_mpApplyUserInfo[pPlayer->GetUID()] = autoAddScore;
    LockApplyScore(pPlayer,bankerScore);        

    msg.set_result(net::RESULT_CODE_SUCCESS);    
    pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_APPLY_BANKER);
	//�л��ж�
	if(GetGameState() == net::TABLE_STATE_NIUNIU_FREE && m_ApplyUserArray.size() == 1)
	{
		ChangeBanker(false);
	} 
    SendApplyUser();

	//ˢ�¿��ƽ������ׯ�б�
	OnBrcControlFlushAppleList();

    return true;
}
bool    CGameBaccaratTable::OnUserJumpApplyQueue(CGamePlayer* pPlayer)
{
    LOG_DEBUG("player jump queue:%d",pPlayer->GetUID());
    int64 cost = CDataCfgMgr::Instance().GetJumpQueueCost();  
    net::msg_baccarat_jump_apply_queue_rep msg;
    if(pPlayer->GetAccountValue(emACC_VALUE_COIN) < cost){
        LOG_DEBUG("the jump cost can't pay:%lld",cost);
        msg.set_result(net::RESULT_CODE_CION_ERROR);
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_JUMP_APPLY_QUEUE);
        return false;
    }
    LOG_DEBUG("jump apply queue %lld,%lld",pPlayer->GetAccountValue(emACC_VALUE_COIN),cost);
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
            
			cost = -cost;
			pPlayer->SyncChangeAccountValue(emACCTRAN_OPER_TYPE_JUMPQUEUE, 0, 0, cost, 0, 0, 0, 0);

            SendApplyUser(NULL);
            msg.set_result(net::RESULT_CODE_SUCCESS);
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_JUMP_APPLY_QUEUE);

			//ˢ�¿��ƽ������ׯ�б�
			OnBrcControlFlushAppleList();

            LOG_DEBUG("after jump apply queue %lld,%lld",pPlayer->GetAccountValue(emACC_VALUE_COIN),cost);
			return true;
		}
	}      
    
    return false;
}

bool    CGameBaccaratTable::OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg)
{
	//LOG_DEBUG("player place jetton:%d--%d--%lld",pPlayer->GetUID(),cbJettonArea,lJettonScore);
	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,branker:%d,GetGameState:%d,info_size:%d",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), GetBankerUID(), GetGameState(), msg.info_size());

	net::msg_player_continuous_pressure_jetton_rep rep;
	rep.set_result(net::RESULT_CODE_FAIL);
	if (msg.info_size() == 0 || GetGameState() != net::TABLE_STATE_NIUNIU_PLACE_JETTON || pPlayer->GetUID() == GetBankerUID())
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BACCARAT_CONTINUOUS_PRESSURE_REP);

		return false;
	}
	//Ч�����
	int64 lTotailScore = 0;
	for (int i = 0; i < msg.info_size(); i++)
	{
		net::bairen_jetton_info info = msg.info(i);

		if (info.score() <= 0 || info.area()>=AREA_MAX)
		{
			pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BACCARAT_CONTINUOUS_PRESSURE_REP);
			return false;
		}
		lTotailScore += info.score();
	}

	//��������
	int64 lJettonCount = 0L;
	for (int nAreaIndex = 0; nAreaIndex < AREA_MAX; ++nAreaIndex)
	{
		lJettonCount += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}

	//��һ���
	int64 lUserScore = GetPlayerCurScore(pPlayer);
	//�Ϸ�У��
	if (lUserScore < lJettonCount + lTotailScore)
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BACCARAT_CONTINUOUS_PRESSURE_REP);

		LOG_DEBUG("the jetton more than you have - uid:%d", pPlayer->GetUID());

		return true;
	}
	//�ɹ���ʶ
	//bool bPlaceJettonSuccess = true;
	int64 lUserMaxHettonScore = GetUserMaxJetton(pPlayer, 0);
	if (lUserMaxHettonScore < lTotailScore)
	{
		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BACCARAT_CONTINUOUS_PRESSURE_REP);

		LOG_DEBUG("error_pressu - uid:%d,lUserMaxHettonScore:%lld,lUserScore:%lld,lJettonCount:%lld,, lTotailScore:%lld,, GetJettonMin:%lld",
			pPlayer->GetUID(), lUserMaxHettonScore, lUserScore, lJettonCount, lTotailScore, GetJettonMin());

		return false;
	}

	//for (int i = 0; i < msg.info_size(); i++)
	//{
	//	net::bairen_jetton_info info = msg.info(i);
	//	if (info.score() <= 0 || info.area()>=AREA_MAX)
	//	{
	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BACCARAT_CONTINUOUS_PRESSURE_REP);
	//		return false;
	//	}
	//	if (GetUserMaxJetton(pPlayer, info.area()) < info.score())
	//	{
	//		pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BACCARAT_CONTINUOUS_PRESSURE_REP);
	//		return false;
	//	}
	//}

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

		net::msg_baccarat_place_jetton_rep msg;
		msg.set_jetton_area(cbJettonArea);
		msg.set_jetton_score(lJettonScore);
		msg.set_result(net::RESULT_CODE_SUCCESS);
		//������Ϣ
		pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BACCARAT_PLACE_JETTON_REP);

		net::msg_baccarat_place_jetton_broadcast broad;
		broad.set_uid(pPlayer->GetUID());
		broad.set_jetton_area(cbJettonArea);
		broad.set_jetton_score(lJettonScore);
		broad.set_total_jetton_score(m_allJettonScore[cbJettonArea]);

		SendMsgToAll(&broad, net::S2C_MSG_BACCARAT_PLACE_JETTON_BROADCAST);
	}

	rep.set_result(net::RESULT_CODE_SUCCESS);
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BACCARAT_CONTINUOUS_PRESSURE_REP);

	//ˢ�°��˳����ƽ������ע��Ϣ
	OnBrcControlBetDeal(pPlayer);

	return true;
}

//ȡ������
bool    CGameBaccaratTable::OnUserCancelBanker(CGamePlayer* pPlayer)
{
    LOG_DEBUG("cance banker:%d",pPlayer->GetUID());

    net::msg_baccarat_apply_banker_rep msg;
    msg.set_apply_oper(0);
    msg.set_result(net::RESULT_CODE_SUCCESS);
        
    //��ǰׯ�� 
	if(pPlayer->GetUID() == GetBankerUID() && GetGameState() != net::TABLE_STATE_NIUNIU_FREE)
	{
		//������Ϣ
		LOG_DEBUG("the game is run,you can't cance banker");
        m_needLeaveBanker = true;
        pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_APPLY_BANKER);
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
            pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_APPLY_BANKER);
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

bool    CGameBaccaratTable::DispatchRandTableCard()
{
	for (uint8 i = 0; i < 2; ++i)
	{
		m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
		m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
	}

	//�״η���
	m_cbCardCount[INDEX_PLAYER] = 2;
	m_cbCardCount[INDEX_BANKER] = 2;


	//�������
	BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
	BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

	//�мҲ���
	BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
	if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
	{
		//�������
		m_cbCardCount[INDEX_PLAYER]++;
		m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
		cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);

	}
	//ׯ�Ҳ���
	if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
	{
		switch (cbBankerCount)
		{
		case 0:
		case 1:
		case 2:
		{
			m_cbCardCount[INDEX_BANKER]++;
		}
		break;
		case 3:
		{
			if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
				m_cbCardCount[INDEX_PLAYER] == 2) {
				m_cbCardCount[INDEX_BANKER]++;
			}
		}
		break;
		case 4:
		{
			if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
				cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
				m_cbCardCount[INDEX_BANKER]++;
			}
		}
		break;
		case 5:
		{
			if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
				cbPlayerThirdCardValue != 3
				&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
				m_cbCardCount[INDEX_PLAYER] == 2) {
				m_cbCardCount[INDEX_BANKER]++;
			}
		}
		break;
		case 6:
		{
			if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
			{
				m_cbCardCount[INDEX_BANKER]++;
			}

		}
		break;
		//���벹��
		case 7:
		case 8:
		case 9:
			break;
		default:
			break;
		}
	}
	if (m_cbCardCount[INDEX_BANKER] == 3)
	{
		m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
	}

	return true;
}

bool    CGameBaccaratTable::DispatchTableCard()
{
	//DispatchRandTableCard();

	bool bAreaIsControl = false;
	bool bBrankerIsRobot = true;
	bool bBrankerIsControl = false;
	bool bIsControlPlayerIsJetton = false;
	bool bIsFalgControl = false;

	uint32 control_uid = m_tagControlPalyer.uid;
	uint32 game_count = m_tagControlPalyer.count;
	uint32 control_type = m_tagControlPalyer.type;
	
	bAreaIsControl = OnBrcAreaControl();

	if (!bAreaIsControl && m_pCurBanker != NULL)
	{
		bBrankerIsRobot = m_pCurBanker->IsRobot();
		if (control_uid == m_pCurBanker->GetUID())
		{
			bBrankerIsControl = true;
		}
	}

	if (!bAreaIsControl && bBrankerIsControl && game_count>0 && (control_type == GAME_CONTROL_WIN || control_type == GAME_CONTROL_LOST))
	{
		if (control_type == GAME_CONTROL_WIN)
		{
			bIsFalgControl = SetBrankerWin();
		}
		if (control_type == GAME_CONTROL_LOST)
		{
			bIsFalgControl = SetBrankerLost();
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

	if (!bAreaIsControl && !bBrankerIsControl && control_uid != 0 && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		for (uint8 i = AREA_XIAN; i < AREA_MAX; ++i)
		{
			if (m_userJettonScore[i][control_uid] > 0)
			{
				bIsControlPlayerIsJetton = true;
				break;
			}
		}
	}

	if (!bAreaIsControl && bIsControlPlayerIsJetton && game_count>0 && control_type != GAME_CONTROL_CANCEL)
	{
		if (control_type == GAME_CONTROL_WIN)
		{
			bIsFalgControl = DispatchTableCardControlPalyerWin(control_uid);
		}
		if (control_type == GAME_CONTROL_LOST)
		{
			bIsFalgControl = DispatchTableCardControlPalyerLost(control_uid);
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


	if (!bAreaIsControl && !bBrankerIsControl && !bIsControlPlayerIsJetton && tmpJackpotScore.iUserJackpotControl==1 && tmpJackpotScore.lCurPollScore>tmpJackpotScore.lMaxPollScore && bIsSysLostPro) // �±�
	{
		bIsPoolScoreControl = true;
		if (bBrankerIsRobot)
		{
			SetLeisurePlayerWin();
		}
		else
		{
			SetBrankerWin();
		}
	}
	if (!bAreaIsControl && !bBrankerIsControl && !bIsControlPlayerIsJetton && tmpJackpotScore.iUserJackpotControl == 1 && tmpJackpotScore.lCurPollScore<tmpJackpotScore.lMinPollScore && bIsSysWinPro) // �Ա�
	{
		bIsPoolScoreControl = true;
		if (bBrankerIsRobot)
		{
			SetLeisurePlayerLost();
		}
		else
		{
			SetBrankerLost();
		}
	}

	// add by har
	bool bIsDispatchTableCardStock = false;
	if (!bAreaIsControl && !bBrankerIsControl && !bIsControlPlayerIsJetton && !bIsPoolScoreControl)
		bIsDispatchTableCardStock = SetStockWinLose(); // add by har end

	bool bIsDispatchTableCardBrankerIsRobot = false;
	if (!bAreaIsControl && !bBrankerIsControl && !bIsControlPlayerIsJetton &&!bIsPoolScoreControl && !bIsDispatchTableCardStock && bBrankerIsRobot)
	{
		DispatchTableCardBrankerIsRobot();
		bIsDispatchTableCardBrankerIsRobot = true;
	}
	if (!bAreaIsControl && !bBrankerIsControl && !bIsControlPlayerIsJetton &&!bIsPoolScoreControl && !bIsDispatchTableCardStock && !bBrankerIsRobot)
	{
		DispatchTableCardBrankerIsPlayer();
	}
	SetIsAllRobotOrPlayerJetton(IsAllRobotOrPlayerJetton()); // add by har
	LOG_DEBUG("robot win - roomid:%d,tableid:%d,bAreaIsControl:%d bBrankerIsRobot:%d,control_uid:%d,bIsControlPlayerIsJetton:%d,bBrankerIsControl:%d,bIsFalgControl:%d,bIsDispatchTableCardBrankerIsRobot:%d,GetIsAllRobotOrPlayerJetton:%d,bIsDispatchTableCardStock:%d",
		m_pHostRoom->GetRoomID(), GetTableID(), bAreaIsControl, bBrankerIsRobot, control_uid, bIsControlPlayerIsJetton, bBrankerIsControl, bIsFalgControl, bIsDispatchTableCardBrankerIsRobot, GetIsAllRobotOrPlayerJetton(), bIsDispatchTableCardStock);
	return true;
}

//�����˿�
bool    CGameBaccaratTable::DispatchTableCardBrankerIsRobot()
{
    //����˿�
    if (m_poolCards.size() < 60) {
        InitRandCard();
        //m_vecRecord.clear();
        //SendPlayLog(NULL);
    }
    BYTE winType = 0;
	bool bBrankerIsRobot = false;
	if (m_pCurBanker != NULL) {
		bBrankerIsRobot = m_pCurBanker->IsRobot();
	}
	//�����˵�ׯ
	if (bBrankerIsRobot && g_RandGen.RandRatio(100, 100))
	{
		if (m_playerJettonScore[AREA_ZHUANG] < m_playerJettonScore[AREA_XIAN])
		{
			winType = 1; //��ҪׯӮ
		}
		else if (m_playerJettonScore[AREA_ZHUANG] == m_playerJettonScore[AREA_XIAN])
		{
			winType = 0; //������
		}
		else 
		{
			winType = 2; //��Ҫ��Ӯ
		}
		
		if (winType != 0)
		{
			if (m_imake_card_count == 0 && m_inotmake_card_count == 0) 
			{
				//�����˵�ׯ���Ƹ��ʣ����ܾ��� / 2 + 1�� / �ܾ��� - ���ܾ��� / 2�� / �ܾ���   18�� 5.5%��16�� 6.25%�����Ƹ���
				double fmakecardpro = 0;
				int imax_notmake_round = m_imax_notmake_round;
				if (imax_notmake_round > 0)
				{
					fmakecardpro = (((double)imax_notmake_round / (double)2 + 1) / (double)imax_notmake_round - ((double)imax_notmake_round / (double)2) / (double)imax_notmake_round);
				}
				fmakecardpro *= 10000;
				uint32 umakecardpro = (uint32)fmakecardpro;
				if (m_inotmake_card_round < imax_notmake_round)
				{
					if (g_RandGen.RandRatio(umakecardpro, 10000))
					{
						//����
						m_imake_card_count = 1;
						m_inotmake_card_round = 0;
					}
					else
					{
						//������
						winType = 0;
						m_imake_card_count = 0;
						m_inotmake_card_round++;
					}
				}
				else 
				{
					//����
					m_imake_card_count = 1;
					m_inotmake_card_round = 0;					
				}
			}
		}
	}


	
	LOG_DEBUG("all make card type:%d,zhaung_area_score:%lld,xian_area_score:%lld,m_imake_card_count:%d,m_inotmake_card_count:%d,m_inotmake_card_round:%d",
		winType, m_playerJettonScore[AREA_ZHUANG], m_playerJettonScore[AREA_XIAN], m_imake_card_count, m_inotmake_card_count, m_inotmake_card_round);


	int irand_count = 1000;
	int p = 0;
	for(; p < irand_count; p++)
    {// 100��û��������ͷ���
        for (uint8 i = 0; i < 2; ++i)
		{
            m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
            m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
        }

        //�״η���
        m_cbCardCount[INDEX_PLAYER] = 2;
        m_cbCardCount[INDEX_BANKER] = 2;


        //�������
        BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
        BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

        //�мҲ���
        BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
        if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
            //�������
            m_cbCardCount[INDEX_PLAYER]++;
            m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
            cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);

        }
        //ׯ�Ҳ���
        if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
            switch (cbBankerCount)
			{
                case 0:
                case 1:
                case 2:
				{
                    m_cbCardCount[INDEX_BANKER]++;
                }
                    break;
                case 3: 
				{
                    if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
                        m_cbCardCount[INDEX_PLAYER] == 2) {
                        m_cbCardCount[INDEX_BANKER]++;
                    }
                }
                    break;
                case 4:
				{
                    if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
                         cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
                        m_cbCardCount[INDEX_BANKER]++;
                    }
                }
                    break;
                case 5:
				{
                    if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
                         cbPlayerThirdCardValue != 3
                         && cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
                        m_cbCardCount[INDEX_PLAYER] == 2) {
                        m_cbCardCount[INDEX_BANKER]++;
                    }
                }
                    break;
                case 6:
				{
                    if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7)) 
					{
                        m_cbCardCount[INDEX_BANKER]++;
                    }

                }
                    break;
                    //���벹��
                case 7:
                case 8:
                case 9:
                    break;
                default:
                    break;
            }
        }
        if(m_cbCardCount[INDEX_BANKER] == 3)
		{
            m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
        }

        bool bSuccess = false;
        //�ƶ����
        BYTE  cbWinArea[AREA_MAX];
        memset(cbWinArea,0, sizeof(cbWinArea));
        DeduceWinner(cbWinArea);
        if(winType == 0)// ������
        {
            bSuccess = true;
        }else if(winType == 1)// ׯӮ
        {
            if(1 == cbWinArea[AREA_ZHUANG]){
                bSuccess = true;
            }else{
                bSuccess = false;
            }
        }else{// ��Ӯ
            if(1 == cbWinArea[AREA_XIAN]){
                bSuccess = true;
            }else{
                bSuccess = false;
            }
        }
		//��
		if(bSuccess==false && cbWinArea[AREA_PING] == 1 && cbWinArea[AREA_ZHUANG] == 2 &&	cbWinArea[AREA_XIAN] == 2)
		{
			bSuccess = true;
		}

		//LOG_DEBUG("make card result - p:%d,winType:%d,zhuang:%d,xian:%d,bSuccess:%d", p, winType, cbWinArea[AREA_ZHUANG], cbWinArea[AREA_XIAN], bSuccess);


        if(!bSuccess)// ���ɹ�,�Ż�ϴ��
        {
			if(p >= (irand_count-1))
			{
				LOG_DEBUG("make card failed - irand_count:%d,m_poolCards.size:%d,p:%d", irand_count,m_poolCards.size(), p);
				return true;
			}
            for(uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			{
                m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
            }
            for(uint8 i = 0;i< m_cbCardCount[INDEX_BANKER];++i){
                m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
            }
            RandPoolCard();
        }
		else
		{
			if (winType != 0) 
			{
				LOG_DEBUG("make card success - irand_count:%d,m_poolCards.size:%d,p:%d,m_imake_card_count:%d,m_inotmake_card_count:%d", irand_count, m_poolCards.size(), p, m_imake_card_count, m_inotmake_card_count);
			}
			if (m_imake_card_count > 0) {
				m_imake_card_count--;
			}
			
			return true;
        }
    }

	LOG_DEBUG("not dispatch table card - irand_count:%d,m_poolCards.size:%d,p:%d", irand_count, m_poolCards.size(), p);

    return true;
}

bool    CGameBaccaratTable::DispatchTableCardBrankerIsPlayer()
{
	//����˿�
	if (m_poolCards.size() < 60) {
		InitRandCard();
		//m_vecRecord.clear();
		//SendPlayLog(NULL);
	}
	BYTE winType = 0;
	bool bBrankerIsRobot = false;
	if (m_pCurBanker != NULL) {
		bBrankerIsRobot = m_pCurBanker->IsRobot();
	}

	int irand_count = 1000;
	int p = 0;
	for (; p < irand_count; p++)
	{// 100��û��������ͷ���
		for (uint8 i = 0; i < 2; ++i)
		{
			m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
			m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
		}

		//�״η���
		m_cbCardCount[INDEX_PLAYER] = 2;
		m_cbCardCount[INDEX_BANKER] = 2;


		//�������
		BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

		//�мҲ���
		BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
		if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
			//�������
			m_cbCardCount[INDEX_PLAYER]++;
			m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
			cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);

		}
		//ׯ�Ҳ���
		if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
			switch (cbBankerCount)
			{
			case 0:
			case 1:
			case 2:
			{
				m_cbCardCount[INDEX_BANKER]++;
			}
			break;
			case 3:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 4:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
					cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 5:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
					cbPlayerThirdCardValue != 3
					&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 6:
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
				{
					m_cbCardCount[INDEX_BANKER]++;
				}

			}
			break;
			//���벹��
			case 7:
			case 8:
			case 9:
				break;
			default:
				break;
			}
		}
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
		}
		
		//�ƶ����
		BYTE  cbWinArea[AREA_MAX];

		memset(cbWinArea, 0, sizeof(cbWinArea));
		cbBankerCount = DeduceWinner(cbWinArea); // ���ƺ�������ܻ�ı䣬�������»�ȡׯ�ҵ���������������Ӯ�ֿ��ܻ᲻��ȷ��  modify by har

		map<uint32, int64> mpUserWinScore;
		map<uint32, int64> mpUserLostScore;
		mpUserWinScore.clear();
		mpUserLostScore.clear();
		int64 lBankerWinScore = 0;
		int64 lRobotWinScore = 0;
		//������λ����
		for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				int64 scoreWin = 0;
				if (1 == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
						mpUserWinScore[pPlayer->GetUID()] += scoreWin;
						lBankerWinScore -= scoreWin;
					}
					else {
						scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * s_Multiple[wAreaIndex]);
						mpUserWinScore[pPlayer->GetUID()] += scoreWin;
						lBankerWinScore -= scoreWin;
					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					scoreWin = -m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
					mpUserLostScore[pPlayer->GetUID()] += scoreWin;
					lBankerWinScore -= scoreWin;
				}
				else {// ��

				}
				if (pPlayer->IsRobot()) {
					lRobotWinScore += scoreWin;
				}
			}
			//�ܵķ���
			mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
			mpUserLostScore[pPlayer->GetUID()] = 0;
		}
		//�����Թ��߻���
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				int64 scoreWin = 0;
				if (TRUE == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
						mpUserWinScore[pPlayer->GetUID()] += scoreWin;
						lBankerWinScore -= scoreWin;
					}
					else {
						scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * s_Multiple[wAreaIndex]);
						mpUserWinScore[pPlayer->GetUID()] += scoreWin;
						lBankerWinScore -= scoreWin;
					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					scoreWin = -m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
					mpUserLostScore[pPlayer->GetUID()] += scoreWin;
					lBankerWinScore -= scoreWin;
				}
				else {// ����

				}
				if (pPlayer->IsRobot()) {
					lRobotWinScore += scoreWin;
				}
			}
			//�ܵķ���
			mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
			mpUserLostScore[pPlayer->GetUID()] = 0;
		}

		

		bool bSuccess = false;
		uint32  playerBankerLosePro = m_playerBankerLosePro;
		bool bisMakeCard = g_RandGen.RandRatio(playerBankerLosePro, PRO_DENO_10000);
		if (lRobotWinScore > 0) {
			bSuccess = true;
		}
		if (!bisMakeCard) {
			bSuccess = true;
		}
		if (!bSuccess)// ���ɹ�,�Ż�ϴ��
		{
			if (p >= (irand_count - 1))
			{
				//LOG_DEBUG("make card failed - irand_count:%d,m_poolCards.size:%d,p:%d", irand_count,m_poolCards.size(), p);
				LOG_DEBUG("make card faild - irand_count:%d,m_poolCards.size:%d,p:%d,bisMakeCard:%d,playerBankerLosePro:%d", irand_count, m_poolCards.size(), p, bisMakeCard, playerBankerLosePro);

				return true;
			}
			for (uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			{
				m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
			}
			for (uint8 i = 0; i< m_cbCardCount[INDEX_BANKER]; ++i) {
				m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
			}
			RandPoolCard();
		}
		else
		{
			LOG_DEBUG("make card success - irand_count:%d,m_poolCards.size:%d,p:%d,bisMakeCard:%d,playerBankerLosePro:%d,lRobotWinScore:%lld,lBankerWinScore:%lld",irand_count, m_poolCards.size(), p, bisMakeCard, playerBankerLosePro, lRobotWinScore, lBankerWinScore);

			return true;
		}
	}

	LOG_DEBUG("not dispatch table card - irand_count:%d,m_poolCards.size:%d,p:%d", irand_count, m_poolCards.size(), p);

	return true;
}


bool    CGameBaccaratTable::DispatchTableCardControlPalyerWin(uint32 control_uid)
{
	//����˿�
	if (m_poolCards.size() < 60) {
		InitRandCard();
		//m_vecRecord.clear();
		//SendPlayLog(NULL);
	}

	int irand_count = 100;
	int iRandIndex = 0;
	for (; iRandIndex < irand_count; iRandIndex++)
	{// 100��û��������ͷ���
		for (uint8 i = 0; i < 2; ++i)
		{
			m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
			m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
		}

		//�״η���
		m_cbCardCount[INDEX_PLAYER] = 2;
		m_cbCardCount[INDEX_BANKER] = 2;


		//�������
		BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

		//�мҲ���
		BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
		if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
			//�������
			m_cbCardCount[INDEX_PLAYER]++;
			m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
			cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);
		}
		//ׯ�Ҳ���
		if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
			switch (cbBankerCount)
			{
			case 0:
			case 1:
			case 2:
			{
				m_cbCardCount[INDEX_BANKER]++;
			}
			break;
			case 3:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 4:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
					cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 5:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
					cbPlayerThirdCardValue != 3
					&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 6:
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
				{
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			//���벹��
			case 7:
			case 8:
			case 9:
				break;
			default:
				break;
			}
		}
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
		}
		//�ƶ����
		BYTE  cbWinArea[AREA_MAX];

		memset(cbWinArea, 0, sizeof(cbWinArea));
		cbBankerCount = DeduceWinner(cbWinArea); // ���ƺ�������ܻ�ı䣬�������»�ȡׯ�ҵ���������������Ӯ�ֿ��ܻ᲻��ȷ��  modify by har

		int64 control_win_score = 0;
		int64 control_lose_score = 0;
		int64 control_result_score = 0;
		bool bIsInChairID = false;
		//������λ����
		for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			uint32 uid = pPlayer->GetUID();
			if (control_uid == uid)
			{
				bIsInChairID = true;
				for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
				{
					if (m_userJettonScore[wAreaIndex][uid] == 0)
					{
						continue;
					}

					if (1 == cbWinArea[wAreaIndex])// Ӯ��
					{
						if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
							control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
						}
						else {
							control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);
							
						}
					}
					else if (0 == cbWinArea[wAreaIndex])// ����
					{
						control_lose_score -= m_userJettonScore[wAreaIndex][uid];
					}
					else {// ��

					}
				}
				control_result_score = control_win_score + control_lose_score;
			}
		}
		//�����Թ��߻���
		if (!bIsInChairID)
		{
			map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
			for (; it != m_mpLookers.end(); ++it)
			{
				CGamePlayer* pPlayer = it->second;
				if (pPlayer == NULL)continue;
				uint32 uid = pPlayer->GetUID();
				if (control_uid == uid)
				{
					for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
					{
						if (m_userJettonScore[wAreaIndex][uid] == 0)
						{
							continue;
						}

						if (1 == cbWinArea[wAreaIndex])// Ӯ��
						{
							if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
								control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
							}
							else {
								control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);
								
							}
						}
						else if (0 == cbWinArea[wAreaIndex])// ����
						{
							control_lose_score -= m_userJettonScore[wAreaIndex][uid];
						}
						else {// ��

						}
					}
					control_result_score = control_win_score + control_lose_score;
				}
			}
		}
		if (control_result_score > 0)
		{
			return true;
		}
		else
		{
			if (iRandIndex >= (irand_count - 1))
			{
				LOG_DEBUG("make card faild - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

				return false;
			}
			for (uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			{
				m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
			}
			for (uint8 i = 0; i< m_cbCardCount[INDEX_BANKER]; ++i) {
				m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
			}
			RandPoolCard();
		}
	}

	LOG_DEBUG("not dispatch table card - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

	return false;
}


bool    CGameBaccaratTable::DispatchTableCardControlPalyerLost(uint32 control_uid)
{
	//����˿�
	if (m_poolCards.size() < 60) {
		InitRandCard();
		//m_vecRecord.clear();
		//SendPlayLog(NULL);
	}
	
	int irand_count = 100;
	int iRandIndex = 0;
	for (; iRandIndex < irand_count; iRandIndex++)
	{// 100��û��������ͷ���
		for (uint8 i = 0; i < 2; ++i)
		{
			m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
			m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
		}

		//�״η���
		m_cbCardCount[INDEX_PLAYER] = 2;
		m_cbCardCount[INDEX_BANKER] = 2;


		//�������
		BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

		//�мҲ���
		BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
		if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
			//�������
			m_cbCardCount[INDEX_PLAYER]++;
			m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
			cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);
		}
		//ׯ�Ҳ���
		if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
			switch (cbBankerCount)
			{
			case 0:
			case 1:
			case 2:
			{
				m_cbCardCount[INDEX_BANKER]++;
			}
			break;
			case 3:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 4:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
					cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 5:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
					cbPlayerThirdCardValue != 3
					&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 6:
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
				{
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			//���벹��
			case 7:
			case 8:
			case 9:
				break;
			default:
				break;
			}
		}
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
		}
		//�ƶ����
		BYTE  cbWinArea[AREA_MAX];

		memset(cbWinArea, 0, sizeof(cbWinArea));
		cbBankerCount = DeduceWinner(cbWinArea); // ���ƺ�������ܻ�ı䣬�������»�ȡׯ�ҵ���������������Ӯ�ֿ��ܻ᲻��ȷ��  modify by har

		int64 control_win_score = 0;
		int64 control_lose_score = 0;
		int64 control_result_score = 0;
		bool bIsInChairID = false;
		//������λ����
		for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			uint32 uid = pPlayer->GetUID();
			if (control_uid == uid)
			{
				bIsInChairID = true;
				for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
				{
					if (m_userJettonScore[wAreaIndex][uid] == 0)
					{
						continue;
					}

					if (1 == cbWinArea[wAreaIndex])// Ӯ��
					{
						if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
							control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
						}
						else {
							control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);
							
						}
					}
					else if (0 == cbWinArea[wAreaIndex])// ����
					{
						control_lose_score -= m_userJettonScore[wAreaIndex][uid];
					}
					else {// ��

					}
				}
				control_result_score = control_win_score + control_lose_score;
			}
		}
		//�����Թ��߻���
		if (!bIsInChairID)
		{
			map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
			for (; it != m_mpLookers.end(); ++it)
			{
				CGamePlayer* pPlayer = it->second;
				if (pPlayer == NULL)continue;
				uint32 uid = pPlayer->GetUID();
				if (control_uid == uid)
				{
					for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
					{
						if (m_userJettonScore[wAreaIndex][uid] == 0)
						{
							continue;
						}

						if (1 == cbWinArea[wAreaIndex])// Ӯ��
						{
							if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
								control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
							}
							else {
								control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);
								
							}
						}
						else if (0 == cbWinArea[wAreaIndex])// ����
						{
							control_lose_score -= m_userJettonScore[wAreaIndex][uid];
						}
						else {// ��

						}
					}
					control_result_score = control_win_score + control_lose_score;
				}
			}
		}
		if (control_result_score < 0)
		{
			return true;
		}
		else
		{
			if (iRandIndex >= (irand_count - 1))
			{
				LOG_DEBUG("make card faild - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

				return false;
			}
			for (uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			{
				m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
			}
			for (uint8 i = 0; i< m_cbCardCount[INDEX_BANKER]; ++i) {
				m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
			}
			RandPoolCard();
		}
	}

	LOG_DEBUG("not dispatch table card - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

	return false;
}
bool    CGameBaccaratTable::SetBrankerWin()
{
	//����˿�
	if (m_poolCards.size() < 60) {
		InitRandCard();
		//m_vecRecord.clear();
		//SendPlayLog(NULL);
	}

	int irand_count = 100;
	int iRandIndex = 0;
	for (; iRandIndex < irand_count; iRandIndex++)
	{// 100��û��������ͷ���
		for (uint8 i = 0; i < 2; ++i)
		{
			m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
			m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
		}

		//�״η���
		m_cbCardCount[INDEX_PLAYER] = 2;
		m_cbCardCount[INDEX_BANKER] = 2;


		//�������
		BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

		//�мҲ���
		BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
		if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
			//�������
			m_cbCardCount[INDEX_PLAYER]++;
			m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
			cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);
		}
		//ׯ�Ҳ���
		if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
			switch (cbBankerCount)
			{
			case 0:
			case 1:
			case 2:
			{
				m_cbCardCount[INDEX_BANKER]++;
			}
			break;
			case 3:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 4:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
					cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 5:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
					cbPlayerThirdCardValue != 3
					&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 6:
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
				{
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			//���벹��
			case 7:
			case 8:
			case 9:
				break;
			default:
				break;
			}
		}
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
		}
		//�ƶ����
		BYTE  cbWinArea[AREA_MAX];

		memset(cbWinArea, 0, sizeof(cbWinArea));
		cbBankerCount = DeduceWinner(cbWinArea); // ���ƺ�������ܻ�ı䣬�������»�ȡׯ�ҵ���������������Ӯ�ֿ��ܻ᲻��ȷ��  modify by har

		int64 control_win_score = 0;
		int64 control_lose_score = 0;
		int64 control_result_score = 0;

		//������λ����
		for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			uint32 uid = pPlayer->GetUID();

			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][uid] == 0)
				{
					continue;
				}

				if (1 == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					}
					else {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);
					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					control_lose_score -= m_userJettonScore[wAreaIndex][uid];
				}
				else {// ��

				}
			}
		}
		//�����Թ��߻���


		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			uint32 uid = pPlayer->GetUID();

			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][uid] == 0)
				{
					continue;
				}

				if (1 == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					}
					else {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);

					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					control_lose_score -= m_userJettonScore[wAreaIndex][uid];
				}
				else {// ��

				}
			}
		}

		control_result_score = control_win_score + control_lose_score;

		if (control_result_score < 0)
		{
			return true;
		}
		else
		{
			if (iRandIndex >= (irand_count - 1))
			{
				//LOG_DEBUG("make card faild - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

				return false;
			}
			for (uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			{
				m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
			}
			for (uint8 i = 0; i< m_cbCardCount[INDEX_BANKER]; ++i) {
				m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
			}
			RandPoolCard();
		}
	}

	//LOG_DEBUG("not dispatch table card - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

	return false;
}

bool    CGameBaccaratTable::SetBrankerLost()
{
	//����˿�
	if (m_poolCards.size() < 60) {
		InitRandCard();
		//m_vecRecord.clear();
		//SendPlayLog(NULL);
	}

	int irand_count = 100;
	int iRandIndex = 0;
	for (; iRandIndex < irand_count; iRandIndex++)
	{// 100��û��������ͷ���
		for (uint8 i = 0; i < 2; ++i)
		{
			m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
			m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
		}

		//�״η���
		m_cbCardCount[INDEX_PLAYER] = 2;
		m_cbCardCount[INDEX_BANKER] = 2;


		//�������
		BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

		//�мҲ���
		BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
		if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
			//�������
			m_cbCardCount[INDEX_PLAYER]++;
			m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
			cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);
		}
		//ׯ�Ҳ���
		if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
			switch (cbBankerCount)
			{
			case 0:
			case 1:
			case 2:
			{
				m_cbCardCount[INDEX_BANKER]++;
			}
			break;
			case 3:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 4:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
					cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 5:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
					cbPlayerThirdCardValue != 3
					&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 6:
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
				{
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			//���벹��
			case 7:
			case 8:
			case 9:
				break;
			default:
				break;
			}
		}
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
		}
		//�ƶ����
		BYTE  cbWinArea[AREA_MAX];

		memset(cbWinArea, 0, sizeof(cbWinArea));
		cbBankerCount = DeduceWinner(cbWinArea); // ���ƺ�������ܻ�ı䣬�������»�ȡׯ�ҵ���������������Ӯ�ֿ��ܻ᲻��ȷ��  modify by har

		int64 control_win_score = 0;
		int64 control_lose_score = 0;
		int64 control_result_score = 0;
		
		//������λ����
		for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			uint32 uid = pPlayer->GetUID();

			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][uid] == 0)
				{
					continue;
				}

				if (1 == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					}
					else {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);
					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					control_lose_score -= m_userJettonScore[wAreaIndex][uid];
				}
				else {// ��

				}
			}
		}
		//�����Թ��߻���
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			uint32 uid = pPlayer->GetUID();

			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][uid] == 0)
				{
					continue;
				}

				if (1 == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					}
					else {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);

					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					control_lose_score -= m_userJettonScore[wAreaIndex][uid];
				}
				else {// ��

				}
			}
		}

		control_result_score += control_win_score + control_lose_score;

		if (control_result_score > 0)
		{
			return true;
		}
		else
		{
			if (iRandIndex >= (irand_count - 1))
			{
				//LOG_DEBUG("make card faild - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

				return false;
			}
			for (uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			{
				m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
			}
			for (uint8 i = 0; i< m_cbCardCount[INDEX_BANKER]; ++i) {
				m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
			}
			RandPoolCard();
		}
	}

	//LOG_DEBUG("not dispatch table card - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

	return false;
}

bool    CGameBaccaratTable::SetLeisurePlayerWin()
{
	//����˿�
	if (m_poolCards.size() < 60) {
		InitRandCard();
		//m_vecRecord.clear();
		//SendPlayLog(NULL);
	}

	int irand_count = 100;
	int iRandIndex = 0;
	for (; iRandIndex < irand_count; iRandIndex++)
	{// 100��û��������ͷ���
		for (uint8 i = 0; i < 2; ++i)
		{
			m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
			m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
		}

		//�״η���
		m_cbCardCount[INDEX_PLAYER] = 2;
		m_cbCardCount[INDEX_BANKER] = 2;


		//�������
		BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

		//�мҲ���
		BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
		if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
			//�������
			m_cbCardCount[INDEX_PLAYER]++;
			m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
			cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);
		}
		//ׯ�Ҳ���
		if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
			switch (cbBankerCount)
			{
			case 0:
			case 1:
			case 2:
			{
				m_cbCardCount[INDEX_BANKER]++;
			}
			break;
			case 3:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 4:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
					cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 5:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
					cbPlayerThirdCardValue != 3
					&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 6:
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
				{
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			//���벹��
			case 7:
			case 8:
			case 9:
				break;
			default:
				break;
			}
		}
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
		}
		//�ƶ����
		BYTE  cbWinArea[AREA_MAX];

		memset(cbWinArea, 0, sizeof(cbWinArea));
		cbBankerCount = DeduceWinner(cbWinArea); // ���ƺ�������ܻ�ı䣬�������»�ȡׯ�ҵ���������������Ӯ�ֿ��ܻ᲻��ȷ��  modify by har

		int64 control_win_score = 0;
		int64 control_lose_score = 0;
		int64 control_result_score = 0;

		//������λ����
		for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			uint32 uid = pPlayer->GetUID();

			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][uid] == 0)
				{
					continue;
				}

				if (1 == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					}
					else {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);
					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					control_lose_score -= m_userJettonScore[wAreaIndex][uid];
				}
				else {// ��

				}
			}
		}
		//�����Թ��߻���
		map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin();
		for (; it != m_mpLookers.end(); ++it)
		{
			CGamePlayer* pPlayer = it->second;
			if (pPlayer == NULL)continue;
			if(pPlayer->IsRobot())continue;
			uint32 uid = pPlayer->GetUID();

			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][uid] == 0)
				{
					continue;
				}

				if (1 == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					}
					else {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);

					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					control_lose_score -= m_userJettonScore[wAreaIndex][uid];
				}
				else {// ��

				}
			}
		}

		control_result_score += control_win_score + control_lose_score;

		if (control_result_score > 0)
		{
			return true;
		}
		else
		{
			if (iRandIndex >= (irand_count - 1))
			{
				//LOG_DEBUG("make card faild - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

				return false;
			}
			for (uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			{
				m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
			}
			for (uint8 i = 0; i< m_cbCardCount[INDEX_BANKER]; ++i) {
				m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
			}
			RandPoolCard();
		}
	}

	//LOG_DEBUG("not dispatch table card - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

	return false;
}

bool    CGameBaccaratTable::SetLeisurePlayerLost()
{
	//����˿�
	if (m_poolCards.size() < 60) {
		InitRandCard();
		//m_vecRecord.clear();
		//SendPlayLog(NULL);
	}

	int irand_count = 100;
	int iRandIndex = 0;
	for (; iRandIndex < irand_count; iRandIndex++)
	{// 100��û��������ͷ���
		for (uint8 i = 0; i < 2; ++i)
		{
			m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
			m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
		}

		//�״η���
		m_cbCardCount[INDEX_PLAYER] = 2;
		m_cbCardCount[INDEX_BANKER] = 2;


		//�������
		BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

		//�мҲ���
		BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
		if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
			//�������
			m_cbCardCount[INDEX_PLAYER]++;
			m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
			cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);
		}
		//ׯ�Ҳ���
		if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
			switch (cbBankerCount)
			{
			case 0:
			case 1:
			case 2:
			{
				m_cbCardCount[INDEX_BANKER]++;
			}
			break;
			case 3:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 4:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
					cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 5:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
					cbPlayerThirdCardValue != 3
					&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 6:
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
				{
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			//���벹��
			case 7:
			case 8:
			case 9:
				break;
			default:
				break;
			}
		}
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
		}
		//�ƶ����
		BYTE  cbWinArea[AREA_MAX];

		memset(cbWinArea, 0, sizeof(cbWinArea));
		cbBankerCount = DeduceWinner(cbWinArea); // ���ƺ�������ܻ�ı䣬�������»�ȡׯ�ҵ���������������Ӯ�ֿ��ܻ᲻��ȷ��  modify by har

		int64 control_win_score = 0;
		int64 control_lose_score = 0;
		int64 control_result_score = 0;

		//������λ����
		for (WORD wChairID = 0; wChairID<GAME_PLAYER; wChairID++)
		{
			//��ȡ�û�
			CGamePlayer * pPlayer = GetPlayer(wChairID);
			if (pPlayer == NULL)continue;
			if (pPlayer->IsRobot())continue;
			uint32 uid = pPlayer->GetUID();

			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][uid] == 0)
				{
					continue;
				}

				if (1 == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					}
					else {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);
					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					control_lose_score -= m_userJettonScore[wAreaIndex][uid];
				}
				else {// ��

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
			uint32 uid = pPlayer->GetUID();

			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
			{
				if (m_userJettonScore[wAreaIndex][uid] == 0)
				{
					continue;
				}

				if (1 == cbWinArea[wAreaIndex])// Ӯ��
				{
					if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					}
					else {
						control_win_score += (m_userJettonScore[wAreaIndex][uid] * s_Multiple[wAreaIndex]);

					}
				}
				else if (0 == cbWinArea[wAreaIndex])// ����
				{
					control_lose_score -= m_userJettonScore[wAreaIndex][uid];
				}
				else {// ��

				}
			}
		}

		control_result_score += control_win_score + control_lose_score;

		if (control_result_score < 0)
		{
			return true;
		}
		else
		{
			if (iRandIndex >= (irand_count - 1))
			{
				//LOG_DEBUG("make card faild - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

				return false;
			}
			for (uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			{
				m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
			}
			for (uint8 i = 0; i< m_cbCardCount[INDEX_BANKER]; ++i) {
				m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
			}
			RandPoolCard();
		}
	}

	//LOG_DEBUG("not dispatch table card - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

	return false;
}

// ���ÿ����Ӯ add by har
bool CGameBaccaratTable::SetStockWinLose() {
	int64 stockChange = m_pHostRoom->IsStockChangeCard(this);
	if (stockChange == 0)
		return false;

	int64 playerAllWinScore = 0;
	int64 lBankerWinScore = 0;
	//����˿�
	if (m_poolCards.size() < 60)
		InitRandCard();

	int irand_count = 100;
	int iRandIndex = 0;
	for (; iRandIndex < irand_count; iRandIndex++)
	{// 100��û��������ͷ���
		for (uint8 i = 0; i < 2; ++i)
		{
			m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
			m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
		}

		//�״η���
		m_cbCardCount[INDEX_PLAYER] = 2;
		m_cbCardCount[INDEX_BANKER] = 2;

		//�������
		uint8 cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		uint8 cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

		//�мҲ���
		uint8 cbPlayerThirdCardValue = 0;    //�������Ƶ���
		if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
			//�������
			m_cbCardCount[INDEX_PLAYER]++;
			m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
			cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);
		}
		//ׯ�Ҳ���
		if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
			switch (cbBankerCount)
			{
			case 0:
			case 1:
			case 2:
			{
				m_cbCardCount[INDEX_BANKER]++;
			}
			break;
			case 3:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 4:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
					cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 5:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
					cbPlayerThirdCardValue != 3
					&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 6:
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
				{
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			//���벹��
			case 7:
			case 8:
			case 9:
				break;
			default:
				break;
			}
		}
		if (m_cbCardCount[INDEX_BANKER] == 3)
			m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
		// �ƶ�������Ӯ
		uint8  cbWinArea[AREA_MAX];

		memset(cbWinArea, 0, sizeof(cbWinArea));
		uint8 cbBankerCount2 = DeduceWinner(cbWinArea);  // ���ƺ���������˱仯����Ҫ���»�ȡׯ�ҵ�����������������ҵ�Ӯ�ֲ�׼ȷ��

		playerAllWinScore = 0;
		lBankerWinScore = 0;

		//������λ����
		for (uint16 wChairID = 0; wChairID < GAME_PLAYER; ++wChairID)
			playerAllWinScore += GetSinglePlayerWinScore(GetPlayer(wChairID), cbWinArea, cbBankerCount2, lBankerWinScore);
		//�����Թ��߻���
		for (map<uint32, CGamePlayer*>::iterator it = m_mpLookers.begin(); it != m_mpLookers.end(); ++it)
			playerAllWinScore += GetSinglePlayerWinScore(it->second, cbWinArea, cbBankerCount2, lBankerWinScore);

		if (IsBankerRealPlayer())
			playerAllWinScore += lBankerWinScore;

		if (CheckStockChange(stockChange, playerAllWinScore, iRandIndex)) {
			LOG_DEBUG("SetStockWinLose suc roomid:%d,tableid:%d,stockChange:%lld,iRandIndex:%d,playerAllWinScore:%lld,lBankerWinScore:%lld,cbBankerCount:%d,cbBankerCount2:%d,cbWinArea:%d-%d-%d-%d-%d-%d-%d-%d,m_cbTableCardArray:%d-%d-%d-%d-%d-%d",
				GetRoomID(), GetTableID(), stockChange, iRandIndex, playerAllWinScore, lBankerWinScore, cbBankerCount, cbBankerCount2, cbWinArea[0],
				cbWinArea[1], cbWinArea[2], cbWinArea[3], cbWinArea[4], cbWinArea[5], cbWinArea[6], cbWinArea[7], m_cbTableCardArray[0][0],
				m_cbTableCardArray[0][1], m_cbTableCardArray[0][2], m_cbTableCardArray[1][0], m_cbTableCardArray[1][1], m_cbTableCardArray[1][2]);
			return true;
		}
			
		if (iRandIndex >= (irand_count - 1)) {
			LOG_ERROR("SetStockWinLose fail - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d,playerAllWinScore:%d,stockChange:%lld,lBankerWinScore:%lld,cbBankerCount:%d,cbBankerCount2:%d",
				irand_count, m_poolCards.size(), iRandIndex, playerAllWinScore, stockChange, lBankerWinScore, cbBankerCount, cbBankerCount2);
			return false;
		}
		for (uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
		for (uint8 i = 0; i < m_cbCardCount[INDEX_BANKER]; ++i)
			m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
		RandPoolCard();
	}
	LOG_ERROR("SetStockWinLose fail2 - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d,playerAllWinScore:%d,lBankerWinScore:%d,stockChange:%d",
		irand_count, m_poolCards.size(), iRandIndex, playerAllWinScore, lBankerWinScore, stockChange);
	return false;
}

// ��ȡĳ����ҵ�Ӯ��  add by har
int64 CGameBaccaratTable::GetSinglePlayerWinScore(CGamePlayer *pPlayer, uint8 cbWinArea[AREA_MAX], uint8 cbBankerCount, int64 &lBankerWinScore) {
	if (pPlayer == NULL)
		return 0;
	uint32 uid = pPlayer->GetUID();
	int64 playerWinScore = 0; // �����Ӯ��
	for (uint16 wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex) {
		int64 jetton = m_userJettonScore[wAreaIndex][uid];
		if (jetton == 0)
			continue;
		if (TRUE == cbWinArea[wAreaIndex]) // Ӯ��
			if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG)
				playerWinScore += (jetton * s_Multiple[wAreaIndex] * SUPER_SIX_RATE);
			else
				playerWinScore += (jetton * s_Multiple[wAreaIndex]);
		else if (FALSE == cbWinArea[wAreaIndex]) // ����
			playerWinScore -= jetton;
	}
	lBankerWinScore -= playerWinScore;
	if (pPlayer->IsRobot())
		return 0;
	return playerWinScore;
}

//����ׯ��
void    CGameBaccaratTable::SendApplyUser(CGamePlayer* pPlayer)
{
    net::msg_baccarat_apply_list msg;
	uint32 uid = 0;
	if (pPlayer!=NULL) {
		uid = pPlayer->GetUID();
	}
	LOG_DEBUG("send_banker_list  begin - pPlayer:%p,uid:%d", pPlayer, uid);
   	for(uint32 nUserIdx=0; nUserIdx<m_ApplyUserArray.size(); ++nUserIdx)
	{
		CGamePlayer *pTmp = m_ApplyUserArray[nUserIdx];
		//ׯ���ж�
		if(pTmp == m_pCurBanker) 
            continue;        
        msg.add_player_ids(pTmp->GetUID());
        msg.add_apply_score(m_ApplyUserScore[pTmp->GetUID()]);

		LOG_DEBUG("send_banker_list  doing - uid:%d", pTmp->GetUID());
	}
	LOG_DEBUG("send_banker_list finish - pPlayer:%p,uid:%d,player_ids_size:%d", pPlayer, uid, msg.player_ids_size());

    //LOG_DEBUG("player_ids_size:%d",msg.player_ids_size());
    if(pPlayer){
       pPlayer->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_APPLY_LIST);
    }else{
       SendMsgToAll(&msg,net::S2C_MSG_BACCARAT_APPLY_LIST);
    }    
}
//����ׯ��
bool    CGameBaccaratTable::ChangeBanker(bool bCancelCurrentBanker)
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
			SendApplyUser();
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
        net::msg_baccarat_change_banker msg;
        msg.set_banker_user(GetBankerUID());
        msg.set_banker_score(m_lBankerScore);
        
        SendMsgToAll(&msg,net::S2C_MSG_BACCARAT_CHANGE_BANKER);
	}

	return bChangeBanker; 
}
//�ֻ��ж�
void    CGameBaccaratTable::TakeTurns()
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

				m_lBankerShowScore = GetPlayerCurScore(pPlayer) - m_lBankerScore;

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
void    CGameBaccaratTable::CalcBankerScore()
{
    if(m_pCurBanker == NULL)return;
    net::msg_baccarat_banker_calc_rep msg;
    msg.set_banker_time(m_wBankerTime);
    msg.set_win_count(m_wBankerWinTime);
    msg.set_buyin_score(m_lBankerBuyinScore);
    msg.set_win_score(m_lBankerWinScore);
    msg.set_win_max(m_lBankerWinMaxScore);
    msg.set_win_min(m_lBankerWinMinScore);

    m_pCurBanker->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_BANKER_CALC);
    
    int64 score = m_lBankerWinScore;

    LOG_DEBUG("the turn the banker win:%lld,rest:%lld,buyin:%lld",score,m_lBankerScore,m_lBankerBuyinScore);
    RemoveApplyBanker(m_pCurBanker->GetUID());

    //����ׯ��
	m_pCurBanker = NULL;     
    m_robotApplySize = g_RandGen.RandRange(4, 8);//��������������
    m_robotChairSize = g_RandGen.RandRange(5, 7);//��������λ��
    
    ResetGameData();    
}
//�Զ�����
void    CGameBaccaratTable::AutoAddBankerScore()
{
    //��ׯ�ж�
	if (m_pCurBanker == NULL || m_bankerAutoAddScore == 0 || m_needLeaveBanker || GetBankerTimeLimit() <= m_wBankerTime)
	{
		return;
	}
	
	int64 lBankerScore = m_lBankerScore;
	int64 diffScore = m_lBankerInitBuyinScore - lBankerScore;
	int64 canAddScore = GetPlayerCurScore(m_pCurBanker) - m_lBankerScore;

	//LOG_DEBUG("1 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
	//	GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

	LOG_DEBUG("1 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,curScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, GetPlayerCurScore(m_pCurBanker), m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

	//�ж�Ǯ�Ƿ�
	if (diffScore<=0)
	{
		//LOG_DEBUG("2 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		//	GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

		return;
	}
	if (canAddScore <= 0)
	{
		//LOG_DEBUG("3 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		//	GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

		return;
	}
    if(canAddScore < diffScore)
	{
        diffScore = canAddScore;
    }
	if ((m_lBankerScore + diffScore) < GetApplyBankerCondition())
	{
		//LOG_DEBUG("4 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		//	GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

		return;
	}

    m_lBankerBuyinScore += diffScore;
    m_lBankerScore      += diffScore;
    
	m_lBankerShowScore += diffScore;

    net::msg_baccarat_add_bankerscore_rep msg;
    msg.set_buyin_score(diffScore);

	//LOG_DEBUG("5 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
	//	GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

	LOG_DEBUG("5 - roomid:%d,tableid:%d,uid:%d,GetApplyBankerCondition:%lld,m_lBankerInitBuyinScore:%lld,curScore:%lld,m_lBankerBuyinScore:%lld,m_lBankerScore:%lld,canAddScore:%lld,diffScore:%lld",
		GetRoomID(), GetTableID(), GetBankerUID(), GetApplyBankerCondition(), m_lBankerInitBuyinScore, GetPlayerCurScore(m_pCurBanker), m_lBankerBuyinScore, m_lBankerScore, canAddScore, diffScore);

    m_pCurBanker->SendMsgToClient(&msg,net::S2C_MSG_BACCARAT_ADD_BANKER_SCORE);
}
//������Ϸ��¼
void  CGameBaccaratTable::SendPlayLog(CGamePlayer* pPlayer)
{
	uint32 uid = 0;
	if (pPlayer != NULL)
	{
		uid = pPlayer->GetUID();
	}
    net::msg_baccarat_play_log_rep msg;
    for(uint16 i=0;i<m_vecRecord.size();++i){
        net::baccarat_play_log* plog = msg.add_logs();
        baccaratGameRecord& record = m_vecRecord[i];
        plog->set_banker_count(record.cbBankerCount);
        plog->set_player_count(record.cbPlayerCount);
        plog->set_banker_pair(record.bBankerTwoPair);
        plog->set_player_pair(record.bPlayerTwoPair);
		plog->set_is_small(record.cbIsSmall);
		plog->set_is_super_six(record.cbIsSuperSix);
    }
	LOG_DEBUG("�����ƾּ�¼ size:%d,uid:%d", msg.logs_size(), uid);

    if(pPlayer != NULL) {
        pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BACCARAT_PLAY_LOG);
    }else{
        SendMsgToAll(&msg,net::S2C_MSG_BACCARAT_PLAY_LOG);
    }
}
//�����ע
int64   CGameBaccaratTable::GetUserMaxJetton(CGamePlayer* pPlayer, BYTE cbJettonArea)
{
	int iTimer = 1;// s_MultipleTime[cbJettonArea];
	//����ע��
	int64 lNowJetton = 0;
    //lNowJetton = m_userJettonScore[cbJettonArea][pPlayer->GetUID()];
	for (int nAreaIndex = 0; nAreaIndex < AREA_MAX; ++nAreaIndex)
	{
		lNowJetton += m_userJettonScore[nAreaIndex][pPlayer->GetUID()];
	}
	//ׯ�ҽ��
	int64 lBankerScore = 0;
	if(m_pCurBanker != NULL)
	{
        lBankerScore = m_lBankerScore;
    }
	for (int nAreaIndex = 0; nAreaIndex < AREA_MAX; ++nAreaIndex)
	{
		lBankerScore -= m_allJettonScore[nAreaIndex] * s_MultipleTime[nAreaIndex];
	}
    //lBankerScore -= m_allJettonScore[cbJettonArea]*iTimer;

	//��������
	int64 lMeMaxScore = (GetPlayerCurScore(pPlayer) - lNowJetton);

	//ׯ������
	lMeMaxScore = min(lMeMaxScore,lBankerScore/iTimer);

	//��������
	lMeMaxScore = MAX(lMeMaxScore, 0);

	LOG_DEBUG("roomid:%d,tableid:%d,uid:%d,m_pCurBanker:%p,m_lBankerScore:%lld,iTimer:%d,curScore:%lld,lNowJetton:%lld,lBankerScore:%lld,lMeMaxScore:%lld",
		GetRoomID(), GetTableID(), pPlayer->GetUID(), m_pCurBanker, m_lBankerScore, iTimer, GetPlayerCurScore(pPlayer), lNowJetton, lBankerScore, lMeMaxScore);


	return (lMeMaxScore);
}
uint8   CGameBaccaratTable::GetRobotJettonArea(CGamePlayer* pPlayer)
{
    //for(uint8 i=AREA_XIAN;i<AREA_MAX;++i){
    //    if(m_userJettonScore[i][pPlayer->GetUID()] > 0){
    //        return i;
    //    }
    //}

    //if(g_RandGen.RandRatio(80,100)){
    //    if(g_RandGen.RandRatio(15,100)){
    //        return AREA_XIAN;
    //    }else{
    //        return g_RandGen.RandRange(AREA_XIAN, AREA_ZHUANG_DUI);
    //    }
    //}
	uint8 area = AREA_MAX;
	int32 iAreaRatio = g_RandGen.RandRange(AREA_XIAN, AREA_BIG);
	if (m_cbJettonArea == AREA_MAX)
	{
		if (g_RandGen.RandRatio(50, 100))
		{
			m_cbJettonArea = AREA_ZHUANG;
		}
		else
		{
			m_cbJettonArea = AREA_XIAN;
		}
	}
	else
	{
		if (g_RandGen.RandRatio(60, 100))
		{
			area = m_cbJettonArea;
		}
		else if (g_RandGen.RandRatio(30, 100))
		{
			if (m_cbJettonArea == AREA_ZHUANG)
			{
				area = AREA_XIAN;
			}
			else
			{
				area = AREA_ZHUANG;
			}
		}
		else
		{
			area = iAreaRatio;
		}
	}

	if (area == AREA_MAX) {
		area = g_RandGen.RandRange(AREA_XIAN, AREA_BIG);
	}

	//LOG_DEBUG("uid:%d,lAllScore:%lld,xian:%lld,zhuang:%lld,area:%d", pPlayer->GetUID(),lAllScore, m_allJettonScore[AREA_XIAN], m_allJettonScore[AREA_ZHUANG], area);

	return area;


}


//uint32  CGameBaccaratTable::GetBankerUID()
//{
//    if(m_pCurBanker)
//        return m_pCurBanker->GetUID();
//    return 0;
//}
//void    CGameBaccaratTable::RemoveApplyBanker(uint32 uid)
//{
//    LOG_DEBUG("remove apply banker:%d",uid);
//    for(uint32 i=0; i<m_ApplyUserArray.size(); ++i)
//	{
//		//��������
//		CGamePlayer* pPlayer = m_ApplyUserArray[i];
//		if(pPlayer->GetUID() == uid){
//    		//ɾ�����
//    		m_ApplyUserArray.erase(m_ApplyUserArray.begin()+i);
//            m_mpApplyUserInfo.erase(uid);
//            UnLockApplyScore(pPlayer);
//			LOG_DEBUG("remove_apply_banker_success uid:%d", uid);
//
//    		break;
//		}
//	}    
//}
//bool    CGameBaccaratTable::LockApplyScore(CGamePlayer* pPlayer,int64 score)
//{
//	if (pPlayer == NULL) {
//		return false;
//	}
//
//    if(GetPlayerCurScore(pPlayer) < score)
//        return false;
//    
//    ChangePlayerCurScore(pPlayer,-score);     
//    m_ApplyUserScore[pPlayer->GetUID()] = score;
//
//    return true;
//}
//bool    CGameBaccaratTable::UnLockApplyScore(CGamePlayer* pPlayer)
//{
//	if (pPlayer == NULL) {
//		return false;
//	}
//
//    int64 lockScore = m_ApplyUserScore[pPlayer->GetUID()];
//    ChangePlayerCurScore(pPlayer,lockScore);
//    
//    return true;
//}    
//ׯ��վ��
void    CGameBaccaratTable::StandUpBankerSeat(CGamePlayer* pPlayer)
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
bool    CGameBaccaratTable::IsSetJetton(uint32 uid)
{
    //if(TABLE_STATE_NIUNIU_PLACE_JETTON != GetGameState())
    //    return false;
	if (TABLE_STATE_NIUNIU_GAME_END == GetGameState())
	{
		return false;
	}

    for(uint8 i=0;i<AREA_MAX;++i){
        if(m_userJettonScore[i][uid] > 0)
            return true;
    }
	for (uint32 i = 0; i < m_RobotPlaceJetton.size(); i++) {
		uint32 temp_uid = 0;
		if (m_RobotPlaceJetton[i].pPlayer)
		{
			temp_uid = m_RobotPlaceJetton[i].pPlayer->GetUID();
		}
		if (uid == temp_uid) {
			return true;
		}
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
	
    return false;
}    
bool    CGameBaccaratTable::IsInApplyList(uint32 uid)
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
int64   CGameBaccaratTable::CalculateScore()
{
    //�����Ƶ�
    BYTE cbPlayerCount = m_GameLogic.GetCardListPip( m_cbTableCardArray[INDEX_PLAYER],m_cbCardCount[INDEX_PLAYER] );
    BYTE cbBankerCount = m_GameLogic.GetCardListPip( m_cbTableCardArray[INDEX_BANKER],m_cbCardCount[INDEX_BANKER] );

    //�ƶ����
    memset(m_cbWinArea,0, sizeof(m_cbWinArea));
    DeduceWinner(m_cbWinArea);

    //��Ϸ��¼
    m_record.cbBankerCount = cbBankerCount;
    m_record.cbPlayerCount = cbPlayerCount;
    m_record.bPlayerTwoPair = m_cbWinArea[AREA_XIAN_DUI];
    m_record.bBankerTwoPair = m_cbWinArea[AREA_ZHUANG_DUI];
	m_record.cbIsSmall = m_cbWinArea[AREA_SMALL];
	m_record.cbIsSuperSix = m_cbWinArea[AREA_SUPSIX];

    m_vecRecord.push_back(m_record);
	if (m_vecRecord.size() > MAX_BACCARAT_GAME_RECORD) {
		m_vecRecord.erase(m_vecRecord.begin());
	}

    m_cbTableCardType[INDEX_PLAYER] = m_record.cbPlayerCount;
    m_cbTableCardType[INDEX_BANKER] = m_record.cbBankerCount;

    for(uint8 i=0;i<MAX_SEAT_INDEX;++i){        
        WriteOutCardLog(i,m_cbTableCardArray[i], m_cbCardCount[i],0);
    }
	//ׯ������
	int64 lBankerWinScore = 0;

	//��ҳɼ�
	m_mpUserWinScore.clear();
	map<uint32,int64> mpUserLostScore;
	mpUserLostScore.clear();
	//������λ����
	for(WORD wChairID=0; wChairID<GAME_PLAYER; wChairID++)
	{
		//��ȡ�û�
		CGamePlayer * pPlayer = GetPlayer(wChairID);
		if(pPlayer == NULL)continue;
		//int64 lResultScore = 0;
		for(WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
		{
            int64 scoreWin = 0;
			if(1 == m_cbWinArea[wAreaIndex])// Ӯ��
			{
				if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG ){
					scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					m_mpUserWinScore[pPlayer->GetUID()] += scoreWin;
					lBankerWinScore -= scoreWin;
				}
				else {
					scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * s_Multiple[wAreaIndex]);
					m_mpUserWinScore[pPlayer->GetUID()] += scoreWin;
					lBankerWinScore -= scoreWin;
				}
			}
			else if(0 == m_cbWinArea[wAreaIndex])// ����
			{
                scoreWin = -m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				mpUserLostScore[pPlayer->GetUID()] += scoreWin;
				lBankerWinScore -= scoreWin;
			}else{// ��
                
                
            }
			//lResultScore -= scoreWin;
            WriteAddScoreLog(pPlayer->GetUID(),wAreaIndex,m_userJettonScore[wAreaIndex][pPlayer->GetUID()],scoreWin);
		}
		//�ܵķ���
		m_mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
        mpUserLostScore[pPlayer->GetUID()] = 0;
	}
    //�����Թ��߻���
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL)continue;
		//int64 lResultScore = 0;
		for(WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
		{
            int64 scoreWin = 0;
			if(TRUE == m_cbWinArea[wAreaIndex])// Ӯ��
			{
				if (cbBankerCount == SUPER_SIX_NUMBER && wAreaIndex == AREA_ZHUANG) {
					scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * (s_Multiple[wAreaIndex] * SUPER_SIX_RATE));
					m_mpUserWinScore[pPlayer->GetUID()] += scoreWin;
					lBankerWinScore -= scoreWin;
				}
				else {
					scoreWin = (m_userJettonScore[wAreaIndex][pPlayer->GetUID()] * s_Multiple[wAreaIndex]);
					m_mpUserWinScore[pPlayer->GetUID()] += scoreWin;
					lBankerWinScore -= scoreWin;
				}
			}
			else if(0 == m_cbWinArea[wAreaIndex])// ����
			{
                scoreWin = -m_userJettonScore[wAreaIndex][pPlayer->GetUID()];
				mpUserLostScore[pPlayer->GetUID()] += scoreWin;
				lBankerWinScore -= scoreWin;
			}else{// ����
                
            }
			//lResultScore -= scoreWin;
            WriteAddScoreLog(pPlayer->GetUID(),wAreaIndex,m_userJettonScore[wAreaIndex][pPlayer->GetUID()],scoreWin);
		}
		//�ܵķ���
		m_mpUserWinScore[pPlayer->GetUID()] += mpUserLostScore[pPlayer->GetUID()];
        mpUserLostScore[pPlayer->GetUID()] = 0;
    }

	//�ۼƻ���
	m_lBankerWinScore    += lBankerWinScore;
	//��ǰ����
    m_lBankerScore       += lBankerWinScore;
    if(lBankerWinScore > 0)m_wBankerWinTime++;
    m_lBankerWinMaxScore = MAX(lBankerWinScore,m_lBankerWinMaxScore);
    m_lBankerWinMinScore = MIN(lBankerWinScore,m_lBankerWinMinScore);

	bool bBrankerTakeAll = true;
	bool bBrankerCompensation = true;
	bool bIsUserPlaceJetton = false;
	for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
	{
		if (TRUE == m_cbWinArea[wAreaIndex])// Ӯ��
		{
			bBrankerTakeAll = false;
		}
		else if (0 == m_cbWinArea[wAreaIndex])// ����
		{
			bBrankerCompensation = false;
		}
		else {// ����
			bBrankerTakeAll = false;
			bBrankerCompensation = false;
		}
		if (m_allJettonScore[wAreaIndex]>0)
		{
			bIsUserPlaceJetton = true;
		}
	}

	if (bBrankerTakeAll && bIsUserPlaceJetton)
	{
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_TAKE_ALL;
	}
	else if (bBrankerCompensation && bIsUserPlaceJetton)
	{
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_COMPENSATION;
	}
	else
	{
		m_cbBrankerSettleAccountsType = BRANKER_TYPE_NULL;
	}
	return lBankerWinScore;
}
//�ƶ�Ӯ��
uint8 CGameBaccaratTable::DeduceWinner(BYTE* pWinArea)
{
    //�����Ƶ�
    BYTE cbPlayerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER],m_cbCardCount[INDEX_PLAYER]);
    BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER],m_cbCardCount[INDEX_BANKER]);
    //ʤ������--------------------------
    //ƽ
    if( cbPlayerCount == cbBankerCount )
    {
        pWinArea[AREA_PING]   = 1;
        pWinArea[AREA_ZHUANG] = 2;
        pWinArea[AREA_XIAN]   = 2;
        
    }// ׯ
    else if ( cbPlayerCount < cbBankerCount)
    {
        pWinArea[AREA_ZHUANG] = 1;
    }// ��
    else
    {
        pWinArea[AREA_XIAN] = 1;
    }
    //�����ж�
    if (m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_PLAYER][0]) == m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_PLAYER][1]))
    {
        pWinArea[AREA_XIAN_DUI] = 1;
    }
    if (m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_BANKER][0]) == m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_BANKER][1]))
    {
        pWinArea[AREA_ZHUANG_DUI] = 1;
    }
	if (pWinArea[AREA_ZHUANG] == 1 && cbBankerCount == SUPER_SIX_NUMBER)
	{
		pWinArea[AREA_SUPSIX] = 1;
	}
	BYTE cbSumCardCount = m_cbCardCount[0] + m_cbCardCount[1];
	if (cbSumCardCount == 4)
	{
		pWinArea[AREA_SMALL] = 1;
	}
	if (cbSumCardCount == 5 || cbSumCardCount == 6)
	{
		pWinArea[AREA_BIG] = 1;
	}
	return cbBankerCount; // add by har
}
//��������
int64   CGameBaccaratTable::GetApplyBankerCondition()
{
    return GetBaseScore();
}
int64   CGameBaccaratTable::GetApplyBankerConditionLimit()
{
    return GetBaseScore()*20;
}
//��������
int32   CGameBaccaratTable::GetBankerTimeLimit()
{
    return m_BankerTimeLimit;
}

//void CGameBaccaratTable::OnChairRobotOper()
//{
//	vector<CGamePlayer*> robots;
//	for (uint32 i = 0; i<GAME_PLAYER; ++i)
//	{
//		CGamePlayer* pPlayer = GetPlayer(i);
//		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
//			continue;
//		int iJettonCount = g_RandGen.RandRange(1, 9);
//		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
//		{
//			//uint8 area = GetRobotJettonArea(pPlayer);
//			//if (m_userJettonScore[area][pPlayer->GetUID()] > 0)
//			//	continue;
//			robots.push_back(pPlayer);
//		}
//	}
//	if (robots.size() == 0)
//		return;
//	uint16 iSize = robots.size();// g_RandGen.RandRange(1, robots.size());
//
//	//LOG_DEBUG("-------------------------------------------------------------------------------------------------------------------------------------");
//	for (uint16 i = 0; i<iSize; ++i)
//	{
//		//uint16 pos = g_RandGen.RandRange(0, robots.size() - 1);
//		CGamePlayer* pPlayer = robots[i];
//		if (pPlayer == NULL)
//		{
//			continue;
//		}
//		if (g_RandGen.RandRatio(50, PRO_DENO_100))
//			continue;
//		uint8 area = GetRobotJettonArea(pPlayer);
//		//int64 maxJetton = GetUserMaxJetton(pPlayer, area) / 5;
//		//if (maxJetton < 1000)
//		//	continue;
//
//		int64 jetton = GetRobotJettonScore(pPlayer, area);
//		if (jetton == 0)
//		{
//			continue;
//		}
//		uint32 passtick = m_coolLogic.getPassTick();
//		uint32 totaltick = m_coolLogic.getTotalTick();
//
//		tagRobotPlaceJetton robotPlaceJetton;
//		robotPlaceJetton.pPlayer = pPlayer;
//
//		passtick = passtick / 1000;
//		totaltick = totaltick / 1000;
//
//		robotPlaceJetton.time = g_RandGen.RandRange(passtick + 3, totaltick - 3);
//		if (robotPlaceJetton.time <= 1) {
//			continue;
//		}
//		robotPlaceJetton.area = area;
//		robotPlaceJetton.jetton = jetton;
//		robotPlaceJetton.bflag = false;
//		//PushRobotPlaceJetton(robotPlaceJetton);
//
//		m_chairRobotPlaceJetton.push_back(robotPlaceJetton);
//
//		//if(!OnUserPlaceJetton(pPlayer,area,jetton))
//		//    break;
//		//LOG_DEBUG("uid:%d,xian:%lld,zhuang:%lld,area:%d,jetton:%lld", pPlayer->GetUID(), m_allJettonScore[AREA_XIAN], m_allJettonScore[AREA_ZHUANG], area, jetton);
//
//		//LOG_DEBUG("uid:%d,time:%d,area:%d,jetton:%lld,passtick:%d,totaltick:%d,getState:%d", pPlayer->GetUID(), robotPlaceJetton.time, area, jetton, passtick, totaltick, m_coolRobotJetton.getState());
//	}
//}


void CGameBaccaratTable::OnChairRobotOper()
{
	if (m_bIsChairRobotAlreadyJetton)
	{
		return;
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
		int64 lUserRealJetton = GetRobotJettonScore(pPlayer, 0);
		int64 lOldRealJetton = -1;
		int iJettonTypeCount = g_RandGen.RandRange(1, 2);
		int iJettonStartCount = g_RandGen.RandRange(2, 3);
		uint32 iJettonOldTime = 0;
		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
		{
			//if (g_RandGen.RandRatio(95, PRO_DENO_100))
			//{
			//	continue;
			//}

			uint8 cbJettonArea = GetRobotJettonArea(pPlayer);
			int64 lUserRealJetton = GetRobotJettonScore(pPlayer, cbJettonArea);
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
			if (lUserRealJetton == 0)
			{
				//LOG_DEBUG("1111111111111111111111111111111");
				continue;
			}

			tagRobotPlaceJetton robotPlaceJetton;
			robotPlaceJetton.uid = pPlayer->GetUID();
			robotPlaceJetton.pPlayer = pPlayer;
			uint32 uRemainTime = m_coolLogic.getCoolTick();
			uRemainTime = uRemainTime / 1000;

			uint32 passtick = m_coolLogic.getPassTick();
			passtick = passtick / 1000;
			uint32 uMaxDelayTime = s_PlaceJettonTime / 1000;

			robotPlaceJetton.time = iIndex;
			//if (iIndex % 2 == 0)
			{
				robotPlaceJetton.time += g_RandGen.RandRange(4, 5);
			}

			if (iJettonOldTime == 0)
			{
				robotPlaceJetton.time = g_RandGen.RandRange(1, 2);
				iJettonOldTime = robotPlaceJetton.time;
			}
			else
			{
				robotPlaceJetton.time = iJettonOldTime + g_RandGen.RandRange(1, 2);
				iJettonOldTime = robotPlaceJetton.time;
			}

			if (robotPlaceJetton.time <= 0 || robotPlaceJetton.time > uMaxDelayTime)
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

	return;
}

void    CGameBaccaratTable::OnRobotOper()
{
	/*
    vector<CGamePlayer*> robots;
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
            continue;
		uint8 area = GetRobotJettonArea(pPlayer);
        if(m_userJettonScore[area][pPlayer->GetUID()] > 0)
            continue;
        robots.push_back(pPlayer);
    }
  //  for(uint32 i=0;i<GAME_PLAYER;++i)
  //  {
  //      CGamePlayer* pPlayer = GetPlayer(i);
  //      if(pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
  //          continue;
		//uint8 area = GetRobotJettonArea(pPlayer);
  //      if(m_userJettonScore[area][pPlayer->GetUID()] > 0)
  //          continue;
  //      robots.push_back(pPlayer);
  //  }
    if(robots.size() == 0)
        return;
    uint16 iSize = g_RandGen.RandRange(1,robots.size());

	//LOG_DEBUG("-------------------------------------------------------------------------------------------------------------------------------------");
    for(uint16 i=0;i<iSize;++i)
    {
        uint16 pos = g_RandGen.RandRange(0,robots.size()-1);
        CGamePlayer* pPlayer = robots[pos];

		uint8 area = GetRobotJettonArea(pPlayer);
        int64 maxJetton = GetUserMaxJetton(pPlayer,area)/5;
        if(maxJetton < 1000)
            continue;
        //int64 minJetton = GetUserMaxJetton(pPlayer,area)/10;
		//int64 jetton = g_RandGen.RandRange(minJetton, maxJetton);

        //jetton = (jetton/1000)*1000;
        //jetton = MAX(jetton,1000);
		int64 jetton = GetRobotJettonScore(pPlayer, area);
		if (jetton == 0)
		{
			continue;
		}
		uint32 passtick = m_coolLogic.getPassTick();
		uint32 totaltick = m_coolLogic.getTotalTick();

		tagRobotPlaceJetton robotPlaceJetton;
		robotPlaceJetton.pPlayer = pPlayer;

		passtick = passtick / 1000;
		totaltick = totaltick / 1000;

		robotPlaceJetton.time = g_RandGen.RandRange(passtick+3, totaltick-3);
		if (robotPlaceJetton.time <= 1) {
			continue;
		}
		robotPlaceJetton.area = area;
		robotPlaceJetton.jetton = jetton;
		robotPlaceJetton.bflag = false;
		PushRobotPlaceJetton(robotPlaceJetton);
        //if(!OnUserPlaceJetton(pPlayer,area,jetton))
        //    break;
		//LOG_DEBUG("uid:%d,xian:%lld,zhuang:%lld,area:%d,jetton:%lld", pPlayer->GetUID(), m_allJettonScore[AREA_XIAN], m_allJettonScore[AREA_ZHUANG], area, jetton);

		//LOG_DEBUG("uid:%d,time:%d,area:%d,jetton:%lld,passtick:%d,totaltick:%d,getState:%d", pPlayer->GetUID(), robotPlaceJetton.time, area, jetton, passtick, totaltick, m_coolRobotJetton.getState());
    }
	*/
}


int64 CGameBaccaratTable::GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area)
{
	int64 lUserRealJetton = 1000;
	int64 lUserMinJetton = 1000;
	int64 lUserMaxJetton = GetUserMaxJetton(pPlayer, area);
	int64 lUserCurJetton = GetPlayerCurScore(pPlayer);

	if (lUserCurJetton < 2000)
	{
		lUserRealJetton = 1000;
	}
	else if (lUserCurJetton >= 2000 && lUserCurJetton < 50000)
	{
		if (g_RandGen.RandRatio(77, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(15, PRO_DENO_100))
		{
			lUserRealJetton = 5000;
		}
		else
		{
			lUserRealJetton = 10000;
		}
	}
	else if (lUserCurJetton >= 50000 && lUserCurJetton < 200000)
	{
		if (g_RandGen.RandRatio(60, PRO_DENO_100))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(15, PRO_DENO_100))
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
	else if (lUserCurJetton >= 200000 && lUserCurJetton < 2000000)
	{
		if (g_RandGen.RandRatio(3500, PRO_DENO_10000))
		{
			lUserRealJetton = 1000;
		}
		else if (g_RandGen.RandRatio(2300, PRO_DENO_10000))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(3200, PRO_DENO_10000))
		{
			lUserRealJetton = 10000;
		}
		else if (g_RandGen.RandRatio(800, PRO_DENO_10000))
		{
			lUserRealJetton = 50000;
		}
		else
		{
			lUserRealJetton = 100000;
		}
	}
	else if (lUserCurJetton >= 2000000)
	{
		if (g_RandGen.RandRatio(4000, PRO_DENO_10000))
		{
			lUserRealJetton = 5000;
		}
		else if (g_RandGen.RandRatio(4150, PRO_DENO_10000))
		{
			lUserRealJetton = 10000;
		}
		else if (g_RandGen.RandRatio(1500, PRO_DENO_10000))
		{
			lUserRealJetton = 50000;
		}
		else if (g_RandGen.RandRatio(300, PRO_DENO_10000))
		{
			lUserRealJetton = 100000;
		}
		else
		{
			//lUserRealJetton = 500000;
			lUserRealJetton = 1000;
		}
	}
	else
	{
		lUserRealJetton = 1000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 500000)
	{
		lUserRealJetton = 100000;
	}
	if (lUserRealJetton > lUserMaxJetton && lUserRealJetton == 100000)
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
		lUserRealJetton = 0;
	}
	if (lUserRealJetton < lUserMinJetton)
	{
		lUserRealJetton = 0;
	}
	return lUserRealJetton;
}


//void CGameBaccaratTable::OnRobotPlaceJetton()
//{
//	if (m_robotPlaceJetton.size() == 0) {
//		return;
//	}
//	int iJettonCount = 0;
//	for (uint32 i = 0; i < m_robotPlaceJetton.size(); i++)
//	{
//		if (m_robotPlaceJetton.size() == 0)
//		{
//			return;
//		}
//		tagRobotPlaceJetton robotPlaceJetton = m_robotPlaceJetton[i];
//		CGamePlayer * pPlayer = robotPlaceJetton.pPlayer;
//
//		if (pPlayer == NULL) {
//			continue;
//		}
//		if (m_robotPlaceJetton.size() == 0)
//		{
//			return;
//		}
//		uint32 passtick = m_coolLogic.getPassTick();
//		uint32 totaltick = m_coolLogic.getTotalTick();
//
//		passtick = passtick / 1000;
//		totaltick = totaltick / 1000;
//
//		if (robotPlaceJetton.time <= passtick && m_robotPlaceJetton[i].bflag==false)
//		{
//			bool bflag = OnUserPlaceJetton(robotPlaceJetton.pPlayer, robotPlaceJetton.area, robotPlaceJetton.jetton);
//
//			m_robotPlaceJetton[i].bflag = true;
//			
//			iJettonCount++;
//
//		}
//		//LOG_DEBUG("uid:%d,time:%d,passtick:%d,totaltick:%d", pPlayer->GetUID(), robotPlaceJetton.time, passtick, totaltick);
//
//	}
//	uint32 uiIndex = 0;
//	while (true)
//	{
//		if (m_robotPlaceJetton.size() == 0)
//		{
//			return;
//		}
//		if (uiIndex >= m_robotPlaceJetton.size())
//		{
//			break;
//		}
//		if (m_robotPlaceJetton[uiIndex].bflag == true)
//		{
//			m_robotPlaceJetton.erase(m_robotPlaceJetton.begin() + uiIndex);
//			
//			iJettonCount--;
//
//			uiIndex = 0;
//		}
//		else
//		{
//			uiIndex++;
//		}
//		if (uiIndex >= m_robotPlaceJetton.size())
//		{
//			break;
//		}
//		if (iJettonCount <= 0)
//		{
//			break;
//		}
//	}
//	//LOG_DEBUG("uiIndex:%d,iJettonCount:%d,m_robotPlaceJetton.size:%d,getState:%d", uiIndex, iJettonCount, m_robotPlaceJetton.size(), m_coolRobotJetton.getState());
//
//}
//
//bool CGameBaccaratTable::PushRobotPlaceJetton(tagRobotPlaceJetton &robotPlaceJetton)
//{
//	for (uint32 i = 0; i < m_robotPlaceJetton.size(); i++)
//	{
//		if (m_robotPlaceJetton[i].pPlayer)
//		{
//			if (m_robotPlaceJetton[i].pPlayer == robotPlaceJetton.pPlayer && m_robotPlaceJetton[i].area == robotPlaceJetton.area)
//			{
//				return false;
//			}
//		}
//	}
//	m_robotPlaceJetton.push_back(robotPlaceJetton);
//	return true;
//}

void    CGameBaccaratTable::OnRobotStandUp()
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
void    CGameBaccaratTable::CheckRobotApplyBanker()
{   
    if(m_pCurBanker != NULL || m_ApplyUserArray.size() >= m_robotApplySize)
        return;
    LOG_DEBUG("robot apply banker - m_mpLookers.size:%d", m_mpLookers.size());
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL || !pPlayer->IsRobot())
            continue;
        int64 curScore = GetPlayerCurScore(pPlayer);
        if(curScore < GetApplyBankerCondition())
            continue;
        
        int64 buyinScore = GetApplyBankerCondition()*2;
        if(curScore < buyinScore)
        {
            OnUserApplyBanker(pPlayer,curScore,0);            
        }else{
            buyinScore = g_RandGen.RandRange(buyinScore,curScore);
            buyinScore = (buyinScore/10000)*10000;
            
            OnUserApplyBanker(pPlayer,buyinScore,1);     
        }        
        if(m_ApplyUserArray.size() > m_robotApplySize)
            break;
    }        
}
void    CGameBaccaratTable::AddPlayerToBlingLog()
{
    map<uint32,CGamePlayer*>::iterator it = m_mpLookers.begin();
    for(;it != m_mpLookers.end();++it)
    {
        CGamePlayer* pPlayer = it->second;
        if(pPlayer == NULL)
            continue;
        for(uint8 i=0;i<AREA_MAX;++i){
            if(m_userJettonScore[i][pPlayer->GetUID()] > 0){
                AddUserBlingLog(pPlayer);
                break;
            }
        }           
    }
    
    AddUserBlingLog(m_pCurBanker);
}
//��ʼ��ϴ��
void    CGameBaccaratTable::InitRandCard()
{
    m_poolCards.clear();
    BYTE cbCardListData[52 * 8];
    m_GameLogic.RandCardList(cbCardListData,sizeof(cbCardListData));
    for(uint32 i=0;i< sizeof(cbCardListData);++i)
    {
        m_poolCards.push_back(cbCardListData[i]);
    }
    RandPoolCard();
    LOG_DEBUG("��ʼ���Ƴ�:����:%d",m_poolCards.size());
}
//�Ƴ�ϴ��
void 	CGameBaccaratTable::RandPoolCard()
{
    for(uint32 i=0;i<m_poolCards.size();++i){
        std::swap(m_poolCards[i],m_poolCards[g_RandGen.RandUInt()%m_poolCards.size()]);
    }
}
//��һ����
BYTE    CGameBaccaratTable::PopCardFromPool()
{
    BYTE card = 0;
    if(m_poolCards.size() > 0){
        card = m_poolCards[0];
        m_poolCards.erase(m_poolCards.begin());
    }
    return card;
}



bool CGameBaccaratTable::OnChairRobotJetton()
{
	if (m_bIsChairRobotAlreadyJetton)
	{
		return false;
	}

	m_cbJettonArea = AREA_MAX;

	m_bIsChairRobotAlreadyJetton = true;
	m_chairRobotPlaceJetton.clear();
	for (uint32 i = 0; i < GAME_PLAYER; ++i)
	{
		CGamePlayer* pPlayer = GetPlayer(i);
		if (pPlayer == NULL || !pPlayer->IsRobot() || pPlayer == m_pCurBanker)
		{
			continue;
		}
		int iJettonCount = g_RandGen.RandRange(9, 13);
		uint8 cbJettonArea = GetRobotJettonArea(pPlayer);
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
				iJettonCount = g_RandGen.RandRange(9, 18);
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
				cbJettonArea = GetRobotJettonArea(pPlayer);

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
					robotPlaceJetton.time = g_RandGen.RandRange(1000, 4000);
					iJettonOldTime = robotPlaceJetton.time;
				}
			}
			else
			{
				robotPlaceJetton.time = g_RandGen.RandRange(1000, uMaxDelayTime - 500);
				iJettonOldTime = robotPlaceJetton.time;
			}

			if (bIsContinuouslyJetton == true)
			{
				robotPlaceJetton.time = iJettonOldTime + 1000;
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

void	CGameBaccaratTable::OnChairRobotPlaceJetton()
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

bool CGameBaccaratTable::OnRobotJetton()
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
		int iJettonCount = g_RandGen.RandRange(3, 18);
		uint8 cbJettonArea = GetRobotJettonArea(pPlayer);
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
				iJettonCount = g_RandGen.RandRange(9, 18);
			}
		}

		for (int iIndex = 0; iIndex < iJettonCount; iIndex++)
		{
			if (bIsContinuouslyJetton == false)
			{
				cbJettonArea = GetRobotJettonArea(pPlayer);

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
				iJettonOldTime = g_RandGen.RandRange(1000, 2000);
			}
			if (iJettonOldTime == -1)
			{
				robotPlaceJetton.time = g_RandGen.RandRange(1000, 2000);
				iJettonOldTime = robotPlaceJetton.time;
			}
			else
			{
				robotPlaceJetton.time = iJettonOldTime + g_RandGen.RandRange(1000, 3000);
				iJettonOldTime = robotPlaceJetton.time;
			}
			if (bIsContinuouslyJetton == true)
			{
				robotPlaceJetton.time = iJettonOldTime + 1000;
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


void	CGameBaccaratTable::OnRobotPlaceJetton()
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


bool    CGameBaccaratTable::IsInTableRobot(uint32 uid, CGamePlayer * pPlayer)
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

void CGameBaccaratTable::OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer)
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
		for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
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
		for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
		{
			info->add_area_info(m_userJettonScore[wAreaIndex][uid]);
			total_bet += m_userJettonScore[wAreaIndex][uid];
		}
		info->set_total_bet(total_bet);
		info->set_ismaster(IsBrcControlPlayer(uid));
	}
	pPlayer->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_ALL_PLAYER_BET_INFO);
}

void CGameBaccaratTable::OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer)
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
			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
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
			for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
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

void CGameBaccaratTable::OnBrcControlSendAllRobotTotalBetInfo()
{
	LOG_DEBUG("notice brc control all robot totol bet info.");

	net::msg_brc_control_total_robot_bet_info rep;
	for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
	{
		rep.add_area_info(m_allJettonScore[wAreaIndex] - m_playerJettonScore[wAreaIndex]);
		LOG_DEBUG("wAreaIndex:%d m_allJettonScore[%d]:%lld m_playerJettonScore[d]:%lld", wAreaIndex, wAreaIndex , m_allJettonScore[wAreaIndex], wAreaIndex, m_playerJettonScore[wAreaIndex]);
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_ROBOT_BET_INFO);
	}
}

void CGameBaccaratTable::OnBrcControlSendAllPlayerTotalBetInfo()
{
	LOG_DEBUG("notice brc control all player totol bet info.");

	net::msg_brc_control_total_player_bet_info rep;
	for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
	{
		rep.add_area_info(m_playerJettonScore[wAreaIndex]);
	}

	for (auto &it : m_setControlPlayers)
	{
		it->SendMsgToClient(&rep, net::S2C_MSG_BRC_CONTROL_TOTAL_PLAYER_BET_INFO);
	}
}

bool CGameBaccaratTable::OnBrcControlEnterControlInterface(CGamePlayer* pPlayer)
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

void CGameBaccaratTable::OnBrcControlBetDeal(CGamePlayer* pPlayer)
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

bool CGameBaccaratTable::OnBrcAreaControl()
{	
	LOG_DEBUG("brc area control.");

	if (m_real_control_uid==0)
	{
		LOG_DEBUG("brc area control the control uid is zero.");
		return false;
	}

	//��ȡ��ǰ��������
	uint8 ctrl_area = AREA_MAX;
	for (uint8 i = 0; i < AREA_MAX; ++i) 
	{
		if (m_req_control_area[i] == 1)
		{
			ctrl_area = i;
			break;
		}
	}
	if (ctrl_area == AREA_MAX)
	{
		LOG_DEBUG("brc area control the ctrl_area is error. ctrl_area:%d", ctrl_area);
		return false;
	}

	//����˿�
	if (m_poolCards.size() < 60) 
	{
		InitRandCard();
	}

	int irand_count = 1000;		// 100��û��������ͷ���
	int iRandIndex = 0;
	for (; iRandIndex < irand_count; iRandIndex++)
	{
		for (uint8 i = 0; i < 2; ++i)
		{
			m_cbTableCardArray[INDEX_PLAYER][i] = PopCardFromPool();
			m_cbTableCardArray[INDEX_BANKER][i] = PopCardFromPool();
		}

		//�״η���
		m_cbCardCount[INDEX_PLAYER] = 2;
		m_cbCardCount[INDEX_BANKER] = 2;

		//�������
		BYTE cbBankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);
		BYTE cbPlayerTwoCardCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);

		//�мҲ���
		BYTE cbPlayerThirdCardValue = 0;    //�������Ƶ���
		if (cbPlayerTwoCardCount <= 5 && cbBankerCount < 8)
		{
			//�������
			m_cbCardCount[INDEX_PLAYER]++;
			m_cbTableCardArray[INDEX_PLAYER][2] = PopCardFromPool();
			cbPlayerThirdCardValue = m_GameLogic.GetCardPip(m_cbTableCardArray[INDEX_PLAYER][2]);
		}
		//ׯ�Ҳ���
		if (cbPlayerTwoCardCount < 8 && cbBankerCount < 8)
		{
			switch (cbBankerCount)
			{
			case 0:
			case 1:
			case 2:
			{
				m_cbCardCount[INDEX_BANKER]++;
			}
			break;
			case 3:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 8) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 4:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 8 &&
					cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) || m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 5:
			{
				if ((m_cbCardCount[INDEX_PLAYER] == 3 && cbPlayerThirdCardValue != 1 && cbPlayerThirdCardValue != 2 &&
					cbPlayerThirdCardValue != 3
					&& cbPlayerThirdCardValue != 8 && cbPlayerThirdCardValue != 9 && cbPlayerThirdCardValue != 0) ||
					m_cbCardCount[INDEX_PLAYER] == 2) {
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			case 6:
			{
				if (m_cbCardCount[INDEX_PLAYER] == 3 && (cbPlayerThirdCardValue == 6 || cbPlayerThirdCardValue == 7))
				{
					m_cbCardCount[INDEX_BANKER]++;
				}
			}
			break;
			//���벹��
			case 7:
			case 8:
			case 9:
				break;
			default:
				break;
			}
		}
		if (m_cbCardCount[INDEX_BANKER] == 3)
		{
			m_cbTableCardArray[INDEX_BANKER][2] = PopCardFromPool();
		}

		//�����Ƶ�
		uint8 PlayerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_PLAYER], m_cbCardCount[INDEX_PLAYER]);
		uint8 BankerCount = m_GameLogic.GetCardListPip(m_cbTableCardArray[INDEX_BANKER], m_cbCardCount[INDEX_BANKER]);

		//�жϵ�ǰ���
		bool find_flag = false;
		switch (ctrl_area)
		{
			case AREA_XIAN:
			{
				if (PlayerCount > BankerCount)
				{
					find_flag = true;
				}
				break;
			}
			case AREA_PING:
			{
				if (PlayerCount == BankerCount)
				{
					find_flag = true;
				}
				break;
			}
			case AREA_ZHUANG:
			{
				if (PlayerCount < BankerCount)
				{
					find_flag = true;
				}
				break;
			}
			case AREA_XIAN_DUI:
			{
				if (m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_PLAYER][0]) == m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_PLAYER][1]))
				{
					find_flag = true;
				}
				break;
			}
			case AREA_ZHUANG_DUI:
			{
				if (m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_BANKER][0]) == m_GameLogic.GetCardValue(m_cbTableCardArray[INDEX_BANKER][1]))
				{
					find_flag = true;
				}
				break;
			}
			case AREA_SUPSIX:
			{
				if (PlayerCount < BankerCount && BankerCount == SUPER_SIX_NUMBER)
				{
					find_flag = true;
				}
				break;
			}
			case AREA_SMALL:
			{
				BYTE cbSumCardCount = m_cbCardCount[0] + m_cbCardCount[1];
				if (cbSumCardCount == 4)
				{
					find_flag = true;
				}
				break;
			}
			case AREA_BIG:
			{
				BYTE cbSumCardCount = m_cbCardCount[0] + m_cbCardCount[1];
				if (cbSumCardCount == 5 || cbSumCardCount == 6)
				{
					find_flag = true;
				}
				break;
			}
		}

		if (find_flag)
		{
			return true;
		}
		else
		{
			if (iRandIndex >= (irand_count - 1))
			{
				LOG_DEBUG("make card faild - irand_count:%d,m_poolCards.size:%d,iRandIndex:%d", irand_count, m_poolCards.size(), iRandIndex);

				return false;
			}
			for (uint8 i = 0; i < m_cbCardCount[INDEX_PLAYER]; ++i)
			{
				m_poolCards.push_back(m_cbTableCardArray[INDEX_PLAYER][i]);
			}
			for (uint8 i = 0; i < m_cbCardCount[INDEX_BANKER]; ++i) {
				m_poolCards.push_back(m_cbTableCardArray[INDEX_BANKER][i]);
			}
			RandPoolCard();
		}
	}
	return false;
}

void CGameBaccaratTable::OnBrcFlushSendAllPlayerInfo()
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
		for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
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
		for (WORD wAreaIndex = AREA_XIAN; wAreaIndex < AREA_MAX; ++wAreaIndex)
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

void CGameBaccaratTable::OnNotityForceApplyUser(CGamePlayer* pPlayer)
{
	LOG_DEBUG("Notity Force Apply uid:%d.", pPlayer->GetUID());
	
	net::msg_baccarat_apply_banker_rep msg;
	msg.set_apply_oper(0);
	msg.set_result(net::RESULT_CODE_SUCCESS);

	pPlayer->SendMsgToClient(&msg, net::S2C_MSG_BACCARAT_APPLY_BANKER);	
}