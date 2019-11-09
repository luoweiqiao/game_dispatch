

#include "server_mgr.h"
#include "pb/msg_define.pb.h"
#include "msg_server_handle.h"
#include "player_base.h"
#include "player_mgr.h"
#include "player.h"

using namespace svrlib;
using namespace Network;

namespace
{

}
CServerMgr::CServerMgr()
{
	m_mpServers.clear();
	m_bTimeStopSvr   = 0;
	m_eTimeStopSvr   = 0;
	m_stopSvrContent = "";
	m_stopSvrs.clear();
	m_pTimer 	     = NULL;

}
CServerMgr::~CServerMgr()
{
}
void  CServerMgr::OnTimer(uint8 eventID)
{
	CheckRepairServer();
}
bool  CServerMgr::Init()
{
	SetMsgSinker(new CHandleServerMsg());
	m_pTimer = CApplication::Instance().MallocTimer(this,1);
	m_pTimer->StartTimer(5000,5000);
    return true;
}
void  CServerMgr::ShutDown()
{
	CApplication::Instance().FreeTimer(m_pTimer);
}
bool  CServerMgr::AddServer(NetworkObject* pNetObj,uint16 svrID,uint16 gameType,uint8 gameSubType,uint8 openRobot)
{	
	stGServer server;
	server.svrID	    = svrID;
	server.gameType     = gameType;
    server.gameSubType  = gameSubType;
    server.openRobot    = openRobot;
	server.pNetObj      = pNetObj;
	pNetObj->SetUID(svrID);

	SyncPlayerCurSvrID(svrID);
	
	//if (GetServerBySvrID(svrID) != NULL)
	//{
	//	return true;
	//}

	pair< stl_hash_map<uint32, stGServer>::iterator, bool > ret;

	ret = m_mpServers.insert(make_pair(svrID, server));
	bool bretvalue =  ret.second;
	LOG_DEBUG("addserver - ip:%s,port:%d,svrID:%d,gameType:%d,gameSubType:%d,openRobot:%d,ret:%d", pNetObj->GetSIP().c_str(), pNetObj->GetPort(), svrID, gameType, gameSubType, openRobot, bretvalue);
	SendSvrsInfoToAll();
	return bretvalue;
}

void   CServerMgr::UpdateOpenRobot(uint16 svrID, uint8 openRobot)
{
	LOG_DEBUG("1 open_robot - svrID:%d,openRobot:%d,map_openRobot:%d", svrID, openRobot, m_mpServers.size());

	MAP_SERVERS::iterator it = m_mpServers.find(svrID);
	if (it != m_mpServers.end())
	{
		//stGServer & server = it->second;
		//server.openRobot = openRobot;
		it->second.openRobot = openRobot;
		LOG_DEBUG("2 open_robot - svrID:%d,openRobot:%d,map_openRobot:%d", svrID, openRobot, it->second.openRobot);
	}

}

