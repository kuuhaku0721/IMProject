#include "stdafx.h"
#include "EdoyunIMClient.h"
#include "IniFile.h"
#include "LoginDlg.h"
#include "net/IUProtocolData.h"
#include "IULog.h"
#include "UpdateDlg.h"
#include "EncodingUtil.h"
#include "UserSessionData.h"
#include "CustomMsgDef.h"
#include "Startup.h"
#include "File.h"

CEdoyunClient& CEdoyunClient::GetInstance()
{
	static CEdoyunClient client;
	return client;
}


CEdoyunClient::CEdoyunClient(void) :
	m_SocketClient(&m_RecvMsgThread),
	m_SendMsgThread(&m_SocketClient),
	m_RecvMsgThread(&m_SocketClient),
	m_FileTask(&m_SocketClient)
{
	m_hwndCreateNewGroup = NULL;
	m_ServerTime = 0;

	m_bNetworkAvailable = TRUE;

	m_hwndRegister = NULL;
	m_hwndFindFriend = NULL;
	m_hwndModifyPassword = NULL;

	m_bBuddyIDsAvailable = FALSE;
	m_bBuddyListAvailable = FALSE;

	m_nGroupCount = 0;
	m_bGroupInfoAvailable = FALSE;
	m_bGroupMemberInfoAvailable = FALSE;

	m_SendMsgThread.m_lpUserMgr = &m_UserMgr;
	m_RecvMsgThread.m_lpUserMgr = &m_UserMgr;
	m_RecvMsgThread.m_pFMGClient = this;
	m_FileTask.m_lpFMGClient = this;
}

CEdoyunClient::~CEdoyunClient(void)
{

}

// ��ʼ���ͻ���
BOOL CEdoyunClient::Init()
{
	BOOL bRet = CreateProxyWnd();	// ����������
	if (!bRet)
		return FALSE;

	m_SocketClient.Init();

	m_SendMsgThread.Start();
	m_RecvMsgThread.Start();

	m_SendMsgThread.m_lpFMGClient = this;

	m_FileTask.Start();

	return TRUE;
}

// ����ʼ���ͻ���
void CEdoyunClient::UnInit()
{
	DestroyProxyWnd();				// ���ٴ�����

	m_SocketClient.Uninit();
	m_SendMsgThread.Stop();
	m_RecvMsgThread.Stop();
	m_FileTask.Stop();
	try {
		m_SocketClient.Join();
		m_SendMsgThread.Join();
		m_RecvMsgThread.Join();
	}
	catch (...) {
		//TRACE("exception found");
	}
	m_FileTask.Join();
}

void CEdoyunClient::SetServer(PCTSTR pszServer)
{
	m_SocketClient.SetServer(pszServer);
}

void CEdoyunClient::SetFileServer(PCTSTR pszServer)
{
	m_SocketClient.SetFileServer(pszServer);
}

void CEdoyunClient::SetPort(short port)
{
	m_SocketClient.SetPort(port);
}

void CEdoyunClient::SetFilePort(short port)
{
	m_SocketClient.SetFilePort(port);
}


// ����UTalk���������
void CEdoyunClient::SetUser(LPCTSTR lpUserAccount, LPCTSTR lpUserPwd)
{
	if (NULL == lpUserAccount || NULL == lpUserPwd || m_UserMgr.m_UserInfo.m_nStatus != STATUS_OFFLINE)
		return;

	m_UserMgr.m_UserInfo.m_strAccount = lpUserAccount;
	m_UserMgr.m_UserInfo.m_strPassword = lpUserPwd;
}

// ���õ�¼״̬
void CEdoyunClient::SetLoginStatus(long nStatus)
{
	m_UserMgr.m_UserInfo.m_nStatus = nStatus;
}

// ���ûص����ھ��
void CEdoyunClient::SetCallBackWnd(HWND hCallBackWnd)
{
	m_UserMgr.m_hCallBackWnd = hCallBackWnd;
}

void CEdoyunClient::SetRegisterWindow(HWND hwndRegister)
{
	m_hwndRegister = hwndRegister;
}

void CEdoyunClient::SetModifyPasswordWindow(HWND hwndModifyPassword)
{
	m_hwndModifyPassword = hwndModifyPassword;
}

void CEdoyunClient::SetCreateNewGroupWindow(HWND hwndCreateNewGroup)
{
	m_hwndCreateNewGroup = hwndCreateNewGroup;
}

void CEdoyunClient::Register(PCTSTR pszAccountName, PCTSTR pszNickName, PCTSTR pszPassword)
{
	//TODO: �����ʱ��������ֱ�ӷ���
	CRegisterRequest* pRequest = new CRegisterRequest();
	char szData[64] = { 0 };
	UnicodeToUtf8(pszAccountName, szData, ARRAYSIZE(szData));
	strcpy_s(pRequest->m_szAccountName, ARRAYSIZE(pRequest->m_szAccountName), szData);

	memset(szData, 0, sizeof(szData));
	UnicodeToUtf8(pszNickName, szData, ARRAYSIZE(szData));
	strcpy_s(pRequest->m_szNickName, ARRAYSIZE(pRequest->m_szNickName), szData);

	memset(szData, 0, sizeof(szData));
	UnicodeToUtf8(pszPassword, szData, ARRAYSIZE(szData));
	strcpy_s(pRequest->m_szPassword, ARRAYSIZE(pRequest->m_szPassword), szData);

	m_SendMsgThread.AddItem(pRequest);
}

// ���Һ���
BOOL CEdoyunClient::FindFriend(PCTSTR pszAccountName, long nType, HWND hReflectionWnd)
{
	//TODO: ���ж�һ���Ƿ�����

	if (pszAccountName == NULL || *pszAccountName == NULL)
		return FALSE;

	m_hwndFindFriend = hReflectionWnd;

	CFindFriendRequest* pRequest = new CFindFriendRequest();
	char szData[64] = { 0 };
	UnicodeToUtf8(pszAccountName, szData, ARRAYSIZE(szData));
	strcpy_s(pRequest->m_szAccountName, ARRAYSIZE(pRequest->m_szAccountName), szData);
	pRequest->m_nType = nType;

	m_SendMsgThread.AddItem(pRequest);

	return TRUE;
}

BOOL CEdoyunClient::AddFriend(UINT uAccountToAdd)
{
	//TODO: ���ж��Ƿ�����
	if (uAccountToAdd == 0)
		return FALSE;

	COperateFriendRequest* pRequest = new COperateFriendRequest();
	pRequest->m_uCmd = Apply;
	pRequest->m_uAccountID = uAccountToAdd;
	//m_mapAddFriendCache.insert(std::pair<UINT, UINT>(uAccountToAdd, Apply));

	m_SendMsgThread.AddItem(pRequest);
	return TRUE;
}

// ɾ������
BOOL CEdoyunClient::DeleteFriend(UINT uAccountID)
{
	//TODO: ���ж��Ƿ�����
	COperateFriendRequest* pRequest = new COperateFriendRequest();
	pRequest->m_uCmd = Delete;
	pRequest->m_uAccountID = uAccountID;

	m_SendMsgThread.AddItem(pRequest);

	return TRUE;
}

BOOL CEdoyunClient::UpdateLogonUserInfo(PCTSTR pszNickName,
	PCTSTR pszSignature,
	UINT uGender,
	long nBirthday,
	PCTSTR pszAddress,
	PCTSTR pszPhone,
	PCTSTR pszMail,
	UINT uSysFaceID,
	PCTSTR pszCustomFacePath,
	BOOL bUseCustomThumb)
{
	if (!m_bNetworkAvailable)
		return FALSE;

	CUpdateLogonUserInfoRequest* pRequest = new CUpdateLogonUserInfoRequest();
	char szData[512] = { 0 };
	if (pszNickName != NULL && *pszNickName != NULL)
	{
		UnicodeToUtf8(pszNickName, szData, ARRAYSIZE(szData));
		strcpy_s(pRequest->m_szNickName, ARRAYSIZE(pRequest->m_szNickName), szData);
	}

	if (pszSignature != NULL && *pszSignature != NULL)
	{
		memset(szData, 0, sizeof(szData));
		UnicodeToUtf8(pszSignature, szData, ARRAYSIZE(szData));
		strcpy_s(pRequest->m_szSignature, ARRAYSIZE(pRequest->m_szSignature), szData);
	}

	if (pszAddress != NULL && *pszAddress != NULL)
	{
		memset(szData, 0, sizeof(szData));
		UnicodeToUtf8(pszAddress, szData, ARRAYSIZE(szData));
		strcpy_s(pRequest->m_szAddress, ARRAYSIZE(pRequest->m_szAddress), szData);
	}

	if (pszPhone != NULL && *pszPhone != NULL)
	{
		memset(szData, 0, sizeof(szData));
		UnicodeToUtf8(pszPhone, szData, ARRAYSIZE(szData));
		strcpy_s(pRequest->m_szPhone, ARRAYSIZE(pRequest->m_szPhone), szData);
	}

	if (pszMail != NULL && *pszMail != NULL)
	{
		memset(szData, 0, sizeof(szData));
		UnicodeToUtf8(pszMail, szData, ARRAYSIZE(szData));
		strcpy_s(pRequest->m_szMail, ARRAYSIZE(pRequest->m_szMail), szData);
	}

	pRequest->m_uGender = uGender;
	pRequest->m_nBirthday = nBirthday;
	pRequest->m_uFaceID = uSysFaceID;

	if (!bUseCustomThumb)
	{
		pRequest->m_bUseCustomThumb = FALSE;
		m_UserMgr.m_UserInfo.m_strCustomFace = _T("");
		m_UserMgr.m_UserInfo.m_nFace = uSysFaceID;
		m_UserMgr.m_UserInfo.m_bUseCustomFace = FALSE;
	}
	else
	{
		pRequest->m_bUseCustomThumb = TRUE;
		if (pszCustomFacePath != NULL && &pszCustomFacePath != NULL)
		{
			_tcscpy_s(pRequest->m_szCustomFace, ARRAYSIZE(pRequest->m_szCustomFace), pszCustomFacePath);
		}
		else
		{
			pRequest->m_bUseCustomThumb = FALSE;
			m_UserMgr.m_UserInfo.m_strCustomFace = _T("");
			m_UserMgr.m_UserInfo.m_bUseCustomFace = FALSE;
		}
	}

	m_SendMsgThread.AddItem(pRequest);

	return TRUE;
}

void CEdoyunClient::StartCheckNetworkStatusTask()
{
	m_CheckNetworkStatusTask.m_pTalkClient = this;
}

void CEdoyunClient::StartHeartbeatTask()
{
	//TODO:
}


void CEdoyunClient::SendHeartbeatMessage()
{
	if (!m_bNetworkAvailable)
		return;

	CHeartbeatMessageRequest* pRequest = new CHeartbeatMessageRequest();
	//m_SendMsgTask.AddItem(pRequest);
}

void CEdoyunClient::ModifyPassword(PCTSTR pszOldPassword, PCTSTR pszNewPassword)
{
	if (!m_bNetworkAvailable)
		return;

	char szData[64] = { 0 };
	CModifyPasswordRequest* pRequest = new CModifyPasswordRequest();
	UnicodeToUtf8(pszOldPassword, szData, ARRAYSIZE(szData));
	strcpy_s(pRequest->m_szOldPassword, ARRAYSIZE(pRequest->m_szOldPassword), szData);
	memset(szData, 0, sizeof(szData));
	UnicodeToUtf8(pszNewPassword, szData, ARRAYSIZE(szData));
	strcpy_s(pRequest->m_szNewPassword, ARRAYSIZE(pRequest->m_szNewPassword), szData);
	m_SendMsgThread.AddItem(pRequest);
}

