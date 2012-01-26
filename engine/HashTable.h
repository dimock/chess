#pragma once

#include "BasicTypes.h"

__declspec (align(16)) struct GeneralHItem
{
  GeneralHItem() : hcode_(0), score_(0), depth_(0), color_(0), flag_(0), ply_(0), threat_(0), halfmovesCount_(0)
  {}

  operator bool () const { return hcode_ != 0; }

  uint64     hcode_;
  ScoreType  score_;
  uint32     color_ : 1,
             depth_ : 6,
             ply_ : 7,
             flag_ : 2,
             threat_ : 1;

  uint8      halfmovesCount_;

  PackedMove move_;

  PackedMove tmove_;

#ifdef USE_HASH_MOVE_EX
  PackedMove move_ex_[2];
#endif
};


__declspec (align(16)) struct CaptureHItem
{
  CaptureHItem() : hcode_(0), score_(0), color_(0), flag_(0), depth_(0), ply_(0), halfmovesCount_(0)
  {}

  operator bool () const { return hcode_ != 0; }

  uint64     hcode_;
  ScoreType  score_;
  uint16     color_ : 1,
             flag_ : 2,
             ply_ : 7,
             depth_ : 6;

  uint8      halfmovesCount_;

  PackedMove move_;
};


template <class ITEM>
class HashTable
{

public:

  enum Flag { None, AlphaBetta, Alpha, Betta };

  HashTable(int size) : buffer_(0)
  {
    resize(size);
  }

  ~HashTable()
  {
    delete [] buffer_;
  }

  void resize(int size)
  {
    delete [] buffer_;
    buffer_ = 0;
    szMask_ = 0;
    size_ = size;

    THROW_IF((unsigned)size > 24, "hash table size if too big");

    buffer_ = new ITEM[ONE_SIZE_T<<size_];
    szMask_ = (ONE_SIZE_T<<size_) - 1;
  }

  size_t size() const
  {
    return (ONE_SIZE_T<<size_) - 1;
  }

  ITEM & operator [] (const uint64 & code)
  {
    return buffer_[code & szMask_];
  }

  bool load(const char * fname)
  {
    FILE * f = fopen(fname, "rb");
    if ( !f )
      return false;
    size_t n = 0;
    if ( fread(&size_, sizeof(size_), 1, f) == 1 && size_ > 0 && size_ <= 24 )
    {
      delete [] buffer_;
      buffer_ = new ITEM[ONE_SIZE_T<<size_];
      szMask_ = (ONE_SIZE_T<<size_) - 1;
      n = fread(buffer_, sizeof(ITEM), size(), f);
    }
    fclose(f);
    return n == size();
  }

  bool save(const char * fname) const
  {
    FILE * f = fopen(fname, "wb");
    if ( !f )
      return false;
    size_t n = 0;
    if ( fwrite(&size_, sizeof(size_), 1, f) == 1 )
    {
      n = fwrite(buffer_, sizeof(ITEM), size(), f);
    }
    fclose(f);
    return n == size();
  }

private:

  ITEM * buffer_;
  int size_;
  uint32 szMask_;
};

class GeneralHashTable : public HashTable<GeneralHItem>
{
public:

  GeneralHashTable(int size) : HashTable<GeneralHItem>(size)
  {}

  void push(const uint64 & hcode, ScoreType s, int depth, int ply, int halfmovesCount, Figure::Color color, Flag flag, const PackedMove & move)
  {
    GeneralHItem & hitem = (*this)[hcode];
    //if ( depth < hitem.depth_ || (Alpha == flag && (AlphaBetta == hitem.flag_ || Betta == hitem.flag_)) )

	  if ( (depth < (int)hitem.depth_) || (Alpha == flag && hitem.flag_ != None && s >= hitem.score_) ||
		     (depth == (int)hitem.depth_ && Alpha == flag && (hitem.flag_ == AlphaBetta || hitem.flag_ == Betta)) )
    {
      // we are going to skip this item, check is it to old so we could overwrite it
      bool overwrite = (int)(hitem.halfmovesCount_+hitem.depth_) < halfmovesCount+depth-HalfnodesCountToOverwrite && hitem.hcode_ != hcode;

      if ( !overwrite )
        return;

      hitem.threat_ = 0;
      hitem.tmove_ = PackedMove();
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
    else
      hitem.threat_ = 0;
  }
};

class CapturesHashTable : public HashTable<CaptureHItem>
{
public:

  CapturesHashTable(int size) : HashTable<CaptureHItem>(size)
  {}

  void push(const uint64 & hcode, ScoreType s, Figure::Color color, Flag flag, int depth, int ply, int halfmovesCount, const PackedMove & move)
  {
    CaptureHItem & hitem = operator [] (hcode);

    if ( Alpha == flag && (AlphaBetta == hitem.flag_ || Betta == hitem.flag_) )
    {
      // if we are going to return, check if we could overwrite this item
      bool overwrite = hitem.halfmovesCount_ < halfmovesCount-HalfnodesCountToOverwrite && hitem.hcode_ != hcode;

      if ( !overwrite )
        return;
    }

    hitem.hcode_ = hcode;
    hitem.score_ = s;
    hitem.color_ = color;
    hitem.flag_  = flag;
    hitem.depth_ = depth;
    hitem.ply_   = ply;
    hitem.halfmovesCount_ = halfmovesCount;

    if ( Alpha != flag )
      hitem.move_ = move;
  }
};