void  CServerMgr::RemoveServer(NetworkObject* pNetObj)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.pNetObj == pNetObj)
		{
			LOG_DEBUG("removeserver - svrID:%d", server.svrID);

			m_mpServers.erase(it);
			pNetObj->SetUID(0);
			SendSvrsInfoToAll();
			break;
		}
	}
}
stGServer* CServerMgr::GetServerBySocket(NetworkObject* pNetObj)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.pNetObj == pNetObj)
		{
			return &it->second;
		}
	}
	return NULL;
}
stGServer* CServerMgr::GetServerBySvrID(uint16 svrID)
{
	MAP_SERVERS::iterator it = m_mpServers.find(svrID);
	if(it != m_mpServers.end())
	{
		return &it->second;
	}
	return NULL;
}
void   CServerMgr::SendMsg2Server(uint16 svrID,const google::protobuf::Message* msg,uint16 msg_type,uint32 uin)
{
	stGServer* pServer = GetServerBySvrID(svrID);
	if(pServer == NULL) {
		LOG_DEBUG("send msg server is not exists - serverid:%d",svrID);
		return;
	}
	SendProtobufMsg(pServer->pNetObj,msg,msg_type,uin);
}
void   CServerMgr::SendMsg2AllServer(const google::protobuf::Message* msg,uint16 msg_type,uint32 uin)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		SendProtobufMsg(server.pNetObj,msg,msg_type,uin);
	}
}
void   CServerMgr::SendMsg2Server(uint16 svrID,const uint8* pkt_buf, uint16 buf_len,uint16 msg_type,uint32 uin)
{
	stGServer* pServer = GetServerBySvrID(svrID);
	if(pServer == NULL) {
		LOG_DEBUG("send serverid is not exist - serverid:%d",svrID);
		return;
	}
	SendProtobufMsg(pServer->pNetObj,pkt_buf,buf_len,msg_type,uin);
}
void   CServerMgr::SendMsg2AllServer(const uint8* pkt_buf, uint16 buf_len,uint16 msg_type,uint32 uin)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		SendProtobufMsg(server.pNetObj,pkt_buf,buf_len,msg_type,uin);
	}
}
// 获取服务器列表
void   CServerMgr::SendSvrsInfo2Client(uint32 uid)
{
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	if(pPlayer == NULL)
		return;
	net::msg_svrs_info_rep info;
	info.set_cur_svrid(pPlayer->GetCurSvrID());

	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		net::svr_info *pSvr = info.add_svrs();
		pSvr->set_svrid(server.svrID);
		pSvr->set_game_type(server.gameType);
        pSvr->set_game_subtype(server.gameSubType);
		pSvr->set_state(server.status);

		net::player_num *pNum = info.add_num();
		pNum->set_svrid(server.svrID);
		pNum->set_players(server.playerNum);
    	pNum->set_robots(server.robotNum);

		LOG_DEBUG("send_serverlist ing - uid:%d,size:%02d,svrID:%03d,gametype:%02d,gameSubType:%d,status:%d,playerNum:%d,robotNum:%d",
			uid, info.svrs_size(), server.svrID, server.gameType, server.gameSubType, server.status, server.playerNum, server.robotNum);

	}
	pPlayer->SendMsgToClient(&info,net::S2C_MSG_SVRS_INFO);
	LOG_DEBUG("send_serverlist end - uid:%d,size:%d",uid,info.svrs_size());
}
void   CServerMgr::SendSvrsPlayInfo2Client(uint32 uid)
{
	CPlayer* pPlayer = (CPlayer*)CPlayerMgr::Instance().GetPlayer(uid);
	if(pPlayer == NULL)
		return;
	net::msg_send_server_info info;	
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		net::server_info *pSvr = info.add_servers();
		pSvr->set_svrid(server.svrID);
		pSvr->set_player_num(server.playerNum);
        pSvr->set_robot_num(server.robotNum);
        pSvr->set_game_type(server.gameType);
		
	}
	pPlayer->SendMsgToClient(&info,net::S2C_MSG_SEND_SERVER_INFO);     
}
void   CServerMgr::SendSvrsInfoToAll()
{
	vector<CPlayerBase*> vecPlayers;
	CPlayerMgr::Instance().GetAllPlayers(vecPlayers);
	for(uint32 i=0;i<vecPlayers.size();++i)
	{
		CPlayer* pPlayer = (CPlayer*)vecPlayers[i];
		SendSvrsInfo2Client(pPlayer->GetUID());
	}
	vecPlayers.clear();
}
// 发送维护公告
void   CServerMgr::SendSvrRepairContent(NetworkObject* pNetObj)
{
	if(pNetObj == NULL)
		return;
	if(IsNeedRepair(CApplication::Instance().GetServerID()))
	{	
    	net::msg_system_broadcast_rep broad;
    	broad.set_msg(m_stopSvrContent);
    	SendProtobufMsg(pNetObj,&broad,net::S2C_MSG_SYSTEM_BROADCAST,0);
	}
}
// 发送停服广播
void   CServerMgr::SendSvrRepairAll()
{
    net::msg_system_broadcast_rep broad;
	broad.set_msg(m_stopSvrContent);    
    if(CApplication::Instance().GetStatus() == emSERVER_STATE_REPAIR || IsNeedRepair(CApplication::Instance().GetServerID())){
        CPlayerMgr::Instance().SendMsgToAll(&broad,net::S2C_MSG_SYSTEM_BROADCAST); 
        return;
    }    
    vector<CPlayerBase*> vecPlayers;
	CPlayerMgr::Instance().GetAllPlayers(vecPlayers);
	for(uint32 i=0;i<vecPlayers.size();++i)
	{
		CPlayer* pPlayer = (CPlayer*)vecPlayers[i];
        uint16 curSvrID = pPlayer->GetCurSvrID();
        if(IsNeedRepair(curSvrID))
        {
            pPlayer->SendMsgToClient(&broad,net::S2C_MSG_SYSTEM_BROADCAST);
        }                       
	}
	vecPlayers.clear();        
}    
bool   CServerMgr::IsNeedRepair(uint16 svrID)
{
    for(uint32 i=0;i<m_stopSvrs.size();++i){
        if(m_stopSvrs[i] == svrID){
            return true;
        }        
    }      
    return false;
}

