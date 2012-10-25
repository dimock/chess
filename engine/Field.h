#pragma once
/*************************************************************
  Field.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Figure.h"

class Board;

#pragma pack (push, 1)

__declspec (align(1)) class Field
{
public:
  Field() : color_(0), type_(0)
  {
  }

  inline operator bool () const { return type_ != 0; }
  inline Figure::Type  type() const { return (Figure::Type)type_; }
  inline Figure::Color color() const { return (Figure::Color)color_; }

  inline void set(const Figure::Type type, const Figure::Color color)
  {
    THROW_IF( *this, "try to put figure to occupied field");

    color_ = color;
    type_  = type;
  }

  inline void clear()
  {
    *reinterpret_cast<uint8*>(this) = 0;
  }

private:

  uint8 color_ : 1,
        type_  : 3;
};

#pragma pack (pop)
