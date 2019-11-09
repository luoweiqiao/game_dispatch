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
#include "poker/niuniu_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_niuniu;

class CGamePlayer;
class CGameRoom;

//�������
#define GAME_PLAYER					6									//��λ����

//��������
#define ID_TIAN_MEN					0									//��
#define ID_DI_MEN					1									//��
#define ID_XUAN_MEN					2									//��
#define ID_HUANG_MEN				3									//��

#define AREA_COUNT					4									//������Ŀ
#define MAX_SEAT_INDEX              5                                   //��λ

//��������
enum emAREA
{
	AREA_TIAN_MEN = 0,							//��
	AREA_DI_MEN,								//��
	AREA_XUAN_MEN,								//��
	AREA_HUANG_MEN,								//��
	AREA_BANK,									//ׯӮ
	AREA_XIAN,									//��Ӯ
	AREA_MAX,									//�������
};

//��¼��Ϣ
struct bainiuGameRecord
{
	BYTE	wins[AREA_COUNT];//��Ӯ���
	bainiuGameRecord(){
		memset(this,0, sizeof(bainiuGameRecord));
	}
};
struct tagRobotPlaceJetton
{
	uint32 uid;
	bool bflag;
	CGamePlayer* pPlayer;
	uint8 area;
	int64 time;
	int64 jetton;
	tagRobotPlaceJetton()
	{
		uid = 0;
		bflag = false;
		pPlayer = NULL;
		area = 0;
		time = 0;
		jetton = 0;
	}
};

// ��ţ��Ϸ����
class CGameBaiNiuTable : public CGameTable
{
public:
    CGameBaiNiuTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameBaiNiuTable();

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
    virtual void ShutDown();
	virtual bool ReAnalysisParam();

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

	virtual void    OnNewDay();


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
	void    WriteAddScoreLog(uint32 uid,uint8 area,int64 score);
	// д���������
	void 	WriteMaxCardType(uint32 uid,uint8 cardType);
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

	//��������ׯ��ұ�ǿ����ׯ����Ҫ֪ͨ����ׯ���
	void  OnNotityForceApplyUser(CGamePlayer* pPlayer);

protected:
    //�����˿�
	bool  DispatchTableCard();
	//����ׯ��
	void  SendApplyUser(CGamePlayer* pPlayer = NULL);
	//����ׯ��
	void  FlushApplyUserSort();
	//����ׯ��
	bool  ChangeBanker(bool bCancelCurrentBanker);
	//�ֻ��ж�
	void  TakeTurns();
    //����ׯ��
    void  CalcBankerScore();  
    //�Զ�����
    void  AutoAddBankerScore();
	void  SendPlayLog(CGamePlayer* pPlayer);

	//��ע����
private:
	//�����ע
	int64   GetUserMaxJetton(CGamePlayer* pPlayer,BYTE cbJettonArea);
    uint32  GetBankerUID();
    //void    RemoveApplyBanker(uint32 uid);
    bool    LockApplyScore(CGamePlayer* pPlayer,int64 score);
    bool    UnLockApplyScore(CGamePlayer* pPlayer);
    //ׯ��վ��
    void    StandUpBankerSeat(CGamePlayer* pPlayer);
    bool    IsSetJetton(uint32 uid);
    bool    IsInApplyList(uint32 uid);
    //�Ƿ�����ʵ�����ע
	bool    IsUserPlaceJetton();
	//��Ϸͳ��
private:
	//����÷�
	int64   CalculateScore();
	// �ƶ�Ӯ�Ҵ��� add by har
	// cbTableCardArray �����ݴ��˿��ƶ�
	void DeduceWinnerDeal(bool &bWinTian, bool &bWinDi, bool &bWinXuan, bool &bWinHuan, uint8 &TianMultiple, uint8 &diMultiple, uint8 &TianXuanltiple, uint8 &HuangMultiple, uint8 cbTableCardArray[][5]);
	//�ƶ�Ӯ��
	void    DeduceWinner(bool &bWinTian, bool &bWinDi, bool &bWinXuan,bool &bWinHuan,BYTE &TianMultiple,BYTE &diMultiple,BYTE &TianXuanltiple,BYTE &HuangMultiple );
    
    
    //��������
    int64   GetApplyBankerCondition();
    int64   GetApplyBankerConditionLimit();
    
    //��������
    int32   GetBankerTimeLimit();

    //����ׯ�Ҷ�������
	bool	CompareApplyBankers(CGamePlayer* pBanker1,CGamePlayer* pBanker2);

    //�����˲���
protected:
	int64	GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area);

    void    OnRobotOper();
    void    OnRobotStandUp();
    void 	CheckRobotCancelBanker();

    void    CheckRobotApplyBanker();
    //���û�����ׯ��ӮǮ
    bool    SetRobotBankerWin(bool bBrankerIsRobot);
    //���û�����ׯ����Ǯ
    void    SetRobotBankerLose();

	bool    SetPlayerBrankerWin();
	
	bool    SetPlayerBrankerLost();

	bool	SetControlPlayerWin(uint32 control_uid);

	bool	SetControlPlayerLost(uint32 control_uid);

	bool    SetLeisurePlayerWin();

	bool    SetLeisurePlayerLost();

	// ���ÿ����Ӯ  add by har
    // return  true ���������Ӯ  false δ����
	bool SetStockWinLose();
	// ��ȡĳ����ҵ�Ӯ��  add by har
	// pPlayer : ���
	// bWinFlag : Ӯ�ı��
	// cbMultiple : Ӯ�ı���
	// lBankerWinScore : ׯ���ۼ�Ӯ��
	// return �ǻ��������Ӯ��
	int64 GetSinglePlayerWinScore(CGamePlayer *pPlayer, bool bWinFlag[AREA_COUNT], uint8 cbMultiple[], int64 &lBankerWinScore);
	// ��ȡׯ�Һͷǻ��������Ӯ����� add by har
    // cbTableCardArray : ���������
    // bankerWinScore out : ׯ��Ӯ��
    // return : �ǻ��������Ӯ�����
	int64 GetBankerAndPlayerWinScore(uint8 cbTableCardArray[][5], int64 &lBankerWinScore);

	bool	GetPlayerGameRest(uint32 uid);

	bool	GetCardSortIndex(uint8 uArSortIndex[]);

	bool	GetJettonSortIndex(uint32 uid,uint8 uArSortIndex[]);

    void    AddPlayerToBlingLog();
    //test 
    void    TestMultiple();

	void    GetAllRobotPlayer(vector<CGamePlayer*> & robots);

        
    //��Ϸ����
