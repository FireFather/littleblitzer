#include "stdafx.h"
#include "LittleBlitzer.h"
#include "LittleBlitzerDlg.h"

#include <cstring>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
TBatchOptions ParseBatchOptions()
{
   TBatchOptions options;
   for (int i = 1; i < __argc; ++i)
   {
      if (_stricmp(__argv[i], "--batch") == 0)
      {
         options.enabled = true;
         break;
      }
   }
   if (!options.enabled) return options;

   for (int i = 1; i < __argc; ++i)
   {
      const char* argument = __argv[i];
      if (_stricmp(argument, "--batch") == 0)
      {
         continue;
      }
      if (_stricmp(argument, "--overwrite") == 0)
      {
         options.overwrite = true;
         continue;
      }

      CString* destination = nullptr;
      if (_stricmp(argument, "--engines") == 0) destination = &options.enginesPath;
      else if (_stricmp(argument, "--settings") == 0) destination = &options.tournamentPath;
      else if (_stricmp(argument, "--results") == 0) destination = &options.resultsPath;
      else if (_stricmp(argument, "--status") == 0) destination = &options.statusPath;
      else
      {
         options.error.Format("Unknown batch argument: %s", argument);
         return options;
      }

      if (++i >= __argc)
      {
         options.error.Format("Missing value after %s", argument);
         return options;
      }
      *destination = __argv[i];
   }

   if (options.enginesPath.IsEmpty()) options.error = "--engines is required in batch mode";
   else if (options.tournamentPath.IsEmpty()) options.error = "--settings is required in batch mode";
   else if (options.resultsPath.IsEmpty()) options.error = "--results is required in batch mode";
   else if (options.statusPath.IsEmpty()) options.statusPath = options.resultsPath + ".status.log";

   return options;
}
}

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

   const TBatchOptions batchOptions = ParseBatchOptions();
   m_bBatchMode = batchOptions.enabled;
   CLittleBlitzerDlg dlg(batchOptions);
   m_pMainWnd = &dlg;
   if (const INT_PTR nResponse = dlg.DoModal(); nResponse == IDOK)
   {
   }
   else if (nResponse == IDCANCEL)
   {
   }

   if (batchOptions.enabled)
      m_nExitCode = dlg.GetBatchExitCode();

   return FALSE;
}

int CLittleBlitzerApp::ExitInstance()
{
   return m_bBatchMode ? m_nExitCode : CWinAppEx::ExitInstance();
}
