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

  int score_;
  int good_count_, bad_count_;
  static int history_max_;
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

  int hist_max() const
  {
	  return history_max_;
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

  inline void MovesGenerator::add_move(int & m, int8 from, int8 to, int8 rindex, int8 new_type)
  {
    Move & move = moves_[m++];
    move.set(from, to, rindex, new_type, 0);
    calculateWeight(move);
  }

  inline void calculateWeight(Move & move)
  {
    const Field & ffield = board_.getField(move.from_);
    THROW_IF( !ffield, "no figure on field we move from" );

	const History & hist = history_[move.from_][move.to_];
	if ( hist.bad_count_ )
		move.score_ = hist.score_*hist.good_count_/hist.bad_count_;
	else
		move.score_ = hist.score_;

	if ( move.score_ > history_max_ )
		history_max_ = move.score_;

    if ( move.rindex_ >= 0 )
    {
      const Figure & rfig = board_.getFigure(Figure::otherColor(board_.color_), move.rindex_);
      move.score_ = (int)Figure::figureWeight_[rfig.getType()] - (int)Figure::figureWeight_[ffield.type()] + ((int)rfig.getType()<<4) + 1000000;
    }
    else if ( move.new_type_ > 0 )
    {
      move.score_ = (int)Figure::figureWeight_[move.new_type_] - (int)Figure::figureWeight_[Figure::TypePawn] + 800000;
    }
#ifdef USE_KILLER
    else if ( move == killer_ )
    {
      move.score_ = 400000;
	  move.fkiller_ = 1;
    }
#endif
  }

  int current_;
  int numOfMoves_;
  Player * player_;
  Board & board_;
  Move killer_;
  int depth_;
  int ply_;
  Move moves_[Board::MovesMax];
  int history_max_;
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

  bool find(const Move & move) const
  {
    for (int i = 0; i < numOfMoves_; ++i)
    {
      if ( move == captures_[i] )
        return true;
    }
    return false;
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

  void calculateWeight(Move & move) const
  {
    const Field & ffield = board_.getField(move.from_);
    THROW_IF( !ffield, "no figure on field we move from" );
	
	const History & hist = MovesGenerator::history(move.from_, move.to_);
	if ( hist.bad_count_ > 0 )
		move.score_ = hist.score_*hist.good_count_/hist.bad_count_;
	else
		move.score_ = hist.score_;

    if ( move.rindex_ >= 0 )
    {
      const Figure & rfig = board_.getFigure(Figure::otherColor(board_.color_), move.rindex_);
      move.score_ += (int)Figure::figureWeight_[rfig.getType()] - (int)Figure::figureWeight_[ffield.type()] + ((int)rfig.getType()<<4) + 1000000;
    }
    else if ( move.new_type_ > 0 )
    {
      move.score_ += Figure::figureWeight_[move.new_type_] - Figure::figureWeight_[Figure::TypePawn] + 500000;
    }
  }


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

// generate all captures with type gt/eq to minimalType
class ChecksGenerator
{
public:

  ChecksGenerator(CapsGenerator * cg, Board &, int ply, Player & player, ScoreType & alpha, ScoreType betta, Figure::Type minimalType, int & counter);

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

  inline bool maybeCheck(int8 pt, const uint64 & mask_all, const uint64 & brq_mask, const Figure & oking)
  {
    bool checking = false;
    uint64 from_msk_inv = ~(1ULL << pt);
    uint64 chk_msk = board_.g_betweenMasks->from(oking.where(), pt) & (brq_mask & from_msk_inv);
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

  void add_check(int & m, int8 from, int8 to, int8 rindex, Figure::Type new_type)
  {
	  Move & move = checks_[m];
	  move.set(from, to, rindex, new_type, 0);
    
    if ( rindex >= 0 && cg_ && cg_->find(move) )
      return;

	const History & hist = MovesGenerator::history(move.from_, move.to_);
	if ( hist.bad_count_ > 0 )
		move.score_ = hist.score_*hist.good_count_/hist.bad_count_;
	else
		move.score_ = hist.score_;
    ++m;
  }

  Player & player_;
  int ply_;

  CapsGenerator * cg_;
  Figure::Type minimalType_;

  Move killer_;
  int numOfMoves_;

  Board & board_;
  Move checks_[Board::MovesMax];
};
