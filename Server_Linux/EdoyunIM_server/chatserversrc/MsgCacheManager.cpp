
#include "../base/logging.h"
#include "MsgCacheManager.h"

MsgCacheManager::MsgCacheManager()
{
}

MsgCacheManager::~MsgCacheManager()
{
}


/*
* 两类消息管理方式是一样的，有消息来就先加入缓冲区链表(链表能够保证消息的顺序)，然后需要发送时就从链表中取出数据然后由调用它的函数去发送
* 好处是处理和发送分开，互不耽误
* 保存的时候都是以结构体保存，结构体里面维护了消息发送者的id和消息本体，这样在获取消息的时候就可以根据id来找到对应的消息了
*/

bool MsgCacheManager::AddNotifyMsgCache(int32_t userid, const std::string& cache)
{
    std::lock_guard<std::mutex> guard(m_mtNotifyMsgCache);
    NotifyMsgCache nc;
    nc.userid = userid;
    nc.notifymsg.append(cache.c_str(), cache.length());;
    m_listNotifyMsgCache.push_back(nc);
    LOG_INFO << "append notify msg to cache, userid: " << userid << ", m_mapNotifyMsgCache.size() : " << m_listNotifyMsgCache.size() << ", cache length : " << cache.length();
    

    //TODO: 存盘或写入数据库以防止程序崩溃丢失

    return true;
}

void MsgCacheManager::GetNotifyMsgCache(int32_t userid, std::list<NotifyMsgCache>& cached)
{
    std::lock_guard<std::mutex> guard(m_mtNotifyMsgCache);
    for (auto iter = m_listNotifyMsgCache.begin(); iter != m_listNotifyMsgCache.end(); )
    {
        if (iter->userid == userid)
        {
            cached.push_back(*iter);
            iter = m_listNotifyMsgCache.erase(iter);
        }
        else
        {
            iter++;
        }
    }

   
    LOG_INFO << "get notify msg  cache,  userid: " << userid << ", m_mapNotifyMsgCache.size(): " << m_listNotifyMsgCache.size() << ", cached size: " << cached.size();
}

bool MsgCacheManager::AddChatMsgCache(int32_t userid, const std::string& cache)
{
    std::lock_guard<std::mutex> guard(m_mtChatMsgCache);
    ChatMsgCache c;
    c.userid = userid;
    c.chatmsg.append(cache.c_str(), cache.length());
    m_listChatMsgCache.push_back(c);
    LOG_INFO << "append chat msg to cache, userid: " << userid << ", m_listChatMsgCache.size() : " << m_listChatMsgCache.size() << ", cache length : " << cache.length();
    //TODO: 存盘或写入数据库以防止程序崩溃丢失

    return true;
}

void MsgCacheManager::GetChatMsgCache(int32_t userid, std::list<ChatMsgCache>& cached)
{
    std::lock_guard<std::mutex> guard(m_mtChatMsgCache);
    for (auto iter = m_listChatMsgCache.begin(); iter != m_listChatMsgCache.end(); )
    {
        if (iter->userid == userid)
        {
            cached.push_back(*iter);
            iter = m_listChatMsgCache.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    LOG_INFO << "get chat msg cache, no cache,  userid: " << userid << ",m_listChatMsgCache.size(): " << m_listChatMsgCache.size() << ", cached size: " << cached.size();
}