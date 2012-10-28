#pragma once

/*************************************************************
  Board.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Figure.h"
#include "Field.h"
#include "Move.h"
#include "MovesTable.h"
#include "FigureDirs.h"

class MovesGeneratorBase;
class MovesGenerator;
class CapsGenerator;
class EscapeGenerator;
class ChecksGenerator;
class ChecksGenerator2;

#ifdef VERIFY_CHECKS_GENERATOR
class Player;
#endif

struct FiguresMobility
{
  FiguresMobility() :
    knightMob_(0), bishopMob_(0), rookMob_(0), queenMob_(0),
    knightDist_(0), bishopDist_(0), rookDist_(0), queenDist_(0)
  {}

  int knightMob_, bishopMob_, rookMob_, queenMob_;
  int knightDist_, bishopDist_, rookDist_, queenDist_;
};

/*! board representation
 */
class Board
{
  friend class MovesGeneratorBase;
  friend class MovesGenerator;
  friend class CapsGenerator;
  friend class EscapeGenerator;
  friend class ChecksGenerator;
  friend class ChecksGenerator2;

#ifdef VERIFY_CHECKS_GENERATOR
  friend class Player;
#endif

public:

  static int64 ticks_;
  static int   tcounter_;

  /// constants
  enum State { Invalid, Ok = 1, UnderCheck = 2, Stalemat = 4, DrawReps = 8, DrawInsuf = 16, Draw50Moves = 64, ChessMat = 128 };
  enum { NumOfFields = 64, MovesMax = 256, FENsize = 512, GameLength = 4096 };

  bool operator != (const Board & ) const;

  /// c-tor
  Board();

  static bool load(Board & , std::istream &);
  static bool save(const Board & , std::ostream &);

  // only for debugging purposes, saves complete board memory dump
  void save(const char * fname) const;
  void load(const char * fname);

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
  bool unpack(const PackedMove &, Move & move) const;

  PackedMove pack(const Move & move) const
  {
    PackedMove pm;
    pm.from_ = move.from_;
    pm.to_ = move.to_;
    pm.new_type_ = move.new_type_;
    return pm;
  }

  /// used in LMR
  bool canBeReduced(const Move & move) const
  {
    // don't reduce captures
    if ( (move.capture_ || move.new_type_ > 0) )
      return false;

    // don't allow reduction of pawn's movement to pre-last line or pawn's attack
    return !isDangerPawn(move);
  }

  /// used to detect reduction/extension necessity
  bool isDangerPawn(const Move & move) const
  {
    const Field & fto = getField(move.to_);
    if ( fto.type() != Figure::TypePawn )
      return false;

    // attacking
    const uint64 & p_caps = g_movesTable->pawnCaps_o(fto.color(), move.to_);
    const uint64 & o_mask = fmgr_.mask(color_);
    if ( p_caps & o_mask )
      return true;

    // becomes passed
    const uint64 & pmsk = fmgr_.pawn_mask_t(color_);
    Figure::Color ocolor = Figure::otherColor(color_);
    const uint64 & opmsk = fmgr_.pawn_mask_t(ocolor);
    const uint64 & passmsk = g_pawnMasks->mask_passed(color_, move.to_);
    const uint64 & blckmsk = g_pawnMasks->mask_blocked(color_, move.to_);

    return !(opmsk & passmsk) && !(pmsk & blckmsk);
  }

  // becomes passed
  bool pawnPassed(const MoveCmd & move) const
  {
    const Field & fto = getField(move.to_);
    if ( fto.type() != Figure::TypePawn )
      return false;

    THROW_IF( color_ == fto.color(), "invalid color of passed pawn" );

    Figure::Color ocolor = Figure::otherColor(color_);
    const uint64 & pmsk = fmgr_.pawn_mask_t(ocolor);
    const uint64 & opmsk = fmgr_.pawn_mask_t(color_);
    const uint64 & passmsk = g_pawnMasks->mask_passed(ocolor, move.to_);
    const uint64 & blckmsk = g_pawnMasks->mask_blocked(ocolor, move.to_);

    return !(opmsk & passmsk) && !(pmsk & blckmsk);
  }

