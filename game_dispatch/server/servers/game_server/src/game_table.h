
#ifndef __GAME_TABLE_H__
#define __GAME_TABLE_H__

#include "game_player.h"
#include "svrlib.h"
#include "pb/msg_define.pb.h"
#include "center_log.h"
#include "json/json.h"


using namespace svrlib;
using namespace std;

class CGamePlayer;
class CGameRoom;

enum BRANKER_SETTLE_ACCOUNTS_TYPE
{
	BRANKER_TYPE_NULL				= 0,	// 正常结算
	BRANKER_TYPE_TAKE_ALL			= 1,	// 庄家通杀
	BRANKER_TYPE_COMPENSATION		= 2,	// 庄家通赔
};

enum GAME_END_TYPE
{
    GER_NORMAL      = 0,		//常规结束
    GER_DISMISS,		        //游戏解散
    GER_USER_LEAVE,	            //用户强退
    GER_NETWORK_ERROR,	        //网络中断

};

enum GAME_CONTROL_PALYER_TYPE
{
	GAME_CONTROL_CANCEL			= 0,	// 取消控制
	GAME_CONTROL_WIN			= 1,	// 控制赢
	GAME_CONTROL_LOST			= 2,	// 控制输
};

enum GAME_CONTROL_MULTI_PALYER_TYPE
{
	GAME_CONTROL_MULTI_CANCEL = 0,	// 取消控制
	GAME_CONTROL_MULTI_WIN = 1,	// 控制赢
	GAME_CONTROL_MULTI_LOST = 2,	// 控制输
	GAME_CONTROL_MULTI_ALL_CANCEL = 3,	// 取消全部控制
};

#define CONTROL_TIME_OUT		(5*60*1000)		//控制超时(5分钟)
#define BRC_MAX_CONTROL_AREA	(20)			//所有百人场最大的下注区域值

struct tagControlPalyer
{
	uint32			uid;
	int32			count;
	uint32			type; //控制类型 0 取消 1 赢 2 输
	uint64			utime;
	tagControlPalyer()
	{
		Init();
	}
	void Init()
	{
		uid = 0;
		count = 0;
		type = 0;
	}
};

struct tagControlMultiPalyer
{
	uint32			uid;
	int32			count;	//保留不使用
	uint32			type;	//控制类型 0 输 1 赢 2 取消uid 3 取消全部
	uint64			ctime;	//控制时间 0 永久
	uint64			stime;	//控制开始时间
	uint64			etime;	//控制结束时间
	int64			tscore; //赢总分数
	uint16			chairID;//椅子ID
	tagControlMultiPalyer()
	{
		Init();
	}
	void Init()
	{
		uid = 0;
		count = 0;
		type = 0;
		ctime = 0;
		stime = 0;
		etime = 0;
		tscore = 0;
		chairID = 255;
	}
};

struct tagJackpotScore
{
	int64	lMaxPollScore;		//最大奖池分数
	int64	lMinPollScore;		//最小奖池分数
	int64	lCurPollScore;		//当前奖池分数
	int64	lDiffPollScore;		//奖池分数变化
	uint32	uSysWinPro;			//系统赢概率
	uint32	uSysLostPro;		//系统输概率
	uint32	uSysLostWinProChange; //吃币吐币概率变化
	int64	lUpdateJackpotScore;// 更新奖池分数
	int		iUserJackpotControl; //是否使用奖池控制 1使用 否则不适用
	uint64	ulLastRestTime;		//最后重置时间
	int tm_min;   // minutes after the hour - [0, 59]
	int tm_hour;  // hours since midnight - [0, 23]

