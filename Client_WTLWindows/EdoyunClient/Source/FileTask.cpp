#include "stdafx.h"
#include "FileTask.h"
#include "UserSessionData.h"
#include "IULog.h"
#include "MD5Sum.h"
#include "Utils.h"
#include "EncodingUtil.h"
#include "Path.h"
#include "EdoyunIMClient.h"
#include "File.h"
#include "MiniBuffer.h"
#include "net/IUProtocolData.h"
#include "net/IUSocket.h"
#include "net/FileMsg.h"
#include "net/protocolstream.h"
#include "net/IUProtocolData.h"

CFileTask::CFileTask(CIUSocket* sockeClient) : m_SocketClient(sockeClient), m_seq(0)
{
	m_lpFMGClient = NULL;
	m_pProtocol = NULL;
	
	m_pCurrentTransferringItem = NULL;
}

CFileTask::~CFileTask()
{
	ClearAllItems();
}


void CFileTask::Stop()
{
	m_bStop = true;
	m_cvItems.notify_one();
}

BOOL CFileTask::AddItem(CFileItemRequest* pItem)
{
	if (NULL == pItem)
		return FALSE;

	std::lock_guard<std::mutex> guard(m_mtItems);
	m_Filelist.push_back(pItem);
	m_cvItems.notify_one();

	return TRUE;
}

void CFileTask::RemoveItem(CFileItemRequest* pItem)
{
	if(pItem == NULL)
		return;

	std::lock_guard<std::mutex> guard(m_mtItems);
	//δ��ʼ���ػ��ϴ�����ֱ���Ƴ�
	if(pItem->m_bPending)
	{
		std::list<CFileItemRequest*>::iterator iter = m_Filelist.begin();
		for(; iter!=m_Filelist.end(); ++iter)
		{
			if(pItem ==*iter)
			{
				m_Filelist.erase(iter);
				delete pItem;
				break;
			}
		}
	}
	//�������ػ��ϴ�����ʹ���ź�����ֹͣ
	else
	{
		if(pItem->m_hCancelEvent != NULL)
			::SetEvent(pItem->m_hCancelEvent);
	}

	//AtlTrace(_T("File item count: %d.\n"), m_Filelist.size());
}

void CFileTask::ClearAllItems()
{
	std::lock_guard<std::mutex> guard(m_mtItems);
	for (auto& iter : m_Filelist)
	{
		delete iter;
	}

	m_Filelist.clear();
}

void CFileTask::Run()
{
	while (!m_bStop)
	{
		CFileItemRequest* pFileItem;
		{
			std::unique_lock<std::mutex> guard(m_mtItems);
			while (m_Filelist.empty())
			{
				if (m_bStop)
					return;

				m_cvItems.wait(guard);
			}

			pFileItem = m_Filelist.front();
			m_Filelist.pop_front();
		}

		HandleItem(pFileItem);
	}
}

