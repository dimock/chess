#pragma once

#include "Board.h"

class MovesGenerator
{
public:

  MovesGenerator(Board & );

  Move & move()
  {
    return moves_[current_++];
  }

  operator bool () const
  {
    return numOfMoves_ > 0;
  }

private:

  /// generate movies from this position. don't verify and sort them. only calculate sort weights. returns number of moves found
  int generateMoves();

  int current_;
  int numOfMoves_;

  Board & board_;
  Move moves_[Board::MovesMax];
};