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

  void clear()
  {
    move_.clear();
    mask_ = 0;
    score_ = 0;
  }

  uint64     hcode_;
  ScoreType  score_;

  union {

  struct
  {
  uint16     depth_  : 6,
             flag_   : 2,
             threat_ : 1,
             mode_   : 2;
  };

  uint16     mask_;
  };

  uint8      movesCount_;

  PackedMove move_;
};

__declspec (align(16)) 
struct HBucket
{
  static const int BucketSize = 4;

  const HItem * find(const uint64 & hcode) const
  {
    for (int i = 0; i < BucketSize; ++i)
    {
      if ( items_[i].hcode_ == hcode )
        return items_ + i;
    }
    return 0;
  }

  HItem * get(const uint64 & hcode)
  {
    uint8 movesCount = +std::numeric_limits<uint8>::max();
    HItem * hfar = 0;
    HItem * hempty = 0;
    for (int i = 0; i < BucketSize; ++i)
    {
      if ( items_[i].hcode_ == hcode )
        return items_ + i;

      if ( !items_[i].hcode_ && !hempty )
        hempty = items_ + i;
      
      if ( !hfar || (int)items_[i].movesCount_ < movesCount )
      {
        movesCount = items_[i].movesCount_;
        hfar = items_ + i;
      }
    }

    if ( hempty )
      return hempty;

    return hfar;
  }

  HItem items_[BucketSize];
};

template <class ITEM>
class HashTable2
{

public:

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

protected:

  ITEM & operator [] (const uint64 & code)
  {
    return buffer_[code & szMask_];
  }

  const ITEM & operator [] (const uint64 & code) const
  {
    return buffer_[code & szMask_];
  }

  ITEM * buffer_;
  int size_;
  uint32 szMask_;
  uint8 movesCount_;
};

class GHashTable : public HashTable2<HBucket>
{
public:

  enum Flag { NoFlag, Alpha, AlphaBetta, Betta };
  enum Mode { NoMode, General, Capture, Eval };

  GHashTable(int size) : HashTable2<HBucket>(size)
  {}

  void push(const uint64 & hcode, ScoreType score, int depth, Flag flag, const PackedMove & move, bool threat)
  {
    HBucket & hb = (*this)[hcode];
    HItem * hitem = hb.get(hcode);
    if( !hitem )
      return;

    if ( (hitem->mode_ == General) &&
         (hitem->hcode_ == hcode) &&
          ( (hitem->depth_ > depth) || 
            (depth == hitem->depth_ && Alpha == flag && hitem->flag_ > Alpha) ) )
    {
      return;
    }

    THROW_IF(score > 32760, "wrong value to hash");

    if ( hitem->hcode_ && hitem->hcode_ != hcode )
      hitem->clear();

    hitem->hcode_ = hcode;
    hitem->score_ = score;
    hitem->depth_ = depth;
    hitem->flag_  = flag;
    hitem->mode_  = General;
    hitem->threat_ = threat;
    hitem->movesCount_ = movesCount_;

    if ( move )
      hitem->move_ = move;
  }

  void pushCap(const uint64 & hcode, ScoreType score, Flag flag, const PackedMove & move)
  {
    HBucket & hb = (*this)[hcode];
    HItem * hitem = hb.get(hcode);
    if( !hitem )
      return;

    if ( hitem->hcode_ == hcode &&
         ( hitem->mode_ == General || hitem->mode_ == Eval ||
           Alpha == flag && hitem->flag_ > Alpha ) )
    {
      return;
    }

    if ( hitem->hcode_ && hitem->hcode_ != hcode )
      hitem->clear();

    hitem->hcode_ = hcode;
    hitem->score_ = score;
    hitem->flag_  = flag;
    hitem->mode_  = Capture;
    hitem->movesCount_ = movesCount_;

    if ( move )
      hitem->move_ = move;
  }

  const HItem * find(const uint64 & hcode) const
  {
    const HBucket & hb = this->operator [] (hcode);
    const HItem * hitem = hb.find(hcode);
    return hitem;
  }
};
