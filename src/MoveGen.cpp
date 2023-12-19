#include "stdafx.h"
#include "MoveGen.h"
#include "Board.h"
#include "Move.h"
int GenerateCaptureMoves(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   if (b->nSideToMove == WHITE)
   {
      nNumMoves += GenerateWhitePawnCaps(b, a_tMoveList);
      nNumMoves += GenerateWhiteKnightCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateWhiteBishopQueenCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateWhiteRookQueenCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateWhiteKingCaps(b, a_tMoveList + nNumMoves);
   }
   else
   {
      nNumMoves += GenerateBlackPawnCaps(b, a_tMoveList);
      nNumMoves += GenerateBlackKnightCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateBlackBishopQueenCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateBlackRookQueenCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateBlackKingCaps(b, a_tMoveList + nNumMoves);
   }

   return nNumMoves;
}

int GenerateNonCaptureMoves(const TBoard* b, TMove* a_tMoveList, const int nVariant)
{
   int nNumMoves = 0;

   if (b->nSideToMove == WHITE)
   {
      nNumMoves += GenerateWhitePawnNonCaps(b, a_tMoveList);
      nNumMoves += GenerateWhiteKnightNonCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateWhiteBishopQueenNonCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateWhiteRookQueenNonCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateWhiteKingNonCaps(b, a_tMoveList + nNumMoves, nVariant);
   }
   else
   {
      nNumMoves += GenerateBlackPawnNonCaps(b, a_tMoveList);
      nNumMoves += GenerateBlackKnightNonCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateBlackBishopQueenNonCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateBlackRookQueenNonCaps(b, a_tMoveList + nNumMoves);
      nNumMoves += GenerateBlackKingNonCaps(b, a_tMoveList + nNumMoves, nVariant);
   }

   return nNumMoves;
}

