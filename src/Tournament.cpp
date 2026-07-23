#include "StdAfx.h"
#include "Tournament.h"

#include <afxwin.h>

#include "Common.h"
#include "Timer.h"
#include "Board.h"
#include "Move.h"
#include "MoveGen.h"
#include "Engine.h"

#ifdef _MSC_VER
#pragma warning(disable : 4018)   
#pragma warning(disable : 4267)           
#else
#endif

namespace
{
std::mt19937& RandomGenerator()
{
   thread_local std::mt19937 generator(std::random_device{}());
   return generator;
}

unsigned long long MixOpeningIndex(unsigned long long value)
{
   value += 0x9e3779b97f4a7c15ULL;
   value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
   value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
   return value ^ (value >> 31);
}

int PairedOpeningIndex(const unsigned long long pairing, const int positionCount,
   const bool randomize, const unsigned long long seed)
{
   if (positionCount <= 1) return 0;
   const unsigned long long value = randomize ? MixOpeningIndex(pairing ^ seed) : pairing;
   return static_cast<int>(value % static_cast<unsigned long long>(positionCount));
}

unsigned long long RoundRobinPairing(const long round, const int engineCount, const int white, const int black)
{
   const unsigned long long gamesPerCycle = static_cast<unsigned long long>(engineCount) * (engineCount - 1);
   const unsigned long long pairsPerCycle = gamesPerCycle / 2;
   const unsigned long long cycle = static_cast<unsigned long long>(round) / gamesPerCycle;
   const int first = MIN(white, black);
   const int second = MAX(white, black);
   const unsigned long long pairInCycle =
      static_cast<unsigned long long>(first) * (2ULL * engineCount - first - 1) / 2 + second - first - 1;
   return cycle * pairsPerCycle + pairInCycle;
}
}

CTournament::CTournament() : m_sStartPositions(nullptr), m_nThreadID(0)
{
   m_nType = 1;
   m_nTC = 1;
   m_nBase = 1000;
   m_nInc = 100;
   m_nRound = 0;
   m_nHash = 32;
   m_bPonder = false;
   m_bOwnBook = true;
   m_nVariant = 0;
   m_nNumStartPositions = 0;
   m_nNumEngines = 0;
   m_nWastedTime = 0;
   m_nAdjMateScore = 1000;
   m_nAdjMateMoves = 12;
   m_nAdjDrawMoves = 150;
   m_nRandomize = 0;
   m_nOpeningSeed = 0;

   m_pWnd = nullptr;

   m_bRunning = false;
}

CTournament::~CTournament()
= default;

