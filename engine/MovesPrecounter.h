#pragma once

#include "Figure.h"

class MovesTable
{
  // figure current position, color, figure type, direction, figure next position
  static int s_moveIndices_[64*2*8*8*8];

  // position, color, type, attacked position
  static int s_attackIndices[64*2*8*8];

  // position, color, figure type (1..6)
  static uint64 s_eatMasks_[64*2*8];

  static FPos delta_pos_pawn_[2][4];
  static FPos delta_pos_bishop_[4];
  static FPos delta_pos_knight_[8];
  static FPos delta_pos_rook_[4];
  static FPos delta_pos_queen_[8];
  static FPos delta_pos_king_[8];

public:

  MovesTable();

  static inline const int * inds(int pos, int color, int ftype, int dir)
  {
    THROW_IF((unsigned)pos > 63 || (unsigned)color > 1 || (unsigned)(ftype-Figure::TypePawn) > 5 || (unsigned)dir > 7, "try to get invalid index for next step");
    return s_moveIndices_ + ((pos<<10) | (color<<9) | ((ftype-Figure::TypePawn)<<6) | (dir<<3));
  }

  static inline const uint64 & eats(int pos, int color, int ftype)
  {
    THROW_IF( (unsigned)pos > 63 || (unsigned)color > 1 || (unsigned)(ftype-Figure::TypePawn) > 5, "try to get invalid mask for figure" );
    return *( s_eatMasks_ + ((pos<<4) | (color<<3) | (ftype-Figure::TypePawn)) );
  }

  static inline int * inds(int pos, int color, int ftype)
  {
    THROW_IF((unsigned)pos > 63 || (unsigned)color > 1 || (unsigned)(ftype-Figure::TypePawn) > 5 , "try to det invalid index for next step");
    return s_moveIndices_ + ((pos<<10) | (color<<9) | ((ftype-Figure::TypePawn)<<6));
  }

  static inline int * attack(int pos, int color, int ftype)
  {
	  THROW_IF((unsigned)pos > 63  || (unsigned)color > 1 || (unsigned)(ftype-Figure::TypePawn) > 5, "invalid attack indices");
	  return s_attackIndices + ((pos<<7) | (color<< 6) | ((ftype-Figure::TypePawn)<<3));
  }

private:

  static inline int * inds_intr(int pos, int color, int ftype, int dir)
  {
    THROW_IF((unsigned)pos > 63 || (unsigned)color > 1 || (unsigned)(ftype-Figure::TypePawn) > 5 || (unsigned)dir > 7, "try to get invalid index for next step");
    return s_moveIndices_ + ((pos<<10) | (color<<9) | ((ftype-Figure::TypePawn)<<6) | (dir<<3));
  }

  static inline uint64 & mask(int pos, int color, int ftype)
  {
    THROW_IF( (unsigned)pos > 63 || (unsigned)color > 1 || (unsigned)(ftype-Figure::TypePawn) > 5, "try to get invalid mask for figure" );
    return *( s_eatMasks_ + ((pos<<4) | (color<<3) | (ftype-Figure::TypePawn)) );
  }

  void calcPawn(int pos);
  void calcBishop(int pos);
  void calcKnight(int pos);
  void calcRook(int pos);
  void calcQueen(int pos);
  void calcKing(int pos);

  void calcPawnEat(int pos);
  void calcBishopEat(int pos);
  void calcKnightEat(int pos);
  void calcRookEat(int pos);
  void calcQueenEat(int pos);
  void calcKingEat(int pos);

  void calcPawnAttack(int pos);
  void calcBishopAttack(int pos);
  void calcKnightAttack(int pos);
  void calcRookAttack(int pos);
  void calcQueenAttack(int pos);
  void calcKingAttack(int pos);
};
