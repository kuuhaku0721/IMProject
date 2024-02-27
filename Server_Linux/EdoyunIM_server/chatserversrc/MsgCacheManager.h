
#pragma once
#include <list>
#include <stdint.h>
#include <string>
#include <mutex>

struct NotifyMsgCache //通知消息缓冲区
{
    int32_t     userid;
    std::string notifymsg;  
};

struct ChatMsgCache  //聊天消息缓冲区
{
    int32_t     userid;
    std::string chatmsg;
};

class MsgCacheManager final  //消息缓冲区管理类
{
public:
    MsgCacheManager();
    ~MsgCacheManager();
    MsgCacheManager(const MsgCacheManager& rhs) = delete;
    MsgCacheManager& operator =(const MsgCacheManager& rhs) = delete;

public:
    bool AddNotifyMsgCache(int32_t userid, const std::string& cache);
    void GetNotifyMsgCache(int32_t userid, std::list<NotifyMsgCache>& cached);

    bool AddChatMsgCache(int32_t userid, const std::string& cache);
    void GetChatMsgCache(int32_t userid, std::list<ChatMsgCache>& cached);


private:
    std::list<NotifyMsgCache>       m_listNotifyMsgCache;    //通知类消息缓存，比如加好友消息
    std::mutex                      m_mtNotifyMsgCache;
    std::list<ChatMsgCache>         m_listChatMsgCache;      //聊天消息缓存
    std::mutex                      m_mtChatMsgCache;
};
