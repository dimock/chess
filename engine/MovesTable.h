#pragma once

/*************************************************************
  MovesTable.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "BasicTypes.h"

class MovesTable
{
  int8   s_tablePawn_[2][64][6];
  int8   s_tableKnight_[64][10];
  int8   s_tableKing_[64][10];
  uint16 s_tableOther_[4][64][10];

  // masks only for captures
  uint64 s_pawnsCaps_t_[2][64];
  uint64 s_pawnsCaps_o_[2][64];
  uint64 s_otherCaps_[8][64];
  uint64 s_pawnPromotions_t_[2];
  uint64 s_pawnPromotions_o_[2];

  // mobility mask
  uint64 s_bishopMob_[64];
  uint64 s_rookMob_[64];

  void resetAllTables(int);

  void initPawns(int);
  void initKnights(int);
  void initKings(int);
  void initBishops(int);
  void initRooks(int);
  void initQueens(int);

public:

  MovesTable();

  inline const int8 * pawn(int color, int pos) const
  {
    THROW_IF((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move for invalid position, color");
    return s_tablePawn_[color][pos];
  }

  inline const int8 * knight(int pos) const
  {
    THROW_IF((unsigned)pos > 63, "try to get knight move for invalid position");
    return s_tableKnight_[pos];
  }

  inline const int8 * king(int pos) const
  {
    THROW_IF((unsigned)pos > 63, "try to get king move from invalid position");
    return s_tableKing_[pos];
  }

  inline const uint16 * move(int type, int pos) const
  {
    THROW_IF((unsigned)type > 2 || (unsigned)pos > 63, "try to get figure move from invalid position or type");
    return s_tableOther_[type][pos];
  }

  // transposed captures
  inline const uint64 & pawnCaps_t(int color, int pos) const
  {
    THROW_IF((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move from invalid position, color");
    return s_pawnsCaps_t_[color][pos];
  }

  // ordinary captures
  inline const uint64 & pawnCaps_o(int color, int pos) const
  {
    THROW_IF((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move from invalid position or color");
    return s_pawnsCaps_o_[color][pos];
  }

  // transposed promotion mask
  inline const uint64 & promote_t(int color) const
  {
    THROW_IF( (unsigned)color > 1, "invalid color of promotion mask" );
    return s_pawnPromotions_t_[color];
  }

  inline const uint64 & promote_o(int color) const
  {
    THROW_IF( (unsigned)color > 1, "invalid color of promotion mask" );
    return s_pawnPromotions_o_[color];
  }

  inline const uint64 & caps(int type, int pos) const
  {
    THROW_IF((unsigned)type > 6 || (unsigned)pos > 63, "try to get figure move for invalid position or type");
    return s_otherCaps_[type][pos];
  }

  inline const uint64 & bishop_mobility(int pos) const
  {
    THROW_IF((unsigned)pos > 63, "invalid bishop pos");
    return s_bishopMob_[pos];
  }

  inline const uint64 & rook_mobility(int pos) const
  {
    THROW_IF((unsigned)pos > 63, "invalid rook pos");
    return s_rookMob_[pos];
  }
};
