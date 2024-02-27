
#pragma once
#include <memory>
#include <list>
#include <map>
#include <mutex>
#include "../net/tcpserver.h"
#include "../net/eventloop.h"
#include "FileSession.h"

using namespace net;

struct StoredUserInfo
{
    int32_t         userid;
    std::string     username;
    std::string     password;
    std::string     nickname;
};

class FileServer final
{
public:
    FileServer() = default;
    ~FileServer() = default;
    FileServer(const FileServer& rhs) = delete;
    FileServer& operator =(const FileServer& rhs) = delete;

    bool Init(const char* ip, short port, EventLoop* loop);

private:
    //新连接到来调用或连接断开，所以需要通过conn->connected()来判断，一般只在主loop里面调用
    void OnConnection(std::shared_ptr<TcpConnection> conn);  
    //连接断开
    void OnClose(const std::shared_ptr<TcpConnection>& conn);
   

private:
    std::shared_ptr<TcpServer>                     m_server;
    std::list<std::shared_ptr<FileSession>>        m_sessions;          //需要传文件的客户端也是一个会话，逻辑上当作一个客户端对待，传文件就是客户端的业务功能
    std::mutex                                     m_sessionMutex;      //多线程之间保护m_sessions
    int                                            m_baseUserId{};
    std::mutex                                     m_idMutex;           //多线程之间保护m_baseUserId
};