	tagJackpotScore()
	{
		Init();
	}
	void Init()
	{
		lMaxPollScore = 0;
		lMinPollScore = 0;
		lCurPollScore = 0;
		lDiffPollScore = 0;
		uSysWinPro = 0;
		uSysLostPro = 0;
		uSysLostWinProChange = 0;
		lUpdateJackpotScore = 0;
		iUserJackpotControl = 0;
		ulLastRestTime = 0;
	}
	void UpdateCurPoolScore(int64 lScore)
	{
		__sync_add_and_fetch(&lCurPollScore, lScore);
	}
	void SetCurPoolScore(int64 lScore)
	{
		int64 lPoolScore = -lCurPollScore;
		__sync_add_and_fetch(&lCurPollScore, lPoolScore);
		__sync_add_and_fetch(&lCurPollScore, lScore);
	}
};
// 座位信息
struct stSeat
{
    CGamePlayer* pPlayer;
    uint8        readyState;  // 准备状态
    uint8        autoState;   // 托管状态
    uint32       readyTime;   // 准备时间
    uint8        overTimes;   // 超时次数
    uint32       uid;         // 德州梭哈用户游戏结束前退出用uid处理
    int64        buyinScore;  // buyin游戏币
    stSeat(){
        Reset();
    }
    void Reset(){
        memset(this,0,sizeof(stSeat));
    }
};
// 桌子类型
enum TABLE_TYPE
{
    emTABLE_TYPE_SYSTEM  = 0, // 系统桌子
    emTABLE_TYPE_PLAYER  = 1, // 玩家桌子
};

// 桌子类型
enum BRC_TABLE_STATUS
{
	emTABLE_STATUS_FREE = 1,	// 空闲
	emTABLE_STATUS_START = 2,	// 开始
	emTABLE_STATUS_END = 3,		// 结束
};

// 桌子信息
struct stTableConf
{
    string tableName;   // 桌子名字
    string passwd;      // 密码
    string hostName;    // 房主名字
    uint32 hostID;      // 房主ID
    uint8  deal;        // 发牌方式
    int64  baseScore;   // 底分
    uint8  consume;     // 消费类型
    int64  enterMin;    // 最低进入
    int64  enterMax;    // 最大带入
    uint8  isShow;      // 是否显示
    uint8  feeType;     // 台费类型
    int64  feeValue;    // 台费值
    uint32 dueTime;     // 到期时间
    uint16  seatNum;    // 座位数

    void operator=(const stTableConf& conf)
    {
        hostID    = conf.hostID;
        tableName = conf.tableName;
        passwd    = conf.passwd;
        hostName  = conf.hostName;
        hostID    = conf.hostID;
        deal      = conf.deal;
        baseScore = conf.baseScore;
        consume   = conf.consume;
        enterMin  = conf.enterMin;
        enterMax  = conf.enterMax;
        isShow    = conf.isShow;
        feeType   = conf.feeType;
        feeValue  = conf.feeValue;
        dueTime   = conf.dueTime;
        seatNum   = conf.seatNum;      // 座位数
    }
    stTableConf(){
        hostID    = 0;
        tableName = "送钱";
        passwd    = "";
        hostName  = "隔壁老王";
        hostID    = 0;
        deal      = 0;
        baseScore = 0;
        consume   = 0;
        enterMin  = 0;
        enterMax  = 0;
        isShow    = 0;
        feeType   = 0;
        feeValue  = 0;
        dueTime   = 0;
        seatNum   = 0;  // 座位数
    }
};


struct tagGameSortInfo
{
	//CGamePlayer * pPlayer;
	uint32 uid;
	int wincount;
	int64 betscore;
	tagGameSortInfo()
	{
		//pPlayer = NULL;
		uid = 0;
		wincount = 0;
		betscore = 0;
	}
};

struct tagMasterShowUserInfo
{
	int64 lMinScore;
	int64 lMaxScore;
	tagMasterShowUserInfo()
	{
		lMinScore = 300000;
		lMaxScore = 1000000;
	}
};

// 游戏桌子
class CGameTable
{
public:
    CGameTable(CGameRoom* pRoom,uint32 tableID,uint8 tableType);
    virtual ~CGameTable();
	uint16			GetRoomID();
	uint16			GetGameType();
    void            SetTableID(uint32 tableID){ m_tableID = tableID; }
    uint32          GetTableID(){ return m_tableID; };
    uint8           GetTableType(){ return m_tableType; }
    CGameRoom*      GetHostRoom();

