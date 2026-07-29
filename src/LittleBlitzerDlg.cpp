#include "stdafx.h"
#include "LittleBlitzer.h"
#include "LittleBlitzerDlg.h"
#include "Common.h"
#include "TournSettings.h"
#include "Board.h"
#include "Move.h"

#include <cstdarg>

#ifdef _MSC_VER
#pragma warning(disable : 4244)          
#pragma warning(disable : 4267)           
#else
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CString m_sResultsPath;

namespace
{
constexpr int MAX_ENGINES = 100;
constexpr char STANDARD_START_FEN[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

void FreeStartPositions(CTournament& tournament)
{
   for (int i = 0; i < tournament.m_nNumStartPositions; ++i)
      delete[] tournament.m_sStartPositions[i];
   delete[] tournament.m_sStartPositions;
   tournament.m_sStartPositions = nullptr;
   tournament.m_nNumStartPositions = 0;
}

void SetDefaultStartPosition(CTournament& tournament)
{
   FreeStartPositions(tournament);
   tournament.m_sStartPositions = new char* [1];
   tournament.m_sStartPositions[0] = new char[sizeof(STANDARD_START_FEN)];
   strcpy_s(tournament.m_sStartPositions[0], sizeof(STANDARD_START_FEN), STANDARD_START_FEN);
   tournament.m_nNumStartPositions = 1;
}
}

CLittleBlitzerDlg::CLittleBlitzerDlg(const TBatchOptions& batchOptions, CWnd* pParent)
   : CDialog(IDD, pParent), m_sStartPositionFile{}, m_bPaused(false), m_batchOptions(batchOptions),
     m_nBatchExitCode(0), m_nBatchIllegalGames(0)
{
   m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

   m_nGameNum = 0;
   m_nNumGamesPlayed = 0;
   m_nNumEngines = 0;
   m_nNumActiveTournaments = 0;
   m_nNumTournaments = 4;
   m_nNumGames = 50000;
   m_nStartPositionType = 0;

   for (auto& m_nResult : m_nResults)
      m_nResult = nullptr;
   m_nWins = nullptr;
   m_nLosses = nullptr;
   m_nDraws = nullptr;
   m_nGames = nullptr;
   m_dTotalTime = nullptr;
   m_dTotalSearches = nullptr;
   m_nTotalDepth = nullptr;
   m_nTotalDepthCount = nullptr;
   m_nTotalNPS = nullptr;
   m_nTotalNPSCount = nullptr;
   m_dTotalGamesLen = 0;

   m_Tournaments = new CTournament[MAX_THREADS];

   m_Tournaments[0].m_nNumStartPositions = 0;
   m_Tournaments[0].m_sStartPositions = nullptr;

   for (auto& m_Engine : m_Engines)
      m_Engine.m_sPath = nullptr;

   LockInit(&m_nLockGameNum, NULL);
}

CLittleBlitzerDlg::~CLittleBlitzerDlg()
{
   for (int i = 0; i < MAX_THREADS; i++)
      m_Tournaments[i].Abort();
   for (const auto& m_nResult : m_nResults)
      delete[] m_nResult;
   delete[] m_nWins;
   delete[] m_nLosses;
   delete[] m_nDraws;
   delete[] m_nGames;
   delete[] m_dTotalTime;
   delete[] m_dTotalSearches;
   delete[] m_nTotalDepth;
   delete[] m_nTotalDepthCount;
   delete[] m_nTotalNPS;
   delete[] m_nTotalNPSCount;

   FreeStartPositions(m_Tournaments[0]);
   delete[] m_Tournaments;
   LockFree(&m_nLockGameNum);

}

void CLittleBlitzerDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_STATUS, m_wndResults);
   DDX_Control(pDX, IDC_PAUSE, m_wndPause);
   DDX_Control(pDX, IDC_ENGINE_FILE, m_wndEngineFile);
   DDX_Control(pDX, IDC_TOURN_FILE, m_wndTournFile);
   DDX_Control(pDX, IDC_SAVE_PGN, m_wndSavePGN);
   DDX_Control(pDX, IDC_START, m_wndStart);
   DDX_Control(pDX, IDC_NUMTHREADS, m_wndNumThreads);
   DDX_Control(pDX, IDC_CHK_LOG, m_wndChkLog);
   DDX_Control(pDX, IDC_LOAD_ENGINES, m_wndLoadEngines);
   DDX_Control(pDX, IDC_TOURNAMENT, m_wndLoadTournament);
   DDX_Control(pDX, IDC_CHK_ILLEGAL, m_wndDumpIllegalMoves);
   DDX_Control(pDX, IDC_CHK_FULLPGN, m_wndFullPGN);
}

BEGIN_MESSAGE_MAP(CLittleBlitzerDlg, CDialog)
   ON_WM_PAINT()
   ON_WM_QUERYDRAGICON()
   ON_BN_CLICKED(IDC_TOURNAMENT, &CLittleBlitzerDlg::OnBnClickedLoadTournament)
   ON_BN_CLICKED(IDC_START, &CLittleBlitzerDlg::OnBnClickedStart)

   ON_MESSAGE(GAME_DONE, &CLittleBlitzerDlg::OnGameDone)
   ON_MESSAGE(BATCH_START, &CLittleBlitzerDlg::OnBatchStart)
   ON_BN_CLICKED(IDC_PAUSE, &CLittleBlitzerDlg::OnBnClickedPause)
   ON_BN_CLICKED(IDC_OPEN_ENGINES, &CLittleBlitzerDlg::OnBnClickedOpenEngines)
   ON_BN_CLICKED(IDC_LOAD_ENGINES, &CLittleBlitzerDlg::OnBnClickedLoadEngines)
   ON_BN_CLICKED(IDC_OPEN_TOURN, &CLittleBlitzerDlg::OnBnClickedOpenTourn)
   ON_BN_CLICKED(IDC_COPYTEXT, &CLittleBlitzerDlg::OnBnClickedCopytext)
   ON_BN_CLICKED(IDC_INC, &CLittleBlitzerDlg::OnBnClickedInc)
   ON_BN_CLICKED(IDC_DEC, &CLittleBlitzerDlg::OnBnClickedDec)
   ON_BN_CLICKED(IDC_CHK_LOG, &CLittleBlitzerDlg::OnBnClickedChkLog)
   ON_BN_CLICKED(IDC_CHK_ILLEGAL, &CLittleBlitzerDlg::OnBnClickedChkIllegal)
   ON_BN_CLICKED(IDC_CHK_FULLPGN, &CLittleBlitzerDlg::OnBnClickedChkFullPGN)
END_MESSAGE_MAP()

