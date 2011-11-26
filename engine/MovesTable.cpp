#include "MovesTable.h"
#include "Figure.h"

int8  MovesTable::s_tablePawn_[2][64][6];
int8  MovesTable::s_tableKnight_[64][10];
int8  MovesTable::s_tableKing_[64][10];
uint16 MovesTable::s_tableOther_[4][64][10];

uint64 MovesTable::s_pawnsCaps_[2][64];
uint64 MovesTable::s_otherCaps_[8][64];


static MovesTable s_movesTable;

//////////////////////////////////////////////////////////////////////////
void MovesTable::initPawns(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, 1), FPos(1, 1), FPos(0, 1), FPos(0, 2) };

  for (int color = 0; color < 2; ++color)
  {
    if ( p.y() == 0 || p.y() == 7 )
    {
      for (int i = 0; i < 4; ++i)
        s_tablePawn_[color][pos][i] = -1;
      continue;
    }
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
    for (int i = n; i < 6; ++i)
      s_tablePawn_[color][pos][i] = -1;

    // fill captures masks
    for (int i = 0; i < 2 && s_tablePawn_[color][pos][i] >= 0; ++i)
      s_pawnsCaps_[color][pos] |= 1ULL << s_tablePawn_[color][pos][i];
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
  for ( ; j < 10; ++j)
    s_tableKnight_[pos][j] = -1;

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
  for ( ; j < 10; ++j)
    s_tableKing_[pos][j] = -1;

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
    for (FPos q = p + d; q; ++n, q += d);
    if ( !n )
      continue;
    s_tableOther_[0][pos][j++] = (d.delta() << 8) | (n);
  }

  // fill captures masks
  for (int i = 0; i < 4 && s_tableOther_[0][pos][i] >= 0; ++i)
    s_otherCaps_[Figure::TypeBishop][pos] |= 1ULL << s_tableOther_[0][pos][i];
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
    for (FPos q = p + d; q; ++n, q += d);
    if ( !n )
      continue;
    s_tableOther_[1][pos][j++] = (d.delta() << 8) | (n);
  }

  // fill captures masks
  for (int i = 0; i < 4 && s_tableOther_[1][pos][i] >= 0; ++i)
    s_otherCaps_[Figure::TypeRook][pos] |= 1ULL << s_tableOther_[1][pos][i];
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
    for (FPos q = p + d; q; ++n, q += d);
    if ( !n )
      continue;
    s_tableOther_[2][pos][j++] = (d.delta() << 8) | (n);
  }

  // fill captures masks
  for (int i = 0; i < 4 && s_tableOther_[2][pos][i] >= 0; ++i)
    s_otherCaps_[Figure::TypeQueen][pos] |= 1ULL << s_tableOther_[2][pos][i];
}

//////////////////////////////////////////////////////////////////////////
MovesTable::MovesTable()
{
  for (int i = 0; i < 64; ++i)
  {
    initPawns(i);
    initKnights(i);
    initKings(i);
    initBishops(i);
    initRooks(i);
    initQueens(i);
  }
}