int GenerateWhitePawnCaps(const TBoard* b, TMove* a_tMoveList)
{
   int to;
   int nNumMoves = 0;

   BitBoard bbAttacks = (b->bbPieces[WHITE][PIECE_PAWN - 1] & g_bbMaskNotFileA) << 9 & b->bbPieces[BLACK][PIECE_ALL -
      1];

   while (bbAttacks)
   {
      to = RemoveBit(bbAttacks);

      if (to >= H8)
      {
         SET_MOVE(a_tMoveList[0], to - 9, to, MOVE_TYPE_PROM, PIECE_QUEEN, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[1], to - 9, to, MOVE_TYPE_PROM, PIECE_ROOK, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[2], to - 9, to, MOVE_TYPE_PROM, PIECE_BISHOP, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[3], to - 9, to, MOVE_TYPE_PROM, PIECE_KNIGHT, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList += 4;
         nNumMoves += 4;
      }
      else
      {
         SET_MOVE(a_tMoveList[0], to - 9, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   bbAttacks = (b->bbPieces[WHITE][PIECE_PAWN - 1] & g_bbMaskNotFileH) << 7 & b->bbPieces[BLACK][PIECE_ALL - 1];

   while (bbAttacks)
   {
      to = RemoveBit(bbAttacks);

      if (to >= H8)
      {
         SET_MOVE(a_tMoveList[0], to - 7, to, MOVE_TYPE_PROM, PIECE_QUEEN, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[1], to - 7, to, MOVE_TYPE_PROM, PIECE_ROOK, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[2], to - 7, to, MOVE_TYPE_PROM, PIECE_BISHOP, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[3], to - 7, to, MOVE_TYPE_PROM, PIECE_KNIGHT, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList += 4;
         nNumMoves += 4;
      }
      else
      {
         SET_MOVE(a_tMoveList[0], to - 7, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   if (b->nEPSquare != NULL_SQUARE)
   {
      bbAttacks = g_nnWhiteEPAttacks[b->nEPSquare] & b->bbPieces[WHITE][PIECE_PAWN - 1];
      while (bbAttacks)
      {
         const int from = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, b->nEPSquare, MOVE_TYPE_EP, PIECE_NONE, PIECE_PAWN, PIECE_PAWN);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   bbAttacks = b->bbPieces[WHITE][PIECE_PAWN - 1] << 8 & g_bbMaskRank8 & ~(b->bbPieces[WHITE][PIECE_ALL - 1] | b->
      bbPieces[BLACK][PIECE_ALL - 1]);
   while (bbAttacks)
   {
      to = RemoveBit(bbAttacks);

      SET_MOVE(a_tMoveList[0], to - 8, to, MOVE_TYPE_PROM, PIECE_QUEEN, PIECE_PAWN, PIECE_NONE);
      SET_MOVE(a_tMoveList[1], to - 8, to, MOVE_TYPE_PROM, PIECE_ROOK, PIECE_PAWN, PIECE_NONE);
      SET_MOVE(a_tMoveList[2], to - 8, to, MOVE_TYPE_PROM, PIECE_BISHOP, PIECE_PAWN, PIECE_NONE);
      SET_MOVE(a_tMoveList[3], to - 8, to, MOVE_TYPE_PROM, PIECE_KNIGHT, PIECE_PAWN, PIECE_NONE);
      a_tMoveList += 4;
      nNumMoves += 4;
   }

   return nNumMoves;
}

int GenerateWhiteKnightCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[WHITE][PIECE_KNIGHT - 1];
   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = g_bbKnightAttacks[from] & b->bbPieces[BLACK][PIECE_ALL - 1];

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_KNIGHT,
            b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateWhiteKingCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   const int from = GetBit(b->bbPieces[WHITE][PIECE_KING - 1]);

   BitBoard bbAttacks = g_bbKingAttacks[from] & b->bbPieces[BLACK][PIECE_ALL - 1];

   while (bbAttacks)
   {
      const int to = RemoveBit(bbAttacks);

      SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_KING, b->nPieces[to] & SQUARE_PIECE_MASK);
      a_tMoveList++;
      nNumMoves++;
   }

   return nNumMoves;
}

int GenerateWhiteRookQueenCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[WHITE][PIECE_ROOK - 1] | b->bbPieces[WHITE][PIECE_QUEEN - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = ROOK_MOVES(from) & b->bbPieces[BLACK][PIECE_ALL - 1];

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, b->nPieces[from] & SQUARE_PIECE_MASK,
            b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateWhiteBishopQueenCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[WHITE][PIECE_BISHOP - 1] | b->bbPieces[WHITE][PIECE_QUEEN - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = BISHOP_MOVES(from) & b->bbPieces[BLACK][PIECE_ALL - 1];

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, b->nPieces[from] & SQUARE_PIECE_MASK,
            b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateWhitePawnNonCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbAttacks = b->bbPieces[WHITE][PIECE_PAWN - 1] << 8 & g_bbMaskNotRank8 & ~(b->bbPieces[BLACK][PIECE_ALL
      - 1] | b
      ->bbPieces[WHITE][PIECE_ALL - 1]);

   while (bbAttacks)
   {
      const int to = RemoveBit(bbAttacks);

      SET_MOVE(a_tMoveList[0], to - 8, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_PAWN, PIECE_NONE);
      a_tMoveList++;
      nNumMoves++;

      if (to >= H3 && to <= A3 && bit[to + 8] & ~(b->bbPieces[WHITE][PIECE_ALL - 1] | b->bbPieces[BLACK][PIECE_ALL -
         1]))
      {
         SET_MOVE(a_tMoveList[0], to - 8, to + 8, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_PAWN, PIECE_NONE);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateWhiteKnightNonCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[WHITE][PIECE_KNIGHT - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = g_bbKnightAttacks[from] & ~(b->bbPieces[BLACK][PIECE_ALL - 1] | b->bbPieces[WHITE][PIECE_ALL -
         1]);

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_KNIGHT, PIECE_NONE);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateWhiteKingNonCaps(const TBoard* b, TMove* a_tMoveList, const int nVariant)
{
   int nNumMoves = 0;

   const int from = GetBit(b->bbPieces[WHITE][PIECE_KING - 1]);

   BitBoard bbAttacks = g_bbKingAttacks[from] & ~(b->bbPieces[BLACK][PIECE_ALL - 1] | b->bbPieces[WHITE][PIECE_ALL -
      1]);

   while (bbAttacks)
   {
      const int to = RemoveBit(bbAttacks);

      SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_KING, PIECE_NONE);
      a_tMoveList++;
      nNumMoves++;
   }

   if (b->nCastling & WHITE_KING_CASTLE)
   {
      if (nVariant == VARIANT_960)
      {
         const int ksq1 = from;
         constexpr int ksq2 = G1;
         int rsq1;
         for (rsq1 = H1; rsq1 < A1; rsq1++) { if (b->nPieces[rsq1] == WHITE_ROOK) break; }
         constexpr int rsq2 = F1;
         bool illegal = false;
         for (int sq = MIN(ksq2, from); sq <= MAX(ksq2, from); sq++)
         {
            if (sq != rsq1 && sq != ksq1 && sq != ksq2 && b->nPieces[sq])
            {
               illegal = true;
               break;
            }
            if (IsSquareAttackedBy(b, sq, BLACK))
            {
               illegal = true;
               break;
            }
         }
         if (!illegal)
            for (int sq = MIN(rsq2, rsq1); sq <= MAX(rsq2, rsq1); sq++)
            {
               if (sq != rsq1 && sq != rsq2 && sq != ksq1 && b->nPieces[sq]) illegal = true;
            }
         if (rsq1 != rsq2 && b->nPieces[rsq2] && b->nPieces[rsq2] != WHITE_KING) illegal = true;
         if (from != ksq2 && b->nPieces[ksq2] &&
            (b->nPieces[ksq2] != WHITE_ROOK || (b->nPieces[ksq2] == WHITE_ROOK && ksq2 != rsq1)))
            illegal = true;
         if (!illegal)
         {
            SET_MOVE(a_tMoveList[0], from, G1, MOVE_TYPE_CASTLE, PIECE_NONE, PIECE_KING, PIECE_NONE);
            a_tMoveList++;
            nNumMoves++;
         }
      }
      else
      {
         if (!b->nPieces[F1] && !b->nPieces[G1])
         {
            if (!IsSquareAttackedBy(b, E1, BLACK) && !IsSquareAttackedBy(b, F1, BLACK) && !IsSquareAttackedBy(
               b, G1, BLACK))
            {
               SET_MOVE(a_tMoveList[0], E1, G1, MOVE_TYPE_CASTLE, PIECE_NONE, PIECE_KING, PIECE_NONE);
               a_tMoveList++;
               nNumMoves++;
            }
         }
      }
   }
   if (b->nCastling & WHITE_QUEEN_CASTLE)
   {
      if (nVariant == VARIANT_960)
      {
         const int ksq1 = from;
         constexpr int ksq2 = C1;
         int rsq1;
         for (rsq1 = A1; rsq1 > H1; rsq1--) { if (b->nPieces[rsq1] == WHITE_ROOK) break; }
         constexpr int rsq2 = D1;
         bool illegal = false;
         for (int sq = MIN(ksq2, from); sq <= MAX(ksq2, from); sq++)
         {
            if (sq != rsq1 && sq != ksq1 && sq != ksq2 && b->nPieces[sq])
            {
               illegal = true;
               break;
            }
            if (IsSquareAttackedBy(b, sq, BLACK))
            {
               illegal = true;
               break;
            }
         }
         if (!illegal)
            for (int sq = MIN(rsq2, rsq1); sq <= MAX(rsq2, rsq1); sq++)
            {
               if (sq != rsq1 && sq != rsq2 && sq != ksq1 && b->nPieces[sq]) illegal = true;
            }
         if (rsq1 != rsq2 && b->nPieces[rsq2] && b->nPieces[rsq2] != WHITE_KING) illegal = true;
         if (from != ksq2 && b->nPieces[ksq2] &&
            (b->nPieces[ksq2] != WHITE_ROOK || (b->nPieces[ksq2] == WHITE_ROOK && ksq2 != rsq1)))
            illegal = true;
         if (!illegal)
         {
            SET_MOVE(a_tMoveList[0], from, C1, MOVE_TYPE_CASTLE, PIECE_NONE, PIECE_KING, PIECE_NONE);
            nNumMoves++;
         }
      }
      else
      {
         if (!b->nPieces[D1] && !b->nPieces[C1] && !b->nPieces[B1])
         {
            if (!IsSquareAttackedBy(b, E1, BLACK) && !IsSquareAttackedBy(b, D1, BLACK) && !IsSquareAttackedBy(
               b, C1, BLACK))
            {
               SET_MOVE(a_tMoveList[0], E1, C1, MOVE_TYPE_CASTLE, PIECE_NONE, PIECE_KING, PIECE_NONE);
               nNumMoves++;
            }
         }
      }
   }

   return nNumMoves;
}

int GenerateWhiteRookQueenNonCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[WHITE][PIECE_ROOK - 1] | b->bbPieces[WHITE][PIECE_QUEEN - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = ROOK_MOVES(from) & ~(b->bbPieces[BLACK][PIECE_ALL - 1] | b->bbPieces[WHITE][PIECE_ALL - 1]);

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, b->nPieces[from] & SQUARE_PIECE_MASK,
            PIECE_NONE);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateWhiteBishopQueenNonCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[WHITE][PIECE_BISHOP - 1] | b->bbPieces[WHITE][PIECE_QUEEN - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = BISHOP_MOVES(from) & ~(b->bbPieces[BLACK][PIECE_ALL - 1] | b->bbPieces[WHITE][PIECE_ALL - 1]);

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, b->nPieces[from] & SQUARE_PIECE_MASK,
            PIECE_NONE);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateBlackPawnCaps(const TBoard* b, TMove* a_tMoveList)
{
   int to;
   int nNumMoves = 0;

   BitBoard bbAttacks = (b->bbPieces[BLACK][PIECE_PAWN - 1] & g_bbMaskNotFileA) >> 7 & b->bbPieces[WHITE][PIECE_ALL -
      1];

   while (bbAttacks)
   {
      to = RemoveBit(bbAttacks);

      if (to <= A1)
      {
         SET_MOVE(a_tMoveList[0], to + 7, to, MOVE_TYPE_PROM, PIECE_QUEEN, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[1], to + 7, to, MOVE_TYPE_PROM, PIECE_ROOK, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[2], to + 7, to, MOVE_TYPE_PROM, PIECE_BISHOP, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[3], to + 7, to, MOVE_TYPE_PROM, PIECE_KNIGHT, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList += 4;
         nNumMoves += 4;
      }
      else
      {
         SET_MOVE(a_tMoveList[0], to + 7, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   bbAttacks = (b->bbPieces[BLACK][PIECE_PAWN - 1] & g_bbMaskNotFileH) >> 9 & b->bbPieces[WHITE][PIECE_ALL - 1];

   while (bbAttacks)
   {
      to = RemoveBit(bbAttacks);

      if (to <= A1)
      {
         SET_MOVE(a_tMoveList[0], to + 9, to, MOVE_TYPE_PROM, PIECE_QUEEN, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[1], to + 9, to, MOVE_TYPE_PROM, PIECE_ROOK, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[2], to + 9, to, MOVE_TYPE_PROM, PIECE_BISHOP, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         SET_MOVE(a_tMoveList[3], to + 9, to, MOVE_TYPE_PROM, PIECE_KNIGHT, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList += 4;
         nNumMoves += 4;
      }
      else
      {
         SET_MOVE(a_tMoveList[0], to + 9, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_PAWN, b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   if (b->nEPSquare != NULL_SQUARE)
   {
      bbAttacks = g_nnBlackEPAttacks[b->nEPSquare] & b->bbPieces[BLACK][PIECE_PAWN - 1];
      while (bbAttacks)
      {
         const int from = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, b->nEPSquare, MOVE_TYPE_EP, PIECE_NONE, PIECE_PAWN, PIECE_PAWN);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   bbAttacks = b->bbPieces[BLACK][PIECE_PAWN - 1] >> 8 & g_bbMaskRank1 & ~(b->bbPieces[BLACK][PIECE_ALL - 1] | b->
      bbPieces[WHITE][PIECE_ALL - 1]);
   while (bbAttacks)
   {
      to = RemoveBit(bbAttacks);

      SET_MOVE(a_tMoveList[0], to + 8, to, MOVE_TYPE_PROM, PIECE_QUEEN, PIECE_PAWN, PIECE_NONE);
      SET_MOVE(a_tMoveList[1], to + 8, to, MOVE_TYPE_PROM, PIECE_ROOK, PIECE_PAWN, PIECE_NONE);
      SET_MOVE(a_tMoveList[2], to + 8, to, MOVE_TYPE_PROM, PIECE_BISHOP, PIECE_PAWN, PIECE_NONE);
      SET_MOVE(a_tMoveList[3], to + 8, to, MOVE_TYPE_PROM, PIECE_KNIGHT, PIECE_PAWN, PIECE_NONE);
      a_tMoveList += 4;
      nNumMoves += 4;
   }

   return nNumMoves;
}

int GenerateBlackKnightCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[BLACK][PIECE_KNIGHT - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = g_bbKnightAttacks[from] & b->bbPieces[WHITE][PIECE_ALL - 1];

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_KNIGHT,
            b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateBlackKingCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   const int from = GetBit(b->bbPieces[BLACK][PIECE_KING - 1]);

   BitBoard bbAttacks = g_bbKingAttacks[from] & b->bbPieces[WHITE][PIECE_ALL - 1];

   while (bbAttacks)
   {
      const int to = RemoveBit(bbAttacks);

      SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_KING, b->nPieces[to] & SQUARE_PIECE_MASK);
      a_tMoveList++;
      nNumMoves++;
   }

   return nNumMoves;
}

int GenerateBlackRookQueenCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[BLACK][PIECE_ROOK - 1] | b->bbPieces[BLACK][PIECE_QUEEN - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = ROOK_MOVES(from) & b->bbPieces[WHITE][PIECE_ALL - 1];

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, b->nPieces[from] & SQUARE_PIECE_MASK,
            b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateBlackBishopQueenCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[BLACK][PIECE_BISHOP - 1] | b->bbPieces[BLACK][PIECE_QUEEN - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = BISHOP_MOVES(from) & b->bbPieces[WHITE][PIECE_ALL - 1];

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, b->nPieces[from] & SQUARE_PIECE_MASK,
            b->nPieces[to] & SQUARE_PIECE_MASK);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateBlackPawnNonCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbAttacks = b->bbPieces[BLACK][PIECE_PAWN - 1] >> 8 & g_bbMaskNotRank1 & ~(b->bbPieces[WHITE][PIECE_ALL
      - 1] | b
      ->bbPieces[BLACK][PIECE_ALL - 1]);

   while (bbAttacks)
   {
      const int to = RemoveBit(bbAttacks);

      SET_MOVE(a_tMoveList[0], to + 8, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_PAWN, PIECE_NONE);
      a_tMoveList++;
      nNumMoves++;

      if (to >= H6 && to <= A6 && bit[to - 8] & ~(b->bbPieces[BLACK][PIECE_ALL - 1] | b->bbPieces[WHITE][PIECE_ALL -
         1]))
      {
         SET_MOVE(a_tMoveList[0], to + 8, to - 8, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_PAWN, PIECE_NONE);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateBlackKnightNonCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[BLACK][PIECE_KNIGHT - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = g_bbKnightAttacks[from] & ~(b->bbPieces[WHITE][PIECE_ALL - 1] | b->bbPieces[BLACK][PIECE_ALL -
         1]);

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_KNIGHT, PIECE_NONE);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateBlackKingNonCaps(const TBoard* b, TMove* a_tMoveList, const int nVariant)
{
   int nNumMoves = 0;

   const int from = GetBit(b->bbPieces[BLACK][PIECE_KING - 1]);

   BitBoard bbAttacks = g_bbKingAttacks[from] & ~(b->bbPieces[WHITE][PIECE_ALL - 1] | b->bbPieces[BLACK][PIECE_ALL -
      1]);

   while (bbAttacks)
   {
      const int to = RemoveBit(bbAttacks);

      SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, PIECE_KING, PIECE_NONE);
      a_tMoveList++;
      nNumMoves++;
   }

   if (b->nCastling & BLACK_KING_CASTLE)
   {
      if (nVariant == VARIANT_960)
      {
         const int ksq1 = from;
         constexpr int ksq2 = G8;
         int rsq1;
         for (rsq1 = H8; rsq1 < A8; rsq1++) { if (b->nPieces[rsq1] == BLACK_ROOK) break; }
         constexpr int rsq2 = F8;
         bool illegal = false;
         for (int sq = MIN(ksq2, from); sq <= MAX(ksq2, from); sq++)
         {
            if (sq != rsq1 && sq != ksq1 && sq != ksq2 && b->nPieces[sq])
            {
               illegal = true;
               break;
            }
            if (IsSquareAttackedBy(b, sq, WHITE))
            {
               illegal = true;
               break;
            }
         }
         if (!illegal)
            for (int sq = MIN(rsq2, rsq1); sq <= MAX(rsq2, rsq1); sq++)
            {
               if (sq != rsq1 && sq != rsq2 && sq != ksq1 && b->nPieces[sq]) illegal = true;
            }
         if (rsq1 != rsq2 && b->nPieces[rsq2] && b->nPieces[rsq2] != BLACK_KING) illegal = true;
         if (from != ksq2 && b->nPieces[ksq2] &&
            (b->nPieces[ksq2] != BLACK_ROOK || (b->nPieces[ksq2] == BLACK_ROOK && ksq2 != rsq1)))
            illegal = true;
         if (!illegal)
         {
            SET_MOVE(a_tMoveList[0], from, G8, MOVE_TYPE_CASTLE, PIECE_NONE, PIECE_KING, PIECE_NONE);
            a_tMoveList++;
            nNumMoves++;
         }
      }
      else
      {
         if (!b->nPieces[F8] && !b->nPieces[G8])
         {
            if (!IsSquareAttackedBy(b, E8, WHITE) && !IsSquareAttackedBy(b, F8, WHITE) && !IsSquareAttackedBy(
               b, G8, WHITE))
            {
               SET_MOVE(a_tMoveList[0], E8, G8, MOVE_TYPE_CASTLE, PIECE_NONE, PIECE_KING, PIECE_NONE);
               a_tMoveList++;
               nNumMoves++;
            }
         }
      }
   }
   if (b->nCastling & BLACK_QUEEN_CASTLE)
   {
      if (nVariant == VARIANT_960)
      {
         const int ksq1 = from;
         constexpr int ksq2 = C8;
         int rsq1;
         for (rsq1 = A8; rsq1 > H8; rsq1--) { if (b->nPieces[rsq1] == BLACK_ROOK) break; }
         constexpr int rsq2 = D8;
         bool illegal = false;
         for (int sq = MIN(ksq2, from); sq <= MAX(ksq2, from); sq++)
         {
            if (sq != rsq1 && sq != ksq1 && sq != ksq2 && b->nPieces[sq])
            {
               illegal = true;
               break;
            }
            if (IsSquareAttackedBy(b, sq, WHITE))
            {
               illegal = true;
               break;
            }
         }
         if (!illegal)
            for (int sq = MIN(rsq2, rsq1); sq <= MAX(rsq2, rsq1); sq++)
            {
               if (sq != rsq1 && sq != rsq2 && sq != ksq1 && b->nPieces[sq]) illegal = true;
            }
         if (rsq1 != rsq2 && b->nPieces[rsq2] && b->nPieces[rsq2] != BLACK_KING) illegal = true;
         if (from != ksq2 && b->nPieces[ksq2] &&
            (b->nPieces[ksq2] != BLACK_ROOK || (b->nPieces[ksq2] == BLACK_ROOK && ksq2 != rsq1)))
            illegal = true;
         if (rsq1 == B8 && (b->nPieces[A8] == WHITE_ROOK || b->nPieces[A8] == WHITE_QUEEN)) illegal = true;
         if (!illegal)
         {
            SET_MOVE(a_tMoveList[0], from, C8, MOVE_TYPE_CASTLE, PIECE_NONE, PIECE_KING, PIECE_NONE);
            nNumMoves++;
         }
      }
      else
      {
         if (!b->nPieces[D8] && !b->nPieces[C8] && !b->nPieces[B8])
         {
            if (!IsSquareAttackedBy(b, E8, WHITE) && !IsSquareAttackedBy(b, D8, WHITE) && !IsSquareAttackedBy(
               b, C8, WHITE))
            {
               SET_MOVE(a_tMoveList[0], E8, C8, MOVE_TYPE_CASTLE, PIECE_NONE, PIECE_KING, PIECE_NONE);
               nNumMoves++;
            }
         }
      }
   }

   return nNumMoves;
}

int GenerateBlackRookQueenNonCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[BLACK][PIECE_ROOK - 1] | b->bbPieces[BLACK][PIECE_QUEEN - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = ROOK_MOVES(from) & ~(b->bbPieces[WHITE][PIECE_ALL - 1] | b->bbPieces[BLACK][PIECE_ALL - 1]);

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, b->nPieces[from] & SQUARE_PIECE_MASK,
            PIECE_NONE);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

int GenerateBlackBishopQueenNonCaps(const TBoard* b, TMove* a_tMoveList)
{
   int nNumMoves = 0;

   BitBoard bbPieces = b->bbPieces[BLACK][PIECE_BISHOP - 1] | b->bbPieces[BLACK][PIECE_QUEEN - 1];

   while (bbPieces)
   {
      const int from = RemoveBit(bbPieces);

      BitBoard bbAttacks = BISHOP_MOVES(from) & ~(b->bbPieces[WHITE][PIECE_ALL - 1] | b->bbPieces[BLACK][PIECE_ALL - 1]);

      while (bbAttacks)
      {
         const int to = RemoveBit(bbAttacks);

         SET_MOVE(a_tMoveList[0], from, to, MOVE_TYPE_NORMAL, PIECE_NONE, b->nPieces[from] & SQUARE_PIECE_MASK,
            PIECE_NONE);
         a_tMoveList++;
         nNumMoves++;
      }
   }

   return nNumMoves;
}

bool IsAnyLegalMoves(TBoard* b, const int nVariant)
{
   int nNumMoves;
   TMove tMoveList[50];
   TUndoMove undo;
   if (b->nSideToMove == WHITE)
   {
      nNumMoves = GenerateWhitePawnNonCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateWhitePawnCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateWhiteKnightNonCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateWhiteKnightCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateWhiteKingNonCaps(b, tMoveList, nVariant);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateWhiteKingCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateWhiteRookQueenNonCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateWhiteRookQueenCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateWhiteBishopQueenNonCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateWhiteBishopQueenCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[WHITE][PIECE_KING - 1]), BLACK))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
   }
   else
   {
      nNumMoves = GenerateBlackPawnNonCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateBlackPawnCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateBlackKnightNonCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateBlackKnightCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateBlackKingNonCaps(b, tMoveList, nVariant);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateBlackKingCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateBlackRookQueenNonCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateBlackRookQueenCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateBlackBishopQueenNonCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
      nNumMoves = GenerateBlackBishopQueenCaps(b, tMoveList);
      for (int i = 0; i < nNumMoves; i++)
      {
         MakeMove2(b, tMoveList[i], &undo, 0, nVariant);
         if (IsSquareAttackedBy(b, GetBit(b->bbPieces[BLACK][PIECE_KING - 1]), WHITE))
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
         }
         else
         {
            UnMakeMove(b, tMoveList[i], &undo, 0, nVariant);
            return true;
         }
      }
   }

   return false;
}
