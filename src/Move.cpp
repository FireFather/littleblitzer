#include "stdafx.h"
#include "Move.h"
#include "Board.h"
#include "Common.h"
#include "Hash.h"

bool IsValidMoveQuick(const TMove m, const TBoard* b, const int nVariant)
{

   if (GET_MOVE_MOVED(m) == 0)
      return false;

   if (m == 0)
      return false;

   const int nFromSquare = GET_MOVE_FROM(m);

   if ((b->nPieces[nFromSquare] & SQUARE_PIECE_MASK) != GET_MOVE_MOVED(m))
      return false;

   const int nToSquare = GET_MOVE_TO(m);

   if ((b->nPieces[nToSquare] & SQUARE_COLOR_MASK) == (b->nPieces[nFromSquare] & SQUARE_COLOR_MASK)
      && GET_MOVE_TYPE(m) != MOVE_TYPE_CASTLE)
      return false;

   if ((b->nPieces[nFromSquare] & SQUARE_COLOR_MASK) >> 4 != b->nSideToMove)
      return false;

   switch (b->nPieces[nFromSquare])
   {
   case WHITE_PAWN:
      if (nToSquare - nFromSquare == 16)
      {
         if (nFromSquare < H2 || nFromSquare > A2) return false;
         if (b->nPieces[nFromSquare + 8]) return false;
         if (b->nPieces[nToSquare]) return false;
      }
      else if (nToSquare - nFromSquare == 8)
      {
         if (b->nPieces[nToSquare]) return false;
         if (nToSquare > A7 && GET_MOVE_TYPE(m) != MOVE_TYPE_PROM) return false;
      }
      else
      {
         if (GET_MOVE_TYPE(m) == MOVE_TYPE_EP)
         {
            if (b->nEPSquare != nToSquare) return false;
            if (b->nPieces[nToSquare - 8] != BLACK_PAWN) return false;
         }
         else if (!(g_bbWhitePawnAttacks[nFromSquare] & bit[nToSquare] & b->bbPieces[BLACK][PIECE_ALL - 1]))
         {
            return false;
         }
         else if (nToSquare >= H8 && GET_MOVE_TYPE(m) != MOVE_TYPE_PROM)
         {
            return false;
         }
      }
      break;

   case BLACK_PAWN:
      if (nToSquare - nFromSquare == -16)
      {
         if (nFromSquare < H7 || nFromSquare > A7) return false;
         if (b->nPieces[nFromSquare - 8]) return false;
         if (b->nPieces[nToSquare]) return false;
      }
      else if (nToSquare - nFromSquare == -8)
      {
         if (b->nPieces[nToSquare]) return false;
         if (nToSquare < H2 && GET_MOVE_TYPE(m) != MOVE_TYPE_PROM) return false;
      }
      else
      {
         if (GET_MOVE_TYPE(m) == MOVE_TYPE_EP)
         {
            if (b->nEPSquare != nToSquare) return false;
            if (b->nPieces[nToSquare + 8] != WHITE_PAWN) return false;
         }
         else if (!(g_bbBlackPawnAttacks[nFromSquare] & bit[nToSquare] & b->bbPieces[WHITE][PIECE_ALL - 1]))
         {
            return false;
         }
         else if (nToSquare <= A1 && GET_MOVE_TYPE(m) != MOVE_TYPE_PROM)
         {
            return false;
         }
      }
      break;

   case WHITE_BISHOP:
   case BLACK_BISHOP:
      if (!(BISHOP_MOVES(nFromSquare) & bit[nToSquare])) return false;
      break;

   case WHITE_ROOK:
   case BLACK_ROOK:
      if (!(ROOK_MOVES(nFromSquare) & bit[nToSquare])) return false;
      break;

   case WHITE_QUEEN:
   case BLACK_QUEEN:
      if (!((BISHOP_MOVES(nFromSquare) | ROOK_MOVES(nFromSquare)) & bit[nToSquare])) return false;
      break;

   case WHITE_KNIGHT:
   case BLACK_KNIGHT:
      if (!(g_bbKnightAttacks[nFromSquare] & bit[nToSquare])) return false;
      break;

   case WHITE_KING:
      if (nVariant == VARIANT_960)
      {
         if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && nToSquare == G1)
         {
            if (!(b->nCastling & WHITE_KING_CASTLE)) return false;
            const int ksq1 = nFromSquare;
            constexpr int ksq2 = G1;
            int rsq1;
            for (rsq1 = H1; rsq1 < A1; rsq1++) { if (b->nPieces[rsq1] == WHITE_ROOK) break; }
            constexpr int rsq2 = F1;
            for (int sq = MIN(ksq2, ksq1); sq <= MAX(ksq2, ksq1); sq++)
            {
               if (sq != rsq1 && sq != ksq1 && sq != ksq2 && b->nPieces[sq]) { return false; }
               if (IsSquareAttackedBy(b, sq, BLACK)) { return false; }
            }
            for (int sq = MIN(rsq2, rsq1); sq <= MAX(rsq2, rsq1); sq++)
            {
               if (sq != rsq1 && sq != rsq2 && sq != ksq1 && b->nPieces[sq]) return false;
            }
            if (rsq1 != rsq2 && b->nPieces[rsq2] && b->nPieces[rsq2] != WHITE_KING) return false;
            if (ksq1 != ksq2 && b->nPieces[ksq2] &&
               (b->nPieces[ksq2] != WHITE_ROOK || (b->nPieces[ksq2] == WHITE_ROOK && ksq2 != rsq1)))
               return false;
         }
         else if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && nToSquare == C1)
         {
            if (!(b->nCastling & WHITE_QUEEN_CASTLE)) return false;
            const int ksq1 = nFromSquare;
            constexpr int ksq2 = C1;
            int rsq1;
            for (rsq1 = A1; rsq1 > H1; rsq1--) { if (b->nPieces[rsq1] == WHITE_ROOK) break; }
            constexpr int rsq2 = D1;
            for (int sq = MIN(ksq2, ksq1); sq <= MAX(ksq2, ksq1); sq++)
            {
               if (sq != rsq1 && sq != ksq1 && sq != ksq2 && b->nPieces[sq]) { return false; }
               if (IsSquareAttackedBy(b, sq, BLACK)) { return false; }
            }
            for (int sq = MIN(rsq2, rsq1); sq <= MAX(rsq2, rsq1); sq++)
            {
               if (sq != rsq1 && sq != rsq2 && sq != ksq1 && b->nPieces[sq]) return false;
            }
            if (rsq1 != rsq2 && b->nPieces[rsq2] && b->nPieces[rsq2] != WHITE_KING) return false;
            if (ksq1 != ksq2 && b->nPieces[ksq2] &&
               (b->nPieces[ksq2] != WHITE_ROOK || (b->nPieces[ksq2] == WHITE_ROOK && ksq2 != rsq1)))
               return false;
         }
      }
      else if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && nToSquare == G1)
      {
         if (nFromSquare != E1) return false;
         if (nToSquare != G1) return false;
         if (b->nPieces[F1]) return false;
         if (b->nPieces[G1]) return false;
         if (b->nPieces[H1] != WHITE_ROOK) return false;
         if (IsSquareAttackedBy(b, E1, BLACK)) return false;
         if (IsSquareAttackedBy(b, F1, BLACK)) return false;
         if (IsSquareAttackedBy(b, G1, BLACK)) return false;
         if (!(b->nCastling & WHITE_KING_CASTLE)) return false;
      }
      else if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && nToSquare == C1)
      {
         if (nFromSquare != E1) return false;
         if (nToSquare != C1) return false;
         if (b->nPieces[D1]) return false;
         if (b->nPieces[C1]) return false;
         if (b->nPieces[B1]) return false;
         if (b->nPieces[A1] != WHITE_ROOK) return false;
         if (IsSquareAttackedBy(b, E1, BLACK)) return false;
         if (IsSquareAttackedBy(b, D1, BLACK)) return false;
         if (IsSquareAttackedBy(b, C1, BLACK)) return false;
         if (!(b->nCastling & WHITE_QUEEN_CASTLE)) return false;
      }
      else
      {
         if (!(g_bbKingAttacks[nFromSquare] & bit[nToSquare])) return false;
      }
      break;

   case BLACK_KING:
      if (nVariant == VARIANT_960)
      {
         if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && nToSquare == G8)
         {
            if (!(b->nCastling & BLACK_KING_CASTLE)) return false;
            const int ksq1 = nFromSquare;
            constexpr int ksq2 = G8;
            int rsq1;
            for (rsq1 = H8; rsq1 < A8; rsq1++) { if (b->nPieces[rsq1] == BLACK_ROOK) break; }
            constexpr int rsq2 = F8;
            for (int sq = MIN(ksq2, ksq1); sq <= MAX(ksq2, ksq1); sq++)
            {
               if (sq != rsq1 && sq != ksq1 && sq != ksq2 && b->nPieces[sq]) { return false; }
               if (IsSquareAttackedBy(b, sq, WHITE)) { return false; }
            }
            for (int sq = MIN(rsq2, rsq1); sq <= MAX(rsq2, rsq1); sq++)
            {
               if (sq != rsq1 && sq != rsq2 && sq != ksq1 && b->nPieces[sq]) return false;
            }
            if (rsq1 != rsq2 && b->nPieces[rsq2] && b->nPieces[rsq2] != BLACK_KING) return false;
            if (ksq1 != ksq2 && b->nPieces[ksq2] &&
               (b->nPieces[ksq2] != BLACK_ROOK || (b->nPieces[ksq2] == BLACK_ROOK && ksq2 != rsq1)))
               return false;
         }
         else if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && nToSquare == C8)
         {
            if (!(b->nCastling & BLACK_QUEEN_CASTLE)) return false;
            const int ksq1 = nFromSquare;
            constexpr int ksq2 = C8;
            int rsq1;
            for (rsq1 = A8; rsq1 > H8; rsq1--) { if (b->nPieces[rsq1] == BLACK_ROOK) break; }
            constexpr int rsq2 = D8;
            for (int sq = MIN(ksq2, ksq1); sq <= MAX(ksq2, ksq1); sq++)
            {
               if (sq != rsq1 && sq != ksq1 && sq != ksq2 && b->nPieces[sq]) { return false; }
               if (IsSquareAttackedBy(b, sq, WHITE)) { return false; }
            }
            for (int sq = MIN(rsq2, rsq1); sq <= MAX(rsq2, rsq1); sq++)
            {
               if (sq != rsq1 && sq != rsq2 && sq != ksq1 && b->nPieces[sq]) return false;
            }
            if (rsq1 != rsq2 && b->nPieces[rsq2] && b->nPieces[rsq2] != BLACK_KING) return false;
            if (ksq1 != ksq2 && b->nPieces[ksq2] &&
               (b->nPieces[ksq2] != BLACK_ROOK || (b->nPieces[ksq2] == BLACK_ROOK && ksq2 != rsq1)))
               return false;
            if (rsq1 == B8 && (b->nPieces[A8] == WHITE_ROOK || b->nPieces[A8] == WHITE_QUEEN)) return false;
         }
      }
      else if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && nToSquare == G8)
      {
         if (nFromSquare != E8) return false;
         if (nToSquare != G8) return false;
         if (b->nPieces[F8]) return false;
         if (b->nPieces[G8]) return false;
         if (b->nPieces[H8] != BLACK_ROOK) return false;
         if (IsSquareAttackedBy(b, E8, WHITE)) return false;
         if (IsSquareAttackedBy(b, F8, WHITE)) return false;
         if (IsSquareAttackedBy(b, G8, WHITE)) return false;
         if (!(b->nCastling & BLACK_KING_CASTLE)) return false;
      }
      else if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && nToSquare == C8)
      {
         if (nFromSquare != E8) return false;
         if (nToSquare != C8) return false;
         if (b->nPieces[D8]) return false;
         if (b->nPieces[C8]) return false;
         if (b->nPieces[B8]) return false;
         if (b->nPieces[A8] != BLACK_ROOK) return false;
         if (IsSquareAttackedBy(b, E8, WHITE)) return false;
         if (IsSquareAttackedBy(b, D8, WHITE)) return false;
         if (IsSquareAttackedBy(b, C8, WHITE)) return false;
         if (!(b->nCastling & BLACK_QUEEN_CASTLE)) return false;
      }
      else
      {
         if (!(g_bbKingAttacks[nFromSquare] & bit[nToSquare])) return false;
      }
      break;
   default:;
   }

   if (GET_MOVE_TYPE(m) != MOVE_TYPE_EP && GET_MOVE_TYPE(m) != MOVE_TYPE_CASTLE && GET_MOVE_CAPTURED(m) != (b->nPieces[nToSquare] & SQUARE_PIECE_MASK))
      return false;

   ASSERT_MOVE(m)

      return true;
}

