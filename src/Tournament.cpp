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

   m_pWnd = nullptr;

   m_bRunning = false;
}

CTournament::~CTournament()
= default;

void CTournament::Start()
{
   m_bRunning = true;
   if (m_nNumEngines == 0) return;

   srand(clock());

   long nBase[2], nInc[2];
   double fTimeLeft[2];
   int nSTM = WHITE;
   CTimer t;
   CString sMove;
   CString sMoveList;
   int nMoveNum;

   long nStartTime = clock();

   TResult tResult;
   tResult.dTotalTime[WHITE] = tResult.dTotalTime[BLACK] = 0.0;
   tResult.dTotalSearches[WHITE] = tResult.dTotalSearches[BLACK] = 0.0;
   tResult.nTotalDepth[WHITE] = tResult.nTotalDepth[BLACK] = 0;
   tResult.nTotalDepthCount[WHITE] = tResult.nTotalDepthCount[BLACK] = 0;
   tResult.nTotalNPS[WHITE] = tResult.nTotalNPS[BLACK] = 0;
   tResult.nTotalNPSCount[WHITE] = tResult.nTotalNPSCount[BLACK] = 0;

   long* nPrevScores[2];
   nPrevScores[WHITE] = new long[m_nAdjMateMoves];
   nPrevScores[BLACK] = new long[m_nAdjMateMoves];
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
         int idx;
         if (m_nRandomize) idx = rand() % m_nNumStartPositions;
         else idx = m_nRound / (2 * (m_nNumEngines - 1)) % m_nNumStartPositions;
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
         int idx;
         if (m_nRandomize) idx = rand() % m_nNumStartPositions;
         else idx = m_nRound % m_nNumStartPositions;
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
   m_CurrEngines[WHITE].Init();
   m_CurrEngines[BLACK].m_nHash = m_nHash;
   m_CurrEngines[BLACK].m_bPonder = m_bPonder;
   m_CurrEngines[BLACK].m_bOwnBook = m_bOwnBook;
   m_CurrEngines[BLACK].m_nVariant = m_nVariant;
   m_CurrEngines[BLACK].Init();

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

   char* sGameMoves;
   int nGameMovesLen = 0;
   int nGameMovesAlloc = 1024;
   sGameMoves = static_cast<char*>(malloc(sizeof(char) * nGameMovesAlloc));
   sGameMoves[0] = 0;
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

      if (fTimeLeft[nSTM] <= 0)
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

      if (!Move2Coord(&m, &b, sMove.GetBuffer(), m_nVariant))
      {
         ASSERT(false);
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
         if (nGameMovesLen + strlen(sSAN) + 7 >= nGameMovesAlloc)
         {
            nGameMovesAlloc *= 2;
            sGameMoves = static_cast<char*>(realloc(sGameMoves, sizeof(char) * nGameMovesAlloc));
         }
         sGameMoves[nGameMovesLen++] = ' ';
         if (nSTM == WHITE)
         {
            char sMoveNum[10];
            sprintf(sMoveNum, "%d. ", nMoveNum);
            strcpy(sGameMoves + nGameMovesLen, sMoveNum);
            nGameMovesLen += strlen(sMoveNum);
            strcpy(sGameMoves + nGameMovesLen, sSAN);
            nGameMovesLen += strlen(sSAN);
         }
         else
         {
            if (nGameMovesLen == 1)
            {
               char sMoveNum[10];
               sprintf(sMoveNum, "%d.. ", nMoveNum);
               strcpy(sGameMoves + nGameMovesLen, sMoveNum);
               nGameMovesLen += strlen(sMoveNum);
            }
            strcpy(sGameMoves + nGameMovesLen, sSAN);
            nGameMovesLen += strlen(sSAN);
         }
         sGameMoves[nGameMovesLen] = 0;
         delete sSAN;
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
         if (nGameMovesLen + 1 >= nGameMovesAlloc)
         {
            nGameMovesAlloc *= 2;
            sGameMoves = static_cast<char*>(realloc(sGameMoves, sizeof(char) * nGameMovesAlloc));
         }
         sGameMoves[nGameMovesLen++] = '+';
         sGameMoves[nGameMovesLen] = 0;
      }

      if (IsInsufficientMaterial(&b))
      {
         Log("RESULT: DRAW BY INSUFFICIENT MATERIAL");
         tResult.nResult = INSUF_MAT;
         break;
      }
      if (b.nFiftyMoveCount > 100)
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

   tResult.sSAN = sGameMoves;

   m_CurrEngines[WHITE].Quit();
   m_CurrEngines[BLACK].Quit();

   delete nPrevScores[WHITE];
   delete nPrevScores[BLACK];

   m_nWastedTime += clock() - nStartTime;

   m_bRunning = false;

   if (m_pWnd && m_pWnd->m_hWnd)
      m_pWnd->SendMessage(GAME_DONE, reinterpret_cast<WPARAM>(&tResult), m_nThreadID);
}

void CTournament::DumpIllegalMove(const TBoard* b, char* sStartingPosition, CString sMoveList, CEngine* e)
{
   char name[100];
   srand(clock());
   sprintf(name, "illegal_%s_%u", e->m_sName, rand());
   FILE* fout = fopen(name, "wt");
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
