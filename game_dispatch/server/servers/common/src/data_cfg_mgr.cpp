#include <pb/error_code.pb.h>
#include "data_cfg_mgr.h"
#include "player_mgr.h"
#include "common_logic.h"
#include "crypt/md5.h"
#include "json/json.h"
#include "db_struct_define.h"
#include "msg_define.pb.h"
#include "player_base.h"
#include "master_city.h"
#include <random>
using namespace std;
using namespace svrlib;


CDataCfgMgr::CDataCfgMgr()
{

}
CDataCfgMgr::~CDataCfgMgr()
{
    
}
bool CDataCfgMgr::Init()
{
    if(!Reload())return false;

    stServerCfg* pSvrCfg = GetServerCfg(CApplication::Instance().GetServerID());
    if(pSvrCfg == NULL){
        LOG_DEBUG("服务器配置信息未找到 svrid:%d",CApplication::Instance().GetServerID());
        return false;
    }
    m_curSvrCfg = *pSvrCfg;
	InitMasterShowInfo();
    return true;
}
void CDataCfgMgr::ShutDown()
{

}
bool CDataCfgMgr::Reload()
{
    CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadMissionCfg(m_Missioncfg);
    CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadSysCfg(m_mpSysCfg);
    CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_ACC).LoadSvrCfg(m_mpSvrCfg);
    //CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadExchangeDiamondCfg(m_mpExchangeDiamond);
    CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadExchangeCoinCfg(m_mpExchangeCoin);
    //CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadExchangeScoreCfg(m_mpExchangeScore);
	CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadVipProxyRecharge(m_mpVipProxyWeChatInfo);
	CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadVipAliAccRecharge(m_mpVipProxyAliAccInfo);
	CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadUserControlCfg(m_mpUserControlCfg);
	CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadRobotOnlineCfg(m_mpRobotOnlineCfg);

    return InitSysConf();
}
int32  CDataCfgMgr::GetGiveTax(bool sendVip,bool recvVip)
{
    if(sendVip && recvVip)
        return 0;
    if(sendVip){
        return m_givetax[1];
    }
    return m_givetax[0];
}
int32  CDataCfgMgr::GetCLoginScore(uint32 days)
{
	if (days-1 < m_cloginrewards.size()) {
		return m_cloginrewards[days-1];
	}
    return m_cloginrewards[m_cloginrewards.size()-1];
}
int32  CDataCfgMgr::GetWLoginIngot(uint8 days)
{
    if(days < m_wloginrewards.size()){
        return m_wloginrewards[days];
    }
    return 0;
}
uint32  CDataCfgMgr::GetBankruptCount()
{
    return m_bankrupt[0];
}
int32  CDataCfgMgr::GetBankruptBase()
{
    return m_bankrupt[1];
}
int32  CDataCfgMgr::GetBankruptValue()
{
    return m_bankrupt[2];
}
int32  CDataCfgMgr::GetBankruptType()
{
    return m_bankrupt[3];
}    
int32  CDataCfgMgr::GetSpeakCost()
{
    return m_speakCost;
}
int32  CDataCfgMgr::GetJumpQueueCost()
{
    return m_jumpQueueCost;
}    
uint32  CDataCfgMgr::GetSignGameCount()
{
    return m_signGameCount;
}
bool    CDataCfgMgr::GetExchangeValue(uint8 exchangeType,uint32 id,int64& fromValue,int64& toValue)
{
    map<uint32,stExchangeCfg>::iterator it;
    if(exchangeType == net::EXCHANGE_TYPE_COIN){
        it = m_mpExchangeCoin.find(id);
        if(it != m_mpExchangeCoin.end()){
            fromValue = it->second.fromValue;
            toValue   = it->second.toValue;
            return true;
        }
        LOG_ERROR("兑换ID未找到:%d",id);
        return false;
    }
    if(exchangeType == net::EXCHANGE_TYPE_SCORE)
    {
        it = m_mpExchangeScore.find(id);
        if(it != m_mpExchangeScore.end()){
            fromValue = it->second.fromValue;
            toValue   = it->second.toValue;
            return true;
        }
        LOG_ERROR("兑换ID未找到:%d",id);
        return false;
    }
    return false;
}
uint32  CDataCfgMgr::GetExchangeID(uint8 exchangeType,int64 wantValue)
{
    map<uint32,stExchangeCfg>::iterator it;
    uint32 curid    = 0;
    uint32 curValue = 0;
    if(exchangeType == net::EXCHANGE_TYPE_COIN){
        it = m_mpExchangeCoin.begin();
        curid = it->first;
        curValue = it->second.toValue;
        for(;it != m_mpExchangeCoin.end();++it)
        {
            if(it->second.toValue <= wantValue)
            {
                if(it->second.toValue > curValue) {
                    curid = it->first;
                    curValue = it->second.toValue;
                }
            }else{
                if(it->second.toValue < curValue) {
                    curid = it->first;
                    curValue = it->second.toValue;
                }
            }
        }
    }
    if(exchangeType == net::EXCHANGE_TYPE_SCORE)
    {
        it = m_mpExchangeScore.begin();
        curid = it->first;
        curValue = it->second.toValue;
        for(;it != m_mpExchangeScore.end();++it)
        {
            if(it->second.toValue <= wantValue)
            {
                if(it->second.toValue > curValue) {
                    curid = it->first;
                    curValue = it->second.toValue;
                }
            }else{
                if(it->second.toValue < curValue) {
                    curid = it->first;
                    curValue = it->second.toValue;
                }
            }
        }
    }
    return curid;
}
bool    CDataCfgMgr::GetRobotPayDiamondRmb(int64 wantDiamond,int64 &rmb,int64 &diamond)
{
    rmb     = 0;
    diamond = 0;
    if(m_mpExchangeDiamond.empty())
        return false;

    map<uint32,stExchangeCfg>::iterator it = m_mpExchangeDiamond.begin();
    uint32 curid      = it->first;
    int64  currmb     = it->second.fromValue;
    int64  curdiamond = it->second.toValue;

    for(;it != m_mpExchangeDiamond.end();++it)
    {
        if(it->second.toValue > wantDiamond){
            if(it->second.toValue < curdiamond) {
                curid = it->first;
                currmb = it->second.fromValue;
                curdiamond = it->second.toValue;
            }
        }else{
            if(it->second.toValue > curdiamond){
                curid   = it->first;
                currmb  = it->second.fromValue;
                curdiamond = it->second.toValue;
            }
        }
    }
    uint32 num = (wantDiamond/curdiamond) + 1;
    rmb     = currmb*num;
    diamond = curdiamond*num;
    return true;
}
// 获得梭哈底注
int64   CDataCfgMgr::GetShowhandBaseScore(int64 handScore)
{
    for(uint32 i=0;i<m_vecShowhandBaseCfg.size();++i)
    {
        stShowHandBaseCfg& cfg = m_vecShowhandBaseCfg[i];
        if(handScore < cfg.maxValue || (i == m_vecShowhandBaseCfg.size()-1))
        {
            if(i < 2){
                return cfg.paramValue;
            }else{
                return handScore * cfg.paramValue/PRO_DENO_100w;
            }
        }
    }
    return 100;
}
bool    CDataCfgMgr::CheckBaseScore(uint8 deal,uint32 score)
{
    if(m_curSvrCfg.gameType == net::GAME_CATE_LAND){
        map<uint32,vector<uint32> >::iterator it = m_mplandbasescores.find(deal);
        if(it == m_mplandbasescores.end()){
            return false;
        }
        for(uint32 i=0;i<it->second.size();++i){
            if(score == it->second[i]){
                return true;
            }
        }
    }else if(m_curSvrCfg.gameType == net::GAME_CATE_SHOWHAND){
        for(uint32 i=0;i<m_vecShowhandBaseScoreCfg.size();++i){
            if(score == m_vecShowhandBaseScoreCfg[i]) {
                return true;
            }
        }
    }else if(m_curSvrCfg.gameType == net::GAME_CATE_TEXAS){
        for(uint32 i=0;i<m_vecTexasBaseScoreCfg.size();++i){
            if(score == m_vecTexasBaseScoreCfg[i]) {
                return true;
            }
        }
    }

    return false;
}
bool    CDataCfgMgr::CheckFee(uint8 feeType,uint32 feeValue)
{
    map<uint32,vector<uint32> >::iterator it = m_mplandfees.find(feeType);
    if(it == m_mplandfees.end()){
        return false;
    }
    for(uint32 i=0;i<it->second.size();++i){
        if(feeValue == it->second[i]){
            return true;
        }
    }
    return false;
}
int64   CDataCfgMgr::GetLandPRoomRice(uint32 days)
{
    return m_proomprice * days;
}
int64   CDataCfgMgr::CalcEnterMin(int64 baseScore,uint8 deal)
{
    if(m_curSvrCfg.gameType == net::GAME_CATE_LAND)
    {
        map<uint32,vector<uint32> >::iterator it = m_mplandbasescores.find(deal);
        if(it != m_mplandbasescores.end())
        {
            for(uint32 i=0;i<it->second.size();++i){
                if(it->second[i] == baseScore){
                    return m_mplandentermin[deal][i];
                }
            }
        }
        return baseScore * 10;
    }else if(m_curSvrCfg.gameType == net::GAME_CATE_SHOWHAND){
        for(uint32 i=0;i<m_vecShowhandBaseScoreCfg.size();++i){
            if(m_vecShowhandBaseScoreCfg[i] == baseScore){
                return m_vecShowhandMinBuyinCfg[i];
            }
        }
        return baseScore * 50;
    }else if(m_curSvrCfg.gameType == net::GAME_CATE_TEXAS){
        for(uint32 i=0;i<m_vecTexasBaseScoreCfg.size();++i){
            if(m_vecTexasBaseScoreCfg[i] == baseScore){
                return m_vecTexasMinBuyinCfg[i];
            }
        }
        return baseScore * 50;
    }
    return baseScore*10;
}
stServerCfg* CDataCfgMgr::GetServerCfg(uint32 svrid)
{
    MAP_SVRCFG::iterator it = m_mpSvrCfg.find(svrid);
    if (it != m_mpSvrCfg.end() )
    {
        return &it->second;
    }
    return NULL;
}
void    CDataCfgMgr::GetLobbySvrsCfg(uint32 group,vector<stServerCfg>& lobbysCfg)
{
    MAP_SVRCFG::iterator it = m_mpSvrCfg.begin();
    for(;it != m_mpSvrCfg.end();++it)
    {
        stServerCfg& cfg = it->second;
        if(cfg.group == group && cfg.svrType == 1)// 同组大厅服务器
        {
            lobbysCfg.push_back(cfg);
        }
    }
}
const stMissionCfg* CDataCfgMgr::GetMissionCfg(uint32 missid)
{
    MAP_MISSCFG::iterator it = m_Missioncfg.find(missid);
    if(it != m_Missioncfg.end()){
        return &it->second;
    }

    return NULL;
}

