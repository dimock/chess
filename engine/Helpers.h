#pragma once

/*************************************************************
  Helpers.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "BasicTypes.h"
#include "fpos.h"

class Board;

// got from chessprogramming.wikispaces.com
inline bool one_bit_set(uint64 n)
{
	return (n & (n-1)) == 0ULL;
}

inline int pop_count(uint64 n)
{
  if ( n == 0ULL )
    return 0;
  else if ( one_bit_set(n) )
    return 1;
  n =  n - ((n >> 1)  & 0x5555555555555555ULL);
  n = (n & 0x3333333333333333ULL) + ((n >> 2) & 0x3333333333333333ULL);
  n = (n + (n >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
  n = (n * 0x0101010101010101ULL) >> 56;
  return (int)n;
}

inline BitMask set_mask_bit(int bit)
{
  return 1ULL << bit;
}

inline int set_bit(int bit)
{
  return 1 << bit;
}

#ifdef _MSC_VER
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)

inline int _lsb32(unsigned long n)
{
  unsigned long i;
  uint8 b = _BitScanForward(&i, n);
  THROW_IF( !b, "no bit found in nonzero number" );
  return i;
}

inline int _msb32(unsigned long n)
{
  unsigned long i;
  uint8 b = _BitScanReverse(&i, n);
  THROW_IF( !b, "no bit found in nonzero number" );
  return i;
}
#elif (defined __GNUC__)
inline int _lsb32(unsigned long n)
{
  unsigned long i = __bsfd(n);
  THROW_IF( !n, "number should be non-zero in _lsb32" );
  return i;
}

inline int _msb32(unsigned long n)
{
  unsigned long i = __bsrd(n);
  THROW_IF( !n, "number should be non-zero in _msb32" );
  return i;
}
#endif

#ifdef _M_X64

#ifdef __GNUC__
#pragma (message "compiling with x64")
#endif

#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)

inline int _lsb64(const uint64 & mask)
{
  unsigned long n;
  uint8 b = _BitScanForward64(&n, mask);
  THROW_IF( !b, "no bit found in nonzero number" );
  return n;
}

inline int _msb64(const uint64 & mask)
{
  unsigned long n;
  uint8 b = _BitScanReverse64(&n, mask);
  THROW_IF( !b, "no bit found in nonzero number" );
  return n;
}

inline int log2(uint64 n)
{
  unsigned long i = 0;
  if ( _BitScanReverse64(&i, n) )
    return i;
  return 0;
}

#else

#ifdef _MSC_VER
inline int _lsb64(const uint64 & mask)
{
  unsigned long n;
  const unsigned * pmask = reinterpret_cast<const unsigned int *>(&mask);

  if ( _BitScanForward(&n, pmask[0]) )
    return n;

  uint8 b = _BitScanForward(&n, pmask[1]);
  THROW_IF( !b, "no bit found in nonzero number" );
  return n+32;
}

inline int _msb64(const uint64 & mask)
{
  unsigned long n;
  const unsigned * pmask = reinterpret_cast<const unsigned int * >(&mask);

  if ( _BitScanReverse(&n, pmask[1]) )
    return n+32;

  uint8 b = _BitScanReverse(&n, pmask[0]);
  THROW_IF( !b, "no bit found in nonzero number" );
  return n;
}

inline int log2(uint64 n)
{
  unsigned long i = 0;
  const unsigned * pn = reinterpret_cast<const unsigned int *>(&n);

  if ( _BitScanReverse(&i, pn[1]) )
    return i+32;

  if ( _BitScanReverse(&i, pn[0]) )
    return i;

  return 0;
}
#elif (defined __GNUC__)
inline int _lsb64(const uint64 & mask)
{
  const unsigned * pmask = reinterpret_cast<const unsigned int *>(&mask);
  if ( pmask[0] )
    return __bsfd(pmask[0]);

  THROW_IF( !pmask[1], "number should be non-zero in _lsb64" );
  return __bsfd(pmask[1])+32;
}

inline int _msb64(const uint64 & mask)
{
  const unsigned * pmask = reinterpret_cast<const unsigned int * >(&mask);
  if ( pmask[1] )
    return __bsrd(pmask[1])+32;

  THROW_IF( !pmask[0], "number should be non-zero in _msb64" );
  return __bsrd(pmask[0]);
}

inline int log2(uint64 n)
{
  const unsigned * pn = reinterpret_cast<const unsigned int *>(&n);
  if ( pn[1] )
    return __bsrd(pn[1]) +32;

  if ( pn[0] )
    return __bsrd(pn[0]);

  return 0;
}
#endif

#endif

inline int clear_lsb(uint64 & mask)
{
	unsigned long n = _lsb64(mask);
	mask &= mask-1;
	return n;
}

inline int clear_msb(uint64 & mask)
{
  unsigned long n = _msb64(mask);
  mask ^= set_mask_bit(n);
  return n;
}

inline unsigned mul_div(unsigned v, unsigned n, unsigned d)
{
  uint64 r = (uint64)v * n;
  r >>= log2((uint64)(d) + (uint64)(n));
  unsigned x = *((unsigned*)&r);
  return x;
}

class PawnMasks
{
public:

  PawnMasks();

  inline const BitMask & mask_guarded(int color, int pos) const
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pmasks_guarded_[color][pos];
  }

  inline const BitMask & mask_disconnected(int pos) const
  {
	  THROW_IF( (unsigned)pos > 63, "invalid pawn pos" );
	  return pmasks_disconnected_[pos];
  }

  inline const BitMask & mask_passed(int color, int pos) const
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pmasks_passed_[color][pos];
  }

  inline const BitMask & mask_blocked(int color, int pos) const
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pmasks_blocked_[color][pos];
  }

  inline const BitMask & mask_isolated(int x) const
  {
    THROW_IF( (unsigned)x > 7, "invalid pawn x or color" );
    return pmask_isolated_[x];
  }


  inline int8 pawn_dst_color(int color, int pos) const
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pawn_dst_color_[color][pos];
  }

  inline const BitMask & mask_kpk(int color, int p) const
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)p > 63, "invalid pawn pos or color" );
    return pmask_kpk_[color][p];
  }

private:

  void clearAll(int);

  BitMask pmasks_guarded_[2][64];
  BitMask pmasks_passed_[2][64];
  BitMask pmasks_blocked_[2][64];
  BitMask pmasks_disconnected_[64];
  BitMask pmask_isolated_[8];
  BitMask pmask_kpk_[2][64];
  int8    pawn_dst_color_[2][64];
};

class BitsCounter
{
  static int8 s_array_[256];

public:

  static inline int8 numBitsInByte(uint8 byte)
  {
    return s_array_[byte];
  }

  static inline int8 numBitsInWord(uint16 word)
  {
    return numBitsInByte(word&0xff) + numBitsInByte(word>>8);
  }
};

class DeltaPosCounter
{
  FPos s_array_[4096];

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

  DeltaPosCounter();

  /// returns { 0, 0 }  if (to - from) isn't horizontal, vertical or diagonal
  inline const FPos & getDeltaPos(int to, int from) const
  {
    THROW_IF( to < 0 || to > 63 || from < 0 || from > 63, "invalid points given" );
    return s_array_[(to<<6) | from];
  }
};


class BetweenMask
{
  // masks between two fields
  BitMask s_between_[64][64];

  // mask 'from' field in direction 'to'. until border
  BitMask s_from_[64][64];

  // mask from field in given direction up to the border
  BitMask s_from_dir_[8][64];

public:
  
  BetweenMask(DeltaPosCounter *);

  // mask contains only bits BETWEEN from & to
  inline const BitMask & between(int8 from, int8 to) const
  {
    THROW_IF( (uint8)from > 63 || (uint8)to > 63, "invalid positions given" );
    return s_between_[from][to];
  }

  // mask contains bits along from-to direction starting from and finishing at border
  inline const BitMask & from(int8 from, int8 to) const
  {
    THROW_IF( (uint8)from > 63 || (uint8)to > 63, "invalid positions given" );
    return s_from_[from][to];
  }

  // mask contains bits from square in dir direction  up to the border
  inline const BitMask & from_dir(int8 from, nst::dirs dir) const
  {
    THROW_IF( (uint8)from > 63 || (uint8)dir > 8 || dir == nst::no_dir, "invalid positions given" );
    return s_from_dir_[(uint8)dir-1][from];
  }
};


class DistanceCounter
{
  int s_array_[4096];

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

  DistanceCounter();

  // returns distance between 2 points - 'a' & 'b'
  inline int getDistance(int a, int b) const
  {
    THROW_IF( a < 0 || a > 63 || b < 0 || b > 63, "invalid points given" );
    return s_array_[(a<<6) | b];
  }
};

//////////////////////////////////////////////////////////////////////////

struct Move;

enum eMoveNotation
{
	mnUnknown, mnSmith, mnSAN
};

eMoveNotation detectNotation(const char * str);

/// e2e4 - Smith notation
bool moveToStr(const Move & move, char * str, bool full);
bool strToMove(const char * i_str, const Board & board, Move & move);

/// Rxf5+ - Standard algebraic notation
bool parseSAN(const Board & board, const char * str, Move & move);
bool printSAN(Board & board, const Move & move, char * str);