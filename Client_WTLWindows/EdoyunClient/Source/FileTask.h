#pragma once
#include "Thread.h"
#include <list>
#include <mutex>
#include <condition_variable>

class CIUProtocol;
class CFileItemRequest;
class CEdoyunClient;
class CUploadFileResult;
class CIUSocket;

//TODO: ����ĳ�FileThread
class CFileTask : public CThread
{
public:
	CFileTask(CIUSocket* sockeClient);
	virtual ~CFileTask(void);

public:
	virtual void Stop() override;
	
	BOOL AddItem(CFileItemRequest* pItem);
	void RemoveItem(CFileItemRequest* pItem);									//�Ƴ����������ػ����ϴ�����
	void ClearAllItems();

	void HandleItem(CFileItemRequest* pFileItem);

	//void SendConfirmMessage(const CUploadFileResult* pUploadFileResult);		//����ͼƬ�����ļ��ϴ��ɹ��Ժ���ȷ��������Ϣ

    BOOL DownloadFileSynchronously(LPCSTR lpszFileName, LPCTSTR lpszDestPath, BOOL bOverwriteIfExist);

protected:
	virtual void Run() override;

private:
	long UploadFile(PCTSTR pszFileName, HWND hwndReflection, HANDLE hCancelEvent, CUploadFileResult& uploadFileResult);
    BOOL UploadUserThumb(PCTSTR pszFileName, HWND hwndReflection, CUploadFileResult& uploadFileResult);
	void FillUploadFileResult(CUploadFileResult& uploadFileResult, PCTSTR pszLocalName, PCSTR pszRemoteName, long nFileSize, char* pszMd5);
    long DownloadFile3(LPCSTR lpszFileName, LPCTSTR lpszDestPath, BOOL bOverwriteIfExist, HWND hwndReflection, HANDLE hCancelEvent);
    
public:
	CIUProtocol*					m_pProtocol;
	CEdoyunClient*					m_lpFMGClient;

private:
	CFileItemRequest*				m_pCurrentTransferringItem;					//��ǰ���ڴ������
	std::list<CFileItemRequest*>	m_Filelist;	
	std::mutex						m_mtItems;
	std::condition_variable			m_cvItems;
    CIUSocket*                      m_SocketClient;
    int32_t                         m_seq;
};