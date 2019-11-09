#ifndef DATA_CFG_MGR_H__
#define DATA_CFG_MGR_H__

#include <string>
#include "json/json.h"
#include "fundamental/noncopyable.h"
#include "svrlib.h"
#include "db_struct_define.h"
#include "dbmysql_mgr.h"
#include <vector>
#include "game_define.h"

using namespace std;
using namespace svrlib;

/*************************************************************/
class CDataCfgMgr : public AutoDeleteSingleton<CDataCfgMgr>
{
private:
    struct stShowHandBaseCfg
    {
        int64   minValue;
        int64   maxValue;
        uint32  paramValue;
    };
public:
    CDataCfgMgr();
    virtual ~CDataCfgMgr();

public:
    bool    Init();
    void    ShutDown();
    bool    Reload();

    int32   GetGiveTax(bool sendVip,bool recvVip);
    int32   GetCLoginScore(uint32 days);
    int32   GetWLoginIngot(uint8 days);
    uint32  GetBankruptCount();
    int32   GetBankruptBase();
    int32   GetBankruptValue();
    int32   GetBankruptType();
    int32   GetSpeakCost();
    int32   GetJumpQueueCost();
    uint32   GetSignGameCount();

    bool    GetExchangeValue(uint8 exchangeType,uint32 id,int64& fromValue,int64& toValue);
    uint32  GetExchangeID(uint8 exchangeType,int64 wantValue);
    bool    GetRobotPayDiamondRmb(int64 wantDiamond,int64 &rmb,int64 &diamond);

    // 获得梭哈底注
    int64   GetShowhandBaseScore(int64 handScore);

    bool    CheckBaseScore(uint8 deal,uint32 score);
    bool    CheckFee(uint8 feeType,uint32 feeValue);
    int64   GetLandPRoomRice(uint32 days);
    int64   CalcEnterMin(int64 baseScore,uint8 deal);

    const stServerCfg& GetCurSvrCfg(){ return m_curSvrCfg; }
    stServerCfg* GetServerCfg(uint32 svrid);
    void    GetLobbySvrsCfg(uint32 group,vector<stServerCfg>& lobbysCfg);
    void    GetDispatchSvrsCfg(uint32 group, std::vector<stServerCfg>& dispatchsCfg);

    const stMissionCfg* GetMissionCfg(uint32 missid);
    const map<uint32,stMissionCfg>& GetAllMissionCfg(){ return m_Missioncfg; }

	//vip广播
	bool   VipBroadCastAnalysis(bool bPhpSend, string & strBroadcast);
	bool	VipAliAccRechargeAnalysis(bool bPhpSend, string & strVipProxyRecharge);

	int64 GetVipBroadcastRecharge() { return m_lVipRecharge; }
	string GetVipBroadcastMsg() { return m_strVipBroadcast; }
	int32 GetVipBroadcastStatus() { return m_iVipStatus; }

