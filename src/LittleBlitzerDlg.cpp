#include "stdafx.h"
#include "LittleBlitzer.h"
#include "LittleBlitzerDlg.h"
#include "Common.h"
#include "TournSettings.h"
#include "Board.h"
#include "Move.h"

#include <cstdarg>
#include <io.h>
#include <sstream>
#include <string>
#include <vector>

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

std::string JsonEscape(const char* value)
{
   std::string result;
   if (!value) return result;
   for (const unsigned char c : std::string(value))
   {
      switch (c)
      {
      case '"': result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\b': result += "\\b"; break;
      case '\f': result += "\\f"; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:
         if (c < 0x20)
         {
            char encoded[7];
            sprintf_s(encoded, "\\u%04X", c);
            result += encoded;
         }
         else
         {
            result += static_cast<char>(c);
         }
      }
   }
   return result;
}

CStringA AbsolutePath(const CStringA& path)
{
   char fullPath[32768];
   const DWORD length = GetFullPathNameA(path, static_cast<DWORD>(std::size(fullPath)), fullPath, nullptr);
   if (length == 0 || length >= std::size(fullPath)) return path;
   return CStringA(fullPath);
}

bool IsValidFenSyntax(const char* fen)
{
   if (!fen || !*fen) return false;
   std::istringstream input(fen);
   std::string board;
   std::string side;
   std::string castling;
   std::string enPassant;
   if (!(input >> board >> side >> castling >> enPassant)) return false;

   int rank = 0;
   int files = 0;
   for (const char c : board)
   {
      if (c == '/')
      {
         if (files != 8 || ++rank >= 8) return false;
         files = 0;
      }
      else if (c >= '1' && c <= '8')
      {
         files += c - '0';
      }
      else if (strchr("prnbqkPRNBQK", c))
      {
         ++files;
      }
      else
      {
         return false;
      }
      if (files > 8) return false;
   }
   if (rank != 7 || files != 8) return false;
   if (side != "w" && side != "b") return false;
   if (castling != "-")
   {
      for (const char c : castling)
         if (!strchr("KQkqABCDEFGHabcdefgh", c)) return false;
   }
   if (enPassant != "-"
      && (enPassant.size() != 2 || enPassant[0] < 'a' || enPassant[0] > 'h'
         || (enPassant[1] != '3' && enPassant[1] != '6')))
      return false;
   return true;
}

constexpr int ENGINE_COLUMN_WIDTH = 36;

CString EngineColumnText(const char* name)
{
   CString text(name && *name ? name : "(unnamed)");
   text.Replace('\t', ' ');
   text.Replace('\r', ' ');
   text.Replace('\n', ' ');
   if (text.GetLength() <= ENGINE_COLUMN_WIDTH) return text;
   return text.Left(ENGINE_COLUMN_WIDTH - 3) + "...";
}

bool CopyTextToClipboard(const HWND owner, const CString& text)
{
   if (!OpenClipboard(owner)) return false;
   if (!EmptyClipboard())
   {
      CloseClipboard();
      return false;
   }

   const SIZE_T byteCount = (static_cast<SIZE_T>(text.GetLength()) + 1) * sizeof(TCHAR);
   HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, byteCount);
   if (!memory)
   {
      CloseClipboard();
      return false;
   }

   void* destination = GlobalLock(memory);
   if (!destination)
   {
      GlobalFree(memory);
      CloseClipboard();
      return false;
   }
   CopyMemory(destination, text.GetString(), byteCount);
   GlobalUnlock(memory);

#ifdef _UNICODE
   constexpr UINT clipboardFormat = CF_UNICODETEXT;
#else
   constexpr UINT clipboardFormat = CF_TEXT;
#endif
   if (!SetClipboardData(clipboardFormat, memory))
   {
      GlobalFree(memory);
      CloseClipboard();
      return false;
   }
   CloseClipboard();
   return true;
}
}

