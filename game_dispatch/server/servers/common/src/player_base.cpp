

#include "player_base.h"
#include "center_log.h"
#include "data_cfg_mgr.h"

using namespace std;
using namespace svrlib;
using namespace Network;

namespace
{

};
CPlayerBase::CPlayerBase(uint8 type)
:m_bPlayerType(type)
,m_pSession(NULL)
,m_bPlayerState(PLAYER_STATE_NULL)
{
	m_uid = 0;
    memset(m_loadState,0,sizeof(m_loadState));
    memset(m_loadGameState,0,sizeof(m_loadGameState));
    
    memset(m_gameInfo,0,sizeof(m_gameInfo));

	m_strCity = "";
	m_strLoginIP = "";
}
CPlayerBase::~CPlayerBase()
{
    if(m_pSession != NULL){
        m_pSession->DestroyObj();
        m_pSession = NULL;
    }    
}
bool  CPlayerBase::SetBaseInfo(stPlayerBaseInfo& info)
{
    m_baseInfo = info;
    SetLoadState(emACCDATA_TYPE_BASE,1);
    return true;
}
bool  CPlayerBase::SetAccountInfo(stAccountInfo& info)
{
    m_accInfo = info;
    SetLoadState(emACCDATA_TYPE_ACC,1);
    return true;
}
bool  CPlayerBase::SetGameInfo(uint16 gameType,const stGameBaseInfo& info)
{
    m_gameInfo[gameType] = info;
    
    m_loadGameState[gameType] = 1;
    bool isAllLoad = true;
    for(uint16 i=1;i<net::GAME_CATE_MAX_TYPE;++i){
        if(m_loadGameState[i] == 0){
            isAllLoad = false;
            break;
        }
    }
    if(isAllLoad){
        SetLoadState(emACCDATA_TYPE_GAME,1);
    }    
    return true;
}
bool  CPlayerBase::IsLoadData(uint8 dataType)
{    
    if(dataType < emACCDATA_TYPE_MAX){
        return m_loadState[dataType];
    }
    return false;
}
void  CPlayerBase::SetLoadState(uint8 dataType,uint8 state)
{
    if(dataType < emACCDATA_TYPE_MAX){
        m_loadState[dataType] = state;
    }
}
bool CPlayerBase::IsLoadOver()
{
    for(uint8 i=emACCDATA_TYPE_BASE;i<emACCDATA_TYPE_MAX;++i)
    {
        if(m_loadState[i] == 0)
            return false;
    }
    return true;
}  
void  CPlayerBase::SetPlayerState(uint8 state)
{
	m_bPlayerState = state;
}
uint8 CPlayerBase::GetPlayerState()
{
	return m_bPlayerState;
}
bool  CPlayerBase::IsPlaying()
{
	return  m_bPlayerState == PLAYER_STATE_PLAYING;
}
uint32	CPlayerBase::GetUID()
{
	return m_uid;
}
void	CPlayerBase::SetUID(uint32 uid)
{
	m_uid = uid;
}
bool    CPlayerBase::IsRobot()
{
    return (m_bPlayerType == PLAYER_TYPE_ROBOT);
}
string 	CPlayerBase::GetPlayerName()
{
	return m_baseInfo.name;
}
void 	CPlayerBase::SetPlayerName(string name)
{
    m_baseInfo.name = name;
}
uint8	CPlayerBase::GetSex()
{
    return m_baseInfo.sex;
}
void 	CPlayerBase::SetSex(uint8 sex)
{
    m_baseInfo.sex = sex;
}
uint16	CPlayerBase::GetHeadIcon()
{
    return m_baseInfo.headIcon;
}
void  	CPlayerBase::SetSession(NetworkObject* pSession)
{
    m_pSession = pSession;
}
NetworkObject* CPlayerBase::GetSession()
{
    return m_pSession;
}
void CPlayerBase::SetIP(string strIP)
{
	if (strIP.empty())
	{
		return;
	}
	uint32 uloginip = inet_addr(strIP.c_str());
	if (uloginip == INADDR_NONE)
	{
		return;
	}
	m_strLoginIP = strIP;
	m_baseInfo.loginIP = uloginip;
}
uint32	CPlayerBase::GetIP()
{
 //   if(m_pSession){
	//	return m_pSession->GetIP();
	//}
    return m_baseInfo.loginIP;
}
string  CPlayerBase::GetIPStr()
{
    if(m_pSession){
		return m_strLoginIP;// m_pSession->GetSIP();
	}
	return " ";
}
bool  	CPlayerBase::SendMsgToClient(const google::protobuf::Message* msg,uint16 msg_type)
{
    if(m_pSession)
    {
        return SendProtobufMsg(m_pSession,msg,msg_type);
    }
    return false;
}
bool  	CPlayerBase::SendMsgToClient(const void *msg, uint16 msg_len, uint16 msg_type)
{
    if(m_pSession){
        return SendProtobufMsg(m_pSession, msg, msg_len, msg_type);
    }
    return false;
}
void 	CPlayerBase::OnLoginOut(uint32 leaveparam)
{

}
void  	CPlayerBase::OnLogin()
{

}
void	CPlayerBase::OnGetAllData()
{

}
void	CPlayerBase::ReLogin()
{

}
void 	CPlayerBase::OnTimeTick(uint64 uTime,bool bNewDay)
{

}
// 是否需要回收
bool 	CPlayerBase::NeedRecover()
{

    return false;
}
void	CPlayerBase::GetPlayerBaseData(net::base_info* pInfo)
{
    pInfo->set_uid(GetUID());
    pInfo->set_name(m_baseInfo.name);
    pInfo->set_sex(m_baseInfo.sex);
    pInfo->set_safeboxstate(m_baseInfo.safeboxState);
    pInfo->set_clogin(m_baseInfo.clogin);
    pInfo->set_weeklogin(m_baseInfo.weekLogin);
    pInfo->set_reward(m_baseInfo.reward);
    pInfo->set_bankrupt(m_baseInfo.bankrupt);

    pInfo->set_diamond(m_accInfo.diamond);
    pInfo->set_coin(m_accInfo.coin);
    pInfo->set_score(m_accInfo.score);
    pInfo->set_ingot(m_accInfo.ingot);
    pInfo->set_cvalue(m_accInfo.cvalue);
    pInfo->set_safe_coin(m_accInfo.safecoin);
    pInfo->set_vip(m_accInfo.vip);
    pInfo->set_head_icon(m_baseInfo.headIcon);
    pInfo->set_day_game_count(m_baseInfo.dayGameCount);
    pInfo->set_login_ip(m_baseInfo.loginIP);

	pInfo->set_city(GetCity());
	pInfo->set_recharge(m_accInfo.recharge);
	pInfo->set_converts(m_accInfo.converts);
	pInfo->set_batchid(m_batchID);
	pInfo->set_lvscore(m_lvScore);
	pInfo->set_lvcoin(m_lvCoin);
}
void	CPlayerBase::SetPlayerBaseData(const net::base_info& info)
{
    SetUID(info.uid());
    m_baseInfo.name         = info.name();
    m_baseInfo.sex          = info.sex();
    m_baseInfo.safeboxState = info.safeboxstate();
    m_baseInfo.clogin       = info.clogin();
    m_baseInfo.weekLogin    = info.weeklogin();
    m_baseInfo.reward       = info.reward();
    m_baseInfo.bankrupt     = info.bankrupt();
    m_baseInfo.headIcon     = info.head_icon();
    m_baseInfo.dayGameCount = info.day_game_count();
    m_baseInfo.loginIP      = info.login_ip();

    m_accInfo.diamond       = info.diamond();
    m_accInfo.coin          = info.coin();
    m_accInfo.score         = info.score();
    m_accInfo.ingot         = info.ingot();
    m_accInfo.cvalue        = info.cvalue();
    m_accInfo.safecoin      = info.safe_coin();
    m_accInfo.vip           = info.vip();

	SetCity(info.city());
	m_accInfo.recharge = info.recharge();
	m_accInfo.converts = info.converts();

	SetBatchID(info.batchid());
	SetLvScore(info.lvscore());
	SetLvCoin(info.lvcoin());
}
void    CPlayerBase::GetGameData(uint16 gameType,net::game_data_info* pInfo)
{
    stGameBaseInfo& info = m_gameInfo[gameType];
    pInfo->set_game_type(gameType);
    pInfo->set_win(info.win);
    pInfo->set_lose(info.lose);
    pInfo->set_maxwin(info.maxwin);
    pInfo->set_winc(info.winc);
    pInfo->set_losec(info.losec);
    pInfo->set_maxwinc(info.maxwinc);
    pInfo->set_daywin(info.daywin);
    pInfo->set_daywinc(info.daywinc);

    pInfo->set_land(info.land);
    pInfo->set_spring(info.spring);
    pInfo->set_landc(info.landc);
    pInfo->set_springc(info.springc);
	pInfo->set_totalwinc(info.totalwinc);
	pInfo->set_stockscore(info.stockscore);
	pInfo->set_gamecount(info.gamecount);
    for(uint8 i=0;i<5;++i){
        pInfo->add_maxcard(info.bestCard[i]);
        pInfo->add_maxcardc(info.bestCardc[i]);
    }    
}
void    CPlayerBase::SetGameData(uint16 gameType,const net::game_data_info& refInfo)
{
    stGameBaseInfo& info = m_gameInfo[gameType]; 
    info.win      = refInfo.win();
    info.lose     = refInfo.lose();
    info.maxwin   = refInfo.maxwin();
    info.winc     = refInfo.winc();
    info.losec    = refInfo.losec();
    info.maxwinc  = refInfo.maxwinc();
    info.daywin   = refInfo.daywin();
    info.daywinc  = refInfo.daywinc();

    info.land     = refInfo.land();
    info.spring   = refInfo.spring();
    info.landc    = refInfo.landc();
    info.springc   = refInfo.springc();
	info.totalwinc = refInfo.totalwinc();
	info.stockscore = refInfo.stockscore();
	info.gamecount = refInfo.gamecount();
    for(uint8 i=0;i<refInfo.maxcard_size() && i<5;++i){
        info.bestCard[i] = refInfo.maxcard(i);
    }
    for(uint8 i=0;i<refInfo.maxcardc_size() && i<5;++i){
        info.bestCardc[i] = refInfo.maxcardc(i);
    }
}
void 	CPlayerBase::SetPlayerGameData(const net::msg_enter_into_game_svr& info)
{
    SetPlayerBaseData(info.base_data());
    SetGameData(info.game_data().game_type(),info.game_data());
    
}
void	CPlayerBase::GetPlayerGameData(uint16 gameType,net::msg_enter_into_game_svr* pInfo)
{
    GetPlayerBaseData(pInfo->mutable_base_data());
    GetGameData(gameType,pInfo->mutable_game_data());    
}

