//
// Created by toney on 16/6/28.
//
// ����ţţ�������߼�

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "poker/baccarat_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_baccarat;

class CGamePlayer;
class CGameRoom;

//�������
#define GAME_PLAYER					6									//��λ����
#define MAX_SEAT_INDEX              2                                   //�����λ
#define CARD_NUM                    3
#define INDEX_PLAYER				0									//�м�����
#define INDEX_BANKER				1									//ׯ������

#define SUPER_SIX_RATE				(0.5)								//supersix
#define SUPER_SIX_NUMBER			(6)									//supersix



//�������
enum emAREA
{
    AREA_XIAN = 0,													//�м�����
    AREA_PING,														//ƽ������
    AREA_ZHUANG,													//ׯ������
    AREA_XIAN_DUI,													//�ж���
    AREA_ZHUANG_DUI,												//ׯ����
	AREA_SUPSIX,													//6
	AREA_SMALL,														//С
	AREA_BIG,														//��
    AREA_MAX,														//�������
};

#define	MAX_BACCARAT_GAME_RECORD	(10)

//��¼��Ϣ
struct baccaratGameRecord
{
    BYTE							bPlayerTwoPair;						//���ӱ�ʶ
    BYTE							bBankerTwoPair;						//���ӱ�ʶ
    BYTE							cbPlayerCount;						//�мҵ���
    BYTE							cbBankerCount;						//ׯ�ҵ���
	BYTE							cbIsSmall;							//�Ƿ�С
	BYTE							cbIsSuperSix;

    baccaratGameRecord(){
        memset(this,0,sizeof(baccaratGameRecord));
    }
};

struct tagRobotPlaceJetton {
	bool bflag;
	CGamePlayer* pPlayer;
	uint8 area;
	int64 time;
	int64 jetton;
	uint32 uid;
	tagRobotPlaceJetton() {
		bflag = false;
		pPlayer = NULL;
		area = AREA_MAX;
		time = 0;
		jetton = 0;
		uid = 0;
	}
};

// �ټ�����Ϸ����
class CGameBaccaratTable : public CGameTable
{
public:
    CGameBaccaratTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameBaccaratTable();

// ���ػ��ຯ��
public:
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);
    virtual bool    CanSitDown(CGamePlayer* pPlayer,uint16 chairID);
    virtual bool    CanStandUp(CGamePlayer* pPlayer); 
    
    virtual bool    IsFullTable();
    virtual void    GetTableFaceInfo(net::table_face_info* pInfo);
public:
    //��������
    virtual bool Init();
	virtual bool ReAnalysisParam();

    virtual void ShutDown();

    //��λ����
    virtual void ResetTable();
    virtual void OnTimeTick();
    //��Ϸ��Ϣ
    virtual int  OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len);
    //�û����߻�����
    virtual bool OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin = true);
	//�û�����
	virtual bool OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer);
	//�û�����
	virtual bool OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer); 
public:
    // ��Ϸ��ʼ
    virtual bool OnGameStart();
    // ��Ϸ����
    virtual bool OnGameEnd(uint16 chairID,uint8 reason);
    //��ҽ�����뿪
    virtual void OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);

public:
    // ���ͳ�����Ϣ(��������)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
    
	int64    CalcPlayerInfo(uint32 uid,int64 winScore,bool isBanker = false);
    // ������Ϸ����
    void    ResetGameData();
    void    ClearTurnGameData();
    
protected:
	// ��ȡ������ע���ǻ����˻������  add by har
    // �����ע����Ϊ�����ˣ���isAllPlayer=false, ����isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
    // д�����log
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount,int32 mulip);
    // д���עlog
    void    WriteAddScoreLog(uint32 uid,uint8 area,int64 score,int64 win);
    // д��ׯ����Ϣ
    void    WriteBankerInfo();

    
    //��Ϸ�¼�
protected:
	//��ע�¼�
	bool OnUserPlaceJetton(CGamePlayer* pPlayer, BYTE cbJettonArea, int64 lJettonScore);
	//����ׯ��
	bool OnUserApplyBanker(CGamePlayer* pPlayer,int64 bankerScore,uint8 autoAddScore);
    bool OnUserJumpApplyQueue(CGamePlayer* pPlayer);
	bool OnUserContinuousPressure(CGamePlayer* pPlayer, net::msg_player_continuous_pressure_jetton_req & msg);

	//ȡ������
	bool OnUserCancelBanker(CGamePlayer* pPlayer);
    