void CEdoyunClient::CreateNewGroup(PCTSTR pszGroupName)
{
	if (!m_bNetworkAvailable)
		return;

	char szData[64] = { 0 };
	CCreateNewGroupRequest* pRequest = new CCreateNewGroupRequest();
	UnicodeToUtf8(pszGroupName, szData, ARRAYSIZE(szData));
	strcpy_s(pRequest->m_szGroupName, ARRAYSIZE(pRequest->m_szGroupName), szData);
	m_SendMsgThread.AddItem(pRequest);
}

void CEdoyunClient::ResponseAddFriendApply(UINT uAccountID, UINT uCmd)
{
	if (uCmd != Agree && uCmd != Refuse)
		return;

	COperateFriendRequest* pRequest = new COperateFriendRequest();

	pRequest->m_uAccountID = uAccountID;
	pRequest->m_uCmd = uCmd;

	//m_mapAddFriendCache.insert(std::pair<UINT, UINT>(uAccountID, uCmd));

	m_SendMsgThread.AddItem(pRequest);
}

// ��¼
void CEdoyunClient::Login()
{
	if (!IsOffline() || m_UserMgr.m_UserInfo.m_strAccount.empty() || m_UserMgr.m_UserInfo.m_strPassword.empty())
		return;

	CLoginRequest* pLoginRequest = new CLoginRequest();
	if (pLoginRequest == NULL)
		return;

	char szData[64] = { 0 };
	UnicodeToUtf8(m_UserMgr.m_UserInfo.m_strAccount.c_str(), szData, ARRAYSIZE(szData));
	strcpy_s(pLoginRequest->m_szAccountName, ARRAYSIZE(pLoginRequest->m_szAccountName), szData);

	memset(szData, 0, sizeof(szData));
	UnicodeToUtf8(m_UserMgr.m_UserInfo.m_strPassword.c_str(), szData, ARRAYSIZE(szData));
	strcpy_s(pLoginRequest->m_szPassword, ARRAYSIZE(pLoginRequest->m_szPassword), szData);

	pLoginRequest->m_nStatus = STATUS_ONLINE;

	if (IsMobileNumber(m_UserMgr.m_UserInfo.m_strAccount.c_str()))
		pLoginRequest->m_nLoginType = LOGIN_USE_MOBILE_NUMBER;
	else
		pLoginRequest->m_nLoginType = LOGIN_USE_ACCOUNT;


	m_SendMsgThread.AddItem(pLoginRequest);
}

void CEdoyunClient::GetFriendList()
{
	CUserBasicInfoRequest* pBasicInfoRequest = new CUserBasicInfoRequest();
	pBasicInfoRequest->m_setAccountID.insert(m_UserMgr.m_UserInfo.m_uUserID);
	m_SendMsgThread.AddItem(pBasicInfoRequest);
}

void CEdoyunClient::GetGroupMembers(int32_t groupid)
{
	CGroupBasicInfoRequest* pGroupInfoRequest = new CGroupBasicInfoRequest();
	pGroupInfoRequest->m_groupid = groupid;
	m_SendMsgThread.AddItem(pGroupInfoRequest);
}

// ע��
BOOL CEdoyunClient::Logout()
{
	if (IsOffline())
		return FALSE;

	return FALSE;
}

// ȡ����¼
void CEdoyunClient::CancelLogin()
{
	m_SendMsgThread.DeleteAllItems();
}

void CEdoyunClient::SetFindFriendWindow(HWND hwndFindFriend)
{
	m_hwndFindFriend = hwndFindFriend;
}

// �ı�����״̬
void CEdoyunClient::ChangeStatus(long nStatus)
{
	if (IsOffline())
		return;
}

// ���º����б�
void CEdoyunClient::UpdateBuddyList()
{
	if (IsOffline())
		return;
}

// ����Ⱥ�б�
void CEdoyunClient::UpdateGroupList()
{
	if (IsOffline())
		return;
}

// ���������ϵ���б�
void CEdoyunClient::UpdateRecentList()
{
	if (IsOffline())
		return;
}

// ���º�����Ϣ
void CEdoyunClient::UpdateBuddyInfo(UINT nUTalkUin)
{
	if (IsOffline())
		return;
}

// ����Ⱥ��Ա��Ϣ
void CEdoyunClient::UpdateGroupMemberInfo(UINT nGroupCode, UINT nUTalkUin)
{
	if (IsOffline())
		return;
}

// ����Ⱥ��Ϣ
void CEdoyunClient::UpdateGroupInfo(UINT nGroupCode)
{
	if (IsOffline())
		return;

}

// ���º��Ѻ���
void CEdoyunClient::UpdateBuddyNum(UINT nUTalkUin)
{
	if (IsOffline())
		return;

	//TODO:
}

// ����Ⱥ��Ա����
void CEdoyunClient::UpdateGroupMemberNum(UINT nGroupCode, UINT nUTalkUin)
{
	if (IsOffline())
		return;

	//TODO
}

// ����Ⱥ��Ա����
void CEdoyunClient::UpdateGroupMemberNum(UINT nGroupCode, std::vector<UINT>* arrUTalkUin)
{
	//TODO
}

// ����Ⱥ����
void CEdoyunClient::UpdateGroupNum(UINT nGroupCode)
{
	if (IsOffline())
		return;

	//TODO:
}

// ���º��Ѹ���ǩ��
void CEdoyunClient::UpdateBuddySign(UINT nUTalkUin)
{
	if (IsOffline())
		return;

	//TODO:
}

// ����Ⱥ��Ա����ǩ��
void CEdoyunClient::UpdateGroupMemberSign(UINT nGroupCode, UINT nUTalkUin)
{
	if (IsOffline())
		return;

	//TODO:
}

// �޸�UTalk����ǩ��
void CEdoyunClient::ModifyUTalkSign(LPCTSTR lpSign)
{
	const CBuddyInfo& currentLogonUser = m_UserMgr.m_UserInfo;

	UpdateLogonUserInfo(currentLogonUser.m_strNickName.c_str(),
		lpSign,
		currentLogonUser.m_nGender,
		currentLogonUser.m_nBirthday,
		currentLogonUser.m_strAddress.c_str(),
		currentLogonUser.m_strMobile.c_str(),
		currentLogonUser.m_strEmail.c_str(),
		currentLogonUser.m_nFace,
		currentLogonUser.m_strRawCustomFace.c_str(),
		currentLogonUser.m_bUseCustomFace);
}

// ���º���ͷ��
void CEdoyunClient::UpdateBuddyHeadPic(UINT nUTalkUin, UINT nUTalkNum)
{
	if (IsOffline())
		return;
	//TODO��
}

// ����Ⱥ��Աͷ��
void CEdoyunClient::UpdateGroupMemberHeadPic(UINT nGroupCode, UINT nUTalkUin, UINT nUTalkNum)
{
	if (IsOffline())
		return;

	//TODO:
}

// ����Ⱥͷ��
void CEdoyunClient::UpdateGroupHeadPic(UINT nGroupCode, UINT nGroupNum)
{
	if (IsOffline())
		return;

	//TODO:
}

// ����Ⱥ��������
void CEdoyunClient::UpdateGroupFaceSignal()
{
	if (IsOffline())
		return;
}

// ���ͺ�����Ϣ
BOOL CEdoyunClient::SendBuddyMsg(UINT nFromUin, const tstring& strFromNickName, UINT nToUin, const tstring& strToNickName, time_t nTime, const tstring& strChatMsg, HWND hwndFrom/* = NULL*/)
{
	return m_SendMsgThread.AddBuddyMsg(nFromUin, strFromNickName, nToUin, strToNickName, nTime, strChatMsg, hwndFrom);
}

// ����Ⱥ��Ϣ
BOOL CEdoyunClient::SendGroupMsg(UINT nGroupId, time_t nTime, LPCTSTR lpMsg, HWND hwndFrom)
{
	if (IsOffline())
		return FALSE;

	return m_SendMsgThread.AddBuddyMsg(m_UserMgr.m_UserInfo.m_uUserID, m_UserMgr.m_UserInfo.m_strAccount, nGroupId, _T(""), nTime, lpMsg, hwndFrom);
}

// ������ʱ�Ự��Ϣ
BOOL CEdoyunClient::SendSessMsg(UINT nGroupId, UINT nToUin, time_t nTime, LPCTSTR lpMsg)
{
	if (IsOffline())
		return FALSE;

	return m_SendMsgThread.AddSessMsg(nGroupId, nToUin, nTime, lpMsg);
}


BOOL CEdoyunClient::SendMultiChatMsg(const std::set<UINT> setAccountID, time_t nTime, LPCTSTR lpMsg, HWND hwndFrom/*=NULL*/)
{
	if (IsOffline())
		return FALSE;
	//Ⱥ����Ϣ
	return m_SendMsgThread.AddMultiMsg(setAccountID, nTime, lpMsg, hwndFrom);
}

// �Ƿ�����״̬
BOOL CEdoyunClient::IsOffline()
{
	return (STATUS_OFFLINE == m_UserMgr.m_UserInfo.m_nStatus) ? TRUE : FALSE;
}

// ��ȡ����״̬
long CEdoyunClient::GetStatus()
{
	return m_UserMgr.m_UserInfo.m_nStatus;
}

// ��ȡ��֤��ͼƬ
BOOL CEdoyunClient::GetVerifyCodePic(const BYTE*& lpData, DWORD& dwSize)
{
	//lpData = (const BYTE*)m_UserMgr.m_VerifyCodePic.GetData();
	//dwSize = m_UserMgr.m_VerifyCodePic.GetSize();

	//if (lpData != NULL && dwSize > 0)
	//{
	//	return TRUE;
	//}
	//else
	//{
	//	lpData = NULL;
	//	dwSize = 0;
	//	return FALSE;
	//}
	return FALSE;
}

void CEdoyunClient::SetBuddyListAvailable(BOOL bAvailable)
{
	m_bBuddyListAvailable = bAvailable;
}

BOOL CEdoyunClient::IsBuddyListAvailable()
{
	return m_bBuddyListAvailable;
}

// ��ȡ�û���Ϣ
CBuddyInfo* CEdoyunClient::GetUserInfo(UINT uAccountID/*=0*/)
{
	if (uAccountID == 0 || uAccountID == m_UserMgr.m_UserInfo.m_uUserID)
		return &m_UserMgr.m_UserInfo;

	CBuddyTeamInfo* pTeamInfo = NULL;
	CBuddyInfo* pBuddyInfo = NULL;
	for (size_t i = 0; i < m_UserMgr.m_BuddyList.m_arrBuddyTeamInfo.size(); ++i)
	{
		pTeamInfo = m_UserMgr.m_BuddyList.m_arrBuddyTeamInfo[i];
		for (size_t j = 0; j < pTeamInfo->m_arrBuddyInfo.size(); ++j)
		{
			pBuddyInfo = pTeamInfo->m_arrBuddyInfo[j];
			if (uAccountID == pBuddyInfo->m_uUserID)
				return pBuddyInfo;
		}
	}


	CGroupInfo* pGroupInfo = NULL;
	for (size_t i = 0; i < m_UserMgr.m_GroupList.m_arrGroupInfo.size(); ++i)
	{
		pGroupInfo = m_UserMgr.m_GroupList.m_arrGroupInfo[i];
		for (size_t j = 0; j < pGroupInfo->m_arrMember.size(); ++j)
		{
			pBuddyInfo = pGroupInfo->m_arrMember[j];
			if (uAccountID == pBuddyInfo->m_uUserID)
				return pBuddyInfo;
		}
	}

	return NULL;
}