// 修改玩家账号数值（增量修改）
bool 	CPlayerBase::CanChangeAccountValue(uint8 valueType,int64 value)
{
    if(value >= 0)
        return true;
    int64 curValue = GetAccountValue(valueType);
    if(curValue < abs(value))
        return false;
    return true;
}
int64   CPlayerBase::ChangeAccountValue(uint8 valueType,int64 value)
{
    if(value == 0)
        return 0;
    int64 curValue      = GetAccountValue(valueType);
    int64 changeValue   = value;
    curValue += value;
    if(curValue < 0){
        curValue = 0;
        changeValue = -GetAccountValue(valueType);
    }
    SetAccountValue(valueType,curValue);
    return changeValue;
}
int64 	CPlayerBase::GetAccountValue(uint8 valueType) {
    switch (valueType)
    {
    case emACC_VALUE_DIAMOND:    // 钻石
        {
            return m_accInfo.diamond;
        }break;
    case emACC_VALUE_COIN:       // 财富币
        {
            return m_accInfo.coin;
        }break;
    case emACC_VALUE_SCORE:      // 积分
        {
            return m_accInfo.score;
        }break;
    case emACC_VALUE_INGOT:      // 元宝
        {
            return m_accInfo.ingot;
        }break;
    case emACC_VALUE_CVALUE:     // 魅力值
        {
            return m_accInfo.cvalue;
        }break;
    case emACC_VALUE_SAFECOIN:   // 保险箱值
        {
            return m_accInfo.safecoin;
        }break;
	//case emACC_VALUE_STOCKSCORE:
	//	{
	//		return m_StockScore;  //库存分数
	//	}
    default:
        assert(false);
    }
    return 0;
}
void 	CPlayerBase::SetAccountValue(uint8 valueType,int64 value)
{
    switch (valueType)
    {
    case emACC_VALUE_DIAMOND:    // 钻石
        {
            m_accInfo.diamond = value;
        }break;
    case emACC_VALUE_COIN:       // 财富币
        {
            m_accInfo.coin = value;
        }break;
    case emACC_VALUE_SCORE:      // 积分
        {
            m_accInfo.score = value;
        }break;
    case emACC_VALUE_INGOT:      // 元宝
        {
            m_accInfo.ingot = value;
        }break;
    case emACC_VALUE_CVALUE:     // 魅力值
        {
            m_accInfo.cvalue = value;
        }break;
    case emACC_VALUE_SAFECOIN:
        {
            m_accInfo.safecoin = value;
        }break;
	//case emACC_VALUE_STOCKSCORE:
	//	{
	//		m_StockScore = value;
	//	}break;
    default:
        assert(false);
    }
}
// 原子操作账号数值
bool    CPlayerBase::AtomChangeAccountValue(uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin)
{
    if(!CanChangeAccountValue(emACC_VALUE_DIAMOND,diamond) || !CanChangeAccountValue(emACC_VALUE_COIN,coin) || !CanChangeAccountValue(emACC_VALUE_SCORE,score)
       || !CanChangeAccountValue(emACC_VALUE_INGOT,ingot) || !CanChangeAccountValue(emACC_VALUE_CVALUE,cvalue) || !CanChangeAccountValue(emACC_VALUE_SAFECOIN,safecoin))
    {
        return false;
    }
    bool ret = SyncChangeAccountValue(operType,subType,diamond,coin,ingot,score,cvalue,safecoin);
    return ret;
}

