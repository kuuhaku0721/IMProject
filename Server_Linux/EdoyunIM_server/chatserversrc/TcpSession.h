

#pragma once

#include <memory>
#include "../net/tcpconnection.h"

using namespace net;

//Ϊ����ҵ�����߼��ֿ���ʵ��Ӧ������һ������̳���TcpSession����TcpSession��ֻ���߼����룬��������ҵ�����
class TcpSession
{
public:
    TcpSession(const std::shared_ptr<TcpConnection>& conn);
    ~TcpSession();

    TcpSession(const TcpSession& rhs) = delete;
    TcpSession& operator =(const TcpSession& rhs) = delete;

    std::shared_ptr<TcpConnection> GetConnectionPtr()
    {
        return tmpConn_.lock();
    }

    void Send(const std::string& buf);
    void Send(const char* p, int length);

protected:
    std::weak_ptr<TcpConnection>    tmpConn_;
};