uint16 CServerMgr::GetGameTypeSvrID(uint32 gameType)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.gameType == gameType)
			return server.svrID;
	}
	return 0;
}
bool   CServerMgr::IsOpenRobot(uint32 gameType)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.gameType == gameType){
			return server.openRobot == 1;
		}
	}
	return false;
}
// 服务器重连检测玩家所在服务器状态
void   CServerMgr::SyncPlayerCurSvrID(uint16 svrID)
{
	vector<CPlayerBase*> vecPlayers;
	CPlayerMgr::Instance().GetAllPlayers(vecPlayers);
	for(uint32 i=0;i<vecPlayers.size();++i)
	{
		CPlayer* pPlayer = (CPlayer*)vecPlayers[i];
		pPlayer->SyncCurSvrIDFromRedis(svrID);
	}
	vecPlayers.clear();
}
// 停服通知
void   CServerMgr::NotifyStopService(uint32 btime,uint32 etime,vector<uint16>& svrids,string content)
{
	m_bTimeStopSvr 	 = btime;
	m_eTimeStopSvr 	 = etime;
	m_stopSvrContent = content;
	m_stopSvrs       = svrids;
}
// 获得机器人服务器ID
uint16 CServerMgr::GetRobotSvrID(CLobbyRobot* pRobot)
{
    vector<stGServer*> vecSvrs;
    MAP_SERVERS::iterator it = m_mpServers.begin();
	for(;it != m_mpServers.end();++it)
	{
		stGServer& server = it->second;
		if(server.openRobot == 1 && server.status == emSERVER_STATE_NORMAL && pRobot->CanEnterGameType(server.gameType))
		{
            vecSvrs.push_back(&server);
		}
	}
    if(vecSvrs.size() > 0){
        sort(vecSvrs.begin(),vecSvrs.end(),CompareServerRobotNum);
        return vecSvrs[0]->svrID;
    }    
	return 0;       
}    
// 服务器机器人数量排序
bool   CServerMgr::CompareServerRobotNum(stGServer* pSvr1,stGServer* pSvr2)
{
    return (pSvr1->playerNum+pSvr1->robotNum) < (pSvr2->playerNum+pSvr2->robotNum);//总人数少的优先分配机器人
}
void   CServerMgr::CheckRepairServer()
{
	if(m_bTimeStopSvr == 0)
		return;
	uint32 curTime = getSysTime(); 
	if(!m_stopSvrs.empty())
	{
		if(m_bTimeStopSvr < curTime)
		{
            LOG_DEBUG("维护时间到了，通知服务器开启维护状态");
            bool isRepairLobby = false;
            for(uint32 i=0;i<m_stopSvrs.size();++i){
                if(m_stopSvrs[i] == CApplication::Instance().GetServerID()){
                    isRepairLobby = true;
                }
            }
            net::msg_notify_stop_service msg;
		    msg.set_btime(m_bTimeStopSvr);
		    msg.set_etime(m_eTimeStopSvr); 
            if(isRepairLobby)
            {
        		CApplication::Instance().SetStatus(emSERVER_STATE_REPAIR);
                MAP_SERVERS::iterator it = m_mpServers.begin();
            	for(;it != m_mpServers.end();++it)
            	{
            		stGServer& server = it->second;
            		server.status = emSERVER_STATE_REPAIR;
            	}
                SendMsg2AllServer(&msg,net::L2S_MSG_NOTIFY_STOP_SERVICE,0);
            }else{            
    			for(uint32 i=0;i<m_stopSvrs.size();++i)
    			{
    				uint16 svrid = m_stopSvrs[i];
    				stGServer* pServer = GetServerBySvrID(svrid);
    				if(pServer != NULL){
    					pServer->status = emSERVER_STATE_REPAIR;    					
    					SendMsg2Server(svrid,&msg,net::L2S_MSG_NOTIFY_STOP_SERVICE,0);
    				}                    
    			}
            }
			m_stopSvrs.clear();
            SendSvrsInfoToAll();
		}else{
			if(m_coolBroad.isTimeOut()){
                LOG_DEBUG("发送维护广播:%d",curTime);
                SendSvrRepairAll();
                
				uint32 diffTime = m_bTimeStopSvr - curTime;
				if(diffTime < SECONDS_IN_MIN*5){
					m_coolBroad.beginCooling(SECONDS_IN_MIN*1000);
				}else{
					m_coolBroad.beginCooling((m_eTimeStopSvr - curTime - SECONDS_IN_MIN)*1000);
				}
			}
		}
	}
	// 检测是否维护完毕
    if(curTime > m_bTimeStopSvr && curTime < m_eTimeStopSvr)
    {
    	bool isAllOver = true;
    	MAP_SERVERS::iterator it = m_mpServers.begin();
    	for(;it != m_mpServers.end();++it)
    	{
    		stGServer& server = it->second;
    		if(server.status == emSERVER_STATE_REPAIR)
    		{
    			isAllOver = false;
    			break;
    		}
    	}
    	if(isAllOver){
            LOG_DEBUG("维护结束");
    		m_stopSvrContent = "";
    		m_bTimeStopSvr   = 0;
    		m_eTimeStopSvr   = 0;
    	}
    }
}

