#pragma once

#ifndef __AFXWIN_H__
#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "LittleBlitzerDlg.h"

#define VERSION	"2.77 x64"

class CLittleBlitzerApp final : public CWinAppEx
{
public:
   CLittleBlitzerApp();

   BOOL InitInstance() override;

   DECLARE_MESSAGE_MAP()
};

extern CLittleBlitzerApp theApp;