bool Move2Coord(TMove* m, const TBoard* b, const char* sMove, const int nVariant)
{
   *m = 0;

   int nToSquare = 0;
   int nFromSquare = 0;
   int nMoveType = 0;
   int nPromPiece = 0;

   const int nSTM = b->nSideToMove;

   if (!_stricmp(sMove, "O-O"))
   {
      nMoveType = MOVE_TYPE_CASTLE;
      if (nVariant == VARIANT_STD)
      {
         nFromSquare = nSTM == WHITE ? E1 : E8;
         nToSquare = nSTM == WHITE ? G1 : G8;
      }
      else if (nVariant == VARIANT_960)
      {
         nFromSquare = GetBit(b->bbPieces[nSTM][PIECE_KING - 1]);
         nToSquare = nSTM == WHITE ? G1 : G8;
      }
   }
   else if (!_stricmp(sMove, "O-O-O"))
   {
      nMoveType = MOVE_TYPE_CASTLE;
      if (nVariant == VARIANT_STD)
      {
         nFromSquare = nSTM == WHITE ? E1 : E8;
         nToSquare = nSTM == WHITE ? C1 : C8;
      }
      else if (nVariant == VARIANT_960)
      {
         nFromSquare = GetBit(b->bbPieces[nSTM][PIECE_KING - 1]);
         nToSquare = nSTM == WHITE ? C1 : C8;
      }
   }
   else
   {
      if ((sMove[2] | 32) < 'a' || (sMove[2] | 32) > 'h') return false;
      nToSquare = 'h' - (sMove[2] | 32);
      if (sMove[3] < '1' || sMove[3] > '8') return false;
      nToSquare += 8 * (sMove[3] - '1');

      if ((sMove[0] | 32) < 'a' || (sMove[0] | 32) > 'h') return false;
      nFromSquare = 'h' - (sMove[0] | 32);
      if (sMove[1] < '1' || sMove[1] > '8') return false;
      nFromSquare += 8 * (sMove[1] - '1');
   }

   switch (sMove[4] | 32)
   {
   case 'q':
      nMoveType = MOVE_TYPE_PROM;
      nPromPiece = PIECE_QUEEN;
      break;
   case 'r':
      nMoveType = MOVE_TYPE_PROM;
      nPromPiece = PIECE_ROOK;
      break;
   case 'b':
      nMoveType = MOVE_TYPE_PROM;
      nPromPiece = PIECE_BISHOP;
      break;
   case 'n':
      nMoveType = MOVE_TYPE_PROM;
      nPromPiece = PIECE_KNIGHT;
      break;
   case '\0':
      break;
   default:;
   }

   if (nVariant == VARIANT_960)
   {
      if (b->nPieces[nFromSquare] == WHITE_KING && b->nPieces[nToSquare] == WHITE_ROOK)
      {
         nMoveType = MOVE_TYPE_CASTLE;
         const int r1 = GetBlackBit(b->bbPieces[WHITE][PIECE_ROOK - 1] & g_bbMaskRank1);
         if (const int nr = CountBits(b->bbPieces[WHITE][PIECE_ROOK - 1] & g_bbMaskRank1); nr == 1)
         {
            if (b->nCastling & WHITE_KING_CASTLE) nToSquare = G1;
            else nToSquare = C1;
         }
         else if (nr == 2)
         {
            if (nToSquare == r1)
               nToSquare = C1;
            else
               nToSquare = G1;
         }
      }
      else if (b->nPieces[nFromSquare] == BLACK_KING && b->nPieces[nToSquare] == BLACK_ROOK)
      {
         nMoveType = MOVE_TYPE_CASTLE;
         const int r1 = GetBlackBit(b->bbPieces[BLACK][PIECE_ROOK - 1] & g_bbMaskRank8);
         if (const int nr = CountBits(b->bbPieces[BLACK][PIECE_ROOK - 1] & g_bbMaskRank8); nr == 1)
         {
            if (b->nCastling & BLACK_KING_CASTLE) nToSquare = G8;
            else nToSquare = C8;
         }
         else if (nr == 2)
         {
            if (nToSquare == r1)
               nToSquare = C8;
            else
               nToSquare = G8;
         }
      }
   }
   else
   {
      if (b->nPieces[nFromSquare] == WHITE_KING && nFromSquare == E1 && nToSquare == G1)
      {
         nMoveType = MOVE_TYPE_CASTLE;
      }
      else if (b->nPieces[nFromSquare] == WHITE_KING && nFromSquare == E1 && nToSquare == C1)
      {
         nMoveType = MOVE_TYPE_CASTLE;
      }
      else if (b->nPieces[nFromSquare] == BLACK_KING && nFromSquare == E8 && nToSquare == G8)
      {
         nMoveType = MOVE_TYPE_CASTLE;
      }
      else if (b->nPieces[nFromSquare] == BLACK_KING && nFromSquare == E8 && nToSquare == C8)
      {
         nMoveType = MOVE_TYPE_CASTLE;
      }
   }

   int nCap = 0;
   if (b->nPieces[nFromSquare] == WHITE_PAWN && nToSquare - nFromSquare != 8 && nToSquare - nFromSquare != 16 && b->nPieces[nToSquare] == SQUARE_EMPTY)
   {
      nMoveType = MOVE_TYPE_EP;
      nCap = PIECE_PAWN;
   }
   if (b->nPieces[nFromSquare] == BLACK_PAWN && nFromSquare - nToSquare != 8 && nFromSquare - nToSquare != 16 && b->nPieces[nToSquare] == SQUARE_EMPTY)
   {
      nMoveType = MOVE_TYPE_EP;
      nCap = PIECE_PAWN;
   }

   if (!nCap && nMoveType != MOVE_TYPE_CASTLE)
   {
      nCap = b->nPieces[nToSquare] & SQUARE_PIECE_MASK;
   }

   SET_MOVE(*m, nFromSquare, nToSquare, nMoveType, nPromPiece, b->nPieces[nFromSquare] & SQUARE_PIECE_MASK, nCap);

   return true;
}