// ��ȡ�����б�
CBuddyList* CEdoyunClient::GetBuddyList()
{
	return &m_UserMgr.m_BuddyList;
}

// ��ȡȺ�б�
CGroupList* CEdoyunClient::GetGroupList()
{
	return &m_UserMgr.m_GroupList;
}

// ��ȡ�����ϵ���б�
CRecentList* CEdoyunClient::GetRecentList()
{
	return &m_UserMgr.m_RecentList;
}

// ��ȡ��Ϣ�б�
CMessageList* CEdoyunClient::GetMessageList()
{
	return &m_UserMgr.m_MsgList;
}

// ��ȡ��Ϣ��¼������
CMessageLogger* CEdoyunClient::GetMsgLogger()
{
	return &m_UserMgr.m_MsgLogger;
}

// ��ȡ�û��ļ��д��·��
tstring CEdoyunClient::GetUserFolder()
{
	return m_UserMgr.GetUserFolder();
}

// ��ȡ�����ļ��д��·��
tstring CEdoyunClient::GetPersonalFolder(UINT nUserNum/* = 0*/)
{
	return m_UserMgr.GetPersonalFolder();
}

// ��ȡ����ͼƬ���·��
tstring CEdoyunClient::GetChatPicFolder(UINT nUserNum/* = 0*/)
{
	return m_UserMgr.GetChatPicFolder();
}

// ��ȡ�û�ͷ��ͼƬȫ·���ļ���
tstring CEdoyunClient::GetUserHeadPicFullName(UINT nUserNum/* = 0*/)
{
	return m_UserMgr.GetUserHeadPicFullName(nUserNum);
}

// ��ȡ����ͷ��ͼƬȫ·���ļ���
tstring CEdoyunClient::GetBuddyHeadPicFullName(UINT nUTalkNum)
{
	return m_UserMgr.GetBuddyHeadPicFullName(nUTalkNum);
}

// ��ȡȺͷ��ͼƬȫ·���ļ���
tstring CEdoyunClient::GetGroupHeadPicFullName(UINT nGroupNum)
{
	return m_UserMgr.GetGroupHeadPicFullName(nGroupNum);
}

// ��ȡȺ��Աͷ��ͼƬȫ·���ļ���
tstring CEdoyunClient::GetSessHeadPicFullName(UINT nUTalkNum)
{
	return m_UserMgr.GetSessHeadPicFullName(nUTalkNum);
}

// ��ȡ����ͼƬȫ·���ļ���
tstring CEdoyunClient::GetChatPicFullName(LPCTSTR lpszFileName)
{
	return m_UserMgr.GetChatPicFullName(lpszFileName);
}

// ��ȡ��Ϣ��¼ȫ·���ļ���
tstring CEdoyunClient::GetMsgLogFullName(UINT nUserNum/* = 0*/)
{
	return m_UserMgr.GetMsgLogFullName();
}

// �ж��Ƿ���Ҫ���º���ͷ��
BOOL CEdoyunClient::IsNeedUpdateBuddyHeadPic(UINT nUTalkNum)
{
	return m_UserMgr.IsNeedUpdateBuddyHeadPic(nUTalkNum);
}

// �ж��Ƿ���Ҫ����Ⱥͷ��
BOOL CEdoyunClient::IsNeedUpdateGroupHeadPic(UINT nGroupNum)
{
	return m_UserMgr.IsNeedUpdateGroupHeadPic(nGroupNum);
}

// �ж��Ƿ���Ҫ����Ⱥ��Աͷ��
BOOL CEdoyunClient::IsNeedUpdateSessHeadPic(UINT nUTalkNum)
{
	return m_UserMgr.IsNeedUpdateSessHeadPic(nUTalkNum);
}

// ��ȡ������ʱ��
void CEdoyunClient::RequestServerTime()
{
	//m_IUProtocol.RequestServerTime();
}

time_t CEdoyunClient::GetCurrentTime()
{
	//return m_IUProtocol.GetServerTime();

	return 0;
}

void CEdoyunClient::OnNetworkStatusChange(UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL bAvailable = (wParam > 0 ? TRUE : FALSE);

	//��״̬�͵�ǰ��һ�£����ֲ���
	if (m_bNetworkAvailable == bAvailable)
		return;

	m_bNetworkAvailable = bAvailable;

	if (m_bNetworkAvailable)
	{
		CIULog::Log(LOG_WARNING, __FUNCSIG__, _T("Local connection is on service, try to relogon."));
		GoOnline();
	}
	else
	{
		CIULog::Log(LOG_WARNING, __FUNCSIG__, _T("Local connection is out of service, make the user offline."));
		GoOffline();
	}

}

void CEdoyunClient::OnHeartbeatResult(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (wParam == HEARTBEAT_DEAD)
	{
		GoOffline();
	}
}

void CEdoyunClient::OnRegisterResult(UINT message, WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_hwndRegister, message, wParam, lParam);
}

void CEdoyunClient::OnLoginResult(UINT message, WPARAM wParam, LPARAM lParam)
{
	CLoginResult* pLoginResult = (CLoginResult*)lParam;
	if (pLoginResult == NULL)
		return;

	long nLoginResultCode = pLoginResult->m_LoginResultCode;
	UINT uAccountID = pLoginResult->m_uAccountID;

	if (uAccountID > 0)
		m_UserMgr.m_UserInfo.m_uUserID = uAccountID;
	if (nLoginResultCode == LOGIN_SUCCESS)
	{
		m_UserMgr.m_UserInfo.m_nStatus = STATUS_ONLINE;
		TCHAR szAccountName[64] = { 0 };
		Utf8ToUnicode(pLoginResult->m_szAccountName, szAccountName, ARRAYSIZE(szAccountName));
		m_UserMgr.m_UserInfo.m_strAccount = szAccountName;
		TCHAR szNickName[64] = { 0 };
		Utf8ToUnicode(pLoginResult->m_szNickName, szNickName, ARRAYSIZE(szNickName));
		m_UserMgr.m_UserInfo.m_strNickName = szNickName;
		m_UserMgr.m_UserInfo.m_nFace = pLoginResult->m_nFace;
		m_UserMgr.m_UserInfo.m_nGender = pLoginResult->m_nGender;
		m_UserMgr.m_UserInfo.m_nBirthday = pLoginResult->m_nBirthday;

		TCHAR szSignature[512] = { 0 };
		Utf8ToUnicode(pLoginResult->m_szSignature, szSignature, ARRAYSIZE(szSignature));
		m_UserMgr.m_UserInfo.m_strSign = szSignature;

		TCHAR szAddress[512] = { 0 };
		Utf8ToUnicode(pLoginResult->m_szAddress, szAddress, ARRAYSIZE(szAddress));
		m_UserMgr.m_UserInfo.m_strAddress = szAddress;

		TCHAR szPhoneNumber[64] = { 0 };
		Utf8ToUnicode(pLoginResult->m_szPhoneNumber, szPhoneNumber, ARRAYSIZE(szPhoneNumber));
		m_UserMgr.m_UserInfo.m_strMobile = szPhoneNumber;

		TCHAR szMail[512] = { 0 };
		Utf8ToUnicode(pLoginResult->m_szMail, szMail, ARRAYSIZE(szMail));
		m_UserMgr.m_UserInfo.m_strEmail = szMail;

		m_UserMgr.m_UserInfo.m_strCustomFaceName = pLoginResult->m_szCustomFace;
		if (!m_UserMgr.m_UserInfo.m_strCustomFaceName.IsEmpty())
		{
			m_UserMgr.m_UserInfo.m_bUseCustomFace = TRUE;
			CString cachedUserThumb;
			cachedUserThumb.Format(_T("%s%d.png"), m_UserMgr.GetCustomUserThumbFolder().c_str(), uAccountID);
			if (Edoyun::CPath::IsFileExist(cachedUserThumb))
			{
				TCHAR szCachedThumbMd5[64] = { 0 };
				GetFileMd5ValueW(cachedUserThumb, szCachedThumbMd5, ARRAYSIZE(szCachedThumbMd5));

				TCHAR szThumbMd5Unicode[64] = { 0 };
				Utf8ToUnicode(pLoginResult->m_szCustomFace, szThumbMd5Unicode, ARRAYSIZE(szThumbMd5Unicode));
				if (_tcsncmp(szCachedThumbMd5, szThumbMd5Unicode, 32) == 0)
				{
					m_UserMgr.m_UserInfo.m_bUseCustomFace = TRUE;
					m_UserMgr.m_UserInfo.m_bCustomFaceAvailable = TRUE;
				}
				//ͷ�񲻴��ڣ����ظ�ͷ��
				else
				{
					CFileItemRequest* pFileItem = new CFileItemRequest();
					pFileItem->m_nFileType = FILE_ITEM_DOWNLOAD_USER_THUMB;
					pFileItem->m_uAccountID = uAccountID;
					strcpy_s(pFileItem->m_szUtfFilePath, ARRAYSIZE(pFileItem->m_szUtfFilePath), pLoginResult->m_szCustomFace);
					m_FileTask.AddItem(pFileItem);
				}
			}
			else
			{
				//����ͷ��
				CFileItemRequest* pFileItem = new CFileItemRequest();
				pFileItem->m_nFileType = FILE_ITEM_DOWNLOAD_USER_THUMB;
				pFileItem->m_uAccountID = uAccountID;
				strcpy_s(pFileItem->m_szUtfFilePath, ARRAYSIZE(pFileItem->m_szUtfFilePath), pLoginResult->m_szCustomFace);
				m_FileTask.AddItem(pFileItem);
			}
		}
	}

	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_LOGIN_RESULT, 0, nLoginResultCode);
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_BUDDY_INFO, 0, (LPARAM)m_UserMgr.m_UserInfo.m_uUserID);

	delete pLoginResult;
}