void CServerMgr::GetGameTypeSvrID(uint32 gametype,vector<uint16> & vecServerID)
{
	vecServerID.clear();

	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		if (server.gameType == gametype)
		{
			vecServerID.push_back(server.svrID);
		}
	}
}

void	CServerMgr::ChangeRoomParam(uint32 gametype, uint32 roomid, string param) {
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_change_room_param msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_param(param.c_str());
	vector<uint16> vecServerID;
	GetGameTypeSvrID(gametype, vecServerID);
	for (uint32 i = 0; i < vecServerID.size(); i++)
	{
		svrid = vecServerID[i];
		SendMsg2Server(svrid, &msg, net::L2S_MSG_CHANGE_ROOM_PARAM, 0);
	}	
}

void	CServerMgr::ChangeContorlPlayer(uint32 gametype,uint32 roomid, uint32 uid, uint32 operatetype, uint32 gamecount) {
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_contorl_player msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_uid(uid);
	msg.set_operatetype(operatetype);
	msg.set_gamecount(gamecount);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_CONTORL_PLAYER, 0);
}

void	CServerMgr::ChangeContorlMultiPlayer(uint32 gametype, uint32 roomid, uint32 uid, uint32 operatetype, uint32 gamecount, uint64 gametime, int64 totalscore) {
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_contorl_multi_player msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_uid(uid);
	msg.set_operatetype(operatetype);
	msg.set_gamecount(gamecount);
	msg.set_gametime(gametime);
	msg.set_totalscore(totalscore);

	SendMsg2Server(svrid, &msg, net::L2S_MSG_CONTORL_MULTI_PLAYER, 0);
}

void	CServerMgr::StopSnatchCoin(uint32 gametype, uint32 roomid, uint32 stop) {
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_stop_snatch_coin msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_stop(stop);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_STOP_SNATCH_COIN, 0);
}
void CServerMgr::SetSvrStatus(uint16 svrID, uint8 status)
{
	MAP_SERVERS::iterator iter = m_mpServers.find(svrID);
	if(iter == m_mpServers.end())
	{
		LOG_ERROR("cant find this svr:%d", svrID);
		return;
	}

	stGServer &server = iter->second;
	if(server.status != emSERVER_STATE_NORMAL)
	{
		LOG_ERROR("gamesvr has retired or repair, status:%d", server.status);
		return;
	}

	server.status = emSERVER_STATE_RETIRE;
}

void	CServerMgr::ContorlDiceGameCard(uint32 gametype, uint32 roomid, uint32 uDice[])
{
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_dice_control_req msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	net::dice_control_req* pdice_control_req = msg.mutable_dice();

	for (int i = 0; i < 3; i++)
	{
		pdice_control_req->add_table_cards(uDice[i]);
	}

	SendMsg2Server(svrid, &msg, net::L2S_MSG_DICE_CONTROL_REQ, 0);
}



void	CServerMgr::ConfigMajiangHandCard(uint32 gametype, uint32 roomid, string strHandCard)
{
	uint16 svrid = GetGameTypeSvrID(gametype);
	net::msg_majiang_config_hand_card msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_hand_card(strHandCard.c_str());

	SendMsg2Server(svrid, &msg, net::L2S_MSG_MAJIANG_CONFIG_HAND_CARD, 0);
}