char* GetNotation(const TBoard* b, const TMove m)
{
   short pos = 0;
   const auto sNotation = new char[8];
   const int nFromSquare = GET_MOVE_FROM(m);
   const int nToSquare = GET_MOVE_TO(m);

   if (m == 0)
   {
      sNotation[0] = '\0';
      return sNotation;
   }

   if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && FILE(GET_MOVE_TO(m)) == 1)
   {
      strcpy(sNotation, "O-O");
      return sNotation;
   }
   if (GET_MOVE_TYPE(m) == MOVE_TYPE_CASTLE && FILE(GET_MOVE_TO(m)) == 5)
   {
      strcpy(sNotation, "O-O-O");
      return sNotation;
   }

   switch (GET_MOVE_MOVED(m))
   {
   case PIECE_PAWN:
      if (b->nPieces[nToSquare] || GET_MOVE_TYPE(m) == MOVE_TYPE_EP) sNotation[pos++] = 'h' - FILE(nFromSquare);
      break;
   case PIECE_KNIGHT:
      sNotation[pos++] = 'N';
      if (b->nSideToMove == WHITE)
      {
         if (CountBits(
            g_bbKnightAttacks[nToSquare] & b->bbPieces[WHITE][PIECE_KNIGHT - 1] & g_bbMaskRank[RANK(nFromSquare)]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
         else if (CountBits(
            g_bbKnightAttacks[nToSquare] & b->bbPieces[WHITE][PIECE_KNIGHT - 1] & g_bbMaskFile[FILE(nFromSquare)]) > 1)
            sNotation[pos++] = '1' + RANK(nFromSquare);
         else if (CountBits(g_bbKnightAttacks[nToSquare] & b->bbPieces[WHITE][PIECE_KNIGHT - 1]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
      }
      else
      {
         if (CountBits(
            g_bbKnightAttacks[nToSquare] & b->bbPieces[BLACK][PIECE_KNIGHT - 1] & g_bbMaskRank[RANK(nFromSquare)]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
         else if (CountBits(
            g_bbKnightAttacks[nToSquare] & b->bbPieces[BLACK][PIECE_KNIGHT - 1] & g_bbMaskFile[FILE(nFromSquare)]) > 1)
            sNotation[pos++] = '1' + RANK(nFromSquare);
         else if (CountBits(g_bbKnightAttacks[nToSquare] & b->bbPieces[BLACK][PIECE_KNIGHT - 1]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
      }
      break;
   case PIECE_BISHOP:
      sNotation[pos++] = 'B';
      if (b->nSideToMove == WHITE)
      {
         if (CountBits(BISHOP_MOVES(nToSquare) & b->bbPieces[WHITE][PIECE_BISHOP - 1] & g_bbMaskRank[RANK(nFromSquare)])
            > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
         else if (CountBits(
            BISHOP_MOVES(nToSquare) & b->bbPieces[WHITE][PIECE_BISHOP - 1] & g_bbMaskFile[FILE(nFromSquare)]) > 1)
            sNotation[pos++] = '1' + RANK(nFromSquare);
         else if (CountBits(BISHOP_MOVES(nToSquare) & b->bbPieces[WHITE][PIECE_BISHOP - 1]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
      }
      else
      {
         if (CountBits(BISHOP_MOVES(nToSquare) & b->bbPieces[BLACK][PIECE_BISHOP - 1] & g_bbMaskRank[RANK(nFromSquare)])
            > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
         else if (CountBits(
            BISHOP_MOVES(nToSquare) & b->bbPieces[BLACK][PIECE_BISHOP - 1] & g_bbMaskFile[FILE(nFromSquare)]) > 1)
            sNotation[pos++] = '1' + RANK(nFromSquare);
         else if (CountBits(BISHOP_MOVES(nToSquare) & b->bbPieces[BLACK][PIECE_BISHOP - 1]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
      }
      break;
   case PIECE_ROOK:
      sNotation[pos++] = 'R';
      if (b->nSideToMove == WHITE)
      {
         if (CountBits(ROOK_MOVES(nToSquare) & b->bbPieces[WHITE][PIECE_ROOK - 1] & g_bbMaskRank[RANK(nFromSquare)]) >
            1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
         else if (CountBits(
            ROOK_MOVES(nToSquare) & b->bbPieces[WHITE][PIECE_ROOK - 1] & g_bbMaskFile[FILE(nFromSquare)]) > 1)
            sNotation[pos++] = '1' + RANK(nFromSquare);
         else if (CountBits(ROOK_MOVES(nToSquare) & b->bbPieces[WHITE][PIECE_ROOK - 1]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
      }
      else
      {
         if (CountBits(ROOK_MOVES(nToSquare) & b->bbPieces[BLACK][PIECE_ROOK - 1] & g_bbMaskRank[RANK(nFromSquare)]) >
            1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
         else if (CountBits(
            ROOK_MOVES(nToSquare) & b->bbPieces[BLACK][PIECE_ROOK - 1] & g_bbMaskFile[FILE(nFromSquare)]) > 1)
            sNotation[pos++] = '1' + RANK(nFromSquare);
         else if (CountBits(ROOK_MOVES(nToSquare) & b->bbPieces[BLACK][PIECE_ROOK - 1]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
      }
      break;
   case PIECE_QUEEN:
      sNotation[pos++] = 'Q';
      if (b->nSideToMove == WHITE)
      {
         if (CountBits(
            (ROOK_MOVES(nToSquare) | BISHOP_MOVES(nToSquare)) & b->bbPieces[WHITE][PIECE_QUEEN - 1] & g_bbMaskRank[
               RANK(nFromSquare)]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
         else if (CountBits(
            (ROOK_MOVES(nToSquare) | BISHOP_MOVES(nToSquare)) & b->bbPieces[WHITE][PIECE_QUEEN - 1] & g_bbMaskFile[
               FILE(nFromSquare)]) > 1)
            sNotation[pos++] = '1' + RANK(nFromSquare);
         else if (CountBits((ROOK_MOVES(nToSquare) | BISHOP_MOVES(nToSquare)) & b->bbPieces[WHITE][PIECE_QUEEN - 1]) >
            1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
      }
      else
      {
         if (CountBits(
            (ROOK_MOVES(nToSquare) | BISHOP_MOVES(nToSquare)) & b->bbPieces[BLACK][PIECE_QUEEN - 1] & g_bbMaskRank[
               RANK(nFromSquare)]) > 1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
         else if (CountBits(
            (ROOK_MOVES(nToSquare) | BISHOP_MOVES(nToSquare)) & b->bbPieces[BLACK][PIECE_QUEEN - 1] & g_bbMaskFile[
               FILE(nFromSquare)]) > 1)
            sNotation[pos++] = '1' + RANK(nFromSquare);
         else if (CountBits((ROOK_MOVES(nToSquare) | BISHOP_MOVES(nToSquare)) & b->bbPieces[BLACK][PIECE_QUEEN - 1]) >
            1)
            sNotation[pos++] = 'h' - FILE(nFromSquare);
      }
      break;
   case PIECE_KING:
      sNotation[pos++] = 'K';
      break;
   default:;
   }

   if (GET_MOVE_CAPTURED(m))
   {
      sNotation[pos++] = 'x';
   }

   sNotation[pos++] = 'h' - FILE(nToSquare);
   sNotation[pos++] = '1' + RANK(nToSquare);

   if (GET_MOVE_MOVED(m) == PIECE_PAWN)
   {
      if (nToSquare < H2 || nToSquare > A7)
      {
         switch (GET_MOVE_PROM(m))
         {
         case PIECE_QUEEN: sNotation[pos++] = '=';
            sNotation[pos++] = 'Q';
            break;
         case PIECE_ROOK: sNotation[pos++] = '=';
            sNotation[pos++] = 'R';
            break;
         case PIECE_BISHOP: sNotation[pos++] = '=';
            sNotation[pos++] = 'B';
            break;
         case PIECE_KNIGHT: sNotation[pos++] = '=';
            sNotation[pos++] = 'N';
            break;
         default: sNotation[pos++] = '?';
            break;
         }
      }
   }

   sNotation[pos++] = '\0';

   return sNotation;
}

void MakeMove2(TBoard* b, const TMove m, TUndoMove* undo, const int nThreadID, const int nVariant)
{
   const int nSideToMove = b->nSideToMove;
   const int nFromSquare = GET_MOVE_FROM(m);
   const int nToSquare = GET_MOVE_TO(m);
   const int nMoveType = GET_MOVE_TYPE(m);
   const int nPromPiece = GET_MOVE_PROM(m);
   const int nMovedSquare = b->nPieces[nFromSquare];
   const int nMovedPiece = nMovedSquare & SQUARE_PIECE_MASK;
   const int nCapturedSquare = b->nPieces[nToSquare];
   int nCapturedPiece = nCapturedSquare & SQUARE_PIECE_MASK;
   if (nVariant == VARIANT_960)
   {
      if (nCapturedPiece == PIECE_KING) nCapturedPiece = 0;
      else if (nSideToMove == WHITE && nCapturedSquare == WHITE_ROOK) nCapturedPiece = 0;
      else if (nSideToMove == BLACK && nCapturedSquare == BLACK_ROOK) nCapturedPiece = 0;
   }
   BitBoard bbFromTo;

   ASSERT(b->nSideToMove == (nMovedSquare & SQUARE_COLOR_MASK) >> 4);
   ASSERT_MOVE(m)
      ASSERT(m);
   ASSERT(nMovedPiece);
   ASSERT(
      nVariant == VARIANT_960 || nMoveType == MOVE_TYPE_CASTLE || (b->nPieces[nFromSquare] & SQUARE_COLOR_MASK) != (b->
         nPieces[nToSquare] & SQUARE_COLOR_MASK));
   ASSERT(nFromSquare >= 0);
   ASSERT(nFromSquare < 64);
   ASSERT(nToSquare >= 0);
   ASSERT(nToSquare < 64);

   undo->nEPSquare = b->nEPSquare;
   undo->nCastleFlags = b->nCastling;
   undo->nCapturedPiece = nCapturedSquare;
   undo->n50Moves = b->nFiftyMoveCount;
   undo->nTransHash = b->nTransHash;
   undo->nPawnHash = b->nPawnHash;
   undo->nMaterialHash = b->nMaterialHash;
   undo->nRepListHead = g_nRepetitionListHead[nThreadID];
   undo->nMaterial[WHITE] = b->nMaterial[WHITE];
   undo->nMaterial[BLACK] = b->nMaterial[BLACK];
   const int nCastleBefore = b->nCastling;
   if (nCapturedPiece || nMoveType == MOVE_TYPE_EP)
   {
      b->nTransHash ^= g_nHash50Move[b->nFiftyMoveCount];
      b->nFiftyMoveCount = 0;
   }
   else
   {
      b->nTransHash ^= g_nHash50Move[b->nFiftyMoveCount];
      b->nFiftyMoveCount++;
      b->nTransHash ^= g_nHash50Move[b->nFiftyMoveCount];
   }
   if (b->nEPSquare != NULL_SQUARE)
   {
      b->nTransHash ^= g_nHashEPMove[b->nEPSquare];
   }
   b->nEPSquare = NULL_SQUARE;

   b->nTransHash ^= GET_HASH_COL(nFromSquare, nSideToMove, nMovedPiece);
   b->nTransHash ^= GET_HASH_COL(nToSquare, nSideToMove, nMovedPiece);
   b->nTransHash ^= GET_HASH_COL(nToSquare, OPP(nSideToMove), nCapturedPiece);
   b->nTransHash ^= g_nHashWhiteToMove;
   b->nPawnHash ^= g_nHashWhiteToMove;

   if (nFromSquare != nToSquare)
   {
      bbFromTo = bit[nFromSquare] | bit[nToSquare];
      b->nPieces[nToSquare] = b->nPieces[nFromSquare];
      b->nPieces[nFromSquare] = SQUARE_EMPTY;
   }
   else
   {
      bbFromTo = 0;
   }

   if (nSideToMove == WHITE)
   {
      b->bbPieces[WHITE][PIECE_ALL - 1] ^= bbFromTo;
      b->nMaterial[BLACK] -= g_nPieceValues[nCapturedPiece];

      b->bbPieces[WHITE][nMovedPiece - 1] ^= bbFromTo;

      switch (nMovedPiece)
      {
      case PIECE_PAWN:
         if (nToSquare - nFromSquare == 16)
         {
            b->nEPSquare = nToSquare - 8;
            b->nTransHash ^= g_nHashEPMove[b->nEPSquare];
         }
         if (nMoveType == MOVE_TYPE_EP)
         {
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[BLACK][PIECE_PAWN - 1]), BLACK_PAWN);
            b->bbPieces[BLACK][PIECE_PAWN - 1] ^= bit[nToSquare - 8];
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[BLACK][PIECE_PAWN - 1]), BLACK_PAWN);
            b->bbPieces[BLACK][PIECE_ALL - 1] ^= bit[nToSquare - 8];
            b->bbRotate90 ^= bit[g_nRotate90Map[nToSquare - 8]];
            b->bbRotate45L ^= bit[g_nRotate45LMap[nToSquare - 8]];
            b->bbRotate45R ^= bit[g_nRotate45RMap[nToSquare - 8]];
            b->nPieces[nToSquare - 8] = SQUARE_EMPTY;
            b->nTransHash ^= GET_HASH(nToSquare - 8, BLACK_PAWN);
            b->nPawnHash ^= GET_HASH(nToSquare - 8, BLACK_PAWN);
            b->nMaterial[BLACK] -= SCORE_PAWN;
         }
         b->nPawnHash ^= GET_HASH(nFromSquare, WHITE_PAWN);
         if (nToSquare >= H8)
         {
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[WHITE][PIECE_PAWN - 1]), WHITE_PAWN);
            b->bbPieces[WHITE][PIECE_PAWN - 1] ^= bit[nToSquare];
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[WHITE][PIECE_PAWN - 1]), WHITE_PAWN);

            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[WHITE][nPromPiece - 1]), PIECE_WHITE | nPromPiece);
            b->bbPieces[WHITE][nPromPiece - 1] ^= bit[nToSquare];
            b->nPieces[nToSquare] = PIECE_WHITE | nPromPiece;
            b->nTransHash ^= GET_HASH(nToSquare, WHITE_PAWN);
            b->nTransHash ^= GET_HASH(nToSquare, PIECE_WHITE | nPromPiece);
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[WHITE][nPromPiece - 1]), PIECE_WHITE | nPromPiece);
         }
         else
         {
            b->nPawnHash ^= GET_HASH(nToSquare, WHITE_PAWN);
         }
         b->nTransHash ^= g_nHash50Move[b->nFiftyMoveCount];
         b->nFiftyMoveCount = 0;
         break;

      case PIECE_ROOK:

         b->nCastling &= ~b->g_bbCastles[nFromSquare];
         break;

      case PIECE_KING:

         b->nCastling &= WHITE_CANT_CASTLE;
         if (nVariant == VARIANT_960)
         {
            if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == G1)
            {
               const int rsq = GetWhiteBit(b->bbPieces[WHITE][PIECE_ROOK - 1] & g_bbMaskRank1);
               undo->nOrigRookSq = rsq;
               b->bbPieces[WHITE][PIECE_ROOK - 1] ^= bit[rsq] ^ bit[F1];
               b->bbPieces[WHITE][PIECE_ALL - 1] ^= bit[rsq] ^ bit[F1];
               b->bbRotate90 ^= bit[g_nRotate90Map[rsq]] ^ bit[g_nRotate90Map[F1]];
               b->bbRotate45L ^= bit[g_nRotate45LMap[rsq]] ^ bit[g_nRotate45LMap[F1]];
               b->bbRotate45R ^= bit[g_nRotate45RMap[rsq]] ^ bit[g_nRotate45RMap[F1]];
               if (rsq != nToSquare) b->nPieces[rsq] = SQUARE_EMPTY;
               b->nPieces[F1] = WHITE_ROOK;
               b->nTransHash ^= GET_HASH(rsq, WHITE_ROOK);
               b->nTransHash ^= GET_HASH(F1, WHITE_ROOK);
               b->nCastling |= WHITE_HAS_CASTLED;
            }
            else if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == C1)
            {
               const int rsq = GetBlackBit(b->bbPieces[WHITE][PIECE_ROOK - 1] & g_bbMaskRank1);
               undo->nOrigRookSq = rsq;
               b->bbPieces[WHITE][PIECE_ROOK - 1] ^= bit[rsq] ^ bit[D1];
               b->bbPieces[WHITE][PIECE_ALL - 1] ^= bit[rsq] ^ bit[D1];
               b->bbRotate90 ^= bit[g_nRotate90Map[rsq]] ^ bit[g_nRotate90Map[D1]];
               b->bbRotate45L ^= bit[g_nRotate45LMap[rsq]] ^ bit[g_nRotate45LMap[D1]];
               b->bbRotate45R ^= bit[g_nRotate45RMap[rsq]] ^ bit[g_nRotate45RMap[D1]];
               if (rsq != nToSquare) b->nPieces[rsq] = SQUARE_EMPTY;
               b->nPieces[D1] = WHITE_ROOK;
               b->nTransHash ^= GET_HASH(rsq, WHITE_ROOK);
               b->nTransHash ^= GET_HASH(D1, WHITE_ROOK);
               b->nCastling |= WHITE_HAS_CASTLED;
            }
         }
         else
         {
            if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == G1)
            {
               b->bbPieces[WHITE][PIECE_ROOK - 1] ^= MOVE_WHITE_ROOK_CASTLE_K;
               b->bbPieces[WHITE][PIECE_ALL - 1] ^= MOVE_WHITE_ROOK_CASTLE_K;
               b->bbRotate90 ^= MOVE_WHITE_ROOK_CASTLE_K_90;
               b->bbRotate45L ^= MOVE_WHITE_ROOK_CASTLE_K_45L;
               b->bbRotate45R ^= MOVE_WHITE_ROOK_CASTLE_K_45R;
               b->nPieces[nToSquare - 1] = SQUARE_EMPTY;
               b->nPieces[nToSquare + 1] = WHITE_ROOK;
               b->nTransHash ^= GET_HASH(nToSquare - 1, WHITE_ROOK);
               b->nTransHash ^= GET_HASH(nToSquare + 1, WHITE_ROOK);
               b->nCastling |= WHITE_HAS_CASTLED;
            }
            else if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == C1)
            {
               b->bbPieces[WHITE][PIECE_ROOK - 1] ^= MOVE_WHITE_ROOK_CASTLE_Q;
               b->bbPieces[WHITE][PIECE_ALL - 1] ^= MOVE_WHITE_ROOK_CASTLE_Q;
               b->bbRotate90 ^= MOVE_WHITE_ROOK_CASTLE_Q_90;
               b->bbRotate45L ^= MOVE_WHITE_ROOK_CASTLE_Q_45L;
               b->bbRotate45R ^= MOVE_WHITE_ROOK_CASTLE_Q_45R;
               b->nPieces[nToSquare + 2] = SQUARE_EMPTY;
               b->nPieces[nToSquare - 1] = WHITE_ROOK;
               b->nTransHash ^= GET_HASH(nToSquare - 1, WHITE_ROOK);
               b->nTransHash ^= GET_HASH(nToSquare + 2, WHITE_ROOK);
               b->nCastling |= WHITE_HAS_CASTLED;
            }
         }
         break;
      default:;
      }

      if (nCapturedPiece > 0)
      {
         b->nMaterialHash ^= GET_HASH_COL(CountBits(b->bbPieces[BLACK][nCapturedPiece - 1]), BLACK, nCapturedPiece);
         b->bbPieces[BLACK][PIECE_ALL - 1] ^= bit[nToSquare];
         b->bbPieces[BLACK][nCapturedPiece - 1] ^= bit[nToSquare];
         b->nMaterialHash ^= GET_HASH_COL(CountBits(b->bbPieces[BLACK][nCapturedPiece - 1]), BLACK, nCapturedPiece);

         switch (nCapturedPiece)
         {
         case PIECE_PAWN:
            b->nPawnHash ^= GET_HASH(nToSquare, BLACK_PAWN);
            break;
         case PIECE_ROOK:
            b->nCastling &= ~b->g_bbCastles[nToSquare];
            break;
         default:;
#ifdef _DEBUG
         case PIECE_KING:
            PrintBoard(b, stdout);
            ASSERT(0);
#endif
         }
      }

      b->nSideToMove = BLACK;
   }
   else
   {
      b->bbPieces[BLACK][PIECE_ALL - 1] ^= bbFromTo;
      b->nMaterial[WHITE] -= g_nPieceValues[nCapturedPiece];

      b->bbPieces[BLACK][nMovedPiece - 1] ^= bbFromTo;

      switch (nMovedPiece)
      {
      case PIECE_PAWN:
         if (nFromSquare - nToSquare == 16)
         {
            b->nEPSquare = nToSquare + 8;
            b->nTransHash ^= g_nHashEPMove[b->nEPSquare];
         }
         if (nMoveType == MOVE_TYPE_EP)
         {
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[WHITE][PIECE_PAWN - 1]), WHITE_PAWN);
            b->bbPieces[WHITE][PIECE_PAWN - 1] ^= bit[nToSquare + 8];
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[WHITE][PIECE_PAWN - 1]), WHITE_PAWN);
            b->bbPieces[WHITE][PIECE_ALL - 1] ^= bit[nToSquare + 8];
            b->bbRotate90 ^= bit[g_nRotate90Map[nToSquare + 8]];
            b->bbRotate45L ^= bit[g_nRotate45LMap[nToSquare + 8]];
            b->bbRotate45R ^= bit[g_nRotate45RMap[nToSquare + 8]];
            b->nPieces[nToSquare + 8] = SQUARE_EMPTY;
            b->nTransHash ^= GET_HASH(nToSquare + 8, WHITE_PAWN);
            b->nPawnHash ^= GET_HASH(nToSquare + 8, WHITE_PAWN);
            b->nMaterial[WHITE] -= SCORE_PAWN;
         }
         b->nPawnHash ^= GET_HASH(nFromSquare, BLACK_PAWN);
         if (nToSquare <= A1)
         {
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[BLACK][PIECE_PAWN - 1]), BLACK_PAWN);
            b->bbPieces[BLACK][PIECE_PAWN - 1] ^= bit[nToSquare];
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[BLACK][PIECE_PAWN - 1]), BLACK_PAWN);

            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[BLACK][nPromPiece - 1]), PIECE_BLACK | nPromPiece);
            b->bbPieces[BLACK][nPromPiece - 1] ^= bit[nToSquare];
            b->nPieces[nToSquare] = PIECE_BLACK | nPromPiece;
            b->nTransHash ^= GET_HASH(nToSquare, BLACK_PAWN);
            b->nTransHash ^= GET_HASH(nToSquare, PIECE_BLACK | nPromPiece);
            b->nMaterialHash ^= GET_HASH(CountBits(b->bbPieces[BLACK][nPromPiece - 1]), PIECE_BLACK | nPromPiece);
         }
         else
         {
            b->nPawnHash ^= GET_HASH(nToSquare, BLACK_PAWN);
         }
         b->nTransHash ^= g_nHash50Move[b->nFiftyMoveCount];
         b->nFiftyMoveCount = 0;
         break;

      case PIECE_ROOK:
         b->nCastling &= ~b->g_bbCastles[nFromSquare];
         break;

      case PIECE_KING:
         b->nCastling &= BLACK_CANT_CASTLE;
         if (nVariant == VARIANT_960)
         {
            if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == G8)
            {
               const int rsq = GetWhiteBit(b->bbPieces[BLACK][PIECE_ROOK - 1] & g_bbMaskRank8);
               undo->nOrigRookSq = rsq;
               b->bbPieces[BLACK][PIECE_ROOK - 1] ^= bit[rsq] ^ bit[F8];
               b->bbPieces[BLACK][PIECE_ALL - 1] ^= bit[rsq] ^ bit[F8];
               b->bbRotate90 ^= bit[g_nRotate90Map[rsq]] ^ bit[g_nRotate90Map[F8]];
               b->bbRotate45L ^= bit[g_nRotate45LMap[rsq]] ^ bit[g_nRotate45LMap[F8]];
               b->bbRotate45R ^= bit[g_nRotate45RMap[rsq]] ^ bit[g_nRotate45RMap[F8]];
               if (rsq != nToSquare) b->nPieces[rsq] = SQUARE_EMPTY;
               b->nPieces[F8] = BLACK_ROOK;
               b->nTransHash ^= GET_HASH(rsq, BLACK_ROOK);
               b->nTransHash ^= GET_HASH(F8, BLACK_ROOK);
               b->nCastling |= BLACK_HAS_CASTLED;
            }
            else if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == C8)
            {
               const int rsq = GetBlackBit(b->bbPieces[BLACK][PIECE_ROOK - 1] & g_bbMaskRank8);
               undo->nOrigRookSq = rsq;
               b->bbPieces[BLACK][PIECE_ROOK - 1] ^= bit[rsq] ^ bit[D8];
               b->bbPieces[BLACK][PIECE_ALL - 1] ^= bit[rsq] ^ bit[D8];
               b->bbRotate90 ^= bit[g_nRotate90Map[rsq]] ^ bit[g_nRotate90Map[D8]];
               b->bbRotate45L ^= bit[g_nRotate45LMap[rsq]] ^ bit[g_nRotate45LMap[D8]];
               b->bbRotate45R ^= bit[g_nRotate45RMap[rsq]] ^ bit[g_nRotate45RMap[D8]];
               if (rsq != nToSquare) b->nPieces[rsq] = SQUARE_EMPTY;
               b->nPieces[D8] = BLACK_ROOK;
               b->nTransHash ^= GET_HASH(rsq, BLACK_ROOK);
               b->nTransHash ^= GET_HASH(D8, BLACK_ROOK);
               b->nCastling |= BLACK_HAS_CASTLED;
            }
         }
         else
         {
            if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == G8)
            {
               b->bbPieces[BLACK][PIECE_ROOK - 1] ^= MOVE_BLACK_ROOK_CASTLE_K;
               b->bbPieces[BLACK][PIECE_ALL - 1] ^= MOVE_BLACK_ROOK_CASTLE_K;
               b->bbRotate90 ^= MOVE_BLACK_ROOK_CASTLE_K_90;
               b->bbRotate45L ^= MOVE_BLACK_ROOK_CASTLE_K_45L;
               b->bbRotate45R ^= MOVE_BLACK_ROOK_CASTLE_K_45R;
               b->nPieces[nToSquare - 1] = SQUARE_EMPTY;
               b->nPieces[nToSquare + 1] = BLACK_ROOK;
               b->nTransHash ^= GET_HASH(nToSquare - 1, BLACK_ROOK);
               b->nTransHash ^= GET_HASH(nToSquare + 1, BLACK_ROOK);
               b->nCastling |= BLACK_HAS_CASTLED;
            }
            else if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == C8)
            {
               b->bbPieces[BLACK][PIECE_ROOK - 1] ^= MOVE_BLACK_ROOK_CASTLE_Q;
               b->bbPieces[BLACK][PIECE_ALL - 1] ^= MOVE_BLACK_ROOK_CASTLE_Q;
               b->bbRotate90 ^= MOVE_BLACK_ROOK_CASTLE_Q_90;
               b->bbRotate45L ^= MOVE_BLACK_ROOK_CASTLE_Q_45L;
               b->bbRotate45R ^= MOVE_BLACK_ROOK_CASTLE_Q_45R;
               b->nPieces[nToSquare + 2] = SQUARE_EMPTY;
               b->nPieces[nToSquare - 1] = BLACK_ROOK;
               b->nTransHash ^= GET_HASH(nToSquare - 1, BLACK_ROOK);
               b->nTransHash ^= GET_HASH(nToSquare + 2, BLACK_ROOK);
               b->nCastling |= BLACK_HAS_CASTLED;
            }
         }
         break;
      default:;
      }

      if (nCapturedPiece > 0)
      {
         b->nMaterialHash ^= GET_HASH_COL(CountBits(b->bbPieces[WHITE][nCapturedPiece - 1]), WHITE, nCapturedPiece);
         b->bbPieces[WHITE][PIECE_ALL - 1] ^= bit[nToSquare];
         b->bbPieces[WHITE][nCapturedPiece - 1] ^= bit[nToSquare];
         b->nMaterialHash ^= GET_HASH_COL(CountBits(b->bbPieces[WHITE][nCapturedPiece - 1]), WHITE, nCapturedPiece);

         switch (nCapturedPiece)
         {
         case PIECE_PAWN:
            b->nPawnHash ^= GET_HASH_COL(nToSquare, WHITE, PIECE_PAWN);
            break;
         case PIECE_ROOK:
            b->nCastling &= ~b->g_bbCastles[nToSquare];
            break;
         default:;
#ifdef _DEBUG
         case PIECE_KING:
            PrintBoard(b, stdout);
            ASSERT(0);
            break;
#endif
         }
      }

      b->nSideToMove = WHITE;
   }

   if (nCapturedPiece != SQUARE_EMPTY)
   {
      b->bbRotate90 ^= bit[g_nRotate90Map[nFromSquare]];
      b->bbRotate45L ^= bit[g_nRotate45LMap[nFromSquare]];
      b->bbRotate45R ^= bit[g_nRotate45RMap[nFromSquare]];
   }
   else if (nFromSquare != nToSquare)
   {
      b->bbRotate90 ^= bit[g_nRotate90Map[nFromSquare]] | bit[g_nRotate90Map[nToSquare]];
      b->bbRotate45L ^= bit[g_nRotate45LMap[nFromSquare]] | bit[g_nRotate45LMap[nToSquare]];
      b->bbRotate45R ^= bit[g_nRotate45RMap[nFromSquare]] | bit[g_nRotate45RMap[nToSquare]];
   }

   if (const int nCastleAfter = b->nCastling ^ nCastleBefore)
   {
      if (nCastleAfter & WHITE_KING_CASTLE)
      {
         b->nTransHash ^= g_nHashWhiteKingCastle;
      }
      if (nCastleAfter & WHITE_QUEEN_CASTLE)
      {
         b->nTransHash ^= g_nHashWhiteQueenCastle;
      }
      if (nCastleAfter & BLACK_KING_CASTLE)
      {
         b->nTransHash ^= g_nHashBlackKingCastle;
      }
      if (nCastleAfter & BLACK_QUEEN_CASTLE)
      {
         b->nTransHash ^= g_nHashBlackQueenCastle;
      }
   }