BOOL CLittleBlitzerDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   SetIcon(m_hIcon, TRUE);
   SetIcon(m_hIcon, FALSE);

   char sFile[1024];
   GetModuleFileName(nullptr, sFile, 1024);
   char* sPath = GetFilePath(sFile);
   char sConfigPath[1024];
   sprintf_s(sConfigPath, _T("%s\\Engines.lbe"), sPath);
   m_wndEngineFile.SetWindowText(sConfigPath);
   sprintf_s(sConfigPath, _T("%s\\Tournament.lbt"), sPath);
   m_wndTournFile.SetWindowText(sConfigPath);
   sprintf_s(sConfigPath, _T("%s\\Results.pgn"), sPath);
   m_wndSavePGN.SetWindowText(sConfigPath);
   delete[] sPath;

   m_bPaused = false;

   m_wndResults.SetWindowText(_T("No engines loaded"));
   m_wndDumpIllegalMoves.SetCheck(g_bDumpIllegalMoves);
   m_wndFullPGN.SetCheck(true);
   g_bFullPGN = true;

   UpdateNumTourneys();

   CString s;
   s.Format(_T("LittleBlitzer %s"), _T(VERSION));
   this->SetWindowText(s);

   InitialiseArrays();

   if (m_batchOptions.enabled)
   {
      ShowWindow(SW_HIDE);
      PostMessage(BATCH_START);
   }

   return TRUE;
}

void CLittleBlitzerDlg::OnPaint()
{
   if (IsIconic())
   {
      CPaintDC dc(this);

      SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

      const int cxIcon = GetSystemMetrics(SM_CXICON);
      const int cyIcon = GetSystemMetrics(SM_CYICON);
      CRect rect;
      GetClientRect(&rect);
      const int x = (rect.Width() - cxIcon + 1) / 2;
      const int y = (rect.Height() - cyIcon + 1) / 2;

      dc.DrawIcon(x, y, m_hIcon);
   }
   else
   {
      CDialog::OnPaint();
   }
}

HCURSOR CLittleBlitzerDlg::OnQueryDragIcon()
{
   return m_hIcon;
}

void CLittleBlitzerDlg::OnBnClickedLoadTournament()
{
   LoadTournamentSettings(true);
}

