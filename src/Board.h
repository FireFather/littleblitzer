#pragma once

#include <stdio.h>
#include "Common.h"
#include "Types.h"
#define MAX_FEN_LEN			500

enum { WHITE, BLACK };

#define OPP(c) ((c) ^ 1)				 

#define NO_CASTLE				0x00
#define WHITE_KING_CASTLE		0x01
#define WHITE_QUEEN_CASTLE		0x02
#define BLACK_KING_CASTLE		0x04
#define BLACK_QUEEN_CASTLE		0x08
#define WHITE_CANT_CASTLE		0xFC
#define BLACK_CANT_CASTLE		0xF3
#define WHITE_CAN_CASTLE		0x03
#define BLACK_CAN_CASTLE		0x0C
#define WHITE_HAS_CASTLED		0x10
#define BLACK_HAS_CASTLED		0x20
#define ALL_CASTLE_BITS			0x3F
#define CAN_CASTLE				0x0F

#define NULL_SQUARE	(-1)

using TBoard = CACHE_ALIGN struct
{
   BitBoard bbPieces[2][7];

   BitBoard bbRotate90;
   BitBoard bbRotate45L;
   BitBoard bbRotate45R;

   U8 nSideToMove;
   S8 nEPSquare;
   U8 nFiftyMoveCount;
   U8 nCastling;
   U64 nTransHash;

   U8 nPieces[64];
   U64 nPawnHash;
   U64 nMaterialHash;
   int nMaterial[2];
   int nPieceSquare[2];

   BitBoard g_bbCastles[64];
};

#define WHITE_HALF_BOARD		0x00000000FFFFFFFF
#define BLACK_HALF_BOARD		0xFFFFFFFF00000000

constexpr BitBoard g_bbMaskNotFileA = 0x7F7F7F7F7F7F7F7F;
constexpr BitBoard g_bbMaskNotFileH = 0xFEFEFEFEFEFEFEFE;
constexpr BitBoard g_bbMaskNotRank8 = 0x00FFFFFFFFFFFFFF;
constexpr BitBoard g_bbMaskNotRank1 = 0xFFFFFFFFFFFFFF00;
constexpr BitBoard g_bbMaskRank8 = 0xFF00000000000000;
constexpr BitBoard g_bbMaskRank7 = 0x00FF000000000000;
constexpr BitBoard g_bbMaskRank2 = 0x000000000000FF00;
constexpr BitBoard g_bbMaskRank1 = 0x00000000000000FF;

constexpr BitBoard g_bbMaskFile[8] = {
   0x0101010101010101, 0x0202020202020202, 0x0404040404040404, 0x0808080808080808,
   0x1010101010101010, 0x2020202020202020, 0x4040404040404040, 0x8080808080808080
};
constexpr BitBoard g_bbMaskRank[8] = {
   0x00000000000000FF, 0x000000000000FF00, 0x0000000000FF0000, 0x00000000FF000000,
   0x000000FF00000000, 0x0000FF0000000000, 0x00FF000000000000, 0xFF00000000000000
};

constexpr BitBoard g_bbMaskFileAH = 0x8181818181818181;

extern BitBoard g_bbMaskIsoPawns[8];

#define SQUARE_PIECE_MASK		0x07
#define SQUARE_COLOR_MASK		0x18
#define SQUARE_EMPTY			0x00
#define PIECE_WHITE				0x08
#define PIECE_BLACK				0x10

enum
{
   PIECE_NONE,
   PIECE_PAWN,
   PIECE_KNIGHT,
   PIECE_BISHOP,
   PIECE_ROOK,
   PIECE_QUEEN,
   PIECE_KING,
   PIECE_ALL
};

#define WHITE_PAWN				(PIECE_WHITE | PIECE_PAWN)
#define WHITE_KNIGHT			(PIECE_WHITE | PIECE_KNIGHT)
#define WHITE_BISHOP			(PIECE_WHITE | PIECE_BISHOP)
#define WHITE_ROOK				(PIECE_WHITE | PIECE_ROOK)
#define WHITE_QUEEN				(PIECE_WHITE | PIECE_QUEEN)
#define WHITE_KING				(PIECE_WHITE | PIECE_KING)
#define BLACK_PAWN				(PIECE_BLACK | PIECE_PAWN)
#define BLACK_KNIGHT			(PIECE_BLACK | PIECE_KNIGHT)
#define BLACK_BISHOP			(PIECE_BLACK | PIECE_BISHOP)
#define BLACK_ROOK				(PIECE_BLACK | PIECE_ROOK)
#define BLACK_QUEEN				(PIECE_BLACK | PIECE_QUEEN)
#define BLACK_KING				(PIECE_BLACK | PIECE_KING)

