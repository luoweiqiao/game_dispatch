#include <data_cfg_mgr.h>

#include "game_player.h"
#include "helper/bufferStream.h"
#include "stdafx.h"
#include <time.h>
#include "game_table.h"
#include "game_room.h"

using namespace svrlib;
using namespace std;
using namespace Network;

namespace 
{	
	
}
CGamePlayer::CGamePlayer(uint8 type)
:CPlayerBase(type)
{
	m_pGameTable 	 = NULL;
	m_pGameRoom  	 = NULL;
	m_msgHeartTime   = getSysTime();
	m_autoReady		 = false;
	m_playDisconnect = false;
	m_belongLobbyID  = 0;
	m_ctrl_flag		 = false;
	m_lucky_read	 = false;
}
CGamePlayer::~CGamePlayer()
{

}
bool  	CGamePlayer::SendMsgToClient(const google::protobuf::Message* msg,uint16 msg_type,bool bSendRobot)
{
	if(IsRobot() && !bSendRobot)
		return false;

	if(IsPlayDisconnect() && msg_type > 3000)
	{
		LOG_DEBUG("断线重连未完成不发游戏消息%d--%d",GetUID(),msg_type);
		return false;
	}

    CLobbyMgr::Instance().SendMsg2Client(msg,msg_type,GetUID());
    return true;
}    
void	CGamePlayer::OnLoginOut(uint32 leaveparam)
{
	LOG_DEBUG("GamePlayer OnLoginOut - uid:%d",GetUID());
	SetPlayerState(PLAYER_STATE_LOGINOUT);
	if(m_pGameTable != NULL){
		m_pGameTable->LeaveTable(this);
	}
	if(m_pGameRoom != NULL){
		m_pGameRoom->LeaveRoom(this);
	}
	FlushOnlineSvrIDToRedis(0);
	NotifyLeaveGame(leaveparam);

	if(!IsRobot())
	{
		CRedisMgr::Instance().SavePlayerBlockers(GetUID(), m_blocks);
		CRedisMgr::Instance().SavePlayerBlockerIPs(GetUID(), m_blockIPs);
	}
}
void	CGamePlayer::OnLogin()
{	
	SetNetState(1);
	FlushOnlineSvrIDToRedis(CApplication::Instance().GetServerID());

	if(!IsRobot())
	{
		CRedisMgr::Instance().LoadPlayerBlockers(GetUID(), m_blocks);
		CRedisMgr::Instance().LoadPlayerBlockerIPs(GetUID(), m_blockIPs);
	}

	//读取幸运值配置
	if (!m_lucky_read)
	{
		GetLuckyCfg();
	}
}
void	CGamePlayer::ReLogin()
{
	SetNetState(1);
	FlushOnlineSvrIDToRedis(CApplication::Instance().GetServerID());
}
void 	CGamePlayer::OnGameEnd()
{

}
// 更新游戏服务器ID到redis
void 	CGamePlayer::FlushOnlineSvrIDToRedis(uint16 svrID)
{
	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}
	CRedisMgr::Instance().SetPlayerOnlineSvrID(GetUID(),svrID);
	CRedisMgr::Instance().UpdatePlayerOnlineGameSvrID(GetUID(), svrID);
}
// 通知返回大厅(退出游戏)
void	CGamePlayer::NotifyLeaveGame(uint16 gotoSvrid)
{
	LOG_DEBUG("notify leave game:%d->%d",GetUID(),gotoSvrid);

	if (CDataCfgMgr::Instance().GetCurSvrCfg().gameType == net::GAME_CATE_EVERYCOLOR)
	{
		return;
	}

	net::msg_leave_svr msg;
	msg.set_uid(GetUID());
	msg.set_goto_svr(gotoSvrid);

	SendMsgToClient(&msg,S2L_MSG_LEAVE_SVR,true);

	if(gotoSvrid == 0){
		net::msg_back_lobby_rep rep;
		rep.set_result(1);
		SendMsgToClient(&rep, net::S2C_MSG_BACK_LOBBY_REP,true);
	}
}
// 请求大厅修改数值
void	CGamePlayer::NotifyLobbyChangeAccValue(int32 operType,int32 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safeCoin,const string& chessid)
{
	CLobbyMgr::Instance().NotifyLobbyChangeAccValue(GetUID(),operType,subType,diamond,coin,ingot,score,cvalue,safeCoin,chessid);
}
// 修改玩家账号数值（增量修改）
bool    CGamePlayer::SyncChangeAccountValue(uint16 operType,uint16 subType,int64 diamond,int64 coin,int64 ingot,int64 score,int32 cvalue,int64 safecoin,const string& chessid)
{
	diamond = ChangeAccountValue(emACC_VALUE_DIAMOND,diamond);
	coin    = ChangeAccountValue(emACC_VALUE_COIN,coin);
	score   = ChangeAccountValue(emACC_VALUE_SCORE,score);
	ingot   = ChangeAccountValue(emACC_VALUE_INGOT,ingot);
	cvalue  = ChangeAccountValue(emACC_VALUE_CVALUE,cvalue);
	safecoin = ChangeAccountValue(emACC_VALUE_SAFECOIN,safecoin);
	NotifyLobbyChangeAccValue(operType,subType,diamond,coin,ingot,score,cvalue,safecoin,chessid);
	return true;
}
// 修改玩家账号数值（减少修改）
void    CGamePlayer::SubChangeAccountValue(uint16 operType, uint16 subType, int64 diamond, int64 coin, int64 ingot, int64 score, int32 cvalue, int64 safecoin, const string& chessid)
{
	diamond = ChangeAccountValue(emACC_VALUE_DIAMOND, diamond);
	coin = ChangeAccountValue(emACC_VALUE_COIN, coin);
	score = ChangeAccountValue(emACC_VALUE_SCORE, score);
	ingot = ChangeAccountValue(emACC_VALUE_INGOT, ingot);
	cvalue = ChangeAccountValue(emACC_VALUE_CVALUE, cvalue);
	safecoin = ChangeAccountValue(emACC_VALUE_SAFECOIN, safecoin);

	CLobbyMgr::Instance().UpDateLobbyChangeAccValue(GetUID(), operType, subType, diamond, coin, ingot, score, cvalue, safecoin, chessid);

}
// 能否退出
bool 	CGamePlayer::CanBackLobby()
{
	if(m_pGameTable != NULL	&& !m_pGameTable->CanLeaveTable(this))
	{
		return false;
	}

	return true;
}
int 	CGamePlayer::OnGameMessage(uint16 cmdID, const uint8* pkt_buf, uint16 buf_len)
{
	if(m_pGameTable != NULL)
	{
		m_pGameTable->OnGameMessage(this,cmdID,pkt_buf,buf_len);
	}
	ResetHeart();
	return 0;
}
// 重置心跳时间
void	CGamePlayer::ResetHeart()
{
	m_msgHeartTime = getSysTime();
}
void  	CGamePlayer::OnTimeTick(uint64 uTime,bool bNewDay)
{
	if((getSysTime() - m_msgHeartTime) > SECONDS_IN_MIN*60)
	{// 10分钟没有收到消息算掉线
		if(m_pGameTable == NULL || m_pGameTable->CanLeaveTable(this)) {
			SetNetState(0);
		}
	}
}
// 是否需要回收
bool 	CGamePlayer::NeedRecover()
{
	if(GetNetState() == 0 || CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR){
		if(CanBackLobby())
			return true;
	}
	return false;
}
// 是否正在游戏中
bool 	CGamePlayer::IsInGamePlaying()
{
	if(m_pGameTable == NULL)
		return false;
	if(m_pGameTable->GetGameState() != TABLE_STATE_FREE || m_pGameTable->IsReady(this))
		return true;

	return false;
}
// 更新数值到桌子
void 	CGamePlayer::FlushAccValue2Table()
{
	if(m_pGameTable != NULL){
		m_pGameTable->SendSeatInfoToClient(NULL);
	}
}
// 修改游戏数值并更新到数据库
void 	CGamePlayer::AsyncChangeGameValue(uint16 gameType,bool isCoin,int64 winScore, int64 lExWinScore)
{
    int32 lose = (winScore > 0) ? 0 : 1;
    int32 win = (winScore > 0) ? 1 : 0;
    ChangeGameValue(gameType,isCoin, win, lose, winScore);
    CDBMysqlMgr::Instance().ChangeGameValue(gameType,GetUID(), isCoin, win, lose,winScore, lExWinScore,GetGameMaxScore(gameType,isCoin));
}
void    CGamePlayer::AsyncSetGameMaxCard(uint16 gameType,bool isCoin,uint8 cardData[],uint8 cardCount)
{
    SetGameMaxCard(gameType,isCoin,cardData,cardCount);
    CDBMysqlMgr::Instance().UpdateGameMaxCard(gameType,GetUID(),isCoin,cardData,cardCount);        
}
uint8	CGamePlayer::GetNetState()
{
	return m_netState;
}
void	CGamePlayer::SetNetState(uint8 state)
{
	m_netState = state;
	if(m_pGameTable != NULL)
	{
		m_pGameTable->OnActionUserNetState(this,m_netState,false);
		if(state == 0)
		{
			if(m_pGameTable->CanLeaveTable(this)){
				if(m_pGameTable->LeaveTable(this)){
					m_pGameRoom->LeaveRoom(this);
				}
			}
		}
	}else{
		SetPlayDisconnect(false);
	}
	//LOG_DEBUG("设置网络状态:uid:%d--%d",GetUID(),state);
}
uint16  CGamePlayer::GetRoomID()
{
	if(m_pGameRoom != NULL){
		return m_pGameRoom->GetRoomID();
	}
	return 0;
}
void	CGamePlayer::SetTable(CGameTable* pTable)
{
	m_pGameTable = pTable;
	if(m_pGameTable == NULL){
		SetPlayDisconnect(false);
	}
}
uint32  CGamePlayer::GetTableID()
{
	if(m_pGameTable != NULL){
		return m_pGameTable->GetTableID();
	}
	return 0;
}
void 	CGamePlayer::AddBlocker(uint32 uid) {
	if(uid != 0 && uid != GetUID())
	{
		m_blocks.push_back(uid);
		//LOG_DEBUG("添加黑名单:%d-->%d",GetUID(),uid);
	}
}
bool 	CGamePlayer::IsExistBlocker(uint32 uid)
{
	if(uid == 0)
		return false;
	for(uint32 i=0;i<m_blocks.size();++i){
		if(m_blocks[i] == uid)
			return true;
	}
	return false;
}
void 	CGamePlayer::ClearBlocker()
{
	m_blocks.clear();
	//LOG_DEBUG("清理黑名单:%d",GetUID());
}
void 	CGamePlayer::AddBlockerIP(uint32 ip)
{
	//return;
	if(ip != 0 && ip != GetIP()){
		m_blockIPs.push_back(ip);
		//LOG_DEBUG("添加黑名单IP:%d-->%d",GetUID(),ip);
	}
}
bool 	CGamePlayer::IsExistBlockerIP(uint32 ip)
{
	if(ip == 0)
		return false;
	for(uint32 i=0;i<m_blockIPs.size();++i){
		if(m_blockIPs[i] == ip)
			return true;
	}
	return false;
}
void 	CGamePlayer::ClearBlockerIP()
{
	m_blockIPs.clear();
	//LOG_DEBUG("清理黑名单IP:%d",GetUID());
}

