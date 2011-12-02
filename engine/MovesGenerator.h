#pragma once

#include "Board.h"

/// generate all movies from this position. don't verify and sort them. only calculate sort weights
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

  int count() const
  {
    return numOfMoves_;
  }

  bool verify(const Move (&caps)[Board::MovesMax], const Move (&quiets)[Board::MovesMax]) const;

private:

  /// returns number of moves found
  int generate();

  int current_;
  int numOfMoves_;

  Board & board_;
  Move moves_[Board::MovesMax];
};


class Player;

/// generate captures and promotions to queen only
class CapsGenerator
{
public:

  CapsGenerator(Board & , Figure::Type minimalType, Player &, ScoreType & alpha, ScoreType betta);

  Move & capture()
  {
    return captures_[current_++];
  }

  operator bool () const
  {
    return numOfMoves_ > 0;
  }

  int count() const
  {
    return numOfMoves_;
  }

  const Move (&caps() const)[Board::MovesMax]
  {
    return captures_;
  }

private:

  /// returns number of moves found
  int generate(ScoreType & alpha, ScoreType betta);

  int current_;
  int numOfMoves_;
  Figure::Type minimalType_;
  Player & player_;

  Board & board_;
  Move captures_[Board::MovesMax];
};


/// generate all but captures/promotions
class QuietGenerator
{
public:

  QuietGenerator(Board &);

  Move & quiet()
  {
    return quiets_[current_++];
  }

  operator bool () const
  {
    return numOfMoves_ > 0;
  }

  int count() const
  {
    return numOfMoves_;
  }

  const Move (&quiets() const)[Board::MovesMax]
  {
    return quiets_;
  }

private:

  /// returns number of moves found
  int generate();

  int current_;
  int numOfMoves_;

  Board & board_;
  Move quiets_[Board::MovesMax];
};