#ifdef _DEBUG
   ValidateBoard(b);
#endif
}

void UnMakeMove(TBoard* b, const TMove m, const TUndoMove* undo, const int nThreadID, const int nVariant)
{
   b->nSideToMove = OPP(b->nSideToMove);
   const int nSideToMove = b->nSideToMove;
   const int nFromSquare = GET_MOVE_FROM(m);
   const int nToSquare = GET_MOVE_TO(m);
   const int nMoveType = GET_MOVE_TYPE(m);
   const int nPromPiece = GET_MOVE_PROM(m);
   const int nMovedSquare = b->nPieces[nToSquare];
   int nMovedPiece = nMovedSquare & SQUARE_PIECE_MASK;
   BitBoard bbFromTo;
   ASSERT_MOVE(m)
      ASSERT(m);
   ASSERT(nMovedPiece);
   ASSERT(nFromSquare >= 0);
   ASSERT(nFromSquare < 64);
   ASSERT(nToSquare >= 0);
   ASSERT(nToSquare < 64);
   ASSERT(b->nSideToMove == (nMovedSquare & SQUARE_COLOR_MASK) >> 4);

   b->nEPSquare = undo->nEPSquare;
   b->nCastling = undo->nCastleFlags;
   const int nCapturedSquare = undo->nCapturedPiece;
   int nCapturedPiece = nCapturedSquare & SQUARE_PIECE_MASK;
   if (nVariant == VARIANT_960)
   {
      if (nCapturedPiece == PIECE_KING) nCapturedPiece = 0;
      else if (nSideToMove == WHITE && nCapturedSquare == WHITE_ROOK) nCapturedPiece = 0;
      else if (nSideToMove == BLACK && nCapturedSquare == BLACK_ROOK) nCapturedPiece = 0;
   }
   b->nFiftyMoveCount = undo->n50Moves;
   b->nTransHash = undo->nTransHash;
   b->nPawnHash = undo->nPawnHash;
   b->nMaterialHash = undo->nMaterialHash;
   g_nRepetitionListHead[nThreadID] = undo->nRepListHead;
   b->nMaterial[WHITE] = undo->nMaterial[WHITE];
   b->nMaterial[BLACK] = undo->nMaterial[BLACK];

   if (nMoveType == MOVE_TYPE_PROM)
   {
      nMovedPiece = PIECE_PAWN;
   }

   if (nFromSquare != nToSquare)
   {
      bbFromTo = bit[nFromSquare] | bit[nToSquare];
      b->nPieces[nFromSquare] = b->nPieces[nToSquare];
      b->nPieces[nToSquare] = nCapturedSquare;
   }
   else
   {
      bbFromTo = 0;
   }

   if (nSideToMove == WHITE)
   {
      b->bbPieces[WHITE][PIECE_ALL - 1] ^= bbFromTo;

      b->bbPieces[WHITE][nMovedPiece - 1] ^= bbFromTo;

      switch (nMovedPiece)
      {
      case PIECE_PAWN:
         if (nMoveType == MOVE_TYPE_EP)
         {
            b->bbPieces[BLACK][PIECE_PAWN - 1] ^= bit[nToSquare - 8];
            b->bbPieces[BLACK][PIECE_ALL - 1] ^= bit[nToSquare - 8];
            b->bbRotate90 ^= bit[g_nRotate90Map[nToSquare - 8]];
            b->bbRotate45L ^= bit[g_nRotate45LMap[nToSquare - 8]];
            b->bbRotate45R ^= bit[g_nRotate45RMap[nToSquare - 8]];
            b->nPieces[nToSquare - 8] = BLACK_PAWN;
         }
         if (nToSquare >= H8)
         {
            b->bbPieces[WHITE][PIECE_PAWN - 1] ^= bit[nToSquare];
            b->bbPieces[WHITE][nPromPiece - 1] ^= bit[nToSquare];
            b->nPieces[nFromSquare] = PIECE_WHITE | PIECE_PAWN;
         }
         break;

      case PIECE_KING:
         if (nVariant == VARIANT_960)
         {
            if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == G1)
            {
               const int rsq = undo->nOrigRookSq;
               b->bbPieces[WHITE][PIECE_ROOK - 1] ^= bit[rsq] ^ bit[F1];
               b->bbPieces[WHITE][PIECE_ALL - 1] ^= bit[rsq] ^ bit[F1];
               b->bbRotate90 ^= bit[g_nRotate90Map[rsq]] ^ bit[g_nRotate90Map[F1]];
               b->bbRotate45L ^= bit[g_nRotate45LMap[rsq]] ^ bit[g_nRotate45LMap[F1]];
               b->bbRotate45R ^= bit[g_nRotate45RMap[rsq]] ^ bit[g_nRotate45RMap[F1]];
               b->nPieces[rsq] = WHITE_ROOK;
               if (nFromSquare != F1 && rsq != F1) b->nPieces[F1] = SQUARE_EMPTY;
            }
            else if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == C1)
            {
               const int rsq = undo->nOrigRookSq;
               b->bbPieces[WHITE][PIECE_ROOK - 1] ^= bit[rsq] ^ bit[D1];
               b->bbPieces[WHITE][PIECE_ALL - 1] ^= bit[rsq] ^ bit[D1];
               b->bbRotate90 ^= bit[g_nRotate90Map[rsq]] ^ bit[g_nRotate90Map[D1]];
               b->bbRotate45L ^= bit[g_nRotate45LMap[rsq]] ^ bit[g_nRotate45LMap[D1]];
               b->bbRotate45R ^= bit[g_nRotate45RMap[rsq]] ^ bit[g_nRotate45RMap[D1]];
               if (nFromSquare != D1 && rsq != D1) b->nPieces[D1] = SQUARE_EMPTY;
               b->nPieces[rsq] = WHITE_ROOK;
            }
         }
         else
         {
            if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == G1)
            {
               b->bbPieces[WHITE][PIECE_ROOK - 1] ^= MOVE_WHITE_ROOK_CASTLE_K;
               b->bbPieces[WHITE][PIECE_ALL - 1] ^= MOVE_WHITE_ROOK_CASTLE_K;
               b->bbRotate90 ^= MOVE_WHITE_ROOK_CASTLE_K_90;
               b->bbRotate45L ^= MOVE_WHITE_ROOK_CASTLE_K_45L;
               b->bbRotate45R ^= MOVE_WHITE_ROOK_CASTLE_K_45R;
               b->nPieces[nToSquare - 1] = WHITE_ROOK;
               b->nPieces[nToSquare + 1] = SQUARE_EMPTY;
            }
            else if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == C1)
            {
               b->bbPieces[WHITE][PIECE_ROOK - 1] ^= MOVE_WHITE_ROOK_CASTLE_Q;
               b->bbPieces[WHITE][PIECE_ALL - 1] ^= MOVE_WHITE_ROOK_CASTLE_Q;
               b->bbRotate90 ^= MOVE_WHITE_ROOK_CASTLE_Q_90;
               b->bbRotate45L ^= MOVE_WHITE_ROOK_CASTLE_Q_45L;
               b->bbRotate45R ^= MOVE_WHITE_ROOK_CASTLE_Q_45R;
               b->nPieces[nToSquare - 1] = SQUARE_EMPTY;
               b->nPieces[nToSquare + 2] = WHITE_ROOK;
            }
         }
         break;
      default:;
      }

      if (nCapturedPiece > 0)
      {
         b->bbPieces[BLACK][PIECE_ALL - 1] ^= bit[nToSquare];
         b->bbPieces[BLACK][nCapturedPiece - 1] ^= bit[nToSquare];
      }
   }
   else
   {
      b->bbPieces[BLACK][PIECE_ALL - 1] ^= bbFromTo;

      b->bbPieces[BLACK][nMovedPiece - 1] ^= bbFromTo;

      switch (nMovedPiece)
      {
      case PIECE_PAWN:
         if (nMoveType == MOVE_TYPE_EP)
         {
            b->bbPieces[WHITE][PIECE_PAWN - 1] ^= bit[nToSquare + 8];
            b->bbPieces[WHITE][PIECE_ALL - 1] ^= bit[nToSquare + 8];
            b->bbRotate90 ^= bit[g_nRotate90Map[nToSquare + 8]];
            b->bbRotate45L ^= bit[g_nRotate45LMap[nToSquare + 8]];
            b->bbRotate45R ^= bit[g_nRotate45RMap[nToSquare + 8]];
            b->nPieces[nToSquare + 8] = WHITE_PAWN;
         }
         if (nToSquare <= A1)
         {
            b->bbPieces[BLACK][PIECE_PAWN - 1] ^= bit[nToSquare];
            b->bbPieces[BLACK][nPromPiece - 1] ^= bit[nToSquare];
            b->nPieces[nFromSquare] = PIECE_BLACK | PIECE_PAWN;
         }
         break;

      case PIECE_KING:
         if (nVariant == VARIANT_960)
         {
            if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == G8)
            {
               const int rsq = undo->nOrigRookSq;
               b->bbPieces[BLACK][PIECE_ROOK - 1] ^= bit[rsq] ^ bit[F8];
               b->bbPieces[BLACK][PIECE_ALL - 1] ^= bit[rsq] ^ bit[F8];
               b->bbRotate90 ^= bit[g_nRotate90Map[rsq]] ^ bit[g_nRotate90Map[F8]];
               b->bbRotate45L ^= bit[g_nRotate45LMap[rsq]] ^ bit[g_nRotate45LMap[F8]];
               b->bbRotate45R ^= bit[g_nRotate45RMap[rsq]] ^ bit[g_nRotate45RMap[F8]];
               if (nFromSquare != F8 && rsq != F8) b->nPieces[F8] = SQUARE_EMPTY;
               b->nPieces[rsq] = BLACK_ROOK;
            }
            else if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == C8)
            {
               const int rsq = undo->nOrigRookSq;
               b->bbPieces[BLACK][PIECE_ROOK - 1] ^= bit[rsq] ^ bit[D8];
               b->bbPieces[BLACK][PIECE_ALL - 1] ^= bit[rsq] ^ bit[D8];
               b->bbRotate90 ^= bit[g_nRotate90Map[rsq]] ^ bit[g_nRotate90Map[D8]];
               b->bbRotate45L ^= bit[g_nRotate45LMap[rsq]] ^ bit[g_nRotate45LMap[D8]];
               b->bbRotate45R ^= bit[g_nRotate45RMap[rsq]] ^ bit[g_nRotate45RMap[D8]];
               if (nFromSquare != D8 && rsq != D8) b->nPieces[D8] = SQUARE_EMPTY;
               b->nPieces[rsq] = BLACK_ROOK;
            }
         }
         else
         {
            if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == G8)
            {
               b->bbPieces[BLACK][PIECE_ROOK - 1] ^= MOVE_BLACK_ROOK_CASTLE_K;
               b->bbPieces[BLACK][PIECE_ALL - 1] ^= MOVE_BLACK_ROOK_CASTLE_K;
               b->bbRotate90 ^= MOVE_BLACK_ROOK_CASTLE_K_90;
               b->bbRotate45L ^= MOVE_BLACK_ROOK_CASTLE_K_45L;
               b->bbRotate45R ^= MOVE_BLACK_ROOK_CASTLE_K_45R;
               b->nPieces[nToSquare - 1] = BLACK_ROOK;
               b->nPieces[nToSquare + 1] = SQUARE_EMPTY;
            }
            else if (nMoveType == MOVE_TYPE_CASTLE && nToSquare == C8)
            {
               b->bbPieces[BLACK][PIECE_ROOK - 1] ^= MOVE_BLACK_ROOK_CASTLE_Q;
               b->bbPieces[BLACK][PIECE_ALL - 1] ^= MOVE_BLACK_ROOK_CASTLE_Q;
               b->bbRotate90 ^= MOVE_BLACK_ROOK_CASTLE_Q_90;
               b->bbRotate45L ^= MOVE_BLACK_ROOK_CASTLE_Q_45L;
               b->bbRotate45R ^= MOVE_BLACK_ROOK_CASTLE_Q_45R;
               b->nPieces[nToSquare + 2] = BLACK_ROOK;
               b->nPieces[nToSquare - 1] = SQUARE_EMPTY;
            }
         }
         break;
      default:;
      }

      if (nCapturedPiece > 0)
      {
         b->bbPieces[WHITE][PIECE_ALL - 1] ^= bit[nToSquare];
         b->bbPieces[WHITE][nCapturedPiece - 1] ^= bit[nToSquare];
      }
   }

   if (nCapturedPiece != SQUARE_EMPTY)
   {
      b->bbRotate90 ^= bit[g_nRotate90Map[nFromSquare]];
      b->bbRotate45L ^= bit[g_nRotate45LMap[nFromSquare]];
      b->bbRotate45R ^= bit[g_nRotate45RMap[nFromSquare]];
   }
   else if (nFromSquare != nToSquare)
   {
      b->bbRotate90 ^= bit[g_nRotate90Map[nFromSquare]] | bit[g_nRotate90Map[nToSquare]];
      b->bbRotate45L ^= bit[g_nRotate45LMap[nFromSquare]] | bit[g_nRotate45LMap[nToSquare]];
      b->bbRotate45R ^= bit[g_nRotate45RMap[nFromSquare]] | bit[g_nRotate45RMap[nToSquare]];
   }

