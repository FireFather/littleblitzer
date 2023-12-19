#pragma once

#include "Engine.h"
#include "Board.h"

#define MAX_START_POSITIONS	32000     

#define TC_FIXED_TPM	0
#define TC_BLITZ		1
#define TC_TOURNAMENT	2

class CTournament
{
public:
   CTournament();
   ~CTournament();

   int m_nType;
   long m_nBase;
   long m_nInc;
   long m_nRound;
   long m_nHash;
   bool m_bPonder;
   bool m_bOwnBook;
   int m_nVariant;
   int m_nTC;

   int m_nNumStartPositions;
   char** m_sStartPositions;
   int m_nRandomize;

   long m_nAdjMateScore;
   long m_nAdjMateMoves;
   long m_nAdjDrawMoves;

   bool m_bRunning;

   long m_nWastedTime;

   long m_nNumEngines;
   CEngine m_Engines[100];
   CEngine m_CurrEngines[2];
   CWnd* m_pWnd;
   int m_nThreadID;

   void Start();
   static void DumpIllegalMove(const TBoard* b, char* sStartingPosition, CString sMoveList, CEngine* e);
   void Abort();
};
