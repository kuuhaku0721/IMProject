#pragma once

#include <thread>
#include <mutex>
#include <string>
#include <memory>
#include <condition_variable>

class CRecvMsgThread;

//����ͨ�Ų�ֻ�������ݴ���ͽ���
class CIUSocket
{
public:
	CIUSocket(CRecvMsgThread* pThread);
    ~CIUSocket(void);

    CIUSocket(const CIUSocket& rhs) = delete;
    CIUSocket& operator = (const CIUSocket& rhs) = delete;

    bool Init();
    void Uninit();

    void Join();
	
	void	LoadConfig();
	void	SetServer(PCTSTR lpszServer);
	void	SetFileServer(PCTSTR lpszFileServer);
	void	SetProxyServer(PCTSTR lpszProxyServer);
	void	SetPort(short nPort);
	void    SetFilePort(short nFilePort);
	void	SetProxyPort(short nProxyPort);
	void    SetProxyType(long nProxyType);

	PCTSTR  GetServer() const;
	PCTSTR	GetFileServer() const;
	short   GetPort() const;
    short	GetFilePort() const;

    /** 
    *@param timeout ��ʱʱ�䣬��λΪs
    **/
	BOOL	Connect(int timeout = 3);
	BOOL	ConnectToFileServer();
	
	BOOL	IsClosed();
	BOOL	IsFileServerClosed();
	
	void	Close();
	void	CloseFileServerConnection();

	bool    CheckReceivedData();							//�ж���ͨSocket���Ƿ��յ����ݣ��з���true��û�з���false

	//�첽�ӿ�
    void    Send(const std::string& strBuffer);
	
	//ͬ���ӿ�
	BOOL    SendOnFilePort(const char* pBuffer, long nSize);	
	BOOL	RecvOnFilePort(char* pBuffer, long nSize);

private:   
    void    SendThreadProc();
    void    RecvThreadProc();

    bool	Send();
    bool	Recv();
    

private:	
	SOCKET							m_hSocket;				//һ����;Socket��������socket��
	SOCKET							m_hFileSocket;			//���ļ���Socket������socket��
	short							m_nPort;
	short							m_nFilePort;
	CString							m_strServer;			//��������ַ
	CString							m_strFileServer;		//�ļ���������ַ
	
	long							m_nProxyType;			//�������ͣ�0��ʹ�ô�����
	CString							m_strProxyServer;		//�����������ַ
	short							m_nProxyPort;			//����������˿ں�
	
	BOOL							m_bConnected;
	BOOL							m_bConnectedOnFileSocket;

    std::shared_ptr<std::thread>    m_spSendThread;
    std::shared_ptr<std::thread>    m_spRecvThread;

    std::string                     m_strSendBuf;
    std::string                     m_strRecvBuf;

    std::mutex                      m_mtSendBuf;

    std::condition_variable         m_cvSendBuf;
    std::condition_variable         m_cvRecvBuf;

    bool                            m_bStop;

	CRecvMsgThread*					m_pRecvMsgThread;
};