  /// is pt attacked by figure in position 'p'
  inline bool ptAttackedBy(int8 pt, int p) const
  {
    const Field & field = getField(p);
    int dir = g_figureDir->dir(field.type(), field.color(), p, pt);
    if ( dir < 0 )
      return false;

    if ( field.type() == Figure::TypeKnight )
      return true;

    const uint64 & mask = g_betweenMasks->between(p, pt);
    const uint64 & black = fmgr_.mask(Figure::ColorBlack);
    if ( (~black & mask) != mask )
      return false;

    const uint64 & white = fmgr_.mask(Figure::ColorWhite);
    if ( (~white & mask) != mask )
      return false;

    return true;
  }

  /// get index of figure of color 'ocolor', which attacks field 'pt'
  /// returns -1 if 'pt' was already attacked from this direction (independently of figure)
  int getAttackedFrom(Figure::Color ocolor, int8 pt, int8 from) const;

  const FiguresManager & fmgr() const
  {
    return fmgr_;
  }

  /// verify is this move is physically possible. it could be invalid anyway
  bool verifyMove(const Move &) const;

  /// null-move
  void makeNullMove(MoveCmd & move);
  void unmakeNullMove(MoveCmd & move);

  inline bool allowNullMove() const
  {
    if ( (fmgr_.knights(color_)+fmgr_.bishops(color_)+fmgr_.rooks(color_)+fmgr_.queens(color_) == 0) ||
          !can_win_[0] ||
          !can_win_[1] ||
         (fmgr_.weight(color_) < Figure::figureWeight_[Figure::TypeRook]) ||
         (fmgr_.weight(color_) < Figure::figureWeight_[Figure::TypeRook]+Figure::figureWeight_[Figure::TypeKnight] && !fmgr_.pawns(color_)) )
    {
      return false;
    }

    return true;
  }

  inline bool shortNullMoveReduction() const
  {
    return fmgr_.weight(color_) <= Figure::figureWeight_[Figure::TypeRook]+Figure::figureWeight_[Figure::TypePawn];
  }

  inline bool limitedNullMoveReduction() const
  {
    return fmgr_.weight(color_)- fmgr_.pawns()*Figure::figureWeight_[Figure::TypePawn] <= Figure::figureWeight_[Figure::TypeQueen];
  }

  inline bool isWinnerLoser() const
  {
    return !can_win_[0] || !can_win_[1];
  }

  Figure::Color getWinnerColor() const
  {
    return can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  }

  /// really make move. perform validation
  bool makeMove(const Move & );

  /// called after makeMove
  void unmakeMove();
  /*! end of movements */

  /// verify if there is draw or mat
  void verifyState();

  /// 2 means that only king's movements are valid
  int doubleCheck() const { return lastMove().checkingNum_ > 1; }

  /// returns position evaluation that depends on color
  ScoreType evaluate() const;

  /// add new figure
  bool addFigure(const Figure::Color color, const Figure::Type type, int pos);

  /// verify position and calculate checking figures
  /// very slow. should be used only while initialization
  bool invalidate();

