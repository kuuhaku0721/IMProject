
#include "../net/inetaddress.h"
#include "../base/logging.h"
#include "../base/singleton.h"
#include "IMServer.h"
#include "ClientSession.h"
#include "UserManager.h"

bool IMServer::Init(const char* ip, short port, EventLoop* loop)
{
    //从数据库中加载所有用户信息
    //TODO: 后面把数据库账号信息统一到一个地方
    if (!Singleton<UserManager>::Instance().Init("192.168.65.132", "root", "123456", "myim")) //服务器初始化的时候先加载所有用户信息
    {
        LOG_ERROR << "Load users from db error";
        return false;
    }
    
    InetAddress addr(ip, port);
    m_server.reset(new TcpServer(loop, addr, "YFY-MYIMSERVER", TcpServer::kReusePort));
    m_server->setConnectionCallback(std::bind(&IMServer::OnConnection, this, std::placeholders::_1));
    //启动侦听
    m_server->start();

    return true;
}

void IMServer::OnConnection(std::shared_ptr<TcpConnection> conn)
{
    if (conn->connected())
    {
        LOG_INFO << "client connected:" << conn->peerAddress().toIpPort();
        ++m_baseUserId; //不一样的地方，要维护一个最大用户id，当作判断标志使用

        std::shared_ptr<ClientSession> spSession(new ClientSession(conn)); //然后就是实例化ClientSession，不一样的是这里是用shared_ptr的形式
        conn->setMessageCallback(std::bind(&ClientSession::OnRead, spSession.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                                                    //依然还是先到OnRead读取，然后去Process解析，最后分发到各个业务函数去处理
        std::lock_guard<std::mutex> guard(m_sessionMutex);
        m_sessions.push_back(spSession); //为了避免前后顺序混乱，在加入列表的时候加个锁
    }
    else
    {
        OnClose(conn);
    }
}

void IMServer::OnClose(const std::shared_ptr<TcpConnection>& conn)
{
    //是否有用户下线
    //bool bUserOffline = false;
    UserManager& userManager = Singleton<UserManager>::Instance();

    //TODO: 这样的代码逻辑太混乱，需要优化 ps:有吗，我觉着挺好的吧
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    for (auto iter = m_sessions.begin(); iter != m_sessions.end(); ++iter)
    {
        if ((*iter)->GetConnectionPtr() == NULL)
        {
            LOG_ERROR << "connection is NULL";
            break;
        }
        
        if ((*iter)->GetConnectionPtr() == conn)
        {
            //遍历其在线好友，给其好友推送离线消息
            std::list<User> friends;
            int32_t offlineUserId = (*iter)->GetUserId();
            userManager.GetFriendInfoByUserId(offlineUserId, friends);
            for (const auto& iter2 : friends)
            {
                std::shared_ptr<ClientSession> targetSession;
                for (auto& iter3 : m_sessions)
                {
                    if (iter2.userid == iter3->GetUserId())                 
                        iter3->SendUserStatusChangeMsg(offlineUserId, 2);
                }
            }
            
            //用户下线
            m_sessions.erase(iter);
            //bUserOffline = true;
            LOG_INFO << "client disconnected: " << conn->peerAddress().toIpPort();
            break;
        }
    }

    LOG_INFO << "current online user count: " << m_sessions.size();
}

void IMServer::GetSessions(std::list<std::shared_ptr<ClientSession>>& sessions)
{
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    sessions = m_sessions;
}

bool IMServer::GetSessionByUserId(std::shared_ptr<ClientSession>& session, int32_t userid)
{
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    std::shared_ptr<ClientSession> tmpSession;
    for (const auto& iter : m_sessions)
    {
        tmpSession = iter;
        if (iter->GetUserId() == userid)
        {
            session = tmpSession;
            return true;
        }
    }

    return false;
}

bool IMServer::IsUserSessionExsit(int32_t userid)
{
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    for (const auto& iter : m_sessions)
    {
        if (iter->GetUserId() == userid)
        {
            return true;
        }
    }

    return false;
}