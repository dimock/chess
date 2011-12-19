#pragma once

#include "Board.h"

class Player;
class ChecksGenerator;

/// generate all movies from this position. don't verify and sort them. only calculate sort weights
class MovesGenerator
{
  friend class ChecksGenerator;

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

  CapsChecksGenerator(Board & , Figure::Type minimalType, int ply, Player &, ScoreType & alpha, ScoreType betta, int & counter);

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
	  move.score_ = MovesGenerator::history_[move.from_][move.to_];
  }

  //void add_check_knight(int & m, int8 from, int8 to)
  //{
  //  Move & move = checks_[m++];
  //  move.set(from, to, -1, Figure::TypeKnight, 0);
  //  move.score_ = MovesGenerator::history_[move.from_][move.to_];
  //}

  Player & player_;
  int ply_;

  int numOfMoves_;

  Board & board_;
  Move checks_[Board::MovesMax];
};
