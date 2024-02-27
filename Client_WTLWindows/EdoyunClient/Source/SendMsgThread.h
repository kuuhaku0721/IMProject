#pragma once
#include <list>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "net/IUProtocolData.h"
#include "Utils.h"
#include "Thread.h"

class CEdoyunUserMgr;
class CEdoyunClient;
class CBuddyMessage;
class CGroupMessage;
class CSessMessage;
class CContent;

// д��һ��������Ϣ��¼
void WriteBuddyMsgLog(CEdoyunUserMgr* lpUserMgr, UINT nUTalkNum, LPCTSTR lpNickName, 
					  BOOL bSelf, CBuddyMessage* lpMsg);

// д��һ��Ⱥ��Ϣ��¼
void WriteGroupMsgLog(CEdoyunUserMgr* lpUserMgr, UINT nGroupNum, UINT nUTalkNum, 
					  LPCTSTR lpNickName, CBuddyMessage* lpMsg);

// д��һ����ʱ�Ự(Ⱥ��Ա)��Ϣ��¼
void WriteSessMsgLog(CEdoyunUserMgr* lpUserMgr, UINT nUTalkNum, LPCTSTR lpNickName, 
					 BOOL bSelf, CSessMessage* lpMsg);

class CMsgItem
{
public:
	CMsgItem(void);
	~CMsgItem(void);

public:
	long				m_nType;
	void*				m_lpMsg;
	UINT				m_nGroupNum;		
	UINT				m_nUTalkNum;		//TODO: ��Ҫɾ��
	std::vector<UINT>	m_arrTargetIDs;		//����Ϣ��Ŀ���û�ID
	tstring				m_strNickName;
	tstring				m_strGroupSig;
	HWND				m_hwndFrom;			//��Ϣ�����ĸ����촰��
};

class CNetData;
class CRegisterRequest;
class CLoginRequest;
class CUserBasicInfoRequest;
class CUserExtendInfoRequest;
class CLoginUserFriendsIDRequest;
class CSendChatMessage;
class CSendChatConfirmImageMessage;
class CFindFriendRequest;
class COperateFriendRequest;
class CHeartbeatMessageRequest;
class CUpdateLogonUserInfoRequest;
class CModifyPasswordRequest;
class CCreateNewGroupRequest;

class CIUSocket;

class CSendMsgThread : public CThread
{
public:
    CSendMsgThread(CIUSocket* socketClient);
    virtual ~CSendMsgThread(void);

public:
	void AddItem(CNetData* pItem);
	void DeleteAllItems();
	
    BOOL AddBuddyMsg(UINT nFromUin, const tstring& strFromNickName, UINT nToUin, const tstring& strToNickName, time_t nTime, const tstring& strChatMsg, HWND hwndFrom = NULL);
	BOOL AddGroupMsg(UINT nGroupId, time_t nTime, LPCTSTR lpMsg, HWND hwndFrom=NULL);
	BOOL AddMultiMsg(const std::set<UINT> setAccountID, time_t nTime, LPCTSTR lpMsg, HWND hwndFrom=NULL);
	BOOL AddSessMsg(UINT nGroupId, UINT nToUin, time_t nTime, LPCTSTR lpMsg);

	virtual void Stop() override;

protected:
    virtual void Run() override;

private:
    void HandleItem(CNetData* pNetData);
	void HandleRegister(const CRegisterRequest* pRegisterRequest);
    void HandleLogon(const CLoginRequest* pLoginRequest);
	void HandleUserBasicInfo(const CUserBasicInfoRequest* pUserBasicInfo);
    void HandleGroupBasicInfo(const CGroupBasicInfoRequest* pGroupBasicInfo);
	BOOL HandleSentChatMessage(const CSendChatMessage* pSentChatMessage);				
	BOOL HandleSentConfirmImageMessage(const CSendChatConfirmImageMessage* pConfirmImageMessage);	//ͼƬ�ϴ��ɹ��Ժ��ȷ����Ϣ
	void HandleFindFriendMessage(const CFindFriendRequest* pFindFriendRequest);
	void HandleOperateFriendMessage(const COperateFriendRequest* pOperateFriendRequest);
	BOOL HandleHeartbeatMessage(const CHeartbeatMessageRequest* pHeartbeatRequest);
	void HandleUpdateLogonUserInfoMessage(const CUpdateLogonUserInfoRequest* pRequest);
	void HandleModifyPassword(const CModifyPasswordRequest* pModifyPassword);
	void HandleCreateNewGroup(const CCreateNewGroupRequest* pCreateNewGroup);

	BOOL HandleFontInfo(LPCTSTR& p, tstring& strText, std::vector<CContent*>& arrContent);
	BOOL HandleSysFaceId(LPCTSTR& p, tstring& strText, std::vector<CContent*>& arrContent);
	BOOL HandleShakeWindowMsg(LPCTSTR& p, tstring& strText, std::vector<CContent*>& arrContent);
	BOOL HandleCustomPic(LPCTSTR& p, tstring& strText, std::vector<CContent*>& arrContent);
	BOOL HandleFile(LPCTSTR& p, tstring& strText, std::vector<CContent*>& arrContent);
	BOOL CreateMsgContent(const tstring& strChatMsg, std::vector<CContent*>& arrContent);
	
    //TODO: ���ĸ�������ʼ���Ժϲ���һ������
    BOOL SendBuddyMsg(CMsgItem* lpMsgItem);			// ���ͺ�����Ϣ
	BOOL SendGroupMsg(CMsgItem* lpMsgItem);			// ����Ⱥ��Ϣ
	BOOL SendMultiMsg(CMsgItem* lpMsgItem);			// Ⱥ����Ϣ
    BOOL SendMultiChatMessage(const char* pszChatMsg, int nChatMsgLength, UINT* pAccountList, int nAccountNum);
	BOOL SendSessMsg(CMsgItem* lpMsgItem);			// ����Ⱥ��Ա��Ϣ
    
	
	BOOL ProcessBuddyMsg(CBuddyMessage* pMsg);		//�Է��͵���Ϣ���мӹ�
	BOOL ProcessMultiMsg(CMsgItem* pMsgItem);		//��Ⱥ����Ϣ���мӹ�
	void AddItemToRecentSessionList(UINT uUserID, UINT uFaceID, PCTSTR pszNickName, PCTSTR pszText, time_t nMsgTime);		//�������ϵ���б������һ��
	std::wstring UnicodeToHexStr(const WCHAR* lpStr, BOOL bDblSlash);

public:
	CEdoyunClient*				m_lpFMGClient;
	CEdoyunUserMgr*					m_lpUserMgr;

private:
	UINT						m_nMsgId;
	tstring						m_strGFaceKey;
	tstring						m_strGFaceSig;
	std::vector<CMsgItem*>		m_arrItem;

	std::list<CNetData*>		m_listItems;
    std::mutex                  m_mtItems;
    std::condition_variable     m_cvItems;

    int                         m_seq{};            //�����к�

    CIUSocket*                  m_SocketClient;
};