void CEdoyunClient::OnUpdateUserBasicInfo(UINT message, WPARAM wParam, LPARAM lParam)
{
	CUserBasicInfoResult* pResult = (CUserBasicInfoResult*)lParam;
	if (pResult == NULL)
		return;

	m_UserMgr.ClearUserInfo();
	//TODO: �����ȴ���һ��Ĭ�Ϻ��ѷ���
	CBuddyTeamInfo* pTeamInfo = new CBuddyTeamInfo();
	pTeamInfo->m_nIndex = 0;
	pTeamInfo->m_strName = _T("�ҵĺ���");
	m_UserMgr.m_BuddyList.m_arrBuddyTeamInfo.push_back(pTeamInfo);

	//TODO: �����ȴ���һ��Ĭ��Ⱥ����

	CBuddyInfo* pBuddyInfo = NULL;
	TCHAR szAccountName[32] = { 0 };
	TCHAR szNickName[32] = { 0 };
	TCHAR szSignature[256] = { 0 };
	TCHAR szPhoneNumber[32] = { 0 };
	TCHAR szMail[32] = { 0 };

	TCHAR szGroupAccount[32];

	CGroupInfo* pGroupInfo = NULL;
	for (auto& iter : pResult->m_listUserBasicInfo)
	{
		Utf8ToUnicode(iter->szAccountName, szAccountName, ARRAYSIZE(szAccountName));
		Utf8ToUnicode(iter->szNickName, szNickName, ARRAYSIZE(szNickName));
		Utf8ToUnicode(iter->szSignature, szSignature, ARRAYSIZE(szSignature));
		Utf8ToUnicode(iter->szPhoneNumber, szPhoneNumber, ARRAYSIZE(szPhoneNumber));
		Utf8ToUnicode(iter->szMail, szMail, ARRAYSIZE(szMail));

		if (iter->uAccountID < 0xFFFFFFF)
		{
			pBuddyInfo = new CBuddyInfo();
			pBuddyInfo->m_uUserID = iter->uAccountID;

			pBuddyInfo->m_strAccount = szAccountName;

			pBuddyInfo->m_strNickName = szNickName;

			pBuddyInfo->m_strSign = szSignature;

			pBuddyInfo->m_strMobile = szPhoneNumber;

			pBuddyInfo->m_strEmail = szMail;
			pBuddyInfo->m_nStatus = iter->nStatus;
			pBuddyInfo->m_nClientType = iter->clientType;
			pBuddyInfo->m_nFace = iter->uFaceID;
			pBuddyInfo->m_nBirthday = iter->nBirthday;
			pBuddyInfo->m_nGender = iter->nGender;

			pBuddyInfo->m_strCustomFaceName = iter->customFace;
			if (!pBuddyInfo->m_strCustomFaceName.IsEmpty())
			{
				pBuddyInfo->m_bUseCustomFace = TRUE;
				CString cachedUserThumb;
				cachedUserThumb.Format(_T("%s%d.png"), m_UserMgr.GetCustomUserThumbFolder().c_str(), iter->uAccountID);
				if (Edoyun::CPath::IsFileExist(cachedUserThumb))
				{
					TCHAR szCachedThumbMd5[64] = { 0 };
					GetFileMd5ValueW(cachedUserThumb, szCachedThumbMd5, ARRAYSIZE(szCachedThumbMd5));

					TCHAR szThumbMd5Unicode[64] = { 0 };
					Utf8ToUnicode(iter->customFace, szThumbMd5Unicode, ARRAYSIZE(szThumbMd5Unicode));
					if (_tcsncmp(szCachedThumbMd5, szThumbMd5Unicode, 32) == 0)
					{
						pBuddyInfo->m_bUseCustomFace = TRUE;
						pBuddyInfo->m_bCustomFaceAvailable = TRUE;
					}
					//ͷ�񲻴��ڣ����ظ�ͷ��
					else
					{
						//����ͷ��
						CFileItemRequest* pFileItem = new CFileItemRequest();
						pFileItem->m_nFileType = FILE_ITEM_DOWNLOAD_USER_THUMB;
						pFileItem->m_uAccountID = iter->uAccountID;
						strcpy_s(pFileItem->m_szUtfFilePath, ARRAYSIZE(pFileItem->m_szUtfFilePath), iter->customFace);
						m_FileTask.AddItem(pFileItem);
					}
				}
				else
				{
					//����ͷ��
					CFileItemRequest* pFileItem = new CFileItemRequest();
					pFileItem->m_nFileType = FILE_ITEM_DOWNLOAD_USER_THUMB;
					pFileItem->m_uAccountID = iter->uAccountID;
					strcpy_s(pFileItem->m_szUtfFilePath, ARRAYSIZE(pFileItem->m_szUtfFilePath), iter->customFace);
					m_FileTask.AddItem(pFileItem);
				}
			}

			pBuddyInfo->m_nTeamIndex = 0;
			pTeamInfo->m_arrBuddyInfo.push_back(pBuddyInfo);
		}
		else
		{
			pGroupInfo = new CGroupInfo();
			pGroupInfo->m_nGroupId = iter->uAccountID;
			pGroupInfo->m_nGroupCode = iter->uAccountID;
			pGroupInfo->m_strName = szNickName;
			memset(szGroupAccount, 0, sizeof(szGroupAccount));
			_stprintf_s(szGroupAccount, ARRAYSIZE(szGroupAccount), _T("%d"), iter->uAccountID);
			pGroupInfo->m_strAccount = szGroupAccount;

			m_UserMgr.m_GroupList.AddGroup(pGroupInfo);

			GetGroupMembers(iter->uAccountID);
		}

		DEL(iter);
	}


	delete pResult;

	//::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_BUDDY_INFO, 0, (LPARAM)m_UserMgr.m_UserInfo.m_uUserID);
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_BUDDY_LIST, 0, 0);
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_RECENT_LIST, 0, 0);
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_GROUP_LIST, 0, 0);
}

void CEdoyunClient::OnUpdateGroupBasicInfo(UINT message, WPARAM wParam, LPARAM lParam)
{
	CGroupBasicInfoResult* pResult = (CGroupBasicInfoResult*)lParam;
	if (pResult == NULL)
		return;

	CBuddyInfo* pBuddyInfo = NULL;
	TCHAR szAccountName[32] = { 0 };
	TCHAR szNickName[32] = { 0 };
	TCHAR szSignature[256] = { 0 };
	TCHAR szPhoneNumber[32] = { 0 };
	TCHAR szMail[32] = { 0 };

	//TCHAR szGroupAccount[32];

	CGroupInfo* pGroupInfo = m_UserMgr.m_GroupList.GetGroupById(pResult->m_groupid);

	for (auto& iter : pResult->m_listUserBasicInfo)
	{
		Utf8ToUnicode(iter->szAccountName, szAccountName, ARRAYSIZE(szAccountName));
		Utf8ToUnicode(iter->szNickName, szNickName, ARRAYSIZE(szNickName));
		Utf8ToUnicode(iter->szSignature, szSignature, ARRAYSIZE(szSignature));
		Utf8ToUnicode(iter->szPhoneNumber, szPhoneNumber, ARRAYSIZE(szPhoneNumber));
		Utf8ToUnicode(iter->szMail, szMail, ARRAYSIZE(szMail));


		pBuddyInfo = new CBuddyInfo();
		pBuddyInfo->m_uUserID = iter->uAccountID;

		pBuddyInfo->m_strAccount = szAccountName;

		pBuddyInfo->m_strNickName = szNickName;

		pBuddyInfo->m_strSign = szSignature;

		pBuddyInfo->m_strMobile = szPhoneNumber;

		pBuddyInfo->m_strEmail = szMail;
		pBuddyInfo->m_nStatus = iter->nStatus;
		pBuddyInfo->m_nClientType = iter->clientType;
		pBuddyInfo->m_nFace = iter->uFaceID;
		pBuddyInfo->m_nBirthday = iter->nBirthday;
		pBuddyInfo->m_nGender = iter->nGender;

		pBuddyInfo->m_strCustomFaceName = iter->customFace;
		if (!pBuddyInfo->m_strCustomFaceName.IsEmpty())
		{
			pBuddyInfo->m_bUseCustomFace = TRUE;
			CString cachedUserThumb;
			cachedUserThumb.Format(_T("%s%d.png"), m_UserMgr.GetCustomUserThumbFolder().c_str(), iter->uAccountID);
			if (Edoyun::CPath::IsFileExist(cachedUserThumb))
			{
				TCHAR szCachedThumbMd5[64] = { 0 };
				GetFileMd5ValueW(cachedUserThumb, szCachedThumbMd5, ARRAYSIZE(szCachedThumbMd5));

				TCHAR szThumbMd5Unicode[64] = { 0 };
				Utf8ToUnicode(iter->customFace, szThumbMd5Unicode, ARRAYSIZE(szThumbMd5Unicode));
				if (_tcsncmp(szCachedThumbMd5, szThumbMd5Unicode, 32) == 0)
				{
					pBuddyInfo->m_bUseCustomFace = TRUE;
					pBuddyInfo->m_bCustomFaceAvailable = TRUE;
				}
				//ͷ�񲻴��ڣ����ظ�ͷ��
				else
				{
					//����ͷ��
					CFileItemRequest* pFileItem = new CFileItemRequest();
					pFileItem->m_nFileType = FILE_ITEM_DOWNLOAD_USER_THUMB;
					pFileItem->m_uAccountID = iter->uAccountID;
					strcpy_s(pFileItem->m_szUtfFilePath, ARRAYSIZE(pFileItem->m_szUtfFilePath), iter->customFace);
					m_FileTask.AddItem(pFileItem);
				}
			}
			else
			{
				//����ͷ��
				CFileItemRequest* pFileItem = new CFileItemRequest();
				pFileItem->m_nFileType = FILE_ITEM_DOWNLOAD_USER_THUMB;
				pFileItem->m_uAccountID = iter->uAccountID;
				strcpy_s(pFileItem->m_szUtfFilePath, ARRAYSIZE(pFileItem->m_szUtfFilePath), iter->customFace);
				m_FileTask.AddItem(pFileItem);
			}
		}

		pBuddyInfo->m_nTeamIndex = 0;

		if (pGroupInfo != NULL)
		{
			pGroupInfo->AddMember(pBuddyInfo);
		}


		DEL(iter);
	}

	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_GROUP_INFO, pResult->m_groupid, 0);

	delete pResult;

	//::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_BUDDY_INFO, 0, (LPARAM)m_UserMgr.m_UserInfo.m_uUserID);
	//::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_BUDDY_LIST, 0, 0);
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_RECENT_LIST, 0, 0);
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_GROUP_LIST, 0, 0);
}

