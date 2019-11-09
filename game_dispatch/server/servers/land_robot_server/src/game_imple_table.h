//
// Created by toney on 16/4/6.
//
// �������������߼�

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "land_logic.h"

#include "OGLordRobot.h"
#include "CardsEvaluating.h"

//#include "DDZRobot.h"

//  1.���������������ӮΪT.�������������ӮΪC.
//  2.���û�����ֹ���� T3<T2<T1<T.
//	3.��C����Tʱ��, ���Բ�����, Ҳ���Կ��������ӮһЩ, 1 / 5����ȥ������ҵ���.
//	4.��CС��T�Ҵ���T1ʱ��, 1 / 4����ȥ����AI�ĺ���.
//	5.��CС��T1�Ҵ���T2ʱ��, 1 / 2����ȥ����AI�ĺ���.
//	6.��CС��T2�Ҵ���T3ʱ��, 3 / 4����ȥ����AI�ĺ���.
//	7.��CС��T3ʱ��, 100 % ����ȥ����AI�ĺ���.

using namespace svrlib;
using namespace std;
using namespace game_land;

class CGamePlayer;
class CGameRoom;

// ��������Ϸ����
class CGameLandTable : public CGameTable
{
public:
    CGameLandTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameLandTable();

    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);

    virtual void GetTableFaceInfo(net::table_face_info* pInfo);
public:
    //��������
    virtual bool Init();
    virtual void ShutDown();
	virtual bool ReAnalysisParam();

    //��λ����
    virtual void ResetTable();
    virtual void OnTimeTick();
    //��Ϸ��Ϣ
    virtual int  OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);

public:
    // ��Ϸ��ʼ
    virtual bool OnGameStart();
    // ��Ϸ����
    virtual bool OnGameEnd(uint16 chairID,uint8 reason);
    //�û�ͬ��
    virtual bool OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer);
    //��ҽ�����뿪
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);
    // ���ͳ�����Ϣ(��������)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
public:
    // ���������˿���
    void    SendHandleCard(uint16 chairID);
    // �û�����
    bool    OnUserPassCard(uint16 chairID);
    // �û��з�
    bool    OnUserCallScore(uint16 chairID,uint8 score);
    void    OnCallScoreTimeOut();

    // �û�����
    bool    OnUserOutCard(uint16 chairID,uint8 cardData[],uint8 cardCount);
    void    OnOutCardTimeOut();
    // �йܳ���
    void    OnUserAutoCard();

    // ����ʱ����е���ʱ��
    uint32  GetCallScoreTime();
    uint32  GetOutCardTime();

    void    CalcPlayerInfo(CGamePlayer* pPlayer,uint16 chairID,uint8 win,uint8 spring,uint8 land,int64 winScore);
    // ��Ϸ���¿�ʼ
    void    ReGameStart();
    // ������Ϸ����
    void    ResetGameData();
    // ��ӷ�������
    void    AddBlockers();

protected:
	// ��ȡ������ע���ǻ����˻������  add by har
    // �����ע����Ϊ�����ˣ���isAllPlayer=false, ����isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
    // �����˿�
    void    ReInitPoker();
    // ϴ��
    void    ShuffleTableCard(uint8 dealType);
    // ���Ʋ����ص�ǰ���λ��
    uint16  DispatchUserCard(uint16 startUser,uint32 cardIndex);
    uint16  DispatchUserCardByDeal(const uint8 dealArry[],uint8 dealCount,uint16 startUser,uint32 cardIndex);

	uint16  PushUserCard(uint16 startUser, uint8 validCardData, uint8 & cardIndex);
	uint16  PushUserCardByDeal(const uint8 dealArry[], uint8 dealCount, uint16 startUser, uint8 validCardData, uint8 & cardIndex);

    // ������Ƴ���
    void    PushCardToPool(uint8 cardData[],uint8 cardCount);
    // ���׼����ʱ�Ĵ����Ŷ�
    void    CheckReadyTimeOut();

    // д�����log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount);
    void    WriteWinType(uint16 chairID,uint8 winType);
	void    WriteBankerCardLog(uint8 cardData[], uint8 cardCount);
	void    WriteRobotID();
    void    CheckAddRobot();
    // AI ����
	//��Ϸ��ʼ
	bool    OnSubGameStart();
	//ׯ����Ϣ
	bool    OnSubBankerInfo();
	//�û�����
	bool    OnSubOutCard(uint16 chairID);
    //�����˽з�
    bool    OnRobotCallScore();
    //�����˳���
    bool    OnRobotOutCard();
    //�Ƿ���Ҫ����˼��ʱ��
    bool    ReCalcRobotThinkTime();
    //���û�����˼��ʱ��
    bool    OnSetRobotThinkTime(bool bRecalc = false);
	bool    ChairIsRobot(uint16 chairID);
	void RobotCardJudge(BYTE handCardData[],int count, std::vector<int> & argHandCards);
	void RobotCardCheck(BYTE cbCardData[], std::vector<int> argHandCards);
	
	void KickPlayerInTable();

	void  SendHandleCard_2(uint16 chairID);

	//����ֵ����
	bool    SetLuckyCtrl();

    //��Ϸ����
