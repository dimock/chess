#pragma once

#include "Figure.h"
#include "Field.h"
#include "Move.h"

/*! board representation
 */
class Board
{
public:

  /// constants
  enum State { Invalid, Ok, Castle, UnderCheck, Stalemat, DrawReps, DrawInsuf, Draw50Moves, ChessMat };
  enum { PawnIndex, KnightIndex = 8, BishopIndex = 10, RookIndex = 12, QueenIndex = 14, KingIndex = 15, NumOfFigures = 16, NumOfFields = 64, MovesMax = 256 };

  /// c-tor
  Board();

  /// initialize from FEN
  bool initialize(const char * fen);

  /// make movement. fill undo info
  bool makeMove(MoveCmd & );

  /// undo move. restore old state
  void unmakeMove(MoveCmd & );

  /// generate movements from this position. don't verify and sort them. only calculate sort weights. returns index of best move
  int  generateMoves(Move (&)[MovesMax]);

  inline const Figure & getFigure(Figure::Color color, int index) const
  {
    THROW_IF( (size_t)index >= NumOfFigures, "\"Figure & getFigure(Figure::Color, int) const\" - try to get invalid figure");
    return figures_[color][index];
  }

  inline Figure & getFigure(Figure::Color color, int index)
  {
    THROW_IF( (size_t)index >= NumOfFigures, "\"Figure & getFigure(Figure::Color, int) const\" - try to get invalid figure");
    return figures_[color][index];
  }

  inline const Field & getField(int index) const
  {
    THROW_IF((unsigned)index > 63, "field index is invalid" );
    return fields_[index];
  }

  inline Field & getField(int index)
  {
    THROW_IF((unsigned)index > 63, "field index is invalid" );
    return fields_[index];
  }

  int movesCount() const { return movesCounter_; }

  Figure::Color getColor() const { return color_; }

  State getState() const { return state_; }

  static bool isDraw(State state)
  {
    return Stalemat == state || DrawReps == state || DrawInsuf == state || Draw50Moves == state;
  }


private:

  bool addFigure(const Figure &);
  void setFigure(const Figure &);


  void clear();

  /// 0 - no castle, 1 - short, 2 - long
  uint8 castle_[2];

  /// indices of checking figures
  int8 checking_[2];

  /// number of checking figures
  int8 checkingNum_;

  /// en-passant pawn index. must be cleared (set to -1) after move. it has color different from "color_"
  int8  en_passant_;

  /// current state, i.e check, draw, mat, invalid, etc.
  State  state_;

  /// color to make move from this position
  Figure::Color color_;

  /// holds number of figures of each color, hash key, masks etc.
  FiguresManager fmgr_;

  /// figures array 2 x 16
  Figure figures_[2][NumOfFigures];

  /// fields array 8 x 8
  Field fields_[NumOfFields];

  int fiftyMovesCount_, movesCounter_;
};