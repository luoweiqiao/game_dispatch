
#ifndef DB_STRUCT_DEFINE_H__
#define DB_STRUCT_DEFINE_H__


// 玩家基础数据
struct stPlayerBaseInfo
{
    string name;         // 姓名    
    uint8  sex;          // 性别
    uint32 offlinetime;  // 最后登录时间
    string safepwd;      // 保险箱密码
    uint8  safeboxState; // 保险箱状态
    uint32 clogin;       // 连续登陆天数
    uint8  weekLogin;    // 本周累计登陆
    uint32 reward;       // 领奖标记
    uint8  bankrupt;     // 破产次数
    uint16 headIcon;     // 头像ID
    uint32 dayGameCount; // 今日游戏次数
    uint32 loginIP;      // 登陆IP

    stPlayerBaseInfo(){
        clear();
    }
    void operator=(const stPlayerBaseInfo& info)
	{
        name = info.name;
        sex  = info.sex;
        offlinetime  = info.offlinetime;
        safepwd      = info.safepwd;
        safeboxState = info.safeboxState;
        clogin       = info.clogin;
        weekLogin    = info.weekLogin;
        reward       = info.reward;
        bankrupt     = info.bankrupt;
        headIcon     = info.headIcon;
        dayGameCount = info.dayGameCount;
        loginIP      = info.loginIP;

	}
    void  clear(){
        name         = "";
        sex          = 0;
        offlinetime  = 0;
        safepwd      = "";
        safeboxState = 0;
        clogin       = 0;
        weekLogin    = 0;
        reward       = 0;
        bankrupt     = 0;
        headIcon     = 0;
        dayGameCount = 0;
        loginIP      = 0;

    }
};
// 玩家数值信息
struct stAccountInfo
{
    int64  diamond;      // 钻石
    int64  coin;         // 财富币
    int64  score;        // 积分
    int64  ingot;        // 元宝
    int64  cvalue;       // 魅力值   
    int32  vip;          // vip值
    int64  safecoin;     // 保险箱资产
	int64  recharge;	 // 充值总金额
	int64  converts;	 // 总提现金额(兑换)
    stAccountInfo(){
        clear();
    }
    void  clear(){
        memset(this,0,sizeof(stAccountInfo));
    }    
};
enum emACC_VALUE_TYPE
{
    emACC_VALUE_DIAMOND = 1,        // 钻石
    emACC_VALUE_COIN    = 2,        // 财富币
    emACC_VALUE_INGOT   = 3,        // 元宝
    emACC_VALUE_SCORE   = 4,        // 积分
    emACC_VALUE_CVALUE  = 5,        // 魅力值
    emACC_VALUE_SAFECOIN= 6,        // 保险箱值
	//emACC_VALUE_STOCKSCORE = 7,     // 库存分数
    emACC_VALUE_MAX,                //max
};
// 基本游戏信息
struct stGameBaseInfo
{
    int32  win;
    int32  lose;
    int64  maxwin;
    int64  daywin;
    int32  winc;
    int32  losec;
    int64  maxwinc;
    int64  daywinc;    
    uint8  bestCard[5];
    uint8  bestCardc[5];

    //----斗地主专属----
    int32 land;    // 地主次数
    int32 spring;  // 春天次数
    int32 landc;
    int32 springc;

	int64 totalwinc;
	int64 stockscore;
	int64 gamecount;
	vector<int> vecwin;
	vector<int64> vecbet;

    stGameBaseInfo(){
       // memset(this,0,sizeof(stGameBaseInfo));
		win = 0;
		lose = 0;
		maxwin = 0;
		daywin = 0;
		winc = 0;
		losec = 0;
		maxwinc = 0;
		daywinc = 0;
		memset(bestCard, 0, sizeof(bestCard));
		memset(bestCardc, 0, sizeof(bestCardc));

		land = 0;
		spring = 0;
		landc = 0;
		springc = 0;

		totalwinc = 0;
		stockscore = 0;
		gamecount = 0;
		vecwin.clear();
		vecbet.clear();
    }
};
#define MAX_STATISTICS_GAME_COUNT 20

