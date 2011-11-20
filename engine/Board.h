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
  enum { PawnIndex, KnightIndex = 8, BishopIndex = 10, RookIndex = 12, QueenIndex = 14, KingIndex = 15, NumOfFigures = 16, NumOfFields = 64, MovesMax = 256, GameLength = 4096 };

  bool operator != (const Board & ) const;

  /// c-tor
  Board();

  /// initialize from FEN
  bool initialize(const char * fen);

  /*! movements
   */
  /// really make move. perform validation
  bool makeMove(const Move & );

  /// called after makeMove
  void unmakeMove();

  /// generate movements from this position. don't verify and sort them. only calculate sort weights. returns number of moves found
  int  generateMoves(Move (&)[MovesMax]);
  /*! end of movements */

  /// returns position evaluation that depends on color
  WeightType evaluate() const;

  /// always use this method to get figure
  inline const Figure & getFigure(Figure::Color color, int index) const
  {
    THROW_IF( (size_t)index >= NumOfFigures, "\"Figure & getFigure(Figure::Color, int) const\" - try to get invalid figure");
    return figures_[color][index];
  }

  /// always use this method to get figure
  inline Figure & getFigure(Figure::Color color, int index)
  {
    THROW_IF( (size_t)index >= NumOfFigures, "\"Figure & getFigure(Figure::Color, int) const\" - try to get invalid figure");
    return figures_[color][index];
  }

  /// always use this method to get field
  inline const Field & getField(int index) const
  {
    THROW_IF((unsigned)index > 63, "field index is invalid" );
    return fields_[index];
  }

  /// always use this method to get field
  inline Field & getField(int index)
  {
    THROW_IF((unsigned)index > 63, "field index is invalid" );
    return fields_[index];
  }

  /// returns number of moves. starts from 1
  int movesCount() const { return movesCounter_; }

  /// returns number of half-moves from beginning of game. starts from 0
  int halfmovesCount() const { return halfmovesCounter_; }

  const MoveCmd & getMove(int i) const
  {
    THROW_IF( i < 0 || i >= GameLength, "there was no move" );
    return moves_[i];
  }

  /// returns current move color
  Figure::Color getColor() const { return color_; }

  /// returns current state, ie check, mat etc
  State getState() const { return state_; }

  /// just a useful method to quickly check if there is a draw
  static bool isDraw(State state)
  {
    return Stalemat == state || DrawReps == state || DrawInsuf == state || Draw50Moves == state;
  }

  inline bool drawState() const { return isDraw(state_); }

  void zeroMovesFound();

  void restoreState(State state) { state_ = state; }

  /// methods
private:

  /// calculates absolute position evaluation
  WeightType calculateEval() const;
  //WeightType evaluatePawns(Figure::Color color, int stage) const;
  WeightType evaluateWinnerLoser() const;

  /// do move. fill undo info, don't validate
  bool doMove();

  /// undo move. restore old state after doMove
  void undoMove();

  /// verify position after movement
  inline bool wasMoveValid(const MoveCmd & move) const
  {
    if ( UnderCheck == move.old_state_ )
      return wasValidUnderCheck(move);
    else
      return wasValidWithoutCheck(move);
  }

  bool verifyChessDraw();

  /// gets index of figure, attacking from given direction
  int getAttackedFrom(Figure::Color color, int apt) const;

  bool wasValidUnderCheck(const MoveCmd & ) const;

  bool wasValidWithoutCheck(const MoveCmd & ) const;

  /// return true if current color is checking
  bool isChecking(MoveCmd &) const;

  /// is field 'pos' attacked by given color?
  bool isAttacked(const Figure::Color c, int pos) const;

  // returns number of checking figures
  int findCheckingFigures(Figure::Color color, int pos);

  /// verify position and calculate checking figures
  bool invalidate();

  /// add new figure. firstly find empty slot (index). try to put pawn to slots 0-7, knight to slots 8-9 etc...
  bool addFigure(const Figure &);

  /// set figure with given index to given position
  void setFigure(const Figure &);

  /// clear board. remove all figures
  void clear();

  /// data
private:

  /// index of castle's possibility
  bool castle_index_[2][2];

  /// 0 - no castle, 1 - short, 2 - long
  uint8 castle_[2];

  /// indices of checking figures
  int8 checking_[2];

  /// for chess draw detector
  bool can_win_[2];

  /// game stage - opening, middle-game, etc...
  uint8 stages_[2];

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

  int fiftyMovesCount_, halfmovesCounter_, movesCounter_;

  static MoveCmd moves_[GameLength];
};