CLittleBlitzerDlg::CLittleBlitzerDlg(const TBatchOptions& batchOptions, CWnd* pParent)
   : CDialog(IDD, pParent), m_sStartPositionFile{}, m_bPaused(false), m_batchOptions(batchOptions),
     m_nBatchExitCode(0), m_nBatchIllegalGames(0), m_nBatchEngineFailures(0), m_bOutputFailed(false),
     m_nConfiguredOpeningSeed(0)
{
   m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

   m_nGameNum = 0;
   m_nNumGamesPlayed = 0;
   m_nBatchIllegalGames = 0;
   m_nBatchEngineFailures = 0;
   m_bOutputFailed = false;
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
   DDX_Control(pDX, IDC_RUN_GROUP, m_wndRunGroup);
   DDX_Control(pDX, IDC_OPTIONS_GROUP, m_wndOptionsGroup);
   DDX_Control(pDX, IDC_MATCH_GROUP, m_wndMatchGroup);
   DDX_Control(pDX, IDC_RESULTS_GROUP, m_wndResultsGroup);
   DDX_Control(pDX, IDC_STATUS, m_wndResults);
   DDX_Control(pDX, IDC_MATCH_SETTINGS, m_wndMatchSettings);
   DDX_Control(pDX, IDC_MATCH_TIME, m_wndMatchTime);
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

BEGIN_MESSAGE_MAP(CClickThroughGroup, CButton)
   ON_WM_NCHITTEST()
END_MESSAGE_MAP()

LRESULT CClickThroughGroup::OnNcHitTest(CPoint)
{
   return HTTRANSPARENT;
}

BEGIN_MESSAGE_MAP(CLittleBlitzerDlg, CDialog)
   ON_WM_PAINT()
   ON_WM_CTLCOLOR()
   ON_WM_DRAWITEM()
   ON_WM_GETMINMAXINFO()
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

   CRect initialWindowBounds;
   GetWindowRect(initialWindowBounds);
   m_fixedWindowWidth = initialWindowBounds.Width();

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

   m_whiteBackgroundBrush.CreateSolidBrush(RGB(255, 255, 255));
   if (m_resultsFont.CreatePointFont(90, _T("Consolas")))
      m_wndResults.SetFont(&m_resultsFont);
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

HBRUSH CLittleBlitzerDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, const UINT nCtlColor)
{
   const HBRUSH defaultBrush = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
   if (!pWnd || !m_whiteBackgroundBrush.GetSafeHandle()) return defaultBrush;

   const int controlId = pWnd->GetDlgCtrlID();
   if (controlId != IDC_STATUS && controlId != IDC_MATCH_SETTINGS && controlId != IDC_MATCH_TIME
      && controlId != IDC_MATCH_BACKGROUND)
      return defaultBrush;

   pDC->SetTextColor(RGB(0, 0, 0));
   pDC->SetBkColor(RGB(255, 255, 255));
   return static_cast<HBRUSH>(m_whiteBackgroundBrush.GetSafeHandle());
}

void CLittleBlitzerDlg::OnDrawItem(const int nIDCtl, LPDRAWITEMSTRUCT drawItem)
{
   if (nIDCtl != IDC_RUN_GROUP && nIDCtl != IDC_OPTIONS_GROUP
      && nIDCtl != IDC_MATCH_GROUP && nIDCtl != IDC_RESULTS_GROUP)
   {
      CDialog::OnDrawItem(nIDCtl, drawItem);
      return;
   }

   CDC* dc = CDC::FromHandle(drawItem->hDC);
   CRect bounds(drawItem->rcItem);
   dc->FillSolidRect(bounds, GetSysColor(COLOR_3DFACE));

   CWnd* group = GetDlgItem(nIDCtl);
   CString caption;
   group->GetWindowText(caption);

   CFont* oldFont = nullptr;
   if (CFont* font = group->GetFont())
      oldFont = dc->SelectObject(font);

   const CSize captionSize = dc->GetTextExtent(caption);
   const int dpi = dc->GetDeviceCaps(LOGPIXELSX);
   const int lineWidth = max(1, MulDiv(1, dpi, 96));
   const int captionX = bounds.left + MulDiv(10, dpi, 96);
   const int captionGap = MulDiv(4, dpi, 96);
   const int borderY = bounds.top + captionSize.cy / 2;
   const int borderInset = (lineWidth + 1) / 2;
   const int left = bounds.left + borderInset;
   const int right = bounds.right - borderInset - 1;
   const int bottom = bounds.bottom - borderInset - 1;

   CPen borderPen(PS_SOLID, lineWidth, GetSysColor(COLOR_3DSHADOW));
   CPen* oldPen = dc->SelectObject(&borderPen);
   dc->MoveTo(left, borderY);
   dc->LineTo(captionX - captionGap, borderY);
   dc->MoveTo(captionX + captionSize.cx + captionGap, borderY);
   dc->LineTo(right, borderY);
   dc->LineTo(right, bottom);
   dc->LineTo(left, bottom);
   dc->LineTo(left, borderY);
   dc->SelectObject(oldPen);

   const int oldBackgroundMode = dc->SetBkMode(TRANSPARENT);
   const COLORREF oldTextColor = dc->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
   dc->TextOut(captionX, bounds.top, caption);
   dc->SetTextColor(oldTextColor);
   dc->SetBkMode(oldBackgroundMode);
   if (oldFont)
      dc->SelectObject(oldFont);
}