struct tagBairenCount
{
	string chessid;
	int svrid;
	int gametype;
	int roomid;
	int tableid;
	uint32 uid;
	int64 winscore;
	int64 betscore;
	tagBairenCount()
	{
		Init();
	}
	void Init()
	{
		chessid.clear();
		svrid = 255;
		gametype = 255;
		roomid = 255;
		tableid = 255;
		uid = 0;
		winscore = 0;
		betscore = 0;
	}
};

// 房间配置信息
struct stRoomCfg
{
    uint16  roomID;       // 房间ID
    uint8   consume;      // 消费类型
    uint8   deal;         // 发牌类型
    int64   enter_min;    // 进入门槛
    int64   enter_max;    // 进入限制
    int64   baseScore;    // 底分
    uint8   roomType;     // 房间类型(0普通房,1私人房)
    uint8   showhandNum;  // 显示手牌
    uint16  tableNum;     // 创建桌子数
    uint8   marry;        // 匹配方式
    uint8   limitEnter;   // 进入限制(通过最小携带)
    uint8   robot;        // 是否开启机器人
    uint32  showonline;   // 显示在线
    int64   sitdown;      // 坐下条件
    uint8   feeType;      // 台费类型
    int32   feeValue;     // 台费值
    uint16  seatNum;      // 座位数
    uint16  showType;     // 显示类型
    uint16  showPic;      // 显示图片
    int64   robotMaxScore;// 机器人最大进入
	int64   robotMinScore;// 机器人最小进入
	int64   jettonMinScore;//最小下注
    string  param;        // 特殊参数
    
    stRoomCfg()
    {        
        param = "";
    }
};

// 房间库存奖池配置信息
struct stStockCfg {
	uint16  roomID;       // 房间ID
	// 库存控制
	int64 stockMin; // 库存下限
	int64 stockMax; // 库存上限
	int stockConversionRate; // 库存转化率
	int64 stock; // 实时库存

	// 奖池控制
	int64 jackpotMin; // 奖池触发最低金额
	int jackpotMaxRate; // 奖池最大触发金额比例
	int jackpotRate; // 奖池触发概率
	int jackpotCoefficient; // 奖池系数
	int jackpotExtractRate; // 奖池抽取率
	int64 jackpot; // 实时奖池
	int64 killPointsLine; // 杀分警戒线
	int playerWinRate; // 玩家赢概率

	int64 fee; // 实时抽水

	stStockCfg()
	{
		ZeroMemory(this, sizeof(stStockCfg));
	}
};

//任务奖励配置
struct stMissionPrizeCfg
{
    uint32 poid;   //道具id
    uint32 qty;    //数量
    stMissionPrizeCfg(){
        memset(this,0,sizeof(stMissionPrizeCfg));
    }
};

//任务配置信息
struct stMissionCfg
{
    uint32 msid;        //任务ID
    uint16 type;        //动作类型
    uint8  autoprize;   //自动领奖
    uint32 cate1;       //分类
    uint32 cate2;       //分类
    uint32 cate3;       //分类
    uint32 cate4;       //分类
    uint32 mtimes;      //达到次数
    uint8  straight;     //是否连续
    uint8  cycle;        //周期
    uint32 cycletimes;  //可完成次数
    uint8  status;       //任务状态
    vector<stMissionPrizeCfg> missionprize; //任务奖励
};
//玩家任务信息
struct stUserMission
{
    uint32 msid;    // ID
    uint32 rtimes;  // 任务进度
    uint32 ctimes;  // 任务完成次数
    uint32 ptime;   // 操作时间
    uint8  update;  // 更新数据库类型
    uint32 cptime;  // 完成时间
    stUserMission(){
        memset(this,0,sizeof(stUserMission));
    }
};

