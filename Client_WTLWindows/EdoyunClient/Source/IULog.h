#pragma once

enum LOG_LEVEL
{
	LOG_NORMAL,
	LOG_WARNING,
	LOG_ERROR
};

class CIULog
{
public:
	static BOOL Init(BOOL bToFile, PCTSTR pszLogFileName);
	static void Unit();
	
	//������߳�ID�ź����ں���ǩ��
	static BOOL Log(long nLevel, PCTSTR pszFmt, ...);
	//����߳�ID�ź����ں���ǩ��
	static BOOL Log(long nLevel, PCSTR pszFunctionSig, PCTSTR pszFmt, ...);		//ע��:pszFunctionSig����ΪAnsic�汾
	static BOOL Log(long nLevel, PCSTR pszFunctionSig, PCSTR pszFmt, ...);

private:
	static CString GetCurrentTime();
	
private:
	static BOOL		m_bToFile;				//��־д���ļ�����д������̨
	static CString  m_strLogFileName;
	static HANDLE	m_hLogFile;
};