bool   CDataCfgMgr::InitSysConf()
{
    // 税率
    memset(m_givetax,0,sizeof(m_givetax));
    m_givetax[0] = atoi(m_mpSysCfg["givetax"].c_str());
    m_givetax[1] = atoi(m_mpSysCfg["giveviptax"].c_str());

    // 连续登陆
    m_cloginrewards.clear();
    string cloginStr = m_mpSysCfg["cloginreward"];
    Json::Reader reader;
    Json::Value  jvalue;
    if(!reader.parse(cloginStr,jvalue))
    {
        LOG_ERROR("解析连续登陆json错误:%s",cloginStr.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        m_cloginrewards.push_back(jvalue[i].asUInt());
    }
    // 周累计登陆
    m_wloginrewards.clear();
    string wloginStr = m_mpSysCfg["wloginreward"];
    if(!reader.parse(wloginStr,jvalue))
    {
        LOG_ERROR("解析周累计登陆json错误:%s",wloginStr.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        m_wloginrewards.push_back(jvalue[i].asUInt());
    }
    // 破产补助
    memset(m_bankrupt,0,sizeof(m_bankrupt));
    m_bankrupt[0] = atoi(m_mpSysCfg["bankruptcount"].c_str());
    m_bankrupt[1] = atoi(m_mpSysCfg["bankruptbase"].c_str());
    m_bankrupt[2] = atoi(m_mpSysCfg["bankruptvalue"].c_str());
    m_bankrupt[3] = atoi(m_mpSysCfg["bankrupttype"].c_str());
    
        
    // 私人房
    m_proomprice = atoi(m_mpSysCfg["proomprice"].c_str());
    // 斗地主私人房底分配置
    m_mplandbasescores.clear();
    string landbaseStr = m_mpSysCfg["landbasescore"];
    if(!reader.parse(landbaseStr,jvalue)){
        LOG_ERROR("解析斗地主私人房底分配置错误:%s",landbaseStr.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        Json::Value v = jvalue[i];
        uint32 deal = v["deal"].asUInt();
        vector<uint32> scores;
        Json::Value sc = v["score"];
        for(uint32 j=0;j<sc.size();++j){
            scores.push_back(sc[j].asUInt());
        }
        m_mplandbasescores[deal] = scores;
        scores.clear();
        Json::Value enter = v["entermin"];
        for(uint32 j=0;j<enter.size();++j){
            scores.push_back(enter[j].asUInt());
        }
        m_mplandentermin[deal] = scores;
    }
    // 斗地主台费配置
    m_mplandfees.clear();
    string landfeeStr = m_mpSysCfg["proomfee"];
    if(!reader.parse(landfeeStr,jvalue)){
        LOG_ERROR("解析台费配置错误:%s",landfeeStr.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        uint32 feeType  = jvalue[i]["feetype"].asUInt();
        uint32 feeValue = jvalue[i]["fee"].asUInt();
        if(m_mplandfees.find(feeType) != m_mplandfees.end()){
            m_mplandfees[feeType].push_back(feeValue);
        }else{
            vector<uint32> vec1;
            vec1.push_back(feeValue);
            m_mplandfees.insert(make_pair(feeType,vec1));
        }

    }
    // 最小进入系数
    m_mplandenterparam.clear();
    string landEnterStr = m_mpSysCfg["landenterparam"];
    if(!reader.parse(landEnterStr,jvalue)){
        LOG_ERROR("解析最小进入系数错误:%s",landEnterStr.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        uint32 deal  = jvalue[i]["deal"].asUInt();
        uint32 param = jvalue[i]["param"].asUInt();
        m_mplandenterparam.insert(make_pair(deal,param));
    }
    // 梭哈底注配置
    m_vecShowhandBaseCfg.clear();
    m_vecShowhandBaseCfg.resize(3);
    string showhandbaseStr = m_mpSysCfg["showhandbasescore"];
    if(!reader.parse(showhandbaseStr,jvalue)){
        LOG_ERROR("解析梭哈底注配置系数错误:%s",showhandbaseStr.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        stShowHandBaseCfg cfg;
        cfg.minValue    = jvalue[i]["min"].asInt64();
        cfg.maxValue    = jvalue[i]["max"].asInt64();
        cfg.paramValue  = jvalue[i]["score"].asUInt();
        int32 pos = jvalue[i]["pos"].asUInt();
        m_vecShowhandBaseCfg[pos] = cfg;
    }
    // 梭哈私人房底分，带入配置
    m_vecShowhandBaseScoreCfg.clear();
    m_vecShowhandMinBuyinCfg.clear();
    string showhandbasescore = m_mpSysCfg["shbasescore"];
    if(!reader.parse(showhandbasescore,jvalue)){
        LOG_ERROR("解析梭哈私人房底注配置:%s",showhandbasescore.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        m_vecShowhandBaseScoreCfg.push_back(jvalue[i].asUInt());
    }
    string showhandminbuyin = m_mpSysCfg["shminbuyin"];
    if(!reader.parse(showhandminbuyin,jvalue)){
        LOG_ERROR("解析梭哈私人房带入配置:%s",showhandminbuyin.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        m_vecShowhandMinBuyinCfg.push_back(jvalue[i].asUInt());
    }
    // 喇叭收费
    m_speakCost = atoi(m_mpSysCfg["speakcost"].c_str());
    // 插队收费
    m_jumpQueueCost = atoi(m_mpSysCfg["jumpqueue"].c_str());
    // 签到局数
    m_signGameCount = atoi(m_mpSysCfg["signgamecount"].c_str());
    
    // 德州底分配置
    m_vecTexasBaseScoreCfg.clear();
    string texasbasescore = m_mpSysCfg["texasbasescore"];
    if(!reader.parse(texasbasescore,jvalue)){
        LOG_ERROR("解析德州私人房底注配置:%s",texasbasescore.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        m_vecTexasBaseScoreCfg.push_back(jvalue[i].asUInt());
    }
    m_vecTexasMinBuyinCfg.clear();
    string texasminbuyin = m_mpSysCfg["texasminbuyin"];
    if(!reader.parse(texasminbuyin,jvalue)){
        LOG_ERROR("解析德州私人房带入配置:%s",texasminbuyin.c_str());
        return false;
    }
    for(uint32 i=0;i<jvalue.size();++i){
        m_vecTexasMinBuyinCfg.push_back(jvalue[i].asUInt());
    }

	map<string, string>::iterator iter_viphorn = m_mpSysCfg.find("viphorn");
	if (iter_viphorn != m_mpSysCfg.end())
	{
		string strBroadcast = iter_viphorn->second;
		VipBroadCastAnalysis(false,strBroadcast);
	}
	auto iter_vipproxyrecharge = m_mpSysCfg.find("vipproxyrecharge");
	if (iter_vipproxyrecharge != m_mpSysCfg.end())
	{
		string str_vipproxyrecharge = iter_vipproxyrecharge->second;
		VipProxyRechargeAnalysis(false, str_vipproxyrecharge);
	}
	auto iter_vipalipayrecharge = m_mpSysCfg.find("vipalipayrecharge");
	if (iter_vipalipayrecharge != m_mpSysCfg.end())
	{
		string str_vipproxyrecharge = iter_vipalipayrecharge->second;
		VipAliAccRechargeAnalysis(false, str_vipproxyrecharge);
	}
	auto iter_union_pay = m_mpSysCfg.find("unionpayrecharge");
	if (iter_union_pay != m_mpSysCfg.end())
	{
		string str_iter_pay = iter_union_pay->second;
		UnionPayRechargeAnalysis(false, str_iter_pay);
	}

	auto iter_wechat_pay = m_mpSysCfg.find("wechatpayrecharge");
	if (iter_wechat_pay != m_mpSysCfg.end())
	{
		string str_iter_pay = iter_wechat_pay->second;
		WeChatPayRechargeAnalysis(false, str_iter_pay);
	}

	auto iter_ali_pay = m_mpSysCfg.find("alipayrecharge");
	if (iter_ali_pay != m_mpSysCfg.end())
	{
		string str_iter_pay = iter_ali_pay->second;
		AliPayRechargeAnalysis(false, str_iter_pay);
	}
	auto iter_other_pay = m_mpSysCfg.find("otherpayrecharge");
	if (iter_other_pay != m_mpSysCfg.end())
	{
		string str_iter_pay = iter_other_pay->second;
		OtherPayRechargeAnalysis(false, str_iter_pay);
	}
	auto iter_qq_pay = m_mpSysCfg.find("qqpayrecharge");
	if (iter_qq_pay != m_mpSysCfg.end())
	{
		string str_iter_pay = iter_qq_pay->second;
		QQPayRechargeAnalysis(false, str_iter_pay);
	}
	auto iter_wechat_scan_pay = m_mpSysCfg.find("wcscanpayrecharge");
	if (iter_wechat_scan_pay != m_mpSysCfg.end())
	{
		string str_iter_pay = iter_wechat_scan_pay->second;
		WeChatScanPayRechargeAnalysis(false, str_iter_pay);
	}
	auto iter_jd_pay = m_mpSysCfg.find("jdpayrecharge");
	if (iter_jd_pay != m_mpSysCfg.end())
	{
		string str_iter_pay = iter_jd_pay->second;
		JDPayRechargeAnalysis(false, str_iter_pay);
	}
	auto iter_apple_pay = m_mpSysCfg.find("applepayrecharge");
	if (iter_apple_pay != m_mpSysCfg.end())
	{
		string str_iter_pay = iter_apple_pay->second;
		ApplePayRechargeAnalysis(false, str_iter_pay);
	}
	auto iter_large_ali_pay = m_mpSysCfg.find("bigalipayrecharge");
	if (iter_large_ali_pay != m_mpSysCfg.end())
	{
		string str_iter_pay = iter_large_ali_pay->second;
		LargeAliPayRechargeAnalysis(false, str_iter_pay);
	}
	map<string, string>::iterator iter_exclusiveFlashRecharge = m_mpSysCfg.find("exclusiveshanpay");
	if (iter_exclusiveFlashRecharge != m_mpSysCfg.end())
		ExclusiveFlashRechargeAnalysis(false, iter_exclusiveFlashRecharge->second);
    return true;
}

// 读取开关等
bool CDataCfgMgr::StatusRechargeAnalysis(Json::Value &jValueData, int32 &rechargeStatus, int64 &recharge) {
	bool bRet = true;
	if (jValueData.isMember("status") && jValueData["status"].isIntegral())
		rechargeStatus = jValueData["status"].asInt();
	else {
		LOG_ERROR("status err");
		bRet = false;
	}
	if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
		recharge = jValueData["recharge"].asInt64();
	else {
		LOG_ERROR("recharge err");
		bRet = false;
	}
	LOG_DEBUG("rechargeStatus:%d,recharge:%lld,bRet:%d",
		rechargeStatus, recharge, bRet);
	return bRet;
}

void   CDataCfgMgr::GetDispatchSvrsCfg(uint32 group, std::vector<stServerCfg>& dispatchsCfg)
{
    MAP_SVRCFG::iterator it = m_mpSvrCfg.begin();
    for(; it != m_mpSvrCfg.end(); ++it)
    {
        stServerCfg& cfg = it->second;
        if(cfg.group == group && cfg.svrType == 3)// 同组Dispatch服务器
        {
            dispatchsCfg.push_back(cfg);
        }
    }
}

bool   CDataCfgMgr::VipBroadCastAnalysis(bool bPhpSend, string & strBroadcast)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strBroadcast.empty() || !reader.parse(strBroadcast, jvalue))
	{
		LOG_ERROR("解析vip广播系数配置系数错误:%s", strBroadcast.c_str());
		return false;
	}
	bool bIsHaveRecharge = false;
	if (jvalue.isMember("recharge") && jvalue["recharge"].isIntegral())
	{
		bIsHaveRecharge = true;
		m_lVipRecharge = jvalue["recharge"].asInt64();
	}
	if (bIsHaveRecharge && jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jBroadcast = jvalue["data"];
		if (jBroadcast.isMember("status") && jBroadcast["status"].isIntegral())
		{
			m_iVipStatus = jvalue["status"].asInt();
			m_strVipBroadcast = jBroadcast.toFastString();
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendVipBroadCast();
	}

	LOG_ERROR("m_iVipStatus:%d,m_lVipRecharge:%lld,m_strVipBroadcast:%s,strBroadcast:%s", m_iVipStatus,m_lVipRecharge,m_strVipBroadcast.c_str(), strBroadcast.c_str());
	return true;
}



bool   CDataCfgMgr::VipProxyRechargeAnalysis(bool bPhpSend, string & strVipProxyRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strVipProxyRecharge.empty() || !reader.parse(strVipProxyRecharge, jvalue))
	{
		LOG_ERROR("解析vip代理充值配置系数错误:%s", strVipProxyRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis VipProxyRecharge - strVipProxyRecharge:%s", strVipProxyRecharge.c_str());

	//{"data":{"action":"vip_proxy","recharge":10000,"status":0}}
	//{"data":{"action":"vip_wechat", "wcinfo" : [{"sort_id":1, "wx_account" : "weixin001", "wx_title" : "测试标题001"}]}}
	bool bIsWeChatInfo = true;
	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "vip_proxy")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lVipProxyRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_VipProxyRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
			if (straction == "vip_wechat")
			{
				map<string, tagVipRechargeWechatInfo> mpVipProxyWeChatInfo;
				Json::Value jValueInfoWeChat = jValueData["wcinfo"];

				LOG_DEBUG("straction:%s,jValueInfoWeChat.size:%d", straction.c_str(), jValueInfoWeChat.size());

				for (uint32 i = 0; i<jValueInfoWeChat.size(); ++i)
				{
					tagVipRechargeWechatInfo tagInfo;
					Json::Value jValueInfoDetails = jValueInfoWeChat[i];
					if (jValueInfoDetails.isMember("sort_id") && jValueInfoDetails["sort_id"].isIntegral())
					{
						tagInfo.sortid = jValueInfoDetails["sort_id"].asUInt();
					}
					else
					{
						bIsWeChatInfo = false;
						LOG_DEBUG("sordid err - straction:%s,i:%d", straction.c_str(), i);
					}
					if (jValueInfoDetails.isMember("wx_account") && jValueInfoDetails["wx_account"].isString())
					{
						tagInfo.account = jValueInfoDetails["wx_account"].asString();
					}
					else
					{
						bIsWeChatInfo = false;
						LOG_DEBUG("account err - straction:%s,i:%d", straction.c_str(), i);
					}
					if (jValueInfoDetails.isMember("wx_title") && jValueInfoDetails["wx_title"].isString())
					{
						tagInfo.title = jValueInfoDetails["wx_title"].asString();
					}
					else
					{
						LOG_DEBUG("title err - straction:%s,i:%d", straction.c_str(), i);
						bIsWeChatInfo = false;
					}
					bool timecontrol = true;
					if (jValueInfoDetails.isMember("showtime") && jValueInfoDetails["showtime"].isString())
					{
						string strshowtime = jValueInfoDetails["showtime"].asString();
						tagInfo.showtime = strshowtime;
						Json::Value  jshowtime;
						if (!strshowtime.empty() && reader.parse(strshowtime, jshowtime))
						{
							if (jshowtime.isMember("sh") && jshowtime["sh"].isIntegral())
							{
								tagInfo.shour = jshowtime["sh"].asInt();
							}
							else
							{
								LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
								timecontrol = false;
							}
							if (jshowtime.isMember("sm") && jshowtime["sm"].isIntegral())
							{
								tagInfo.sminute = jshowtime["sm"].asInt();
							}
							else
							{
								LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
								timecontrol = false;
							}
							if (jshowtime.isMember("eh") && jshowtime["eh"].isIntegral())
							{
								tagInfo.ehour = jshowtime["eh"].asInt();
							}
							else
							{
								LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
								timecontrol = false;
							}
							if (jshowtime.isMember("em") && jshowtime["em"].isIntegral())
							{
								tagInfo.eminute = jshowtime["em"].asInt();
							}
							else
							{
								LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
								timecontrol = false;
							}
						}
						else
						{
							timecontrol = false;
							LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
						}
					}
					else
					{
						LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());

						timecontrol = false;
					}
					tagInfo.timecontrol = timecontrol;

					LOG_ERROR("解析微信充值显示 - account:%s,bIsWeChatInfo:%d,shour:%d, sminute:%d, ehour:%d, eminute:%d,timecontrol:%d,showtime:%s", tagInfo.account.c_str(), bIsWeChatInfo, tagInfo.shour, tagInfo.sminute, tagInfo.ehour, tagInfo.eminute, tagInfo.timecontrol, tagInfo.showtime.c_str());

					if (jValueInfoDetails.isMember("vip") && jValueInfoDetails["vip"].isString())
					{
						string strVip = jValueInfoDetails["vip"].asString();
						tagInfo.strVip = strVip;
						Json::Value  jvip;

						if (!tagInfo.strVip.empty() && reader.parse(tagInfo.strVip, jvip))
						{
							if (jvip.isArray())
							{
								for (uint32 i = 0; i < jvip.size(); i++)
								{
									LOG_DEBUG("解析微信充值 - strVip:%s,isArray:%d,isIntegral:%d", tagInfo.strVip.c_str(), jvip.isArray(), jvip[i].isIntegral());
									if (jvip[i].isIntegral())
									{
										LOG_DEBUG("解析微信充值 - strVip:%s,jvip:%d", tagInfo.strVip.c_str(), jvip[i].asInt());
										tagInfo.vecVip.push_back(jvip[i].asInt());
									}
								}
							}
							else
							{
								LOG_ERROR("解析微信充值错误 - strVip:%s,jvip.isArray():%d", tagInfo.strVip.c_str(), jvip.isArray());
							}
						}
						else
						{
							LOG_ERROR("解析微信充值错误 - strVip:%s", tagInfo.strVip.c_str());
						}
					}
					else
					{
						LOG_DEBUG("vip err - account:%s", tagInfo.account.c_str());
					}

					if (bIsWeChatInfo)
					{
						mpVipProxyWeChatInfo.insert(make_pair(tagInfo.account, tagInfo));
					}
				}

				if (bIsWeChatInfo)
				{
					m_mpVipProxyWeChatInfo.clear();
					for (auto &iter : mpVipProxyWeChatInfo)
					{
						m_mpVipProxyWeChatInfo.insert(make_pair(iter.first, iter.second));
					}
				}

			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendVipProxyRecharge();
	}

	LOG_DEBUG("bPhpSend:%d,bIsWeChatInfo:%d,m_VipProxyRechargeStatus:%d,m_lVipProxyRecharge:%lld,m_mpVipProxyWeChatInfo:%d,strVipProxyRecharge:%s", bPhpSend, bIsWeChatInfo, m_VipProxyRechargeStatus, m_lVipProxyRecharge, m_mpVipProxyWeChatInfo.size(), strVipProxyRecharge.c_str());
	return true;
}

bool   CDataCfgMgr::VipAliAccRechargeAnalysis(bool bPhpSend, string & strVipProxyRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strVipProxyRecharge.empty() || !reader.parse(strVipProxyRecharge, jvalue))
	{
		LOG_ERROR("解析vip代理充值配置系数错误:%s", strVipProxyRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis VipProxyRecharge - strVipProxyRecharge:%s", strVipProxyRecharge.c_str());


	bool bIsWeChatInfo = true;
	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "vipalipay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lVipAliAccRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_VipAliAccRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
			if (straction == "vip_aliacc")
			{
				map<string, tagVipRechargeWechatInfo> mpVipProxyWeChatInfo;
				Json::Value jValueInfoWeChat = jValueData["wcinfo"];

				LOG_DEBUG("straction:%s,jValueInfoWeChat.size:%d", straction.c_str(), jValueInfoWeChat.size());

				for (uint32 i = 0; i<jValueInfoWeChat.size(); ++i)
				{
					tagVipRechargeWechatInfo tagInfo;
					Json::Value jValueInfoDetails = jValueInfoWeChat[i];
					if (jValueInfoDetails.isMember("sort_id") && jValueInfoDetails["sort_id"].isIntegral())
					{
						tagInfo.sortid = jValueInfoDetails["sort_id"].asUInt();
					}
					else
					{
						bIsWeChatInfo = false;
						LOG_DEBUG("sordid err - straction:%s,i:%d", straction.c_str(), i);
					}
					if (jValueInfoDetails.isMember("low_amount") && jValueInfoDetails["low_amount"].isIntegral())
					{
						tagInfo.low_amount = jValueInfoDetails["low_amount"].asUInt();
					}
					else
					{
						bIsWeChatInfo = false;
						LOG_DEBUG("low_amount err - straction:%s,i:%d", straction.c_str(), i);
					}
					if (jValueInfoDetails.isMember("wx_account") && jValueInfoDetails["wx_account"].isString())
					{
						tagInfo.account = jValueInfoDetails["wx_account"].asString();
					}
					else
					{
						bIsWeChatInfo = false;
						LOG_DEBUG("account err - straction:%s,i:%d", straction.c_str(), i);
					}
					if (jValueInfoDetails.isMember("wx_title") && jValueInfoDetails["wx_title"].isString())
					{
						tagInfo.title = jValueInfoDetails["wx_title"].asString();
					}
					else
					{
						LOG_DEBUG("title err - straction:%s,i:%d", straction.c_str(), i);
						bIsWeChatInfo = false;
					}
					bool timecontrol = true;
					if (jValueInfoDetails.isMember("showtime") && jValueInfoDetails["showtime"].isString())
					{
						string strshowtime = jValueInfoDetails["showtime"].asString();
						tagInfo.showtime = strshowtime;
						Json::Value  jshowtime;
						if (!strshowtime.empty() && reader.parse(strshowtime, jshowtime))
						{
							if (jshowtime.isMember("sh") && jshowtime["sh"].isIntegral())
							{
								tagInfo.shour = jshowtime["sh"].asInt();
							}
							else
							{
								LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
								timecontrol = false;
							}
							if (jshowtime.isMember("sm") && jshowtime["sm"].isIntegral())
							{
								tagInfo.sminute = jshowtime["sm"].asInt();
							}
							else
							{
								LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
								timecontrol = false;
							}
							if (jshowtime.isMember("eh") && jshowtime["eh"].isIntegral())
							{
								tagInfo.ehour = jshowtime["eh"].asInt();
							}
							else
							{
								LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
								timecontrol = false;
							}
							if (jshowtime.isMember("em") && jshowtime["em"].isIntegral())
							{
								tagInfo.eminute = jshowtime["em"].asInt();
							}
							else
							{
								LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
								timecontrol = false;
							}
						}
						else
						{
							timecontrol = false;
							LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());
						}
					}
					else
					{
						LOG_DEBUG("showtime err - account:%s", tagInfo.account.c_str());

						timecontrol = false;
					}
					tagInfo.timecontrol = timecontrol;

					LOG_ERROR("解析微信充值显示 - account:%s,bIsWeChatInfo:%d,shour:%d, sminute:%d, ehour:%d, eminute:%d,timecontrol:%d,showtime:%s", tagInfo.account.c_str(), bIsWeChatInfo, tagInfo.shour, tagInfo.sminute, tagInfo.ehour, tagInfo.eminute, tagInfo.timecontrol, tagInfo.showtime.c_str());

					if (jValueInfoDetails.isMember("vip") && jValueInfoDetails["vip"].isString())
					{
						string strVip = jValueInfoDetails["vip"].asString();
						tagInfo.strVip = strVip;
						Json::Value  jvip;

						if (!tagInfo.strVip.empty() && reader.parse(tagInfo.strVip, jvip))
						{
							if (jvip.isArray())
							{
								for (uint32 i = 0; i < jvip.size(); i++)
								{
									LOG_DEBUG("解析微信充值 - strVip:%s,isArray:%d,isIntegral:%d", tagInfo.strVip.c_str(), jvip.isArray(), jvip[i].isIntegral());
									if (jvip[i].isIntegral())
									{
										LOG_DEBUG("解析微信充值 - strVip:%s,jvip:%d", tagInfo.strVip.c_str(), jvip[i].asInt());
										tagInfo.vecVip.push_back(jvip[i].asInt());
									}
								}
							}
							else
							{
								LOG_ERROR("解析微信充值错误 - strVip:%s,jvip.isArray():%d", tagInfo.strVip.c_str(), jvip.isArray());
							}
						}
						else
						{
							LOG_ERROR("解析微信充值错误 - strVip:%s", tagInfo.strVip.c_str());
						}
					}
					else
					{
						LOG_DEBUG("vip err - account:%s", tagInfo.account.c_str());
					}

					if (bIsWeChatInfo)
					{
						mpVipProxyWeChatInfo.insert(make_pair(tagInfo.account, tagInfo));
					}
				}

				if (bIsWeChatInfo)
				{
					m_mpVipProxyAliAccInfo.clear();
					for (auto &iter : mpVipProxyWeChatInfo)
					{
						m_mpVipProxyAliAccInfo.insert(make_pair(iter.first, iter.second));
					}
				}

			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendVipAliAccRecharge();
	}

	LOG_DEBUG("bPhpSend:%d,bIsWeChatInfo:%d,m_VipAliAccRechargeStatus:%d,m_lVipAliAccRecharge:%lld,m_mpVipProxyWeChatInfo:%d,strVipProxyRecharge:%s",
		bPhpSend, bIsWeChatInfo, m_VipAliAccRechargeStatus, m_lVipAliAccRecharge, m_mpVipProxyAliAccInfo.size(), strVipProxyRecharge.c_str());
	return true;
}

bool   CDataCfgMgr::UnionPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue))
	{
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());

	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "unionpay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lUnionPayRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_UnionPayRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendUnionPayRecharge();
	}

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,m_lUnionpayRecharge:%lld,m_UnionpayRechargeStatus:%d,strjvalueRecharge:%s",
		bPhpSend, m_lUnionPayRecharge, m_UnionPayRechargeStatus, strjvalueRecharge.c_str());
	return true;
}

bool   CDataCfgMgr::WeChatPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue))
	{
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());

	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "wechatpay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lWeChatPayRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_WeChatPayRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendWeChatPayRecharge();
	}

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,PayRecharge:%lld,PayRechargeStatus:%d,strjvalueRecharge:%s",
		bPhpSend, m_lWeChatPayRecharge, m_WeChatPayRechargeStatus, strjvalueRecharge.c_str());
	return true;
}

