

#include "stdafx.h"

#include "resource.h"
#include "Startup.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "UpdateDlg.h"
#include "Updater.h"
#include "LoginSettingsDlg.h"
#include "File.h"
#include "IULog.h"
#include "UserSessionData.h"
#include "Utils.h"

CAppModule _Module;

//����g_hwndOwner��˵����
//֮���Դ����������ΪΪ�˲��������ں͵�¼�Ի�������������ʾ��Ч����
//��������ؽ����ڷ����WS_EX_APPWINDOW����ΪWS_EX_TOOLWINDOW�󣬴��ַ��Ĵ���
//����ʧȥ�����Ĭ��Z���Ϊ0������ر����ᡣ
HWND	   g_hwndOwner = NULL;	

HWND CreateOwnerWindow()
{
	PCTSTR pszOwnerWindowClass = _T("__EdoyunIMClient_Owner__");
	HINSTANCE hInstance = ::GetModuleHandle(NULL);
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = DefWindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = NULL;
	wcex.hCursor = NULL;
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = pszOwnerWindowClass;
	wcex.hIconSm = NULL;

	if (!::RegisterClassEx(&wcex))
		return NULL;

	HWND hOwnerWindow = ::CreateWindow(pszOwnerWindowClass, NULL, WS_OVERLAPPEDWINDOW, 
									   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	
	return hOwnerWindow;
}

int Run(LPTSTR /*lpstrCmdLine = NULL*/, int nCmdShow/* = SW_SHOWDEFAULT*/)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainDlg dlgMain;
	
	g_hwndOwner = CreateOwnerWindow();
	if(dlgMain.Create(g_hwndOwner) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}


BOOL InitSocket()
{
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	int nErrorID = ::WSAStartup(wVersionRequested, &wsaData);
	if(nErrorID != 0)
		return FALSE;

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        UnInitSocket();
        return FALSE;
    }

	return TRUE;
}

void UnInitSocket()
{
	::WSACleanup();
}

//Ĭ�ϵ���ں��� WinMain main _tWinMain _tmain
int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	_CrtSetDebugFillThreshold( 0 );
#endif
	_tcscpy_s(g_szHomePath, MAX_PATH, Edoyun::CPath::GetAppPath().c_str());
	strcpy_s(g_szHomePathAscii, MAX_PATH, Edoyun::CPath::GetAppPathAscii().c_str());

	SYSTEMTIME st = {0};
	::GetLocalTime(&st);
	TCHAR szLogFileName[MAX_PATH] = {0};
	_stprintf_s(szLogFileName, MAX_PATH, _T("%sLog\\%04d%02d%02d%02d%02d%02d.log"), g_szHomePath, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	CIULog::Init(TRUE, szLogFileName);


	if(!InitSocket())
		return 0;

	tstring strFileName = Edoyun::CPath::GetAppPath() + _T("richFace.dll");
	BOOL bRet = DllRegisterServer(strFileName.c_str());	// ע��COM���
	if (!bRet)
	{
		::MessageBox(NULL, _T("COM���ע��ʧ�ܣ�Ӧ�ó����޷���ɳ�ʼ��������"), _T("��ʾ"), MB_OK);
		return 0;
	}

	HRESULT hRes = ::OleInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls
	HMODULE hRichEditDll = ::LoadLibrary(CRichEditCtrl::GetLibraryName());	// ����RichEdit�ؼ�DLL

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	CSkinManager::Init();	// ��ʼ��Ƥ��������
	
	tstring strSkinPath = Edoyun::CPath::GetAppPath() + _T("Skins\\");	// ����Ƥ���ļ���·��
	CSkinManager::GetInstance()->SetSkinPath(strSkinPath.c_str());
	
	CSkinManager::GetInstance()->LoadConfigXml();	// ����Ƥ���б������ļ�

	int nRet = Run(lpstrCmdLine, nCmdShow);

	CSkinManager::UnInit();	// ����ʼ��Ƥ��������

	if (hRichEditDll != NULL)		// ж��RichEdit�ؼ�DLL
	{
		::FreeLibrary(hRichEditDll);
		hRichEditDll = NULL;
	}

	_Module.Term();

	UnInitSocket();

	CIULog::Unit();
	
	::OleUninitialize();

	return nRet;
}