protected:
    uint16  m_firstUser;                        // �׽��û�
    uint16  m_bankerUser;                       // ׯ���û�
    uint16  m_curUser;                          // ��ǰ�û�
    uint8   m_outCardCount[GAME_LAND_PLAYER];   // ���ƴ���
    uint8   m_sendHand[GAME_LAND_PLAYER];       // �Ƿ���
	uint16  m_evaluateUser;                          // 

    // ը����Ϣ
protected:
    uint8   m_bombCount;                        // ը������
    uint8   m_eachBombCount[GAME_LAND_PLAYER];  // ը������
    // �з���Ϣ
protected:
    uint8   m_callScore[GAME_LAND_PLAYER];      // �з���
    uint8   m_curCallScore;                     // ��ǰ�з�
    uint8   m_pressCount[GAME_LAND_PLAYER];     // ѹ�ƴ���
    uint8   m_bankrupts[GAME_LAND_PLAYER];      // �Ƿ��Ʋ�

    // ������Ϣ
protected:
    uint16  m_turnWiner;                        // ʤ�����
    uint8   m_turnCardCount;                    // ������Ŀ
    uint8   m_turnCardData[MAX_LAND_COUNT];     // ��������
	uint8   m_cbOutCardCount;                     // ������Ŀ
	uint8   m_outCardData[FULL_POKER_COUNT];      // ��������

    // �˿���Ϣ
protected:
    uint8   m_bankerCard[3];                        // ��Ϸ����
    uint8   m_handCardCount[GAME_LAND_PLAYER];      // �˿���Ŀ
    uint8   m_handCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];   // �����˿�

    uint8   m_allCardCount[GAME_LAND_PLAYER];      // �˿���Ŀ
    uint8   m_allCardData[GAME_LAND_PLAYER][MAX_LAND_COUNT];   // �����˿�
protected:
    CLandLogic      m_gameLogic;                    // ��Ϸ�߼�
    uint8           m_randCard[FULL_POKER_COUNT];   // ϴ������
    vector<uint8>   m_outCards;                     // �ѳ�����
    CCooling        m_coolRobot;                    // ������CD
    //CDDZAIManager   m_ddzAIRobot[GAME_LAND_PLAYER]; // �ٶȶ�����������

	//DDZRobotHttp	m_DDZRobotHttp[GAME_LAND_PLAYER];

	//COGLordRbtAIClv m_OGLordRbtAIClv[GAME_LAND_PLAYER];
	OGLordRobot m_LordRobot[GAME_LAND_PLAYER];
	// m_LordRobot[GAME_LAND_PLAYER];
	
	

	//OGLordRobot m_AutoLordRobot[GAME_LAND_PLAYER];
	uint32                          m_robotBankerWinPro;					//���Ƹ���


	bool    SetControlCardData();
	bool	ProgressControlPalyer();
	bool	SetControlPalyerWin(uint32 control_uid);
	bool	SetControlPalyerLost(uint32 control_uid);

	// ���ÿ����Ӯ  add by har
    // return  true ���������Ӯ  false δ����
	bool SetStockWinLose();

protected:
	uint16          m_isaddblockers;
	uint16			m_cbMemCardMac[GAME_LAND_PLAYER][FULL_POKER_COUNT];

	//bool			m_bGameEnd;
	//CCooling		m_coolKickPlayer;
};

#endif //SERVER_GAME_IMPLE_TABLE_H