bool   CDataCfgMgr::AliPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue))
	{
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());

	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "alipay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lAliPayRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_AliPayRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendAliPayRecharge();
	}

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,PayRecharge:%lld,PayRechargeStatus:%d,strjvalueRecharge:%s",
		bPhpSend, m_lAliPayRecharge, m_AliPayRechargeStatus, strjvalueRecharge.c_str());
	return true;
}


bool   CDataCfgMgr::OtherPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue))
	{
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());

	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "otherpay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lOtherPayRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_OtherPayRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendOtherPayRecharge();
	}

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,PayRecharge:%lld,PayRechargeStatus:%d,strjvalueRecharge:%s",
		bPhpSend, m_lOtherPayRecharge, m_OtherPayRechargeStatus, strjvalueRecharge.c_str());
	return true;
}


bool   CDataCfgMgr::QQPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue))
	{
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());

	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "qqpay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lQQPayRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_QQPayRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendQQPayRecharge();
	}

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,PayRecharge:%lld,PayRechargeStatus:%d,strjvalueRecharge:%s",
		bPhpSend, m_lQQPayRecharge, m_QQPayRechargeStatus, strjvalueRecharge.c_str());
	return true;
}


bool   CDataCfgMgr::WeChatScanPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue))
	{
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());

	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "wechatscanpay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lWeChatScanPayRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_WeChatScanPayRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendWeChatScanPayRecharge();
	}

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,PayRecharge:%lld,PayRechargeStatus:%d,strjvalueRecharge:%s",
		bPhpSend, m_lWeChatScanPayRecharge, m_WeChatScanPayRechargeStatus, strjvalueRecharge.c_str());
	return true;
}