void CFileTask::HandleItem(CFileItemRequest* pFileItem)
{
	if(pFileItem==NULL || pFileItem->m_uType!=NET_DATA_FILE)
		return;

	//��ʼ���д���
	pFileItem->m_bPending = FALSE;
	m_pCurrentTransferringItem = pFileItem;
	
	BOOL bRet = FALSE;
	CUploadFileResult* pUploadFileResult = new CUploadFileResult();
	if(pFileItem->m_nFileType == FILE_ITEM_UPLOAD_USER_THUMB)
	{
		bRet = UploadUserThumb(pFileItem->m_szFilePath, pFileItem->m_hwndReflection,*pUploadFileResult);
		long nUploadUserThumbResult = (bRet==0 ? UPLOAD_USER_THUMB_RESULT_SUCCESS : UPLOAD_USER_THUMB_RESULT_FAILED);
        ::PostMessage(pFileItem->m_hwndReflection, FMG_MSG_UPLOAD_USER_THUMB, (WPARAM)bRet, (LPARAM)pUploadFileResult);      
	}
	else if(pFileItem->m_nFileType==FILE_ITEM_UPLOAD_CHAT_IMAGE || pFileItem->m_nFileType==FILE_ITEM_UPLOAD_CHAT_OFFLINE_FILE)
	{
		//�ϴ��ļ����ʧ�ܣ�����������
		pUploadFileResult->m_uSenderID = pFileItem->m_uSenderID;
		pUploadFileResult->m_setTargetIDs = pFileItem->m_setTargetIDs;
		pUploadFileResult->m_nFileType = pFileItem->m_nFileType; 
		pUploadFileResult->m_hwndReflection = pFileItem->m_hwndReflection;
		long nRetCode;
		while(pFileItem->m_nRetryTimes < 3)
		{
			//nRetCode = m_pProtocol->UploadFile3(pFileItem->m_szFilePath, pFileItem->m_hwndReflection, pFileItem->m_hCancelEvent,*pUploadFileResult);
			nRetCode = UploadFile(pFileItem->m_szFilePath, pFileItem->m_hwndReflection, pFileItem->m_hCancelEvent,*pUploadFileResult);
			if(nRetCode==FILE_UPLOAD_SUCCESS || nRetCode==FILE_UPLOAD_USERCANCEL)
				break;

			::Sleep(3000);

			++pFileItem->m_nRetryTimes;
		}
		
		//�����û�ȡ���������ϴ��ɹ���ʧ�ܶ�Ҫ���߶Է�
		if(nRetCode != FILE_UPLOAD_USERCANCEL)
		{
			//SendConfirmMessage(pUploadFileResult);

			//��¡һ���ϴ��������������PostMessage
			CUploadFileResult* pResult = new CUploadFileResult();
			pResult->Clone(pUploadFileResult);
			long nSendFileResultCode = (nRetCode==FILE_UPLOAD_SUCCESS ? SEND_FILE_SUCCESS : SEND_FILE_FAILED);
            //����Ի����Ѿ��رգ���ֱ�ӷ���������
            if (::IsWindow(pFileItem->m_hwndReflection))
                ::PostMessage(pFileItem->m_hwndReflection, FMG_MSG_SEND_FILE_RESULT, (WPARAM)nSendFileResultCode, (LPARAM)pResult);
            else
                ::PostMessage(m_lpFMGClient->m_UserMgr.m_hProxyWnd, FMG_MSG_SEND_FILE_RESULT, (WPARAM)nSendFileResultCode, (LPARAM)pResult);			
		}
	}
	else if(pFileItem->m_nFileType==FILE_ITEM_DOWNLOAD_USER_THUMB)
	{
		CString strDumyPath;
		CString strThumb;
		//std::set<UINT>::const_iterator iter = pFileItem->m_setTargetIDs.begin();
		UINT uTargetID = pFileItem->m_uAccountID;
		strThumb.Format(_T("%s%u.png"), m_lpFMGClient->m_UserMgr.GetCustomUserThumbFolder().c_str(), uTargetID);
		long nRetCode = DownloadFile3(pFileItem->m_szUtfFilePath, strThumb, TRUE, pFileItem->m_hwndReflection, pFileItem->m_hCancelEvent);

		if(nRetCode == FILE_DOWNLOAD_SUCCESS)
		{		
			TransformImage(strThumb, strThumb, 64, 64, strDumyPath);
			::PostMessage(m_lpFMGClient->m_UserMgr.m_hProxyWnd, FMG_MSG_CUSTOMFACE_AVAILABLE, (WPARAM)(uTargetID), 0);
		}
        else
            CIULog::Log(LOG_ERROR, __FUNCSIG__, "download user thumb [%s] failed, userid = %d.", pFileItem->m_szUtfFilePath, pFileItem->m_uAccountID);
	}
	//���������ļ�
	else if(pFileItem->m_nFileType == FILE_ITEM_DOWNLOAD_CHAT_OFFLINE_FILE)
	{
		long nRetCode;
		while(pFileItem->m_nRetryTimes < 3)
		{
            nRetCode = DownloadFile3(pFileItem->m_szUtfFilePath, pFileItem->m_szFilePath, TRUE, pFileItem->m_hwndReflection, pFileItem->m_hCancelEvent);
            //nRetCode = m_pProtocol->DownloadFile3(pFileItem->m_szUtfFilePath, pFileItem->m_szFilePath, TRUE, pFileItem->m_hwndReflection, pFileItem->m_hCancelEvent);
			if(nRetCode==FILE_DOWNLOAD_SUCCESS || nRetCode==FILE_DOWNLOAD_USERCANCEL)
				break;

			++pFileItem->m_nRetryTimes;
			::Sleep(3000);
		}
		
		//TODO: Ӧ�øĳ�PostMessage
		if(nRetCode == FILE_DOWNLOAD_SUCCESS)
			::SendMessage(pFileItem->m_hwndReflection, FMG_MSG_RECV_FILE_RESULT, RECV_FILE_SUCCESS, (LPARAM)pFileItem);
		else if(nRetCode == FILE_DOWNLOAD_FAILED)
			::SendMessage(pFileItem->m_hwndReflection, FMG_MSG_RECV_FILE_RESULT, RECV_FILE_FAILED, (LPARAM)pFileItem);
	}
    //��������ͼƬ
	else if(pFileItem->m_nFileType == FILE_ITEM_DOWNLOAD_CHAT_IMAGE)
	{
		long nRetCode;
		while(pFileItem->m_nRetryTimes < 3)
		{
			if(pFileItem->m_szUtfFilePath[0] == NULL)
				break;
			//nRetCode = m_pProtocol->DownloadFile3(pFileItem->m_szUtfFilePath, pFileItem->m_szFilePath, TRUE, pFileItem->m_hwndReflection, pFileItem->m_hCancelEvent);
            nRetCode = DownloadFile3(pFileItem->m_szUtfFilePath, pFileItem->m_szFilePath, TRUE, pFileItem->m_hwndReflection, pFileItem->m_hCancelEvent);
            if(nRetCode==FILE_DOWNLOAD_SUCCESS || nRetCode==FILE_DOWNLOAD_USERCANCEL)
				break;

			++pFileItem->m_nRetryTimes;
			::Sleep(3000);
		}
		
		CBuddyMessage* lpMsg = pFileItem->m_pBuddyMsg;

		UINT nSenderID = lpMsg->m_nFromUin;
		UINT nTargetID = lpMsg->m_nToUin;
		
		if(IsGroupTarget(nTargetID))
		{
			//������ƽ̨ͬ������Ϣ��������ʾ�û�
			if(m_lpFMGClient->m_UserMgr.m_UserInfo.m_uUserID != nSenderID)
				::PostMessage(m_lpFMGClient->m_UserMgr.m_hProxyWnd, FMG_MSG_GROUP_MSG, 0, (LPARAM)lpMsg);
		}
		else
		{
			//����ƽ̨ͬ������Ϣ
			if(m_lpFMGClient->m_UserMgr.m_UserInfo.m_uUserID == nSenderID)
			{
				::PostMessage(m_lpFMGClient->m_UserMgr.m_hProxyWnd, FMG_MSG_BUDDY_MSG, 0, (LPARAM)lpMsg);
			}	
			else
			{
				//�������ѷ�������Ϣ
				if(nTargetID == m_lpFMGClient->m_UserMgr.m_UserInfo.m_uUserID)
					::PostMessage(m_lpFMGClient->m_UserMgr.m_hProxyWnd, FMG_MSG_BUDDY_MSG, 0, (LPARAM)lpMsg);
				else
				{
					lpMsg->m_nFromUin = nTargetID;
					::PostMessage(m_lpFMGClient->m_UserMgr.m_hProxyWnd, FMG_MSG_BUDDY_MSG, 0, (LPARAM)lpMsg);
				}
			}
		}
	}
	
    //TODO: �ǵ��ڶ�Ӧ��Ŀ�괰�ڳ�ɾ������������ڴ�й¶
	delete pFileItem;

	m_pCurrentTransferringItem = NULL;
}

