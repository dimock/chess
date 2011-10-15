#pragma once

#include <stdexcept>
#include <vector>
#include <string>
#include <ostream>
#include <istream>
#include <limits>

#undef DONT_USE_EXTS_
#undef USE_EXTRA_QUIS_

#ifndef NDEBUG
  #define THROW_IF(v, msg) if ( v ) throw std::runtime_error(msg); else;
#else
  #define THROW_IF(v, msg)
#endif

typedef signed char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned uint32;
typedef __int64 int64;
typedef unsigned __int64 uint64;

typedef int16 WeightType;

enum { MaxDepth = 48 };


#pragma pack (push, 1)

class FPos
{
  int x_, y_;

public:

  FPos(int x, int y) : x_(x), y_(y) {}
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

  int index() const
  {
    THROW_IF( !*this, "try to get index from invalid position" );
    THROW_IF( (x_ | (y_<<3)) != (x_ + (y_<<3)), "invalid figure index aquired" );
    return x_ | (y_<<3);
  }

  operator bool () const
  {
    return x_ >= 0 && x_ < 8 && y_ >= 0 && y_ < 8;
  }

  bool is_white() const
  {
    return (x_ + y_) & 1;
  }

  uint8 to_byte() const
  {
    THROW_IF( !*this, "try to convert invalid position to byte" );
    return (x_ & 15) | (y_ << 4);
  }

  void from_byte(int i)
  {
    uint8 u = unsigned(i) & 255;
    x_ = u & 15;
    y_ = u >> 4;
  }
};


class FPosIndexer
{
  static FPos s_fromIndex_[64];

public:

  FPosIndexer();

  static inline const FPos & get(int i)
  {
    THROW_IF( i < 0 || i > 63, "invalid index" );
    return s_fromIndex_[i];
  }
};


__declspec (align(1)) class Index
{
public:

  Index() : index_(-1) {}
  Index(int i) : index_(i) {}
  Index(int x, int y) { index_ = x | (y<<3); }

  operator int () const
  {
    return index_;
  }

  operator int8 & ()
  {
    return index_;
  }

  int x() const
  {
    THROW_IF(index_ < 0, "try to get x of invalid index");
    return index_&7;
  }

  int y() const
  {
    THROW_IF(index_ < 0, "try to get y of invalid index");
    return (index_>>3)&7;
  }

  void set_x(int x)
  {
    index_ &= ~7;
    index_ |= x & 7;
  }

  void set_y(int y)
  {
    index_ &= 7;
    index_ |= (y&7)<<3;
  }

private:

  int8 index_;
};

__declspec (align(1)) struct StepId
{
  int32  index_ : 5, // moving figure
         rindex_ : 5, // removing figure
         newType_ : 4, // if change type
         from_ : 7,
         to_ : 7,
         castle_ : 1,
         ftype_ : 3; // type of moving figure

  void clear()
  {
    index_ = -1;
    rindex_ = -1;
    newType_ = 0;
    from_ = -1;
    to_ = -1;
    castle_ = 0;
    ftype_ = 0;
  }

  operator bool () const
  {
    return index_ >= 0;
  }

  bool operator != (const StepId & step) const
  {
	  return *reinterpret_cast<const int32*>(this) != *reinterpret_cast<const int32*>(&step);
  }

  bool operator == (const StepId & step) const
  {
    return *reinterpret_cast<const int32*>(this) == *reinterpret_cast<const int32*>(&step);
  }

  bool invalid()
  {
	  return index_ >= 0 && (from_ < 0 || to_ < 0);
  }

  unsigned ftype() const
  {
	  return ((unsigned)ftype_) & 0x7;
  }

};

__declspec (align(1)) struct Step : public StepId
{
  int8  rookidx_, // rook index for castling
        rook_from_, // old rook field index
        rook_to_, // new rook field index
        oldType_,
        old_state_, // only for Undo
        fake_index_,
        eaten_type_;

  int   weight_;

  int16 need_undo_ : 1,
        first_step_ : 1,
        need_unmake_ : 1,
        maybe_check_ : 1,
        next_;


  // checking figures
  uint8 checkingNum_;
  uint8 checking_[2];

  // unmake info
  uint8 old_checkingNum_;
  uint8 old_checking_[2];

  void clear()
  {
    StepId::clear();

    rookidx_ = -1;
    rook_from_ = -1;
    rook_to_ = -1;
	  next_ = -1;
    weight_ = -std::numeric_limits<int>::max();
    checkingNum_ = 0;
    maybe_check_ = 0;

    // UNDO info

    //old_state_ = 0;
    //oldType_ = 0;
    //need_undo_ = 0;
    //first_step_ = 0;
    //fake_index_ = -1;
    //eaten_type_ = 0;
    //old_checkingNum_ = 0;
  }

  void writeTo(std::ostream & out) const
  {
    out << (int)index_ << " " << (int)rindex_ << " " << (int)rookidx_ << " " << (int)newType_ << " ";
    out << (int)castle_ << " " << (int)checkingNum_ << " " << (int)checking_[0] << " " << (int)checking_[1] << " ";
	  out << (int)from_ << " " << (int)to_ << " " << (int)rook_from_ << " " << (int)rook_to_ << std::endl;
  }

  void readFrom(std::istream & in)
  {
    int index, rindex, rookidx, newType, castle, checking[2], chkNum, from, to, rfrom, rto;
    in >> index >> rindex >> rookidx >> newType >> castle >> chkNum >> checking[0] >> checking[1] >> from >> to >> rfrom >> rto;
    index_ = index;
    rindex_ = rindex;
    rookidx_ = rookidx;
    newType_ = newType;
    castle_ = castle;
    checkingNum_ = chkNum;
    checking_[0] = checking[0];
    checking_[1] = checking[1];
    from_ = from;
    to_ = to;
    rook_from_ = rfrom;
    rook_to_ = rto;
    next_ = -1;
    weight_ = 0;
  }
};

#pragma pack (pop)

