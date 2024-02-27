
#pragma once

#include "mysqltask.h"
#include "mysqlthrd.h"

class CMysqlThrdMgr
{
public:
    CMysqlThrdMgr(void);
    virtual ~CMysqlThrdMgr(void);

public:
	bool Init(const string& host, const string& user, const string& pwd, const string& dbname);
	bool AddTask(uint32_t dwHashID, IMysqlTask* poTask);
    bool AddTask(IMysqlTask* poTask) 
    { 
        return m_aoMysqlThreads[m_dwThreadsCount].AddTask(poTask); 
    }

	inline uint32_t GetTableHashID(uint32_t dwHashID) const  //获取表的哈希值ID
    { 
        return dwHashID % m_dwThreadsCount; 
    }

    bool ProcessReplyTask(int32_t nCount); 
    static uint32_t GetThreadsCount()  //线程数量
    { 
        return m_dwThreadsCount; 
    }

protected:
	static const uint32_t m_dwThreadsCount = 9;
	CMysqlThrd            m_aoMysqlThreads[m_dwThreadsCount+1];
};

/*
* 这部分涉及三个主要的类
* mysqlthrdmgr：线程管理类，负责总调用
* mysqlthrd：线程类，负责开线程去处理任务
* mysqltask和tasklist：mysql事务的队列，所有任务都交给它来保存
* 当需要调用线程去处理复数的mysql事务的时候：
* 首先调用mysqlthrdmgr初始化任务队列tasklist和总线程mysqlthrd
* 确定任务之后将任务加入tasklist，在外部调用的时候只到这一步，将任务加入tasklist后外部就不需要管了，剩下的由它内部完成
* 在任务队列tasklist中有任务时，由mysqlthrd启动线程去处理
* mysqlthrd的MainLoop循环等待，一旦tasklist中有任务就拿出来处理，没有就继续循环等待
*/
