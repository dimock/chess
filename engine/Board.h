#pragma once

#include "Figure.h"
#include "Field.h"
#include "Move.h"
#include "MovesTable.h"
#include "FigureDirs.h"

class MovesGenerator;
class CapsGenerator;
class EscapeGenerator;
class ChecksGenerator;
class CapsChecksGenerator;

#ifdef VERIFY_CHECKS_GENERATOR
class Player;
#endif

/*! board representation
 */
class Board
{
  friend class MovesGenerator;
  friend class CapsGenerator;
  friend class EscapeGenerator;
  friend class ChecksGenerator;
  friend class CapsChecksGenerator;

#ifdef VERIFY_CHECKS_GENERATOR
  friend class Player;
#endif

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
  void set_MovesTable(const MovesTable * movesTable) { g_movesTable = movesTable; }
  void set_FigureDir(const FigureDir * figureDir) { g_figureDir = figureDir; }
  void set_PawnMasks(const PawnMasks * pawnMasks) { g_pawnMasks = pawnMasks; }
  void set_BetweenMask(const BetweenMask * betweenMask) { g_betweenMasks = betweenMask; }
  void set_DeltaPosCounter(const DeltaPosCounter * deltaPosCounter) { g_deltaPosCounter = deltaPosCounter; }
  void set_DistanceCounter(const DistanceCounter * distanceCounter) { g_distanceCounter = distanceCounter; }

  /// initialize from FEN
  bool fromFEN(const char * fen);

  /// save current position to FEN
  bool toFEN(char * fen) const;

  /// initialize empty board with given color to move
  bool initEmpty(Figure::Color );

  /*! movements
   */

  /// unpack from hash
  Move unpack(const PackedMove & ) const;

  PackedMove pack(const Move & move) const
  {
    PackedMove pm;
    pm.from_ = move.from_;
    pm.to_ = move.to_;
    pm.new_type_ = move.new_type_;
    return pm;
  }

  /// verify 
  bool validMove(const Move &) const;

  /// null-move
  void makeNullMove(MoveCmd & move);
  void unmakeNullMove(MoveCmd & move);

  inline bool allowNullMove() const
  {
    Figure::Color ocolor = Figure::otherColor(color_);
    if ( fmgr_.knights(color_)+fmgr_.bishops(color_)+fmgr_.rooks(color_)+fmgr_.queens(color_) == 0 )
      return false;

    bool ok = fmgr_.weight(color_) > Figure::figureWeight_[Figure::TypeQueen]; //+Figure::figureWeight_[Figure::TypeKnight] ||
              //fmgr_.weight(ocolor) >= Figure::figureWeight_[Figure::TypeRook]+Figure::figureWeight_[Figure::TypeKnight];

    return ok;
  }


  /// really make move. perform validation
  bool makeMove(const Move & );

  /// called after makeMove
  void unmakeMove();
  /*! end of movements */

  /// verify if there is draw or mat
  void verifyState();

  /// 2 means that only king's movements are valid
  int getNumOfChecking() const { return checkingNum_; }

  /// returns position evaluation that depends on color
  ScoreType evaluate() const;

  /// fast evaluation. gives approximate result
  ScoreType expressEval() const;

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

  /// returns max number of repetitions found. it will be set to 0 after pawns move or capture
  uint8 repsCount() const { return repsCounter_; }

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

  const uint64 & hashCode() const
  {
    return fmgr_.hashCode();
  }

  /// methods
private:

  /// clear board. remove all figures. reset all fields, number of moves etc...
  void clear();

  MoveCmd & getMove(int i)
  {
    THROW_IF( i < 0 || i >= GameLength, "there was no move" );
    return g_moves[i];
  }

  /// return short/long castle possibility
  bool castling(Figure::Color color, int8 t /* 0 - short (K), 1 - long (Q) */) const
  {
	  THROW_IF( (unsigned)color > 1 || (unsigned)t > 1, "invalid color or castle type" );
    int8 rpos = ((((int8)color-1) >> 7) & 56) | ((t-1) >> 7) & 7;
    return getFigure(color, KingIndex).isFirstStep() && getField(rpos).type() == Figure::TypeRook && getField(rpos).color() == color && getFigure(color, getField(rpos).index()).isFirstStep();
  }

  /// calculates absolute position evaluation
  ScoreType calculateEval() const;

  ScoreType evaluatePawns(Figure::Color color) const;
  ScoreType evaluateRooks(Figure::Color color) const;
  ScoreType evalPawnsEndgame(Figure::Color color) const;
  ScoreType evaluateWinnerLoser() const;

  /// do move. fill undo info, don't validate
  bool doMove();

  /// undo move. restore old state after doMove
  void undoMove();

  /// verify position after movement
  inline bool wasMoveValid(const MoveCmd & move) const
  {
    if ( UnderCheck == move.old_state_ )
    {
      if ( move.checkVerified_ )
      {
        THROW_IF( !wasValidUnderCheck(move), "move was verified as valid, but actualy itsn't" );
        return true;
      }

      return wasValidUnderCheck(move);
    }
    else
      return wasValidWithoutCheck(move);
  }

  bool verifyChessDraw();

  /// return true if current color is checking
  /// also find all checking figures
  bool isChecking(MoveCmd &) const;

  /// validate current move. set invalid state_
  // move is already done
  bool wasValidUnderCheck(const MoveCmd & ) const;

  // move is already done
  bool wasValidWithoutCheck(const MoveCmd & ) const;

  // move isn't done yet. we can call it only if 1 attacking figure
  bool isMoveValidUnderCheck(const Move & move) const;

  /// is king of given color attacked by given figure
  /// returns index of figure if attacked or -1 otherwise
  inline int isAttackedBy(Figure::Color color, const Figure & fig) const
  {
    const Figure & king = getFigure(color, KingIndex);
    int dir = g_figureDir->dir(fig, king.where());
    if ( dir < 0 || Figure::TypeKing == fig.getType() && dir > 7 )
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

  /// special case - figure isn't moved yet
  int fastAttackedFrom(Figure::Color color, int apt,
    const uint64 & clear_msk /* figure goes from */,
    const uint64 & set_msk /* figure goes to */,
    const uint64 & exclude_msk /* removed figure - can't attack anymore */) const;

  /// is field 'pos' attacked by given color?
  bool isAttacked(const Figure::Color c, int pos) const;
  bool fastAttacked(const Figure::Color c, int8 pos) const;

  // returns number of checking figures.
  // very slow. used only for initial validation
  int findCheckingFigures(Figure::Color color, int pos);


  /// data
private:

  static const char * stdFEN_;
  static char fen_[FENsize];

  /// index of castle's possibility
  /// indices: [0 - black, 1 - white] [0 - short (King), 1 - long (Queen)]
  //bool castle_index_[2][2];

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
  uint8 repsCounter_;

  MoveCmd * g_moves;
  const MovesTable * g_movesTable;
  const FigureDir * g_figureDir;
  const PawnMasks * g_pawnMasks;
  const BetweenMask * g_betweenMasks;
  const DeltaPosCounter * g_deltaPosCounter;
  const DistanceCounter * g_distanceCounter;
};
