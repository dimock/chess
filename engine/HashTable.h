#pragma once

#include "BasicTypes.h"

class HashTable
{
public:

  enum Flag { None, AlphaBetta, Alpha, Betta };

  __declspec (align(16)) struct HItem
  {
    HItem() : hcode_(0), weight_(0), depth_(0), color_(0), flag_(0), ply_(0)
    {
      stepId_.clear();
    }

    operator bool () const { return hcode_ != 0; }

    uint64     hcode_;
    WeightType weight_;
    uint16     color_ : 1,
               depth_ : 6,
               ply_ : 7,
               flag_ : 2;

    StepId     stepId_;
  };

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

    buffer_ = new HItem[1<<size_];
    szMask_ = (1<<size_) - 1;
  }

  void clear()
  {
    for (int i = 0; i < (1<<size_); ++i)
      buffer_[i] = HItem();
  }

  size_t size() const
  {
    return (1<<size_) - 1;
  }

  HItem & operator [] (uint64 code)
  {
    return buffer_[code & szMask_];
  }

  void write(const uint64 & hcode, WeightType w, int depth, int ply, Figure::Color color, Flag flag, const Step & step)
  {
    HashTable::HItem & hitem = operator [] (hcode);
    if ( depth < hitem.depth_ || (Alpha == flag && (AlphaBetta == hitem.flag_ || Betta == hitem.flag_)) )
      return;

    hitem.hcode_ = hcode;
    hitem.weight_ = w;
    hitem.depth_ = depth;
    hitem.color_ = color;
    hitem.flag_ = flag;
    hitem.ply_ = ply;

    if ( Alpha != flag )
      hitem.stepId_ = step;
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
      buffer_ = new HItem[1<<size_];
      szMask_ = (1<<size_) - 1;
      n = fread(buffer_, sizeof(HItem), size(), f);
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
      n = fwrite(buffer_, sizeof(HItem), size(), f);
    }
    fclose(f);
    return n == size();
  }

private:

  HItem * buffer_;
  int size_;
  uint32 szMask_;
};