    virtual bool    EnterTable(CGamePlayer* pPlayer);
	virtual bool    EnterEveryColorTable(CGamePlayer* pPlayer);
    virtual bool    LeaveTable(CGamePlayer* pPlayer,bool bNotify = false);
    // 从新匹配
    virtual void    RenewFastJoinTable(CGamePlayer* pPlayer);

    virtual bool    AddLooker(CGamePlayer* pPlayer);
    virtual bool    RemoveLooker(CGamePlayer* pPlayer);
	virtual bool    AddEveryColorLooker(CGamePlayer* pPlayer);
	virtual bool    RemoveEveryColorLooker(CGamePlayer* pPlayer);

    virtual bool    IsExistLooker(uint32 uid);
    
    virtual bool    CanEnterTable(CGamePlayer* pPlayer);
    virtual bool    CanLeaveTable(CGamePlayer* pPlayer);
    virtual bool    CanSitDown(CGamePlayer* pPlayer,uint16 chairID);
    virtual bool    CanStandUp(CGamePlayer* pPlayer);
    virtual bool    NeedSitDown();
    virtual void    SetRobotEnterCool(uint32 time){ m_robotEnterCooling.beginCooling(time); }

    virtual bool    PlayerReady(CGamePlayer* pPlayer);
    virtual bool    PlayerSitDownStandUp(CGamePlayer* pPlayer,bool sitDown,uint16 chairID);
    
    virtual bool    ResetPlayerReady();
    virtual bool    IsAllReady();
    virtual bool    PlayerSetAuto(CGamePlayer* pPlayer,uint8 autoType);
    virtual bool    IsReady(CGamePlayer* pPlayer);
    virtual void    ReadyAllRobot();
    virtual bool    IsExistIP(uint32 ip);
    virtual bool    IsExistBlock(CGamePlayer* pPlayer);

    virtual uint32  GetPlayerNum();
    virtual uint32  GetChairPlayerNum();
    virtual uint32  GetOnlinePlayerNum();
    virtual uint32  GetReadyNum();
    virtual bool    IsFullTable();
    virtual bool    IsEmptyTable();
    virtual uint32  GetFreeChairNum();

    virtual uint32  GetChairNum();
    virtual bool    IsAllDisconnect();

    virtual void    SetGameState(uint8 state);
    virtual uint8   GetGameState();
    virtual CGamePlayer* GetPlayer(uint16 chairID);
    virtual uint32  GetPlayerID(uint16 chairID);
    virtual uint16  GetChairID(CGamePlayer* pPlayer);
    virtual uint16  GetRandFreeChairID();

    virtual void    SendMsgToLooker(const google::protobuf::Message* msg,uint16 msg_type);
    virtual void    SendMsgToPlayer(const google::protobuf::Message* msg,uint16 msg_type);
    virtual void    SendMsgToAll(const google::protobuf::Message* msg,uint16 msg_type);
    virtual void    SendMsgToClient(uint16 chairID,const google::protobuf::Message* msg,uint16 msg_type);

    virtual void    SendTableInfoToClient(CGamePlayer* pPlayer);
    virtual void    SendReadyStateToClient();
    virtual void    SendSeatInfoToClient(CGamePlayer* pGamePlayer = NULL);
    virtual void    SendLookerListToClient(CGamePlayer* pGamePlayer = NULL);
	virtual void    SendPalyerLookerListToClient(CGamePlayer* pGamePlayer = NULL);
    virtual void    NotifyPlayerJoin(CGamePlayer* pPlayer,bool isJoin);
    virtual void    NotifyPlayerLeave(CGamePlayer* pPlayer);
    
    virtual int64   ChangeScoreValue(uint16 chairID,int64& score,uint16 operType,uint16 subType);
    virtual int64   ChangeScoreValueByUID(uint32 uid,int64& score,uint16 operType,uint16 subType);
	virtual int64   ChangeScoreValueInGame(uint32 uid, int64& score, uint16 operType, uint16 subType, string chessid);
	virtual int64   NotifyChangeScoreValueInGame(uint32 uid, int64 score, uint16 operType, uint16 subType);

