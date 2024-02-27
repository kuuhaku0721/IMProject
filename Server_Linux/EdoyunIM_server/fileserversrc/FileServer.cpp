
#include "../net/inetaddress.h"
#include "../base/logging.h"
#include "../base/singleton.h"
#include "FileServer.h"
#include "FileSession.h"

bool FileServer::Init(const char* ip, short port, EventLoop* loop)
{   //文件服务器也是个服务器，服务器的设计思路就和chatserver一样
    //初始化，设置OnConnection回调，在OnConnection中设置OnRead回调，在OnRead中调用Process处理，然后分发出去
    InetAddress addr(ip, port);
    m_server.reset(new TcpServer(loop, addr, "YFY-MYFileServer", TcpServer::kReusePort));
    m_server->setConnectionCallback(std::bind(&FileServer::OnConnection, this, std::placeholders::_1));
    //启动侦听
    m_server->start();

    return true;
}

void FileServer::OnConnection(std::shared_ptr<TcpConnection> conn)
{
    if (conn->connected())
    {
        LOG_INFO << "client connected:" << conn->peerAddress().toIpPort();
        ++m_baseUserId; //维护一个用户id，用于判别文件谁发的，发给谁
        std::shared_ptr<FileSession> spSession(new FileSession(conn));  //spSession，也就是client
        conn->setMessageCallback(std::bind(&FileSession::OnRead, spSession.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        std::lock_guard<std::mutex> guard(m_sessionMutex);
        m_sessions.push_back(spSession);  //服务器的list集合保留一份所有已连接的客户端
    }
    else
    {
        OnClose(conn);
    }
}

void FileServer::OnClose(const std::shared_ptr<TcpConnection>& conn)
{
    //TODO: 这样的代码逻辑太混乱，需要优化
    //如果客户端断开连接，调用OnClose，从list中取出对应客户端，然后释放
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    for (auto iter = m_sessions.begin(); iter != m_sessions.end(); ++iter)
    {
        if ((*iter)->GetConnectionPtr() == NULL)
        {
            LOG_ERROR << "connection is NULL";
            break;
        }
                          
        //用户下线
        m_sessions.erase(iter); //从list集合中删除
        //bUserOffline = true;
        LOG_INFO << "client disconnected: " << conn->peerAddress().toIpPort();
        break;       
    }    
}