#ifdef _DEBUG
   ValidateBoard(b);
#endif
}

void GameMoves2FEN(char* sMoves, char* sFEN)
{
   char* sWord;
   TBoard b1, * b;
   b = &b1;
   CreateStartingPosition(b, VARIANT_STD);
   int stm = WHITE;
   sWord = strtok(sMoves, " \n\r\t.");
   while (sWord)
   {
      if ((sWord[0] >= 'a' && sWord[0] <= 'z') || (sWord[0] >= 'A' && sWord[0] <= 'Z'))
      {
         TMove m;
         int file1 = -1, rank1 = -1, file2 = -1, rank2 = -1;
         int i = 0, capture = 0, check = 0, prom = 0;
         int piece = 0, from = -1, to = -1, type = MOVE_TYPE_NORMAL;
         BitBoard mask, frpc = 0;
         TUndoMove u;

         if (sWord[i] == 'N') piece = PIECE_KNIGHT;
         else if (sWord[i] == 'B') piece = PIECE_BISHOP;
         else if (sWord[i] == 'R') piece = PIECE_ROOK;
         else if (sWord[i] == 'Q') piece = PIECE_QUEEN;
         else if (sWord[i] == 'K') piece = PIECE_KING;
         else if (sWord[i] == 'O') piece = PIECE_KING;
         else if (sWord[i] >= 'a' && sWord[i] <= 'h')
         {
            piece = PIECE_PAWN;
            i--;
         }
         else { ASSERT(false); }
         i++;

         if (sWord[i] == 'x')
         {
            capture = 1;
            i++;
         }

         if (sWord[i] >= 'a' && sWord[i] <= 'h')
         {
            file1 = 'h' - sWord[i];
            i++;
         }
         if (sWord[i] >= '1' && sWord[i] <= '8')
         {
            rank1 = sWord[i] - '1';
            i++;
         }

         if (sWord[i] == 'x')
         {
            capture = 1;
            i++;
         }

         if (sWord[i] == '+')
         {
            check = 1;
            i++;
         }
         else if (sWord[i] == '=')
         {
            prom = 1;
            i++;
         }

         if (sWord[i] >= 'a' && sWord[i] <= 'h')
         {
            file2 = 'h' - sWord[i];
            i++;
         }
         if (sWord[i] >= '1' && sWord[i] <= '8')
         {
            rank2 = sWord[i] - '1';
            i++;
         }

         if (sWord[i] == '+')
         {
            check = 1;
            i++;
         }
         else if (sWord[i] == '=')
         {
            prom = 1;
            i++;
         }

         if (sWord[i] == 'N') prom = PIECE_KNIGHT;
         else if (sWord[i] == 'B') prom = PIECE_BISHOP;
         else if (sWord[i] == 'R') prom = PIECE_ROOK;
         else if (sWord[i] == 'Q') prom = PIECE_QUEEN;

         if (file1 > -1 && rank1 > -1)
         {
            to = rank1 * 8 + file1;
            mask = 0xFFFFFFFFFFFFFFFF;
         }
         else if (file1 > -1 && rank1 == -1)
         {
            to = rank2 * 8 + file2;
            mask = g_bbMaskFile[file1];
         }
         else if (file1 == -1 && rank1 > -1)
         {
            to = rank2 * 8 + file2;
            mask = g_bbMaskRank[rank1];
         }

         if (sWord[0] == 'O' && sWord[2] == 'O')
         {
            type = MOVE_TYPE_CASTLE;
            from = GetBit(b->bbPieces[stm][PIECE_KING - 1]);
            if (sWord[3] == '-' && sWord[4] == 'O') to = stm ? C8 : C1;
            else to = stm ? G8 : G1;
         }

         if (piece == PIECE_PAWN)
         {
            if (capture)
            {
               if (b->bbPieces[stm][piece - 1])
                  from = GetBit(g_bbPawnAttacks[OPP(stm)][to] & b->bbPieces[stm][piece - 1] & mask);
               else
                  from = stm ? to + 8 : to - 8;
            }
            else if (stm == WHITE && b->nPieces[to - 8])
            {
               from = to - 8;
            }
            else if (stm == WHITE && b->nPieces[to - 16])
            {
               from = to - 16;
            }
            else if (stm == BLACK && b->nPieces[to + 8])
            {
               from = to + 8;
            }
            else if (stm == BLACK && b->nPieces[to + 16])
            {
               from = to + 16;
            }
         }
         else if (piece == PIECE_KNIGHT) frpc = g_bbKnightAttacks[to] & b->bbPieces[stm][piece - 1] & mask;
         else if (piece == PIECE_BISHOP) frpc = BISHOP_MOVES(to) & b->bbPieces[stm][piece - 1] & mask;
         else if (piece == PIECE_ROOK) frpc = ROOK_MOVES(to) & b->bbPieces[stm][piece - 1] & mask;
         else if (piece == PIECE_QUEEN) frpc = QUEEN_MOVES(to) & b->bbPieces[stm][piece - 1] & mask;
         else if (piece == PIECE_KING && type != MOVE_TYPE_CASTLE) frpc = g_bbKingAttacks[to] & b->bbPieces[stm][piece -
            1] & mask;

         if (prom > 0) type = MOVE_TYPE_PROM;
         if (capture) capture = b->nPieces[to] & SQUARE_PIECE_MASK;

         if (CountBits(frpc) > 1)
         {
            do
            {
               from = RemoveBit(frpc);
               SET_MOVE(m, from, to, type, prom, piece, capture);
               MakeMove2(b, m, &u, 0, VARIANT_STD);
               if (IsSquareAttackedBy(b, GetBit(b->bbPieces[stm][PIECE_KING - 1]), OPP(stm)))
               {
                  UnMakeMove(b, m, &u, 1, VARIANT_STD);
                  continue;
               }
               break;
            } while (frpc);
            ASSERT(from > -1);
         }
         else
         {
            if (frpc) from = GetBit(frpc);
            ASSERT(from > -1);
            SET_MOVE(m, from, to, type, prom, piece, capture);
            MakeMove2(b, m, &u, 0, VARIANT_STD);
         }

         stm = OPP(stm);
         ASSERT(stm == b->nSideToMove);
      }
      sWord = strtok(nullptr, " \n\r\t.");
   }

   Board2FEN(b, sFEN);
}
