/*************************************************************
  MovesTable.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "MovesTable.h"
#include "Figure.h"


//////////////////////////////////////////////////////////////////////////
MovesTable::MovesTable()
{
  for (int i = 0; i < 64; ++i)
  {
    resetAllTables(i);

    initPawns(i);
    initKnights(i);
    initKings(i);
    initBishops(i);
    initRooks(i);
    initQueens(i);
  }

  // pawn promotions
  for (int color = 0; color < 2; ++color)
  {
    s_pawnPromotions_t_[color] = 0;
    s_pawnPromotions_o_[color] = 0;

    int y = color ? 6 : 1;
    for (int x = 0; x < 8; ++x)
    {
      int pp = x | (y << 3);

      // fill ordinary promotion mask
      s_pawnPromotions_o_[color] |= 1ULL << pp;

      // fill transposed promotion mask
      s_pawnPromotions_t_[color] |= 1ULL << FiguresCounter::s_transposeIndex_[pp];
    }
  }
}

void MovesTable::resetAllTables(int pos)
{
  for (int color = 0; color < 2; ++color)
  {
    for (int i = 0; i < 6; ++i)
      s_tablePawn_[color][pos][i] = -1;
  }

  for (int i = 0; i < 10; ++i)
  {
    s_tableKnight_[pos][i] = -1;
    s_tableKing_[pos][i] = -1;

    for(int j = 0; j < 4; ++j)
      s_tableOther_[j][pos][i] = 0;
  }

  for (int color = 0; color < 2; ++color)
  {
    s_pawnsCaps_t_[color][pos] = 0;
    s_pawnsCaps_o_[color][pos] = 0;
    s_pawnsMoves_[color][pos] = 0;
    s_pawnsFrom_[color][pos] = 0;
  }

  for (int type = 0; type < 8; ++type)
    s_otherCaps_[type][pos] = 0;

  s_bishopMob_[pos] = 0;
  s_rookMob_[pos] = 0;
}

//////////////////////////////////////////////////////////////////////////
void MovesTable::initPawns(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, 1), FPos(1, 1), FPos(0, 1), FPos(0, 2) };

  for (int color = 0; color < 2; ++color)
  {
    //if ( p.y() == 0 || p.y() == 7 )
    //  continue;

    bool first = color ? p.y() == 1 : p.y() == 6;
    int n = first ? 4 : 3;
    s_tablePawn_[color][pos][3] = -1;
    for (int i = 0; i < n; ++i)
    {
      FPos d = color ? dpos[i] : FPos(dpos[i].x(), -dpos[i].y());
      FPos q = p + d;
      if ( q )
        s_tablePawn_[color][pos][i] = q.index();
      else
        s_tablePawn_[color][pos][i] = -1;
    }

    // fill captures masks
    for (int i = 0; i < 2; ++i)
    {
      if ( s_tablePawn_[color][pos][i] >= 0 )
      {
        int8 & pp = s_tablePawn_[color][pos][i];
        s_pawnsCaps_o_[color][pos] |= 1ULL << pp;
        s_pawnsCaps_t_[color][pos] |= 1ULL << FiguresCounter::s_transposeIndex_[pp];
      }
    }

    // fill moves mask
    int8 * ptable  = s_tablePawn_[color][pos] + 2;
    for ( ; *ptable >= 0; ++ptable)
    {
      s_pawnsMoves_[color][pos] |= 1ULL << *ptable;
    }

    // fill 'from' mask
    if ( p.y() == 0 || p.y() == 7 )
      continue;

    {
      int y_from[2] = { -1, -1 };

      if ( color )
      {
        if ( p.y() == 1 )
          continue;

        y_from[0] = p.y() - 1;
        if ( p.y() == 3 )
          y_from[1] = 1;
      }
      else
      {
        if ( p.y() == 6 )
          continue;

        y_from[0] = p.y() + 1;
        if ( p.y() == 4 )
          y_from[1] = 6;
      }

      for (int i = 0; i < 2; ++i)
      {
        if ( y_from[i] >= 0 )
        {
          int index = p.x() | (y_from[i] << 3);
          s_pawnsFrom_[color][pos] |= 1ULL << index;
        }
      }
    }
  }
}

void MovesTable::initKnights(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-2, 1), FPos(-1, 2), FPos(1, 2), FPos(2, 1), FPos(2, -1), FPos(1, -2), FPos(-1, -2), FPos(-2, -1) };

  int j = 0;
  for (int i = 0; i < 8; ++i)
  {
    const FPos & d = dpos[i];
    FPos q = p + d;
    if ( !q )
      continue;
    s_tableKnight_[pos][j++] = q.index();
  }

  // fill captures masks
  for (int i = 0; i < 8 && s_tableKnight_[pos][i] >= 0; ++i)
    s_otherCaps_[Figure::TypeKnight][pos] |= 1ULL << s_tableKnight_[pos][i];
}

void MovesTable::initKings(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, 0), FPos(-1, 1), FPos(0, 1), FPos(1, 1), FPos(1, 0), FPos(1, -1), FPos(0, -1), FPos(-1, -1) };

  int j = 0;
  for (int i = 0; i < 8; ++i)
  {
    const FPos & d = dpos[i];
    FPos q = p + d;
    if ( !q )
      continue;
    s_tableKing_[pos][j++] = q.index();
  }

  // fill captures masks
  for (int i = 0; i < 8 && s_tableKing_[pos][i] >= 0; ++i)
    s_otherCaps_[Figure::TypeKing][pos] |= 1ULL << s_tableKing_[pos][i];
}

void MovesTable::initBishops(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, -1), FPos(-1, 1), FPos(1, 1), FPos(1, -1) };

  int j = 0;
  for (int i = 0; i < 4; ++i)
  {
    const FPos & d = dpos[i];
    int n = 0;
    for (FPos q = p + d; q; ++n, q += d)
    {
      // fill captures mask
      s_otherCaps_[Figure::TypeBishop][pos] |= 1ULL << q.index();
    }

    if ( !n )
      continue;

    s_tableOther_[Figure::TypeBishop-Figure::TypeBishop][pos][j++] = (d.delta() << 8) | (n);

    int poffset = (p + d).index();
    s_bishopMob_[pos] |= 1ULL << poffset;
  }
}

void MovesTable::initRooks(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, 0), FPos(0, 1), FPos(1, 0), FPos(0, -1) };

  int j = 0;
  for (int i = 0; i < 4; ++i)
  {
    const FPos & d = dpos[i];
    int n = 0;
    for (FPos q = p + d; q; ++n, q += d)
    {
      // fill captures masks
      s_otherCaps_[Figure::TypeRook][pos] |= 1ULL << q.index();
    }
    
    if ( !n )
      continue;

    s_tableOther_[Figure::TypeRook-Figure::TypeBishop][pos][j++] = (d.delta() << 8) | (n);

    int poffset = (p + d).index();
    s_rookMob_[pos] |= 1ULL << poffset;
  }
}

void MovesTable::initQueens(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, 0), FPos(-1, 1), FPos(0, 1), FPos(1, 1), FPos(1, 0), FPos(1, -1), FPos(0, -1), FPos(-1, -1) };

  int j = 0;
  for (int i = 0; i < 8; ++i)
  {
    const FPos & d = dpos[i];
    int n = 0;
    for (FPos q = p + d; q; ++n, q += d)
    {
      // fill captures masks
      s_otherCaps_[Figure::TypeQueen][pos] |= 1ULL << q.index();
    }

    if ( !n )
      continue;
    s_tableOther_[Figure::TypeQueen-Figure::TypeBishop][pos][j++] = (d.delta() << 8) | (n);
  }
}
