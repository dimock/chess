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

public:

  static void clear_history();
  static void normalize_history(int n);

  static void save_history(const char * fname);
  static void load_history(const char * fname);


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

  const Move & operator [] (int i) const
  {
    THROW_IF( (unsigned)i >= (unsigned)numOfMoves_ || numOfMoves_ >= Board::MovesMax, "index of move is out of range" );
    return moves_[i];
  }

  bool find(const Move & m) const;
  bool has_duplicates() const;

protected:

  inline void sortValueOfCap(Move & move)
  {
    const Field & fto = board_.getField(move.to_);
    const Field & ffrom = board_.getField(move.from_);

    Figure::Type vtype = fto.type();

    THROW_IF(vtype != Figure::TypeNone && fto.color() != Figure::otherColor(board_.color_), "invalid color of captured figure");

    // en-passant case
    if ( vtype == Figure::TypeNone && move.to_ == board_.en_passant_ && ffrom.type() == Figure::TypePawn )
    {
      THROW_IF( board_.getField(board_.enpassantPos()).type() != Figure::TypePawn ||
        board_.getField(board_.enpassantPos()).color() == board_.color_, "no en-passant pawn" );

      vtype = Figure::TypePawn;
    }

    // capture, prefer stronger opponent's figure
    if ( vtype != Figure::TypeNone )
    {
      Figure::Type atype = ffrom.type();
      move.vsort_ = Figure::figureWeight_[vtype] - Figure::figureWeight_[atype] + (Figure::figureWeight_[vtype] >> 2);
    }

    // pawn promotion
    if ( move.new_type_ > Figure::TypeNone && move.new_type_ < Figure::TypeKing )
    {
      move.vsort_ += Figure::figureWeight_[move.new_type_];
    }

    // at first we try to eat recently moved opponent's figure
    if ( move.capture_ && board_.halfmovesCount() > 1 )
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

      if ( (move->capture_ || move->new_type_ > 0) && !move->seen_ )
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

  inline void add(int & index, int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    Move & move = moves_[index++];
    move.set(from, to, new_type, capture);
    
    calculateSortValue(move);
  }

  void calculateSortValue(Move & move)
  {
    const Field & ffield = board_.getField(move.from_);
    THROW_IF( !ffield, "no figure on field we move from" );

    if ( move.capture_ || move.new_type_ )
    {
      sortValueOfCap(move);
      move.vsort_ += 10000000;
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

/// generate all moves but captures and promotion to queen. could generate only promotion to knight if it checks
/// couldn't be called under check !!!
class UsualGenerator : public MovesGeneratorBase
{
public:

  UsualGenerator(Board & );

  /// returns number of moves found
  int generate(const Move & hmove, const Move & killer);

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

      move->alreadyDone_ = 1;
      return *move;
    }
  }

private:

  inline bool add(int & index, int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    Move & move = moves_[index];
    move.set(from, to, new_type, capture);

    if ( move == hmove_ )
      return false;

    index++;
    calculateSortValue(move);
    return true;
  }

  inline void calculateSortValue(Move & move)
  {
    const Field & ffield = board_.getField(move.from_);
    THROW_IF( !ffield, "no figure on field we move from" );

#ifdef USE_KILLER
    if ( move == killer_ )
    {
      move.vsort_ = 3000000;
      return;
    }
#endif

    const History & hist = history(move.from_, move.to_);
    move.vsort_ = hist.score_ + 10000;
  }

  Move hmove_, killer_;
};

/// generate captures and promotions to queen only, don't detect checks
class CapsGenerator : public MovesGeneratorBase
{
public:

  CapsGenerator(Board & );
  CapsGenerator(const Move & hcap, Board & , Figure::Type minimalType);

  int generate(const Move & hcap, Figure::Type minimalType);

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

  inline void add(int & m, int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    THROW_IF( !board_.getField(from), "no figure on field we move from" );
    
    Move & move = moves_[m];    
    move.set(from, to, new_type, capture);
    if ( move == hcap_ )
      return;

    sortValueOfCap(move);
    move.vsort_ += 10000; // to prevent negative values
    m++;
 }


  Figure::Type minimalType_;
  Move hcap_;
};

/// generate all moves, that escape from check
class EscapeGenerator : public MovesGeneratorBase
{
public:

  EscapeGenerator(Board &);
  EscapeGenerator(const Move & hmove, Board & );

  int generate(const Move & hmove);

  Move & escape()
  {
    return moves_[current_++];
  }

  const Move & operator [] (int i) const
  {
    return moves_[i];
  }

private:

  /// returns number of moves found
  int generate();
  int generateUsual();
  int generateKingonly(int m);

  void add(int & m, int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    Move & move = moves_[m];
    move.set(from, to, new_type, capture);
    move.checkVerified_ = 1;

    if ( move == hmove_ && m > 0 )
      std::swap(move, moves_[0]);

    ++m;
  }

  int current_;
  Move hmove_;
};

/// first use move from hash, then generate all captures and promotions to queen, at the last generate other moves
/// generates all valid moves if under check
class FastGenerator
{
public:
  FastGenerator(Board & board, const Move & hmove, const Move & killer);

  Move & move();

  int count() const
  {
    return eg_.count();
  }

private:

  enum GOrder
  {
    oHash, oEscapes, oGenCaps, oCaps, oGenUsual, oUsual, oWeak
  } order_;

  CapsGenerator cg_;
  UsualGenerator ug_;
  EscapeGenerator eg_;

  Move caps_[Board::MovesMax];
  Move weak_[Board::MovesMax];

  int numCaps_, numWeak_;

  Move hmove_, killer_, fake_;
  Board & board_;
};

//////////////////////////////////////////////////////////////////////////
// generate all captures with type gt/eq to minimalType
class ChecksGenerator : public MovesGeneratorBase
{
public:

  ChecksGenerator(const Move & hmove, Board & board, Figure::Type minimalType);

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

    if ( move.capture_ || move.new_type_ )
    {
      sortValueOfCap(move);
      move.vsort_ += 10000000;
    }
    else
    {
      const History & hist = history(move.from_, move.to_);
      move.vsort_ = hist.score_;
    }

    m++;
  }

  // add non-processed moves of pawn if it discovers check in caps-loop
  inline void add_other_pawn_moves(int & m, int from, int to, int oki_pos)
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

      add(m, from, *table, Figure::TypeNone, true /* discovered */, true /*cap*/);
    }

    // usual moves
    for ( ; *table >= 0 && !board_.getField(*table); ++table)
    {
      // pawn shouldn't cover opponents king in its new position - i.e it shouldn't go to the same line
      if ( from_oki_mask & (1ULL << *table) )
        break;

      add(m, from, *table, Figure::TypeNone, true /* discovered */, false/*no cap*/);
    }
  }

  Figure::Type minimalType_;
  Move hmove_;
};