long CFileTask::UploadFile(PCTSTR pszFileName, HWND hwndReflection, HANDLE hCancelEvent, CUploadFileResult& uploadFileResult)
{
	_tcscpy_s(uploadFileResult.m_szLocalName, ARRAYSIZE(uploadFileResult.m_szLocalName), pszFileName);

	//�ļ�md5ֵ
	char szMd5[64] = { 0 };
	//TODO: ��ʾ�û�����У���ļ�
	FileProgress* pFileProgress = NULL;
	pFileProgress = new FileProgress();
	memset(pFileProgress, 0, sizeof(FileProgress));
	pFileProgress->nPercent = -1;
	pFileProgress->nVerificationPercent = 0;
	_tcscpy_s(pFileProgress->szDestPath, ARRAYSIZE(pFileProgress->szDestPath), pszFileName);
	::PostMessage(hwndReflection, FMG_MSG_SEND_FILE_PROGRESS, 0, (LPARAM)pFileProgress);
	long nRetCode = GetFileMd5ValueA(pszFileName, szMd5, ARRAYSIZE(szMd5), hwndReflection, hCancelEvent);
	if (nRetCode == GET_FILE_MD5_FAILED)
	{
		CIULog::Log(LOG_ERROR, __FUNCSIG__, _T("Failed to upload file:%s as unable to get file md5."), pszFileName);
		return FILE_UPLOAD_FAILED;
	}
	else if (nRetCode == GET_FILE_MD5_USERCANCEL)
	{
		CIULog::Log(LOG_NORMAL, __FUNCSIG__, _T("User cancel to upload file:%s."), pszFileName);
		return FILE_UPLOAD_USERCANCEL;
	}

	strcpy_s(uploadFileResult.m_szMd5, ARRAYSIZE(uploadFileResult.m_szMd5), szMd5);

	HANDLE	hFile = ::CreateFile(pszFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		CIULog::Log(LOG_ERROR, __FUNCSIG__, _T("Failed to upload file:%s as unable to open the file."), pszFileName);
		return FILE_UPLOAD_FAILED;
	}

	//����hFile�ں������ý���ʱ�Զ��ر�
	CAutoFileHandle autoFile(hFile);

	//�ļ�utf8��ʽ����
	char szUtf8Name[MAX_PATH] = { 0 };
	UnicodeToUtf8(::PathFindFileName(pszFileName), szUtf8Name, ARRAYSIZE(szUtf8Name));

	//�ļ���С
	DWORD nFileSize = ::GetFileSize(hFile, NULL);
	//�ļ�̫��ת����nFileSize���������nFileSizeֵΪ����
	if (nFileSize < 0)
	{
		CIULog::Log(LOG_ERROR, __FUNCSIG__, _T("Failed to upload file:%s as file is too big."), pszFileName);
		return FILE_UPLOAD_FAILED;
	}
	uploadFileResult.m_dwFileSize = nFileSize;

    size_t offsetX = 0;
    size_t t = 9;
    size_t offset2 = 0;

    while (true)
    {
        std::string outbuf;
        yt::BinaryWriteStream3 writeStream(&outbuf);
        writeStream.Write(msg_type_upload_req);
        writeStream.Write(m_seq);
        writeStream.Write(szMd5, 32);
        writeStream.Write((int)offsetX);
        writeStream.Write((int)nFileSize);
        size_t eachfilesize = 512* 1024;
        if (nFileSize - offsetX < eachfilesize)
            eachfilesize = nFileSize - offsetX;
        CMiniBuffer buffer(eachfilesize);
        DWORD dwFileRead;
        if (!::ReadFile(hFile, buffer.GetBuffer(), (DWORD)eachfilesize, &dwFileRead, NULL) || eachfilesize != dwFileRead)
            break;
        string filedata;
        filedata.append(buffer.GetBuffer(), buffer.GetSize());
        writeStream.Write(filedata.c_str(), filedata.length());
        writeStream.Flush();
        file_msg headerx = { outbuf.length() };
        outbuf.insert(0, (const char*)&headerx, sizeof(headerx));
        if (!m_SocketClient->SendOnFilePort(outbuf.c_str(), outbuf.length()))
            break;

        offsetX += eachfilesize;
        pFileProgress = new FileProgress();
        memset(pFileProgress, 0, sizeof(FileProgress));
        //AtlTrace(_T("nTotalSent:%d\n"), nTotalSent);
        //AtlTrace(_T("nFileSize:%d\n"), nFileSize);
        //nTotalSent*100���ܻᳬ��long�ķ�Χ��������ʱת����__int64
        pFileProgress->nPercent = (long)((__int64)offsetX* 100 / nFileSize);
        //AtlTrace(_T("pFileProgress->nPercent:%d\n"), pFileProgress->nPercent);
        _tcscpy_s(pFileProgress->szDestPath, ARRAYSIZE(pFileProgress->szDestPath), pszFileName);
        ::PostMessage(hwndReflection, FMG_MSG_SEND_FILE_PROGRESS, 0, (LPARAM)pFileProgress);

        file_msg header;
        if (!m_SocketClient->RecvOnFilePort((char*)&header, sizeof(header)))
            break;

        CMiniBuffer recvBuf((long)header.packagesize);
        if (!m_SocketClient->RecvOnFilePort(recvBuf.GetBuffer(), recvBuf.GetSize()))
            break;

        yt::BinaryReadStream2 readStream(recvBuf.GetBuffer(), recvBuf.GetSize());
        int cmd;
        if (!readStream.Read(cmd) || cmd != msg_type_upload_resp)
            break;

        //int seq;
        if (!readStream.Read(m_seq))
            break;

        std::string filemd5;
        size_t md5length;
        if (!readStream.Read(&filemd5, 0, md5length) || md5length != 32)
            break;

        int offset;
        if (!readStream.Read(offset))
            break;

        int filesize;
        if (!readStream.Read(filesize))
            break;

        string dummyfiledata;
        size_t filedatalength;
        if (!readStream.Read(&dummyfiledata, 0, filedatalength) || filedatalength != 0)
            break;

        if (offset == -1 && filesize == -1)
        {
            FillUploadFileResult(uploadFileResult, pszFileName, filemd5.c_str(), nFileSize, szMd5);
            CIULog::Log(LOG_NORMAL, __FUNCSIG__, _T("Succeed to upload file:%s as there already exist file on server."), pszFileName);

            //TODO: ����ⲿ���ͷ�������ڴ�й¶
            pFileProgress = new FileProgress();
            memset(pFileProgress, 0, sizeof(FileProgress));
            pFileProgress->nPercent = 100;
            _tcscpy_s(pFileProgress->szDestPath, ARRAYSIZE(pFileProgress->szDestPath), pszFileName);
            ::PostMessage(hwndReflection, FMG_MSG_SEND_FILE_PROGRESS, 0, (LPARAM)pFileProgress);
            return FILE_UPLOAD_SUCCESS;
        }

    }


	return FILE_UPLOAD_FAILED;
}

