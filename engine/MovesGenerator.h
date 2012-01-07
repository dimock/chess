#pragma once

#include "Board.h"

class Player;
class ChecksGenerator;

struct History
{
  History() : score_(0), good_count_(0), bad_count_(0) {}

  void clear()
  {
    score_ = 0;
    good_count_ = 0;
    bad_count_ = 0;
  }

  ScoreType score_;
  int good_count_, bad_count_;
};

/// generate all movies from this position. don't verify and sort them. only calculate sort weights
class MovesGenerator
{
  friend class ChecksGenerator;

  static History history_[64][64];

public:

  MovesGenerator(Board & , int depth, int ply, Player * player, ScoreType & alpha, ScoreType betta, int & counter, bool null_move, bool reduction);
  MovesGenerator(Board & );

  static inline History & history(int8 from, int8 to)
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
  int generate(ScoreType & alpha, ScoreType betta, int & counter, bool null_move, bool reduction);
  bool movement(ScoreType & alpha, ScoreType betta, const Move & move, int & counter, bool null_move, bool reduction);

  void MovesGenerator::add_move(int & m, int8 from, int8 to, int8 rindex, int8 new_type)
  {
    Move & move = moves_[m++];
    move.set(from, to, rindex, new_type, 0);
    calculateWeight(move);
  }

  void calculateWeight(Move & move);

  int current_;
  int numOfMoves_;
  Player * player_;
  Board & board_;
  Move killer_;
  int depth_;
  int ply_;
  Move moves_[Board::MovesMax];
};

/// generate captures and promotions to queen only, don't detect checks
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



/// generate captures and promotions to queen only, or captures/promotions with checks
class CapsChecksGenerator
{
public:

  CapsChecksGenerator(Board & , Figure::Type minimalType, int depth, int ply, Player &, ScoreType & alpha, ScoreType betta, int & counter, bool null_move);

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

  inline bool maybeCheck(const Figure & fig, const uint64 & mask_all, const uint64 & brq_mask, const Figure & oking)
  {
    bool checking = false;
    uint64 from_msk_inv = ~(1ULL << fig.where());
    uint64 chk_msk = board_.g_betweenMasks->from(oking.where(), fig.where()) & (brq_mask & from_msk_inv);
    uint64 mask_all_inv_ex = ~(mask_all & from_msk_inv);
    for ( ; !checking && chk_msk; )
    {
      int n = least_bit_number(chk_msk);

      const Field & field = board_.getField(n);
      THROW_IF( !field || field.color() != board_.color_, "invalid checking figure found" );

      const Figure & cfig = board_.getFigure(board_.color_, field.index());
      if ( board_.g_figureDir->dir(cfig, oking.where()) < 0 )
        continue;

      const uint64 & btw_msk = board_.g_betweenMasks->between(cfig.where(), oking.where());
      if ( (btw_msk & mask_all_inv_ex) != btw_msk )
        continue;

      checking = true;
    }
    return checking;
  }


  /// returns number of moves found
  int generate(ScoreType & alpha, ScoreType betta, int & counter, bool null_move);
  bool movement(ScoreType & alpha, ScoreType betta, const Move & move, int & counter, bool null_move);


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
  int ply_, depth_;
  Move captures_[Board::MovesMax];
};


/// generate all moves, that escape from check
class EscapeGenerator
{
public:

  EscapeGenerator(const Move & pv, Board &, int depth, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter);
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
  int push_pv();
  int generate(ScoreType & alpha, ScoreType betta, int & counter);
  int generateUsual(ScoreType & alpha, ScoreType betta, int & counter);
  int generateKingonly(int m, ScoreType & alpha, ScoreType betta, int & counter);

  bool add_escape(int & m, int8 from, int8 to, int8 rindex, int8 new_type)
  {
    Move & move = escapes_[m];
    move.set(from, to, rindex, new_type, 0);

    if ( move == pv_ )
      return true;

    if ( !board_.isMoveValidUnderCheck(move) )
      return false;

    move.checkVerified_ = 1;
    ++m;

    return true;
  }


  int current_;
  int numOfMoves_;

  Player & player_;
  int ply_;
  int depth_;

  Move pv_;

  Board & board_;
  Move escapes_[Board::MovesMax];
};

// generate only checks without captures and promotions
class ChecksGenerator
{
public:

  ChecksGenerator(Board &, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter);

  Move & check()
  {
	  Move * move = checks_ + numOfMoves_;
	  Move * mv = checks_;
	  for ( ; *mv; ++mv)
	  {
		  if ( mv->alreadyDone_ || mv->score_ < move->score_ )
			  continue;

		  move = mv;
	  }
	  move->alreadyDone_ = 1;
	  return *move;
  }

private:

  inline bool maybeCheck(const Figure & fig, const uint64 & mask_all, const uint64 & brq_mask, const Figure & oking)
  {
    bool checking = false;
    uint64 from_msk_inv = ~(1ULL << fig.where());
    uint64 chk_msk = board_.g_betweenMasks->from(oking.where(), fig.where()) & (brq_mask & from_msk_inv);
    uint64 mask_all_inv_ex = ~(mask_all & from_msk_inv);
    for ( ; !checking && chk_msk; )
    {
      int n = least_bit_number(chk_msk);

      const Field & field = board_.getField(n);
      THROW_IF( !field || field.color() != board_.color_, "invalid checking figure found" );

      const Figure & cfig = board_.getFigure(board_.color_, field.index());
      if ( board_.g_figureDir->dir(cfig, oking.where()) < 0 )
        continue;

      const uint64 & btw_msk = board_.g_betweenMasks->between(cfig.where(), oking.where());
      if ( (btw_msk & mask_all_inv_ex) != btw_msk )
        continue;

      checking = true;
    }
    return checking;
  }

  int generate(ScoreType & alpha, ScoreType betta, int & counter);

  // returns true if there was betta pruning
  bool do_check(ScoreType & alpha, ScoreType betta, int8 from, int8 to, Figure::Type new_type, int & counter);

  void add_check(int & m, int8 from, int8 to, Figure::Type new_type)
  {
	  Move & move = checks_[m++];
	  move.set(from, to, -1, new_type, 0);
	  move.score_ = MovesGenerator::history_[move.from_][move.to_].score_;
  }

  Player & player_;
  int ply_;

  int numOfMoves_;

  Board & board_;
  Move checks_[Board::MovesMax];
};