bool CLittleBlitzerDlg::LoadTournamentSettings(const bool interactive)
{
   CString sPathName;
   m_wndTournFile.GetWindowText(sPathName);

   FILE* f = fopen(sPathName.GetBuffer(), "rt");
   if (f)
   {
      m_nNumTournaments = 0;
      char line[2048];
      while (fgets(line, sizeof(line), f))
      {
         char* separator = strchr(line, ':');
         if (!separator) continue;
         *separator = 0;
         CString key(line);
         CString value(separator + 1);
         key.Trim();
         value.Trim();

         const long number = strtol(value, nullptr, 10);
         if (!key.CompareNoCase("Type")) m_Tournaments[0].m_nType = number;
         else if (!key.CompareNoCase("TC")) m_Tournaments[0].m_nTC = number;
         else if (!key.CompareNoCase("Base")) m_Tournaments[0].m_nBase = MAX(1, number);
         else if (!key.CompareNoCase("Inc")) m_Tournaments[0].m_nInc = MAX(0, number);
         else if (!key.CompareNoCase("Rounds")) m_nNumGames = MAX(1, number);
         else if (!key.CompareNoCase("Hash")) m_Tournaments[0].m_nHash = MAX(1, number);
         else if (!key.CompareNoCase("Ponder")) m_Tournaments[0].m_bPonder = number != 0;
         else if (!key.CompareNoCase("OwnBook")) m_Tournaments[0].m_bOwnBook = number != 0;
         else if (!key.CompareNoCase("Variant")) m_Tournaments[0].m_nVariant = number == VARIANT_960 ? VARIANT_960 : VARIANT_STD;
         else if (!key.CompareNoCase("NumParallel")) m_nNumTournaments = MIN(MAX_THREADS, MAX(1, number));
         else if (!key.CompareNoCase("AdjudicateMateScore")) m_Tournaments[0].m_nAdjMateScore = MAX(1, labs(number));
         else if (!key.CompareNoCase("AdjudicateMateMoves")) m_Tournaments[0].m_nAdjMateMoves = MIN(1000, MAX(1, number));
         else if (!key.CompareNoCase("AdjudicateDrawMoves")) m_Tournaments[0].m_nAdjDrawMoves = MAX(1, number);
         else if (!key.CompareNoCase("Randomize")) m_Tournaments[0].m_nRandomize = number != 0;
         else if (!key.CompareNoCase("Position"))
         {
            if (!value.Left(4).CompareNoCase("FEN:"))
            {
               m_nStartPositionType = 1;
               m_sStartPositionFile[0] = 0;
               FreeStartPositions(m_Tournaments[0]);
               CString fen = value.Mid(4);
               m_Tournaments[0].m_sStartPositions = new char* [1];
               m_Tournaments[0].m_sStartPositions[0] = new char[fen.GetLength() + 1];
               strcpy_s(m_Tournaments[0].m_sStartPositions[0], fen.GetLength() + 1, fen);
               m_Tournaments[0].m_nNumStartPositions = 1;
            }
            else if (!value.Left(4).CompareNoCase("EPD:"))
            {
               m_nStartPositionType = 2;
               strncpy_s(m_sStartPositionFile, value.Mid(4), _TRUNCATE);
            }
            else if (!value.Left(4).CompareNoCase("PGN:"))
            {
               m_nStartPositionType = 3;
               strncpy_s(m_sStartPositionFile, value.Mid(4), _TRUNCATE);
            }
            else
            {
               m_nStartPositionType = 0;
               SetDefaultStartPosition(m_Tournaments[0]);
            }
         }
      }
      fclose(f);

      for (int i = 1; i < MAX_THREADS; i++)
      {
         m_Tournaments[i] = m_Tournaments[0];
      }
   }
   else
   {
      if (interactive)
         MessageBox(_T("Tournament file not found, will be created using defaults"), _T("Warning"));
      return false;
   }

   if (!interactive)
   {
      if (m_nStartPositionType == 2)
      {
         if (GetFileAttributes(m_sStartPositionFile) == INVALID_FILE_ATTRIBUTES)
            return false;
         LoadEPDPositions(m_sStartPositionFile);
      }
      else if (m_nStartPositionType == 3)
      {
         if (GetFileAttributes(m_sStartPositionFile) == INVALID_FILE_ATTRIBUTES)
            return false;
         LoadPGNPositions(m_sStartPositionFile);
      }
      else if (m_nStartPositionType == 0)
      {
         SetDefaultStartPosition(m_Tournaments[0]);
      }

      for (int i = 1; i < MAX_THREADS; i++)
         m_Tournaments[i] = m_Tournaments[0];

      UpdateNumTourneys();
      UpdateResults();
      return m_nNumGames > 0 && m_nNumTournaments > 0 && m_Tournaments[0].m_nNumStartPositions > 0;
   }

   CTournSettings dlg;
   dlg.m_nRounds = m_nNumGames;
   dlg.m_nParallel = m_nNumTournaments;
   dlg.m_nHash = m_Tournaments[0].m_nHash;
   dlg.m_nType = m_Tournaments[0].m_nType;
   dlg.m_nVariant = m_Tournaments[0].m_nVariant;
   dlg.m_nPosition = m_nStartPositionType;
   dlg.m_nAdjMateScore = m_Tournaments[0].m_nAdjMateScore;
   dlg.m_nAdjMateMoves = m_Tournaments[0].m_nAdjMateMoves;
   dlg.m_nAdjDrawMoves = m_Tournaments[0].m_nAdjDrawMoves;
   dlg.m_nTimeBase = m_Tournaments[0].m_nBase;
   dlg.m_nTimeInc = m_Tournaments[0].m_nInc;
   dlg.m_nPonder = m_Tournaments[0].m_bPonder;
   dlg.m_nOwnBook = m_Tournaments[0].m_bOwnBook;
   dlg.m_nTC = m_Tournaments[0].m_nTC;
   if (m_nStartPositionType == 0)
   {
      dlg.m_sStartingPosition[0] = 0;
   }
   else if (m_nStartPositionType == 1)
   {
      strcpy_s(dlg.m_sStartingPosition, m_Tournaments[0].m_sStartPositions[0]);
   }
   else
   {
      strcpy_s(dlg.m_sStartingPosition, m_sStartPositionFile);
   }
   dlg.m_nRandomize = m_Tournaments[0].m_nRandomize;

   if (const INT_PTR res = dlg.DoModal(); res != IDOK) return false;

   SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

   m_nNumGames = dlg.m_nRounds;
   m_nNumTournaments = dlg.m_nParallel;
   m_Tournaments[0].m_nHash = dlg.m_nHash;
   m_Tournaments[0].m_nType = dlg.m_nType;
   m_Tournaments[0].m_nVariant = dlg.m_nVariant;
   m_Tournaments[0].m_nAdjMateScore = dlg.m_nAdjMateScore;
   m_Tournaments[0].m_nAdjMateMoves = dlg.m_nAdjMateMoves;
   m_Tournaments[0].m_nAdjDrawMoves = dlg.m_nAdjDrawMoves;
   m_Tournaments[0].m_nTC = dlg.m_nTC;
   m_Tournaments[0].m_nBase = dlg.m_nTimeBase;
   m_Tournaments[0].m_nInc = dlg.m_nTimeInc;
   m_Tournaments[0].m_bPonder = dlg.m_nPonder;
   m_Tournaments[0].m_bOwnBook = dlg.m_nOwnBook;
   m_nStartPositionType = dlg.m_nPosition;
   char sFEN[MAX_FEN_LEN];
   strcpy_s(sFEN, dlg.m_sStartingPosition);
   if (m_nStartPositionType == 1)
   {
      m_nStartPositionType = 1;
      m_sStartPositionFile[0] = 0;
      FreeStartPositions(m_Tournaments[0]);
      m_Tournaments[0].m_nNumStartPositions = 1;
      m_Tournaments[0].m_sStartPositions = new char* [1];
      m_Tournaments[0].m_sStartPositions[0] = new char[strlen(sFEN) + 1];
      strcpy_s(m_Tournaments[0].m_sStartPositions[0], strlen(sFEN) + 1, sFEN);
   }
   else if (m_nStartPositionType == 2)
   {
      m_nStartPositionType = 2;
      strcpy_s(m_sStartPositionFile, sFEN);
      LoadEPDPositions(sFEN);
   }
   else if (m_nStartPositionType == 3)
   {
      m_nStartPositionType = 3;
      strcpy_s(m_sStartPositionFile, sFEN);
      LoadPGNPositions(sFEN);
   }
   else
   {
      m_nStartPositionType = 0;
      SetDefaultStartPosition(m_Tournaments[0]);
   }
   m_Tournaments[0].m_nRandomize = dlg.m_nRandomize;

   for (int i = 1; i < MAX_THREADS; i++)
   {
      m_Tournaments[i] = m_Tournaments[0];
   }
   m_wndTournFile.GetWindowText(sPathName);

   f = fopen(sPathName.GetBuffer(), "wt");
   if (!f)
   {
      MessageBox(_T("File not found!"), _T("Error"));
      return false;
   }

   fprintf(f, "Type: %d\n", m_Tournaments[0].m_nType);
   fprintf(f, "TC: %d\n", m_Tournaments[0].m_nTC);
   fprintf(f, "Base: %ld\n", m_Tournaments[0].m_nBase);
   fprintf(f, "Inc: %ld\n", m_Tournaments[0].m_nInc);
   fprintf(f, "Rounds: %ld\n", m_nNumGames);
   fprintf(f, "Ponder: %ld\n", m_Tournaments[0].m_bPonder);
   fprintf(f, "OwnBook: %ld\n", m_Tournaments[0].m_bOwnBook);
   fprintf(f, "Hash: %ld\n", m_Tournaments[0].m_nHash);
   fprintf(f, "NumParallel: %d\n", m_nNumTournaments);
   fprintf(f, "Variant: %d\n", m_Tournaments[0].m_nVariant);
   char buf[MAX_FEN_LEN + 5];
   if (m_nStartPositionType == 0) sprintf_s(buf, "OPENING");
   else if (m_nStartPositionType == 1) sprintf_s(buf, "FEN:%s", sFEN);
   else if (m_nStartPositionType == 2) sprintf_s(buf, "EPD:%s", sFEN);
   else if (m_nStartPositionType == 3) sprintf_s(buf, "PGN:%s", sFEN);
   fprintf(f, "Position: %s\n", buf);
   fprintf(f, "Randomize: %d\n", m_Tournaments[0].m_nRandomize);
   fprintf(f, "AdjudicateMateScore: %ld\n", m_Tournaments[0].m_nAdjMateScore);
   fprintf(f, "AdjudicateMateMoves: %ld\n", m_Tournaments[0].m_nAdjMateMoves);
   fprintf(f, "AdjudicateDrawMoves: %ld\n", m_Tournaments[0].m_nAdjDrawMoves);

   fclose(f);

   UpdateNumTourneys();
   UpdateResults();

   SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
   return true;
}

