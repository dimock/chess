#pragma once

#include "BasicTypes.h"

#pragma pack (push, 1)

class FPos
{
  int8 x_, y_;

public:

  FPos(int8 idx) { x_ = idx & 7; y_ = idx >> 3; }
  FPos(int8 x, int8 y) : x_(x), y_(y) {}
  FPos() : x_(0), y_(0) {}

  int8 x() const { return x_; }
  int8 y() const { return y_; }

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

  operator bool () const
  {
    return x_ >= 0 && x_ < 8 && y_ >= 0 && y_ < 8;
  }

  // may be < 0
  int8 delta() const
  {
    return x_ + (y_ << 3);
  }

  bool is_white() const
  {
    return (x_ + y_) & 1;
  }
};


class FPosIndexer
{
  static FPos s_fromIndex_[64];

public:

  FPosIndexer();

  static inline const FPos & get(int8 i)
  {
    THROW_IF( i < 0 || i > 63, "invalid index" );
    return s_fromIndex_[i];
  }
};

#pragma pack (pop)
