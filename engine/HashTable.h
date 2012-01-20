#pragma once

#include "BasicTypes.h"

__declspec (align(16)) struct GeneralHItem
{
  GeneralHItem() : hcode_(0), score_(0), depth_(0), color_(0), flag_(0), ply_(0), counter_(0)
  {}

  operator bool () const { return hcode_ != 0; }

  uint64     hcode_;
  ScoreType  score_;
  uint16     color_ : 1,
             depth_ : 6,
             ply_ : 7,
             flag_ : 2;
  uint8      counter_;

  PackedMove move_;

#ifdef USE_HASH_MOVE_EX
  PackedMove move_ex_[2];
#endif
};


__declspec (align(16)) struct CaptureHItem
{
  CaptureHItem() : hcode_(0), score_(0), color_(0), flag_(0), counter_(0)
  {}

  operator bool () const { return hcode_ != 0; }

  uint64     hcode_;
  ScoreType  score_;
  uint8      color_ : 1,
             flag_ : 2;

  uint8      counter_;
  PackedMove move_;
};


template <class ITEM, int N_ITEMS>
class HashTable
{
	class InternalItem
	{
		ITEM items_[N_ITEMS];

	public:

		ITEM & find(const uint64 & code)
		{
			for (int i = 0; i < N_ITEMS; ++i)
			{
				if ( !items_[i].hcode_ || items_[i].hcode_ == code )
          return items_[i];
			}
			return items_[0];
		}

    ITEM & get(const uint64 & code)
    {
      for (int i = 0; i < N_ITEMS; ++i)
      {
        if ( !items_[i].hcode_ || items_[i].hcode_ == code )
          return items_[i];
      }
      int j = 0;
      for (int i = 1; i < N_ITEMS; ++i)
      {
        if ( items_[i].counter_ < items_[j].counter_ )
          j = i;
      }
      return items_[j];
    }
};

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

    buffer_ = new InternalItem[1<<size_];
    szMask_ = (1<<size_) - 1;
  }

  size_t size() const
  {
    return (1<<size_) - 1;
  }

  ITEM & operator [] (const uint64 & code)
  {
    return buffer_[code & szMask_].find(code);
  }

  ITEM & get(const uint64 & code)
  {
    return buffer_[code & szMask_].get(code);
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
      buffer_ = new InternalItem[1<<size_];
      szMask_ = (1<<size_) - 1;
      n = fread(buffer_, sizeof(InternalItem), size(), f);
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
      n = fwrite(buffer_, sizeof(InternalItem), size(), f);
    }
    fclose(f);
    return n == size();
  }

private:

  InternalItem * buffer_;
  int size_;
  uint32 szMask_;
};

class GeneralHashTable : public HashTable<GeneralHItem, HashItemsN_>
{
public:

  GeneralHashTable(int size) : HashTable<GeneralHItem, HashItemsN_>(size)
  {}

  void push(const uint64 & hcode, ScoreType s, int depth, int ply, Figure::Color color, Flag flag, const PackedMove & move, uint8 counter)
  {
    GeneralHItem & hitem = get(hcode);
	  //if ( depth < hitem.depth_ || (Alpha == flag && (hitem.flag_ == AlphaBetta || hitem.flag_ == Betta)) )
	  if ( (depth < hitem.depth_) || (Alpha == flag && hitem.flag_ != None && s > hitem.score_) ||
		     (depth == hitem.depth_ && Alpha == flag && (hitem.flag_ == AlphaBetta || hitem.flag_ == Betta)) )
      return;

    hitem.hcode_ = hcode;
    hitem.score_ = s;
    hitem.depth_ = depth;
    hitem.color_ = color;
    hitem.flag_  = flag;
    hitem.ply_   = ply;
    hitem.counter_ = counter;

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
  }
};

class CapturesHashTable : public HashTable<CaptureHItem, HashItemsN_>
{
public:

  CapturesHashTable(int size) : HashTable<CaptureHItem, HashItemsN_>(size)
  {}

  void push(const uint64 & hcode, ScoreType s, Figure::Color color, Flag flag, const PackedMove & move, uint8 counter)
  {
    CaptureHItem & hitem = get(hcode);
    if ( Alpha == flag && (AlphaBetta == hitem.flag_ || Betta == hitem.flag_) )
      return;

    hitem.hcode_ = hcode;
    hitem.score_ = s;
    hitem.color_ = color;
    hitem.flag_  = flag;
    hitem.counter_ = counter;

    if ( Alpha != flag )
      hitem.move_ = move;
  }
};