void CLittleBlitzerDlg::LoadEPDPositions(const char* sPath)
{
   FreeStartPositions(m_Tournaments[0]);

   m_Tournaments[0].m_sStartPositions = new char* [MAX_START_POSITIONS];
   if (FILE* fepd = fopen(sPath, "rt"))
   {
      do
      {
         char sFEN[MAX_FEN_LEN];
         int i = 0;
         int c = fgetc(fepd);
         if (c == EOF) break;
         while (c != '\n' && c != '\r')
         {
            if (c == ';')
            {
               while (c != '\n' && c != '\r' && c != EOF) c = fgetc(fepd);
            }
            else
            {
               sFEN[i++] = c;
               c = fgetc(fepd);
            }
            if (c == EOF) break;
            if (i == MAX_FEN_LEN - 1) break;
         }
         sFEN[i] = 0;
         if (i > 0)
         {
            m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions] = new char[strlen(sFEN) + 1];
            strcpy(m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions++], sFEN);
         }
         if (c == EOF) break;
         if (m_Tournaments[0].m_nNumStartPositions == MAX_START_POSITIONS) break;
      } while (true);
      fclose(fepd);
   }
   else
   {
      SetDefaultStartPosition(m_Tournaments[0]);
   }
   if (m_Tournaments[0].m_nNumStartPositions == 0)
      SetDefaultStartPosition(m_Tournaments[0]);
}

int CLittleBlitzerDlg::ReadLine(FILE* f, char* s)
{
   int c = fgetc(f);
   int i = 0;
   while (c != '\n' && c != '\r')
   {
      if (c == ';')
      {
         while (c != '\n' && c != '\r' && c != EOF) c = fgetc(f);
      }
      else
      {
         s[i++] = c;
         c = fgetc(f);
      }
      if (c == EOF) break;
      if (i == MAX_FEN_LEN - 1) break;
   }
   s[i] = 0;
   return c;
}

void CLittleBlitzerDlg::LoadPGNPositions(const char* sPath)
{
   FreeStartPositions(m_Tournaments[0]);

   m_Tournaments[0].m_sStartPositions = new char* [MAX_START_POSITIONS];
   if (FILE* fepd = fopen(sPath, "rt"))
   {
      do
      {
         char sLine[MAX_FEN_LEN];
         int c = ReadLine(fepd, sLine);
         if (c == EOF) break;
         const size_t lineLength = strlen(sLine);
         if (lineLength >= 8 && !_strnicmp(sLine, "[FEN ", 5))
         {
            char sFEN[MAX_FEN_LEN];
            const size_t fenLength = lineLength - 8;
            strncpy_s(sFEN, sLine + 6, fenLength);
            sFEN[fenLength] = 0;
            m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions] = new char[strlen(sFEN) + 1];
            strcpy(m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions++], sFEN);
         }
         else if (sLine[0] == '1' && sLine[1] == '.')
         {
            CString sGame;
            do
            {
               if (!sGame.IsEmpty()) sGame.AppendChar(' ');
               sGame.Append(sLine);
               c = ReadLine(fepd, sLine);
               if (c == EOF) break;
               if (sLine[0] == 0) break;
               if (sLine[0] == '[') break;
            } while (true);
            char sFEN[MAX_FEN_LEN];
            GameMoves2FEN(sGame.GetBuffer(), sFEN);
            m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions] = new char[strlen(sFEN) + 1];
            strcpy(m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions++], sFEN);
         }
         if (c == EOF) break;
         if (m_Tournaments[0].m_nNumStartPositions == MAX_START_POSITIONS) break;
      } while (true);
      fclose(fepd);
   }
   else
   {
      SetDefaultStartPosition(m_Tournaments[0]);
   }
   if (m_Tournaments[0].m_nNumStartPositions == 0)
      SetDefaultStartPosition(m_Tournaments[0]);
}

bool CLittleBlitzerDlg::LoadEngineSettings(const bool interactive)
{
   CString sConfigPath;
   m_wndEngineFile.GetWindowText(sConfigPath);

   FILE* f = fopen(sConfigPath.GetBuffer(), "rt");
   if (!f)
   {
      m_nNumEngines = 0;
      if (interactive)
         MessageBox(_T("File not found!"), _T("Error"));
      return false;
   }

   SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

   m_wndResults.SetSel(0, -1);
   m_wndResults.ReplaceSel(_T("Loading engines..."));

   constexpr int nMaxLen = 1024;
   char sEnginePath[nMaxLen];

   m_nNumEngines = 0;
   char sLine[nMaxLen];
   while (fgets(sLine, sizeof(sLine), f))
   {
      sEnginePath[0] = 0;
      sLine[strcspn(sLine, "\r\n")] = 0;
      CString sUpperLine = sLine;
      sUpperLine.MakeUpper();

      if (sUpperLine.Find("ENGINE=") == 0)
      {
         if (m_nNumEngines >= MAX_ENGINES)
         {
            if (interactive)
               MessageBox(_T("Only the first 100 engines were loaded."), _T("Engine limit"), MB_OK | MB_ICONWARNING);
            break;
         }
         CEngine e;
         strcpy_s(sEnginePath, sLine + 7);
         e.m_sPath = new char[strlen(sEnginePath) + 1];
         strcpy_s(e.m_sPath, strlen(sEnginePath) + 1, sEnginePath);
         m_Engines[m_nNumEngines++] = e;
         m_Engines[m_nNumEngines - 1].m_sParameterNames = new char* [MAX_PARMS];
         m_Engines[m_nNumEngines - 1].m_sParameterValues = new char* [MAX_PARMS];
      }
      else if (sUpperLine.Find('=') != -1 && m_nNumEngines > 0)
      {
         if (m_Engines[m_nNumEngines - 1].m_nNumParameters < MAX_PARMS)
         {
            const int pos = sUpperLine.Find('=');
            CString parameterName = CString(sLine).Left(pos);
            CString parameterValue = CString(sLine).Mid(pos + 1);
            parameterName.Trim();
            parameterValue.Trim();
            if (!parameterName.CompareNoCase("LB_Name"))
            {
               delete[] m_Engines[m_nNumEngines - 1].m_sLBName;
               m_Engines[m_nNumEngines - 1].m_sLBName = new char[parameterValue.GetLength() + 1];
               strcpy_s(m_Engines[m_nNumEngines - 1].m_sLBName, parameterValue.GetLength() + 1, parameterValue);
            }
            else
            {
               CEngine& engine = m_Engines[m_nNumEngines - 1];
               const int index = engine.m_nNumParameters++;
               engine.m_sParameterNames[index] = new char[parameterName.GetLength() + 1];
               engine.m_sParameterValues[index] = new char[parameterValue.GetLength() + 1];
               strcpy_s(engine.m_sParameterNames[index], parameterName.GetLength() + 1, parameterName);
               strcpy_s(engine.m_sParameterValues[index], parameterValue.GetLength() + 1, parameterValue);
            }
         }
      }
   }
   fclose(f);

   for (int i = 0; i < m_nNumEngines; i++)
   {
      if (!m_Engines[i].Init())
      {
         m_nNumEngines = i;
         if (interactive)
            MessageBox(_T("Engine loading stopped because an engine failed to initialize."), _T("Engine error"),
               MB_OK | MB_ICONERROR);
         break;
      }
      m_Engines[i].Quit();
   }

   SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
   return m_nNumEngines >= 2;
}

