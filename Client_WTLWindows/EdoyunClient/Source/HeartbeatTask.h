#pragma once

#include "ThreadPool.h"

class CIUProtocol;
class CEdoyunClient;

//����������
class CHeartbeatTask : public CThreadPoolTask
{
public:
	CHeartbeatTask(void);
	~CHeartbeatTask(void);

public:
	virtual int Run();
	virtual int Stop();
	virtual void TaskFinish();

	void SetAlive(BOOL bAlive);
	BOOL IsAlive();

public:
	CEdoyunClient*		m_pTalkClient;
	long				m_nFrequency;		//�������ķ���Ƶ��(����Ϊ��λ)
	BOOL				m_bStop;

	CRITICAL_SECTION	m_cs;
	BOOL				m_bAlive;
};