// 存取保险箱
bool 	CPlayerBase::TakeSafeBox(int64 takeCoin)
{
    if(takeCoin == 0)
        return false;
    int64 curSafeValue = GetAccountValue(emACC_VALUE_SAFECOIN);
    if(takeCoin > 0)//取钱
    {
        if(curSafeValue < takeCoin){
            return false;
        }
    }else{//存钱
        if(GetAccountValue(emACC_VALUE_COIN) < abs(takeCoin)){
            return false;
        }
    }
    SyncChangeAccountValue(emACCTRAN_OPER_TYPE_SAFEBOX,0,0,takeCoin,0,0,0,-takeCoin);
    return true;
}
void 	CPlayerBase::SetSafePasswd(const string& passwd)
{
    m_baseInfo.safepwd = passwd;
}
bool 	CPlayerBase::CheckSafePasswd(const string& passwd)
{
    if(m_baseInfo.safepwd == passwd){
        return true;
    }
    LOG_DEBUG("old:%s-len:%d,new:%s-len:%d",m_baseInfo.safepwd.c_str(),m_baseInfo.safepwd.length(),passwd.c_str(),passwd.length());
    return false;
}
void 	CPlayerBase::SetSafeBoxState(uint8 state,bool checkpwd)
{
    if(checkpwd && m_baseInfo.safepwd.length() < 1){
        m_baseInfo.safeboxState = 2;
    }else {
        m_baseInfo.safeboxState = state;
    }
}
uint8   CPlayerBase::GetSafeBoxState()
{
    return m_baseInfo.safeboxState;
}
// 钻石兑换积分财富币
uint8 	CPlayerBase::ExchangeScore(uint32 exid,uint8 extype)
{
    int64 fromValue = 0;
    int64 toValue   = 0;
    bool bRet = CDataCfgMgr::Instance().GetExchangeValue(extype,exid,fromValue,toValue);
    if(bRet != true){
        return net::RESULT_CODE_ERROR_PARAM;
    }
    if(GetAccountValue(emACC_VALUE_DIAMOND) < fromValue){
        LOG_DEBUG("兑换钻石不足");
        return net::RESULT_CODE_NOT_DIAMOND;
    }
    if(extype == net::EXCHANGE_TYPE_SCORE){
        SyncChangeAccountValue(emACCTRAN_OPER_TYPE_EXCHANGE,extype,-fromValue,0,0,toValue,0,0);
    }else if(extype == net::EXCHANGE_TYPE_COIN){
        SyncChangeAccountValue(emACCTRAN_OPER_TYPE_EXCHANGE,extype,-fromValue,toValue,0,0,0,0);
    }
    LOG_DEBUG("%d 兑换积分财富币:id:%d--type:%d,from:%lld,to:%lld",GetUID(), exid,extype,fromValue,toValue);
    return net::RESULT_CODE_SUCCESS;
}
//--- 操作记录
void	CPlayerBase::SetRewardBitFlag(uint32 flag)
{
    SetBitFlag(&m_baseInfo.reward,1,flag);
}
void	CPlayerBase::UnsetRewardBitFlag(uint32 flag)
{
    UnsetBitFlag(&m_baseInfo.reward,1,flag);
}
bool	CPlayerBase::IsSetRewardBitFlag(uint32 flag)
{
    return IsSetBitFlag(&m_baseInfo.reward,1,flag);
}
// 修改斗地主数值(增量修改)
void    CPlayerBase::ChangeLandValue(bool isCoin,int32 win,int32 lose,int32 land,int32 spring,int64 winscore)
{    
    if(isCoin){
        m_gameInfo[net::GAME_CATE_LAND].winc      += win;
        m_gameInfo[net::GAME_CATE_LAND].landc     += land;
        m_gameInfo[net::GAME_CATE_LAND].losec     += lose;
        m_gameInfo[net::GAME_CATE_LAND].springc   += spring;
        m_gameInfo[net::GAME_CATE_LAND].maxwinc   = MAX(m_gameInfo[net::GAME_CATE_LAND].maxwinc,winscore);
    }else {
        m_gameInfo[net::GAME_CATE_LAND].win      += win;
        m_gameInfo[net::GAME_CATE_LAND].land     += land;
        m_gameInfo[net::GAME_CATE_LAND].lose     += lose;
        m_gameInfo[net::GAME_CATE_LAND].spring   += spring;
        m_gameInfo[net::GAME_CATE_LAND].maxwin   = MAX(m_gameInfo[net::GAME_CATE_LAND].maxwin,winscore);
    }
}
// 修改游戏数值
void 	CPlayerBase::ChangeGameValue(uint16 gameType,bool isCoin,int32 win,int32 lose,int64 winscore)
{
    assert(gameType < net::GAME_CATE_MAX_TYPE);
    if(isCoin){
        m_gameInfo[gameType].winc     += win;
        m_gameInfo[gameType].losec    += lose;
        m_gameInfo[gameType].maxwinc  = MAX(m_gameInfo[gameType].maxwinc,winscore);
        m_gameInfo[gameType].daywinc  += winscore;
		m_gameInfo[gameType].totalwinc += winscore;
		m_gameInfo[gameType].gamecount += 1;
		SetVecWin(gameType, win);
    }else{
        m_gameInfo[gameType].win      += win;
        m_gameInfo[gameType].lose     += lose;
        m_gameInfo[gameType].maxwin   = MAX(m_gameInfo[gameType].maxwin,winscore);
        m_gameInfo[gameType].daywin   += winscore;
    }        
}
int64	CPlayerBase::GetGameMaxScore(uint16 gameType,bool isCoin)
{
    assert(gameType < net::GAME_CATE_MAX_TYPE);
    if(isCoin){
        return m_gameInfo[gameType].maxwinc;
    }else{
        return m_gameInfo[gameType].maxwin;
    }
}
// 设置最大牌型
void 	CPlayerBase::SetGameMaxCard(uint16 gameType,bool isCoin,uint8 cardData[],uint8 cardCount)
{
    assert(gameType < net::GAME_CATE_MAX_TYPE);
    if(isCoin){
        memcpy(m_gameInfo[gameType].bestCard,cardData,cardCount);
    }else{
        memcpy(m_gameInfo[gameType].bestCardc,cardData,cardCount);
    }    
}
void 	CPlayerBase::GetGameMaxCard(uint16 gameType,bool isCoin,uint8* cardData,uint8 cardCount)
{
    assert(gameType < net::GAME_CATE_MAX_TYPE);
    if(isCoin){
        memcpy(cardData,m_gameInfo[gameType].bestCard,cardCount);
    }else{
        memcpy(cardData,m_gameInfo[gameType].bestCard,cardCount);
    }    
}