    virtual uint8   GetConsumeType();
    virtual int64   GetBaseScore();
    virtual int64   GetEnterMin();
    virtual int64   GetEnterMax();
    virtual uint32  GetDeal();
    virtual bool    IsShow();
    virtual uint32  GetHostID();
    virtual bool    IsRightPasswd(string passwd);

	virtual int64   GetJettonMin();
	virtual bool	TableJettonLimmit(CGamePlayer * pPlayer, int64 lJettonScore, int64 lAllyJetton);


    virtual int64   GetPlayerCurScore(CGamePlayer* pPlayer);
    virtual int64   ChangePlayerCurScore(CGamePlayer* pPlayer,int64 score);

    void    SetTableConf(stTableConf& conf){ m_conf = conf; }
    stTableConf& GetTableConf(){ return m_conf; }

    // 私人房操作
    virtual void    RenewPrivateTable(uint32 days);
    virtual void    LoadPrivateTable(stPrivateTable& data);
    virtual bool    CreatePrivateTable();
    virtual void    ChangePrivateTableIncome(int64 hostIncome,int64 sysIncome);
    virtual void    UpdatePrivateTableDuetime();
    // 牌局日志
	void InitChessID();
	void SetEmptyChessID();
	string GetChessID() { return m_chessid; }
	bool IsBaiRenCount(uint16 gameType);

	void OnAddPlayerJetton(uint32 uid, int64 score);
	void OnSetPlayerWinScore(uint32 uid, int64 score);
	void OnCaclBairenCount();
	void InitPlayerBairenCoint(CGamePlayer * pPlayer);
	bool CalculateDeity();
	void DivineMathematicianAndRichSitdown(std::vector<std::shared_ptr<struct tagGameSortInfo>> & vecGameSortRank);
	static bool CompareRankByWinCount(std::shared_ptr<struct tagGameSortInfo> sptagSort1, std::shared_ptr<struct tagGameSortInfo> sptagSort2);
	static bool CompareRankByBetScore(std::shared_ptr<struct tagGameSortInfo> sptagSort1, std::shared_ptr<struct tagGameSortInfo> sptagSort2);
	void CaclPlayerBaiRenCount();
	bool   ForcePlayerSitDownStandUp(CGamePlayer* pPlayer, bool sitDown, uint16 chairID);
	CGamePlayer * GetGamePlayerByUid(uint32 uid);

	void OnTableGameStart();

	void RecordPlayerBaiRenJettonInfo(CGamePlayer * pPlayer, BYTE area, int64 score);
	bool ContinuousPressureBaiRenGame();
	void RecordFrontJettonInfo();
	void SendFrontJettonInfo(CGamePlayer * pPlayer);
	void SendAllFrontJettonInfo();

	void AddEnterScore(CGamePlayer * pPlayer);
	virtual int64 GetEnterScore(CGamePlayer * pPlayer);
	void RemoveEnterScore(CGamePlayer * pPlayer);
	void UpdateEnterScore(bool isJoin, CGamePlayer * pPlayer);
	void BuyEnterScore(CGamePlayer * pPlayer, int64 lScore);


    virtual void    InitBlingLog(bool bNeedRead = false);
	virtual void InitBlingLog(CGamePlayer* pPlayer);

    virtual void    AddUserBlingLog(CGamePlayer* pPlayer);
	virtual void    FlushUserBlingLog(CGamePlayer* pPlayer, int64 winScore, int64 lJackpotScore = 0, uint8 land = 0);
	virtual void    FlushUserNoExistBlingLog(uint32 uid, int64 winScore, int64 fee, uint8 land = 0);
    virtual void    ChangeUserBlingLogFee(uint32 uid,int64 fee);
    virtual void    SaveBlingLog();

    // 扣除开始台费
    virtual int64    DeductStartFee(bool bNeedReady);
    // 扣除结算台费
    virtual int64    DeducEndFee(uint32 uid,int64& winScore);
        