void CLittleBlitzerDlg::OnBnClickedStart()
{
   StartTournament();
}

bool CLittleBlitzerDlg::StartTournament()
{
   if (m_nNumEngines < 2)
   {
      if (!m_batchOptions.enabled)
         MessageBox(_T("Load at least two valid engines before starting."), _T("Cannot start"), MB_OK | MB_ICONWARNING);
      return false;
   }
   if (m_nNumGames <= 0 || m_nNumTournaments <= 0 || m_Tournaments[0].m_nNumStartPositions <= 0)
   {
      if (!m_batchOptions.enabled)
         MessageBox(_T("Tournament settings must specify at least one game, worker, and starting position."), _T("Cannot start"),
            MB_OK | MB_ICONWARNING);
      return false;
   }
   if (!InitPGN()) return false;

   m_nGameNum = 0;
   m_nNumGamesPlayed = 0;

   m_nNumActiveTournaments = 0;
   for (auto& m_nResult : m_nResults)
   {
      delete[] m_nResult;
      m_nResult = new long[m_nNumEngines];
      memset(const_cast<long*>(m_nResult), 0, sizeof(long) * m_nNumEngines);
   }
   delete[] m_nGames;
   m_nGames = new long[m_nNumEngines];
   memset(const_cast<long*>(m_nGames), 0, sizeof(long) * m_nNumEngines);
   delete[] m_nWins;
   m_nWins = new long[m_nNumEngines];
   memset(const_cast<long*>(m_nWins), 0, sizeof(long) * m_nNumEngines);
   delete[] m_nLosses;
   m_nLosses = new long[m_nNumEngines];
   memset(const_cast<long*>(m_nLosses), 0, sizeof(long) * m_nNumEngines);
   delete[] m_nDraws;
   m_nDraws = new long[m_nNumEngines];
   memset(const_cast<long*>(m_nDraws), 0, sizeof(long) * m_nNumEngines);
   delete[] m_dTotalTime;
   m_dTotalTime = new double[m_nNumEngines];
   memset(const_cast<double*>(m_dTotalTime), 0, sizeof(double) * m_nNumEngines);
   delete[] m_dTotalSearches;
   m_dTotalSearches = new double[m_nNumEngines];
   memset(const_cast<double*>(m_dTotalSearches), 0, sizeof(double) * m_nNumEngines);
   delete[] m_nTotalDepth;
   m_nTotalDepth = new long[m_nNumEngines];
   memset(const_cast<long*>(m_nTotalDepth), 0, sizeof(long) * m_nNumEngines);
   delete[] m_nTotalDepthCount;
   m_nTotalDepthCount = new long[m_nNumEngines];
   memset(const_cast<long*>(m_nTotalDepthCount), 0, sizeof(long) * m_nNumEngines);
   delete[] m_nTotalNPS;
   m_nTotalNPS = new long long[m_nNumEngines];
   memset(const_cast<long long*>(m_nTotalNPS), 0, sizeof(long long) * m_nNumEngines);
   delete[] m_nTotalNPSCount;
   m_nTotalNPSCount = new long long[m_nNumEngines];
   memset(const_cast<long long*>(m_nTotalNPSCount), 0, sizeof(long long) * m_nNumEngines);
   m_dTotalGamesLen = 0;

   m_nTimeTaken.Start();

   std::random_device randomDevice;
   const unsigned long long openingSeed =
      static_cast<unsigned long long>(randomDevice()) << 32 | randomDevice();

   for (int x = 0; x < MAX_THREADS; x++)
   {
      m_Tournaments[x].m_nNumEngines = m_nNumEngines;
      for (int e = 0; e < m_nNumEngines; e++)
      {
         m_Tournaments[x].m_Engines[e] = m_Engines[e];
      }
      m_Tournaments[x].m_pWnd = this;
      m_Tournaments[x].m_nThreadID = x;
      m_Tournaments[x].m_nOpeningSeed = openingSeed;
      m_Tournaments[x].m_nRound = 0;
      if (x < m_nNumTournaments && m_nGameNum < m_nNumGames)
      {
         m_nNumActiveTournaments++;
         m_Tournaments[x].m_nRound = GetNextRound();
         CWinThread* pThread = AfxBeginThread(RunTournament, &m_Tournaments[x]);
      }
   }
   m_wndStart.EnableWindow(false);
   m_wndLoadEngines.EnableWindow(false);
   m_wndLoadTournament.EnableWindow(false);

   UpdateResults();
   UpdateNumTourneys();
   return true;
}

int CLittleBlitzerDlg::GetNextRound()
{
   Lock(&m_nLockGameNum);
   m_nGameNum++;
   Unlock(&m_nLockGameNum);
   return m_nGameNum - 1;
}

UINT CLittleBlitzerDlg::RunTournament(void* pParam)
{
   const auto t = static_cast<CTournament*>(pParam);
   t->Start();
   return 0;
}