int64	CPlayerBase::GetPlayerDayWin(uint16 gameType) {
	if (gameType < net::GAME_CATE_MAX_TYPE)
		return m_gameInfo[gameType].daywinc;
	return 0;
}

int64	CPlayerBase::GetPlayerTotalWinScore(uint16 gameType)
{
	//assert(gameType < net::GAME_CATE_MAX_TYPE);
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		return m_gameInfo[gameType].totalwinc;
	}
	return 0;
}

bool	CPlayerBase::SendVipBroadCast()
{
	string broadCast = CDataCfgMgr::Instance().GetVipBroadcastMsg();
	int64  lVipRecharge = CDataCfgMgr::Instance().GetVipBroadcastRecharge();
	int32  iVipStatus = CDataCfgMgr::Instance().GetVipBroadcastStatus();
	LOG_DEBUG("SendBroadcast  - uid:%d,iVipStatus:%d,GetRecharge:%lld,lVipRecharge:%lld,broadCast:%s", GetUID(), iVipStatus, GetRecharge(), lVipRecharge, broadCast.c_str());

	if (iVipStatus == 1 || GetRecharge() < lVipRecharge || broadCast.empty())
	{
		return false;
	}
	net::msg_php_broadcast_rep rep;
	rep.set_msg(broadCast);
	SendMsgToClient(&rep, net::S2C_MSG_PHP_BROADCAST);

	return true;
}

