
#include "Player.h"

SearchResult::SearchResult() :
  movesCount_(0),
  totalMoves_(0),
  forcedMoves_(0),
  additionalMoves_(0),
  nullMovesCount_(0),
  depth_(0),
  score_(0)
{
  best_.clear();
  for (int i = 0; i < MaxDepth; ++i)
    pv_[i].clear();
}

Player::Player()
{
}

bool Player::initialize(const char * fen)
{
  return board_.initialize(fen);
}

bool Player::findMove(SearchResult & sres)
{
  sres = SearchResult();
  stop_ = true;
  return false;
}