bool   CDataCfgMgr::JDPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue))
	{
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());

	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "jdpay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lJDPayRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_JDPayRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendJDPayRecharge();
	}

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,PayRecharge:%lld,PayRechargeStatus:%d,strjvalueRecharge:%s",
		bPhpSend, m_lJDPayRecharge, m_JDPayRechargeStatus, strjvalueRecharge.c_str());
	return true;
}

bool   CDataCfgMgr::ApplePayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue))
	{
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());

	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "applepay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lApplePayRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_ApplePayRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendApplePayRecharge();
	}

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,PayRecharge:%lld,PayRechargeStatus:%d,strjvalueRecharge:%s",
		bPhpSend, m_lApplePayRecharge, m_ApplePayRechargeStatus, strjvalueRecharge.c_str());
	return true;
}

bool   CDataCfgMgr::LargeAliPayRechargeAnalysis(bool bPhpSend, string & strjvalueRecharge)
{
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue))
	{
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}

	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());

	if (jvalue.isMember("data") && jvalue["data"].isObject())
	{
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString())
		{
			string straction = jValueData["action"].asString();

			LOG_DEBUG("straction:%s", straction.c_str());

			if (straction == "bigalipay")
			{
				if (jValueData.isMember("recharge") && jValueData["recharge"].isIntegral())
				{
					m_lLargeAliPayRecharge = jValueData["recharge"].asInt64();
				}
				else
				{
					LOG_DEBUG("recharge err - straction:%s", straction.c_str());
				}
				if (jValueData.isMember("status") && jValueData["status"].isIntegral())
				{
					m_LargeAliPayRechargeStatus = jValueData["status"].asInt();
				}
				else
				{
					LOG_DEBUG("status err - straction:%s", straction.c_str());
				}
			}
		}
	}
	if (bPhpSend)
	{
		CPlayerMgr::Instance().SendLargeAliPayRecharge();
	}

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,PayRecharge:%lld,PayRechargeStatus:%d,strjvalueRecharge:%s",
		bPhpSend, m_lLargeAliPayRecharge, m_LargeAliPayRechargeStatus, strjvalueRecharge.c_str());
	return true;
}