enum {
   H1, G1, F1, E1, D1, C1, B1, A1,
   H2, G2, F2, E2, D2, C2, B2, A2,
   H3, G3, F3, E3, D3, C3, B3, A3,
   H4, G4, F4, E4, D4, C4, B4, A4,
   H5, G5, F5, E5, D5, C5, B5, A5,
   H6, G6, F6, E6, D6, C6, B6, A6,
   H7, G7, F7, E7, D7, C7, B7, A7,
   H8, G8, F8, E8, D8, C8, B8, A8
};

#define LIGHT_SQUARES	0xAA55AA55AA55AA55	      
#define DARK_SQUARES	0x55AA55AA55AA55AA	      

#define LIGHT_WHITE_PROM_FILES	0xAA
#define DARK_WHITE_PROM_FILES	0x55
#define LIGHT_BLACK_PROM_FILES	0x55
#define DARK_BLACK_PROM_FILES	0xAA

extern CACHE_ALIGN BitBoard bit[64];

extern CACHE_ALIGN BitBoard g_bbPawnAttacks[2][64];
extern CACHE_ALIGN BitBoard g_bbWhitePawnAttacks[64];
extern CACHE_ALIGN BitBoard g_bbBlackPawnAttacks[64];
extern CACHE_ALIGN BitBoard g_nnWhiteEPAttacks[64];
extern CACHE_ALIGN BitBoard g_nnBlackEPAttacks[64];
extern CACHE_ALIGN BitBoard g_bbKnightAttacks[64];
extern CACHE_ALIGN BitBoard g_bbKingAttacks[64];

extern CACHE_ALIGN BitBoard g_bbRankAttacks[64][256];
extern CACHE_ALIGN BitBoard g_bbFileAttacks[64][256];
extern CACHE_ALIGN BitBoard g_bbDiagA8H1Attacks[64][256];
extern CACHE_ALIGN BitBoard g_bbDiagH8A1Attacks[64][256];
extern CACHE_ALIGN int g_nR45LMask[64];
extern CACHE_ALIGN int g_nR45RMask[64];
extern CACHE_ALIGN int g_nDirections[64][64];
extern CACHE_ALIGN BitBoard g_bbDir8Attacks[64];
extern CACHE_ALIGN BitBoard g_bbDir7Attacks[64];
extern CACHE_ALIGN BitBoard g_bbDirM1Attacks[64];
extern CACHE_ALIGN BitBoard g_bbDirM9Attacks[64];
extern CACHE_ALIGN BitBoard g_bbDirM8Attacks[64];
extern CACHE_ALIGN BitBoard g_bbDirM7Attacks[64];
extern CACHE_ALIGN BitBoard g_bbDir1Attacks[64];
extern CACHE_ALIGN BitBoard g_bbDir9Attacks[64];

extern CACHE_ALIGN BitBoard g_bbBetween[64][64];

extern CACHE_ALIGN BitBoard g_bbPassedPawnMask[2][64];

#define FILE(sq) ((sq)&7)					        		 
#define RANK(sq) ((int)((sq)>>3))			     					 

#define FLIP_RANK(sq)	((7-RANK(sq))*8+FILE(sq))

#define SQ_COLOR(sq)	((RANK(sq)&1)^(FILE(sq)&1))

#define FILE_A	7
#define FILE_B	6
#define FILE_C	5
#define FILE_D	4
#define FILE_E	3
#define FILE_F	2
#define FILE_G	1
#define FILE_H	0

#define RANK_1	0
#define RANK_2	1
#define RANK_3	2
#define RANK_4	3
#define RANK_5	4
#define RANK_6	5
#define RANK_7	6
#define RANK_8	7

const CACHE_ALIGN unsigned char g_nRotate90Map[64] = {
   7, 15, 23, 31, 39, 47, 55, 63,
   6, 14, 22, 30, 38, 46, 54, 62,
   5, 13, 21, 29, 37, 45, 53, 61,
   4, 12, 20, 28, 36, 44, 52, 60,
   3, 11, 19, 27, 35, 43, 51, 59,
   2, 10, 18, 26, 34, 42, 50, 58,
   1, 9, 17, 25, 33, 41, 49, 57,
   0, 8, 16, 24, 32, 40, 48, 56
};

CACHE_ALIGN constexpr unsigned char g_nRotate45LMap[64] = {
   A8, B1, C2, D3, E4, F5, G6, H7,
   A7, B8, C1, D2, E3, F4, G5, H6,
   A6, B7, C8, D1, E2, F3, G4, H5,
   A5, B6, C7, D8, E1, F2, G3, H4,
   A4, B5, C6, D7, E8, F1, G2, H3,
   A3, B4, C5, D6, E7, F8, G1, H2,
   A2, B3, C4, D5, E6, F7, G8, H1,
   A1, B2, C3, D4, E5, F6, G7, H8
};