// 修改游戏玩家库存并更新到数据库
void 	CGamePlayer::AsyncChangeStockScore(uint16 gametype, int64 winScore)
{
	ChangeStockScore(gametype,winScore);
	CDBMysqlMgr::Instance().ChangeStockScore(gametype, GetUID(), winScore);
}

// 读取当前玩家的幸运值配置信息
void	CGamePlayer::GetLuckyCfg()
{
	uint16 gametype = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
	if (!CCommonLogic::IsLuckyFuncitonGame(gametype))
	{
		LOG_ERROR("The current gametype is not deal lucky function. - uid:%d gametype:%d.", GetUID(), gametype);
		return;
	}
	CDataCfgMgr::Instance().LoadLuckyConfig(GetUID(), gametype, m_lucky_info);
	LOG_DEBUG("Get current player uid:%d lucky info. gametype:%d result size:%d.", GetUID(), gametype, m_lucky_info.size());
	m_lucky_read = true;
	return;
}

//根据roomid获取当前玩家是否触发幸运值---针对非捕鱼
bool	CGamePlayer::GetLuckyFlag(uint16 roomid, bool &flag)
{
	auto iter_exist = m_lucky_info.find(roomid);
	if (iter_exist == m_lucky_info.end())
	{
		LOG_DEBUG("the roomid is not exist - uid:%d roomid:%d", GetUID(), roomid);
		return false;
	}
	else
	{
		//判断幸运值是否有设置 0:表示未设置
		if (iter_exist->second.lucky_value == 0)
		{
			LOG_DEBUG("the lucky_value is zero - uid:%d", GetUID());
			return false;
		}
		else
		{
			if (g_RandGen.RandRatio(iter_exist->second.rate, PRO_DENO_100))
			{
				//根据幸运值来判断当前控制的赢或输
				if (iter_exist->second.lucky_value > 0)
				{
					flag = true;
				}
				else
				{
					flag = false;
				}
				LOG_DEBUG("get player lucky is success. uid:%d flag:%d", GetUID(), flag);
				return true;
			}
			else
			{
				LOG_DEBUG("the rand is fail. uid:%d rate:%d", GetUID(), iter_exist->second.rate);
				return false;
			}
		}
	}
	return false;
}