void CFileTask::FillUploadFileResult(CUploadFileResult& uploadFileResult, PCTSTR pszLocalName, PCSTR pszRemoteName, long nFileSize, char* pszMd5)
{
	uploadFileResult.m_bSuccessful = TRUE;
	uploadFileResult.m_dwFileSize = (DWORD)nFileSize;
	_tcscpy_s(uploadFileResult.m_szLocalName, ARRAYSIZE(uploadFileResult.m_szLocalName), pszLocalName);
	strcpy_s(uploadFileResult.m_szRemoteName, ARRAYSIZE(uploadFileResult.m_szRemoteName), pszRemoteName);
	strcpy_s(uploadFileResult.m_szMd5, ARRAYSIZE(uploadFileResult.m_szMd5), pszMd5);
}

long CFileTask::DownloadFile3(LPCSTR lpszFileName, LPCTSTR lpszDestPath, BOOL bOverwriteIfExist, HWND hwndReflection, HANDLE hCancelEvent)
{
    //TODO: ȷ���Ƿ񸲸ǵķ���Ӧ���Ǹ���md5ֵ���жϱ��ص��ļ������ص��ļ��Ƿ���ȫ��ͬ
    if (Edoyun::CPath::IsFileExist(lpszDestPath) && IUGetFileSize2(lpszDestPath)>0 && !bOverwriteIfExist)
    {
        CIULog::Log(LOG_NORMAL, __FUNCSIG__, _T("File %s already exsited, there is no need to download."), lpszDestPath);
        return FILE_DOWNLOAD_SUCCESS;
    }

    //ƫ����
    long nOffset = 0;
    long nBreakType = FILE_DOWNLOAD_SUCCESS;

    char* pBuffer = NULL;
    //nCurrentContentSizeΪ��ǰ�յ��İ����ļ������ֽ�����С
    long nCurrentContentSize = 0;
    DWORD dwSizeToWrite = 0;
    DWORD dwSizeWritten = 0;
    BOOL bRet = TRUE;
    long nFileSize = 0;

    HANDLE hFile = ::CreateFile(lpszDestPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    //AtlTrace(_T("lpszDestPath:%s.\n"), lpszDestPath);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        CIULog::Log(LOG_ERROR, __FUNCSIG__, _T("Failed to download file %s as unable to create the file."), lpszDestPath);
        return FILE_DOWNLOAD_FAILED;
    }
    CAutoFileHandle autoFileHandle(hFile);
    FileProgress* pFileProgress = NULL;
    
    size_t offset = 0;
    while (true)
    {
        std::string outbuf;
        yt::BinaryWriteStream3 writeStream(&outbuf);
        writeStream.Write(msg_type_download_req);
        writeStream.Write(m_seq);
        writeStream.Write(lpszFileName, strlen(lpszFileName));
        size_t dummyoffset = 0;
        writeStream.Write((int)dummyoffset);
        size_t dummyfilesize = 0;
        writeStream.Write((int)dummyfilesize);
        string dummyfiledata;
        writeStream.Write(dummyfiledata.c_str(), dummyfiledata.length());
        writeStream.Flush();

        file_msg header = { outbuf.length() };
        outbuf.insert(0, (const char*)&header, sizeof(header));

        if (!m_SocketClient->SendOnFilePort(outbuf.c_str(), outbuf.length()))
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        file_msg recvheader;
        if (!m_SocketClient->RecvOnFilePort((char*)&recvheader, sizeof(recvheader)))
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        CMiniBuffer buffer((long)recvheader.packagesize);
        if (!m_SocketClient->RecvOnFilePort(buffer.GetBuffer(), (long)recvheader.packagesize))
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        yt::BinaryReadStream2 readStream(buffer.GetBuffer(), (size_t)recvheader.packagesize);
        int cmd;
        if (!readStream.Read(cmd) || cmd != msg_type_download_resp)
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        //int seq;
        if (!readStream.Read(m_seq))
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        std::string filemd5;
        size_t md5length;
        if (!readStream.Read(&filemd5, 0, md5length) || md5length == 0)
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        int offset;
        if (!readStream.Read(offset))
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        int filesize;
        if (!readStream.Read(filesize))
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        string filedata;
        size_t filedatalength;
        if (!readStream.Read(&filedata, 0, filedatalength) || filedatalength == 0)
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        DWORD dwBytesWritten;
        if (!::WriteFile(hFile, filedata.c_str(), filedata.length(), &dwBytesWritten, NULL) || dwBytesWritten != filedata.length())
        {
            nBreakType = FILE_DOWNLOAD_FAILED;
            break;
        }

        offset += filedata.length();

        //��������TODO: ���ڷ���������ͼƬ������ڴ���Ϊδ�ͷŶ������ڴ�й¶��������
        pFileProgress = new FileProgress();
        memset(pFileProgress, 0, sizeof(FileProgress));
        _tcscpy_s(pFileProgress->szDestPath, ARRAYSIZE(pFileProgress->szDestPath), lpszDestPath);
        pFileProgress->nPercent = long(((__int64)offset* 100) / filesize);
        ::PostMessage(hwndReflection, FMG_MSG_RECV_FILE_PROGRESS, 0, (LPARAM)(pFileProgress));


        if (offset == filesize)
        {
            nBreakType = FILE_DOWNLOAD_SUCCESS;
            break;
        }
    }// end while-loop

    //���سɹ�
    if (nBreakType == FILE_DOWNLOAD_SUCCESS)
    {
        //::CloseHandle(hFile);
        CIULog::Log(LOG_NORMAL, _T("Succeed to download file: %s."), lpszDestPath);
        pFileProgress = new FileProgress();
        memset(pFileProgress, 0, sizeof(FileProgress));
        _tcscpy_s(pFileProgress->szDestPath, ARRAYSIZE(pFileProgress->szDestPath), lpszDestPath);
        pFileProgress->nPercent = /*nOffset* 100 / nFileSize*/100;
        ::PostMessage(hwndReflection, FMG_MSG_RECV_FILE_PROGRESS, 0, (LPARAM)(pFileProgress));
    }
    //����ʧ�ܻ����û�ȡ������
    else
    {
        if (nBreakType == FILE_DOWNLOAD_FAILED)
            CIULog::Log(LOG_ERROR, _T("Failed to download file: %s."), lpszDestPath);
        else
            CIULog::Log(LOG_NORMAL, __FUNCSIG__, _T("User canceled to download file: %s."), lpszDestPath);
        //Ϊ����ɾ�����صİ��Ʒ����ʽ�ر��ļ����
        autoFileHandle.Release();
        ::DeleteFile(lpszDestPath);
    }

    return nBreakType;
}