// 专享闪付
bool CDataCfgMgr::ExclusiveFlashRechargeAnalysis(bool bPhpSend, string &strjvalueRecharge) {
	Json::Reader reader;
	Json::Value  jvalue;
	if (strjvalueRecharge.empty() || !reader.parse(strjvalueRecharge, jvalue)) {
		LOG_ERROR("strjvalueRecharge - error:%s", strjvalueRecharge.c_str());
		return false;
	}
	LOG_DEBUG("Analysis strjvalueRecharge - strjvalueRecharge:%s", strjvalueRecharge.c_str());
	bool bIsWeChatInfo = true;
	if (jvalue.isMember("data") && jvalue["data"].isObject()) {
		Json::Value jValueData = jvalue["data"];
		if (jValueData.isMember("action") && jValueData["action"].isString()) {
			string straction = jValueData["action"].asString();
			LOG_DEBUG("straction:%s", straction.c_str());
			if (straction == "exclusiveshanpay")
				StatusRechargeAnalysis(jValueData, m_exclusiveFlashRechargeStatus, m_lExclusiveFlashRecharge);
		}
	}
	if (bPhpSend)
		CPlayerMgr::Instance().SendExclusiveFlashRecharge();

	LOG_DEBUG("config_pay_Recharge - bPhpSend:%d,PayRecharge:%lld,PayRechargeStatus:%d,bIsWeChatInfo:%d,strjvalueRecharge:%s",
		bPhpSend, m_lExclusiveFlashRecharge, m_exclusiveFlashRechargeStatus, bIsWeChatInfo, strjvalueRecharge.c_str());
	return true;
}

