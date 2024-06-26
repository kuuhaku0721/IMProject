// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "aboutdlg.h"
#include "UserSessionData.h"
#include "File.h"
#include "MiniBuffer.h"
#include "EncodingUtil.h"
#include "Utils.h"
#include "GDIFactory.h"

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_SkinDlg.SetBgPic(_T("LoginPanel_window_windowBkg.png"), CRect(2, 24, 2, 0));
	m_SkinDlg.SetCloseSysBtnPic(_T("SysBtn\\btn_close_normal.png"), _T("SysBtn\\btn_close_highlight.png"), _T("SysBtn\\btn_close_down.png"));
	m_SkinDlg.SubclassWindow(m_hWnd);
	m_SkinDlg.MoveWindow(0, 0, 430, 450,FALSE);
	m_SkinDlg.SetTitleText(_T("关于Edoyun EdoyunIMClient"));


	HDC hDlgBgDc = m_SkinDlg.GetBgDC();
	m_staAboutInfo.SubclassWindow(GetDlgItem(IDC_ABOUT_EdoyunIMClient));
	m_staAboutInfo.SetTransparent(TRUE, hDlgBgDc);

	m_hyperLinkVersion.SubclassWindow(GetDlgItem(IDC_STATIC_VERSION));
	m_hyperLinkVersion.SetTransparent(TRUE, hDlgBgDc);

	HFONT hFont = CGDIFactory::GetFont(20);

	m_hyperLinkVersion.SetNormalFont(hFont);
	m_hyperLinkVersion.SetLinkColor(RGB(22, 112, 235));
	m_hyperLinkVersion.SetHoverLinkColor(RGB(22, 112, 235));
	m_hyperLinkVersion.SetVisitedLinkColor(RGB(22, 112, 235));
	m_hyperLinkVersion.SetHyperLink(_T("http:// www.edoyun.com/"));

	CenterWindow(::GetDesktopWindow());

	CString strVersionInfoPath;
	strVersionInfoPath.Format(_T("%supdate\\update.version"), g_szHomePath);
	CFile fileVersion;
	if(!fileVersion.Open(strVersionInfoPath, FALSE))
		return FALSE;

	const char* pBuffer = fileVersion.Read();
	CString strText(_T("EdoyunIMClient Application Copyright 2020"));
	if (pBuffer != NULL)
	{
		wchar_t* lpBuffer = new wchar_t[strlen(pBuffer)* 2 + 1];
		AnsiToUnicode(pBuffer, lpBuffer, strlen(pBuffer)* 2);
		CString strVersion = GetBetweenString(lpBuffer, _T("<version>"), _T("</version>")).c_str();
		strText.Format(_T("EdoyunIMClient v%s Copyright 2020"), strVersion);
		DEL_ARR(lpBuffer);
	}

	if(strText.IsEmpty())
		return FALSE;
	
	fileVersion.Close();

	m_hyperLinkVersion.SetWindowText(strText);
	
	//CFile2 cAboutInfoFlie;
	//CString strText;
	//CMiniBuffer strBuff(1000);
	//cAboutInfoFlie.Open(strAboutInfoPath, TRUE);
	//cAboutInfoFlie.Read((void*)strBuff.GetBuffer(), 1000);
	CString strAboutInfoPath;
	strAboutInfoPath.Format(_T("%sconfig\\AboutInfo.txt"), g_szHomePath);
	CFile file;
	if(!file.Open(strAboutInfoPath, FALSE))
		return FALSE;

	pBuffer = file.Read();
	strText = _T("www.edoyun.com 版权所有");

	if(pBuffer != NULL)
	{
		AnsiToUnicode(pBuffer, strText.GetBuffer(file.GetSize()*2), file.GetSize()*2);
		strText.ReleaseBuffer();
	}

	if(strText.IsEmpty())
		return FALSE;

	file.Close();

	m_staAboutInfo.SetWindowText(strText);

	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);

	return 0;
}