// 通知游戏服修改房间库存配置  add by har
bool CServerMgr::NotifyGameSvrsChangeRoomStockCfg(uint32 gameType, stStockCfg &st) {
	uint16 svrid = GetGameTypeSvrID(gameType);
	if (svrid == 0)
		return false;
	net::msg_change_room_stock_cfg msg;
	msg.set_roomid(st.roomID);
	msg.set_stock_max(st.stockMax);
	msg.set_stock_conversion_rate(st.stockConversionRate);
	msg.set_jackpot_min(st.jackpotMin);
	msg.set_jackpot_max_rate(st.jackpotMaxRate);
	msg.set_jackpot_rate(st.jackpotRate);
	msg.set_jackpot_coefficient(st.jackpotCoefficient);
	msg.set_jackpot_extract_rate(st.jackpotExtractRate);
	msg.set_add_stock(st.stock);
	msg.set_kill_points_line(st.killPointsLine);
	msg.set_player_win_rate(st.playerWinRate);
	msg.set_add_jackpot(st.jackpot);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_CHANGE_ROOM_STOCK_CFG, 0);
	return true;
}


void	CServerMgr::UpdateServerRoomRobot(int gametype, int roomid, int robot)
{
	uint16 svrid = GetGameTypeSvrID(gametype);

	LOG_DEBUG("gametype:%d,svrid:%d, roomid:%d, robot:%d", gametype, svrid, roomid, robot);

	net::msg_update_server_room_robot msg;
	msg.set_gametype(gametype);
	msg.set_roomid(roomid);
	msg.set_robot(robot);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_UPDATE_SERVER_ROOM_ROBOT, 0);
}

void	CServerMgr::UpdateRobotOnlineCfg(stRobotOnlineCfg & refCfg)
{
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

	net::msg_update_robot_online_cfg msg;
	msg.set_batchid(refCfg.batchID);
	msg.set_gametype(refCfg.gameType);
	msg.set_roomid(refCfg.roomID);
	msg.set_leveltype(refCfg.leveltype);
	msg.set_entertime(refCfg.enterTime);
	msg.set_leavetime(refCfg.leaveTime);
	
	for (uint8 lv = 0; lv < ROBOT_MAX_LEVEL; ++lv)
	{
		uint32 rcount = refCfg.onlines[lv];
		msg.add_onlines(rcount);
	}

	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, &msg, net::L2S_MSG_UPDATE_ROBOT_OBLINE_CFG, 0);

		LOG_DEBUG("gametype:%d,svrid:%d", refCfg.gameType, server.svrID);
	}
}

void	CServerMgr::DeleteRobotOnlineCfg(int gametype, int batchid)
{

	LOG_DEBUG("gametype:%d,batchid:%d", gametype, batchid);

	net::msg_delete_robot_online_cfg msg;
	msg.set_gametype(gametype);
	msg.set_batchid(batchid);

	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, &msg, net::L2S_MSG_DELETE_ROBOT_OBLINE_CFG, 0);

		LOG_DEBUG("gametype:%d,batchid:%d,svrid:%d", gametype, batchid, server.svrID);
	}
}

void	CServerMgr::StopContrlPlayer(uint16 svrid, uint32 uid)
{
	LOG_DEBUG("svrid:%d, uid:%d", svrid, uid);
	net::msg_stop_conctrl_player msg;	
	msg.set_uid(uid);
	SendMsg2Server(svrid, &msg, net::L2S_MSG_STOP_CONTROL_PLAYER, uid);
}

void	CServerMgr::SynCtrlUserCfg(net::msg_syn_ctrl_user_cfg * pmsg)
{
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, pmsg, net::L2S_MSG_SYN_CTRL_USER_CFG, 0);
	}
}

void	CServerMgr::SynLuckyCfg(net::msg_syn_lucky_cfg * pmsg)
{
	//同步到所有游戏服务器
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, pmsg, net::L2S_MSG_SYN_LUCKY_CFG, 0);
	}
}

void	CServerMgr::SynFishCfg(net::msg_syn_fish_cfg * pmsg)
{
	//同步到所有游戏服务器
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		if (server.gameType == net::GAME_CATE_FISHING)
		{
			SendMsg2Server(server.svrID, pmsg, net::L2S_MSG_SYN_FISH_CFG, 0);
		}
	}
}

void	CServerMgr::ResetLuckyCfg(net::msg_reset_lucky_cfg * pmsg)
{
	//同步到所有游戏服务器
	MAP_SERVERS::iterator it = m_mpServers.begin();
	for (; it != m_mpServers.end(); ++it)
	{
		stGServer& server = it->second;
		SendMsg2Server(server.svrID, pmsg, net::L2S_MSG_RESET_LUCKY_CFG, 0);
	}
}