    // 上报游戏战报
    virtual void    ReportGameResult(uint32 uid,int64 winScore,int64 lExWinScore);
    // 玩家贡献度记录
    virtual void    LogFee(uint32 uid,int64 feewin,int64 feelose);
    // 结算玩家信息
	virtual int64   CalcPlayerGameInfo(uint32 uid, int64& winScore, int64 lJackpotScore = 0, bool bDeductFee = true);

	// 结算玩家信息---捕鱼场
	virtual void   CalcPlayerGameInfoForFish(uint32 uid, int64 winScore);

public:
    // 是否需要回收
    virtual bool    NeedRecover();
    virtual void    GetTableFaceInfo(net::table_face_info* pInfo) = 0;
	virtual int64	GetPlayerTotalWinScore(CGamePlayer *pPlayer);
	void    SaveUserBankruptScore();

public:
	//配置桌子
	virtual bool    Init()         = 0;
    virtual void    ShutDown()     = 0;
    //复位桌子
    virtual void    ResetTable()   = 0;
    virtual void    OnTimeTick()   = 0;
	//游戏消息
	virtual int     OnGameMessage(CGamePlayer* pPlayer,uint16 cmdID, const uint8* pkt_buf, uint16 buf_len) = 0;
	//用户断线或重连
	virtual bool    OnActionUserNetState(CGamePlayer* pPlayer,bool bConnected,bool isJoin = true);
	//用户坐下
	virtual bool    OnActionUserSitDown(WORD wChairID,CGamePlayer* pPlayer);
	//用户起立
	virtual bool    OnActionUserStandUp(WORD wChairID,CGamePlayer* pPlayer);
	//用户同意
	virtual bool    OnActionUserOnReady(WORD wChairID,CGamePlayer* pPlayer);    
    //玩家进入或离开
    virtual void    OnPlayerJoin(bool isJoin,uint16 chairID,CGamePlayer* pPlayer);
    // 发送场景信息(断线重连)
    virtual void    SendGameScene(CGamePlayer* pPlayer);
	//桌子游戏结束
	virtual void    OnTableGameEnd();
    //检查该svr是否要退休
    virtual void    CheckRetireTableUser();

	bool GetIsGameEndKickUser(uint16 gameType);
	void KickLittleScoreUser();

 public:
	 uint16			GetRobotPlayerCount();
	 //重新解析房间Param
	 virtual bool	ReAnalysisParam();
	 //更新控制玩家
	 virtual bool	UpdateControlPlayer(uint32 uid, uint32 gamecount, uint32 operatetype);
	 //停止夺宝
	 virtual bool	SetSnatchCoinState(uint32 stop);

	 virtual bool	DiceGameControlCard(uint32 gametype, uint32 roomid, uint32 udice[]);

	 virtual bool MajiangConfigHandCard(uint32 gametype, uint32 roomid, string strHandCard);

	 //控制玩家局数同步
	 virtual bool	SynControlPlayer(uint32 uid, int32 gamecount, uint32 operatetype);
	 //获取下注分数
	 virtual int64	GetJettonScore(CGamePlayer *pPlayer = NULL);
	 //更新最大下注分数
	 virtual bool	UpdateMaxJettonScore(CGamePlayer* pPlayer = NULL, int64 lScore = 0);
	 //游戏中通知客户端更新分数
	 virtual bool	UpdataScoreInGame(uint32 uid, uint32 gametype, int64 lScore);

     virtual void  RetireUser(CGamePlayer *pPlayer);

	 virtual void    SaveBroadcastLog(stGameBroadcastLog &BroadcastLog);   //广播日志

	 bool			OnTableTick();
	 virtual bool	UpdateControlMultiPlayer(uint32 uid, uint32 gamecount, uint64 gametime, int64 totalscore, uint32 operatetype);