// 设置用户水果机库存 
//bool  CPlayerBase::SetStockScore(int64 nStockScore)
//{
//	m_StockScore = nStockScore;
//	SetLoadState(emACCDATA_TYPE_ACC, 1);
//	return true;
//}

// 更新用户水果机库存 
//int64  CPlayerBase::ChangeStockScore(int64 nStockScore)
//{
//	m_StockScore += nStockScore;
//	return m_StockScore;
//}

// 更新用户水果机库存 
int64  CPlayerBase::ChangeStockScore(uint16 gametype, int64 nStockScore)
{
	int64 stockscore = 0;
	if (gametype < net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gametype].stockscore += nStockScore;
		stockscore = m_gameInfo[gametype].stockscore;
	}
	return stockscore;
}

int64	CPlayerBase::GetPlayerStockScore(uint16 gameType)
{
	//assert(gameType < net::GAME_CATE_MAX_TYPE);
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		return m_gameInfo[gameType].stockscore;
	}
	return 0;
}

void	CPlayerBase::ReSetGameCount(uint16 gameType)
{
	//assert(gameType < net::GAME_CATE_MAX_TYPE);
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].gamecount = 0;
	}
}

int64	CPlayerBase::GetPlayerGameCount(uint16 gameType)
{
	//assert(gameType < net::GAME_CATE_MAX_TYPE);
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		return m_gameInfo[gameType].gamecount;
	}
	return 0;
}

uint32 CPlayerBase::GetPlayerPayStatus(int32 iPayStatus, int64 lPayRecharge) {
	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
		return 1;
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
		return 0;
	return 1;
}

