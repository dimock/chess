/*************************************************************
  MoveMaker.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Board.h"
#include "FigureDirs.h"

bool Board::validMove(const Move & move) const
{
  if ( !move )
    return false;

  THROW_IF( move.to_ > 63 || move.from_ < 0 || move.from_ > 63 || move.rindex_ > 14, "invalid move given" );

  const Field & ffrom = getField(move.from_);
  if ( !ffrom || ffrom.color() != color_ )
    return false;

  // only kings move is possible
  if ( checkingNum_ > 1 && ffrom.type() != Figure::TypeKing )
    return false;

  const Figure & fig = getFigure(color_, ffrom.index());
  THROW_IF( !fig, "there is no figure on field from in validMove" );
  if ( fig.getType() != Figure::TypePawn && move.new_type_ > 0 )
    return false;

  THROW_IF( fig.where() != move.from_, "figure isn't on its fiels" );

  int dir = g_figureDir->dir(ffrom.type(), color_, move.from_, move.to_);
  if ( dir < 0 )
    return false;

  // castle
  if ( fig.getType() == Figure::TypeKing && (8 == dir || 9 == dir) )
    return move.rindex_ < 0 && fig.isFirstStep() && !getField(move.to_) && 0 == checkingNum_ && (fig.where() == 4 && color_ || fig.where() == 60 && !color_);

  const Field & field = getField(move.to_);
  if ( field && (field.type() == Figure::TypeKing || field.color() == color_) )
    return false;

  switch ( fig.getType() )
  {
  case Figure::TypeKnight:
  case Figure::TypeKing:
    {
      return (!field && move.rindex_ < 0) || (field && field.index() == move.rindex_);
    }
    break;

  case Figure::TypePawn:
    {
      int8 y = move.to_ >> 3;
      if ( ((7 == y || 0 == y) && !move.new_type_) || (0 != y && 7 != y && move.new_type_) )
        return false;

      THROW_IF( move.new_type_ < 0 || move.new_type_ > Figure::TypeQueen, "invalid promotion piece" );

      if ( field )
      {
        THROW_IF( field.color() == color_ , "pawn does invalid capture" );
        return (0 == dir || 1 == dir) && (field.index() == move.rindex_);
      }
      else if ( en_passant_ >= 0 && move.rindex_ == en_passant_ && (0 == dir || 1 == dir) )
      {
        int8 dy = (move.to_ >> 3) - (move.from_ >> 3);
        const Figure & epawn = getFigure(Figure::otherColor(color_), en_passant_);
        THROW_IF( !epawn, "there is no pawn for en-passant capture" );
        int e_to = epawn.where() + (dy << 3);
        return move.to_ == e_to;
      }
      else if ( move.rindex_ < 0 )
      {
        int8 to3 = move.from_ + ((move.to_-move.from_) >> 1);
        THROW_IF( (uint8)to3 > 63 || 3 == dir && to3 != move.from_ + (move.to_ > move.from_ ? 8 : -8), "pawn goes to invalid field" );
        return (2 == dir) || (3 == dir && !getField(to3));
      }
    }
    break;

  case Figure::TypeBishop:
  case Figure::TypeRook:
  case Figure::TypeQueen:
    {
      if ( (!field && move.rindex_ >= 0) || (field && field.index() != move.rindex_) )
        return false;

      const uint64 & mask = g_betweenMasks->between(move.from_, move.to_);
      const uint64 & black = fmgr_.mask(Figure::ColorBlack);
      const uint64 & white = fmgr_.mask(Figure::ColorWhite);

      bool ok = (mask & ~(black | white)) == mask;
      return ok;
    }
    break;
  }

  return false;
}

Move Board::unpack(const PackedMove & pm) const
{
  Move move;

  move.clear();

  const Field & fto = getField(pm.to_);
  if ( fto && (fto.color() == color_ || fto.type() == Figure::TypeKing) )
    return move;

  move.from_ = pm.from_;
  move.to_ = pm.to_;
  move.new_type_ = pm.new_type_;

  // to prevent reduction of this move in LMR
  move.strong_ = 1;

  if ( fto )
    move.rindex_ = fto.index();
  else if ( en_passant_ >= 0 )
  {
    const Figure & epawn = getFigure(Figure::otherColor(color_), en_passant_);
    if ( epawn )
    {
      int epos = color_ ? epawn.where()+8 : epawn.where()-8;
      if ( move.to_ == epos )
        move.rindex_ = en_passant_;
    }
  }

  if ( !validMove(move) )
    move.clear();

  return move;
}

bool Board::doMove()
{
  MoveCmd & move = getMove(halfmovesCounter_-1);

  Figure & fig = getFigure(color_, getField(move.from_).index());
  Figure::Color ocolor = Figure::otherColor(color_);
  move.en_passant_ = en_passant_;
  move.index_ = fig.getIndex();

  if ( drawState() || ChessMat == state_ )
    return false;

  state_ = Ok;

  move.mask_[0] = fmgr_.mask(Figure::ColorBlack);
  move.mask_[1] = fmgr_.mask(Figure::ColorWhite);

  if ( move.en_passant_ >= 0 )
  {
    const Figure & epawn = getFigure(ocolor, move.en_passant_);
    THROW_IF(!epawn || epawn.getType() != Figure::TypePawn, "no en-passant pawn to undo hash");
    fmgr_.hashEnPassant(epawn.where(), ocolor);
  }

  /// hashing castle possibility
  if ( castling(color_, 0) && (fig.getType() == Figure::TypeKing || fig.getType() == Figure::TypeRook && (fig.where() == 63 && !color_ || fig.where() == 7 && color_)) )
    fmgr_.hashCastling(color_, 0);

  if ( castling(color_, 1) && (fig.getType() == Figure::TypeKing || fig.getType() == Figure::TypeRook && (fig.where() == 56 && !color_ || fig.where() == 0 && color_)) )
    fmgr_.hashCastling(color_, 1);

  int d = move.to_ - move.from_;
  if ( fig.getType() == Figure::TypeKing && (2 == d || -2 == d) )// castle
  {
    // don't do castling under check
    if ( checkingNum_ > 0 )
      return false;

    d >>= 1;

    move.rook_to_ = move.from_ + d;

    // field, that rook or king is going to move to, is occupied
    if ( getField(move.rook_to_) || getField(move.to_) )
      return false;

    THROW_IF( isAttacked(ocolor, fig.where()), "can't do castling under check" );
    THROW_IF( move.rindex_ >= 0, "can't eat while castling" );
    THROW_IF( !(fig.where() == 4 && color_ || fig.where() == 60 && !color_), "kings position is wrong" );

    // then verify if there is suitable rook fo castling
    move.rook_from_ = move.from_ + ((d>>1) ^ 3);//d < 0 ? move.from_ - 4 : move.from_ + 3

    THROW_IF( (move.rook_from_ != 0 && color_ && d < 0) || (move.rook_from_ != 7 && color_ && d > 0), "invalid castle rook position" );
    THROW_IF( (move.rook_from_ != 56 && !color_ && d < 0) || (move.rook_from_ != 63 && !color_ && d > 0), "invalid castle rook position" );

    Field & rf_field = getField(move.rook_from_);
    if ( rf_field.type() != Figure::TypeRook || rf_field.color() != color_ )
      return false;

    // is long castling possible
    if ( !(move.rook_from_ & 3) && getField(move.rook_from_+1) )
      return false;

    move.rook_index_ = rf_field.index();
    Figure & rook = getFigure(color_, move.rook_index_);

    THROW_IF( rook.getType() != Figure::TypeRook || isAttacked(ocolor, move.rook_to_) != fastAttacked(ocolor, move.rook_to_), "fast attacked returned wrong result");

    if ( !rook.isFirstStep() || fastAttacked(ocolor, move.rook_to_) )
      return false;

    rf_field.clear();
    fmgr_.move(rook, move.rook_to_);
    rook.setMoved();
    Field & field_rook_to  = getField(rook.where());
    move.field_rook_to_ = field_rook_to;
    field_rook_to.set(rook);

    move.castle_ = 1;
    state_ = Castle;
  }

  en_passant_ = -1;

  if ( move.rindex_ >= 0 )
  {
    Figure & rfig = getFigure(ocolor, move.rindex_);

    // castle possibility
    if ( rfig.getType() == Figure::TypeRook )
    {
      // short castle
      if ( castling(ocolor, 0) && (rfig.where() == 63 && !ocolor ||rfig.where() == 7 && ocolor) )
        fmgr_.hashCastling(ocolor, 0);
      // long castle
      else if ( castling(ocolor, 1) && (rfig.where() == 56 && !ocolor || rfig.where() == 0 && ocolor) )
        fmgr_.hashCastling(ocolor, 1);
    }

    move.eaten_type_ = rfig.getType();
    getField(rfig.where()).clear();
    fmgr_.decr(rfig);
    rfig.setType(Figure::TypeNone);
  }

  int pw_dy = move.to_ - fig.where();
  if ( Figure::TypePawn == fig.getType() && fig.isFirstStep() && (16 == pw_dy || -16 == pw_dy) )
  {
    en_passant_ = fig.getIndex();
    fmgr_.hashEnPassant(move.to_, color_);
  }

  move.first_move_ = fig.isFirstStep();

  getField(fig.where()).clear();
  fig.setMoved();

  if ( move.new_type_ > 0 )
  {
    fmgr_.decr(fig);
    fig.go(move.to_);
    fig.setType((Figure::Type)move.new_type_);
    fmgr_.incr(fig);
  }
  else
    fmgr_.move(fig, move.to_);

  Field & field_to = getField(fig.where());
  move.field_to_ = field_to;
  field_to.set(fig);

  move.need_undo_ = true;

  move.fifty_moves_ = fiftyMovesCount_;
  move.reps_counter_ = repsCounter_;

  if ( Figure::TypePawn == fig.getType() || move.rindex_ >= 0 || move.new_type_ > 0 )
  {
    move.irreversible_ = true;
    fiftyMovesCount_ = 0;
    repsCounter_ = 0;
  }
  else
  {
    move.irreversible_ = false;
    fiftyMovesCount_++;
  }

  // add hash color key
  fmgr_.hashColor();

  // put new hash code to detect threefold repetition
  move.zcode_ = fmgr_.hashCode();

  //if ( !color_ )
  //  movesCounter_++;
  movesCounter_ += ~color_ & 1;

  return true;
}

void Board::undoMove()
{
  MoveCmd & move = getMove(halfmovesCounter_);

  if ( !move.need_undo_ )
    return;

  //if ( !color_ )
  //  movesCounter_--;
  movesCounter_ -= ~color_ & 1;

  // restore hash color. we change it early for 3-fold repetition detection
  //fmgr_.hashColor();

  Figure & fig = getFigure(color_, getField(move.to_).index());

  // restore figure

  // restore old type
  if ( move.new_type_ > 0 )
  {
    fmgr_.u_decr(fig);
    fig.go(move.from_);
    fig.setType(Figure::TypePawn);
    fmgr_.u_incr(fig);
  }
  else
  {
    fmgr_.u_move(fig, move.from_);
  }

  // restore position and field
  getField(move.to_) = move.field_to_;
  fig.setFirstStep(move.first_move_);
  getField(fig.where()).set(fig);

  // restore en-passant hash code
  //if ( en_passant_ >= 0 )
  //  fmgr_.hashEnPassant(fig.where(), color_);

  // restore prev. en-passant index
  en_passant_ = move.en_passant_;

  Figure::Color ocolor = Figure::otherColor(color_);

  // restore eaten figure
  if ( move.rindex_ >= 0 )
  {
    Figure & rfig = getFigure(ocolor, move.rindex_);

    THROW_IF( move.eaten_type_ <= 0, "type of eaten figure is invalid" );
    rfig.setType((Figure::Type)move.eaten_type_);
    getField(rfig.where()).set(rfig);
    fmgr_.u_incr(rfig);
  }

  // restore king and rook after castling
  if ( move.rook_index_ >= 0 )
  {
    int d = (move.to_ - move.from_) >> 1;

    // restore rook's field. copy old one to restore all bits
    getField(move.rook_to_) = move.field_rook_to_;

    Figure & rook = getFigure(color_, move.rook_index_);
    fmgr_.u_move(rook, move.rook_from_);

    rook.setFirstStep(true);
    Field & rfield = getField(rook.where());
    rfield.set(rook);
  }

  // restore figures masks
  fmgr_.restoreMasks(move.mask_);

  fiftyMovesCount_ = move.fifty_moves_;
  repsCounter_ = move.reps_counter_;
}

bool Board::makeMove(const Move & mv)
{
  THROW_IF(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  halfmovesCounter_++;

  MoveCmd & move = getMove(halfmovesCounter_-1);
  move.clearUndo();
  move = mv;

  // store Zobrist key and state
  move.old_state_ = state_;
  move.zcode_old_ = fmgr_.hashCode();

  if ( !doMove() )
  {
    state_ = Invalid;
    return false;
  }

  if ( !wasMoveValid(move) )
  {
    state_ = Invalid;
    return false;
  }

  if ( isChecking(move) )
    state_ = UnderCheck;

  move.can_win_[0] = can_win_[0];
  move.can_win_[1] = can_win_[1];

  if ( drawState() || verifyChessDraw() )
  {
    can_win_[0] = move.can_win_[0];
    can_win_[1] = move.can_win_[1];
    move.state_ = state_;
    return true;
  }

  move.state_ = state_;
  move.need_unmake_ = true;

  move.stage_ = stages_[color_];

  Figure::Color ocolor = Figure::otherColor((Figure::Color)color_);

  // we need to go to endgame if there is capture and most of material is lost
  stages_[color_] |= ( (move.rindex_ >= 0 && 0 == stages_[color_]) &&
    ( !((fmgr_.queens(ocolor) > 0 && fmgr_.rooks(ocolor)+fmgr_.knights(ocolor)+fmgr_.bishops(ocolor) > 0) ||
    (fmgr_.rooks (ocolor) > 1 && fmgr_.bishops(ocolor)+fmgr_.knights(ocolor) > 1) ||
    (fmgr_.rooks (ocolor) > 0 && ((fmgr_.bishops_w(ocolor) > 0 && fmgr_.bishops_b(ocolor) > 0) || (fmgr_.bishops(ocolor) + fmgr_.knights(ocolor) > 2)))) ) ) & 1;

  move.old_checkingNum_ = checkingNum_;
  move.old_checking_[0] = checking_[0];
  move.old_checking_[1] = checking_[1];

  checkingNum_ = move.checkingNum_;
  checking_[0] = move.checking_[0];
  checking_[1] = move.checking_[1];

  THROW_IF( isAttacked(color_, getFigure(Figure::otherColor(color_), KingIndex).where()) && UnderCheck != state_, "check isn't detected" );

  // now change color
  color_ = ocolor;

  return true;
}

void Board::unmakeMove()
{
  THROW_IF(halfmovesCounter_ <= 0 || halfmovesCounter_ > GameLength, "number of halfmoves is invalid");

  halfmovesCounter_--;

  MoveCmd & move = getMove(halfmovesCounter_);

  if ( move.need_unmake_ )
  {
    color_ = Figure::otherColor(color_);

    checkingNum_ = move.old_checkingNum_;
    checking_[0] = move.old_checking_[0];
    checking_[1] = move.old_checking_[1];

    stages_[color_] = move.stage_;

    can_win_[0] = move.can_win_[0];
    can_win_[1] = move.can_win_[1];
  }

  undoMove();

  // always restore hash code and state
  state_ = (State)move.old_state_;
  fmgr_.restoreHash(move.zcode_old_);
}

bool Board::verifyChessDraw()
{
  if ( (fiftyMovesCount_ == 100 && UnderCheck != state_) || (fiftyMovesCount_ > 100) )
  {
    state_ = Draw50Moves;
    return true;
  }

  can_win_[0] = ( fmgr_.pawns(Figure::ColorBlack) > 0 || fmgr_.rooks(Figure::ColorBlack) > 0 || fmgr_.queens(Figure::ColorBlack) > 0 ) ||
                ( fmgr_.knights(Figure::ColorBlack) > 0 && fmgr_.bishops(Figure::ColorBlack) > 0 ) ||
                ( fmgr_.bishops_b(Figure::ColorBlack) > 0 && fmgr_.bishops_w(Figure::ColorBlack) > 0 ) ||
                ( fmgr_.knights(Figure::ColorBlack) > 1 && fmgr_.weight(Figure::ColorWhite) > 0 );

  can_win_[1] = ( fmgr_.pawns(Figure::ColorWhite) > 0 || fmgr_.rooks(Figure::ColorWhite) > 0 || fmgr_.queens(Figure::ColorWhite) > 0 ) ||
                ( fmgr_.knights(Figure::ColorWhite) > 0 && fmgr_.bishops(Figure::ColorWhite) > 0 ) ||
                ( fmgr_.bishops_b(Figure::ColorWhite) > 0 && fmgr_.bishops_w(Figure::ColorWhite) > 0 ) ||
                ( fmgr_.knights(Figure::ColorWhite) > 1 && fmgr_.weight(Figure::ColorBlack) > 0 );

  if ( !can_win_[0] && !can_win_[1] )
  {
    state_ = DrawInsuf;
    return true;
  }

  //// we don't need to verify threefold repetition if capture or pawn's move was less than 4 steps ago
  //if ( fiftyMovesCount_ < 4 )
  //  return false;

  int reps = 1;
  int i = halfmovesCounter_-3;
  for (; reps < 3 && i >= 0; i -= 2)
  {
    if ( getMove(i).zcode_ == fmgr_.hashCode() )
      reps++;

    if ( getMove(i).irreversible_ )
      break;
  }

  // may be we forget to test initial position?
  if ( reps < 3 && i == -1 && fmgr_.hashCode() == getMove(0).zcode_old_ )
	  reps++;

  if ( reps > repsCounter_ )
	  repsCounter_ = reps;

  if ( reps >= 3 )
  {
	  state_ = DrawReps;
	  return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////
// NULL MOVE

void Board::makeNullMove(MoveCmd & move)
{
  move.zcode_old_ = hashCode();
  move.state_ = state_;

  if ( en_passant_ >= 0 )
  {
    const Figure & fig = getFigure(Figure::otherColor(color_), en_passant_);
    THROW_IF( !fig, "no en-passant pawn" );
    fmgr_.hashEnPassant(fig.where(), color_);
    move.en_passant_ = en_passant_;
    en_passant_ = -1;
  }
  else
    move.en_passant_ = -1;

  color_ = Figure::otherColor(color_);
  fmgr_.hashColor();
}

void Board::unmakeNullMove(MoveCmd & move)
{
  color_ = Figure::otherColor(color_);
  en_passant_ = move.en_passant_;
  fmgr_.restoreHash(move.zcode_old_);
  state_ = (State)move.state_;
}