  inline int enpassantPos() const
  {
    if ( en_passant_ < 0 )
      return -1;

    // if current color is black, color of en-passant pawn is white and vise versa
    static int pw_delta[2] = { 8, -8 };
    return en_passant_ + pw_delta[color_];
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

  /// get i-th move from begin
  const MoveCmd & getMove(int i) const
  {
    THROW_IF( i < 0 || i >= GameLength, "there was no move" );
    return g_moves[i];
  }

  /// get i-th move from end
  /// '0' means the last one, '-1' means 1 before last
  MoveCmd & getMoveRev(int i)
  {
    THROW_IF( i > 0 || i <= -halfmovesCounter_, "attempt to get move before 1st or after last" );
    return g_moves[halfmovesCounter_+i];
  }

  const MoveCmd & getMoveRev(int i) const
  {
    THROW_IF( i > 0 || i <= -halfmovesCounter_, "attempt to get move before 1st or after last" );
    return g_moves[halfmovesCounter_+i];
  }

  const MoveCmd & lastMove() const
  {
    THROW_IF( halfmovesCounter_ < 0, "invalid halfmovesCounter");
    return g_moves[halfmovesCounter_];
  }

  /// returns current move color
  Figure::Color getColor() const { return color_; }

  /// returns current state, ie check, mat etc
  uint8 getState() const { return state_; }

  /// returns true if we are under check
  bool underCheck() const { return state_ & UnderCheck; }

  /// just a useful method to quickly check if there is a draw
  static bool isDraw(uint8 state)
  {
    return (Stalemat & state) || (DrawReps & state) || (DrawInsuf & state) || (Draw50Moves & state);
  }

  inline bool matState() const { return state_ & ChessMat; }
  inline bool drawState() const { return isDraw(state_); }

  inline void setNoMoves()
  {
    if ( (Invalid == state_) || (ChessMat & state_) || drawState() )
      return;
    if ( underCheck() )
      state_ |= ChessMat;
    else
      state_ |= Stalemat;
  }

  void verifyMasks() const;

  const uint64 & hashCode() const
  {
    return fmgr_.hashCode();
  }

  ScoreType material(Figure::Color color) const
  {
    return fmgr_.weight(color);
  }

  ScoreType material() const
  {
    ScoreType score = fmgr_.weight();
    if ( !color_ )
      score = -score;
    return score;
  }

  bool detectCheck(const Figure::Color acolor, int pos) const;
  bool detectCheck(const Figure::Color acolor, int pos, int exclude) const;

  /// is field 'pos' attacked by given color? figure isn't moved
  bool fastAttacked(const Figure::Color c, int8 pos, int8 exclude_pos) const;

  /// is field 'pos' attacked by given color?
  bool isAttacked(const Figure::Color c, int pos) const;
  bool fastAttacked(const Figure::Color c, int8 pos) const;


  /// static exchange evaluation, should be called before move
  int see(const Move & move) const;

  /// find king's position
  int kingPos(Figure::Color c) const
  {
    return find_lsb(fmgr_.king_mask(c));
  }

  /// methods
private:

  /// clear board. reset all fields, number of moves etc...
  void clear();

  // detect discovered check to king of 'kc' color
  inline bool see_check(Figure::Color kc, uint8 from, uint8 ki_pos, const BitMask & all_mask_inv, const BitMask & a_brq_mask) const
  {
    // we need to verify if there is some attacker on line to king
    const BitMask & from_msk = g_betweenMasks->from(ki_pos, from);

    // no attachers at all
    if ( !(a_brq_mask & from_msk) )
      return false;

    // is there some figure between king and field that we move from
    BitMask all_mask_inv2 = (all_mask_inv | (1ULL << from));

    if ( is_something_between(ki_pos, from, all_mask_inv2) )
      return false;

    int index = find_first_index(ki_pos, from, ~all_mask_inv2);
    if ( index < 0 )
      return false;

    const Field & field = getField(index);

    // figure is the same color as king
    if ( field.color() == kc )
      return false;

    // figure have to be in updated BRQ mask
    if ( !((1ULL<<index) & a_brq_mask) )
      return false;

    THROW_IF( field.type() < Figure::TypeBishop || field.type() > Figure::TypeQueen, "see: not appropriate attacker type" );

    // could figure attack king from it's position
    if ( g_figureDir->dir(field.type(), field.color(), index, ki_pos) >= 0 )
      return true;

    return false;
  }

  MoveCmd & getMove(int i)
  {
    THROW_IF( i < 0 || i >= GameLength, "there was no move" );
    return g_moves[i];
  }

  /// return short/long castle possibility
  bool castling() const
  {
    return castling_ != 0;
  }

  bool castling(Figure::Color c, int t /* 0 - short (K), 1 - long (Q) */) const
  {
    int offset = 1 << ((c<<1) | t);
    return (castling_ >> offset) & 1;
  }

  // white
  bool castling_K() const
  {
    return (castling_ >> 2) & 1;
  }

  bool castling_Q(Figure::Color c) const
  {
    return (castling_ >> 3) & 1;
  }

  // black
  bool castling_k() const
  {
    return castling_ & 1;
  }

  bool castling_q(Figure::Color c) const
  {
    return (castling_ >> 1) & 1;
  }

  /// set short/long castle
  void set_castling(Figure::Color c, int t)
  {
    int offset = 1 << ((c<<1) | t);
    castling_ |= 1 << offset;
  }

  void clear_castling(Figure::Color c, int t)
  {
    int offset = 1 << ((c<<1) | t);
    castling_ &= ~(1 << offset);
  }

  /// calculates absolute position evaluation
  ScoreType calculateEval() const;
  inline ScoreType evaluateKing(Figure::Color color, const FiguresMobility & fmob) const;
  inline ScoreType evaluateFianchetto() const;
  ScoreType evaluatePawns(Figure::Color color) const;
  ScoreType evalPawnsEndgame(Figure::Color color) const;
  ScoreType evaluateRooks(Figure::Color color) const;
  ScoreType evaluateMobility(Figure::Color color, FiguresMobility & fmob) const;
  ScoreType evaluateWinnerLoser() const;

  bool verifyCastling(const Figure::Color , int t) const;

  bool validateMove(const Move & mv) const;

  /// verify position after movement
  //inline bool wasMoveValid(const MoveCmd & move) const
  //{
  //  if ( UnderCheck & move.old_state_ )
  //  {
  //    if ( move.checkVerified_ )
  //    {
  //      THROW_IF( !wasValidUnderCheck(move), "move was verified as valid, but actualy itsn't" );
  //      return true;
  //    }

  //    return wasValidUnderCheck(move);
  //  }
  //  else
  //    return wasValidWithoutCheck(move);
  //}

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
  inline bool isAttackedBy(Figure::Color color, const Figure::Color c, const Figure::Type t, int p) const
  {
    int king_pos = kingPos(color);
    int dir = g_figureDir->dir(t, c, p, king_pos);
    if ( dir < 0 || Figure::TypeKing == t && dir > 7 )
      return false;

    const uint64 & mask = g_betweenMasks->between(p, king_pos);
    const uint64 & black = fmgr_.mask(Figure::ColorBlack);
    if ( (~black & mask) != mask )
      return false;

    const uint64 & white = fmgr_.mask(Figure::ColorWhite);
    if ( (~white & mask) != mask )
      return false;

    return true;
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

  // returns number of checking figures.
  // very slow. used only for initial validation
  int findCheckingFigures(Figure::Color color, int pos);

  // find 1st figure on the whole semi-line given by direction 'from' -> 'to'
  // mask gives all interesting figures
  inline int find_first_index(int from, int to, const BitMask & mask) const
  {    
    BitMask mask_from = mask & g_betweenMasks->from(from, to);
    if ( !mask_from )
      return -1;

    int index = -1;
    if ( from < to ) // use LSB
      index = find_lsb(mask_from);
    else // MSB
      index = find_msb(mask_from);

    return index;
  }

  // check if there is some figure between 'from' and 'to'
  // inv_mask - inverted mask of all interesting figures
  inline bool is_something_between(int from, int to, const BitMask & inv_mask) const
  {
    const BitMask & btw_msk = g_betweenMasks->between(from, to);
    return (btw_msk & inv_mask) != btw_msk;
  }

  // acolor - color of attacking side, ki_pos - attacked king pos
  inline bool discoveredCheck(int8 pt, Figure::Color acolor, const uint64 & mask_all, const uint64 & brq_mask, int ki_pos) const
  {
    BitMask from_msk = g_betweenMasks->from(ki_pos, pt);
    BitMask mask_all_ex = mask_all & ~(1ULL << pt);
    mask_all_ex &= from_msk;
    if ( (mask_all_ex & brq_mask) == 0 )
      return false;

    int apos = ki_pos < pt ? find_lsb(mask_all_ex) : find_msb(mask_all_ex);
    const Field & afield = getField(apos);
    if ( afield.color() != acolor || afield.type() < Figure::TypeBishop || afield.type() > Figure::TypeQueen )
      return false;

    int dir = g_figureDir->dir(afield.type(), afield.color(), apos, ki_pos);
    return dir >= 0;
  }

  /// data
private:

  static const char * stdFEN_;
  static char fen_[FENsize];

  /// for chess draw detector
  bool can_win_[2];

  /// en-passant pawn index. must be cleared (set to -1) after move. it has color different from "color_"
  int8  en_passant_;

  /// current state, i.e check, draw, mat, invalid, etc.
  uint8 state_;

  /// color to make move from this position
  Figure::Color color_;

  /// holds number of figures of each color, hash key, masks etc.
  FiguresManager fmgr_;

  /// fields array 8 x 8
  Field fields_[NumOfFields];

  int fiftyMovesCount_, halfmovesCounter_, movesCounter_;
  uint8 repsCounter_;

  // castling possibility flag
  // (0, 1) bits block for black
  // (2, 3) bits block for white
  // lower bit in block is short (K)
  // higher bit in block is long (Q)
  uint8 castling_;

  MoveCmd * g_moves;
  const MovesTable * g_movesTable;
  const FigureDir * g_figureDir;
  const PawnMasks * g_pawnMasks;
  const BetweenMask * g_betweenMasks;
  const DeltaPosCounter * g_deltaPosCounter;
  const DistanceCounter * g_distanceCounter;
};
