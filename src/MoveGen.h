#pragma once

#include "Board.h"
#include "Move.h"
int GenerateCaptureMoves(const TBoard* b, TMove* a_tMoveList);
int GenerateNonCaptureMoves(const TBoard* b, TMove* a_tMoveList, int nVariant);

int GenerateWhitePawnCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateWhiteKnightCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateWhiteKingCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateWhiteRookQueenCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateWhiteBishopQueenCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateWhitePawnNonCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateWhiteKnightNonCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateWhiteKingNonCaps(const TBoard* b, TMove* a_tMoveList, int nVariant);
int GenerateWhiteRookQueenNonCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateWhiteBishopQueenNonCaps(const TBoard* b, TMove* a_tMoveList);

int GenerateBlackPawnCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateBlackKnightCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateBlackKingCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateBlackRookQueenCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateBlackBishopQueenCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateBlackPawnNonCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateBlackKnightNonCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateBlackKingNonCaps(const TBoard* b, TMove* a_tMoveList, int nVariant);
int GenerateBlackRookQueenNonCaps(const TBoard* b, TMove* a_tMoveList);
int GenerateBlackBishopQueenNonCaps(const TBoard* b, TMove* a_tMoveList);

bool IsAnyLegalMoves(TBoard* b, int nVariant);