void CTournament::Start()
{
   m_bRunning = true;
   if (m_nNumEngines == 0) return;

   long nBase[2], nInc[2];
   double fTimeLeft[2];
   int nSTM = WHITE;
   CTimer t;
   CString sMove;
   CString sMoveList;
   int nMoveNum;

   long nStartTime = clock();

   TResult tResult{};
   tResult.dTotalTime[WHITE] = tResult.dTotalTime[BLACK] = 0.0;
   tResult.dTotalSearches[WHITE] = tResult.dTotalSearches[BLACK] = 0.0;
   tResult.nTotalDepth[WHITE] = tResult.nTotalDepth[BLACK] = 0;
   tResult.nTotalDepthCount[WHITE] = tResult.nTotalDepthCount[BLACK] = 0;
   tResult.nTotalNPS[WHITE] = tResult.nTotalNPS[BLACK] = 0;
   tResult.nTotalNPSCount[WHITE] = tResult.nTotalNPSCount[BLACK] = 0;

   const long scoreHistoryLength = MAX(1, m_nAdjMateMoves);
   long* nPrevScores[2];
   nPrevScores[WHITE] = new long[scoreHistoryLength]{};
   nPrevScores[BLACK] = new long[scoreHistoryLength]{};
   long nPrevScoresHead[2] = { 0, 0 };

   TBoard b;
   TUndoMove u;
   TMove m;
   int nWhite, nBlack;

   if (m_nType == 0)
   {
      int nOpponent = 1 + m_nRound % (m_nNumEngines - 1);
      int nColour = m_nRound / (m_nNumEngines - 1) & 1;
      nWhite = nColour == WHITE ? 0 : nOpponent;
      nBlack = nColour == BLACK ? 0 : nOpponent;

      if (m_nVariant == VARIANT_STD)
      {
         const unsigned long long pairing = static_cast<unsigned long long>(m_nRound) /
            (2ULL * (m_nNumEngines - 1));
         const int idx = PairedOpeningIndex(pairing, m_nNumStartPositions, m_nRandomize != 0, m_nOpeningSeed);
         LoadFEN(&b, m_sStartPositions[idx]);
      }
      else
      {
         CreateStartingPosition(&b, m_nVariant);
      }
   }
   else if (m_nType == 1)
   {
      nWhite = m_nRound / (m_nNumEngines - 1) % m_nNumEngines;
      int i = m_nRound % (m_nNumEngines - 1);
      nBlack = i >= nWhite ? 1 + i : i;

      if (m_nVariant == VARIANT_STD)
      {
         const unsigned long long pairing = RoundRobinPairing(m_nRound, m_nNumEngines, nWhite, nBlack);
         const int idx = PairedOpeningIndex(pairing, m_nNumStartPositions, m_nRandomize != 0, m_nOpeningSeed);
         LoadFEN(&b, m_sStartPositions[idx]);
      }
      else
      {
         CreateStartingPosition(&b, m_nVariant);
      }
   }

   ASSERT(nWhite >= 0 && nWhite < m_nNumEngines);
   ASSERT(nBlack >= 0 && nBlack < m_nNumEngines);
   m_CurrEngines[WHITE] = m_Engines[nWhite];
   m_CurrEngines[BLACK] = m_Engines[nBlack];
   tResult.nWhite = nWhite;
   tResult.nBlack = nBlack;

   m_CurrEngines[WHITE].m_nHash = m_nHash;
   m_CurrEngines[WHITE].m_bPonder = m_bPonder;
   m_CurrEngines[WHITE].m_bOwnBook = m_bOwnBook;
   m_CurrEngines[WHITE].m_nVariant = m_nVariant;
   if (!m_CurrEngines[WHITE].Init())
   {
      tResult.nResult = WHITE_ILLEGAL;
      tResult.sSAN = new char[1];
      tResult.sSAN[0] = 0;
      m_bRunning = false;
      delete[] nPrevScores[WHITE];
      delete[] nPrevScores[BLACK];
      if (m_pWnd && m_pWnd->m_hWnd)
         m_pWnd->SendMessage(GAME_DONE, reinterpret_cast<WPARAM>(&tResult), m_nThreadID);
      return;
   }
   m_CurrEngines[BLACK].m_nHash = m_nHash;
   m_CurrEngines[BLACK].m_bPonder = m_bPonder;
   m_CurrEngines[BLACK].m_bOwnBook = m_bOwnBook;
   m_CurrEngines[BLACK].m_nVariant = m_nVariant;
   if (!m_CurrEngines[BLACK].Init())
   {
      m_CurrEngines[WHITE].Quit();
      tResult.nResult = BLACK_ILLEGAL;
      tResult.sSAN = new char[1];
      tResult.sSAN[0] = 0;
      m_bRunning = false;
      delete[] nPrevScores[WHITE];
      delete[] nPrevScores[BLACK];
      if (m_pWnd && m_pWnd->m_hWnd)
         m_pWnd->SendMessage(GAME_DONE, reinterpret_cast<WPARAM>(&tResult), m_nThreadID);
      return;
   }

   m_CurrEngines[WHITE].NewGame();
   m_CurrEngines[BLACK].NewGame();

   nBase[WHITE] = m_nBase;
   nBase[BLACK] = m_nBase;
   nInc[WHITE] = m_nInc;
   nInc[BLACK] = m_nInc;

   fTimeLeft[WHITE] = nBase[WHITE];
   fTimeLeft[BLACK] = nBase[BLACK];
   nMoveNum = 1;

   char sStartingPositionFEN[100];
   Board2FEN(&b, sStartingPositionFEN);
   if (g_bFullPGN)
   {
      strcpy(tResult.sFEN, sStartingPositionFEN);
   }
   g_nGameHalfMoveNum[m_nThreadID] = 0;
   nSTM = b.nSideToMove;

   CStringA sGameMoves;
   m_nWastedTime += clock() - nStartTime;

   while (true)
   {
      long nDepth = 0;
      long nNPS = 0;
      long nScore = 0;

      if (!m_bRunning) break;

      CString sLine = m_CurrEngines[nSTM].Search(CString(sStartingPositionFEN), sMoveList, m_nTC,
         static_cast<long>(fTimeLeft[WHITE]),
         static_cast<long>(fTimeLeft[BLACK]), nInc[WHITE], nInc[BLACK],
         static_cast<long>(fTimeLeft[nSTM]), &nDepth, &nNPS, &nScore, &t);
      CStringArray sWords;
      CEngine::GetWords(sLine, &sWords);
      if (sWords.GetCount() > 1)
         sMove = sWords[1];
      if (m_nTC != TC_FIXED_TPM)
      {
         fTimeLeft[nSTM] -= t.GetMS();
      }
      tResult.dTotalTime[nSTM] += t.GetMS();
      tResult.dTotalSearches[nSTM]++;
      if (nDepth > 0)
      {
         tResult.nTotalDepth[nSTM] += nDepth;
         tResult.nTotalDepthCount[nSTM]++;
      }
      if (nNPS > 0)
      {
         tResult.nTotalNPS[nSTM] += nNPS;
         tResult.nTotalNPSCount[nSTM]++;
      }

      Log("Took %.1lfms, Left[%c] = %.1lfms", t.GetMS(), nSTM == WHITE ? 'W' : 'B', fTimeLeft[nSTM]);

      if (sLine.IsEmpty() || fTimeLeft[nSTM] <= 0)
      {
         Log("RESULT: TIMEOUT");
         tResult.nResult = nSTM == WHITE ? WHITE_TIMEOUT : BLACK_TIMEOUT;
         break;
      }

      if (m_nTC == TC_BLITZ)
      {
         fTimeLeft[nSTM] += nInc[nSTM];
         if (nInc[nSTM] > 0) Log("Add inc %ldms, Left[%c] = %.1lfms", nInc[nSTM], nSTM == WHITE ? 'W' : 'B',
            fTimeLeft[nSTM]);
      }
      else if (m_nTC == TC_TOURNAMENT && nSTM == BLACK)
      {
         nInc[WHITE]--;
         if (nInc[WHITE] == 0)
         {
            nInc[WHITE] = m_nInc;
            fTimeLeft[WHITE] = nBase[WHITE];
            fTimeLeft[BLACK] = nBase[BLACK];
         }
      }

      if (sWords.GetCount() < 2 || !Move2Coord(&m, &b, sMove.GetBuffer(), m_nVariant))
      {
         Log("RESULT: MALFORMED OR MISSING BESTMOVE");
         tResult.nResult = nSTM == WHITE ? WHITE_ILLEGAL : BLACK_ILLEGAL;
         if (g_bDumpIllegalMoves)
            DumpIllegalMove(&b, sStartingPositionFEN, sMoveList, &m_CurrEngines[nSTM]);
         break;
      }
      if (!IsValidMoveQuick(m, &b, m_nVariant))
      {
         Log("RESULT: ILLEGAL MOVE");
         tResult.nResult = nSTM == WHITE ? WHITE_ILLEGAL : BLACK_ILLEGAL;
         if (g_bDumpIllegalMoves)
            DumpIllegalMove(&b, sStartingPositionFEN, sMoveList, &m_CurrEngines[nSTM]);
         break;
      }

      if (g_bFullPGN)
      {
         char* sSAN = GetNotation(&b, m);
         sGameMoves.AppendChar(' ');
         if (nSTM == WHITE)
         {
            char sMoveNum[32];
            sprintf_s(sMoveNum, "%d. ", nMoveNum);
            sGameMoves.Append(sMoveNum);
            sGameMoves.Append(sSAN);
         }
         else
         {
            if (sGameMoves.GetLength() == 1)
            {
               char sMoveNum[32];
               sprintf_s(sMoveNum, "%d.. ", nMoveNum);
               sGameMoves.Append(sMoveNum);
            }
            sGameMoves.Append(sSAN);
         }
         delete[] sSAN;
      }

      if (g_nGameHalfMoveNum[m_nThreadID] >= m_nAdjDrawMoves * 2)
      {
         Log("RESULT: DRAW BY ADJUDICATION (%d moves)", g_nGameHalfMoveNum[m_nThreadID] / 2);
         tResult.nResult = ADJ_DRAW;
         break;
      }
      nPrevScores[nSTM][nPrevScoresHead[nSTM]++] = nScore;
      if (nPrevScoresHead[nSTM] == m_nAdjMateMoves)
         nPrevScoresHead[nSTM] = 0;
      if (nScore > m_nAdjMateScore)
      {
         int i;
         bool bOK = true;
         for (i = 0; i < m_nAdjMateMoves; i++)
         {
            if (nPrevScores[nSTM][i] < m_nAdjMateScore) bOK = false;
            if (nPrevScores[OPP(nSTM)][i] > -m_nAdjMateScore) bOK = false;
         }
         if (bOK)
         {
            Log("RESULT: MATE BY ADJUDICATION");
            tResult.nResult = nSTM == WHITE ? ADJ_WHITE_MATES : ADJ_BLACK_MATES;
            break;
         }
      }
      else if (nScore < -m_nAdjMateScore)
      {
         int i;
         bool bOK = true;
         for (i = 0; i < m_nAdjMateMoves; i++)
         {
            if (nPrevScores[nSTM][i] > -m_nAdjMateScore) bOK = false;
            if (nPrevScores[OPP(nSTM)][i] < m_nAdjMateScore) bOK = false;
         }
         if (bOK)
         {
            Log("RESULT: MATE BY ADJUDICATION");
            tResult.nResult = nSTM == BLACK ? ADJ_WHITE_MATES : ADJ_BLACK_MATES;
            break;
         }
      }

      sMoveList.AppendFormat(_T(" %s"), sMove.MakeLower());
      if (nSTM == WHITE)
      {
      }
      else
      {
         nMoveNum++;
      }

      MakeMove2(&b, m, &u, 0, m_nVariant);

      if (g_bFullPGN && (b.nSideToMove == WHITE &&
         IsSquareAttackedBy(&b, GetBit(b.bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         || (b.nSideToMove == BLACK && IsSquareAttackedBy(&b, GetBit(b.bbPieces[BLACK][PIECE_KING - 1]), WHITE)))
      {
         sGameMoves.AppendChar('+');
      }

      if (IsInsufficientMaterial(&b))
      {
         Log("RESULT: DRAW BY INSUFFICIENT MATERIAL");
         tResult.nResult = INSUF_MAT;
         break;
      }
      if (b.nFiftyMoveCount >= 100)
      {
         Log("RESULT: DRAW BY FIFTY MOVES");
         tResult.nResult = FIFTY_MOVE;
         break;
      }
      if (IsRepetition(&b, m_nThreadID))
      {
         Log("RESULT: DRAW BY REPETITION");
         tResult.nResult = REPETITION;
         break;
      }
      if (!IsAnyLegalMoves(&b, m_nVariant))
      {
         if (IsSTMInCheck(&b))
         {
            Log("RESULT: MATE");
            tResult.nResult = nSTM == WHITE ? WHITE_MATES : BLACK_MATES;
            break;
         }
         Log("RESULT: DRAW BY STALEMATE");
         tResult.nResult = STALEMATE;
         break;
      }

      nSTM ^= 1;
      g_nGameHalfMoveNum[m_nThreadID]++;
   }

   nStartTime = clock();

   tResult.sSAN = new char[sGameMoves.GetLength() + 1];
   strcpy_s(tResult.sSAN, sGameMoves.GetLength() + 1, sGameMoves);

   m_CurrEngines[WHITE].Quit();
   m_CurrEngines[BLACK].Quit();

   delete[] nPrevScores[WHITE];
   delete[] nPrevScores[BLACK];

   m_nWastedTime += clock() - nStartTime;

   m_bRunning = false;

   if (m_pWnd && m_pWnd->m_hWnd)
      m_pWnd->SendMessage(GAME_DONE, reinterpret_cast<WPARAM>(&tResult), m_nThreadID);
}

void CTournament::DumpIllegalMove(const TBoard* b, char* sStartingPosition, CString sMoveList, CEngine* e)
{
   char name[100];
   const unsigned int suffix = std::uniform_int_distribution<unsigned int>()(RandomGenerator());
   _snprintf_s(name, sizeof(name), _TRUNCATE, "illegal_%s_%u", e->m_sName ? e->m_sName : "engine", suffix);
   FILE* fout = fopen(name, "wt");
   if (!fout) return;
   char fen[100];
   Board2FEN(b, fen);
   PrintBoard(b, fout);
   fprintf(fout, "Starting Position: %s\n", sStartingPosition);
   fprintf(fout, "Movelist: %s\n", sMoveList.GetBuffer());
   fprintf(fout, "Engine Output: %s\n", e->m_sLastOutput);

   fprintf(fout, "Test engine output with:\nposition fen %s moves%s\n", sStartingPosition, sMoveList.GetBuffer());

   fclose(fout);
}

void CTournament::Abort()
{
   m_bRunning = false;

   m_CurrEngines[WHITE].Quit();
   m_CurrEngines[BLACK].Quit();
}
