#pragma once

#include "BasicTypes.h"

class MovesTable
{
  int8   s_tablePawn_[2][64][6];
  int8   s_tableKnight_[64][10];
  int8   s_tableKing_[64][10];
  uint16 s_tableOther_[4][64][10];

  // masks only for captures
  uint64 s_pawnsCaps_[2][64];
  uint64 s_otherCaps_[8][64];

  void resetAllTables(int);

  void initPawns(int);
  void initKnights(int);
  void initKings(int);
  void initBishops(int);
  void initRooks(int);
  void initQueens(int);

public:

  MovesTable();

  inline int8 * pawn(int color, int pos)
  {
    THROW_IF((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move for invalid position, color");
    return s_tablePawn_[color][pos];
  }

  inline int8 * knight(int pos)
  {
    THROW_IF((unsigned)pos > 63, "try to get knight move for invalid position");
    return s_tableKnight_[pos];
  }

  inline int8 * king(int pos)
  {
    THROW_IF((unsigned)pos > 63, "try to get king move for invalid position");
    return s_tableKing_[pos];
  }

  inline uint16 * move(int type, int pos)
  {
    THROW_IF((unsigned)type > 2 || (unsigned)pos > 63, "try to get figure move for invalid position or type");
    return s_tableOther_[type][pos];
  }

  // transposed
  inline const uint64 & pawnCaps(int color, int pos)
  {
    THROW_IF((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move for invalid position, color");
    return s_pawnsCaps_[color][pos];
  }

  inline const uint64 & caps(int type, int pos)
  {
    THROW_IF((unsigned)type > 6 || (unsigned)pos > 63, "try to get figure move for invalid position or type");
    return s_otherCaps_[type][pos];
  }
};