BOOL CFileTask::UploadUserThumb(PCTSTR pszFileName, HWND hwndReflection, CUploadFileResult& uploadFileResult)
{
    _tcscpy_s(uploadFileResult.m_szLocalName, ARRAYSIZE(uploadFileResult.m_szLocalName), pszFileName);

    //�ļ�md5ֵ
    char szMd5[64] = { 0 };
    //TODO: ��ʾ�û�����У���ļ�
    FileProgress* pFileProgress = NULL;
    pFileProgress = new FileProgress();
    memset(pFileProgress, 0, sizeof(FileProgress));
    pFileProgress->nPercent = -1;
    pFileProgress->nVerificationPercent = 0;
    _tcscpy_s(pFileProgress->szDestPath, ARRAYSIZE(pFileProgress->szDestPath), pszFileName);
    ::PostMessage(hwndReflection, FMG_MSG_SEND_FILE_PROGRESS, 0, (LPARAM)pFileProgress);
    long nRetCode = GetFileMd5ValueA(pszFileName, szMd5, ARRAYSIZE(szMd5), hwndReflection, NULL);
    if (nRetCode == GET_FILE_MD5_FAILED)
    {
        CIULog::Log(LOG_ERROR, __FUNCSIG__, _T("Failed to upload file:%s as unable to get file md5."), pszFileName);
        return UPLOAD_USER_THUMB_RESULT_FAILED;
    }
    else if (nRetCode == GET_FILE_MD5_USERCANCEL)
    {
        CIULog::Log(LOG_NORMAL, __FUNCSIG__, _T("User cancel to upload file:%s."), pszFileName);
        return FILE_UPLOAD_USERCANCEL;
    }

    strcpy_s(uploadFileResult.m_szMd5, ARRAYSIZE(uploadFileResult.m_szMd5), szMd5);

    HANDLE	hFile = ::CreateFile(pszFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        CIULog::Log(LOG_ERROR, __FUNCSIG__, _T("Failed to upload file:%s as unable to open the file."), pszFileName);
        return UPLOAD_USER_THUMB_RESULT_FAILED;
    }

    //����hFile�ں������ý���ʱ�Զ��ر�
    CAutoFileHandle autoFile(hFile);

    //�ļ�utf8��ʽ����
    char szUtf8Name[MAX_PATH] = { 0 };
    UnicodeToUtf8(::PathFindFileName(pszFileName), szUtf8Name, ARRAYSIZE(szUtf8Name));

    //�ļ���С
    DWORD nFileSize = ::GetFileSize(hFile, NULL);
    //�ļ�̫��ת����nFileSize���������nFileSizeֵΪ����
    if (nFileSize < 0)
    {
        CIULog::Log(LOG_ERROR, __FUNCSIG__, _T("Failed to upload file:%s as file is too big."), pszFileName);
        return UPLOAD_USER_THUMB_RESULT_FAILED;
    }
    uploadFileResult.m_dwFileSize = nFileSize;

    size_t offsetX = 0;
    size_t t = 9;
    size_t offset2 = 0;

    while (true)
    {
        std::string outbuf;
        yt::BinaryWriteStream3 writeStream(&outbuf);
        writeStream.Write(msg_type_upload_req);
        writeStream.Write(m_seq);
        writeStream.Write(szMd5, 32);
        writeStream.Write((int)offsetX);
        writeStream.Write((int)nFileSize);
        size_t eachfilesize = 512* 1024;
        if (nFileSize - offsetX < eachfilesize)
            eachfilesize = nFileSize - offsetX;
        CMiniBuffer buffer(eachfilesize);
        DWORD dwFileRead;
        if (!::ReadFile(hFile, buffer.GetBuffer(), (DWORD)eachfilesize, &dwFileRead, NULL) || eachfilesize != dwFileRead)
            break;
        string filedata;
        filedata.append(buffer.GetBuffer(), buffer.GetSize());
        writeStream.Write(filedata.c_str(), filedata.length());
        writeStream.Flush();
        file_msg headerx = { outbuf.length() };
        outbuf.insert(0, (const char*)&headerx, sizeof(headerx));
        if (!m_SocketClient->SendOnFilePort(outbuf.c_str(), outbuf.length()))
            break;

        offsetX += eachfilesize;
        pFileProgress = new FileProgress();
        memset(pFileProgress, 0, sizeof(FileProgress));
        //AtlTrace(_T("nTotalSent:%d\n"), nTotalSent);
        //AtlTrace(_T("nFileSize:%d\n"), nFileSize);
        //nTotalSent*100���ܻᳬ��long�ķ�Χ��������ʱת����__int64
        pFileProgress->nPercent = (long)((__int64)offsetX* 100 / nFileSize);
        //AtlTrace(_T("pFileProgress->nPercent:%d\n"), pFileProgress->nPercent);
        _tcscpy_s(pFileProgress->szDestPath, ARRAYSIZE(pFileProgress->szDestPath), pszFileName);
        ::PostMessage(hwndReflection, FMG_MSG_SEND_FILE_PROGRESS, 0, (LPARAM)pFileProgress);

        file_msg header;
        if (!m_SocketClient->RecvOnFilePort((char*)&header, sizeof(header)))
            break;

        CMiniBuffer recvBuf((long)header.packagesize);
        if (!m_SocketClient->RecvOnFilePort(recvBuf.GetBuffer(), recvBuf.GetSize()))
            break;

        yt::BinaryReadStream2 readStream(recvBuf.GetBuffer(), recvBuf.GetSize());
        int cmd;
        if (!readStream.Read(cmd) || cmd != msg_type_upload_resp)
            break;

        //int seq;
        if (!readStream.Read(m_seq))
            break;

        std::string filemd5;
        size_t md5length;
        if (!readStream.Read(&filemd5, 0, md5length) || md5length != 32)
            break;

        int offset;
        if (!readStream.Read(offset))
            break;

        int filesize;
        if (!readStream.Read(filesize))
            break;

        string dummyfiledata;
        size_t filedatalength;
        if (!readStream.Read(&dummyfiledata, 0, filedatalength) || filedatalength != 0)
            break;

        if (offset == -1 && filesize == -1)
        {
            FillUploadFileResult(uploadFileResult, pszFileName, filemd5.c_str(), nFileSize, szMd5);
            CIULog::Log(LOG_NORMAL, __FUNCSIG__, _T("Succeed to upload file:%s as there already exist file on server."), pszFileName);

            //TODO: ����ⲿ���ͷ�������ڴ�й¶
            pFileProgress = new FileProgress();
            memset(pFileProgress, 0, sizeof(FileProgress));
            pFileProgress->nPercent = 100;
            _tcscpy_s(pFileProgress->szDestPath, ARRAYSIZE(pFileProgress->szDestPath), pszFileName);
            ::PostMessage(hwndReflection, FMG_MSG_SEND_FILE_PROGRESS, 0, (LPARAM)pFileProgress);
            return UPLOAD_USER_THUMB_RESULT_SUCCESS;
        }

    }


    return UPLOAD_USER_THUMB_RESULT_FAILED;
}

BOOL CFileTask::DownloadFileSynchronously(LPCSTR lpszFileName, LPCTSTR lpszDestPath, BOOL bOverwriteIfExist)
{
    return DownloadFile3(lpszFileName, lpszDestPath, bOverwriteIfExist, NULL, NULL) == FILE_DOWNLOAD_SUCCESS;
}