	 //百人场精准控制
	 virtual bool   OnBrcControlPlayerEnterInterface(CGamePlayer *pPlayer);		//进入控制界面
	 virtual bool   OnBrcControlPlayerLeaveInterface(CGamePlayer *pPlayer);		//离开控制界面
	 virtual bool   OnBrcControlPlayerBetArea(CGamePlayer *pPlayer, net::msg_brc_control_area_info_req & msg);		//控制区域请求
	 virtual void   OnBrcControlGameEnd();										//百人场游戏结束后，发送实际控制区域结果
	 virtual void   OnBrcControlPlayerEnterTable(CGamePlayer *pPlayer);			//玩家进入游戏
	 virtual void   OnBrcControlPlayerLeaveTable(CGamePlayer *pPlayer);			//玩家离开游戏
	 virtual void   OnBrcControlSetResultInfo(uint32 uid, int64 win_score);		//百人场游戏结束后，修改当前玩家的统计信息---每个子类调用
	 virtual bool   OnBrcControlApplePlayer(CGamePlayer *pPlayer, uint32 target_uid);		//强制玩家下庄
	 virtual void   OnBrcControlFlushTableStatus(CGamePlayer *pPlayer=NULL);							//刷新百人场桌子状态
	 bool			OnBrcControlFlushAreaInfo(CGamePlayer *pPlayer);			//百人场控制账号刷新控制区域信息		
	 uint32			IsBrcControlPlayer(uint32 uid);								//是否控制玩家 

	 uint32			GetBankerUID();
	 void			RemoveApplyBanker(uint32 uid);
	 bool			LockApplyScore(CGamePlayer* pPlayer, int64 score);
	 bool			UnLockApplyScore(CGamePlayer* pPlayer);
	 virtual void   OnBrcControlFlushAppleList();		//刷新上庄玩家列表
	 virtual bool	ChangeBanker(bool bCancelCurrentBanker) { return false; };
	 virtual void	SendApplyUser(CGamePlayer* pPlayer = NULL) {};
	 virtual void	OnNotityForceApplyUser(CGamePlayer* pPlayer) {};
	 //是否为超权用户
	 bool GetIsMasterUser(CGamePlayer* pPlayer);
	 //是否跟踪用户
	 bool GetIsMasterFollowUser(CGamePlayer* pPlayer, CGamePlayer* pTagPlayer);
	 //是否能够进入桌子
	 bool CanEnterCtrledUserTable(CGamePlayer* pGamePlayer);
	 //离开用户
	 bool LeaveMasterUser(CGamePlayer *pPlayer);
	 //初始化超权信息
	 void InitMasterUserShowInfo(CGamePlayer *pPlayer);
	 //更新超权信息
	 void UpdateMasterUserShowInfo(CGamePlayer *pPlayer, int64 lScore);
	 //更新游戏信息
	 void UpdateMasterGameInfo();
	 //是否是超权游戏
	 bool IsMasterGame();
	 //离开机器人
	 bool LeaveRobotUser();

	 // 获取下注的是否全部为机器人或全部为玩家（针对库存系统）  add by har
     // return false 其他  true 全部为机器人或全部为玩家
	 bool IsAllRobotOrPlayerJetton();
	 // 设置最大椅子数
	 void SetMaxChairNum(uint16 maxChairNum) { m_maxChairNum = maxChairNum; };
	 
	 //获取当前桌子的幸运值标志，参数返回:赢玩家一个，输玩家为列表 一个或多个
	 bool GetTableLuckyFlag(uint32 &win_uid, set<uint32> &set_lose_uid);

