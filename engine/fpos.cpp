#include "fpos.h"

FPos FPosIndexer::s_fromIndex_[64];

FPosIndexer::FPosIndexer()
{
  for (int i = 0; i < 64; ++i)
  {
    s_fromIndex_[i] = FPos(i&7, i>>3);
  }
}
