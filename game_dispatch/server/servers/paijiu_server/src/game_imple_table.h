//
// Created by toney on 16/6/28.
//
// �����ƾŵ������߼�

#ifndef SERVER_GAME_IMPLE_TABLE_H
#define SERVER_GAME_IMPLE_TABLE_H

#include <json/value.h>
#include "game_table.h"
#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "poker/paijiu_logic.h"

using namespace svrlib;
using namespace std;
using namespace game_paijiu;

class CGamePlayer;
class CGameRoom;

//�������
#define GAME_PLAYER					6									//��λ����

#define AREA_COUNT					7									//������Ŀ
#define MAX_SEAT_INDEX              4                                   //��λ

//��������
#define ID_HENG_L					0									//���
#define ID_SHUN_MEN					1									//˳��
#define ID_JIAO_L					2									//��߽�
#define ID_DUI_MEN					3									//����
#define ID_JIAO_R					4									//�ұ߽�
#define ID_DAO_MEN					5									//����
#define ID_HENG_R					6									//�Һ�

//�������
#define BANKER_INDEX				0									//ׯ������
#define SHUN_MEN_INDEX				1									//˳������
#define DUI_MEN_INDEX				2									//��������
#define DAO_MEN_INDEX				3									//��������

#define MAX_CARD					2

//������������
enum emAREA
{
	AREA_HENG = 0,							//��
	AREA_SHUN_MEN,							//˳��
	AREA_JIAO_L,							//��߽�
	AREA_DUI_MEN,							//����
	AREA_JIAO_R,							//�ұ߽�
	AREA_DAO_MEN,							//����
	AREA_BANK,								//ׯӮ
	AREA_XIAN,								//��Ӯ
	AREA_MAX,								//�������
};

struct paijiuGameRecord
{
	BYTE	wins[AREA_COUNT];//��Ӯ���
	paijiuGameRecord() {
		memset(this, 0, sizeof(paijiuGameRecord));
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
// �ƾ���Ϸ����
class CGamePaijiuTable : public CGameTable
{
public:
    CGamePaijiuTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGamePaijiuTable();

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
    void    WriteOutCardLog(uint16 chairID,uint8 cardData[],uint8 cardCount);
	// д���עlog
	void    WriteAddScoreLog(uint32 uid, uint8 area, int64 score, int64 win);
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
	bool  ChangeBanker(bool bCancelCurrentBanker);
	//�ֻ��ж�
	void  TakeTurns();
    //����ׯ��
    void  CalcBankerScore();  
    //�Զ�����
    void  AutoAddBankerScore();
    

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
    
	//��Ϸͳ��
private:
	//����÷�
	int64   CalculateScore();
	//�ƶ�Ӯ��
	void    DeduceWinner(bool &bWinShunMen, bool &bWinDuiMen, bool &bWinDaoMen);
	//bool	ControlBrankerWin();
    
    //��������
    int64   GetApplyBankerCondition();
    int64   GetApplyBankerConditionLimit();
    
    //��������
    int32   GetBankerTimeLimit();
    
	void	SendPlayLog(CGamePlayer* pPlayer);

    //�����˲���
protected:
	int64	GetRobotJettonScore(CGamePlayer* pPlayer, uint8 area);

    void    OnRobotOper();
    void    OnRobotStandUp();
    
    void    CheckRobotApplyBanker();
    void    AddPlayerToBlingLog();

	bool    SetPlayerBrankerWin();

	bool    SetPlayerBrankerLost();

	bool	SetRobotBrankerWin();

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
	// lBankerWinScore : ׯ��Ӯ��
    // return �ǻ��������Ӯ��
	int64 GetSinglePlayerWinScore(CGamePlayer *pPlayer, bool bWinFlag[AREA_COUNT], int64 &lBankerWinScore);
	// ��ȡ�ǻ��������Ӯ�� add by har
    // return : �ǻ��������Ӯ��
	int64 GetBankerAndPlayerWinScore();

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
    BYTE							m_cbTableCardArray[MAX_SEAT_INDEX][MAX_CARD];   //�����˿�
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
	uint32                          m_robotBankerWinPro;                    //������ׯ��Ӯ����    
	uint32                          m_playerBankerLosePro;                  //��ʵ���ׯ����

	//int64							m_lMaxPollScore;						  //��󽱳ط���
	//int64							m_lMinPollScore;						  //��С���ط���
	//int64							m_lCurPollScore;						  //��ǰ���ط���
	//int64							m_lFrontPollScore;						  //
	//uint32							m_uSysWinPro;							  //ϵͳӮ����
	//uint32							m_uSysLostPro;							  //ϵͳ�����

	//�������
protected:
	CPaijiuLogic					m_GameLogic;							//��Ϸ�߼�
    uint32                          m_BankerTimeLimit;                      //ׯ�Ҵ���

    uint32                          m_robotApplySize;                       //��������������
    uint32                          m_robotChairSize;                       //��������λ��

	paijiuGameRecord              	m_record;
	vector<paijiuGameRecord>		m_vecRecord;	                        //��Ϸ��¼

	bool IsInTableRobot(uint32 uid, CGamePlayer * pPlayer);

	bool OnChairRobotJetton();
	void OnChairRobotPlaceJetton();

	bool							m_bIsChairRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_chairRobotPlaceJetton;				//��ע�Ļ�����


	bool OnRobotJetton();
	void OnRobotPlaceJetton();
	bool							m_bIsRobotAlreadyJetton;
	vector<tagRobotPlaceJetton>		m_RobotPlaceJetton;				//��ע�Ļ�����

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
	bool   OnBrcAreaControlForA(vector<uint8> &area_list);				//���˳�A�������
	bool   OnBrcAreaControlForB(uint8 ctrl_area_b);						//���˳�B�������
	void   OnBrcFlushSendAllPlayerInfo();								//ˢ������������ʵ�����Ϣ---��ҽ���/�˳�����ʱ����

};

#endif //SERVER_GAME_IMPLE_TABLE_H

