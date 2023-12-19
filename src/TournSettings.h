#pragma once
#include <afxdialogex.h>

#include "afxwin.h"
#include "Board.h"

class CTournSettings final : public CDialogEx
{
   DECLARE_DYNAMIC(CTournSettings)

   explicit CTournSettings(CWnd* pParent = nullptr);
   ~CTournSettings() override;

   enum { IDD = IDD_TOURN_SETTINGS };

protected:
   void DoDataExchange(CDataExchange* pDX) override;

   DECLARE_MESSAGE_MAP()

public:
   long m_nRounds;
   long m_nParallel;
   long m_nHash;
   int m_nType;
   long m_nAdjMateScore;
   long m_nAdjMateMoves;
   long m_nAdjDrawMoves;
   long m_nTimeBase;
   long m_nTimeInc;
   int m_nPonder;
   int m_nOwnBook;
   char m_sStartingPosition[MAX_FEN_LEN];
   BOOL OnInitDialog() override;
   CEdit m_wndRounds;
   CEdit m_wndParallel;
   CEdit m_wndHash;
   CEdit m_wndAdjMateScore;
   CEdit m_wndMateMoves;
   CEdit m_wndDrawMoves;
   CEdit m_wndTimeBase;
   CEdit m_wndTimeInc;
   CButton m_wndPonder;
   CButton m_wndOwnBook;
   CEdit m_wndPosition;
   CButton m_wndRandomize;
   int m_nRandomize;
   int m_nVariant;
   int m_nPosition;
   afx_msg void OnBnClickedOpening();
   afx_msg void OnBnClickedFen();
   afx_msg void OnBnClickedEpd();
   afx_msg void OnBnClickedPgn();
   afx_msg void OnBnClickedOk();
   int m_nTC;
   afx_msg void OnBnClickedFixedtpm();
   CStatic m_wndTC1;
   CStatic m_wndTC2;
   afx_msg void OnBnClickedBlitz();
   afx_msg void OnBnClickedTourn();
   CStatic m_wndTC3;
   CStatic m_wndTC4;
   void UpdateTC();
};