void CEdoyunClient::OnModifyInfoResult(UINT message, WPARAM wParam, LPARAM lParam)
{
	CUpdateLogonUserInfoResult* pResult = (CUpdateLogonUserInfoResult*)lParam;
	if (pResult == NULL)
		delete pResult;

	TCHAR szNickName[64] = { 0 };
	Utf8ToUnicode(pResult->m_szNickName, szNickName, ARRAYSIZE(szNickName));
	m_UserMgr.m_UserInfo.m_strNickName = szNickName;
	m_UserMgr.m_UserInfo.m_nFace = pResult->m_uFaceID;
	m_UserMgr.m_UserInfo.m_nGender = pResult->m_uGender;
	m_UserMgr.m_UserInfo.m_nBirthday = pResult->m_nBirthday;

	TCHAR szSignature[512] = { 0 };
	Utf8ToUnicode(pResult->m_szSignature, szSignature, ARRAYSIZE(szSignature));
	m_UserMgr.m_UserInfo.m_strSign = szSignature;

	TCHAR szAddress[512] = { 0 };
	Utf8ToUnicode(pResult->m_szAddress, szAddress, ARRAYSIZE(szAddress));
	m_UserMgr.m_UserInfo.m_strAddress = szAddress;

	TCHAR szPhoneNumber[64] = { 0 };
	Utf8ToUnicode(pResult->m_szPhone, szPhoneNumber, ARRAYSIZE(szPhoneNumber));
	m_UserMgr.m_UserInfo.m_strMobile = szPhoneNumber;

	TCHAR szMail[512] = { 0 };
	Utf8ToUnicode(pResult->m_szMail, szMail, ARRAYSIZE(szMail));
	m_UserMgr.m_UserInfo.m_strEmail = szMail;

	m_UserMgr.m_UserInfo.m_strCustomFaceName = pResult->m_szCustomFace;
	if (!m_UserMgr.m_UserInfo.m_strCustomFaceName.IsEmpty())
	{
		m_UserMgr.m_UserInfo.m_bUseCustomFace = TRUE;
		CString cachedUserThumb;
		cachedUserThumb.Format(_T("%s%d.png"), m_UserMgr.GetCustomUserThumbFolder().c_str(), m_UserMgr.m_UserInfo.m_uUserID);
		if (Edoyun::CPath::IsFileExist(cachedUserThumb))
		{
			TCHAR szCachedThumbMd5[64] = { 0 };
			GetFileMd5ValueW(cachedUserThumb, szCachedThumbMd5, ARRAYSIZE(szCachedThumbMd5));

			TCHAR szThumbMd5Unicode[64] = { 0 };
			Utf8ToUnicode(pResult->m_szCustomFace, szThumbMd5Unicode, ARRAYSIZE(szThumbMd5Unicode));
			if (_tcsncmp(szCachedThumbMd5, szThumbMd5Unicode, 32) == 0)
			{
				m_UserMgr.m_UserInfo.m_bUseCustomFace = TRUE;
				m_UserMgr.m_UserInfo.m_bCustomFaceAvailable = TRUE;
			}
			//ͷ�񲻴��ڣ����ظ�ͷ��
			else
			{
				CFileItemRequest* pFileItem = new CFileItemRequest();
				pFileItem->m_nFileType = FILE_ITEM_DOWNLOAD_USER_THUMB;
				pFileItem->m_uAccountID = m_UserMgr.m_UserInfo.m_uUserID;
				strcpy_s(pFileItem->m_szUtfFilePath, ARRAYSIZE(pFileItem->m_szUtfFilePath), pResult->m_szCustomFace);
				m_FileTask.AddItem(pFileItem);
			}

		}
		//����ͷ��
		else
		{
			CFileItemRequest* pFileItem = new CFileItemRequest();
			pFileItem->m_nFileType = FILE_ITEM_DOWNLOAD_USER_THUMB;
			pFileItem->m_uAccountID = m_UserMgr.m_UserInfo.m_uUserID;
			strcpy_s(pFileItem->m_szUtfFilePath, ARRAYSIZE(pFileItem->m_szUtfFilePath), pResult->m_szCustomFace);
			m_FileTask.AddItem(pFileItem);
		}

	}

	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_BUDDY_INFO, 0, (LPARAM)m_UserMgr.m_UserInfo.m_uUserID);
	//::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_GROUP_INFO, (WPARAM)uAccountID, 0);

	delete pResult;
}

void CEdoyunClient::OnRecvUserStatusChangeData(UINT message, WPARAM wParam, LPARAM lParam)
{
	CFriendStatus* pFriendStatus = (CFriendStatus*)lParam;
	if (pFriendStatus == NULL)
		return;

	//m_RecvMsgTask.AddMsgData(pFriendStatus);
}

void CEdoyunClient::OnRecvAddFriendRequest(UINT message, WPARAM wParam, LPARAM lParam)
{
	//TODO: ���ּӺ��ѵ��߼�̫�ã���Ҫ�Ż���
	COperateFriendResult* pOperateFriendResult = (COperateFriendResult*)lParam;
	if (pOperateFriendResult == NULL)
		return;

	BOOL bAlreadyExist = FALSE;
	std::map<UINT, UINT>::iterator iter = m_mapAddFriendCache.begin();
	for (; iter != m_mapAddFriendCache.end(); ++iter)
	{
		if (pOperateFriendResult->m_uCmd == Apply && pOperateFriendResult->m_uCmd == iter->second && iter->first == pOperateFriendResult->m_uAccountID)
		{
			bAlreadyExist = TRUE;
			break;
		}
		//else if(pOperateFriendResult->m_uCmd==Agree && pOperateFriendResult->m_uCmd==iter->second && iter->first==pOperateFriendResult->m_uAccountID)
		//{
		//	bAlreadyExist = TRUE;
		//	//����Ҫ���º����б��ˣ��Ƚ�m_bBuddyListAvailable����ΪFALSE���Ա㻺���¼ӵĺ�������֪ͨ
		//	StartGetUserInfoTask(USER_INFO_TYPE_SELF);
		//	StartGetUserInfoTask(USER_INFO_TYPE_FRIENDS);
		//	break;
		//}
		else
		{
			//����Ҫ���º����б��ˣ��Ƚ�m_bBuddyListAvailable����ΪFALSE���Ա㻺���¼ӵĺ�������֪ͨ
			m_bBuddyListAvailable = FALSE;
			m_mapAddFriendCache.erase(iter);
			break;
		}

	}
	if (bAlreadyExist)
		delete pOperateFriendResult;
	else
	{
		::PostMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_RECVADDFRIENDREQUSET, 0, (LPARAM)pOperateFriendResult);
	}

}

void CEdoyunClient::OnUserStatusChange(UINT message, WPARAM wParam, LPARAM lParam)
{
	CFriendStatus* pFriendStatus = (CFriendStatus*)lParam;
	if (pFriendStatus == NULL)
		return;

	UINT uAccountID = pFriendStatus->m_uAccountID;
	long nFlag = pFriendStatus->m_nStatus;
	//long nStatus = ParseBuddyStatus(nFlag);



	//AtlTrace(_T("AccountID=%u, Status=%d\n"), uAccountID, nStatus);
	//����û���Ϣ��ʱ�����ã��Ȼ�������
	//if(!m_bBuddyListAvailable || !m_bGroupMemberInfoAvailable)
	//{
	//	std::map<UINT, long>::iterator iter = m_mapUserStatusCache.find(uAccountID);
	//	if(iter != m_mapUserStatusCache.end())
	//		iter->second = nStatus;
	//	else
	//		m_mapUserStatusCache.insert(std::make_pair(uAccountID, nStatus));

	//	return;
	//}

	if (pFriendStatus->m_type == 1 || pFriendStatus->m_type == 2)
	{
		SetBuddyStatus(uAccountID, nFlag);

		//�Լ�
		//if(uAccountID == m_UserMgr.m_UserInfo.m_uUserID)
		//{
		//	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_SELF_STATUS_CHANGE, (WPARAM)uAccountID, (LPARAM)nStatus);
		//}
		//else
		{
			::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_STATUS_CHANGE_MSG, 0, (LPARAM)uAccountID);
			::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_RECENT_LIST, 0, 0);
			::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_CHATDLG_USERINFO, (WPARAM)uAccountID, 0);
			::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_GROUP_INFO, (WPARAM)uAccountID, 0);
		}
	}
	else if (pFriendStatus->m_type == 3)
	{
		//������Ϣ�и��£��������º����б�(TODO: �����̫�������������Ż���ֻ�����µĺ�����Ϣ)
		GetFriendList();
	}

	delete pFriendStatus;
}

void CEdoyunClient::OnSendConfirmMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	CUploadFileResult* pUploadFileResult = (CUploadFileResult*)lParam;
	if (pUploadFileResult == NULL)
		return;

	if (wParam != 0)
	{
		//ͼƬ�ϴ�ʧ��
		delete pUploadFileResult;
		return;
	}

	//�ϴ�ͼƬ���
	if (pUploadFileResult->m_nFileType == FILE_ITEM_UPLOAD_CHAT_IMAGE)
	{
		time_t nTime = time(NULL);
		TCHAR szMd5[64] = { 0 };
		AnsiToUnicode(pUploadFileResult->m_szMd5, szMd5, ARRAYSIZE(szMd5));
		CString strImageName;
		strImageName.Format(_T("%s.%s"), szMd5, Edoyun::CPath::GetExtension(pUploadFileResult->m_szLocalName).c_str());
		long nWidth = 0;
		long nHeight = 0;
		GetImageWidthAndHeight(pUploadFileResult->m_szLocalName, nWidth, nHeight);
		char szUtf8FileName[MAX_PATH] = { 0 };
		UnicodeToUtf8(strImageName, szUtf8FileName, ARRAYSIZE(szUtf8FileName));
		CStringA strImageAcquireMsg;
		//if (pUploadFileResult->m_bSuccessful)
		//    strImageAcquireMsg.Format("{\"msgType\":2,\"time\":%llu,\"clientType\":1,\"content\":[{\"pic\":[\"%s\",\"%s\",%u,%d,%d]}]}", nTime, szUtf8FileName, pUploadFileResult->m_szRemoteName, pUploadFileResult->m_dwFileSize, nWidth, nHeight);
		//else
		//    strImageAcquireMsg.Format("{\"msgType\":2,\"time\":%llu,\"clientType\":1,\"content\":[{\"pic\":[\"%s\",\"\",%u,%d,%d]}]}", nTime, szUtf8FileName, pUploadFileResult->m_dwFileSize, nWidth, nHeight);

		if (pUploadFileResult->m_bSuccessful)
			strImageAcquireMsg.Format("{\"msgType\":2,\"time\":%llu,\"clientType\":1,\"content\":[{\"pic\":[\"%s\",\"%s\",%u,%d,%d]}]}", nTime, szUtf8FileName, pUploadFileResult->m_szRemoteName, pUploadFileResult->m_dwFileSize, nWidth, nHeight);
		else
			strImageAcquireMsg.Format("{\"msgType\":2,\"time\":%llu,\"clientType\":1,\"content\":[{\"pic\":[\"%s\",\"\",%u,%d,%d]}]}", nTime, szUtf8FileName, pUploadFileResult->m_dwFileSize, nWidth, nHeight);

		long nBodyLength = strImageAcquireMsg.GetLength() + 1;
		char* pszMsgBody = new char[nBodyLength];
		memset(pszMsgBody, 0, nBodyLength);
		strcpy_s(pszMsgBody, nBodyLength, strImageAcquireMsg);
		CSendChatConfirmImageMessage* pConfirm = new CSendChatConfirmImageMessage();
		pConfirm->m_hwndChat = pUploadFileResult->m_hwndReflection;
		pConfirm->m_pszConfirmBody = pszMsgBody;
		pConfirm->m_uConfirmBodySize = nBodyLength - 1;
		pConfirm->m_uSenderID = pUploadFileResult->m_uSenderID;
		pConfirm->m_setTargetIDs = pUploadFileResult->m_setTargetIDs;
		if (pConfirm->m_setTargetIDs.size() > 1)
			pConfirm->m_nType = CHAT_CONFIRM_TYPE_MULTI;
		else
			pConfirm->m_nType = CHAT_CONFIRM_TYPE_SINGLE;

		//SendBuddyMsg(UINT nFromUin, const tstring& strFromNickName, UINT nToUin, const tstring& strToNickName, time_t nTime, const tstring& strChatMsg, HWND hwndFrom/* = NULL*/)

		m_SendMsgThread.AddItem(pConfirm);
	}

	delete pUploadFileResult;
}

void CEdoyunClient::OnUpdateChatMsgID(UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT uAccountID = (UINT)wParam;
	UINT uMsgID = (UINT)lParam;
	m_UserMgr.SetMsgID(uAccountID, uMsgID);
}

void CEdoyunClient::OnFindFriend(UINT message, WPARAM wParam, LPARAM lParam)
{
	::PostMessage(m_hwndFindFriend, FMG_MSG_FINDFREIND, 0, lParam);
}

