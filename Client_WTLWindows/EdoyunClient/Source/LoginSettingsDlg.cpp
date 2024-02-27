#include "stdafx.h"
#include "LoginSettingsDlg.h"
#include "File.h"
#include "IniFile.h"
#include "UserSessionData.h"
#include "EdoyunIMClient.h"

enum PROXY_TYPE
{
	NOT_USE_PROXY     = 0,
	USE_BROWSER_PROXY = 1,
	USE_HTTP_PROXY    = 2,
	USE_SOCKS5_PROXY  = 3
};

CLoginSettingsDlg::CLoginSettingsDlg()
{
	memset(m_szSrvAddr, 0, sizeof(m_szSrvAddr));
	memset(m_szFileSrvAddr, 0, sizeof(m_szFileSrvAddr));
	memset(m_szSrvPort, 0, sizeof(m_szSrvPort));
	memset(m_szProxyAddr, 0, sizeof(m_szProxyAddr));
	memset(m_szProxyPort, 0, sizeof(m_szProxyPort));
	m_pClient = NULL;
};

CLoginSettingsDlg::~CLoginSettingsDlg()
{

};

BOOL CLoginSettingsDlg::PreTranslateMessage(MSG* pMsg)
{
	//TODO: ���Ϊʲôֻ�н����ڿؼ��ϲ�������߼���
	//֧��Esc���رնԻ���
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam==VK_ESCAPE)
 	{
		PostMessage(WM_COMMAND, (WPARAM)ID_LOGINSETTINGCANCEL, 0);
		return TRUE;
 	}

	return CWindow::IsDialogMessage(pMsg);
}


BOOL CLoginSettingsDlg::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	
	// center the dialog on the screen
	InitUI();
	//ModifyStyleEx(WS_EX_APPWINDOW, WS_EX_TOOLWINDOW);
	CenterWindow();
	return TRUE;
}


