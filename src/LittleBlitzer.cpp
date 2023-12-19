#include "stdafx.h"
#include "LittleBlitzer.h"
#include "LittleBlitzerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CLittleBlitzerApp, CWinAppEx)
   ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CLittleBlitzerApp::CLittleBlitzerApp()
= default;

CLittleBlitzerApp theApp;

BOOL CLittleBlitzerApp::InitInstance()
{
   INITCOMMONCONTROLSEX InitCtrls;
   InitCtrls.dwSize = sizeof(InitCtrls);
   InitCtrls.dwICC = ICC_WIN95_CLASSES;
   InitCommonControlsEx(&InitCtrls);

   CWinAppEx::InitInstance();

   AfxEnableControlContainer();

   SetRegistryKey(_T("Local AppWizard-Generated Applications"));

   CLittleBlitzerDlg dlg;
   m_pMainWnd = &dlg;
   if (const INT_PTR nResponse = dlg.DoModal(); nResponse == IDOK)
   {
   }
   else if (nResponse == IDCANCEL)
   {
   }

   return FALSE;
}
