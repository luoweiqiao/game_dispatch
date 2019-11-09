
#ifndef GAME_NET_MGR_H_
#define GAME_NET_MGR_H_

#include "Network.h"
#include "NetworkObject.h"
#include "network/protobuf_pkg.h"
#include "svrlib.h"
#include "network/CCSocketHandler.h"
#include "network/CCSocketServer.h"
#include "network/idxobj.h"
#include "network/CCReactor.h"
#include "hashtab.h"



class CGameNetMgr : public CProtobufMsgHanlde, public AutoDeleteSingleton<CGameNetMgr>
{
public:
    CGameNetMgr() {}
    ~CGameNetMgr() {}

    bool Init();
    

    void Update() { CCReactor::Instance().Update(); }
public:

    //MAP_LINKINFO GetSvrsLinkinfo() {return m_mLinkinfo;}
private:
    typedef std::map<uint32 , std::set<uint32> > MAP_LINKINFO; //每个lobby上所连接的gameserver
    MAP_LINKINFO m_mLinkinfo;
};


#define LOBBY_MAX_CONN_NUM   100
#define SERVER_MAX_CONN_NUM   100
#define PHP_MAX_CONN_NUM   1000
//! 大厅连接
class LobbyHandle : public CCSocketHandler, public CTimerNotify, public CObj
{
public:
    LobbyHandle() {}
    virtual ~LobbyHandle() {}

    virtual int Init();

    //! CObj virtual func
public:
    virtual int GetHashKey(void *pvKey, int& iKeyLength);
    virtual int SetHashKey(const void *pvKey, int iKeyLength);
    virtual int Show(FILE *fpOut) { return 0; }

public:
    virtual int OnPacketComplete(char * data, int len);
    virtual void OnTimer();

    virtual NetworkObject* getObject() { return this; }
    virtual int DestroyObj();

    void ResetHeart();
private:
    int m_flow;
public: 
    //! 数据管理器
    static LobbyHandle* GetDRNodeByKey(unsigned int Key, int isCreate = True);
    static LobbyHandle* CreateDRNode(unsigned int Key);
    static int DeleteNode(unsigned int Key);
    static int GetFlow();

    static int s_flow;
    static CHashTab<LOBBY_MAX_CONN_NUM> m_stConnHash;
    static CObjSeg* m_pConnMng;

    DECLARE_DYN
};

class LobbyServer : public CCSocketServer
{
public:
    LobbyServer(const char* bindIp, uint16_t port, int acceptcnt = 256, int backlog = 256)
        : CCSocketServer(bindIp, port, acceptcnt, backlog)
    {}

    virtual ~LobbyServer() {}

protected:
    virtual NetworkObject* CreateHandler(int netfd, struct sockaddr_in* peer)
    {
        int flow = LobbyHandle::GetFlow();
        LOG_DEBUG("client netfd[%d] flow[%d]", netfd, flow);

        return LobbyHandle::GetDRNodeByKey(flow);
    }
};

// !gameserver连接
class ServerHandle : public CCSocketHandler, public CTimerNotify, public CObj
{
public:
    ServerHandle() {}
    virtual ~ServerHandle() {}

    virtual int Init();

    //! CObj virtual func
public:
    virtual int GetHashKey(void *pvKey, int& iKeyLength);
    virtual int SetHashKey(const void *pvKey, int iKeyLength);
    virtual int Show(FILE *fpOut) { return 0; }

public:
    virtual int OnPacketComplete(char * data, int len);
    virtual void OnTimer();

    virtual NetworkObject* getObject() { return this; }
    virtual int DestroyObj();
private:
    int m_flow;
public:
    //! 数据管理器
    static ServerHandle* GetDRNodeByKey(unsigned int Key, int isCreate = True);
    static ServerHandle* CreateDRNode(unsigned int Key);
    static int DeleteNode(unsigned int Key);
    static int GetFlow();

    static int s_flow;
    static CHashTab<SERVER_MAX_CONN_NUM> m_stConnHash;
    static CObjSeg* m_pConnMng;

    DECLARE_DYN
};

class SocketServer : public CCSocketServer
{
public:
    SocketServer(const char* bindIp, uint16_t port, int acceptcnt = 256, int backlog = 256)
        : CCSocketServer(bindIp, port, acceptcnt, backlog)
    {}

    virtual ~SocketServer() {}

protected:
    virtual NetworkObject* CreateHandler(int netfd, struct sockaddr_in* peer)
    {
        int flow = ServerHandle::GetFlow();
        LOG_DEBUG("server netfd[%d] flow[%d]", netfd, flow);

        return ServerHandle::GetDRNodeByKey(flow);
    }
};

//! php连接
class PhpHandle : public CCSocketHandler, public CTimerNotify, public CObj
{
public:
    PhpHandle() {}
    virtual ~PhpHandle() {}

    virtual int Init();

    //! CObj virtual func
public:
    virtual int GetHashKey(void *pvKey, int& iKeyLength);
    virtual int SetHashKey(const void *pvKey, int iKeyLength);
    virtual int Show(FILE *fpOut) { return 0; }

public:
    virtual int OnPacketComplete(char * data, int len);
    virtual void OnTimer();

    virtual NetworkObject* getObject() { return this; }
    virtual int DestroyObj();

    virtual ICC_Decoder*  CreateDecoder() { _decode = new PhpDecoder();  return _decode; }
private:
    int m_flow;
public:
    //! 数据管理器
    static PhpHandle* GetDRNodeByKey(unsigned int Key, int isCreate = True);
    static PhpHandle* CreateDRNode(unsigned int Key);
    static int DeleteNode(unsigned int Key);
    static int GetFlow();

    static int s_flow;
    static CHashTab<PHP_MAX_CONN_NUM> m_stConnHash;
    static CObjSeg* m_pConnMng;

    DECLARE_DYN
};

class PhpServer : public CCSocketServer
{
public:
    PhpServer(const char* bindIp, uint16_t port, int acceptcnt = 256, int backlog = 256)
        : CCSocketServer(bindIp, port, acceptcnt, backlog)
    {}

    virtual ~PhpServer() {}

protected:
    virtual NetworkObject* CreateHandler(int netfd, struct sockaddr_in* peer)
    {
        int flow = PhpHandle::GetFlow();
        LOG_DEBUG("php netfd[%d] flow[%d]", netfd, flow);

        return PhpHandle::GetDRNodeByKey(flow);
    }
};


#endif // GAME_NET_MGR_H_



