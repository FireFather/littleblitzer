#pragma once

#include "Board.h"

#define MAX_MOVES	218

using TMove = uint32_t;

#define GET_MOVE_FROM(m)		(int)(((m)&0x0FC0)>>6)
#define GET_MOVE_TO(m)			(int)((m)&0x003F)
#define GET_MOVE_TYPE(m)		(int)(((m)&0x3000)>>12)
#define GET_MOVE_PROM(m)		(int)(((m)&0x1C000)>>14)
#define GET_MOVE_MOVED(m)		(int)(((m)&0xE0000)>>17)
#define GET_MOVE_CAPTURED(m)	(int)(((m)&0x700000)>>20)
#define MOVE_NULL		0

#define SET_MOVE(m,fr,to,ty,pr,mo,ca)		\
	((m) = (TMove)(to)							\
		| (((TMove)(fr))<<6)					\
		| (((TMove)(ty))<<12)					\
		| (((TMove)(pr))<<14)					\
		| (((TMove)(mo))<<17)					\
		| (((TMove)(ca))<<20))

#define MOVE_TYPE_NORMAL	0
#define MOVE_TYPE_CASTLE	1
#define MOVE_TYPE_PROM		2
#define MOVE_TYPE_EP		3

#define ASSERT_RANGE(v,a,b) ASSERT((v)>=(a) && (v)<=(b))

#define ASSERT_MOVE(m)								\
		ASSERT_RANGE(GET_MOVE_MOVED(m),    1, 6);	\
		ASSERT_RANGE(GET_MOVE_FROM(m),     0, 63);	\
		ASSERT_RANGE(GET_MOVE_TO(m),       0, 63);	\
		ASSERT_RANGE(GET_MOVE_TYPE(m),     0, 3);	\
		ASSERT_RANGE(GET_MOVE_CAPTURED(m), 0, 6);

using TMoveList = struct
{
   TMove tMoves[MAX_MOVES];
   int nScores[MAX_MOVES];
   TMove tMovesSearched[MAX_MOVES];
   TMove tHashMove;
   TMove tTriedKiller1;
   TMove tTriedKiller2;
   int nNumMoves;
   int nNumGoodMoves;
   int nNumBadMoves;
   TMove* pCurrentMove;
   int* pCurrentScore;
   int nStateOfGeneration;
   int nNumUsedMoves;
};

#define MOVE_WHITE_ROOK_CASTLE_K		0x0000000000000005
#define MOVE_WHITE_ROOK_CASTLE_Q		0x0000000000000090
#define MOVE_BLACK_ROOK_CASTLE_K		0x0500000000000000
#define MOVE_BLACK_ROOK_CASTLE_Q		0x9000000000000000

#define MOVE_WHITE_ROOK_CASTLE_K_90		0x0000000000800080
#define MOVE_WHITE_ROOK_CASTLE_Q_90		0x8000008000000000
#define MOVE_BLACK_ROOK_CASTLE_K_90		0x0000000000010001
#define MOVE_BLACK_ROOK_CASTLE_Q_90		0x0100000100000000

#define MOVE_WHITE_ROOK_CASTLE_K_45L	0x8000000000002000	    
#define MOVE_WHITE_ROOK_CASTLE_Q_45L	0x0001000008000000	    
#define MOVE_BLACK_ROOK_CASTLE_K_45L	0x0000000000200080	    
#define MOVE_BLACK_ROOK_CASTLE_Q_45L	0x0100000800000000	    

#define MOVE_WHITE_ROOK_CASTLE_K_45R	0x8000200000000000	    
#define MOVE_WHITE_ROOK_CASTLE_Q_45R	0x0000000008000001	    
#define MOVE_BLACK_ROOK_CASTLE_K_45R	0x0020000000000080	    
#define MOVE_BLACK_ROOK_CASTLE_Q_45R	0x0000000800000100	    

#define MOVE_SCORE_WORST			(-32000)
#define MOVE_SCORE_CAPTURE_PENALTY	(-10000)
#define MOVE_SCORE_NONE				0
#define MOVE_SCORE_HASH				1		        
#define MOVE_SCORE_CAPTURE_BONUS	10000
#define MOVE_SCORE_PREV_BEST		10000
#define MOVE_SCORE_KILLER_2			(MOVE_HISTORY_SCORE_MAX + 10)
#define MOVE_SCORE_KILLER_1			(MOVE_HISTORY_SCORE_MAX + 20)

#define MG_HASH					1
#define MG_NO_HASH				2
#define MG_GENERATE_EVASIONS	3
#define MG_GENERATE_CAPTURES	4
#define MG_CAPTURES				5
#define MG_KILLERS_1			6
#define MG_KILLERS_2			7
#define MG_GENERATE_NONCAPS		8
#define MG_NONCAPS				9
#define MG_GET_LOSING_CAPTURES	10
#define MG_NEXT_LOSING_CAPTURE	11
#define MG_NEXT_MOVE			12
#define MG_DONE					13
#define MG_GEN_ROOT_MOVES		14

using TUndoMove = struct
{
   S8 nEPSquare;
   U8 nCastleFlags;
   int nCapturedPiece;
   U8 n50Moves;
   U64 nTransHash;
   U64 nPawnHash;
   U64 nMaterialHash;
   int nRepListHead;
   int nMaterial[2];
   U8 nInCheck;
   int nOrigRookSq;
};

void MakeMove2(TBoard* b, TMove m, TUndoMove* undo, int nThreadID, int nVariant);
bool IsValidMoveQuick(TMove m, const TBoard* b, int nVariant);
bool Move2Coord(TMove* m, const TBoard* b, const char* sMove, int nVariant);
void UnMakeMove(TBoard* b, TMove m, const TUndoMove* undo, int nThreadID, int nVariant);
char* GetNotation(const TBoard* b, TMove m);
void GameMoves2FEN(char* sMoves, char* sFEN);
