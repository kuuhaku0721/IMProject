#include "../base/logging.h"
#include "mysqlthrd.h"
#include <functional>
using namespace std;

CMysqlThrd::CMysqlThrd(void)
{
	m_bTerminate	= false;

    m_bStart        = false;
    m_poConn        = NULL;
}

CMysqlThrd::~CMysqlThrd(void)
{
}

void CMysqlThrd::Run()
{
	_MainLoop();
	_Uninit();

	if (NULL != m_pThread)
	{
		m_pThread->join();
	}
}

bool CMysqlThrd::Start(const string& host, const string& user, const string& pwd, const string& dbname)
{
	m_poConn = new CDatabaseMysql();

    if (NULL == m_poConn)
    {
        LOG_FATAL << "CMysqlThrd::Start, Cannot open database";
        return false;
    }

	if (m_poConn->Initialize(host, user, pwd, dbname) == false)   //连接数据库，然后初始化线程
	{
		return false;
	}

    return _Init();
}

void CMysqlThrd::Stop()
{
	if (m_bTerminate)
	{
		return;
	}
	m_bTerminate = true;
	m_pThread->join();
}

bool CMysqlThrd::_Init()
{
    if (m_bStart)
    {
        return true;
    }

    // 启动线程
	m_pThread.reset(new std::thread(std::bind(&CMysqlThrd::_MainLoop, this))); //线程主函数是MainLoop

	{
		std::unique_lock<std::mutex> lock(mutex_);
		while (m_bStart == false)
		{
			cond_.wait(lock);
		}
	} 

    return true;
}

void CMysqlThrd::_Uninit()
{
    //m_poConn->Close();
}

void CMysqlThrd::_MainLoop()
{
	m_bStart = true;

	{
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.notify_all();
	}

    IMysqlTask* poTask;

	while(!m_bTerminate) //只要没有终止就一直循环，终止将在其他函数内触发
	{
        if(NULL != (poTask = m_oTask.Pop())) //任务队列不为空，取出一条
        {
            poTask->Execute(m_poConn); //交给数据库执行
            m_oReplyTask.Push(poTask); //执行完毕有个回复，交给回复线程
            continue; //完成一条，继续循环
        }

		this_thread::sleep_for(chrono::milliseconds(1000));
	}
}