protected:
	bool  DispatchRandTableCard();
	//�����˿�
	bool DispatchTableCard();
	//�����˿�
	bool  DispatchTableCardBrankerIsRobot();
	//��ʵ��ҵ�ׯ����
	bool  DispatchTableCardBrankerIsPlayer();
	
	bool  DispatchTableCardControlPalyerWin(uint32 control_uid);

	bool  DispatchTableCardControlPalyerLost(uint32 control_uid);

	bool    SetBrankerWin();

	bool    SetBrankerLost();

	bool    SetLeisurePlayerWin();

	bool    SetLeisurePlayerLost();

	// ���ÿ����Ӯ  add by har
	// return  true ���������Ӯ  false δ����
	bool SetStockWinLose();
	// ��ȡĳ����ҵ�Ӯ��  add by har
    // pPlayer : ���
    // cbWinArea : Ӯ������
    // cbBankerCount : ׯ�ҵ���
    // lBankerWinScore : ׯ��Ӯ��
    // return �ǻ��������Ӯ��
	int64 GetSinglePlayerWinScore(CGamePlayer *pPlayer, uint8 cbWinArea[AREA_MAX], uint8 cbBankerCount, int64 &lBankerWinScore);

	//����ׯ��
	void  SendApplyUser(CGamePlayer* pPlayer = NULL);
	//����ׯ��
	bool  ChangeBanker(bool bCancelCurrentBanker);
	//�ֻ��ж�
	void  TakeTurns();
    //����ׯ��
    void  CalcBankerScore();  
    //�Զ�����
    void  AutoAddBankerScore();
    //������Ϸ��¼
    void  SendPlayLog(CGamePlayer* pPlayer);
	//��������ׯ��ұ�ǿ����ׯ����Ҫ֪ͨ����ׯ���
	void  OnNotityForceApplyUser(CGamePlayer* pPlayer);

	//��ע����
private:
	//�����ע
	int64   GetUserMaxJetton(CGamePlayer* pPlayer,BYTE cbJettonArea);
	uint8   GetRobotJettonArea(CGamePlayer* pPlayer);

    //uint32  GetBankerUID();
    //void    RemoveApplyBanker(uint32 uid);
   // bool    LockApplyScore(CGamePlayer* pPlayer,int64 score);
   // bool    UnLockApplyScore(CGamePlayer* pPlayer);
    //ׯ��վ��
    void    StandUpBankerSeat(CGamePlayer* pPlayer);
    bool    IsSetJetton(uint32 uid);
    bool    IsInApplyList(uint32 uid);
    
	//��Ϸͳ��
private:
	//����÷�
	int64   CalculateScore();
	//�ƶ�Ӯ��
	// return ׯ�ҵ��� add by har
	uint8 DeduceWinner(BYTE* pWinArea);
    
    
    //��������
    int64   GetApplyBankerCondition();
    int64   GetApplyBankerConditionLimit();
    
    //��������
    int32   GetBankerTimeLimit();
    
    //�����˲���
protected:
	int64	GetRobotJettonScore(CGamePlayer* pPlayer,uint8 area);
    void    OnRobotOper();
	void    OnChairRobotOper();
    void    OnRobotStandUp();
    
    void    CheckRobotApplyBanker();

    void    AddPlayerToBlingLog();
	//void	OnRobotPlaceJetton();
	//void OnChairRobotPlaceJetton();
	//bool	PushRobotPlaceJetton(tagRobotPlaceJetton &robotPlaceJetton);
	//��ʼ��ϴ��
	void    InitRandCard();
	//�Ƴ�ϴ��
	void 	RandPoolCard();
	//��һ����
	BYTE    PopCardFromPool();
	//���ƾ���
	int		m_imake_card_count;
	//���ƾ���
	int		m_inotmake_card_count;
	//�Ѿ�����û����
	int		m_inotmake_card_round;
	//���������
	int		m_imax_notmake_round;

	int64   m_lGameRount;
    //��Ϸ����
protected:
    //����ע��
protected:
	int64						    m_allJettonScore[AREA_MAX];		                //ȫ����ע
	int64							m_playerJettonScore[AREA_MAX];					//�����ע
    map<uint32,int64>               m_userJettonScore[AREA_MAX];                    //������ע
    map<uint32,int64>			    m_mpUserWinScore;			                    //��ҳɼ�
	//�˿���Ϣ
protected:


    //�˿���Ϣ