CACHE_ALIGN constexpr unsigned char g_nRotate45RMap[64] = {
   A8, B7, C6, D5, E4, F3, G2, H1,
   A7, B6, C5, D4, E3, F2, G1, H8,
   A6, B5, C4, D3, E2, F1, G8, H7,
   A5, B4, C3, D2, E1, F8, G7, H6,
   A4, B3, C2, D1, E8, F7, G6, H5,
   A3, B2, C1, D8, E7, F6, G5, H4,
   A2, B1, C8, D7, E6, F5, G4, H3,
   A1, B8, C7, D6, E5, F4, G3, H2
};

const CACHE_ALIGN unsigned char g_nRankShift[64] = {
   0, 0, 0, 0, 0, 0, 0, 0,
   8, 8, 8, 8, 8, 8, 8, 8,
   16, 16, 16, 16, 16, 16, 16, 16,
   24, 24, 24, 24, 24, 24, 24, 24,
   32, 32, 32, 32, 32, 32, 32, 32,
   40, 40, 40, 40, 40, 40, 40, 40,
   48, 48, 48, 48, 48, 48, 48, 48,
   56, 56, 56, 56, 56, 56, 56, 56
};

const CACHE_ALIGN unsigned char g_nR90RankShift[64] = {
   0, 8, 16, 24, 32, 40, 48, 56,
   0, 8, 16, 24, 32, 40, 48, 56,
   0, 8, 16, 24, 32, 40, 48, 56,
   0, 8, 16, 24, 32, 40, 48, 56,
   0, 8, 16, 24, 32, 40, 48, 56,
   0, 8, 16, 24, 32, 40, 48, 56,
   0, 8, 16, 24, 32, 40, 48, 56,
   0, 8, 16, 24, 32, 40, 48, 56
};

const CACHE_ALIGN unsigned char g_nR45LShift[64] = {
   56, 0, 8, 16, 24, 32, 40, 48,
   49, 56, 0, 8, 16, 24, 32, 40,
   42, 49, 56, 0, 8, 16, 24, 32,
   35, 42, 49, 56, 0, 8, 16, 24,
   28, 35, 42, 49, 56, 0, 8, 16,
   21, 28, 35, 42, 49, 56, 0, 8,
   14, 21, 28, 35, 42, 49, 56, 0,
   7, 14, 21, 28, 35, 42, 49, 56
};

const CACHE_ALIGN unsigned char g_nR45RShift[64] = {
   63, 54, 45, 36, 27, 18, 9, 0,
   54, 45, 36, 27, 18, 9, 0, 56,
   45, 36, 27, 18, 9, 0, 56, 48,
   36, 27, 18, 9, 0, 56, 48, 40,
   27, 18, 9, 0, 56, 48, 40, 32,
   18, 9, 0, 56, 48, 40, 32, 24,
   9, 0, 56, 48, 40, 32, 24, 16,
   0, 56, 48, 40, 32, 24, 16, 8
};

const unsigned char g_nLengthA8H1diagonals[64] = {
   8, 7, 6, 5, 4, 3, 2, 1,
   7, 8, 7, 6, 5, 4, 3, 2,
   6, 7, 8, 7, 6, 5, 4, 3,
   5, 6, 7, 8, 7, 6, 5, 4,
   4, 5, 6, 7, 8, 7, 6, 5,
   3, 4, 5, 6, 7, 8, 7, 6,
   2, 3, 4, 5, 6, 7, 8, 7,
   1, 2, 3, 4, 5, 6, 7, 8
};

const unsigned char g_nLengthH8A1diagonals[64] = {
   1, 2, 3, 4, 5, 6, 7, 8,
   2, 3, 4, 5, 6, 7, 8, 7,
   3, 4, 5, 6, 7, 8, 7, 6,
   4, 5, 6, 7, 8, 7, 6, 5,
   5, 6, 7, 8, 7, 6, 5, 4,
   6, 7, 8, 7, 6, 5, 4, 3,
   7, 8, 7, 6, 5, 4, 3, 2,
   8, 7, 6, 5, 4, 3, 2, 1
};

const unsigned char g_nPushedPawnExt[64] = {
   0, 0, 0, 0, 0, 0, 0, 0,
   1, 1, 1, 1, 1, 1, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   1, 1, 1, 1, 1, 1, 1, 1,
   0, 0, 0, 0, 0, 0, 0, 0
};

constexpr char g_nPieceChars[23] = {
   ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', 'P', 'N', 'B', 'R', 'Q', 'K', ' ', ' ', 'p', 'n', 'b', 'r', 'q', 'k'
};

extern CACHE_ALIGN int g_nPieceValues[23];