//设置当前玩家的幸运值统计信息
void	CGamePlayer::SetLuckyInfo(uint16 roomid, int32 add_value)
{
	LOG_DEBUG("update lucky info. uid:%d roomid:%d add_value:%d", GetUID(), roomid, add_value);
	if (add_value == 0)
	{
		LOG_DEBUG("the add_value is zero.");
		return;
	}
	auto iter_exist = m_lucky_info.find(roomid);
	if (iter_exist == m_lucky_info.end())
	{
		LOG_DEBUG("the roomid is not exist - roomid:%d", roomid);
		return;
	}
	else
	{
		//判断幸运值是否有设置 0:表示未设置
		if (iter_exist->second.lucky_value == 0)
		{
			LOG_DEBUG("the lucky_value is zero.");
			return;
		}

		//如果控制当前玩家为赢，则只记录赢的牌局信息
		if (iter_exist->second.lucky_value > 0 && add_value < 0)
		{
			LOG_DEBUG("the set lucky player is win but current result is lose.");
			return;
		}

		//如果控制当前玩家为输，则只记录输的牌局信息
		if (iter_exist->second.lucky_value < 0 && add_value > 0)
		{
			LOG_DEBUG("the set lucky player is lose but current result is win.");
			return;
		}

		//取更新前的幸运值
		int64 before_lucky = iter_exist->second.lucky_value;

		//更新累计值与幸运值
		iter_exist->second.accumulated += add_value;
		iter_exist->second.lucky_value -= add_value;

		//赢的情况---完成条件
		if (before_lucky > 0 && iter_exist->second.lucky_value <= 0)
		{
			iter_exist->second.lucky_value = 0;
			iter_exist->second.rate = 0;
		}

		//输的情况---完成条件
		if (before_lucky < 0 && iter_exist->second.lucky_value >= 0)
		{
			iter_exist->second.lucky_value = 0;
			iter_exist->second.rate = 0;
		}

		LOG_DEBUG("update lucky info after. uid:%d roomid:%d lucky_value:%d rate:%d accumulated:%d",
			GetUID(), roomid, iter_exist->second.lucky_value, iter_exist->second.rate, iter_exist->second.accumulated);

		//更新到数据库
		uint16 gametype = CDataCfgMgr::Instance().GetCurSvrCfg().gameType;
		CDataCfgMgr::Instance().UpdateLuckyInfo(GetUID(), gametype, roomid, iter_exist->second);
	}
	return;
}