protected:
    BYTE							m_cbCardCount[2];						//�˿���Ŀ
    BYTE							m_cbTableCardArray[2][3];				//�����˿�
    BYTE                            m_cbTableCardType[MAX_SEAT_INDEX];      //��������
    BYTE                            m_cbWinArea[AREA_MAX];

	vector<BYTE>                    m_poolCards;                            //�Ƴ��˿�

	//ׯ����Ϣ
protected:	
    //vector<CGamePlayer*>			m_ApplyUserArray;						//�������---�Ƶ�����
    //map<uint32,uint8>               m_mpApplyUserInfo;                      //�Ƿ��Զ�����
    
    //map<uint32,int64>               m_ApplyUserScore;                       //����������
    
	//CGamePlayer*					m_pCurBanker;						    //��ǰׯ��
    uint8                           m_bankerAutoAddScore;                   //�Զ�����
    //bool                            m_needLeaveBanker;                    //�뿪ׯλ
        
    uint16							m_wBankerTime;							//��ׯ����
    uint16                          m_wBankerWinTime;                       //ʤ������
	int64						    m_lBankerScore;							//ׯ�һ���
	int64						    m_lBankerWinScore;						//�ۼƳɼ�

	int64							m_lBankerShowScore;						//ׯ����ʾ����
    int64                           m_lBankerBuyinScore;                    //ׯ�Ҵ���
    int64                           m_lBankerInitBuyinScore;                //ׯ�ҳ�ʼ����
    int64                           m_lBankerWinMaxScore;                   //ׯ�������Ӯ
    int64                           m_lBankerWinMinScore;                   //ׯ�������Ӯ
	uint16							m_cbBrankerSettleAccountsType;			//ׯ�ҽ�������

    
	//���Ʊ���
protected:
	uint32                          m_playerBankerLosePro;                    //��ʵ���ׯ����
	
	//int64							m_lMaxPollScore;						  //��󽱳ط���
	//int64							m_lMinPollScore;						  //��С���ط���
	//int64							m_lCurPollScore;						  //��ǰ���ط���
	//int64							m_lFrontPollScore;						  //��һ�ֽ��ر仯һ��
	//uint32							m_uSysWinPro;							  //ϵͳӮ����
	//uint32							m_uSysLostPro;							  //ϵͳ�����

	//�������
protected:
	CBaccaratLogic					m_GameLogic;							//��Ϸ�߼�
    uint32                          m_BankerTimeLimit;                      //ׯ�Ҵ���


    uint32                          m_robotApplySize;                       //��������������
    uint32                          m_robotChairSize;                       //��������λ��
	//vector<tagRobotPlaceJetton>		m_robotPlaceJetton;						//��ע�Ļ�����
	//vector<tagRobotPlaceJetton>		m_chairRobotPlaceJetton;				//��ע�Ļ�����
	//bool m_bIsChairRobotAlreadyJetton;
	CCooling						m_coolRobotJetton;						//��������עCD
    baccaratGameRecord              m_record;
    vector<baccaratGameRecord>		m_vecRecord;	                        //��Ϸ��¼

	bool IsInTableRobot(uint32 uid, CGamePlayer * pPlayer);

	bool OnChairRobotJetton();
	void OnChairRobotPlaceJetton();
	bool							m_bIsChairRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_chairRobotPlaceJetton;				//��ע�Ļ�����

																			//
	bool OnRobotJetton();
	void OnRobotPlaceJetton();
	bool							m_bIsRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_RobotPlaceJetton;				//��ע�Ļ�����

	uint8 m_cbJettonArea;

public:
	void OnRobotTick();

//���˳���׼����
public:
	void   OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer);			//��������������ʵ�����ע����
	void   OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer);	//֪ͨ���������Ϣ
	void   OnBrcControlSendAllRobotTotalBetInfo();						//�������л���������ע��Ϣ
	void   OnBrcControlSendAllPlayerTotalBetInfo();						//����������ʵ�������ע��Ϣ
	bool   OnBrcControlEnterControlInterface(CGamePlayer* pPlayer);		//������ƽ���
	void   OnBrcControlBetDeal(CGamePlayer* pPlayer);					//��ע����
	bool   OnBrcAreaControl();											//���˳��������
	void   OnBrcFlushSendAllPlayerInfo();								//ˢ������������ʵ�����Ϣ---��ҽ���/�˳�����ʱ����
};

#endif //SERVER_GAME_IMPLE_TABLE_H