//判断当前玩家是否对游戏可控
bool   CDataCfgMgr::GetUserControlFlag(uint32 uid, uint32 game_id, string device_id, string check_code)
{
	LOG_DEBUG("uid:%d game_id:%d device_id:%s check_code:%s", uid, game_id, device_id.c_str(), check_code.c_str());

	if (check_code.size() <= 0)
	{
		LOG_DEBUG("the check_code:%s is empty.", check_code.c_str());
		return false;
	}
	
	LOG_DEBUG("1111111111");

	//判断玩家ID是否存在
	auto iter_player = m_mpUserControlCfg.find(uid);
	if (iter_player == m_mpUserControlCfg.end())
	{  
		LOG_DEBUG("the uid:%d is not exist cfg table.", uid);
		return false;
	}
	LOG_DEBUG("222222222");
	//判断校验码是否正确 
	char szKey[64];
	memset(szKey, 0, sizeof(szKey));
	sprintf(szKey, "%d%s%s", uid, device_id.c_str(), iter_player->second.skey.c_str());
	string strDecy = szKey;
	strDecy = getMD5Str(strDecy.c_str(), strDecy.length());
	LOG_DEBUG("3333333");
	if (strDecy != check_code)
	{
		LOG_DEBUG("the check_code is check error.check_code:%s uid:%d device_id:%s skey:%s strDecy:%s", check_code.c_str(), uid, device_id.c_str(),iter_player->second.skey.c_str(), strDecy.c_str());
		return false;
	}
	LOG_DEBUG("44444444444444 size:%d", iter_player->second.cgid.size());
	//判断游戏ID是否存在
	auto iter_game = iter_player->second.cgid.find(game_id);
	if (iter_game == iter_player->second.cgid.end())
	{
		LOG_DEBUG("the game_id:%d is not exist cfg table.", game_id);
		return false;
	}
	LOG_DEBUG("555555555555");
	LOG_DEBUG("uid:%d game_id:%d device_id:%s check_code:%s", uid, game_id, device_id.c_str(), check_code.c_str());
	return true;
}

