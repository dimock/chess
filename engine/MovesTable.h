#pragma once

#include "BasicTypes.h"

class MovesTable
{
  static int8   s_tablePawn_[2][64][4];
  static int8   s_tableKnight_[64][8];
  static int8   s_tableKing_[64][8];
  static uint16 s_tableOther_[4][64][8];

  void initPawns(int);
  void initKnights(int);
  void initKings(int);
  void initBishops(int);
  void initRooks(int);
  void initQueens(int);

public:

  MovesTable();

  static int8 * pawn(int color, int pos)
  {
    THROW_IF((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move for invalid position, color");
    return s_tablePawn_[color][pos];
  }

  static int8 * knight(int pos)
  {
    THROW_IF((unsigned)pos > 63, "try to get knight move for invalid position");
    return s_tableKnight_[pos];
  }

  static int8 * king(int pos)
  {
    THROW_IF((unsigned)pos > 63, "try to get king move for invalid position");
    return s_tableKing_[pos];
  }

  static uint16 * move(int type, int pos)
  {
    THROW_IF((unsigned)type > 2 || (unsigned)pos > 63, "try to get figure move for invalid position or type");
    return s_tableOther_[type][pos];
  }
};