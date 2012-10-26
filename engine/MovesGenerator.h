#pragma once

/*************************************************************
  MovesGenerator.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "Board.h"
////#include <fstream>

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

  unsigned score() const
  {
    return mul_div(score_, good_count_, bad_count_);
  }

  void normalize(int n)
  {
    score_ >>= n;
    good_count_ >>= n;
    bad_count_ >>= n;
  }

  unsigned score_;
  unsigned good_count_, bad_count_;
  static unsigned history_max_;
};


// base class for all moves generators
class MovesGeneratorBase
{
  static History history_[Board::NumOfFields][Board::NumOfFields];

  static void clear_history();
  static void normalize_history(int n);

  static void save_history(const char * fname);
  static void load_history(const char * fname);

public:

  static inline History & history(int from, int to)
  {
    THROW_IF( (unsigned)from > 63 || (unsigned)to > 63, "invalid history field index" );
    return history_[from][to];
  }

  MovesGeneratorBase(Board & board) : board_(board),
    numOfMoves_(0)
  {}

  operator bool () const
  {
    return numOfMoves_ > 0;
  }

  int count() const
  {
    return numOfMoves_;
  }

  const Move (&moves() const)[Board::MovesMax]
  {
    return moves_;
  }

  bool find(const Move & m) const;
  bool has_duplicates() const;

protected:

  void sortValueOfCap(Move & move)
  {
    const Field & fto = board_.getField(move.to_);
    const Field & ffrom = board_.getField(move.from_);

    Figure::Type vtype = fto.type();

    THROW_IF(vtype != Figure::TypeNone && fto.color() != Figure::otherColor(board_.color_), "invalid color of captured figure");

    // en-passant case
    if ( vtype == Figure::TypeNone && move.to_ == board_.en_passant_ && ffrom.type() == Figure::TypePawn )
    {
#ifdef NDEBUG
      Index ep_pos(board_.en_passant_);
      Index pawn_pos(ep_pos.x(), ep_pos.y() + (board_.color_ ? -8 : +8));
      THROW_IF( board_.getField(pawn_pos).type() != Figure::TypePawn || board_.getField(pawn_pos).color() == board_.color_, "no en-passant pawn" );
#endif
      vtype = Figure::TypePawn;
    }

    THROW_IF( !vtype, "no captured figure" );

    Figure::Type atype = ffrom.type();
    move.vsort_ = Figure::figureWeight_[vtype] - Figure::figureWeight_[atype];

    // we prefer to eat last moved figure at first
    if ( board_.halfmovesCount() > 1 )
    {
      MoveCmd & prev = board_.getMoveRev(-1);
      if ( prev.to_ == move.to_ )
        move.vsort_ += Figure::figureWeight_[vtype] >> 1;
    }
  }

  int numOfMoves_;
  Board & board_;
  Move moves_[Board::MovesMax];
};

/// generate all movies from this position. don't verify and sort them. only calculate sort weights
class MovesGenerator : public MovesGeneratorBase
{
public:

  MovesGenerator(Board & board, const Move & killer);
  MovesGenerator(Board & );

  Move & move()
  {
    for ( ;; )
    {
      Move * move = moves_ + numOfMoves_;
      Move * mv = moves_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ < move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        return *move;

      if ( (move->capture_ >= 0 || move->new_type_ > 0) && !move->seen_ )
      {
        move->seen_ = 1;

        int see_gain = 0;
        if ( (see_gain = board_.see(*move)) < 0 )
        {
          if ( move->capture_ )
            move->vsort_ = see_gain + 3000;
          else
            move->vsort_ = see_gain + 2000;

          continue;
        }

        if ( see_gain >= 0 )
          move->recapture_ = 1;
      }

      move->alreadyDone_ = 1;
      return *move;
    }
  }

private:

  /// returns number of moves found
  int generate();

  inline void add(int & index, int8 from, int8 to, int8 new_type, bool capture)
  {
    Move & move = moves_[index++];
    move.set(from, to, new_type, capture);
    
    calculateSortValue(move);
  }

  void calculateSortValue(Move & move)
  {
    const Field & ffield = board_.getField(move.from_);
    THROW_IF( !ffield, "no figure on field we move from" );

    if ( move.capture_ )
    {
      sortValueOfCap(move);
      move.vsort_ += 10000000;
      return;
    }
    else if ( move.new_type_ )
    {
      move.vsort_ = Figure::figureWeight_[move.new_type_] + 5000000;
      return;
    }
#ifdef USE_KILLER
    else if ( move == killer_ )
    {
      move.vsort_ = 3000000;
      return;
    }
#endif

    const History & hist = history(move.from_, move.to_);
    move.vsort_ = hist.score_ + 10000;
  }

  Move killer_;
};

/// generate captures and promotions to queen only, don't detect checks
class CapsGenerator : public MovesGeneratorBase
{
public:

  CapsGenerator(const Move & hcap, Board & , Figure::Type minimalType);

  inline Move & capture()
  {
    for ( ;; )
    {
      Move * move = moves_ + numOfMoves_;
      Move * mv = moves_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ < move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        return *move;

      if ( !move->seen_ && board_.see(*move) < 0 )
      {
        move->seen_ = 1;
        move->alreadyDone_ = 1;
        continue;
      }

      move->alreadyDone_ = 1;
      return *move;
    }
  }

private:

  /// returns number of moves found
  int generate();

  inline void add(int & m, int8 from, int8 to, int8 new_type, bool capture)
  {
    THROW_IF( !board_.getField(move.from_), "no figure on field we move from" );
    
    Move & move = moves_[m];    
    move.set(from, to, new_type, capture);
    if ( move == hcap_ )
      return;

    if ( capture )
    {
      sortValueOfCap(move);
      move.vsort_ += 1000000;
    }
    else if ( move.new_type_ > 0 )
    {
      move.vsort_ = 500000;
    }
    m++;
 }


  Figure::Type minimalType_;
  Board & board_;
  Move hcap_;
};


/// generate all moves, that escape from check
class EscapeGenerator : public MovesGeneratorBase
{
public:

  EscapeGenerator(const Move & hmove, Board & );

  Move & escape()
  {
    return moves_[current_++];
  }

private:

  /// returns number of moves found
  int push_pv();
  int generate();
  int generateUsual();
  int generateKingonly(int m);

  bool add(int & m, int8 from, int8 to, int8 new_type, bool capture)
  {
    Move & move = moves_[m];
    move.set(from, to, new_type, capture);

    if ( move == hmove_ )
      return true;

    if ( !board_.isMoveValidUnderCheck(move) )
      return false;

    move.checkVerified_ = 1;
    ++m;

    return true;
  }

  int current_;
  Move hmove_;
};

//////////////////////////////////////////////////////////////////////////
// generate all captures with type gt/eq to minimalType
class ChecksGenerator2 : public MovesGeneratorBase
{
public:

  ChecksGenerator2(const Move & hmove, Figure::Type minimalType);

  Move & check()
  {
    for ( ;; )
    {
      Move * move = moves_ + numOfMoves_;
      Move * mv = moves_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ < move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        return *move;

      if ( !move->discoveredCheck_ && !move->seen_ && board_.see(*move) < 0 )
      {
        move->seen_ = 1;
        move->alreadyDone_ = 1;
        continue;
      }

      move->alreadyDone_ = 1;
      return *move;
    }
  }

private:

  int generate();

  inline void add(int & m, int8 from, int8 to, Figure::Type new_type, bool discovered, bool capture)
  {
    Move & move = moves_[m];
    move.set(from, to, new_type, capture);
    if ( move == hmove_ )
      return;

    move.discoveredCheck_ = discovered;

    if ( move.capture_ )
    {
      sortValueOfCap(move);
      move.vsort_ += 10000000;
    }
    else if ( move.new_type_ )
    {
      move.vsort_ = Figure::figureWeight_[move.new_type_] + 5000000;
    }
    else
    {
      const History & hist = history(move.from_, move.to_);
      move.vsort_ = hist.score_;
    }

    m++;
  }

  // add non-processed moves of pawn if it discovers check in caps-loop
  inline void add_other_moves(int & m, int from, int to, int oki_pos)
  {
    const BitMask & from_oki_mask = board_.g_betweenMasks->from(oki_pos, from);

    // not processed captures
    const int8 * table = board_.g_movesTable->pawn(board_.color_, from);
    for (int i = 0; i < 2 && *table >= 0; ++table, ++i)
    {
      if ( *table == to )
        continue;

      const Field & pfield = board_.getField(*table);
      if ( !pfield || pfield.color() == board_.color_ || pfield.type() >= minimalType_ )
        continue;

      // pawn shouldn't cover opponents king in its new position - i.e it shouldn't go to the same line
      if ( (from_oki_mask & (1ULL << *table)) )
        continue;

      // don't add checking pawn's capture, because we add it another way
      int dir = board_.g_figureDir->dir(Figure::TypePawn, board_.color_, *table, oki_pos);
      if ( dir == 0 || dir == 1 )
        continue;

      add(m, from, *table, Figure::TypeNone, true, true /*cap*/);
    }

    // usual moves
    for ( ; *table >= 0 && !board_.getField(*table); ++table)
    {
      // pawn shouldn't cover opponents king in its new position - i.e it shouldn't go to the same line
      if ( from_oki_mask & (1ULL << *table) )
        break;

      add(m, from, *table, Figure::TypeNone, true, false/*no cap*/);
    }
  }

  Figure::Type minimalType_;
  Move hmove_;
};