BOOL CLoginSettingsDlg::InitUI()
{
	m_SkinDlg.SetBgPic(_T("DlgBg\\LoginSettingDlgBg.png"));
	m_SkinDlg.SetCloseSysBtnPic(_T("SysBtn\\btn_close_normal.png"), _T("SysBtn\\btn_close_highlight.png"), _T("SysBtn\\btn_close_down.png"));
	m_SkinDlg.SubclassWindow(m_hWnd);
	m_SkinDlg.SetTitleText(_T("��������"));
	m_SkinDlg.MoveWindow(0, 0, 550, 380, TRUE);
	

	HDC hDlgBgDC = m_SkinDlg.GetBgDC();
	m_staticSrvAddr.SubclassWindow(GetDlgItem(IDC_STATIC_SERADDRESS));
	m_staticSrvAddr.SetTransparent(TRUE, hDlgBgDC);
	//m_staticSrvAddr.MoveWindow(60, 65, 60, 25, TRUE);
	m_staticSrvAddr.SetFocus();

	m_staticSrvPort.SubclassWindow(GetDlgItem(IDC_STATIC_SERPORT));
	m_staticSrvPort.SetTransparent(TRUE, hDlgBgDC);
	//m_staticSrvPort.MoveWindow(215, 65, 60, 25, TRUE);

	m_staticFileSrvAddr.SubclassWindow(GetDlgItem(IDC_STATIC_FILESERVER));
	m_staticFileSrvAddr.SetTransparent(TRUE, hDlgBgDC);
	//m_staticFileSrvAddr.MoveWindow(250, 65, 60, 25, TRUE);

	m_staticFilePort.SubclassWindow(GetDlgItem(IDC_STATIC_FILEPORT));
	m_staticFilePort.SetTransparent(TRUE, hDlgBgDC);
	//m_staticFilePort.MoveWindow(370, 65, 60, 25, TRUE);

	m_comboProxyType.SubclassWindow(GetDlgItem(IDC_COMBO_PROTYPE));
	const TCHAR szProxyType[][10] = 
	{
		_T("��ʹ�ô���"),
		_T("ʹ�����������"),
		_T("HTTP����"),
		_T("SOCKS5����")	
	};
	for(long i=0; i<ARRAYSIZE(szProxyType); ++i)
	{
		m_comboProxyType.InsertString(i, szProxyType[i]);
	}

	m_comboProxyType.SetBgNormalPic(_T("frameBorderEffect_normalDraw.png"), CRect(2,2,2,2));
	m_comboProxyType.SetBgHotPic(_T("frameBorderEffect_mouseDownDraw.png"), CRect(2,2,2,2));
	m_comboProxyType.SetArrowNormalPic(_T("ComboBox\\inputbtn_normal.png"));
	m_comboProxyType.SetArrowHotPic(_T("ComboBox\\inputbtn_highlight.png"));
	m_comboProxyType.SetArrowPushedPic(_T("ComboBox\\inputbtn_down.png"));
	
	m_comboProxyType.SetArrowWidth(28);
	m_comboProxyType.SetTransparent(TRUE, hDlgBgDC);
	//m_comboProxyType.MoveWindow(55, 180, 127, 30, FALSE);
	m_comboProxyType.SetItemHeight(-1, 26);
	//TODO: ��Ϊֻ���Ժ󱳾��Ͼͻ����һ���˺ţ���ֵ�bug����
	//m_comboProxyType.SetReadOnly(TRUE);

	CIniFile iniFile;
	CString strIniPath(g_szHomePath);
	strIniPath += _T("config\\iu.ini");
	long nSel = iniFile.ReadInt(_T("server"), _T("proxyType"), 0, strIniPath);
	if(nSel == -1)
		nSel = 0;
	m_comboProxyType.SetCurSel(nSel);
	
	
	m_editSrvAddr.SetBgNormalPic(_T("frameBorderEffect_normalDraw.png"), CRect(2,2,2,2));
	m_editSrvAddr.SetBgHotPic(_T("frameBorderEffect_mouseDownDraw.png"), CRect(2,2,2,2));
	m_editSrvAddr.SetTransparent(TRUE, hDlgBgDC);
	m_editSrvAddr.SubclassWindow(GetDlgItem(IDC_EDIT_SERADDRESS));
	//m_editSrvAddr.MoveWindow(55, 90, 127, 30, TRUE);
	CString strTemp;
	iniFile.ReadString(_T("server"), _T("server"), _T("code.edoyun.com"), strTemp.GetBuffer(64), 64, strIniPath);
	strTemp.ReleaseBuffer();
	m_editSrvAddr.SetWindowText(strTemp);

	m_editFileSrvAddr.SetBgNormalPic(_T("frameBorderEffect_normalDraw.png"), CRect(2,2,2,2));
	m_editFileSrvAddr.SetBgHotPic(_T("frameBorderEffect_mouseDownDraw.png"), CRect(2,2,2,2));
	m_editFileSrvAddr.SetTransparent(TRUE, hDlgBgDC);
	m_editFileSrvAddr.SubclassWindow(GetDlgItem(IDC_EDIT_FILESERVER));
	iniFile.ReadString(_T("server"), _T("fileserver"), _T("code.edoyun.com"), strTemp.GetBuffer(64), 64, strIniPath);
	strTemp.ReleaseBuffer();
	m_editFileSrvAddr.SetWindowText(strTemp);

	m_edtSrvPort.SetBgNormalPic(_T("frameBorderEffect_normalDraw.png"), CRect(2,2,2,2));
	m_edtSrvPort.SetBgHotPic(_T("frameBorderEffect_mouseDownDraw.png"), CRect(2,2,2,2));
	m_edtSrvPort.SubclassWindow(GetDlgItem(IDC_EDIT_SERPORT));
	//m_edtSrvPort.MoveWindow(215, 90, 126, 30, TRUE);
	iniFile.ReadString(_T("server"), _T("port"), _T("25251"), strTemp.GetBuffer(32), 32, strIniPath);
	strTemp.ReleaseBuffer();
	m_edtSrvPort.SetWindowText(strTemp);

	m_edtFilePort.SetBgNormalPic(_T("frameBorderEffect_normalDraw.png"), CRect(2,2,2,2));
	m_edtFilePort.SetBgHotPic(_T("frameBorderEffect_mouseDownDraw.png"), CRect(2,2,2,2));
	m_edtFilePort.SubclassWindow(GetDlgItem(IDC_EDIT_FILEPORT));
	//m_edtFilePort.MoveWindow(370, 90, 126, 30, TRUE);
	iniFile.ReadString(_T("server"), _T("filePort"), _T("25261"), strTemp.GetBuffer(32), 32, strIniPath);
	strTemp.ReleaseBuffer();
	m_edtFilePort.SetWindowText(strTemp);
	

	m_editProxyAddr.SetBgNormalPic(_T("frameBorderEffect_normalDraw.png"), CRect(2,2,2,2));
	m_editProxyAddr.SetBgHotPic(_T("frameBorderEffect_mouseDownDraw.png"), CRect(2,2,2,2));
	m_editProxyAddr.SubclassWindow(GetDlgItem(IDC_EDIT_PROADD));
	m_editProxyAddr.SetTransparent(TRUE, hDlgBgDC);
	//m_editProxyAddr.MoveWindow(212, 180, 126, 30, TRUE);
	if(nSel > USE_BROWSER_PROXY)
	{
		iniFile.ReadString(_T("server"), _T("proxyServer"), _T(""), strTemp.GetBuffer(32), 32, strIniPath);
		strTemp.ReleaseBuffer();
		m_editProxyAddr.SetWindowText(strTemp);
	}

	m_edtProxyPort.SetBgNormalPic(_T("frameBorderEffect_normalDraw.png"), CRect(2,2,2,2));
	m_edtProxyPort.SetBgHotPic(_T("frameBorderEffect_mouseDownDraw.png"), CRect(2,2,2,2));
	m_edtProxyPort.SubclassWindow(GetDlgItem(IDC_EDIT_PROPORT));
	//m_edtProxyPort.MoveWindow(368, 180, 127, 30, TRUE);
	if(nSel > USE_BROWSER_PROXY)
	{
		iniFile.ReadString(_T("server"), _T("proxyPort"), _T(""), strTemp.GetBuffer(32), 32, strIniPath);
		strTemp.ReleaseBuffer();
		m_edtProxyPort.SetWindowText(strTemp);
	}

	m_btnOK.SetButtonType(SKIN_PUSH_BUTTON);
	m_btnOK.SetTransparent(TRUE, hDlgBgDC);
	m_btnOK.SetBgPic(_T("Button\\btn_normal.png"), _T("Button\\btn_highlight.png"), _T("Button\\btn_down.png"), _T("Button\\btn_focus.png"));
	m_btnOK.SetRound(4, 4);
	m_btnOK.SubclassWindow(GetDlgItem(ID_LOGINSETTING));
	//m_btnOK.MoveWindow(165, 265, 94, 30, TRUE);

	m_btnCancel.SetButtonType(SKIN_PUSH_BUTTON);
	m_btnCancel.SetTransparent(TRUE, hDlgBgDC);
	m_btnCancel.SetBgPic(_T("Button\\btn_normal.png"), _T("Button\\btn_highlight.png"), _T("Button\\btn_down.png"), _T("Button\\btn_focus.png"));
	m_btnCancel.SetRound(4, 4);
	m_btnCancel.SubclassWindow(GetDlgItem(ID_LOGINSETTINGCANCEL));
	//m_btnCancel.MoveWindow(290, 265, 94, 30, TRUE);

	BOOL bEnabled = (m_comboProxyType.GetCurSel() > USE_BROWSER_PROXY ? TRUE : FALSE);
	m_editProxyAddr.EnableWindow(bEnabled);
	m_edtProxyPort.EnableWindow(bEnabled);

	return TRUE;
}