void CLittleBlitzerDlg::OnGetMinMaxInfo(MINMAXINFO* minMaxInfo)
{
   CDialog::OnGetMinMaxInfo(minMaxInfo);
   if (m_fixedWindowWidth <= 0) return;

   minMaxInfo->ptMinTrackSize.x = m_fixedWindowWidth;
   minMaxInfo->ptMaxTrackSize.x = m_fixedWindowWidth;
   minMaxInfo->ptMaxSize.x = m_fixedWindowWidth;
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
   bool invalidPosition = false;
   if (f)
   {
      m_nNumTournaments = 0;
      m_Tournaments[0].m_nOpeningSeed = 0;
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
         else if (!key.CompareNoCase("AdjudicateDrawMoves")) m_Tournaments[0].m_nAdjDrawMoves = MIN(300, MAX(1, number));
         else if (!key.CompareNoCase("Randomize")) m_Tournaments[0].m_nRandomize = number != 0;
         else if (!key.CompareNoCase("OpeningSeed")) m_Tournaments[0].m_nOpeningSeed = _strtoui64(value, nullptr, 10);
         else if (!key.CompareNoCase("Position"))
         {
            if (!value.Left(4).CompareNoCase("FEN:"))
            {
               m_nStartPositionType = 1;
               m_sStartPositionFile[0] = 0;
               FreeStartPositions(m_Tournaments[0]);
               CString fen = value.Mid(4);
               if (!IsValidFenSyntax(fen))
               {
                  invalidPosition = true;
               }
               else
               {
                  m_Tournaments[0].m_sStartPositions = new char* [1];
                  m_Tournaments[0].m_sStartPositions[0] = new char[fen.GetLength() + 1];
                  strcpy_s(m_Tournaments[0].m_sStartPositions[0], fen.GetLength() + 1, fen);
                  m_Tournaments[0].m_nNumStartPositions = 1;
               }
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

      if (invalidPosition || (m_Tournaments[0].m_nType != 0 && m_Tournaments[0].m_nType != 1)
         || m_Tournaments[0].m_nTC < TC_FIXED_TPM || m_Tournaments[0].m_nTC > TC_TOURNAMENT
         || m_nNumTournaments <= 0)
      {
         if (interactive)
            MessageBox(_T("Tournament settings contain an invalid type, time control, worker count, or FEN."),
               _T("Invalid settings"), MB_OK | MB_ICONERROR);
         return false;
      }

      m_nConfiguredOpeningSeed = m_Tournaments[0].m_nOpeningSeed;
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
   fprintf(f, "OpeningSeed: %llu\n", m_nConfiguredOpeningSeed);
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
         bool truncated = false;
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
            if (i == MAX_FEN_LEN - 1)
            {
               truncated = true;
               while (c != '\n' && c != '\r' && c != EOF) c = fgetc(fepd);
               break;
            }
         }
         sFEN[i] = 0;
         if (i > 0 && !truncated && IsValidFenSyntax(sFEN))
         {
            m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions] = new char[strlen(sFEN) + 1];
            strcpy(m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions++], sFEN);
         }
         if (c == EOF) break;
         if (m_Tournaments[0].m_nNumStartPositions == MAX_START_POSITIONS) break;
      } while (true);
      fclose(fepd);
   }
}

int CLittleBlitzerDlg::ReadLine(FILE* f, char* s)
{
   int c = fgetc(f);
   int i = 0;
   if (c == EOF)
   {
      s[0] = 0;
      return EOF;
   }
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
            if (IsValidFenSyntax(sFEN))
            {
               m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions] = new char[strlen(sFEN) + 1];
               strcpy(m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions++], sFEN);
            }
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
            if (IsValidFenSyntax(sFEN))
            {
               m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions] = new char[strlen(sFEN) + 1];
               strcpy(m_Tournaments[0].m_sStartPositions[m_Tournaments[0].m_nNumStartPositions++], sFEN);
            }
         }
         if (c == EOF) break;
         if (m_Tournaments[0].m_nNumStartPositions == MAX_START_POSITIONS) break;
      } while (true);
      fclose(fepd);
   }
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

   m_wndResults.SetWindowText(_T("Loading engines..."));

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
            else if (!parameterName.CompareNoCase("LB_ExpectedUCI"))
            {
               delete[] m_Engines[m_nNumEngines - 1].m_sExpectedName;
               m_Engines[m_nNumEngines - 1].m_sExpectedName = new char[parameterValue.GetLength() + 1];
               strcpy_s(m_Engines[m_nNumEngines - 1].m_sExpectedName, parameterValue.GetLength() + 1, parameterValue);
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
         {
            const CString error = m_Engines[i].m_sError.IsEmpty()
               ? CString(_T("Engine loading stopped because an engine failed to initialize."))
               : m_Engines[i].m_sError;
            MessageBox(error, _T("Engine error"), MB_OK | MB_ICONERROR);
         }
         else if (!m_Engines[i].m_sError.IsEmpty())
            WriteBatchStatus("ENGINE_ERROR index=%d message=%s", i, static_cast<const char*>(m_Engines[i].m_sError));
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
   m_nBatchIllegalGames = 0;
   m_nBatchEngineFailures = 0;
   m_bOutputFailed = false;

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

   unsigned long long openingSeed = m_nConfiguredOpeningSeed;
   if (openingSeed == 0)
   {
      std::random_device randomDevice;
      openingSeed = static_cast<unsigned long long>(randomDevice()) << 32 | randomDevice();
      if (openingSeed == 0) openingSeed = 1;
   }
   m_Tournaments[0].m_nOpeningSeed = openingSeed;

   if (!WriteRunManifest(openingSeed))
   {
      if (!m_batchOptions.enabled)
         MessageBox(_T("Unable to create the run manifest beside the PGN file."), _T("Cannot start"),
            MB_OK | MB_ICONERROR);
      return false;
   }

   m_nTimeTaken.Start();

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
         AfxBeginThread(RunTournament, &m_Tournaments[x]);
      }
   }
   m_wndStart.EnableWindow(false);
   m_wndLoadEngines.EnableWindow(false);
   m_wndLoadTournament.EnableWindow(false);

   UpdateResults();
   UpdateNumTourneys();
   return true;
}