	//微信vip充值
	bool   VipProxyRechargeAnalysis(bool bPhpSend, string & strVipProxyRecharge);
	bool    UnionPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    WeChatPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    AliPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    OtherPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    QQPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    WeChatScanPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    JDPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    ApplePayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool    LargeAliPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge);
	bool ExclusiveFlashRechargeAnalysis(bool bPhpSend, string &strjvalueRecharge); // 专享闪付

	int32   GetVipProxyRechargeStatus() { return m_VipProxyRechargeStatus; }
	int64   GetVipProxyRecharge() { return m_lVipProxyRecharge; }
	void GetVipProxyWeChatInfo(map<string, tagVipRechargeWechatInfo> & mpVipProxyWeChatInfo)
	{
		mpVipProxyWeChatInfo.clear();
		for (auto &iter : m_mpVipProxyWeChatInfo)
		{
			mpVipProxyWeChatInfo.insert(make_pair(iter.first, iter.second));
		}
	}

	//支付宝账号
	int32   GetVipAliAccRechargeStatus() { return m_VipAliAccRechargeStatus; }
	int64   GetVipAliAccRecharge() { return m_lVipAliAccRecharge; }
	map<string, tagVipRechargeWechatInfo> & GetVipAliAccInfo() { return m_mpVipProxyAliAccInfo; }


	int32   GetUnionPayRechargeStatus() { return m_UnionPayRechargeStatus; }
	int64   GetUnionPayRecharge() { return m_lUnionPayRecharge; }

	int32   GetWeChatPayRechargeStatus() { return m_WeChatPayRechargeStatus; }
	int64   GetWeChatPayRecharge() { return m_lWeChatPayRecharge; }

	int32   GetAliPayRechargeStatus() { return m_AliPayRechargeStatus; }
	int64   GetAliPayRecharge() { return m_lAliPayRecharge; }

	int32   GetOtherPayRechargeStatus() { return m_OtherPayRechargeStatus; }
	int64   GetOtherPayRecharge() { return m_lOtherPayRecharge; }

	int32   GetQQPayRechargeStatus() { return m_QQPayRechargeStatus; }
	int64   GetQQPayRecharge() { return m_lQQPayRecharge; }

	int32   GetWeChatScanPayRechargeStatus() { return m_WeChatScanPayRechargeStatus; }
	int64   GetWeChatScanPayRecharge() { return m_lWeChatScanPayRecharge; }

	int32   GetJDPayRechargeStatus() { return m_JDPayRechargeStatus; }
	int64   GetJDPayRecharge() { return m_lJDPayRecharge; }

	int32   GetApplePayRechargeStatus() { return m_ApplePayRechargeStatus; }
	int64   GetApplePayRecharge() { return m_lApplePayRecharge; }

	int32   GetLargeAliPayRechargeStatus() { return m_LargeAliPayRechargeStatus; }
	int64   GetLargeAliPayRecharge() { return m_lLargeAliPayRecharge; }

	// 专享闪付
	int32 GetExclusiveFlashRechargeStatus() { return m_exclusiveFlashRechargeStatus; }
	int64 GetExclusiveFlashRecharge() { return m_lExclusiveFlashRecharge; }

	//判断当前玩家是否对游戏可控
	bool	GetUserControlFlag(uint32 uid, uint32 game_id, string device_id, string check_code);

	//获取精准控制配置信息
	void    GetUserControlCfg(map<uint32, tagUserControlCfg> &mpInfo);

	//更新精准控制配置信息
	bool	UpdateUserControlInfo(uint32 uid, uint32 oper_type, tagUserControlCfg info);

	// 用户是否为被跟踪用户
	bool	GetIsUserControl(uint32 uid, uint32 gameType, uint32 & suid);

	void    InitMasterShowInfo();

	string  GetMasterRandCity();

	uint32  GetMasterRandUid();

	map<uint32, vector<stRobotOnlineCfg>> & GetRobotOnlineCfg() { return m_mpRobotOnlineCfg; }

	void UpdateRobotOnlineCfg(stRobotOnlineCfg & tagCfg);

	bool DeleteRobotOnlineCfg(int gametype, int batchid);

	//幸运值
	void	LoadLuckyConfig(uint32 uid, uint32 gameType, map<uint8, tagUserLuckyCfg> &mpLuckyCfg);
	void	UpdateLuckyInfo(uint32 uid, uint32 gameType, uint32 roomid, tagUserLuckyCfg mpLuckyInfo);

protected:
    bool   InitSysConf();

	// 读取开关等
	bool StatusRechargeAnalysis(Json::Value &jValueData, int32 &rechargeStatus, int64 &recharge);



