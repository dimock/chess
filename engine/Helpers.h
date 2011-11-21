#pragma once

#include "BasicTypes.h"
#include "fpos.h"

class Board;

unsigned long xorshf96();

class DebutsTable
{
public:

  DebutsTable();

  void inc() { current_++; }

  bool readTable(const char * fname);

  //StepId findStep(const std::vector<StepId> & steps, Board * board);
  //StepId findStep2(const std::vector<StepId> & steps, Board * board);

private:

  void initStatic();

  typedef std::vector<std::string> StepsList;
  typedef std::vector<StepsList> TableLines;

  TableLines tlines_;
  int current_;
  static const int N = 55;
  static const int M = 13;
  static const char * s_table_[N][M];
};

class PawnMasks
{
public:

  PawnMasks();

  static const uint64 & mask_guarded(int color, int pos)
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pmasks_guarded_[color][pos];
  }

  static const uint64 & mask_backward(int color, int pos)
  {
	  THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
	  return pmasks_backward_[color][pos];
  }

  static const uint64 & mask_passed(int color, int pos)
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pmasks_passed_[color][pos];
  }

  static const uint64 & mask_blocked(int color, int pos)
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pmasks_blocked_[color][pos];
  }

  static const uint64 & mask_isolated(int x)
  {
    THROW_IF( (unsigned)x > 7, "invalid pawn x or color" );
    return pmask_isolated_[x];
  }

private:

  static uint64 pmasks_guarded_[2][64];
  static uint64 pmasks_passed_[2][64];
  static uint64 pmasks_blocked_[2][64];
  static uint64 pmasks_backward_[2][64];
  static uint64 pmask_isolated_[8];
};

class FieldColors
{
public:

  FieldColors();

  static bool isWhite(int8 i)
  {
    THROW_IF( (uint8)i > 63, "invalid position for field color detection" );
    return colors_[i];
  }

private:

  static bool colors_[64];
};

class BitsCounter
{
  int8 array_[256];

  int8 countBits(uint8 byte) const
  {
    int8 n = 0;
    for ( ; byte != 0; byte >>= 1)
    {
      if ( byte&1 )
        n++;
    }
    return n;
  }

public:

  BitsCounter()
  {
    for (int i = 0; i < 256; ++i)
      array_[i] = countBits((uint8)i);
  }

  int operator [] (uint8 byte) const
  {
    return array_[byte];
  }
};

class DeltaPosCounter
{
  FPos array_[4096];

  FPos deltaP(FPos dp) const
  {
    int x = dp.x();
    int y = dp.y();

    if ( x < 0 )
      x = -x;
    if ( y < 0 )
      y = -y;

    if ( x != 0 && y != 0 && x != y )
      return FPos();

    int xx = dp.x(), yy = dp.y();
    if ( xx != 0 )
      xx /= x;
    if ( yy != 0 )
      yy /= y;

    return FPos(xx, yy);
  }

public:

  DeltaPosCounter()
  {
    for (int i = 0; i < 4096; ++i)
    {
      FPos dp = FPosIndexer::get(i >> 6) - FPosIndexer::get(i & 63);
      array_[i] = deltaP(dp);
    }
  }

  inline const FPos & operator [] (int i) const
  {
    return array_[i];
  }
};

//class LengthCounter
//{
//  int array_[256];
//
//  int length_dP(FPos dp) const
//  {
//    int x = dp.x();
//    int y = dp.y();
//
//    if ( x < 0 )
//      x = -x;
//    if ( y < 0 )
//      y = -y;
//
//    if ( x != 0 && y != 0 && x != y )
//      return -1;
//
//    if ( dp.x() != 0 )
//      return x;
//
//    if ( dp.y() != 0 )
//      return y;
//
//    return 0;
//  }
//
//public:
//
//  LengthCounter()
//  {
//    for (int i = 0; i < 256; ++i)
//    {
//      FPos dp;
//      dp.from_byte(i);
//      array_[i] = length_dP(dp);
//    }
//  }
//
//  int operator [] (FPos dp) const
//  {
//    return array_[dp.to_byte()];
//  }
//};

class DistanceCounter
{
  int array_[4096];

  int dist_dP(FPos dp) const
  {
    int x = dp.x();
    int y = dp.y();
    if ( x < 0 )
      x = -x;
    if ( dp.y() < 0 )
      y = -y;
    return std::max(x, y);
  }

public:

  DistanceCounter()
  {
    for (int i = 0; i < 4096; ++i)
    {
      FPos dp = FPosIndexer::get(i >> 6) - FPosIndexer::get(i & 63);
      array_[i] = dist_dP(dp);
    }
  }

  int operator [] (int i) const
  {
    return array_[i];
  }
};

//
//class BorderDistanceCounter
//{
//  int array_[64];
//
//  int dist_dP(FPos p) const
//  {
//    if ( !p )
//      return 0;
//    int dist = p.x();
//    if ( 7-p.x() < dist )
//      dist = 7-p.x();
//    if ( p.y() < dist )
//      dist = p.y();
//    if ( 7-p.y() < dist )
//      dist = 7-p.y();
//    return dist;
//  }
//
//public:
//
//  BorderDistanceCounter()
//  {
//    for (int i = 0; i < 64; ++i)
//    {
//      array_[i] = dist_dP(FPosIndexer::get(i));
//    }
//  }
//
//  int operator [] (int i) const
//  {
//    return array_[i];
//  }
//};
//////////////////////////////////////////////////////////////////////////

inline int numBitsInByte(uint8 byte)
{
  static BitsCounter bcounter;
  return bcounter[byte];
}

inline int numBitsInWord(uint16 word)
{
  return numBitsInByte(word&0xff) + numBitsInByte(word>>8);
}

/// returns { 0, 0 }  if (to - from) isn't horizontal, vertical or diagonal
inline const FPos & getDeltaPos(int to, int from)
{
  static DeltaPosCounter dp_counter;
  THROW_IF( to < 0 || to > 63 || from < 0 || from > 63, "invalid points given" );
  return dp_counter[(to<<6) | from];
}

//// returns '-1' if dp isn't horizontal, vertical or diagonal
//inline int getLength8(FPos dp)
//{
//  static LengthCounter length_counter;
//  return length_counter[dp];
//}

// returns distance between 2 points - 'a' & 'b'
inline int getDistance(int a, int b)
{
  static DistanceCounter distance_counter;
  THROW_IF( a < 0 || a > 63 || b < 0 || b > 63, "invalid points given" );
  return distance_counter[(a<<6) | b];
}

// returns distance from point to the nearest border
//inline int distToBorder(int p)
//{
//  static BorderDistanceCounter distance_counter;
//  return distance_counter[p];
//}


//bool moveToStrShort(const StepId & sid, char * str);
//bool moveToStr(const StepId & sid, char * str);
//bool strToMove(char * str, Board * board, StepId & sid);
//bool formatMove(StepId & sid, char * str);

struct Move;
bool parseSAN(Board & board, const char * str, Move & move);
bool printSAN(Board & board, int i, char * str);