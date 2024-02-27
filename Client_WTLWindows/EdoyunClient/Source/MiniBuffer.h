#pragma once

//��������������ʧ�Ժ��Զ��ͷŵ������ڴ���
class CMiniBuffer
{
public:
	CMiniBuffer(long nSize, BOOL bAutoRelease = TRUE);
	~CMiniBuffer();

	void Release();

	long GetSize();
	char* GetBuffer();
	
	//TODO: ��һ���ӿڣ�ʹCMiniBuffer�������ֱ�ӱ������ַ���ָ��ʹ��
	//PSTR operator PSTR(); 
	
	void EnableAutoRelease(BOOL bAutoRelease);
	BOOL IsAutoRelease();

private:
	BOOL	m_bAutoRelease;
	long	m_nSize;
	char*	m_pData;
};