// 数据类型
enum emACCDATA_TYPE
{
    emACCDATA_TYPE_BASE = 1,    // 基本信息
    emACCDATA_TYPE_ACC,         // 账户信息
    emACCDATA_TYPE_MISS,        // 任务数据
    emACCDATA_TYPE_GAME,        // 游戏数据
    
    emACCDATA_TYPE_MAX,
};

enum emDBEVENT
{    
    emDBEVENT_LOAD_PLAYER_DATA,         // 加载玩家数据
    emDBEVENT_LOAD_ACCOUNT_DATA,        // 加载帐号数据
    emDBEVENT_LOAD_MISSION_DATA,        // 加载任务数据    
    emDBEVENT_LOAD_GAME_DATA,           // 加载游戏数据
    emDBEVENT_LOAD_ROBOT_LOGIN,         // 加载机器人登陆
    emDBEVENT_SEND_MAIL,                // 发送邮件
	emDBEVENT_SAVE_GAME_DATA,           // 保存游戏数据
    
	emDBEVENT_MAX,
};
 
enum emDBACTION
{
    emDB_ACTION_NONE = 0,           //不操作
    emDB_ACTION_UPDATE,             //修改
    emDB_ACTION_INSERT,             //插入
    emDB_ACTION_DELETE,             //删除
};

// 私人房信息
struct stPrivateTable
{
    uint32 tableID;     // 桌子ID
    string tableName;   // 桌子名字
    string passwd;      // 密码
    string hostName;    // 房主名字
    uint32 hostID;      // 房主ID
    uint8  deal;        // 发牌方式
    int64  baseScore;   // 底分
    uint8  consume;     // 消费类型
    int64  enterMin;    // 最低进入
    uint8  isShow;      // 是否显示
    uint8  feeType;     // 台费类型
    int64  feeValue;    // 台费值
    uint32 dueTime;     // 到期时间
    int64  hostIncome;  // 房主收益
    int64  sysIncome;   // 系统收益
    uint16 gameType;    // 游戏类型
    uint32 createTime;  // 创建时间

    void operator=(const stPrivateTable& info)
    {
        tableID    = info.tableID;
        hostID     = info.hostID;
        tableName  = info.tableName;
        passwd     = info.passwd;
        hostName   = info.hostName;
        hostID     = info.hostID;
        deal       = info.deal;
        baseScore  = info.baseScore;
        consume    = info.consume;
        enterMin   = info.enterMin;
        isShow     = info.isShow;
        feeType    = info.feeType;
        feeValue   = info.feeValue;
        dueTime    = info.dueTime;
        hostIncome = info.hostIncome;
        sysIncome  = info.sysIncome;
        gameType   = info.gameType;
        createTime = info.createTime;

    }
    stPrivateTable(){
        tableID    = 0;
        hostID     = 0;
        tableName  = "送钱";
        passwd     = "";
        hostName   = "隔壁老王";
        hostID     = 0;
        deal       = 0;
        baseScore  = 0;
        consume    = 0;
        enterMin   = 0;
        isShow     = 0;
        feeType    = 0;
        feeValue   = 0;
        dueTime    = 0;
        hostIncome = 0;
        sysIncome  = 0;
        gameType   = 0;
        createTime = 0;
    }
};
// 兑换配置
struct stExchangeCfg
{
    uint16  id;
    int64   fromValue;
    int64   toValue;
    stExchangeCfg(){
        memset(this,0,sizeof(stExchangeCfg));
    }
};
// 机器人配置信息
struct stRobotCfg
{
    uint32 uid;
    uint8  actionType; // 活跃类型
    uint8  playerType; // 打牌类型
    uint8  scoreLv;    // 积分等级 
    uint8  richLv;     // 财富等级
    uint8  status;     // 机器状态
    uint32 actiontime; // 活跃时间
    uint16 gameType;   // 游戏类型
	uint32 loginType;  // 登录类型
	uint32 batchID;	   // 批次 I D
	uint32 leveltype;  // 等级类型  1积分2财富币
    stRobotCfg(){
        memset(this,0,sizeof(stRobotCfg));
    }
};