bool	CPlayerBase::SendVipProxyRecharge()
{
	int32  iVipStatus = CDataCfgMgr::Instance().GetVipProxyRechargeStatus();
	int64  lVipRecharge = CDataCfgMgr::Instance().GetVipProxyRecharge();
	map<string, tagVipRechargeWechatInfo> mpVipProxyWeChatInfo;
	CDataCfgMgr::Instance().GetVipProxyWeChatInfo(mpVipProxyWeChatInfo);

	uint32 uPlayerVipStatus = 1;

	if (iVipStatus == 1 || GetRecharge() < lVipRecharge)
	{
		uPlayerVipStatus = 1;
	}
	else if (iVipStatus == 0 && GetRecharge() >= lVipRecharge)
	{
		uPlayerVipStatus = 0;
	}
	else
	{
		uPlayerVipStatus = 1;
	}

	net::msg_notify_vip_recharge_show msg;
	msg.set_status(uPlayerVipStatus);

	tm	tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	getLocalTime(&tmTime, getSysTime());

	// shour:15, sminute:0, ehour:16, eminute:8
	// tmTime.tm_hour:15,, tmTime.tm_min:43

	// account:weixin003,tmTime.tm_hour:15,tmTime.tm_min:43,bIsShowWeChatInfo:1,timecontrol:1,shour:15, sminute:38, ehour:15, eminute:40,showtime:{"sh":15,"sm":38,"eh":15,"em":40}
	// account:weixin002, tmTime.tm_hour : 16, tmTime.tm_min : 3, bIsShowWeChatInfo : 0, timecontrol : 1, shour : 15, sminute : 0, ehour : 16, eminute : 8, showtime : {"sh":15, "sm" : 0, "eh" : 16, "em" : 8}, size : 3, tagInfo : 2 - 标题002 - weixin002]
	for (auto &iter : mpVipProxyWeChatInfo)
	{
		bool bIsShowWeChatInfo = false;
		tagVipRechargeWechatInfo tagInfo = iter.second;
		if (tagInfo.shour > 23 || tagInfo.ehour > 23 || tagInfo.sminute > 59 || tagInfo.eminute > 59)
		{
			bIsShowWeChatInfo = true;
		}
		else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour < tagInfo.ehour)
		{
			bIsShowWeChatInfo = true;
		}
		else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour == tagInfo.ehour)
		{
			//是否正在进行中
			if (tmTime.tm_min >= tagInfo.sminute && tmTime.tm_min <= tagInfo.eminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour < tagInfo.ehour)
		{
			//是否开始了
			if (tmTime.tm_min >= tagInfo.sminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour == tagInfo.ehour)
		{
			//是否结束了
			if (tmTime.tm_min <= tagInfo.eminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else
		{
			bIsShowWeChatInfo = false;
		}
		if (tagInfo.timecontrol == false)
		{
			bIsShowWeChatInfo = true;
		}


		//bool bIsVipShow = false;
		//if (tagInfo.vecVip.size() == 0)
		//{
		//	bIsVipShow = true;
		//}
		//for (uint32 uIndex = 0; uIndex < tagInfo.vecVip.size(); uIndex++)
		//{
		//	LOG_DEBUG("uid:%d,uIndex:%d,vecVip:%d,GetVipLevel:%d", GetUID(), uIndex, tagInfo.vecVip[uIndex], GetVipLevel());
		//	if (tagInfo.vecVip[uIndex] == GetVipLevel())
		//	{
		//		bIsVipShow = true;
		//		break;
		//	}
		//}
		bool bIsVipShow = true;

		if (bIsVipShow == false)
		{
			bIsShowWeChatInfo = false;
		}

		if (bIsShowWeChatInfo)
		{
			net::vip_recharge_wechatinfo * pwechatinfo = msg.add_info();

			pwechatinfo->set_sortid(tagInfo.sortid);
			pwechatinfo->set_title(tagInfo.title);
			pwechatinfo->set_account(tagInfo.account);
		}

		LOG_DEBUG("SendVipProxyRecharge  - uid:%d,tagInfo.account:%s,tmTime.tm_hour:%d,tmTime.tm_min:%d,bIsShowWeChatInfo:%d,timecontrol:%d,shour:%d, sminute:%d, ehour:%d, eminute:%d,showtime:%s,size:%d,tagInfo:%d - %s - %s", GetUID(), tagInfo.account.c_str(), tmTime.tm_hour, tmTime.tm_min, bIsShowWeChatInfo, tagInfo.timecontrol, tagInfo.shour, tagInfo.sminute, tagInfo.ehour, tagInfo.eminute, tagInfo.showtime.c_str(), msg.info_size(), tagInfo.sortid, tagInfo.title.c_str(), tagInfo.account.c_str());
	}

	//for (uint32 i = 0; i < msg.info_size(); i++)
	//{

	//}

	LOG_DEBUG("SendVipProxyRecharge  - uid:%d,iVipStatus:%d,uPlayerVipStatus:%d,GetRecharge:%lld,lVipRecharge:%lld,size:%d %d", GetUID(), iVipStatus, uPlayerVipStatus, GetRecharge(), lVipRecharge, mpVipProxyWeChatInfo.size(), msg.info_size());

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_VIP_RECHARGE_SHOW);

	return true;
}

bool	CPlayerBase::SendVipAliAccRecharge()
{
	int32  iVipStatus = CDataCfgMgr::Instance().GetVipAliAccRechargeStatus();
	int64  lVipRecharge = CDataCfgMgr::Instance().GetVipAliAccRecharge();
	auto & mpVipProxyWeChatInfo = CDataCfgMgr::Instance().GetVipAliAccInfo();

	uint32 uPlayerVipStatus = 1;

	if (iVipStatus == 1 || GetRecharge() < lVipRecharge)
	{
		uPlayerVipStatus = 1;
	}
	else if (iVipStatus == 0 && GetRecharge() >= lVipRecharge)
	{
		uPlayerVipStatus = 0;
	}
	else
	{
		uPlayerVipStatus = 1;
	}

	net::msg_notify_vip_recharge_show msg;
	msg.set_status(uPlayerVipStatus);

	tm	tmTime;
	memset(&tmTime, 0, sizeof(tmTime));
	getLocalTime(&tmTime, getSysTime());

	// shour:15, sminute:0, ehour:16, eminute:8
	// tmTime.tm_hour:15,, tmTime.tm_min:43

	// account:weixin003,tmTime.tm_hour:15,tmTime.tm_min:43,bIsShowWeChatInfo:1,timecontrol:1,shour:15, sminute:38, ehour:15, eminute:40,showtime:{"sh":15,"sm":38,"eh":15,"em":40}
	// account:weixin002, tmTime.tm_hour : 16, tmTime.tm_min : 3, bIsShowWeChatInfo : 0, timecontrol : 1, shour : 15, sminute : 0, ehour : 16, eminute : 8, showtime : {"sh":15, "sm" : 0, "eh" : 16, "em" : 8}, size : 3, tagInfo : 2 - 标题002 - weixin002]
	for (auto &iter : mpVipProxyWeChatInfo)
	{
		bool bIsShowWeChatInfo = false;
		tagVipRechargeWechatInfo tagInfo = iter.second;
		if (tagInfo.shour > 23 || tagInfo.ehour > 23 || tagInfo.sminute > 59 || tagInfo.eminute > 59)
		{
			bIsShowWeChatInfo = true;
		}
		else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour < tagInfo.ehour)
		{
			bIsShowWeChatInfo = true;
		}
		else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour == tagInfo.ehour)
		{
			//是否正在进行中
			if (tmTime.tm_min >= tagInfo.sminute && tmTime.tm_min <= tagInfo.eminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else if (tmTime.tm_hour == tagInfo.shour && tmTime.tm_hour < tagInfo.ehour)
		{
			//是否开始了
			if (tmTime.tm_min >= tagInfo.sminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else if (tmTime.tm_hour > tagInfo.shour && tmTime.tm_hour == tagInfo.ehour)
		{
			//是否结束了
			if (tmTime.tm_min <= tagInfo.eminute)
			{
				bIsShowWeChatInfo = true;
			}
			else
			{
				bIsShowWeChatInfo = false;
			}
		}
		else
		{
			bIsShowWeChatInfo = false;
		}
		if (tagInfo.timecontrol == false)
		{
			bIsShowWeChatInfo = true;
		}


		//bool bIsVipShow = false;
		//if (tagInfo.vecVip.size() == 0)
		//{
		//	bIsVipShow = true;
		//}
		//for (uint32 uIndex = 0; uIndex < tagInfo.vecVip.size(); uIndex++)
		//{
		//	if (!IsRobot())
		//	{
		//		LOG_DEBUG("uid:%d,uIndex:%d,vecVip:%d,GetVipLevel:%d", GetUID(), uIndex, tagInfo.vecVip[uIndex], GetVipLevel());
		//	}

		//	if (tagInfo.vecVip[uIndex] == GetVipLevel())
		//	{
		//		bIsVipShow = true;
		//		break;
		//	}
		//}
		bool bIsVipShow = true;
		if (bIsVipShow == false)
		{
			bIsShowWeChatInfo = false;
		}

		if (bIsShowWeChatInfo)
		{
			net::vip_recharge_wechatinfo * pwechatinfo = msg.add_info();

			pwechatinfo->set_sortid(tagInfo.sortid);
			pwechatinfo->set_title(tagInfo.title);
			pwechatinfo->set_account(tagInfo.account);
			pwechatinfo->set_low_amount(tagInfo.low_amount);

		}
		if (!IsRobot())
		{
			LOG_DEBUG("SendVipProxyRecharge  - uid:%d,tagInfo.account:%s,tmTime.tm_hour:%d,tmTime.tm_min:%d,bIsShowWeChatInfo:%d,timecontrol:%d,shour:%d, sminute:%d, ehour:%d, eminute:%d,showtime:%s,size:%d,tagInfo:%d - %s - %s", GetUID(), tagInfo.account.c_str(), tmTime.tm_hour, tmTime.tm_min, bIsShowWeChatInfo, tagInfo.timecontrol, tagInfo.shour, tagInfo.sminute, tagInfo.ehour, tagInfo.eminute, tagInfo.showtime.c_str(), msg.info_size(), tagInfo.sortid, tagInfo.title.c_str(), tagInfo.account.c_str());
		}
	}

	//for (uint32 i = 0; i < msg.info_size(); i++)
	//{

	//}
	if (!IsRobot())
	{
		LOG_DEBUG("SendVipProxyRecharge  - uid:%d,iVipStatus:%d,uPlayerVipStatus:%d,GetRecharge:%lld,lVipRecharge:%lld,size:%d %d", GetUID(), iVipStatus, uPlayerVipStatus, GetRecharge(), lVipRecharge, mpVipProxyWeChatInfo.size(), msg.info_size());
	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_VIP_ALIACC_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendUnionPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetUnionPayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetUnionPayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_unionpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_UNION_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendWeChatPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetWeChatPayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetWeChatPayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_wechatpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_WECHAT_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendAliPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetAliPayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetAliPayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_alipayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_ALI_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendOtherPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetOtherPayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetOtherPayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_otherpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_OTHER_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendQQPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetQQPayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetQQPayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_qqpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_QQ_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendWeChatScanPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetWeChatScanPayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetWeChatScanPayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_wechatscanpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_WECHAT_SCAN_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendJDPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetJDPayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetJDPayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_jdpayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_JD_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendApplePayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetApplePayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetApplePayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_applepayrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_APPLE_PAY_RECHARGE_REP);

	return true;
}

bool	CPlayerBase::SendLargeAliPayRecharge()
{
	if (IsRobot())
	{
		return true;
	}
	int32  iPayStatus = CDataCfgMgr::Instance().GetLargeAliPayRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetLargeAliPayRecharge();

	uint32 uPlayerPayStatus = 1;

	if (iPayStatus == 1 || GetRecharge() < lPayRecharge)
	{
		uPlayerPayStatus = 1;
	}
	else if (iPayStatus == 0 && GetRecharge() >= lPayRecharge)
	{
		uPlayerPayStatus = 0;
	}
	else
	{
		uPlayerPayStatus = 1;
	}

	net::msg_notify_large_ali_payrecharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	if (!IsRobot())
	{
		LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
			GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	}

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_LARGE_ALI_PAY_RECHARGE_REP);

	return true;
}

// 发送专享闪付信息
void CPlayerBase::SendExclusiveFlashRecharge() {
	if (IsRobot())
		return;

	int32  iPayStatus = CDataCfgMgr::Instance().GetExclusiveFlashRechargeStatus();
	int64  lPayRecharge = CDataCfgMgr::Instance().GetExclusiveFlashRecharge();

	uint32 uPlayerPayStatus = GetPlayerPayStatus(iPayStatus, lPayRecharge);

	net::msg_notify_exclusive_flash_recharge_show_rep msg;
	msg.set_status(uPlayerPayStatus);

	LOG_DEBUG("player_PayRecharge - uid:%d,iPayStatus:%d,uPlayerPayStatus:%d,GetRecharge:%lld,lPayRecharge:%lld",
		GetUID(), iPayStatus, uPlayerPayStatus, GetRecharge(), lPayRecharge);

	SendMsgToClient(&msg, net::S2C_MSG_NOTIFY_EXCLUSIVE_FLASH_RECHARGE_SHOW_REP);
}

void CPlayerBase::ClearVecWin(uint16 gameType)
{
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].vecwin.clear();
	}
}
void CPlayerBase::ClearVecBet(uint16 gameType)
{
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].vecbet.clear();
	}
}
void CPlayerBase::SetVecWin(uint16 gameType, int win)
{
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].vecwin.push_back(win);
		if (m_gameInfo[gameType].vecwin.size() > MAX_STATISTICS_GAME_COUNT)
		{
			m_gameInfo[gameType].vecwin.erase(m_gameInfo[gameType].vecwin.begin());
		}
	}
}
int CPlayerBase::GetVecWin(uint16 gameType)
{
	int count = 0;
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		for (uint32 i = 0; i < m_gameInfo[gameType].vecwin.size(); i++)
		{
			if (m_gameInfo[gameType].vecwin[i] == 1)
			{
				count++;
			}
		}
	}
	return count;
}
int CPlayerBase::GetBetCount(uint16 gameType)
{
	int count = 0;
	if (gameType < net::GAME_CATE_MAX_TYPE)
	{
		count = m_gameInfo[gameType].vecwin.size();
	}
	return count;
}

void CPlayerBase::SetVecBet(uint16 gameType, int64 score)
{
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		m_gameInfo[gameType].vecbet.push_back(score);
		if (m_gameInfo[gameType].vecbet.size() > MAX_STATISTICS_GAME_COUNT)
		{
			m_gameInfo[gameType].vecbet.erase(m_gameInfo[gameType].vecbet.begin());
		}
	}
}

int64 CPlayerBase::GetVecBet(uint16 gameType)
{
	int64 score = 0;
	if (gameType<net::GAME_CATE_MAX_TYPE)
	{
		for (uint32 i = 0; i < m_gameInfo[gameType].vecbet.size(); i++)
		{
			score += m_gameInfo[gameType].vecbet[i];
		}
	}
	return score;
}
