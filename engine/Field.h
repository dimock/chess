#pragma once

#include "Figure.h"

class Board;

#pragma pack (push, 1)

__declspec (align(1)) class Field
{
public:
  Field() : color_(0), type_(0), index_(0)
  {
  }

  inline operator bool () const { return type_ != 0; }
  
  inline int index() const
  {
	  //int8 mask = (type_ - 1) >> 31; // no type: -1; has type: 0
	  //int8 i = index_ | mask;
	  //THROW_IF( (type_ && i != index_ || !type_ && i != -1), "invalid field index calcualated" );
	  //return i;
	  return type_ ? index_ : -1;
  } // OPTIMIZE

  inline Figure::Type  type() const { return (Figure::Type)type_; }
  inline Figure::Color color() const { return (Figure::Color)color_; }

  inline void set(const Figure & fig)
  {
    THROW_IF( *this, "try to put figure to occupied field");

    color_ = fig.getColor();
    type_  = fig.getType();
    index_ = fig.getIndex();
  }

  inline void clear()
  {
    *reinterpret_cast<uint8*>(this) = 0;
  }

private:

  uint8 color_ : 1,
        type_  : 3,
        index_ : 4;
};

#pragma pack (pop)
