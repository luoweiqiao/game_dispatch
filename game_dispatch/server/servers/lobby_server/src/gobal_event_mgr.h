
#ifndef GOBAL_EVENT_MGR_H__
#define GOBAL_EVENT_MGR_H__

#include "svrlib.h"
#include "game_define.h"
#include "msg_define.pb.h"

using namespace std;
using namespace svrlib;

class CPlayer;

enum   emTIMER_EVENT_ID
{
	emTIMER_EVENT_RECOVER	= 1,    // 上报在线人数
	emTIMER_EVENT_REPORT    = 2,    // 向dispatch上报在线人数
};
class CGobalEventMgr : public ITimerSink,public AutoDeleteSingleton<CGobalEventMgr>
{
public:
	CGobalEventMgr();
	~CGobalEventMgr();

	bool	Init();
	void	ShutDown();
	void	ProcessTime();

	virtual void	OnTimer(uint8 eventID);

	void 	SaveAllPlayerAndLoginOut();

    void    AddSpeak(net::msg_speak_broadcast_rep& msg);
    void    SendSpeakListToPlayer(CPlayer* pPlayer);
    void    ReportInfo2Dispatch();
private:
	void	OnNewDay();
	void	OnNewWeek();
	void	OnNewMonth();

private:
    list<net::msg_speak_broadcast_rep> m_speakList;
    CTimer* m_pReportTimer;
};











































#endif // GOBAL_EVENT_MGR_H__


