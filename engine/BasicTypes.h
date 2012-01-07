#pragma once

#include <stdexcept>
#include <vector>
#include <string>
#include <ostream>
#include <istream>
#include <limits>
#include <stdio.h>
#include <string.h>

#ifdef _M_X64
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#endif

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

typedef int16 ScoreType;

#undef GO_IMMEDIATELY

#define PERFORM_CAPTURES_AND_CHECKS
#define USE_ZERO_WINDOW
#define USE_KILLER
#define USE_FUTILITY_PRUNING
#define USE_DELTA_PRUNING_
#define GO_IMMEDIATELY_CC
#define USE_HASH_TABLE_GENERAL
#define USE_HASH_TABLE_CAPTURE
#define USE_GENERAL_HASH_IN_CAPS
#define RETURN_IF_BETTA
#define USE_NULL_MOVE
#define USE_LMR

#undef NO_TIME_LIMIT

#undef  VERIFY_ESCAPE_GENERATOR
#undef  VERIFY_CHECKS_GENERATOR
#undef  VERIFY_CAPS_GENERATOR

#undef  DO_CHECK_IMMEDIATELY

#ifndef NDEBUG
  #define TIMING_FLAG 0xFFF
#else
  #define TIMING_FLAG 0xFFFF
#endif



enum { MaxPly = 40 };

#ifndef _M_X64
class QpfTimer
{
  int64 t0_;

public:

  QpfTimer()
  {
    __asm
    {
      mov ecx, this
      lea ecx, [ecx]this.t0_
      rdtsc
      mov dword ptr [ecx], eax
      mov dword ptr [ecx+4], edx
    }
  }

  inline int64 ticks()
  {
    int64 t;
    __asm
    {
      lea esi, [t]
      mov edi, this
      lea edi, [edi]this.t0_
      rdtsc
      sub eax, dword ptr [edi]
      sbb edx, dword ptr [edi+4]
      sub eax, 105                ; rdtsc takes 105 ticks
      sbb edx, 0
      mov dword ptr [esi], eax
      mov dword ptr [esi+4], edx
    }
    return t;
  }
};
#else
class QpfTimer
{
	int64 t0_;

public:

	QpfTimer()
	{
		t0_ = __rdtsc();
	}

	inline int64 ticks()
	{
		int64 t;
		t = __rdtsc() - t0_;
		return t;
	}
};
#endif
#pragma pack (push, 1)


__declspec (align(1)) class Index
{
public:

  Index() : index_(-1) {}
  Index(int8 i) : index_(i) {}
  Index(int x, int y) { index_ = x | (y<<3); }
  Index(char x, char y) { index_ = (x-'a') | ((y-'1')<<3); }

  operator int8 () const
  {
    return index_;
  }

  operator int8 & ()
  {
    return index_;
  }

  int8 x() const
  {
    THROW_IF(index_ < 0, "try to get x of invalid index");
    return index_&7;
  }

  int8 y() const
  {
    THROW_IF(index_ < 0, "try to get y of invalid index");
    return (index_>>3)&7;
  }

  void set_x(int8 x)
  {
    index_ &= ~7;
    index_ |= x & 7;
  }

  void set_y(int8 y)
  {
    index_ &= 7;
    index_ |= (y&7)<<3;
  }

private:

  int8 index_;
};

#pragma pack (pop)