protected:
    typedef map<uint32,stMissionCfg> MAP_MISSCFG;
    typedef map<string,string>       MAP_SYSCFG;
    typedef map<uint32,stServerCfg>  MAP_SVRCFG;

    MAP_MISSCFG  m_Missioncfg;
    MAP_SYSCFG   m_mpSysCfg;
    MAP_SVRCFG   m_mpSvrCfg;
    stServerCfg  m_curSvrCfg;

    int32          m_givetax[2];       // 赠送税收
    vector<int32>  m_cloginrewards;    // 连续登陆奖励积分
    vector<int32>  m_wloginrewards;    // 周累计登陆奖励
    int32          m_bankrupt[4];      // 破产补助
    int32          m_proomprice;       // 私人房日租金
    int32          m_speakCost;        // 喇叭收费
    int32          m_jumpQueueCost;    // 插队收费
    int32          m_signGameCount;    // 签到游戏局数
    
    
    map<uint32,vector<uint32> > m_mplandbasescores; // 斗地主私人房底分配置
    map<uint32,vector<uint32> > m_mplandentermin;   // 斗地主私人房进入配置
    map<uint32,vector<uint32> > m_mplandfees;       // 斗地主台费配置
    map<uint32,uint32>          m_mplandenterparam; // 最小进入系数

    map<uint32,stExchangeCfg>   m_mpExchangeDiamond;// 钻石兑换
    map<uint32,stExchangeCfg>   m_mpExchangeCoin;   // 财富币兑换
    map<uint32,stExchangeCfg>   m_mpExchangeScore;  // 积分兑换

    vector<stShowHandBaseCfg>   m_vecShowhandBaseCfg;       // 梭哈底注计算配置
    vector<int64>               m_vecShowhandBaseScoreCfg;  // 梭哈私人房底分配置
    vector<int64>               m_vecShowhandMinBuyinCfg;   // 梭哈私人房最小带入配置

    vector<int64>               m_vecTexasBaseScoreCfg;     // 德州底注计算配置
    vector<int64>               m_vecTexasMinBuyinCfg;      // 德州最小带入配置

	string						m_strVipBroadcast;			// vip广播消息
	int64						m_lVipRecharge;				// vip广播金额
	int32						m_iVipStatus;				// vip广播状态 0 开启 1 关闭

	int32						m_VipProxyRechargeStatus;	// 0 开启 1 关闭
	int64						m_lVipProxyRecharge;		// vip金额
	map<string, tagVipRechargeWechatInfo>          m_mpVipProxyWeChatInfo;		// vip微信信息

	int32						m_VipAliAccRechargeStatus;	// 0 开启 1 关闭
	int64						m_lVipAliAccRecharge;		// vip金额
	map<string, tagVipRechargeWechatInfo>          m_mpVipProxyAliAccInfo;		// vip支付宝信息


	int32						m_UnionPayRechargeStatus;	// 0 开启 1 关闭
	int64						m_lUnionPayRecharge;		// 银联金额

	int32						m_WeChatPayRechargeStatus;
	int64						m_lWeChatPayRecharge;

	int32						m_AliPayRechargeStatus;
	int64						m_lAliPayRecharge;

	int32						m_OtherPayRechargeStatus;
	int64						m_lOtherPayRecharge;

	int32						m_QQPayRechargeStatus;
	int64						m_lQQPayRecharge;

	int32						m_WeChatScanPayRechargeStatus;
	int64						m_lWeChatScanPayRecharge;

	int32						m_JDPayRechargeStatus;
	int64						m_lJDPayRecharge;

	int32						m_ApplePayRechargeStatus;
	int64						m_lApplePayRecharge;

	int32						m_LargeAliPayRechargeStatus;
	int64						m_lLargeAliPayRecharge;

	int32				 m_exclusiveFlashRechargeStatus;        // 0 开启  1 关闭
	int64				 m_lExclusiveFlashRecharge;

	// 精准控制---配置信息
	map<uint32, tagUserControlCfg>		m_mpUserControlCfg;

	vector<string>				m_vecMasterCity;
	vector<uint32>				m_vecMasterUid;

	map<uint32, vector<stRobotOnlineCfg>>    m_mpRobotOnlineCfg; // 机器人在线配置
};




#endif //