void CEdoyunClient::OnBuddyCustomFaceAvailable(UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT uAccountID = (UINT)wParam;
	if (uAccountID == 0)
		return;

	if (uAccountID == m_UserMgr.m_UserInfo.m_uUserID)
		m_UserMgr.m_UserInfo.m_bCustomFaceAvailable = TRUE;
	else
	{
		CBuddyInfo* pBuddyInfo = m_UserMgr.m_BuddyList.GetBuddy(uAccountID);
		if (pBuddyInfo == NULL)
			return;

		pBuddyInfo->m_bCustomFaceAvailable = TRUE;
	}

	::PostMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_BUDDY_INFO, 0, (LPARAM)uAccountID);
	::PostMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_RECENT_LIST, 0, 0);
}

void CEdoyunClient::OnModifyPasswordResult(UINT message, WPARAM wParam, LPARAM lParam)
{
	::PostMessage(m_hwndModifyPassword, message, wParam, lParam);
}

void CEdoyunClient::OnCreateNewGroupResult(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_hwndCreateNewGroup)
		::PostMessage(m_hwndCreateNewGroup, message, wParam, lParam);
}

void CEdoyunClient::OnDeleteFriendResult(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (wParam != DELETE_FRIEND_SUCCESS)
		return;

	UINT uAccountID = (UINT)lParam;
	if (!IsGroupTarget(uAccountID))
	{
		m_UserMgr.DeleteFriend(uAccountID);
		m_UserMgr.m_MsgList.DelMsgSender(FMG_MSG_TYPE_BUDDY, uAccountID);
	}
	else
	{
		m_UserMgr.ExitGroup(uAccountID);
		m_UserMgr.m_MsgList.DelMsgSender(FMG_MSG_TYPE_GROUP, uAccountID);
		if (m_UserMgr.m_RecentList.DeleteRecentItem(uAccountID))
			::PostMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_RECENT_LIST, 0, 0);
	}

	if (m_UserMgr.m_MsgList.GetMsgSenderCount() <= 0)
		::PostMessage(m_UserMgr.m_hCallBackWnd, WM_CANCEL_FLASH, 0, 0);

	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_DELETEFRIEND, DELETE_FRIEND_SUCCESS, lParam);
}

void CEdoyunClient::OnUpdateBuddyList(UINT message, WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_UserMgr.m_hCallBackWnd, message, wParam, lParam);
}

void CEdoyunClient::OnUpdateGroupList(UINT message, WPARAM wParam, LPARAM lParam)
{
	//BOOL bSuccess = FALSE;
	//CGroupListResult* lpGroupListResult = (CGroupListResult*)lParam;
	//if (lpGroupListResult != NULL)
	//{
	//	for (int i = 0; i < (int)lpGroupListResult->m_arrGroupInfo.size(); i++)
	//	{
	//		CGroupInfo* lpGroupInfo = lpGroupListResult->m_arrGroupInfo[i];
	//		if (lpGroupInfo != NULL)
	//			m_UserMgr.m_GroupList.AddGroup(lpGroupInfo);
	//	}
	//	lpGroupListResult->m_arrGroupInfo.clear();
	//	delete lpGroupListResult;
	//	bSuccess = TRUE;
	//}
	//::SendMessage(m_UserMgr.m_hCallBackWnd, message, NULL, bSuccess);
}

void CEdoyunClient::OnUpdateRecentList(UINT message, WPARAM wParam, LPARAM lParam)
{
	::SendMessage(m_UserMgr.m_hCallBackWnd, message, 0, 0);
}

void CEdoyunClient::OnBuddyMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	CBuddyMessage* lpBuddyMsg = (CBuddyMessage*)lParam;
	if (NULL == lpBuddyMsg)
		return;

	UINT nUTalkUin = lpBuddyMsg->m_nFromUin;
	UINT nMsgId = lpBuddyMsg->m_nMsgId;

	m_UserMgr.m_MsgList.AddMsg(FMG_MSG_TYPE_BUDDY, lpBuddyMsg->m_nFromUin, 0, (void*)lpBuddyMsg);

	//��������Ự�б���������Ự�б�����ʾ��Ϣ�ĸ���
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_RECENT_LIST, 0, 0);
	::SendMessage(m_UserMgr.m_hCallBackWnd, message, nUTalkUin, lParam);
}

void CEdoyunClient::OnGroupMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	CBuddyMessage* lpGroupMsg = (CBuddyMessage*)lParam;
	if (NULL == lpGroupMsg)
		return;

	UINT nGroupCode = lpGroupMsg->m_nToUin;
	UINT nMsgId = lpGroupMsg->m_nMsgId;

	m_UserMgr.m_MsgList.AddMsg(FMG_MSG_TYPE_GROUP,
		lpGroupMsg->m_nToUin, lpGroupMsg->m_nToUin, (void*)lpGroupMsg);

	//��������Ự�б���������Ự�б�����ʾ��Ϣ�ĸ���
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_RECENT_LIST, 0, 0);
	::SendMessage(m_UserMgr.m_hCallBackWnd, message, nGroupCode, nMsgId);
}

void CEdoyunClient::OnSessMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	CSessMessage* lpSessMsg = (CSessMessage*)lParam;
	if (NULL == lpSessMsg)
		return;

	UINT nUTalkUin = lpSessMsg->m_nFromUin;
	UINT nMsgId = lpSessMsg->m_nMsgId;
	UINT nGroupCode = 0;

	CGroupInfo* lpGroupInfo = m_UserMgr.m_GroupList.GetGroupById(lpSessMsg->m_nGroupId);
	if (lpGroupInfo != NULL)
	{
		nGroupCode = lpGroupInfo->m_nGroupCode;
		CBuddyInfo* lpBuddyInfo = lpGroupInfo->GetMemberByUin(lpSessMsg->m_nFromUin);
		if (NULL == lpBuddyInfo)
		{
			lpBuddyInfo = new CBuddyInfo;
			if (lpBuddyInfo != NULL)
			{
				lpBuddyInfo->Reset();
				lpBuddyInfo->m_uUserID = lpSessMsg->m_nFromUin;
				//lpBuddyInfo->m_nUTalkNum = lpSessMsg->m_nUTalkNum;
				lpGroupInfo->m_arrMember.push_back(lpBuddyInfo);
			}
			UpdateGroupMemberInfo(nGroupCode, lpSessMsg->m_nFromUin);
		}
	}

	m_UserMgr.m_MsgList.AddMsg(FMG_MSG_TYPE_SESS,
		lpSessMsg->m_nFromUin, nGroupCode, (void*)lpSessMsg);

	::SendMessage(m_UserMgr.m_hCallBackWnd, message, nUTalkUin, nMsgId);
}

void CEdoyunClient::OnSysGroupMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	//CSysGroupMessage* lpSysGroupMsg = (CSysGroupMessage*)lParam;
	//if (NULL == lpSysGroupMsg)
	//	return;

	//UINT nGroupCode = lpSysGroupMsg->m_nGroupCode;

	//m_UserMgr.m_MsgList.AddMsg(FMG_MSG_TYPE_SYSGROUP, lpSysGroupMsg->m_nGroupCode, 
	//	lpSysGroupMsg->m_nGroupCode, (void*)lpSysGroupMsg);

	//::SendMessage(m_UserMgr.m_hCallBackWnd, message, 0, nGroupCode);
}

void CEdoyunClient::OnStatusChangeMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	//UINT nUTalkUin = 0;
	//CStatusChangeMessage* lpStatusChangeMsg = (CStatusChangeMessage*)lParam;
	//if (NULL == lpStatusChangeMsg)
	//	return;
	//
	//nUTalkUin = lpStatusChangeMsg->m_nUTalkUin;
	//CBuddyInfo* lpBuddyInfo = m_UserMgr.m_BuddyList.GetBuddy(nUTalkUin);
	//if (lpBuddyInfo != NULL)
	//{
	//	lpBuddyInfo->m_nStatus = lpStatusChangeMsg->m_nStatus;
	//	lpBuddyInfo->m_nClientType = lpStatusChangeMsg->m_nClientType;
	//	CBuddyTeamInfo* lpBuddyTeamInfo = m_UserMgr.m_BuddyList.GetBuddyTeam(lpBuddyInfo->m_nTeamIndex);
	//	if (lpBuddyTeamInfo != NULL)
	//		lpBuddyTeamInfo->Sort();
	//}
	//delete lpStatusChangeMsg;
	//::SendMessage(m_UserMgr.m_hCallBackWnd, message, 0, nUTalkUin);
}

void CEdoyunClient::OnKickMsg(UINT message, WPARAM wParam, LPARAM lParam)
{
	//CKickMessage* lpKickMsg = (CKickMessage*)lParam;
	//if (NULL == lpKickMsg)
	//	return;
	//
	//delete lpKickMsg;
	//m_UserMgr.m_UserInfo.m_nStatus = STATUS_OFFLINE;
	//m_ThreadPool.RemoveAllTask();
	//::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_KICK_MSG, 0, 0);
}

void CEdoyunClient::OnUpdateBuddyNumber(UINT message, WPARAM wParam, LPARAM lParam)
{
	//UINT nUTalkUin = 0;
	//CGetUTalkNumResult* lpGetUTalkNumResult = (CGetUTalkNumResult*)lParam;
	//if (lpGetUTalkNumResult != NULL)
	//{
	//	nUTalkUin = lpGetUTalkNumResult->m_nUTalkUin;
	//	CBuddyInfo* lpBuddyInfo = m_UserMgr.m_BuddyList.GetBuddy(nUTalkUin);
	//	if (lpBuddyInfo != NULL)
	//		lpBuddyInfo->SetUTalkNum(lpGetUTalkNumResult);
	//	delete lpGetUTalkNumResult;
	//}
	//::SendMessage(m_UserMgr.m_hCallBackWnd, message, 0, nUTalkUin);
}

void CEdoyunClient::OnUpdateGMemberNumber(UINT message, WPARAM wParam, LPARAM lParam)
{
	//UINT nGroupCode = (UINT)wParam;
	//UINT nUTalkUin = 0;
	//CGetUTalkNumResult* lpGetUTalkNumResult = (CGetUTalkNumResult*)lParam;
	//if (nGroupCode != 0 && lpGetUTalkNumResult != NULL)
	//{
	//	nUTalkUin = lpGetUTalkNumResult->m_nUTalkUin;
	//	CBuddyInfo* lpBuddyInfo = m_UserMgr.m_GroupList.GetGroupMemberByCode(nGroupCode, nUTalkUin);
	//	if (lpBuddyInfo != NULL)
	//		lpBuddyInfo->SetUTalkNum(lpGetUTalkNumResult);
	//	delete lpGetUTalkNumResult;
	//}
	//::SendMessage(m_UserMgr.m_hCallBackWnd, message, nGroupCode, nUTalkUin);
}

void CEdoyunClient::OnUpdateGroupNumber(UINT message, WPARAM wParam, LPARAM lParam)
{
	//UINT nGroupCode = (UINT)wParam;
	//CGetUTalkNumResult* lpGetUTalkNumResult = (CGetUTalkNumResult*)lParam;
	//if (nGroupCode != 0 && lpGetUTalkNumResult != NULL)
	//{
	//	CGroupInfo* lpGroupInfo = m_UserMgr.m_GroupList.GetGroupByCode(nGroupCode);
	//	if (lpGroupInfo != NULL)
	//		lpGroupInfo->SetGroupNumber(lpGetUTalkNumResult);
	//	delete lpGetUTalkNumResult;
	//}
	//::SendMessage(m_UserMgr.m_hCallBackWnd, message, 0, nGroupCode);
}