	 // 获取是否全部为玩家或机器人下注
	 bool GetIsAllRobotOrPlayerJetton() { return m_isAllRobotOrPlayerJetton; }

protected:
	// 设置是否全部为玩家或机器人下注
	void SetIsAllRobotOrPlayerJetton(bool isAllRobotOrPlayerJetton) { m_isAllRobotOrPlayerJetton = isAllRobotOrPlayerJetton; }
	// 获取单个下注的是机器人还是玩家（针对库存系统）  add by har
	// 玩家下注，且为机器人，则isAllPlayer=false, 否则isAllRobot=false
	virtual void IsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot) {};
	// 处理获取单个下注的是机器人还是玩家（针对对战类库存系统）  add by har
	// cbPlayStatus ： 玩家游戏状态
	void DealIsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot, uint8	cbPlayStatus[]);
	// 处理获取单个下注的是机器人还是玩家（针对少数玩家对战类库存系统）  add by har
	// cbPlayStatus ： 玩家游戏状态
	void DealSIsRobotOrPlayerJetton(CGamePlayer *pPlayer, bool &isAllPlayer, bool &isAllRobot);
	// 检查库存改变是否成功  add by har
	bool CheckStockChange(int64 stockChange, int64 playerAllWinScore, int i);
	// 庄家是否是真实玩家  add by har
	bool IsBankerRealPlayer();

	CGameRoom*                  m_pHostRoom;        // 所属房间
    stTableConf                 m_conf;             // 桌子配置
    vector<stSeat>              m_vecPlayers;       // 玩家
    map<uint32,CGamePlayer*>    m_mpLookers;        // 围观者
    uint8                       m_gameState;        // 游戏状态
    uint32                      m_tableID;          // 桌子ID
    CCooling                    m_coolLogic;        // 逻辑CD
    uint8                       m_tableType;        // 桌子类型(动态桌子，私人桌子);
    int64                       m_hostIncome;       // 桌子收益(私人房)
    int64                       m_sysIncome;        // 系统收益
    stGameBlingLog              m_blingLog;         // 牌局日志
    Json::Value                 m_operLog;          // 出牌操作LOG;
    string                      m_chessid;          // 牌局ID
    CCooling                    m_robotEnterCooling;// 机器人进入冷却
	tagControlPalyer			m_tagControlPalyer;	//控制玩家变量
	map<uint32, tagControlMultiPalyer> m_mpControlMultiPalyer;

	std::map<uint32, std::shared_ptr<struct tagBairenCount>> m_mpBairenPalyerBet;
	std::vector<std::shared_ptr<std::map<uint32, std::shared_ptr<struct tagBairenCount>>>> m_vecBairenCount;

	std::map<uint32, std::map<BYTE, int64>> m_mpCurPlayerJettonInfo;
	std::shared_ptr<std::map<uint32, std::map<BYTE, int64>>> m_spFrontPlayerJettonInfo;

	map<uint32, int64> m_mpFirstEnterScore;

	uint16 m_maxChairNum; // 最大椅子数 add by har

	//百人场精准控制
	set<CGamePlayer*>    m_setControlPlayers;       // 控制玩家列表---保存所有在控制界面
	int8                 m_control_number;			// 控制局数
	uint8				 m_req_control_area[BRC_MAX_CONTROL_AREA];	// 请求控制区域
	uint8				 m_real_control_area[BRC_MAX_CONTROL_AREA];	// 游戏结束时实际控制区域
	uint32				 m_real_control_uid;		// 实际控制玩家ID---多个玩家控制的情况下，取最后一个玩家的控制结果
	map<uint32, tagPlayerResultInfo>	m_mpPlayerResultInfo;	//所有玩家的在当前游戏的输赢结果信息
	set<uint32>			 m_tableCtrlPlayers;		// 控制玩家列表---进入桌子

	vector<CGamePlayer*>			m_ApplyUserArray;			//申请上庄玩家列表
	map<uint32, uint8>              m_mpApplyUserInfo;          //是否自动补币
	CGamePlayer*					m_pCurBanker;				//当前庄家
	bool                            m_needLeaveBanker;          //离开庄位
	map<uint32, int64>              m_ApplyUserScore;           //申请带入积分

	uint8				m_brc_table_status;			//百人场游戏状态
	uint32				m_brc_table_status_time;	//百人场游戏状态对应的倒计时

	bool				m_lucky_flag;				//当前局是否触发幸运值
	set<uint32>			m_set_ctrl_lucky_uid;		//当前局触发幸运值的玩家列表
	bool m_isAllRobotOrPlayerJetton; // 是否全部为玩家或机器人下注，针对库存系统
};

#endif //__GAME_TABLE_H__


