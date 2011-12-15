#pragma once

#include "Board.h"

class Player;


/// generate all movies from this position. don't verify and sort them. only calculate sort weights
class MovesGenerator
{
  static ScoreType history_[64][64];

public:

  MovesGenerator(Board & , int depth, int ply, Player * player, ScoreType & alpha, ScoreType betta, int & counter);
  MovesGenerator(Board & );

  static inline ScoreType & history(int8 from, int8 to)
  {
    THROW_IF( (unsigned)from > 63 || (unsigned)to > 63, "invalid history field index" );
    return history_[from][to];
  }

  static void clear_history();

  Move & move()
  {
    Move * move = moves_ + numOfMoves_;
    Move * mv = moves_;
    for ( ; *mv; ++mv)
    {
      if ( mv->alreadyDone_ || mv->score_ < move->score_ )
        continue;

      move = mv;
    }
    move->alreadyDone_ = 1;
    return *move;
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

  inline void add_move(int & m, int8 from, int8 to, int8 rindex, int8 new_type)
  {
    Move & move = moves_[m++];
    move.set(from, to, rindex, new_type, 0);
    calculateWeight(move);
  }

  bool movement(ScoreType & alpha, ScoreType betta, const Move & move, int & counter);

  void calculateWeight(Move & move);

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

  CapsGenerator(Board & , Figure::Type minimalType, int ply, Player &, ScoreType & alpha, ScoreType betta, int & counter);

  Move & capture()
  {
    Move * move = captures_ + numOfMoves_;
    Move * mv = captures_;
    for ( ; *mv; ++mv)
    {
      if ( mv->alreadyDone_ || mv->score_ < move->score_ )
        continue;

      move = mv;
    }
    move->alreadyDone_ = 1;
    return *move;
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
  int generate(ScoreType & alpha, ScoreType betta, int & counter);
  bool capture(ScoreType & alpha, ScoreType betta, const Move & move, int & counter);


  inline void add_capture(int & m, int8 from, int8 to, int8 rindex, int8 new_type)
  {
    Move & move = captures_[m++];
    move.set(from, to, rindex, new_type, 0);
    calculateWeight(move);
  }

  void calculateWeight(Move & move);

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

// generate only checks without captures and promotions
class ChecksGenerator
{
public:

  ChecksGenerator(Board &, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter);

private:

  // returns true if there was betta pruning
  bool do_check(ScoreType & alpha, ScoreType betta, int8 from, int8 to, int & counter);

  Player & player_;
  int ply_;

  Board & board_;
};