#define ROBOT_MAX_LEVEL 17

// 机器人在线配置信息
struct stRobotOnlineCfg
{
	int    iLoadindex;
	uint32 batchID;		// 批次 I D

    uint16 gameType;	// 游戏类型
	uint16 roomID;		// 房间 I D

	uint32 leveltype;	// 等级类型  1积分2财富币
	uint32 loginType;	// 登录类型
	uint32 enterTime;	// 进入时间
	uint32 leaveTime;	// 离开时间

    uint32 onlines[ROBOT_MAX_LEVEL]; // 在线人数
    stRobotOnlineCfg()
	{
		init();
    }
	void init()
	{
		memset(this, 0, sizeof(stRobotOnlineCfg));
		iLoadindex = 0;
		roomID = 255;
	}
};


//夺宝游戏数据结构
struct tagSnatchCoinGameData
{
	uint32		uid;
	uint32		type;
	string      card;
};

struct tagVipRechargeWechatInfo
{
	uint32 sortid;//微信排序
	uint32 low_amount;//最低充值额度
	int32 shour;
	int32 sminute;
	int32 ehour;
	int32 eminute;
	vector<int> vecVip;
	string account;//微信账号
	string title;//微信标题
	bool timecontrol;
	string showtime;
	string strVip;
	tagVipRechargeWechatInfo()
	{
		Init();
	}
	void Init()
	{
		sortid = 0;
		low_amount = 0;
		shour = 0;
		sminute = 0;
		ehour = 0;
		eminute = 0;
		account.clear();
		title.clear();
		showtime.clear();
		strVip.clear();
		timecontrol = true;
	}
};

//玩家的输赢信息---精准控制
struct tagPlayerResultInfo
{
	uint32 win;					//赢的次数
	uint32 lose;				//输的次数
	int64  total_win_coin;		//累计赢取
	int64  day_win_coin;		//当天赢取
	uint64 last_update_time;	//最后更新时间---用于清除当天赢取
	tagPlayerResultInfo()
	{
		win = 0;
		lose = 0;
		total_win_coin = 0;
		day_win_coin = 0;
		last_update_time = 0;
	}
};

//配置信息---精准控制
struct tagUserControlCfg
{
	uint32	suid;			//超权id
	string	sdeviceid;		//当前机器码
	uint32  tuid;			//跟踪id
	set<uint8>  cgid;		//控制游戏id
	string skey;			//授权key
	tagUserControlCfg()
	{
		suid = 0;
		sdeviceid.clear();
		tuid = 0;
		cgid.clear();
		skey.clear();
	}
};

//配置信息---幸运值控制
struct tagUserLuckyCfg
{
	uint32	uid;				//玩家ID
	uint32	gametype;			//游戏ID
	uint32	roomid;				//房间ID
	int32	lucky_value;		//幸运值
	uint32  rate;				//输赢百分比
	int32   accumulated;		//累计幸运值

	tagUserLuckyCfg()
	{
		uid = 0;
		gametype = 0;
		roomid = 0;
		lucky_value = 0;
		rate = 0;
		accumulated = 0;
	}
};

//配置信息---捕鱼---鱼的控制
struct tagFishInfoCfg
{
	uint32	id;					//鱼的种类ID
	uint32	prize_min;			//鱼最低奖励倍数
	uint32	prize_max;			//鱼最高奖励倍数
	uint32	kill_rate;			//鱼击杀概率万分比

	tagFishInfoCfg()
	{
		id = 0;
		prize_min = 0;
		prize_max = 0;
		kill_rate = 0;
	}
};

#endif // DB_STRUCT_DEFINE_H__





