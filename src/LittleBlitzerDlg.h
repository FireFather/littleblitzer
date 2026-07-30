#pragma once

#include "Engine.h"
#include "Tournament.h"
#include "afxwin.h"
#include "Timer.h"
#include "Common.h"
#include "resource.h"

struct TBatchOptions
{
   bool enabled = false;
   bool overwrite = false;
   CString enginesPath;
   CString tournamentPath;
   CString resultsPath;
   CString statusPath;
   CString error;
};

class CClickThroughGroup final : public CButton
{
protected:
   afx_msg LRESULT OnNcHitTest(CPoint point);
   DECLARE_MESSAGE_MAP()
};

class CLittleBlitzerDlg final : public CDialog
{
public:
   explicit CLittleBlitzerDlg(const TBatchOptions& batchOptions = {}, CWnd* pParent = nullptr);
   ~CLittleBlitzerDlg() override;
   int GetBatchExitCode() const { return m_nBatchExitCode; }

   enum { IDD = IDD_LITTLEBLITZER_DIALOG };

protected:
   void DoDataExchange(CDataExchange* pDX) override;

   HICON m_hIcon;

   BOOL OnInitDialog() override;
   afx_msg void OnPaint();
   afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
   afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT drawItem);
   afx_msg void OnGetMinMaxInfo(MINMAXINFO* minMaxInfo);
   afx_msg HCURSOR OnQueryDragIcon();
   DECLARE_MESSAGE_MAP()

public:
   long m_nNumEngines;
   CEngine m_Engines[100];
   int m_nNumTournaments;
   volatile int m_nNumActiveTournaments;
   CTournament* m_Tournaments;
   int m_nStartPositionType;
   char m_sStartPositionFile[MAX_FEN_LEN];
   long m_nNumGames;
   tLock m_nLockGameNum;
   volatile long m_nGameNum;
   volatile long m_nNumGamesPlayed;
   volatile long* m_nResults[NUM_TYPES];
   volatile long* m_nWins;
   volatile long* m_nLosses;
   volatile long* m_nDraws;
   volatile long* m_nGames;
   volatile double* m_dTotalTime;
   volatile double* m_dTotalSearches;
   volatile double m_dTotalGamesLen;
   volatile long* m_nTotalDepth;
   volatile long* m_nTotalDepthCount;
   volatile long long* m_nTotalNPS;
   volatile long long* m_nTotalNPSCount;

   bool m_bPaused;

   CTimer m_nTimeTaken;
   CEdit m_wndResults;
   CStatic m_wndMatchSettings;
   CStatic m_wndMatchTime;
   CFont m_resultsFont;
   CBrush m_dialogBackgroundBrush;
   CBrush m_contentBackgroundBrush;
   CBrush m_inputBackgroundBrush;
   int m_fixedWindowWidth = 0;
   CClickThroughGroup m_wndRunGroup;
   CClickThroughGroup m_wndOptionsGroup;
   CClickThroughGroup m_wndMatchGroup;
   CClickThroughGroup m_wndResultsGroup;

   bool InitPGN();
   bool WriteRunManifest(unsigned long long openingSeed);
   bool LoadEngineSettings(bool interactive = true);
   bool LoadTournamentSettings(bool interactive = true);
   bool StartTournament();
   bool WriteBatchStatus(const char* format, ...) const;
   LRESULT OnBatchStart(WPARAM wParam, LPARAM lParam);
   void FinishBatchWithError(int exitCode, const CString& message);
   static UINT RunTournament(void* pParam);
   LRESULT OnGameDone(WPARAM wParam, LPARAM lParam);
   void UpdateResults();
   bool UpdatePGN(TResult* r);
   CButton m_wndPause;
   CEdit m_wndEngineFile;
   afx_msg void OnBnClickedLoadEngines();
   afx_msg void OnBnClickedLoadTournament();
   afx_msg void OnBnClickedOpenEngines();
   afx_msg void OnBnClickedOpenTourn();
   afx_msg void OnBnClickedStart();
   afx_msg void OnBnClickedPause();
   CEdit m_wndTournFile;
   CEdit m_wndSavePGN;
   CButton m_wndStart;
   afx_msg void OnBnClickedCopytext();
   CStatic m_wndNumThreads;
   afx_msg void OnBnClickedInc();
   afx_msg void OnBnClickedDec();
   void UpdateNumTourneys();
   CButton m_wndChkLog;
   afx_msg void OnBnClickedChkLog();
   CButton m_wndLoadEngines;
   CButton m_wndLoadTournament;
   void LoadEPDPositions(const char* sPath);
   void LoadPGNPositions(const char* sPath);
   int GetNextRound();
   afx_msg void OnBnClickedChkIllegal();
   afx_msg void OnBnClickedChkFullPGN();
   afx_msg void OnBnClickedDarkMode();
   CButton m_wndDumpIllegalMoves;
   CButton m_wndFullPGN;
   CButton m_wndDarkMode;
   static int ReadLine(FILE* f, char* s);

private:
   void ApplyVisualTheme(bool darkMode);
   TBatchOptions m_batchOptions;
   int m_nBatchExitCode;
   long m_nBatchIllegalGames;
   long m_nBatchEngineFailures;
   bool m_bOutputFailed;
   unsigned long long m_nConfiguredOpeningSeed;
};
