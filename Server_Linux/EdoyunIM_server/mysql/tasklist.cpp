#include "../base/logging.h"
#include "tasklist.h"


CTaskList::CTaskList() : m_uReadIndex(0), m_uWriteIndex(0)
{
	memset(m_pTaskNode, 0, sizeof(m_pTaskNode));
}

CTaskList::~CTaskList(void)
{
	for (int i = 0; i < MAX_TASK_NUM; i++)
	{
		delete m_pTaskNode[i];
	}
}

bool CTaskList::Push(IMysqlTask* poTask)
{
	//判断事务数量是否已达到最大数量
	uint16_t usNextIndex = static_cast<uint16_t>((m_uWriteIndex + 1) % MAX_TASK_NUM);
	if (usNextIndex == m_uReadIndex)
	{
		// getchar();
		LOG_ERROR << "mysql task list full (read : " << m_uReadIndex << ", write : " << m_uWriteIndex << ")";
		return false;
	}
	//将事务加入队列中
	m_pTaskNode[m_uWriteIndex] = poTask;
	m_uWriteIndex = usNextIndex;

    return true;
}

IMysqlTask* CTaskList::Pop()
{
	//如果队列是空(没有事件待处理)就直接退出
	if (m_uWriteIndex == m_uReadIndex)
	{
		return NULL;
	}
	//从队列中取出一个，然后移动下标
	IMysqlTask* pTask = m_pTaskNode[m_uReadIndex];
	m_uReadIndex = static_cast<uint16_t>((m_uReadIndex + 1) % MAX_TASK_NUM);

	return pTask;
}