bool CLittleBlitzerDlg::WriteRunManifest(const unsigned long long openingSeed)
{
   CString enginesPath;
   CString settingsPath;
   m_wndEngineFile.GetWindowText(enginesPath);
   m_wndTournFile.GetWindowText(settingsPath);
   enginesPath = AbsolutePath(enginesPath);
   settingsPath = AbsolutePath(settingsPath);
   const CStringA resultsPath = AbsolutePath(m_sResultsPath);
   const CStringA manifestPath = resultsPath + ".manifest.json";
   const CStringA temporaryPath = manifestPath + ".tmp";

   char executablePath[32768];
   if (!GetModuleFileNameA(nullptr, executablePath, static_cast<DWORD>(std::size(executablePath))))
      return false;

   CStringA executableHash;
   CStringA enginesHash;
   CStringA settingsHash;
   if (!GetFileSha256(executablePath, &executableHash)
      || !GetFileSha256(enginesPath, &enginesHash)
      || !GetFileSha256(settingsPath, &settingsHash))
      return false;

   CStringA openingPath;
   CStringA openingHash;
   if (m_nStartPositionType == 2 || m_nStartPositionType == 3)
   {
      openingPath = AbsolutePath(m_sStartPositionFile);
      if (!GetFileSha256(openingPath, &openingHash)) return false;
   }

   std::vector<CStringA> engineHashes;
   engineHashes.reserve(m_nNumEngines);
   for (int i = 0; i < m_nNumEngines; ++i)
   {
      CStringA hash;
      if (!GetFileSha256(m_Engines[i].m_sPath, &hash)) return false;
      engineHashes.push_back(hash);
   }

   FILE* file = fopen(temporaryPath, "wb");
   if (!file) return false;

   SYSTEMTIME now;
   GetSystemTime(&now);
   const long pairingCycle = m_Tournaments[0].m_nType == 0
      ? 2 * (m_nNumEngines - 1)
      : m_nNumEngines * (m_nNumEngines - 1);

   fprintf(file, "{\n");
   fprintf(file, "  \"format_version\": 1,\n");
   fprintf(file, "  \"created_utc\": \"%04u-%02u-%02uT%02u:%02u:%02uZ\",\n", now.wYear, now.wMonth, now.wDay,
      now.wHour, now.wMinute, now.wSecond);
   fprintf(file, "  \"littleblitzer_version\": \"%s\",\n", JsonEscape(VERSION).c_str());
   fprintf(file, "  \"littleblitzer_path\": \"%s\",\n", JsonEscape(AbsolutePath(executablePath)).c_str());
   fprintf(file, "  \"littleblitzer_sha256\": \"%s\",\n", executableHash.GetString());
   fprintf(file, "  \"engines_file\": \"%s\",\n", JsonEscape(enginesPath).c_str());
   fprintf(file, "  \"engines_file_sha256\": \"%s\",\n", enginesHash.GetString());
   fprintf(file, "  \"settings_file\": \"%s\",\n", JsonEscape(settingsPath).c_str());
   fprintf(file, "  \"settings_file_sha256\": \"%s\",\n", settingsHash.GetString());
   fprintf(file, "  \"results_file\": \"%s\",\n", JsonEscape(resultsPath).c_str());
   fprintf(file, "  \"opening_file\": \"%s\",\n", JsonEscape(openingPath).c_str());
   fprintf(file, "  \"opening_file_sha256\": \"%s\",\n", openingHash.GetString());
   fprintf(file, "  \"opening_seed\": \"%llu\",\n", openingSeed);
   fprintf(file, "  \"opening_positions_loaded\": %d,\n", m_Tournaments[0].m_nNumStartPositions);
   fprintf(file, "  \"tournament\": {\n");
   fprintf(file, "    \"type\": %d,\n", m_Tournaments[0].m_nType);
   fprintf(file, "    \"time_control\": %d,\n", m_Tournaments[0].m_nTC);
   fprintf(file, "    \"base_ms\": %ld,\n", m_Tournaments[0].m_nBase);
   fprintf(file, "    \"increment_ms\": %ld,\n", m_Tournaments[0].m_nInc);
   fprintf(file, "    \"games\": %ld,\n", m_nNumGames);
   fprintf(file, "    \"parallel_games\": %d,\n", m_nNumTournaments);
   fprintf(file, "    \"hash_mb_per_engine\": %ld,\n", m_Tournaments[0].m_nHash);
   fprintf(file, "    \"ponder\": %s,\n", m_Tournaments[0].m_bPonder ? "true" : "false");
   fprintf(file, "    \"own_book\": %s,\n", m_Tournaments[0].m_bOwnBook ? "true" : "false");
   fprintf(file, "    \"variant\": %d,\n", m_Tournaments[0].m_nVariant);
   fprintf(file, "    \"randomize_openings\": %s,\n", m_Tournaments[0].m_nRandomize ? "true" : "false");
   fprintf(file, "    \"starting_position_type\": %d,\n", m_nStartPositionType);
   fprintf(file, "    \"starting_fen\": \"%s\",\n",
      JsonEscape(m_nStartPositionType <= 1 && m_Tournaments[0].m_nNumStartPositions > 0
         ? m_Tournaments[0].m_sStartPositions[0] : "").c_str());
   fprintf(file, "    \"adjudicate_mate_score_cp\": %ld,\n", m_Tournaments[0].m_nAdjMateScore);
   fprintf(file, "    \"adjudicate_mate_moves\": %ld,\n", m_Tournaments[0].m_nAdjMateMoves);
   fprintf(file, "    \"adjudicate_draw_moves\": %ld,\n", m_Tournaments[0].m_nAdjDrawMoves);
   fprintf(file, "    \"full_pgn\": %s,\n", g_bFullPGN ? "true" : "false");
   fprintf(file, "    \"pairing_cycle_games\": %ld,\n", pairingCycle);
   fprintf(file, "    \"complete_pairing_cycles\": %s\n",
      pairingCycle > 0 && m_nNumGames % pairingCycle == 0 ? "true" : "false");
   fprintf(file, "  },\n");
   fprintf(file, "  \"engines\": [\n");
   for (int i = 0; i < m_nNumEngines; ++i)
   {
      const CEngine& engine = m_Engines[i];
      fprintf(file, "    {\n");
      fprintf(file, "      \"index\": %d,\n", i);
      fprintf(file, "      \"path\": \"%s\",\n", JsonEscape(AbsolutePath(engine.m_sPath)).c_str());
      fprintf(file, "      \"sha256\": \"%s\",\n", engineHashes[i].GetString());
      fprintf(file, "      \"display_name\": \"%s\",\n", JsonEscape(engine.m_sName).c_str());
      fprintf(file, "      \"uci_name\": \"%s\",\n", JsonEscape(engine.m_sUciName).c_str());
      fprintf(file, "      \"expected_uci_name\": \"%s\",\n", JsonEscape(engine.m_sExpectedName).c_str());
      fprintf(file, "      \"options\": {");
      for (int option = 0; option < engine.m_nNumParameters; ++option)
      {
         fprintf(file, "%s\n        \"%s\": \"%s\"", option == 0 ? "" : ",", JsonEscape(engine.m_sParameterNames[option]).c_str(),
            JsonEscape(engine.m_sParameterValues[option]).c_str());
      }
      if (engine.m_nNumParameters > 0) fprintf(file, "\n      ");
      fprintf(file, "}\n");
      fprintf(file, "    }%s\n", i + 1 == m_nNumEngines ? "" : ",");
   }
   fprintf(file, "  ]\n");
   fprintf(file, "}\n");

   const bool writeSucceeded = fflush(file) == 0 && ferror(file) == 0 && _commit(_fileno(file)) == 0;
   const bool closeSucceeded = fclose(file) == 0;
   if (!writeSucceeded || !closeSucceeded
      || !MoveFileExA(temporaryPath, manifestPath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
   {
      DeleteFileA(temporaryPath);
      return false;
   }
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
   else if (tResult->nResult == WHITE_ENGINE_FAILURE)
   {
      m_nWins[tResult->nBlack]++;
      m_nLosses[tResult->nWhite]++;
      m_nResults[tResult->nResult][tResult->nWhite]++;
   }
   else if (tResult->nResult == BLACK_ENGINE_FAILURE)
   {
      m_nWins[tResult->nWhite]++;
      m_nLosses[tResult->nBlack]++;
      m_nResults[tResult->nResult][tResult->nBlack]++;
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
   if (!UpdatePGN(tResult))
   {
      m_bOutputFailed = true;
   }
   if (tResult->nResult == WHITE_ILLEGAL || tResult->nResult == BLACK_ILLEGAL)
      m_nBatchIllegalGames++;
   if (tResult->nResult == WHITE_ENGINE_FAILURE || tResult->nResult == BLACK_ENGINE_FAILURE)
      m_nBatchEngineFailures++;

   m_nNumActiveTournaments--;
   if (const int id = static_cast<int>(lParam); !m_bOutputFailed && id < m_nNumTournaments
      && m_nNumGamesPlayed + m_nNumActiveTournaments < m_nNumGames)
   {
      m_nNumActiveTournaments++;
      m_Tournaments[id].m_bRunning = true;
      m_Tournaments[id].m_nRound = GetNextRound();
      AfxBeginThread(RunTournament, &m_Tournaments[id]);
   }
   UpdateNumTourneys();

   if (m_batchOptions.enabled)
   {
      if (!WriteBatchStatus("PROGRESS completed=%ld total=%ld active=%d illegal=%ld engine_failures=%ld",
         m_nNumGamesPlayed, m_nNumGames, m_nNumActiveTournaments, m_nBatchIllegalGames, m_nBatchEngineFailures))
         m_bOutputFailed = true;
   }

   if (m_nNumActiveTournaments == 0 && m_bOutputFailed)
   {
      m_wndStart.EnableWindow(true);
      m_wndLoadEngines.EnableWindow(true);
      m_wndLoadTournament.EnableWindow(true);
      m_bPaused = false;
      m_wndPause.SetWindowText("Pause");
      if (m_batchOptions.enabled)
         FinishBatchWithError(2, "Unable to append and flush the PGN results or batch status file");
      else
         MessageBox(_T("The tournament stopped because the PGN results file could not be written safely."),
            _T("Results write failure"), MB_OK | MB_ICONERROR);
   }
   else if (m_nNumActiveTournaments == 0 && m_nNumGamesPlayed >= m_nNumGames)
   {
      m_wndStart.EnableWindow(true);
      m_wndLoadEngines.EnableWindow(true);
      m_wndLoadTournament.EnableWindow(true);
      m_bPaused = false;
      m_wndPause.SetWindowText("Pause");
      if (m_batchOptions.enabled)
      {
         m_nBatchExitCode = m_nBatchIllegalGames == 0 && m_nBatchEngineFailures == 0 ? 0 : 3;
         if (!WriteBatchStatus("COMPLETE completed=%ld total=%ld illegal=%ld engine_failures=%ld exit=%d",
            m_nNumGamesPlayed, m_nNumGames, m_nBatchIllegalGames, m_nBatchEngineFailures, m_nBatchExitCode))
            m_nBatchExitCode = 2;
         EndDialog(IDOK);
      }
   }

   return 0;
}

bool CLittleBlitzerDlg::InitPGN()
{
   m_wndSavePGN.GetWindowText(m_sResultsPath);

   if (FILE* fpgn = fopen(m_sResultsPath.GetString(), "rt"))
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

bool CLittleBlitzerDlg::UpdatePGN(TResult* r)
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
   else if (r->nResult == WHITE_ENGINE_FAILURE)
   {
      strcpy(sResult, "0-1");
      sTermination = "death";
   }
   else if (r->nResult == BLACK_ENGINE_FAILURE)
   {
      strcpy(sResult, "1-0");
      sTermination = "death";
   }
   FILE* fpgn;

   if (!(fpgn = fopen(m_sResultsPath.GetBuffer(), "at")))
   {
      delete[] r->sSAN;
      return false;
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
   }
   else
   {
      fprintf(fpgn, "\n%s\n\n", sResult);
   }

   delete[] r->sSAN;
   const bool writeSucceeded = fflush(fpgn) == 0 && ferror(fpgn) == 0 && _commit(_fileno(fpgn)) == 0;
   const bool closeSucceeded = fclose(fpgn) == 0;
   return writeSucceeded && closeSucceeded;
}

void CLittleBlitzerDlg::UpdateResults()
{
   const double dElapsed = m_nTimeTaken.GetMS();
   const bool hasRemainingEstimate = m_nNumGamesPlayed > 0 && m_nNumGames > 0;
   const double dRemaining = hasRemainingEstimate
      ? dElapsed * static_cast<double>(m_nNumGames - m_nNumGamesPlayed) / m_nNumGamesPlayed
      : 0.0;
   CString s;
   s.Format("Games completed: %ld of %ld (%.1lf%%) | Average game length: %.3lf sec\r\n",
      m_nNumGamesPlayed, m_nNumGames,
      100.0 * m_nNumGamesPlayed / (m_nNumGames ? m_nNumGames : 1),
      m_dTotalGamesLen / (m_nNumGamesPlayed ? m_nNumGamesPlayed : 1) / 1000.0);

   CString sVariant, sBook, sTC, sStart, sAdj;
   sVariant.Format("%s", m_Tournaments[0].m_nVariant == VARIANT_960 ? " / FRC" : "");
   sBook.Format("%s", m_Tournaments[0].m_bOwnBook ? "Engine book | " : "");
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
   else
   {
      sStart.Format("Standard start");
   }
   sAdj.Format("M %dcp for %d moves, D %d moves", m_Tournaments[0].m_nAdjMateScore, m_Tournaments[0].m_nAdjMateMoves,
      m_Tournaments[0].m_nAdjDrawMoves);
   CString matchSettings;
   matchSettings.Format("Match: %s%s | Hash: %d MB | %s%s | %s | %s",
      m_Tournaments[0].m_nType == 0 ? "Gauntlet" : "Round robin",
      sVariant.GetString(), m_Tournaments[0].m_nHash, sBook.GetString(), sTC.GetString(), sAdj.GetString(),
      sStart.GetString());
   m_wndMatchSettings.SetWindowText(matchSettings);

   CString matchTime;
   if (!m_nWins)
      matchTime.Format(_T("Time: ready to start"));
   else if (hasRemainingEstimate)
      matchTime.Format(_T("Time: %.0lf sec elapsed | %.0lf sec remaining"), dElapsed / 1000, dRemaining / 1000);
   else
      matchTime.Format(_T("Time: %.0lf sec elapsed | estimating remaining time"), dElapsed / 1000);
   m_wndMatchTime.SetWindowText(matchTime);

   if (m_nNumEngines > 0 && !m_nWins)
   {
      s.Append(_T("\r\nENGINES\r\n"));
      s.AppendFormat(_T(" #  %-36s\r\n"), _T("Engine"));
      for (int i = 0; i < m_nNumEngines; ++i)
      {
         const CString engine = EngineColumnText(m_Engines[i].m_sName);
         s.AppendFormat(_T("%2d. %-36s\r\n"), i + 1, engine.GetString());
      }
   }
   else if (m_nWins)
   {
      s.Append(_T("\r\nRESULTS\r\n"));
      s.AppendFormat(_T(" #  %-36s   %15s   %15s   %15s   %15s   %15s   %15s\r\n"), _T("Engine"),
         _T("Points/Games"), _T("Score"), _T("W-L-D"), _T("ms/move"), _T("Depth"), _T("NPS"));

      for (int i = 0; i < m_nNumEngines; ++i)
      {
         const long wins = m_nWins[i];
         const long draws = m_nDraws[i];
         const long losses = m_nLosses[i];
         const long games = wins + draws + losses;
         const long safeGames = games == 0 ? 1 : games;
         CString points;
         CString record;
         points.Format(_T("%.1f/%ld"), wins + draws / 2.0, games);
         record.Format(_T("%ld-%ld-%ld"), wins, losses, draws);
         const CString engine = EngineColumnText(m_Engines[i].m_sName);
         const double timePerMove = m_dTotalTime[i] / (m_dTotalSearches[i] == 0 ? 1 : m_dTotalSearches[i]);
         const double depth = static_cast<double>(m_nTotalDepth[i])
            / (m_nTotalDepthCount[i] == 0 ? 1 : m_nTotalDepthCount[i]);
         const long long nps = m_nTotalNPS[i] / (m_nTotalNPSCount[i] == 0 ? 1 : m_nTotalNPSCount[i]);
         s.AppendFormat(_T("%2d. %-36s   %15s   %14.2f%%   %15s   %15.1f   %15.2f   %15lld\r\n"), i + 1,
            engine.GetString(), points.GetString(), (wins + draws / 2.0) / safeGames * 100, record.GetString(),
            timePerMove, depth, nps);
      }

      s.Append(_T("\r\nLOSSES\r\n"));
      s.AppendFormat(_T(" #  %-36s   %15s   %15s   %15s   %15s   %15s\r\n"), _T("Engine"),
         _T("Adjudication"), _T("Mate"), _T("Timeout"), _T("Illegal"), _T("Crash"));

      for (int i = 0; i < m_nNumEngines; ++i)
      {
         const long mate = m_nLosses[i] - m_nResults[WHITE_TIMEOUT][i] - m_nResults[BLACK_TIMEOUT][i]
            - m_nResults[WHITE_ILLEGAL][i] - m_nResults[BLACK_ILLEGAL][i]
            - m_nResults[WHITE_ENGINE_FAILURE][i] - m_nResults[BLACK_ENGINE_FAILURE][i]
            - m_nResults[ADJ_WHITE_MATES][i] - m_nResults[ADJ_BLACK_MATES][i];
         const long timeout = m_nResults[WHITE_TIMEOUT][i] + m_nResults[BLACK_TIMEOUT][i];
         const long illegal = m_nResults[WHITE_ILLEGAL][i] + m_nResults[BLACK_ILLEGAL][i];
         const long crash = m_nResults[WHITE_ENGINE_FAILURE][i] + m_nResults[BLACK_ENGINE_FAILURE][i];
         const long adjudicatedMate = m_nResults[ADJ_WHITE_MATES][i] + m_nResults[ADJ_BLACK_MATES][i];
         const CString engine = EngineColumnText(m_Engines[i].m_sName);
         s.AppendFormat(_T("%2d. %-36s   %15ld   %15ld   %15ld   %15ld   %15ld\r\n"), i + 1,
            engine.GetString(), adjudicatedMate, mate, timeout, illegal, crash);
      }

      s.Append(_T("\r\nDRAWS\r\n"));
      s.AppendFormat(_T(" #  %-36s   %15s   %15s   %15s   %15s   %15s\r\n"), _T("Engine"),
         _T("Adjudication"), _T("Repeat"), _T("Insuff"), _T("50Move"), _T("Stalemate"));

      for (int i = 0; i < m_nNumEngines; ++i)
      {
         const CString engine = EngineColumnText(m_Engines[i].m_sName);
         s.AppendFormat(_T("%2d. %-36s   %15ld   %15ld   %15ld   %15ld   %15ld\r\n"), i + 1,
            engine.GetString(), m_nResults[ADJ_DRAW][i], m_nResults[REPETITION][i],
            m_nResults[INSUF_MAT][i], m_nResults[FIFTY_MOVE][i], m_nResults[STALEMATE][i]);
      }
   }

   bool hasTruncatedNames = false;
   for (int i = 0; i < m_nNumEngines; ++i)
   {
      const char* displayName = m_Engines[i].m_sName ? m_Engines[i].m_sName : "";
      const bool truncated = strlen(displayName) > ENGINE_COLUMN_WIDTH;
      if (!truncated) continue;
      if (!hasTruncatedNames)
      {
         s.Append(_T("\r\nFULL ENGINE NAMES (truncated in tables)\r\n"));
         hasTruncatedNames = true;
      }
      s.AppendFormat(_T("%2d. %s\r\n"), i + 1, displayName);
   }

   const int firstVisibleLine = m_wndResults.GetFirstVisibleLine();
   m_wndResults.SetRedraw(FALSE);
   m_wndResults.SetWindowText(s);
   m_wndResults.LineScroll(firstVisibleLine);
   m_wndResults.SetRedraw(TRUE);
   m_wndResults.Invalidate(FALSE);
}

void CLittleBlitzerDlg::OnBnClickedPause()
{
   if (m_nNumEngines == 0) return;

   static int old;
   if (m_bPaused)
   {
      m_nNumTournaments = old;
      m_wndPause.SetWindowText("Pause");
      for (int id = 0; id < m_nNumTournaments; id++)
      {
         if (!m_Tournaments[id].m_bRunning && m_nGameNum < m_nNumGames)
         {
            m_Tournaments[id].m_nRound = GetNextRound();
            AfxBeginThread(RunTournament, &m_Tournaments[id]);
            m_nNumActiveTournaments++;
         }
      }
   }
   else
   {
      old = m_nNumTournaments;
      m_nNumTournaments = 0;
      m_wndPause.SetWindowText("Resume");
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

bool CLittleBlitzerDlg::WriteBatchStatus(const char* format, ...) const
{
   if (!m_batchOptions.enabled || m_batchOptions.statusPath.IsEmpty()) return true;

   FILE* file = fopen(m_batchOptions.statusPath, "at");
   if (!file) return false;

   SYSTEMTIME now;
   GetLocalTime(&now);
   fprintf(file, "%04u-%02u-%02uT%02u:%02u:%02u ", now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
      now.wSecond);

   va_list arguments;
   va_start(arguments, format);
   vfprintf(file, format, arguments);
   va_end(arguments);
   fputc('\n', file);
   const bool writeSucceeded = fflush(file) == 0 && ferror(file) == 0 && _commit(_fileno(file)) == 0;
   const bool closeSucceeded = fclose(file) == 0;
   return writeSucceeded && closeSucceeded;
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
   if (!WriteBatchStatus("START engines=%s settings=%s results=%s",
      static_cast<const char*>(m_batchOptions.enginesPath), static_cast<const char*>(m_batchOptions.tournamentPath),
      static_cast<const char*>(m_batchOptions.resultsPath)))
   {
      m_nBatchExitCode = 2;
      EndDialog(IDCANCEL);
      return 0;
   }

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
   CString settings;
   CString time;
   CString results;
   m_wndMatchSettings.GetWindowText(settings);
   m_wndMatchTime.GetWindowText(time);
   m_wndResults.GetWindowText(results);

   CString report;
   report.Format(_T("%s\r\n%s\r\n\r\n%s"), settings.GetString(), time.GetString(), results.GetString());
   if (!CopyTextToClipboard(GetSafeHwnd(), report))
      MessageBeep(MB_ICONWARNING);
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
      AfxBeginThread(RunTournament, &m_Tournaments[m_nNumTournaments - 1]);
   }

   if (m_bPaused)
   {
      m_bPaused = false;
      m_wndPause.SetWindowText("Pause");
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
   s.Format(_T("%d requested\n%d running"), m_nNumTournaments, m_nNumActiveTournaments);
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
