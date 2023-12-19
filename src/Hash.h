#pragma once

#include <math.h>
#include "Move.h"
#define HASH_FIELDS_DEPTH			0x00000000000000FF
#define HASH_FIELDS_TIMESTAMP		0x0000000000000700
#define HASH_FIELDS_TYPE			0x0000000000001800
#define HASH_FIELDS_SCORE			0x000000001FFFE000
#define HASH_FIELDS_MOVE			0x000FFFFFE0000000
#define HASH_FIELDS_THREAT			0x0010000000000000
#define HASH_FIELDS_SINGLE			0x0020000000000000

#define HASH_SHIFT_DEPTH		0
#define HASH_SHIFT_TIMESTAMP	8
#define HASH_SHIFT_TYPE			11
#define HASH_SHIFT_SCORE		13
#define HASH_SHIFT_MOVE			29
#define HASH_SHIFT_THREAT		52
#define HASH_SHIFT_SINGLE		53

#define HASH_ENTRY_EXACT		1
#define HASH_ENTRY_ALPHA_CUT	2
#define HASH_ENTRY_BETA_CUT		3

#define NUM_TRANS_ENTRIES		4	                
using TTranspositionHash = struct
{
   U32 nKey[NUM_TRANS_ENTRIES];
   U64 nFields[NUM_TRANS_ENTRIES];
};

extern TTranspositionHash* g_aTransTable;
extern U64 g_nNumTransEntries;
#define GET_HASH_DEPTH(f)			(int)(((f)&HASH_FIELDS_DEPTH)>>HASH_SHIFT_DEPTH)
#define GET_HASH_TIMESTAMP(f)		(int)(((f)&HASH_FIELDS_TIMESTAMP)>>HASH_SHIFT_TIMESTAMP)
#define GET_HASH_TYPE(f)			(int)(((f)&HASH_FIELDS_TYPE)>>HASH_SHIFT_TYPE)
#define GET_HASH_SCORE(f)			(__int32)((((f)&HASH_FIELDS_SCORE)>>HASH_SHIFT_SCORE)-INFINITY)
#define GET_HASH_MOVE(f)			(__int32)(((f)&HASH_FIELDS_MOVE)>>HASH_SHIFT_MOVE)
#define GET_HASH_THREAT(f)			(__int32)(((f)&HASH_FIELDS_THREAT)>>HASH_SHIFT_THREAT)
#define GET_HASH_SINGLE(f)			(__int32)(((f)&HASH_FIELDS_SINGLE)>>HASH_SHIFT_SINGLE)

#define SET_HASH_TYPE(f,v)			((f) |= (((U64)(v))<<HASH_SHIFT_TYPE))
#define SET_HASH_DEPTH(f,v)			((f) |= ((U64)(v)))
#define SET_HASH_SCORE(f,v)			((f) |= (((U64)(v)+INFINITY)<<HASH_SHIFT_SCORE))
#define SET_HASH_TIMESTAMP(f,v)		((f) |= (((U64)(v))<<HASH_SHIFT_TIMESTAMP))
#define SET_HASH_MOVE(f,v)			((f) |= (((U64)(v))<<HASH_SHIFT_MOVE))
#define SET_HASH_THREAT(f,v)		((f) |= (((U64)(v))<<HASH_SHIFT_THREAT))
#define SET_HASH_SINGLE(f,v)		((f) |= (((U64)(v))<<HASH_SHIFT_SINGLE))

#define GET_HASH(sq, pc)			(g_nHashValues[sq][((pc)&SQUARE_COLOR_MASK)>>4][(pc)&SQUARE_PIECE_MASK])      

#define GET_HASH_COL(sq, col, pc)	(g_nHashValues[sq][col][pc])

extern U64 g_nHashValues[64][2][7];

extern unsigned char g_nHashWhiteKingCastle;
extern unsigned char g_nHashWhiteQueenCastle;
extern unsigned char g_nHashBlackKingCastle;
extern unsigned char g_nHashBlackQueenCastle;
extern unsigned char g_nHashEPMove[64];
extern unsigned char g_nHash50Move[100];
extern unsigned char g_nHashWhiteToMove;

using TPawnHash = struct
{
   U64 nKey;
   S16 nScore;

   U8 nPawnCountW;
   U8 nPawnCountB;
   U8 nPassedW;
   U8 nPassedB;
   U8 nIsolated[2];
   U8 nDoubled[2];

   U8 nFilesUsed[2];

};

extern TPawnHash* g_aPawnTable;

extern U64 g_nNumPawnEntries;

using TEvalHash = struct
{
   U64 nKey;
   S16 nScore;
};

extern TEvalHash* g_aEvalTable;
extern U64 g_nNumEvalEntries;

void InitRand();
unsigned long Get32BitRandomNum(bool bReset = false);
U64 Get64BitRandomNum();
U64 GetRandomNum();