//获取精准控制配置信息
void   CDataCfgMgr::GetUserControlCfg(map<uint32, tagUserControlCfg> &mpInfo)
{
	auto it = m_mpUserControlCfg.begin();
	for (; it != m_mpUserControlCfg.end(); ++it)
	{
		mpInfo[it->first] = it->second;
	}
}

//更新精准控制配置信息
bool   CDataCfgMgr::UpdateUserControlInfo(uint32 uid, uint32 oper_type, tagUserControlCfg info)
{
	LOG_DEBUG("uid:%d oper_type:%d", uid, oper_type);

	//修改控制玩家配置
	if (oper_type == 1 || oper_type == 2)
	{
		//重置数据
		m_mpUserControlCfg[uid] = info;
	}

	//删除控制玩家配置
	if (oper_type == 3)
	{
		auto iter_player = m_mpUserControlCfg.find(uid);
		if (iter_player != m_mpUserControlCfg.end())
		{			
			//删除当前配置信息
			m_mpUserControlCfg.erase(iter_player);
		}
	}	
	return true;
}

// 用户是否为被跟踪用户
bool   CDataCfgMgr::GetIsUserControl(uint32 uid, uint32 gameType,uint32 & suid)
{
	//LOG_DEBUG("uid:%d gameType:%d", uid, gameType);
	bool bIsUserControl = false;
	suid = 0;
	auto it = m_mpUserControlCfg.begin();
	for (; it != m_mpUserControlCfg.end(); ++it)
	{
		tagUserControlCfg & tagCtrlCfg = it->second;
		if (tagCtrlCfg.tuid == uid)
		{
			auto find_set = tagCtrlCfg.cgid.find(gameType);
			if (find_set != tagCtrlCfg.cgid.end())
			{
				suid = tagCtrlCfg.suid;
				bIsUserControl = true;
				break;
			}
		}
	}

	return bIsUserControl;
}

void    CDataCfgMgr::InitMasterShowInfo()
{
	int iCityIndex = 0;
	while (strcmp("", g_master_city_tables[iCityIndex].c_str()) != 0)
	{
		iCityIndex++;
		m_vecMasterCity.push_back(g_master_city_tables[iCityIndex]);
	}

	for (uint32 i = 5000000; i <= 5999999; i++)
	{
		m_vecMasterUid.push_back(i);
	}
}


string  CDataCfgMgr::GetMasterRandCity()
{
	std::random_device rd;
	std::mt19937 g{ rd() };
	std::shuffle(std::begin(m_vecMasterCity), std::end(m_vecMasterCity), g);
	string tempCity = "北京";
	if (m_vecMasterCity.size()>0)
	{
		tempCity = m_vecMasterCity[0];
	}
	return tempCity;
}

uint32  CDataCfgMgr::GetMasterRandUid()
{
	std::random_device rd;
	std::mt19937 g{ rd() };
	std::shuffle(std::begin(m_vecMasterUid), std::end(m_vecMasterUid), g);
	uint32 tempUid = 5600403;
	if (m_vecMasterUid.size()>0)
	{
		tempUid = m_vecMasterUid[0];
	}
	return tempUid;
}