void CEdoyunClient::OnUpdateBuddySign(UINT message, WPARAM wParam, LPARAM lParam)
{
	//UINT nUTalkUin = 0;
	//CGetSignResult* lpGetSignResult = (CGetSignResult*)lParam;
	//if (lpGetSignResult != NULL)
	//{
	//	nUTalkUin = lpGetSignResult->m_nUTalkUin;
	//	if (m_UserMgr.m_UserInfo.m_nUTalkUin == nUTalkUin)		// �����û�����ǩ��
	//	{
	//		m_UserMgr.m_UserInfo.SetUTalkSign(lpGetSignResult);
	//	}
	//	else											// ���º��Ѹ���ǩ��
	//	{
	//		CBuddyInfo* lpBuddyInfo = m_UserMgr.m_BuddyList.GetBuddy(nUTalkUin);
	//		if (lpBuddyInfo != NULL)
	//			lpBuddyInfo->SetUTalkSign(lpGetSignResult);
	//	}
	//	delete lpGetSignResult;
	//}
	//::SendMessage(m_UserMgr.m_hCallBackWnd, message, NULL, nUTalkUin);
}

void CEdoyunClient::OnUpdateGMemberSign(UINT message, WPARAM wParam, LPARAM lParam)
{
	//UINT nGroupCode = (UINT)wParam;
	//UINT nUTalkUin = 0;
	//CGetSignResult* lpGetSignResult = (CGetSignResult*)lParam;

	//if (nGroupCode != 0 && lpGetSignResult != NULL)
	//{
	//	nUTalkUin = lpGetSignResult->m_nUTalkUin;
	//	CBuddyInfo* lpBuddyInfo = m_UserMgr.m_GroupList.GetGroupMemberByCode(nGroupCode, nUTalkUin);
	//	if (lpBuddyInfo != NULL)
	//		lpBuddyInfo->SetUTalkSign(lpGetSignResult);
	//	delete lpGetSignResult;
	//}
	//::SendMessage(m_UserMgr.m_hCallBackWnd, message, nGroupCode, nUTalkUin);
}

void CEdoyunClient::OnUpdateBuddyInfo(UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT uUserID = m_UserMgr.m_UserInfo.m_uUserID;

	CBuddyInfo* lpBuddyInfo = m_UserMgr.m_BuddyList.GetBuddy(uUserID);
	if (lpBuddyInfo != NULL)
		lpBuddyInfo->SetBuddyInfo(lpBuddyInfo);

	::SendMessage(m_UserMgr.m_hCallBackWnd, message, NULL, 0);
}

void CEdoyunClient::OnUpdateGMemberInfo(UINT message, WPARAM wParam, LPARAM lParam)
{
	//UINT nGroupCode = (UINT)wParam;
	//UINT nUTalkUin = 0;
	//CBuddyInfoResult* lpBuddyInfoResult = (CBuddyInfoResult*)lParam;

	//if (nGroupCode != 0 && lpBuddyInfoResult != NULL)
	//{
	//	//nUTalkUin = lpBuddyInfoResult->m_nUTalkUin;
	//	CBuddyInfo* lpBuddyInfo = m_UserMgr.m_GroupList.GetGroupMemberByCode(nGroupCode, nUTalkUin);
	//	if (lpBuddyInfo != NULL)
	//		lpBuddyInfo->SetBuddyInfo(lpBuddyInfoResult);
	//	delete lpBuddyInfoResult;
	//}
	//::SendMessage(m_UserMgr.m_hCallBackWnd, message, nGroupCode, nUTalkUin);
}

void CEdoyunClient::OnUpdateGroupInfo(UINT message, WPARAM wParam, LPARAM lParam)
{
	//UINT nGroupCode = 0;
	//CGroupInfoResult* lpGroupInfoResult = (CGroupInfoResult*)lParam;
	//if (lpGroupInfoResult != NULL)
	//{
	//	nGroupCode = lpGroupInfoResult->m_nGroupCode;
	//	CGroupInfo* lpGroupInfo = m_UserMgr.m_GroupList.GetGroupByCode(nGroupCode);
	//	if (lpGroupInfo != NULL)
	//		lpGroupInfo->SetGroupInfo(lpGroupInfoResult);
	//	delete lpGroupInfoResult;
	//}

	UINT nGroupCode = (UINT)lParam;
	::SendMessage(m_UserMgr.m_hCallBackWnd, message, 0, nGroupCode);
}

void CEdoyunClient::OnChangeStatusResult(UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL bSuccess = (BOOL)wParam;
	long nNewStatus = lParam;

	if (bSuccess)
		m_UserMgr.m_UserInfo.m_nStatus = nNewStatus;

	::SendMessage(m_UserMgr.m_hCallBackWnd, message, wParam, lParam);
}

void CEdoyunClient::OnTargetInfoChange(UINT message, WPARAM wParam, LPARAM lParam)
{
	CTargetInfoChangeResult* pResult = (CTargetInfoChangeResult*)lParam;
	if (pResult == NULL)
		return;

	UINT uAccountID = pResult->m_uAccountID;
	delete pResult;

	if (uAccountID == 0 || !IsGroupTarget(uAccountID))
		return;

	//Ⱥ���˺�ֻ�л�����Ϣ��ID�б�û����չ��Ϣ
	//CUserBasicInfoRequest* pBasicInfoRequest = new CUserBasicInfoRequest();
	//pBasicInfoRequest->m_setAccountID.insert(uAccountID);
	//m_SendMsgTask.AddItem(pBasicInfoRequest);

	m_bGroupMemberInfoAvailable = FALSE;
	CLoginUserFriendsIDRequest* pFriendsIDRequest = new CLoginUserFriendsIDRequest();
	pFriendsIDRequest->m_uAccountID = uAccountID;
	//m_SendMsgTask.AddItem(pFriendsIDRequest);
}

void CEdoyunClient::OnInternal_GetBuddyData(UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT nUTalkUin = (UINT)wParam;
	RMT_BUDDY_DATA* lpBuddyData = (RMT_BUDDY_DATA*)lParam;
	if (NULL == lpBuddyData)
		return;

	CBuddyInfo* lpBuddyInfo = m_UserMgr.m_BuddyList.GetBuddy(nUTalkUin);
	if (NULL == lpBuddyInfo)
		return;

	lpBuddyData->nUTalkNum = lpBuddyInfo->m_uUserID;

	int nMaxCnt = sizeof(lpBuddyData->szNickName) / sizeof(TCHAR);
	if (!lpBuddyInfo->m_strMarkName.empty())
		_tcsncpy(lpBuddyData->szNickName, lpBuddyInfo->m_strMarkName.c_str(), nMaxCnt);
	else
		_tcsncpy(lpBuddyData->szNickName, lpBuddyInfo->m_strNickName.c_str(), nMaxCnt);
	lpBuddyData->szNickName[nMaxCnt - 1] = _T('\0');
}

void CEdoyunClient::OnInternal_GetGroupData(UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT nGroupCode = (UINT)wParam;
	RMT_GROUP_DATA* lpGroupData = (RMT_GROUP_DATA*)lParam;
	if (NULL == lpGroupData)
		return;

	CGroupInfo* lpGroupInfo = m_UserMgr.m_GroupList.GetGroupByCode(nGroupCode);
	if (NULL == lpGroupInfo)
		return;

	lpGroupData->bHasGroupInfo = lpGroupInfo->m_bHasGroupInfo;
	lpGroupData->nGroupNum = lpGroupInfo->m_nGroupNumber;
}

void CEdoyunClient::OnInternal_GetGMemberData(UINT message, WPARAM wParam, LPARAM lParam)
{
	RMT_GMEMBER_REQ* lpGMemberReq = (RMT_GMEMBER_REQ*)wParam;
	RMT_BUDDY_DATA* lpGMemberData = (RMT_BUDDY_DATA*)lParam;
	if (NULL == lpGMemberReq || NULL == lpGMemberData)
		return;

	CGroupInfo* lpGroupInfo = m_UserMgr.m_GroupList.GetGroupByCode(lpGMemberReq->nGroupCode);
	if (NULL == lpGroupInfo)
		return;

	CBuddyInfo* lpBuddyInfo = lpGroupInfo->GetMemberByUin(lpGMemberReq->nUTalkUin);
	if (NULL == lpBuddyInfo)
		return;

	lpGMemberData->nUTalkNum = lpBuddyInfo->m_uUserID;

	int nMaxCnt = sizeof(lpGMemberData->szNickName) / sizeof(TCHAR);
	_tcsncpy(lpGMemberData->szNickName, lpBuddyInfo->m_strNickName.c_str(), nMaxCnt);
	lpGMemberData->szNickName[nMaxCnt - 1] = _T('\0');
}

UINT CEdoyunClient::OnInternal_GroupId2Code(UINT message, WPARAM wParam, LPARAM lParam)
{
	CGroupInfo* lpGroupInfo = m_UserMgr.m_GroupList.GetGroupById(lParam);
	return ((lpGroupInfo != NULL) ? lpGroupInfo->m_nGroupCode : 0);
}
// �����û�������Ϣ
void CEdoyunClient::LoadUserConfig()
{
	if (m_UserMgr.m_UserInfo.m_strAccount.empty())
		return;

	PCTSTR pszAccount = m_UserMgr.m_UserInfo.m_strAccount.c_str();

	TCHAR szConfigPath[MAX_PATH] = { 0 };
	_stprintf_s(szConfigPath, MAX_PATH, _T("%sUsers\\%s\\%s.cfg"), g_szHomePath, pszAccount, pszAccount);

	m_UserConfig.LoadConfig(szConfigPath);
}

void CEdoyunClient::SaveUserConfig()
{
	if (m_UserMgr.m_UserInfo.m_strAccount.empty())
		return;

	PCTSTR pszAccount = m_UserMgr.m_UserInfo.m_strAccount.c_str();

	TCHAR szConfigPath[MAX_PATH] = { 0 };
	_stprintf_s(szConfigPath, MAX_PATH, _T("%sUsers\\%s\\%s.cfg"), g_szHomePath, pszAccount, pszAccount);

	m_UserConfig.SaveConfig(szConfigPath);
}

BOOL CEdoyunClient::SetBuddyStatus(UINT uAccountID, long nStatus)
{
	//��������
	if (uAccountID != m_UserMgr.m_UserInfo.m_uUserID)
	{
		m_UserMgr.SetStatus(uAccountID, nStatus);
		return TRUE;
	}

	return FALSE;
}


long CEdoyunClient::ParseBuddyStatus(long nFlag)
{
	long nOnlineClientType = OFFLINE_CLIENT_BOTH;

	if (nFlag & 0x40)
		nOnlineClientType = ONLINE_CLIENT_PC;
	else if (nFlag & 0x80)
		nOnlineClientType = ONLINE_CLIENT_MOBILE;

	long nStatus = STATUS_OFFLINE;
	if (nOnlineClientType == ONLINE_CLIENT_PC)
	{
		if (nFlag & 0x01)
			nStatus = STATUS_ONLINE;
		else if (nFlag & 0x02)
			nStatus = STATUS_INVISIBLE;
		else if (nFlag & 0x03)
			nStatus = STATUS_BUSY;
		else if (nFlag & 0x04)
			nStatus = STATUS_AWAY;

	}
	else if (nOnlineClientType == ONLINE_CLIENT_MOBILE)
	{
		nStatus = STATUS_MOBILE_ONLINE;
	}

	return nStatus;
}



