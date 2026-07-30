#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "LittleBlitzerDlg.h"

#define VERSION	"3.01 x64"

class CLittleBlitzerApp final : public CWinAppEx
{
public:
   CLittleBlitzerApp();

   BOOL InitInstance() override;
   int ExitInstance() override;

   DECLARE_MESSAGE_MAP()

private:
   bool m_bBatchMode = false;
   int m_nExitCode = 0;
};

extern CLittleBlitzerApp theApp;