void CLoginSettingsDlg::OnClose()
{
	EndDialog(IDCANCEL);
}

void CLoginSettingsDlg::OnDestroy()
{
	UninitUI();

	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
}

void CLoginSettingsDlg::OnComboBox_Select(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	BOOL bEnabled = (m_comboProxyType.GetCurSel() > USE_BROWSER_PROXY ? TRUE : FALSE);
	m_editProxyAddr.EnableWindow(bEnabled);
	m_edtProxyPort.EnableWindow(bEnabled);
}

void CLoginSettingsDlg::UninitUI()
{
	if (m_editSrvAddr.IsWindow())
		m_editSrvAddr.DestroyWindow();

	if (m_editFileSrvAddr.IsWindow())
		m_editFileSrvAddr.DestroyWindow();

	if (m_edtSrvPort.IsWindow())
		m_edtSrvPort.DestroyWindow();

	if (m_editProxyAddr.IsWindow())
		m_editProxyAddr.DestroyWindow();

	if (m_edtProxyPort.IsWindow())
		m_edtProxyPort.DestroyWindow();

	if (m_btnOK.IsWindow())
		m_btnOK.DestroyWindow();

	if (m_btnCancel.IsWindow())
		m_btnCancel.DestroyWindow();

}

void CLoginSettingsDlg::OnBtn_OK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	m_editSrvAddr.GetWindowText(m_szSrvAddr, MAX_SRV_ADDR);
	if(m_szSrvAddr[0]==NULL || m_szSrvAddr[0]==_T(' '))
	{
		::MessageBox(m_hWnd, _T("��������ַ����Ϊ�գ�"), _T("EdoyunIMClient"), MB_OK|MB_ICONINFORMATION);
		m_editSrvAddr.SetFocus();
		return;
	}

	m_editFileSrvAddr.GetWindowText(m_szFileSrvAddr, MAX_SRV_ADDR);
	if(m_szFileSrvAddr[0]==NULL || m_szFileSrvAddr[0]==_T(' '))
	{
		::MessageBox(m_hWnd, _T("�ļ������ַ����Ϊ�գ�"), _T("EdoyunIMClient"), MB_OK|MB_ICONINFORMATION);
		m_editFileSrvAddr.SetFocus();
		return;
	}

	m_edtSrvPort.GetWindowText(m_szSrvPort, MAX_SRV_PORT);
	if(m_szSrvPort[0]==NULL || m_szSrvPort[0]==_T(' '))
	{
		::MessageBox(m_hWnd, _T("����˿ںŲ���Ϊ�գ�"), _T("EdoyunIMClient"), MB_OK|MB_ICONINFORMATION);
		m_edtSrvPort.SetFocus();
		return;
	}

	m_edtFilePort.GetWindowText(m_szFilePort, MAX_SRV_PORT);
	if(m_szFilePort[0]==NULL || m_szFilePort[0]==_T(' '))
	{
		::MessageBox(m_hWnd, _T("�ļ��˿ںŲ���Ϊ�գ�"), _T("EdoyunIMClient"), MB_OK|MB_ICONINFORMATION);
		m_edtFilePort.SetFocus();
		return;
	}
	
	long nCurSel = m_comboProxyType.GetCurSel();
	if(nCurSel > USE_BROWSER_PROXY)			
	{
		m_editProxyAddr.GetWindowText(m_szProxyAddr, MAX_SRV_ADDR);
		if(m_szProxyAddr[0]==NULL || m_szProxyAddr[0]==_T(' '))
		{
			::MessageBox(m_hWnd, _T("�����������ַ����Ϊ�գ�"), _T("EdoyunIMClient"), MB_OK|MB_ICONINFORMATION);
			m_editProxyAddr.SetFocus();
			return;
		}

		m_edtProxyPort.GetWindowText(m_szProxyPort, MAX_SRV_PORT);
		if(m_szProxyPort[0]==NULL || m_szProxyPort[0]==_T(' '))
		{
			::MessageBox(m_hWnd, _T("����˿ںŲ���Ϊ�գ�"), _T("EdoyunIMClient"), MB_OK|MB_ICONINFORMATION);
			m_edtProxyPort.SetFocus();
			return;
		}
	}

	CIniFile iniFile;
	CString strIniPath(g_szHomePath);
	strIniPath += _T("config\\iu.ini");
	iniFile.WriteString(_T("server"), _T("server"), m_szSrvAddr, strIniPath); 
	iniFile.WriteString(_T("server"), _T("fileserver"), m_szFileSrvAddr, strIniPath); 
	iniFile.WriteString(_T("server"), _T("port"), m_szSrvPort, strIniPath); 
	iniFile.WriteString(_T("server"), _T("filePort"), m_szFilePort, strIniPath);
	
	iniFile.WriteInt(_T("server"), _T("proxyType"), nCurSel, strIniPath);
	if(nCurSel > USE_BROWSER_PROXY)
	{
		iniFile.WriteString(_T("server"), _T("proxyServer"), m_szProxyAddr, strIniPath); 
		iniFile.WriteString(_T("server"), _T("proxyPort"), m_szProxyPort, strIniPath);
	}

	m_pClient->SetServer(m_szSrvAddr);
	m_pClient->SetFileServer(m_szFileSrvAddr);
	m_pClient->SetPort((short)_wtol(m_szSrvPort));
	m_pClient->SetFilePort((short)_wtol(m_szFilePort));
	EndDialog(IDOK);
}

void CLoginSettingsDlg::OnBtn_Cancel(UINT uNotifyCode, int nID, CWindow wndCtl)	
{
	EndDialog(IDCANCEL);
}