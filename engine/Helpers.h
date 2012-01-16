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

#ifndef _M_X64
// return least significant bit index, clear it
inline int least_bit_number(uint64 & mask)
{
  int n = -1;
  __asm
  {
    ; scan lower dword

    mov edi, mask
    mov eax, dword ptr [edi]
    bsf ecx, eax
    jz  nxt
    mov ebx, 1
    shl ebx, cl
    xor eax, ebx
    mov dword ptr [edi], eax
    jmp end


    ; scan upper dword

nxt:mov eax, dword ptr [edi+4]
    bsf ecx, eax
    mov ebx, 1
    shl ebx, cl
    xor eax, ebx
    mov dword ptr [edi+4], eax
    add ecx, 32

end:mov dword ptr [n], ecx
  }
  return n;
}
#else
#pragma intrinsic(_BitScanForward64)
inline int least_bit_number(uint64 & mask)
{
	unsigned long n;
	uint8 b = _BitScanForward64(&n, mask);
	THROW_IF( !b, "no bit found in nonzero number" );
	mask &= mask-1;
	//mask ^= 1ULL << n;
	return n;
}
#endif

class PawnMasks
{
public:

  PawnMasks();

  inline const uint64 & mask_guarded(int color, int pos) const
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pmasks_guarded_[color][pos];
  }

  inline const uint64 & mask_backward(int pos) const
  {
	  THROW_IF( (unsigned)pos > 63, "invalid pawn pos" );
	  return pmasks_backward_[pos];
  }

  inline const uint64 & mask_passed(int color, int pos) const
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pmasks_passed_[color][pos];
  }

  inline const uint64 & mask_blocked(int color, int pos) const
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pmasks_blocked_[color][pos];
  }

  inline const uint64 & mask_isolated(int x) const
  {
    THROW_IF( (unsigned)x > 7, "invalid pawn x or color" );
    return pmask_isolated_[x];
  }


  inline int8 pawn_dst_color(int color, int pos) const
  {
    THROW_IF( (unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color" );
    return pawn_dst_color_[color][pos];
  }

private:

  void clearAll(int);

  uint64 pmasks_guarded_[2][64];
  uint64 pmasks_passed_[2][64];
  uint64 pmasks_blocked_[2][64];
  uint64 pmasks_backward_[64];
  uint64 pmask_isolated_[8];
  int8   pawn_dst_color_[2][64];
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
  uint64 s_between_[64][64];

  // mask from field in given direction. until border
  uint64 s_from_[64][64];

public:
  
  BetweenMask(DeltaPosCounter *);

  // mask contains only bits BETWEEN from & to
  inline const uint64 & between(int8 from, int8 to) const
  {
    THROW_IF( (uint8)from > 63 || (uint8)to > 63, "invalid positions given" );
    return s_between_[from][to];
  }

  // mask contains bits along from-to direction starting from and finishing at border
  inline const uint64 & from(int8 from, int8 to) const
  {
    THROW_IF( (uint8)from > 63 || (uint8)to > 63, "invalid positions given" );
    return s_from_[from][to];
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

bool moveToStr(const Move & move, char * str, bool full);
bool strToMove(char * str, const Board & board, Move & move);

bool parseSAN(Board & board, const char * str, Move & move);
bool printSAN(Board & board, const Move & move, char * str);