LRESULT CLittleBlitzerDlg::OnGameDone(const WPARAM wParam, const LPARAM lParam)
{
   m_nNumGamesPlayed++;

   const auto tResult = reinterpret_cast<TResult*>(wParam);

   ASSERT(tResult->nWhite >= 0 && tResult->nWhite < m_nNumEngines);
   ASSERT(tResult->nBlack >= 0 && tResult->nBlack < m_nNumEngines);
   ASSERT(tResult->nWhite != tResult->nBlack);

   m_nGames[tResult->nWhite]++;
   m_nGames[tResult->nBlack]++;
   if (tResult->nResult == BLACK_MATES)
   {
      m_nWins[tResult->nBlack]++;
      m_nLosses[tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
   }
   else if (tResult->nResult == WHITE_MATES)
   {
      m_nWins[tResult->nWhite]++;
      m_nLosses[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
   }
   else if (tResult->nResult == BLACK_TIMEOUT)
   {
      m_nWins[tResult->nWhite]++;
      m_nLosses[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
   }
   else if (tResult->nResult == WHITE_TIMEOUT)
   {
      m_nWins[tResult->nBlack]++;
      m_nLosses[tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
   }
   else if (tResult->nResult == STALEMATE)
   {
      m_nDraws[tResult->nWhite]++;
      m_nDraws[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
   }
   else if (tResult->nResult == INSUF_MAT)
   {
      m_nDraws[tResult->nWhite]++;
      m_nDraws[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
   }
   else if (tResult->nResult == REPETITION)
   {
      m_nDraws[tResult->nWhite]++;
      m_nDraws[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
   }
   else if (tResult->nResult == FIFTY_MOVE)
   {
      m_nDraws[tResult->nWhite]++;
      m_nDraws[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
   }
   else if (tResult->nResult == WHITE_ILLEGAL)
   {
      m_nWins[tResult->nBlack]++;
      m_nLosses[tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
   }
   else if (tResult->nResult == BLACK_ILLEGAL)
   {
      m_nWins[tResult->nWhite]++;
      m_nLosses[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
   }
   else if (tResult->nResult == ADJ_DRAW)
   {
      m_nDraws[tResult->nWhite]++;
      m_nDraws[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
   }
   else if (tResult->nResult == ADJ_WHITE_MATES)
   {
      m_nWins[tResult->nWhite]++;
      m_nLosses[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
   }
   else if (tResult->nResult == ADJ_BLACK_MATES)
   {
      m_nWins[tResult->nBlack]++;
      m_nLosses[tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
   }
   m_dTotalTime[tResult->nWhite] += tResult->dTotalTime[WHITE];
   m_dTotalTime[tResult->nBlack] += tResult->dTotalTime[BLACK];
   m_dTotalGamesLen += tResult->dTotalTime[WHITE] + tResult->dTotalTime[BLACK];
   m_dTotalSearches[tResult->nWhite] += tResult->dTotalSearches[WHITE];
   m_dTotalSearches[tResult->nBlack] += tResult->dTotalSearches[BLACK];
   m_nTotalDepth[tResult->nWhite] += tResult->nTotalDepth[WHITE];
   m_nTotalDepth[tResult->nBlack] += tResult->nTotalDepth[BLACK];
   m_nTotalDepthCount[tResult->nWhite] += tResult->nTotalDepthCount[WHITE];
   m_nTotalDepthCount[tResult->nBlack] += tResult->nTotalDepthCount[BLACK];
   m_nTotalNPS[tResult->nWhite] += tResult->nTotalNPS[WHITE];
   m_nTotalNPS[tResult->nBlack] += tResult->nTotalNPS[BLACK];
   m_nTotalNPSCount[tResult->nWhite] += tResult->nTotalNPSCount[WHITE];
   m_nTotalNPSCount[tResult->nBlack] += tResult->nTotalNPSCount[BLACK];

   UpdateResults();
   UpdatePGN(tResult);
   if (tResult->nResult == WHITE_ILLEGAL || tResult->nResult == BLACK_ILLEGAL)
      m_nBatchIllegalGames++;

   m_nNumActiveTournaments--;
   if (const int id = static_cast<int>(lParam); id < m_nNumTournaments && m_nNumGamesPlayed + m_nNumActiveTournaments < m_nNumGames)
   {
      m_nNumActiveTournaments++;
      m_Tournaments[id].m_bRunning = true;
      m_Tournaments[id].m_nRound = GetNextRound();
      CWinThread* pThread = AfxBeginThread(RunTournament, &m_Tournaments[id]);
   }
   UpdateNumTourneys();

   if (m_batchOptions.enabled)
      WriteBatchStatus("PROGRESS completed=%ld total=%ld active=%d illegal=%ld", m_nNumGamesPlayed, m_nNumGames,
         m_nNumActiveTournaments, m_nBatchIllegalGames);

   if (m_nNumActiveTournaments == 0 && m_nNumGamesPlayed >= m_nNumGames)
   {
      m_wndStart.EnableWindow(true);
      m_wndLoadEngines.EnableWindow(true);
      m_wndLoadTournament.EnableWindow(true);
      m_bPaused = false;
      m_wndPause.SetWindowText("Pause\n(zero threads)");
      if (m_batchOptions.enabled)
      {
         m_nBatchExitCode = m_nBatchIllegalGames == 0 ? 0 : 3;
         WriteBatchStatus("COMPLETE completed=%ld total=%ld illegal=%ld exit=%d", m_nNumGamesPlayed, m_nNumGames,
            m_nBatchIllegalGames, m_nBatchExitCode);
         EndDialog(IDOK);
      }
   }

   return 0;
}

bool CLittleBlitzerDlg::InitPGN()
{
   m_wndSavePGN.GetWindowText(m_sResultsPath);

   if (FILE* fpgn; fpgn = fopen(m_sResultsPath.GetBuffer(), "rt"))
   {
      fclose(fpgn);
      if (m_batchOptions.enabled)
      {
         if (!m_batchOptions.overwrite)
         {
            WriteBatchStatus("ERROR results file already exists (use --overwrite): %s",
               static_cast<const char*>(m_sResultsPath));
            return false;
         }
         if (!(fpgn = fopen(m_sResultsPath.GetBuffer(), "wt")))
            return false;
         fclose(fpgn);
         return true;
      }
      if (const int r = MessageBox(
         "The results file already exists, do you wish to append to the current file?\nYes = Append\nNo = Overwrite",
         "WARNING", MB_YESNOCANCEL); r == 6)
      {
      }
      else if (r == 7)
      {
         if (!(fpgn = fopen(m_sResultsPath.GetBuffer(), "wt")))
            return false;
         fclose(fpgn);
      }
      else if (r == 2)
      {
         return false;
      }
   }
   else
   {
      if (!(fpgn = fopen(m_sResultsPath.GetBuffer(), "wt")))
         return false;
      fclose(fpgn);
   }

   return true;
}

void CLittleBlitzerDlg::UpdatePGN(TResult* r)
{
   char sResult[20] = "*";
   const char* sTermination = "unterminated";
   if (r->nResult == BLACK_MATES)
   {
      strcpy(sResult, "0-1");
      sTermination = "normal";
   }
   else if (r->nResult == WHITE_MATES)
   {
      strcpy(sResult, "1-0");
      sTermination = "normal";
   }
   else if (r->nResult == BLACK_TIMEOUT)
   {
      strcpy(sResult, "1-0");
      sTermination = "time forfeit";
   }
   else if (r->nResult == WHITE_TIMEOUT)
   {
      strcpy(sResult, "0-1");
      sTermination = "time forfeit";
   }
   else if (r->nResult == STALEMATE)
   {
      strcpy(sResult, "1/2-1/2");
      sTermination = "normal";
   }
   else if (r->nResult == INSUF_MAT)
   {
      strcpy(sResult, "1/2-1/2");
      sTermination = "normal";
   }
   else if (r->nResult == REPETITION)
   {
      strcpy(sResult, "1/2-1/2");
      sTermination = "normal";
   }
   else if (r->nResult == FIFTY_MOVE)
   {
      strcpy(sResult, "1/2-1/2");
      sTermination = "normal";
   }
   else if (r->nResult == WHITE_ILLEGAL)
   {
      strcpy(sResult, "0-1");
      sTermination = "rules infraction";
   }
   else if (r->nResult == BLACK_ILLEGAL)
   {
      strcpy(sResult, "1-0");
      sTermination = "rules infraction";
   }
   else if (r->nResult == ADJ_DRAW)
   {
      strcpy(sResult, "1/2-1/2");
      sTermination = "adjudication";
   }
   else if (r->nResult == ADJ_WHITE_MATES)
   {
      strcpy(sResult, "1-0");
      sTermination = "adjudication";
   }
   else if (r->nResult == ADJ_BLACK_MATES)
   {
      strcpy(sResult, "0-1");
      sTermination = "adjudication";
   }
   FILE* fpgn;

   if (!(fpgn = fopen(m_sResultsPath.GetBuffer(), "at")))
   {
      delete[] r->sSAN;
      return;
   }

   fprintf(fpgn, "[White \"%s\"]\n", m_Engines[r->nWhite].m_sName);
   fprintf(fpgn, "[Black \"%s\"]\n", m_Engines[r->nBlack].m_sName);
   fprintf(fpgn, "[Result \"%s\"]\n", sResult);
   fprintf(fpgn, "[Termination \"%s\"]\n", sTermination);

   if (g_bFullPGN)
   {
      fprintf(fpgn, "[SetUp \"1\"]\n");
      fprintf(fpgn, "[FEN \"%s\"]\n", r->sFEN);
      fprintf(fpgn, "\n%s %s\n\n", r->sSAN, sResult);
      delete[] r->sSAN;
   }
   else
   {
      fprintf(fpgn, "\n%s\n\n", sResult);
      delete[] r->sSAN;
   }

   fclose(fpgn);
}

void CLittleBlitzerDlg::UpdateResults()
{
   const double dElapsed = m_nTimeTaken.GetMS();
   const double dRemaining = dElapsed / (static_cast<double>(m_nNumGamesPlayed) / (m_nNumGames ? m_nNumGames : 0.00001)) -
      dElapsed;
   CString s;
   s.Format("Games Completed = %ld of %ld (Avg game length = %.3lf sec)\r\n", m_nNumGamesPlayed, m_nNumGames,
      m_dTotalGamesLen / (m_nNumGamesPlayed ? m_nNumGamesPlayed : 1) / 1000.0);

   CString sVariant, sBook, sTC, sStart, sAdj;
   sVariant.Format("%s", m_Tournaments[0].m_nVariant == VARIANT_960 ? "FRC/" : "");
   sBook.Format("%s", m_Tournaments[0].m_bOwnBook ? "Book/" : "");
   if (m_Tournaments[0].m_nTC == TC_FIXED_TPM)
   {
      sTC.Format("%dms per move", m_Tournaments[0].m_nBase);
   }
   else if (m_Tournaments[0].m_nTC == TC_BLITZ)
   {
      sTC.Format("%dms+%dms", m_Tournaments[0].m_nBase, m_Tournaments[0].m_nInc);
   }
   else if (m_Tournaments[0].m_nTC == TC_TOURNAMENT)
   {
      sTC.Format("%dms in %d moves", m_Tournaments[0].m_nBase, m_Tournaments[0].m_nInc);
   }
   if (m_nStartPositionType == 1)
   {
      sStart.Format("FEN:%s", m_Tournaments[0].m_sStartPositions[0]);
   }
   else if (m_nStartPositionType == 2)
   {
      sStart.Format("EPD:%s(%d)", m_sStartPositionFile, m_Tournaments[0].m_nNumStartPositions);
   }
   else if (m_nStartPositionType == 3)
   {
      sStart.Format("PGN:%s(%d)", m_sStartPositionFile, m_Tournaments[0].m_nNumStartPositions);
   }
   sAdj.Format("M %dcp for %d moves, D %d moves", m_Tournaments[0].m_nAdjMateScore, m_Tournaments[0].m_nAdjMateMoves,
      m_Tournaments[0].m_nAdjDrawMoves);
   s.AppendFormat("Settings = %s/%s%dMB/%s%s/%s/%s\r\n", m_Tournaments[0].m_nType == 0 ? "Gauntlet" : "RR", sVariant,
      m_Tournaments[0].m_nHash, sBook, sTC, sAdj, sStart);

   s.AppendFormat(_T("Time = %0.0lf sec elapsed, %0.0lf sec remaining\r\n"), dElapsed / 1000, dRemaining / 1000);

   for (int i = 0; i < m_nNumEngines; i++)
   {
      s.AppendFormat(_T("%2d.  %-25s"), i + 1, CString(m_Engines[i].m_sName).GetBuffer());
      if (m_nWins)
      {
         const int nWins = m_nWins ? m_nWins[i] : 0;
         const int nDraws = m_nDraws ? m_nDraws[i] : 0;
         const int nLosses = m_nLosses ? m_nLosses[i] : 0;
         int nGames = m_nWins[i] + m_nDraws[i] + m_nLosses[i];
         if (nGames == 0)
             nGames = 1;
           s.AppendFormat(
            _T(
               "%0.1f/%ld\t%d-%d-%d (%.2f%%)\t(loss: m=%d t=%d i=%d a=%d)\t(draw: r=%d i=%d f=%d s=%d a=%d)\t(avg: tpm=%.1lf d=%.2lf nps=%lld)")
            , nWins + nDraws / 2.0, m_nGames[i]
            , nWins, nLosses, nDraws, (nWins + nDraws / 2.0) / nGames * 100
            , nLosses - m_nResults[WHITE_TIMEOUT][i] - m_nResults[BLACK_TIMEOUT][i] - m_nResults[WHITE_ILLEGAL][i] -
            m_nResults[BLACK_ILLEGAL][i] - m_nResults[ADJ_WHITE_MATES][i] - m_nResults[ADJ_BLACK_MATES][i],
            m_nResults[WHITE_TIMEOUT][i] + m_nResults[BLACK_TIMEOUT][i],
            m_nResults[WHITE_ILLEGAL][i] + m_nResults[BLACK_ILLEGAL][i],
            m_nResults[ADJ_WHITE_MATES][i] + m_nResults[ADJ_BLACK_MATES][i]
            , m_nResults[REPETITION][i], m_nResults[INSUF_MAT][i], m_nResults[FIFTY_MOVE][i], m_nResults[STALEMATE][i],
            m_nResults[ADJ_DRAW][i]
            , m_dTotalTime[i] / (m_dTotalSearches[i] == 0 ? 1 : m_dTotalSearches[i]),
            static_cast<double>(m_nTotalDepth[i]) / (m_nTotalDepthCount[i] == 0 ? 1 : m_nTotalDepthCount[i]),
            m_nTotalNPS[i] / (m_nTotalNPSCount[i] == 0 ? 1 : m_nTotalNPSCount[i]));
      }
      s.Append(_T("\r\n"));
   }

   m_wndResults.SetSel(0, -1);
   m_wndResults.ReplaceSel(s);
}

void CLittleBlitzerDlg::OnBnClickedPause()
{
   if (m_nNumEngines == 0) return;

   static int old;
   if (m_bPaused)
   {
      m_nNumTournaments = old;
      m_wndPause.SetWindowText("Pause\n(zero threads)");
      for (int id = 0; id < m_nNumTournaments; id++)
      {
         if (!m_Tournaments[id].m_bRunning && m_nGameNum < m_nNumGames)
         {
            m_Tournaments[id].m_nRound = GetNextRound();
            CWinThread* pThread = AfxBeginThread(RunTournament, &m_Tournaments[id]);
            m_nNumActiveTournaments++;
         }
      }
   }
   else
   {
      old = m_nNumTournaments;
      m_nNumTournaments = 0;
      CString s;
      s.Format("Resume\n(%d threads)", old);
      m_wndPause.SetWindowText(s);
   }
   UpdateNumTourneys();

   m_bPaused = !m_bPaused;
}

void CLittleBlitzerDlg::OnBnClickedOpenEngines()
{
   char szFilters[] = "Engine Files (*.lbe)|*.lbe|All Files (*.*)|*.*||";

   if (CFileDialog fileDlg(TRUE, _T("lbe"), _T("*.lbe"), OFN_FILEMUSTEXIST, szFilters, this); fileDlg.DoModal() == IDOK)
   {
      const CString sPathName = fileDlg.GetPathName();
      m_wndEngineFile.SetWindowText(sPathName);
   }
}

void CLittleBlitzerDlg::OnBnClickedLoadEngines()
{
   LoadEngineSettings(true);
   UpdateResults();
}

void CLittleBlitzerDlg::WriteBatchStatus(const char* format, ...) const
{
   if (!m_batchOptions.enabled || m_batchOptions.statusPath.IsEmpty()) return;

   FILE* file = fopen(m_batchOptions.statusPath, "at");
   if (!file) return;

   SYSTEMTIME now;
   GetLocalTime(&now);
   fprintf(file, "%04u-%02u-%02uT%02u:%02u:%02u ", now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
      now.wSecond);

   va_list arguments;
   va_start(arguments, format);
   vfprintf(file, format, arguments);
   va_end(arguments);
   fputc('\n', file);
   fclose(file);
}

void CLittleBlitzerDlg::FinishBatchWithError(const int exitCode, const CString& message)
{
   m_nBatchExitCode = exitCode;
   WriteBatchStatus("ERROR exit=%d message=%s", exitCode, static_cast<const char*>(message));
   EndDialog(IDCANCEL);
}

LRESULT CLittleBlitzerDlg::OnBatchStart(WPARAM, LPARAM)
{
   if (!m_batchOptions.error.IsEmpty())
   {
      FinishBatchWithError(2, m_batchOptions.error);
      return 0;
   }

   m_wndEngineFile.SetWindowText(m_batchOptions.enginesPath);
   m_wndTournFile.SetWindowText(m_batchOptions.tournamentPath);
   m_wndSavePGN.SetWindowText(m_batchOptions.resultsPath);

   CString workingDirectory = m_batchOptions.tournamentPath;
   const int slash = MAX(workingDirectory.ReverseFind('\\'), workingDirectory.ReverseFind('/'));
   if (slash >= 0)
   {
      workingDirectory = workingDirectory.Left(slash);
      if (!SetCurrentDirectory(workingDirectory))
      {
         FinishBatchWithError(2, "Unable to use the tournament settings directory");
         return 0;
      }
   }

   if (m_batchOptions.overwrite && !m_batchOptions.statusPath.IsEmpty())
      DeleteFile(m_batchOptions.statusPath);
   WriteBatchStatus("START engines=%s settings=%s results=%s",
      static_cast<const char*>(m_batchOptions.enginesPath), static_cast<const char*>(m_batchOptions.tournamentPath),
      static_cast<const char*>(m_batchOptions.resultsPath));

   if (!LoadTournamentSettings(false))
   {
      FinishBatchWithError(2, "Unable to load valid tournament settings or starting positions");
      return 0;
   }
   if (!LoadEngineSettings(false))
   {
      FinishBatchWithError(2, "Unable to load at least two valid engines");
      return 0;
   }
   if (!StartTournament())
   {
      FinishBatchWithError(2, "Unable to start tournament");
      return 0;
   }

   WriteBatchStatus("RUNNING games=%ld parallel=%d engines=%ld", m_nNumGames, m_nNumTournaments, m_nNumEngines);
   return 0;
}

void CLittleBlitzerDlg::OnBnClickedOpenTourn()
{
   char szFilters[] = "Tournament Files (*.lbt)|*.lbt|All Files (*.*)|*.*||";

   if (CFileDialog fileDlg(TRUE, _T("lbt"), _T("*.lbt"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilters, this); fileDlg.DoModal() == IDOK)
   {
      const CString sPathName = fileDlg.GetPathName();
      m_wndTournFile.SetWindowText(sPathName);
   }

   UpdateResults();
}

void CLittleBlitzerDlg::OnBnClickedCopytext()
{
   m_wndResults.SetSel(0, -1);
   m_wndResults.Copy();
   m_wndResults.SetSel(0, 0);
}

void CLittleBlitzerDlg::OnBnClickedInc()
{
   if (m_nNumTournaments >= MAX_THREADS) return;

   m_nNumTournaments++;

   if (m_Tournaments[m_nNumTournaments - 1].m_nNumEngines > 0 &&
      !m_Tournaments[m_nNumTournaments - 1].m_bRunning && m_nGameNum < m_nNumGames)
   {
      m_nNumActiveTournaments++;
      m_Tournaments[m_nNumTournaments - 1].m_nRound = GetNextRound();
      CWinThread* pThread = AfxBeginThread(RunTournament, &m_Tournaments[m_nNumTournaments - 1]);
   }

   if (m_bPaused)
   {
      m_bPaused = false;
      m_wndPause.SetWindowText("Pause\n(zero threads)");
   }

   UpdateNumTourneys();
}

void CLittleBlitzerDlg::OnBnClickedDec()
{
   if (m_nNumTournaments <= 0) return;

   m_nNumTournaments--;
   UpdateNumTourneys();
}

void CLittleBlitzerDlg::UpdateNumTourneys()
{
   CString s;
   s.Format(_T("%d Requested\n%d Running"), m_nNumTournaments, m_nNumActiveTournaments);
   m_wndNumThreads.SetWindowText(s.GetBuffer());
   m_wndFullPGN.EnableWindow(m_nNumActiveTournaments == 0);
}

void CLittleBlitzerDlg::OnBnClickedChkLog()
{
   g_bLogging = m_wndChkLog.GetCheck();
}

void CLittleBlitzerDlg::OnBnClickedChkIllegal()
{
   g_bDumpIllegalMoves = m_wndDumpIllegalMoves.GetCheck();
}

void CLittleBlitzerDlg::OnBnClickedChkFullPGN()
{
   g_bFullPGN = m_wndFullPGN.GetCheck();
}