//根据roomid获取当前玩家是否触发幸运值---针对捕鱼
bool	CGamePlayer::GetLuckyFlagForFish(uint16 roomid, bool &flag, uint32 &rate)
{
	auto iter_exist = m_lucky_info.find(roomid);
	if (iter_exist == m_lucky_info.end())
	{
		LOG_DEBUG("the roomid is not exist - uid:%d roomid:%d", GetUID(), roomid);
		return false;
	}
	else
	{
		//判断幸运值是否有设置 0:表示未设置
		if (iter_exist->second.lucky_value == 0)
		{
			LOG_DEBUG("the lucky_value is zero - uid:%d", GetUID());
			return false;
		}
		else
		{
			//根据幸运值来判断当前控制的赢或输
			if (iter_exist->second.lucky_value > 0)
			{
				flag = true;
			}
			else
			{
				flag = false;
			}
			rate = iter_exist->second.rate;
			LOG_DEBUG("get player lucky is success. uid:%d flag:%d rate:%d", GetUID(), flag, rate);
			return true;
		}
	}
	return false;
}

//重置当前玩家的幸运值信息---每天凌晨
void	CGamePlayer::ResetLuckyInfo()
{
	LOG_DEBUG("reset lucky info. uid:%d", GetUID());

	map<uint8, tagUserLuckyCfg>::iterator iter = m_lucky_info.begin();
	for (; iter != m_lucky_info.end(); iter++)
	{
		iter->second.lucky_value = 0;
		iter->second.accumulated = 0;
		iter->second.rate = 0;
	}
	return;
}

