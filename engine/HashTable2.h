#pragma once

/*************************************************************
  HashTable2.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "BasicTypes.h"

__declspec (align(16)) struct HItem
{
  HItem() : hcode_(0), score_(0), mask_(0), movesCount_(0)
  {}

  operator bool () const { return hcode_ != 0; }

  uint64     hcode_;
  ScoreType  score_;

  union {
  uint16     color_  : 1,
             depth_  : 6,
             flag_   : 2,
             threat_ : 1;

  uint16     mask_;
  };

  uint8      movesCount_;

  PackedMove move_;
};

__declspec (align(16)) 
template <int BucketSize = 4>
struct HBucket
{
  HItem * find(const uint64 & hcode)
  {
    for (int i = 0; i < BucketSize; ++i)
    {
      if ( items_[i].hcode_ = hcode )
        return items_ + i;
    }
    return 0;
  }

  HItem * get(const uint64 & hcode, int depth)
  {
    uint8 movesCount = +std::numeric_limits<uint8>::max();
    HItem * hfar = 0;
    HItem * hemp = 0;
    for (int i = 0; i < BucketSize; ++i)
    {
      if ( items_[i].hcode_ == hcode && items_[i].depth_ < depth )
        return items_ + i;

      if ( !items_[i].hcode_ && !hemp )
        hemp = items_ + i;
      
      if ( !hfar || (int)items_[i].movesCount_ < movesCount )
      {
        movesCount = items_[i].movesCount_;
        hfar = items_ + i;
      }

      if ( hemp )
        return hemp;

      return hfar;
    }
  }

  HItem items_[BucketSize];
};

template <class ITEM>
class HashTable2
{

public:

  enum Flag { None, AlphaBetta, Alpha, Betta };

  HashTable2(int size) : buffer_(0)
  {
    resize(size);
  }

  ~HashTable2()
  {
    delete [] buffer_;
  }

  void clear()
  {
    if ( size_ > 0 )
      resize(size_);
  }

  void resize(int size)
  {
    delete [] buffer_;
    buffer_ = 0;
    szMask_ = 0;
    size_ = size;
    movesCount_ = 0;

    THROW_IF((unsigned)size > 24, "hash table size if too large");

    buffer_ = new ITEM[((size_t)1) << size_];
    szMask_ = (((size_t)1) << size_) - 1;
  }

  int size() const
  {
    return (((size_t)1) << size_) - 1;
  }

  ITEM & operator [] (const uint64 & code)
  {
    return buffer_[code & szMask_];
  }

  void inc()
  {
    movesCount_++;
  }

  bool load(const char * fname)
  {
    FILE * f = fopen(fname, "rb");
    if ( !f )
      return false;
    int n = 0;
    if ( fread(&size_, sizeof(size_), 1, f) == 1 && size_ > 0 && size_ <= 24 )
    {
      delete [] buffer_;
      buffer_ = new ITEM[((size_t)1) << size_];
      szMask_ = (((size_t)1) << size_) - 1;
      n = (int)fread(buffer_, sizeof(ITEM), size(), f);
    }
    fclose(f);
    return n == size();
  }

  bool save(const char * fname) const
  {
    FILE * f = fopen(fname, "wb");
    if ( !f )
      return false;
    int n = 0;
    if ( fwrite(&size_, sizeof(size_), 1, f) == 1 )
    {
      n = (int)fwrite(buffer_, sizeof(ITEM), size(), f);
    }
    fclose(f);
    return n == size();
  }

private:

  ITEM * buffer_;
  int size_;
  uint32 szMask_;
  uint8 movesCount_;
};

class GHashTable : public HashTable2<HBucket>
{
public:

  GHashTable(int size) : HashTable2<HBucket>(size)
  {}

  void push(const uint64 & hcode, ScoreType s, int depth, Figure::Color color, Flag flag, const PackedMove & move)
  {
    HBucket & hb = (*this)[hcode];
    HItem * hitem = hb.get(hcode, depth);
    if( !hitem )
      return;

    if ( (hitem->hcode_ == hcode) && (hitem->depth_ > depth || (Alpha == flag && (hitem.flag_ == AlphaBetta || hitem.flag_ == Betta))) )
      return;

    //if ( (hitem->hcode_ == hcode) && 
    //     ((depth < (int)hitem->depth_) || (Alpha == flag && hitem.flag_ != None && s >= hitem.score_) ||
    //      (depth == (int)hitem.depth_ && Alpha == flag && (hitem.flag_ == AlphaBetta || hitem.flag_ == Betta))) )
    //{
    //  //if ( s < hitem.score_ && hitem.hcode_ == hcode )
    //  //  hitem.score_ = s;

    //  return;
    //}

    THROW_IF(s > 32760, "wrong value to hash");

    if ( hitem.hcode_ && hitem.hcode_ != hcode )
    {
      hitem.threat_ = 0;
      hitem.move_ = PackedMove();

#ifdef USE_THREAT_MOVE
      hitem.tmove_ = PackedMove();
#endif

#ifdef USE_HASH_MOVE_EX
      hitem.move_ex_[0] = PackedMove();
      hitem.move_ex_[1] = PackedMove();
#endif
    }

    hitem.hcode_ = hcode;
    hitem.score_ = s;
    hitem.depth_ = depth;
    hitem.color_ = color;
    hitem.flag_  = flag;
    hitem.ply_   = ply;
    hitem.halfmovesCount_ = halfmovesCount;

    if ( flag != Alpha && move && move != hitem.move_ )
    {
#ifdef USE_HASH_MOVE_EX
      // save previously found movies
      if ( hitem.move_ex_[0] == move )
        hitem.move_ex_[0] = hitem.move_;
      else
      {
        hitem.move_ex_[1] = hitem.move_ex_[0];
        hitem.move_ex_[0] = hitem.move_;
      }
#endif
      hitem.move_ = move;
    }
    //else
    //  hitem.threat_ = 0;
  }
};