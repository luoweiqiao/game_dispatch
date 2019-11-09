
#ifndef GOBAL_EVENT_MGR_H__
#define GOBAL_EVENT_MGR_H__

#include "svrlib.h"
#include "game_define.h"


using namespace std;
using namespace svrlib;


enum   emTIMER_EVENT_ID
{
	emTIMER_EVENT_REPORT = 1,    // 上报在线人数
    emTIMER_EVENT_RETIRE = 2,    // 退休gameserver
};

enum emGAMESVR_TYPE
{
    emSVR_TYPE_GOLD     = 1,  //金币房server
    emSVR_TYPE_PRIVATE  = 3,  //私人房server
};

class CGameSvrEventMgr : public ITimerSink,public AutoDeleteSingleton<CGameSvrEventMgr>
{
public:
	CGameSvrEventMgr();
	~CGameSvrEventMgr();

	bool	Init();
	void	ShutDown();
	void	ProcessTime();

	void	OnTimer(uint8 eventID);


    // 通用初始化
    bool    GameServerInit();    
    // 通用关闭
    bool    GameServerShutDown();    
    // 通用TICK
    bool    GameServerTick();
    
    void    ReportInfo2Lobby();

    void    ReportInfo2Dispatch();

    void    StartRetire();

    void    DoRetire();
    
private:
    CTimer* m_pReportTimer;
    CTimer* m_pRetireTimer;

};



#endif // GOBAL_EVENT_MGR_H__