extern CACHE_ALIGN int g_nDistance[64][64];

extern U64 g_nRepetitionHashes[600][MAX_THREADS];
extern int g_nRepetitionListHead[MAX_THREADS];
extern int g_nGameHalfMoveNum[MAX_THREADS];

#define SCORE_PAWN				100
#define SCORE_KNIGHT			325    
#define SCORE_BISHOP			325
#define SCORE_ROOK				500
#define SCORE_QUEEN				975    
#define SCORE_KING				0
#define MAX_PIECE_SCORE			(8*SCORE_PAWN + 2*SCORE_KNIGHT + 2*SCORE_BISHOP + 2*SCORE_ROOK + SCORE_QUEEN)

#ifdef CACHE_MOVES

typedef struct {
   U64 nRookHash;
   U64 nBishopHash;
   BitBoard bbRookMoves;
   BitBoard bbBishopMoves;
} TMovesCache;
extern TMovesCache g_tMovesCache[64];

#define ROOK_MOVES(sq)		( ROOK_MOVES_FN(b,sq) )
#define BISHOP_MOVES(sq)	( BISHOP_MOVES_FN(b,sq) )
#define QUEEN_MOVES(sq)		(ROOK_MOVES(sq) | BISHOP_MOVES(sq))

__forceinline U64 ROOK_MOVES_FN(TBoard* b, int sq) {
   BitBoard bb;
   if (g_tMovesCache[sq].nRookHash == b->nTransHash) {
      bb = g_tMovesCache[sq].bbRookMoves;
   }
   else {
      g_tMovesCache[sq].nRookHash = b->nTransHash;
      bb = g_bbRankAttacks[sq][((b->bbPieces[BLACK][PIECE_ALL - 1] | b->bbPieces[WHITE][PIECE_ALL - 1]) >> g_nRankShift[sq]) & 0xFF] | g_bbFileAttacks[sq][(b->bbRotate90 >> g_nR90RankShift[sq]) & 0xFF];
      g_tMovesCache[sq].bbRookMoves = bb;
   }
   return bb;
}

__forceinline U64 BISHOP_MOVES_FN(TBoard* b, int sq) {
   BitBoard bb;
   if (g_tMovesCache[sq].nBishopHash == b->nTransHash) {
      bb = g_tMovesCache[sq].bbBishopMoves;
   }
   else {
      g_tMovesCache[sq].nBishopHash = b->nTransHash;
      bb = g_bbDiagA8H1Attacks[sq][(b->bbRotate45L >> g_nR45LShift[sq]) & g_nR45LMask[sq]] | g_bbDiagH8A1Attacks[sq][(b->bbRotate45R >> g_nR45RShift[sq]) & g_nR45RMask[sq]];
      g_tMovesCache[sq].bbBishopMoves = bb;
   }
   return bb;
}

#else

#define ROOK_MOVES(sq) ( \
		g_bbRankAttacks[sq][((b->bbPieces[BLACK][PIECE_ALL-1] | b->bbPieces[WHITE][PIECE_ALL-1]) >> g_nRankShift[sq]) & 0xFF]	\
		| g_bbFileAttacks[sq][(b->bbRotate90 >> g_nR90RankShift[sq]) & 0xFF]				\
	)

#define BISHOP_MOVES(sq) ( \
		g_bbDiagA8H1Attacks[sq][(b->bbRotate45L >> g_nR45LShift[sq]) & g_nR45LMask[sq]]		\
		| g_bbDiagH8A1Attacks[sq][(b->bbRotate45R >> g_nR45RShift[sq]) & g_nR45RMask[sq]]	\
	)
#define QUEEN_MOVES(sq)		(ROOK_MOVES(sq) | BISHOP_MOVES(sq))

#endif

void CreateStartingPosition(TBoard* b, int nVariant);
void PrintBoard(const TBoard* b, FILE* f);
void PrintBoardNoEval(TBoard* b, FILE* f);
void ZeroBoard(TBoard* b);
int IsSquareAttackedBy(const TBoard* b, int sq, int color);
int IsSTMInCheck(const TBoard* b);
int CountSquareAttackedBy(const TBoard* b, int sq, int color, int* nNumAttacks);
BitBoard SquareAttacks(const TBoard* b, int sq);
void ValidateBoard(const TBoard* b);
void CalculateHashValues(TBoard* b);
void FlipBoard(TBoard* b);
void FlopBoard(TBoard* b);
void Board2FEN(const TBoard* b, char* sFEN);
void SetBitBoards(TBoard* b);
bool IsInsufficientMaterial(const TBoard* b);
bool IsRepetition(const TBoard* b, int nThreadID);
void LoadFEN(TBoard* b, const char a_sFEN[]);
void InitialiseArrays();
