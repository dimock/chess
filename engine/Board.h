#pragma once

#include "Figure.h"
#include "Field.h"
#include "Move.h"
#include "MovesTable.h"
#include "FigureDirs.h"

/*! board representation
 */
class Board
{
public:

  static int64 ticks_;
  static int   tcounter_;

  /// constants
  enum State { Invalid, Ok, Castle, UnderCheck, Stalemat, DrawReps, DrawInsuf, Draw50Moves, ChessMat };
  enum { PawnIndex, KnightIndex = 8, BishopIndex = 10, RookIndex = 12, QueenIndex = 14, KingIndex = 15, NumOfFigures = 16, NumOfFields = 64, MovesMax = 256, FENsize = 512, GameLength = 4096 };

  bool operator != (const Board & ) const;

  /// c-tor
  Board();

  static bool load(Board & , std::istream &);
  static bool save(const Board & , std::ostream &);

  /// init global data
  void set_moves(MoveCmd * moves) { g_moves = moves; }
  void set_MovesTable(MovesTable * movesTable) { g_movesTable = movesTable; }
  void set_FigureDir(FigureDir * figureDir) { g_figureDir = figureDir; }
  void set_PawnMasks(PawnMasks * pawnMasks) { g_pawnMasks = pawnMasks; }
  void set_BetweenMask(BetweenMask * betweenMask) { g_betweenMasks = betweenMask; }
  void set_DeltaPosCounter(DeltaPosCounter * deltaPosCounter) { g_deltaPosCounter = deltaPosCounter; }
  void set_DistanceCounter(DistanceCounter * distanceCounter) { g_distanceCounter = distanceCounter; }

  /// initialize from FEN
  bool fromFEN(const char * fen);

  /// save current position to FEN
  bool toFEN(char * fen) const;

  /// initialize empty board with given color to move
  bool initEmpty(Figure::Color );

  /*! movements
   */
  /// verify 
  bool validMove(const Move &) const;

  /// really make move. perform validation
  bool makeMove(const Move & );

  /// called after makeMove
  void unmakeMove();

  /// generate movements from this position. don't verify and sort them. only calculate sort weights. returns number of moves found
  int  generateMoves(Move (&)[MovesMax]);
  /*! end of movements */

  /// verify if there is draw or mat
  void verifyState();

  /// returns position evaluation that depends on color
  ScoreType evaluate() const;

  /// add new figure. firstly find empty slot (index). try to put pawn to slots 0-7, knight to slots 8-9 etc...
  bool addFigure(const Figure &);

  /// set figure with given index to given position
  void setFigure(const Figure &);

  /// verify position and calculate checking figures
  /// very slow. should be used only while initialization
  bool invalidate();

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

  inline int8 getEnPassant() const
  {
    return en_passant_;
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
    return g_moves[i];
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

  inline void setNoMoves()
  {
    if ( Invalid == state_ || ChessMat == state_ || drawState() )
      return;
    if ( UnderCheck == state_ )
      state_ = ChessMat;
    else
      state_ = Stalemat;
  }

  void verifyMasks() const;

  /// methods
private:

  /// clear board. remove all figures. reset all fields, number of moves etc...
  void clear();

  MoveCmd & getMove(int i)
  {
    THROW_IF( i < 0 || i >= GameLength, "there was no move" );
    return g_moves[i];
  }

  /// calculates absolute position evaluation
  ScoreType calculateEval() const;

  //ScoreType evaluatePawns(Figure::Color color, int stage) const;
  ScoreType evaluateWinnerLoser() const;

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

  /// return true if current color is checking
  /// also find all checking figures
  bool isChecking(MoveCmd &) const;

  /// validate current move. set invalid state_
  bool wasValidUnderCheck(const MoveCmd & ) const;

  bool wasValidWithoutCheck(const MoveCmd & ) const;

  /// is king of given color attacked by given figure
  /// returns index of figure if attacked or -1 otherwise
  inline int isAttackedBy(Figure::Color color, const Figure & fig) const
  {
    const Figure & king = getFigure(color, KingIndex);
    int dir = g_figureDir->dir(fig, king.where());
    if ( dir < 0 )
      return -1;

    const uint64 & mask = g_betweenMasks->between(fig.where(), king.where());
    const uint64 & black = fmgr_.mask(Figure::ColorBlack);
    if ( (~black & mask) != mask )
      return -1;

    const uint64 & white = fmgr_.mask(Figure::ColorWhite);
    if ( (~white & mask) != mask )
      return -1;

    return fig.getIndex();
  }

  /// gets index of figure, attacking from given direction
  /// check only bishops, rook and queens
  int getAttackedFrom(Figure::Color color, int apt) const;
  int fastAttackedFrom(Figure::Color color, int apt) const;

  /// is field 'pos' attacked by given color?
  bool isAttacked(const Figure::Color c, int pos) const;

  // returns number of checking figures.
  // very slow. used only for initial validation
  int findCheckingFigures(Figure::Color color, int pos);


  /// data
private:

  static const char * stdFEN_;
  static char fen_[FENsize];

  /// index of castle's possibility
  /// indices: [0 - black, 1 - white] [0 - short (King), 1 - long (Queen)]
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

  MoveCmd * g_moves;
  MovesTable * g_movesTable;
  FigureDir * g_figureDir;
  PawnMasks * g_pawnMasks;
  BetweenMask * g_betweenMasks;
  DeltaPosCounter * g_deltaPosCounter;
  DistanceCounter * g_distanceCounter;
};
