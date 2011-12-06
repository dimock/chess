#pragma once

#include "Board.h"

class Player;


/// generate all movies from this position. don't verify and sort them. only calculate sort weights
class MovesGenerator
{
public:

  MovesGenerator(Board & , int depth, int ply, Player * player, ScoreType & alpha, ScoreType betta, int & counter);
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

  bool find(const Move & m) const;

private:

  /// returns number of moves found
  int generate(ScoreType & alpha, ScoreType betta, int & counter);

  bool movement(ScoreType & alpha, ScoreType betta, const Move & move, int & counter);

  int current_;
  int numOfMoves_;
  Player * player_;
  Board & board_;
  int depth_;
  int ply_;
  Move moves_[Board::MovesMax];
};


/// generate captures and promotions to queen only
class CapsGenerator
{
public:

  CapsGenerator(Board & , Figure::Type minimalType, int ply, Player &, ScoreType & alpha, ScoreType betta);

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
  bool capture(ScoreType & alpha, ScoreType betta, const Move & move);


  int current_;
  int numOfMoves_;
  Figure::Type minimalType_;
  Player & player_;
  Board & board_;
  int ply_;
  Move captures_[Board::MovesMax];
};


/// generate all moves, that escape from check
class EscapeGenerator
{
public:

  EscapeGenerator(Board &, int depth, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter);

  Move & escape()
  {
    return escapes_[current_++];
  }

  operator bool () const
  {
    return numOfMoves_ > 0;
  }

  int count() const
  {
    return numOfMoves_;
  }

  bool find(const Move & m) const;

private:

  /// returns number of moves found
  int generate(ScoreType & alpha, ScoreType betta, int & counter);
  int generateUsual(ScoreType & alpha, ScoreType betta, int & counter);
  int generateKingonly(int m, ScoreType & alpha, ScoreType betta, int & counter);
  bool escape_movement(int & m, ScoreType & alpha, ScoreType betta, const Move & move, int & counter);


  int current_;
  int numOfMoves_;

  Player & player_;
  int ply_;
  int depth_;

  Board & board_;
  Move escapes_[Board::MovesMax];
};