void CDataCfgMgr::UpdateRobotOnlineCfg(stRobotOnlineCfg & tagCfg)
{

	bool bIsBreak = false;
	auto it_loop_fir = m_mpRobotOnlineCfg.begin();
	for (; it_loop_fir != m_mpRobotOnlineCfg.end(); it_loop_fir++)
	{
		if (it_loop_fir->first == tagCfg.gameType)
		{
			vector<stRobotOnlineCfg> & vecTempCfg = it_loop_fir->second;
			for (uint32 idx = 0; idx < vecTempCfg.size(); idx++)
			{
				stRobotOnlineCfg & refCfg = vecTempCfg[idx];
				if (tagCfg.batchID == refCfg.batchID)
				{
					refCfg.batchID = tagCfg.batchID;
					refCfg.gameType = tagCfg.gameType;
					refCfg.roomID = tagCfg.roomID;
					refCfg.leveltype = tagCfg.leveltype;
					refCfg.loginType = tagCfg.loginType;
					refCfg.enterTime = tagCfg.enterTime;
					refCfg.leaveTime = tagCfg.leaveTime;

					for (uint8 lv = 0; lv < ROBOT_MAX_LEVEL; ++lv)
					{
						refCfg.onlines[lv] = tagCfg.onlines[lv];
					}
					bIsBreak = true;
				}
				if (bIsBreak)
				{
					break;
				}
			}
		}
		if (bIsBreak)
		{
			break;
		}
	}

	if (bIsBreak == false)
	{
		auto iter_find = m_mpRobotOnlineCfg.find(tagCfg.gameType);
		if (iter_find != m_mpRobotOnlineCfg.end())
		{
			vector<stRobotOnlineCfg> & vecTempCfg = iter_find->second;
			vecTempCfg.push_back(tagCfg);
		}
		else
		{
			vector<stRobotOnlineCfg> vecTempCfg;
			vecTempCfg.push_back(tagCfg);
			m_mpRobotOnlineCfg.insert(make_pair(tagCfg.gameType, vecTempCfg));
		}
	}

	string strOnlines;
	uint32 allRobotCount = 0;
	for (uint8 lv = 0; lv < ROBOT_MAX_LEVEL; ++lv)
	{
		uint32 rcount = tagCfg.onlines[lv];
		strOnlines += CStringUtility::FormatToString("%d-%d ", lv, rcount);
		allRobotCount += rcount;
	}

	LOG_DEBUG("tagCfg - bIsBreak:%d,batchID:%d,gametype:%02d,roomID:%d,leveltype:%d,loginType:%d,enterTime:%d,leaveTime:%d,allRobotCount:%d,strOnlines:%s",
		bIsBreak,tagCfg.batchID, tagCfg.gameType, tagCfg.roomID, tagCfg.leveltype, tagCfg.loginType, tagCfg.enterTime, tagCfg.leaveTime, allRobotCount, strOnlines.c_str());


	auto it_loop_sen = m_mpRobotOnlineCfg.begin();
	for (; it_loop_sen != m_mpRobotOnlineCfg.end(); it_loop_sen++)
	{
		vector<stRobotOnlineCfg> & vecTempCfg = it_loop_sen->second;
		for (uint32 idx = 0; idx < vecTempCfg.size(); idx++)
		{
			stRobotOnlineCfg & refCfg = vecTempCfg[idx];
			string strOnlines;
			uint32 allRobotCount = 0;
			for (uint8 lv = 0; lv < ROBOT_MAX_LEVEL; ++lv)
			{
				uint32 rcount = refCfg.onlines[lv];
				strOnlines += CStringUtility::FormatToString("%d-%d ", lv, rcount);
				allRobotCount += rcount;
			}
			LOG_DEBUG("robot_cfg - batchID:%d,gametype:%02d,roomID:%d,leveltype:%d,loginType:%d,enterTime:%d,leaveTime:%d,allRobotCount:%d,strOnlines:%s",
				refCfg.batchID, refCfg.gameType, refCfg.roomID, refCfg.leveltype, refCfg.loginType, refCfg.enterTime, refCfg.leaveTime, allRobotCount, strOnlines.c_str());
		}
	}

}


bool CDataCfgMgr::DeleteRobotOnlineCfg(int gametype, int batchid)
{
	bool bIsDelete = false;
	auto it_loop_fir = m_mpRobotOnlineCfg.begin();
	for (; it_loop_fir != m_mpRobotOnlineCfg.end(); it_loop_fir++)
	{
		if (it_loop_fir->first == (uint32)gametype)
		{
			vector<stRobotOnlineCfg> & vecTempCfg = it_loop_fir->second;
			uint32 idx = 0;
			for (; idx < vecTempCfg.size(); idx++)
			{
				stRobotOnlineCfg & refCfg = vecTempCfg[idx];
				if ((uint32)batchid == refCfg.batchID)
				{
					bIsDelete = true;
				}
				if (bIsDelete)
				{
					break;
				}
			}
			if (bIsDelete)
			{
				vecTempCfg.erase(vecTempCfg.begin() + idx);
			}
		}
		if (bIsDelete)
		{
			break;
		}
	}

	LOG_DEBUG("bIsDelete:%d,gametype:%d,batchid:%d", bIsDelete, gametype, batchid);


	auto it_loop_sen = m_mpRobotOnlineCfg.begin();
	for (; it_loop_sen != m_mpRobotOnlineCfg.end(); it_loop_sen++)
	{
		vector<stRobotOnlineCfg> & vecTempCfg = it_loop_sen->second;
		for (uint32 idx = 0; idx < vecTempCfg.size(); idx++)
		{
			stRobotOnlineCfg & refCfg = vecTempCfg[idx];
			string strOnlines;
			uint32 allRobotCount = 0;
			for (uint8 lv = 0; lv < ROBOT_MAX_LEVEL; ++lv)
			{
				uint32 rcount = refCfg.onlines[lv];
				strOnlines += CStringUtility::FormatToString("%d-%d ", lv, rcount);
				allRobotCount += rcount;
			}
			LOG_DEBUG("robot_cfg - batchID:%d,gametype:%02d,roomID:%d,leveltype:%d,loginType:%d,enterTime:%d,leaveTime:%d,allRobotCount:%d,strOnlines:%s",
				refCfg.batchID, refCfg.gameType, refCfg.roomID, refCfg.leveltype, refCfg.loginType, refCfg.enterTime, refCfg.leaveTime, allRobotCount, strOnlines.c_str());
		}
	}
	return bIsDelete;
}

void CDataCfgMgr::LoadLuckyConfig(uint32 uid, uint32 gameType, map<uint8, tagUserLuckyCfg> &mpLuckyCfg)
{
	CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).LoadLuckyCfg(uid, gameType, mpLuckyCfg);
}

void CDataCfgMgr::UpdateLuckyInfo(uint32 uid, uint32 gameType, uint32 roomid, tagUserLuckyCfg mpLuckyInfo)
{
	CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).UpdateLuckyCfg(uid, gameType, roomid, mpLuckyInfo);
	CDBMysqlMgr::Instance().GetSyncDBOper(DB_INDEX_TYPE_CFG).UpdateLuckyLog(uid, gameType, roomid, mpLuckyInfo);
}