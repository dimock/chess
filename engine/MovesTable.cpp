#include "MovesTable.h"

int8  MovesTable::s_tablePawn_[2][64][4];
int8  MovesTable::s_tableKnight_[64][8];
int8  MovesTable::s_tableKing_[64][8];
uint16 MovesTable::s_tableOther_[4][64][8];

static MovesTable s_movesTable;

//////////////////////////////////////////////////////////////////////////
class FPos
{
  int x_, y_;

public:

  FPos(int x, int y) : x_(x), y_(y) {}
  FPos(int idx) { x_ = idx & 7; y_ = idx >> 3; }
  FPos() : x_(0), y_(0) {}

  int x() const { return x_; }
  int y() const { return y_; }

  FPos & operator += (const FPos & p)
  {
    x_ += p.x_;
    y_ += p.y_;
    return *this;
  }

  FPos & operator -= (const FPos & p)
  {
    x_ -= p.x_;
    y_ -= p.y_;
    return *this;
  }

  FPos operator + (const FPos & p) const
  {
    FPos q(*this);
    q += p;
    return q;
  }

  FPos operator - (const FPos & p) const
  {
    FPos q(*this);
    q -= p;
    return q;
  }

  bool operator == (const FPos & p) const
  {
    return x_ == p.x_ && y_ == p.y_;
  }

  bool operator != (const FPos & p) const
  {
    return x_ != p.x_ || y_ != p.y_;
  }

  int8 index() const
  {
    THROW_IF( !*this, "try to get index from invalid position" );
    THROW_IF( (x_ | (y_<<3)) != (x_ + (y_<<3)), "invalid figure index aquired" );
    return x_ | (y_<<3);
  }

  int8 delta() const
  {
    return x_ + (y_ << 3);
  }

  operator bool () const
  {
    return x_ >= 0 && x_ < 8 && y_ >= 0 && y_ < 8;
  }
};
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
    s_tableKnight_[pos][j] = q.index();
  }
  for ( ; j < 8; ++j)
    s_tableKnight_[pos][j] = -1;
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
    s_tableKing_[pos][j] = q.index();
  }
  for ( ; j < 8; ++j)
    s_tableKing_[pos][j] = -1;
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