void CEdoyunClient::GoOnline()
{
	if (m_bNetworkAvailable)
		Login();
}

void CEdoyunClient::GoOffline()
{
	//m_IUProtocol.Disconnect();
	//m_IUProtocol.DisconnectFileServer();

	m_UserMgr.ResetToOfflineStatus();
	//m_HeartbeatTask.Stop();

	//m_mapUserStatusCache.clear();

	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_BUDDY_INFO, 0, (LPARAM)m_UserMgr.m_UserInfo.m_uUserID);
	//::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_BUDDY_LIST, 0, 0);
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_RECENT_LIST, 0, 0);
	::SendMessage(m_UserMgr.m_hCallBackWnd, FMG_MSG_UPDATE_GROUP_LIST, 0, 0);
}

void CEdoyunClient::CacheBuddyStatus()
{
	CBuddyTeamInfo* pBuddyTeamInfo = NULL;
	CBuddyInfo* pBuddyInfo = NULL;
	size_t nTeamCount = m_UserMgr.m_BuddyList.m_arrBuddyTeamInfo.size();
	size_t nBuddyCount = 0;
	for (size_t i = 0; i < nTeamCount; ++i)
	{
		pBuddyTeamInfo = m_UserMgr.m_BuddyList.m_arrBuddyTeamInfo[i];
		if (pBuddyTeamInfo == NULL)
			continue;

		nBuddyCount = pBuddyTeamInfo->m_arrBuddyInfo.size();

		for (size_t j = 0; j < nBuddyCount; ++j)
		{
			pBuddyInfo = pBuddyTeamInfo->m_arrBuddyInfo[j];
			if (pBuddyInfo == NULL)
				continue;
			if (pBuddyInfo->m_nStatus != STATUS_OFFLINE)
				m_mapUserStatusCache.insert(std::pair<UINT, long>(pBuddyInfo->m_uUserID, pBuddyInfo->m_nStatus));
		}
	}

	//Ⱥ��Ա������״̬ҲҪ����
	CGroupInfo* pGroupInfo = NULL;
	for (std::vector<CGroupInfo*>::iterator iter = m_UserMgr.m_GroupList.m_arrGroupInfo.begin();
		iter != m_UserMgr.m_GroupList.m_arrGroupInfo.end();
		++iter)
	{
		pGroupInfo = *iter;
		if (pGroupInfo == NULL)
			continue;;

		for (size_t j = 0; j < pGroupInfo->m_arrMember.size(); ++j)
		{
			pBuddyInfo = pGroupInfo->m_arrMember[j];
			if (pBuddyInfo == NULL)
				continue;

			if (pBuddyInfo->m_nStatus != STATUS_OFFLINE)
				m_mapUserStatusCache.insert(std::pair<UINT, long>(pBuddyInfo->m_uUserID, pBuddyInfo->m_nStatus));
		}
	}
}

// ����������
BOOL CEdoyunClient::CreateProxyWnd()
{
	WNDCLASSEX wcex;
	LPCTSTR szWindowClass = _T("UTALK_PROXY_WND");
	HWND hWnd;

	DestroyProxyWnd();	// ���ٴ�����

	HINSTANCE hInstance = ::GetModuleHandle(NULL);

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = ProxyWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = NULL;
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = NULL;

	if (!::RegisterClassEx(&wcex))
		return FALSE;

	hWnd = ::CreateWindow(szWindowClass, NULL, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (NULL == hWnd)
		return FALSE;

	::SetWindowLong(hWnd, GWL_USERDATA, (LONG)this);

	m_RecvMsgThread.SetProxyWnd(hWnd);
	m_UserMgr.m_hProxyWnd = hWnd;

	return TRUE;
}

// ���ٴ�����
BOOL CEdoyunClient::DestroyProxyWnd()
{
	if (m_UserMgr.m_hProxyWnd != NULL)
	{
		::DestroyWindow(m_UserMgr.m_hProxyWnd);
		m_UserMgr.m_hProxyWnd = NULL;
	}
	return TRUE;
}

LRESULT CALLBACK CEdoyunClient::ProxyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	CEdoyunClient* lpFMGClient = (CEdoyunClient*)::GetWindowLong(hWnd, GWL_USERDATA);
	if (NULL == lpFMGClient)
		return ::DefWindowProc(hWnd, message, wParam, lParam);

	if (message < FMG_MSG_FIRST || message > FMG_MSG_LAST)
		return ::DefWindowProc(hWnd, message, wParam, lParam);

	switch (message)
	{
	case FMG_MSG_HEARTBEAT:
		lpFMGClient->OnHeartbeatResult(message, wParam, lParam);
		break;

	case FMG_MSG_NETWORK_STATUS_CHANGE:
		lpFMGClient->OnNetworkStatusChange(message, wParam, lParam);
		break;

	case FMG_MSG_REGISTER:				// ע����
		lpFMGClient->OnRegisterResult(message, wParam, lParam);
		break;
	case FMG_MSG_LOGIN_RESULT:			// ��¼������Ϣ
		lpFMGClient->OnLoginResult(message, wParam, lParam);
		break;
	case FMG_MSG_LOGOUT_RESULT:			// ע��������Ϣ
	case FMG_MSG_UPDATE_BUDDY_HEADPIC:	// ���º���ͷ��
		//::MessageBox(NULL, _T("Change headpic"), _T("Change head"), MB_OK);
	case FMG_MSG_UPDATE_GMEMBER_HEADPIC:	// ����Ⱥ��Աͷ��
	case FMG_MSG_UPDATE_GROUP_HEADPIC:	// ����Ⱥͷ��
		::SendMessage(lpFMGClient->m_UserMgr.m_hCallBackWnd, message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_USER_BASIC_INFO:	//�յ��û��Ļ�����Ϣ
		lpFMGClient->OnUpdateUserBasicInfo(message, wParam, lParam);
		break;

	case FMG_MSG_UPDATE_GROUP_BASIC_INFO:
		lpFMGClient->OnUpdateGroupBasicInfo(message, wParam, lParam);
		break;

	case FMG_MSG_MODIFY_USER_INFO:				//�޸ĸ�����Ϣ���
		lpFMGClient->OnModifyInfoResult(message, wParam, lParam);
		break;
	case FMG_MSG_RECV_USER_STATUS_CHANGE_DATA:
		lpFMGClient->OnRecvUserStatusChangeData(message, wParam, lParam);
		break;

	case FMG_MSG_USER_STATUS_CHANGE:
		lpFMGClient->OnUserStatusChange(message, wParam, lParam);
		break;

	case FMG_MSG_UPLOAD_USER_THUMB:
		lpFMGClient->OnSendConfirmMessage(message, wParam, lParam);
		break;

	case FMG_MSG_UPDATE_USER_CHAT_MSG_ID:
		lpFMGClient->OnUpdateChatMsgID(message, wParam, lParam);
		break;
	case FMG_MSG_FINDFREIND:
		lpFMGClient->OnFindFriend(message, wParam, lParam);
		break;

	case FMG_MSG_DELETEFRIEND:
		lpFMGClient->OnDeleteFriendResult(message, wParam, lParam);
		break;

	case FMG_MSG_RECVADDFRIENDREQUSET:
		lpFMGClient->OnRecvAddFriendRequest(message, wParam, lParam);
		break;

	case FMG_MSG_CUSTOMFACE_AVAILABLE:
		lpFMGClient->OnBuddyCustomFaceAvailable(message, wParam, lParam);
		break;

	case FMG_MSG_MODIFY_PASSWORD_RESULT:
		lpFMGClient->OnModifyPasswordResult(message, wParam, lParam);
		break;

	case FMG_MSG_CREATE_NEW_GROUP_RESULT:
		lpFMGClient->OnCreateNewGroupResult(message, wParam, lParam);
		break;

	case FMG_MSG_UPDATE_BUDDY_LIST:				//���º����б�
		lpFMGClient->OnUpdateBuddyList(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_GROUP_LIST:		// ����Ⱥ�б���Ϣ
		lpFMGClient->OnUpdateGroupList(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_RECENT_LIST:		// ���������ϵ���б���Ϣ
		lpFMGClient->OnUpdateRecentList(message, wParam, lParam);
		break;
	case FMG_MSG_BUDDY_MSG:				// ������Ϣ
		lpFMGClient->OnBuddyMsg(message, wParam, lParam);
		break;
	case FMG_MSG_GROUP_MSG:				// Ⱥ��Ϣ
		lpFMGClient->OnGroupMsg(message, wParam, lParam);
		break;
	case FMG_MSG_SESS_MSG:				// ��ʱ�Ự��Ϣ
		lpFMGClient->OnSessMsg(message, wParam, lParam);
		break;
	case FMG_MSG_STATUS_CHANGE_MSG:		// ����״̬�ı���Ϣ
		lpFMGClient->OnStatusChangeMsg(message, wParam, lParam);
		break;
	case FMG_MSG_KICK_MSG:				// ����������Ϣ
		lpFMGClient->OnKickMsg(message, wParam, lParam);
		break;
	case FMG_MSG_SYS_GROUP_MSG:			// Ⱥϵͳ��Ϣ
		lpFMGClient->OnSysGroupMsg(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_BUDDY_NUMBER:	// ���º��Ѻ���
		lpFMGClient->OnUpdateBuddyNumber(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_GMEMBER_NUMBER:	// ����Ⱥ��Ա����
		lpFMGClient->OnUpdateGMemberNumber(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_GROUP_NUMBER:	// ����Ⱥ����
		lpFMGClient->OnUpdateGroupNumber(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_BUDDY_SIGN:		// ���º��Ѹ���ǩ��
		lpFMGClient->OnUpdateBuddySign(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_GMEMBER_SIGN:	// ����Ⱥ��Ա����ǩ��
		lpFMGClient->OnUpdateGMemberSign(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_BUDDY_INFO:		// �����û���Ϣ
		lpFMGClient->OnUpdateBuddyInfo(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_GMEMBER_INFO:	// ����Ⱥ��Ա��Ϣ
		lpFMGClient->OnUpdateGMemberInfo(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_GROUP_INFO:		// ����Ⱥ��Ϣ
		lpFMGClient->OnUpdateGroupInfo(message, wParam, lParam);
		break;
	case FMG_MSG_UPDATE_C2CMSGSIG:		// ������ʱ�Ự����
		//lpFMGClient->OnUpdateC2CMsgSig(message, wParam, lParam);
		break;
	case FMG_MSG_CHANGE_STATUS_RESULT:	// �ı�����״̬������Ϣ
		lpFMGClient->OnChangeStatusResult(message, wParam, lParam);
		break;
	case FMG_MSG_TARGET_INFO_CHANGE:		//���û���Ϣ�����ı䣺
		lpFMGClient->OnTargetInfoChange(message, wParam, lParam);
		break;

	case FMG_MSG_INTERNAL_GETBUDDYDATA:
		lpFMGClient->OnInternal_GetBuddyData(message, wParam, lParam);
		break;
	case FMG_MSG_INTERNAL_GETGROUPDATA:
		lpFMGClient->OnInternal_GetGroupData(message, wParam, lParam);
		break;
	case FMG_MSG_INTERNAL_GETGMEMBERDATA:
		lpFMGClient->OnInternal_GetGMemberData(message, wParam, lParam);
		break;
	case FMG_MSG_INTERNAL_GROUPID2CODE:
		return lpFMGClient->OnInternal_GroupId2Code(message, wParam, lParam);
		break;

	default:
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
