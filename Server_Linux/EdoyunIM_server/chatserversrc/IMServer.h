
#pragma once
#include <memory>
#include <list>
#include <map>
#include <mutex>
#include "../net/tcpserver.h"
#include "../net/eventloop.h"
#include "ClientSession.h"

using namespace net;

struct StoredUserInfo
{
    int32_t         userid;
    std::string     username;
    std::string     password;
    std::string     nickname;
};

class IMServer final
{
public:
    IMServer() = default;
    ~IMServer() = default;

    IMServer(const IMServer& rhs) = delete;
    IMServer& operator =(const IMServer& rhs) = delete;

    bool Init(const char* ip, short port, EventLoop* loop);

    void GetSessions(std::list<std::shared_ptr<ClientSession>>& sessions);
    bool GetSessionByUserId(std::shared_ptr<ClientSession>& session, int32_t userid);
    bool IsUserSessionExsit(int32_t userid);

private:
    //�����ӵ������û����ӶϿ���������Ҫͨ��conn->connected()���жϣ�һ��ֻ����loop�������
    void OnConnection(std::shared_ptr<TcpConnection> conn);  
    //���ӶϿ�
    void OnClose(const std::shared_ptr<TcpConnection>& conn);
   

private:
    std::shared_ptr<TcpServer>                     m_server;
    std::list<std::shared_ptr<ClientSession>>      m_sessions;
    std::mutex                                     m_sessionMutex;      //���߳�֮�䱣��m_sessions
    int                                            m_baseUserId{};
    std::mutex                                     m_idMutex;           //���߳�֮�䱣��m_baseUserId
};
