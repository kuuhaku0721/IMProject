
#pragma once
#include "../net/buffer.h"
#include "TcpSession.h"         //TcpSession设置和chatserver完全一致，主要功能是转发，把服务端处理完的数据发回去给对应客户端

class FileSession : public TcpSession
{
public:
    FileSession(const std::shared_ptr<TcpConnection>& conn);
    virtual ~FileSession();
    FileSession(const FileSession& rhs) = delete;
    FileSession& operator =(const FileSession& rhs) = delete;

    //有数据可读, 会被多个工作loop调用
    void OnRead(const std::shared_ptr<TcpConnection>& conn, Buffer* pBuffer, Timestamp receivTime);   
    void SendUserStatusChangeMsg(int32_t userid, int type);

private:
    bool Process(const std::shared_ptr<TcpConnection>& conn, const char* inbuf, size_t length);
    
    void OnUploadFileResponse(const std::string& filemd5, int32_t offset, int32_t filesize, const std::string& filedata, const std::shared_ptr<TcpConnection>& conn);
    void OnDownloadFileResponse(const std::string& filemd5, int32_t offset, int32_t filesize, const std::shared_ptr<TcpConnection>& conn);

    void ResetFile();

private:
    int32_t           m_id;         //session id
    int               m_seq;        //当前Session数据包序列号

    //当前文件信息
    FILE*             m_fp{};       //当前文件
    int32_t           m_offset{};   //偏移
    int32_t           m_filesize{}; //文件总大小
};