protected:
    //����ע��
protected:
	int64						    m_allJettonScore[AREA_COUNT];		            //ȫ����ע
	int64							m_playerJettonScore[AREA_COUNT];				//�����ע

    map<uint32,int64>               m_userJettonScore[AREA_COUNT];                  //������ע
    map<uint32,int64>			    m_mpUserWinScore;			                    //��ҳɼ�
	//�˿���Ϣ
protected:
    BYTE							m_cbTableCardArray[MAX_SEAT_INDEX][5];		    //�����˿�
    BYTE                            m_cbTableCardType[MAX_SEAT_INDEX];              //��������
    int32                           m_winMultiple[AREA_COUNT];                      //��Ӯ����
    
	//ׯ����Ϣ
protected:	
    //vector<CGamePlayer*>			m_ApplyUserArray;						//�������
    //map<uint32,uint8>               m_mpApplyUserInfo;                      //�Ƿ��Զ�����
    
    //map<uint32,int64>               m_ApplyUserScore;                       //����������
    
	//CGamePlayer*					m_pCurBanker;						    //��ǰׯ��
    uint8                           m_bankerAutoAddScore;                   //�Զ�����
    //bool                            m_needLeaveBanker;                      //�뿪ׯλ
        
    uint16							m_wBankerTime;							//��ׯ����
    uint16                          m_wBankerWinTime;                       //ʤ������
	int64						    m_lBankerScore;							//ׯ�һ���
	int64						    m_lBankerWinScore;						//�ۼƳɼ�

    int64                           m_lBankerBuyinScore;                    //ׯ�Ҵ���
    int64                           m_lBankerInitBuyinScore;                //ׯ�ҳ�ʼ����
    int64                           m_lBankerWinMaxScore;                   //ׯ�������Ӯ
    int64                           m_lBankerWinMinScore;                   //ׯ�������Ӯ
	uint16							m_cbBrankerSettleAccountsType;			//ׯ�ҽ�������
    
	//���Ʊ���
protected:
	int								m_iMaxJettonRate;
	tagNiuMultiple					m_tagNiuMultiple;

	//int64							m_lMaxPollScore;						  //��󽱳ط���
	//int64							m_lMinPollScore;						  //��С���ط���
	//int64							m_lCurPollScore;						  //��ǰ���ط���
	//int64							m_lFrontPollScore;						  //
	//uint32							m_uSysWinPro;							  //ϵͳӮ����
	//uint32							m_uSysLostPro;							  //ϵͳ�����


	//�������
protected:
	CNiuNiuLogic					m_GameLogic;							//��Ϸ�߼�
    uint32                          m_BankerTimeLimit;                      //ׯ�Ҵ���
    
    uint32                          m_robotBankerWinPro;                    //������ׯ��Ӯ����
	uint32                          m_robotBankerMaxCardPro;                //������ׯ������Ƹ���
    uint32                          m_robotApplySize;                       //��������������
    uint32                          m_robotChairSize;                       //��������λ��      

	bainiuGameRecord              	m_record;
	vector<bainiuGameRecord>		m_vecRecord;	                        //��Ϸ��¼

	

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
	int64							m_lGameCount;
public:
		void OnRobotTick();

		uint32						m_uBairenTotalCount;
		vector<uint32>				m_vecAreaWinCount;
		vector<uint32>				m_vecAreaLostCount;
		void AddGameCount();
		void InitAreaSize(uint32 count);
		void OnTablePushAreaWin(uint32 index, int win);

	//���˳���׼����
public:
	void   OnBrcControlSendAllPlayerInfo(CGamePlayer* pPlayer);			//��������������ʵ�����ע����
	void   OnBrcControlNoticeSinglePlayerInfo(CGamePlayer* pPlayer);	//֪ͨ���������Ϣ
	void   OnBrcControlSendAllRobotTotalBetInfo();						//�������л���������ע��Ϣ
	void   OnBrcControlSendAllPlayerTotalBetInfo();						//����������ʵ�������ע��Ϣ
	bool   OnBrcControlEnterControlInterface(CGamePlayer* pPlayer);		//������ƽ���
	void   OnBrcControlBetDeal(CGamePlayer* pPlayer);					//��ע����
	bool   OnBrcAreaControl();											//���˳��������
	bool   OnBrcAreaControlForA(uint8 ctrl_area_a);						//���˳�A������� ׯӮ/ׯ��
	bool   OnBrcAreaControlForB(set<uint8> &area_list);					//���˳�B������� ��/��/��/�� ֧�ֶ�ѡ
	void   OnBrcFlushSendAllPlayerInfo();								//ˢ������������ʵ�����Ϣ---��ҽ���/�˳�����ʱ����
	
	bool   SetControlBankerScore(bool isWin);							//����ׯ������ΪӮ/�� ͨ����������
};

#endif //SERVER_GAME_IMPLE_TABLE_H

