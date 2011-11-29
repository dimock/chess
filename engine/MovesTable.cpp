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
    s_pawnsCaps_[color][pos] = 0;

  for (int type = 0; type < 8; ++type)
    s_otherCaps_[type][pos] = 0;
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
        s_pawnsCaps_[color][pos] |= 1ULL << FiguresCounter::s_transposeIndex_[s_tablePawn_[color][pos][i]];
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
