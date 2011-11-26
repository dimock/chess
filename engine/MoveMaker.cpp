#include "Board.h"
#include "FigureDirs.h"

bool Board::validMove(const Move & move) const
{
  if ( move.to_ < 0 )
    return false;

  THROW_IF( move.from_ < 0 || move.rindex_ > 14, "invalid move given" );

  const Figure & fig = getFigure(color_, getField(move.from_).index());
  if ( !fig )
    return false;

  int dir = FigureDir::dir(fig, move.to_);
  if ( dir < 0 )
    return false;

  // castle. don't check if there is piece in "to"-field
  if ( fig.getType() == Figure::TypeKing && (8 == dir || 9 == dir) )
    return move.rindex_ < 0 && fig.isFirstStep() && 0 == checkingNum_ && (fig.where() == 4 && color_ || fig.where() == 60 && !color_);

  const Field & field = getField(move.to_);
  
  if ( field && ( field.color() == color_ || field.index() != move.rindex_ ) )
    return false;

  THROW_IF( field.type() > Figure::TypeQueen, "try to eat king" );

  if ( fig.getType() == Figure::TypeKing || fig.getType() == Figure::TypeKnight )
    return true;

  switch ( fig.getType() )
  {
  case Figure::TypePawn:
    {
      int8 y = move.to_ >> 3;
      if ( (y == 7 || y == 0) && !move.new_type_ )
        return false;

      THROW_IF( move.new_type_ < 0 || move.new_type_ > Figure::TypeQueen, "invalid promotion piece" );

      if ( field )
      {
        THROW_IF( field.color() == color_ || field.index() != move.rindex_, "pawn does invalid capture" );
        return 0 == dir || 1 == dir;
      }
      else if ( en_passant_ >= 0 && move.rindex_ == en_passant_ && (0 == dir || 1 == dir) )
      {
        int8 dy = (move.to_ >> 3) - (move.from_ >> 3);
        const Figure & epawn = getFigure(Figure::otherColor(color_), en_passant_);
        THROW_IF( !epawn, "there is no pawn for en-passant capture" );
        int e_to = epawn.where() + (dy << 3);
        return move.to_ == e_to;
      }
      else
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
      const uint64 & mask = BetweenMask::mask(move.from_, move.to_);
      const uint64 & black = fmgr_.mask(Figure::ColorBlack);
      const uint64 & white = fmgr_.mask(Figure::ColorWhite);

      bool ok = (mask & ~black & ~white) == mask;
      return ok;
    }
    break;
  }

  return false;
}

bool Board::doMove()
{
  MoveCmd & move = moves_[halfmovesCounter_-1];

  Figure & fig = getFigure(color_, getField(move.from_).index());
  Figure::Color ocolor = Figure::otherColor(color_);
  move.en_passant_ = en_passant_;
  move.old_state_ = state_;
  move.index_ = fig.getIndex();

  if ( drawState() || ChessMat == state_ )
    return false;

  state_ = Ok;

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
    Field & rf_field = getField(move.rook_from_);
    if ( !rf_field )
      return false;

    move.rook_index_ = rf_field.index();
    Figure & rook = getFigure(color_, move.rook_index_);

    if ( rook.getType() != Figure::TypeRook || !rook.isFirstStep() || isAttacked(ocolor, move.rook_to_) )
      return false;

    rf_field.clear();
    fmgr_.move(rook, move.rook_to_);
    rook.setMoved();
    Field & field_rook_to  = getField(rook.where());
    move.field_rook_to_ = field_rook_to;
    field_rook_to.set(rook);

    castle_[color_] = 1 + (((unsigned)d & 0x80000000) >> 31);//d > 0 ? 1 : 2;
    state_ = Castle;

    THROW_IF(!castle_index_[color_][0] && !castle_index_[color_][1], "try to castle while it is impossible");
  }

  /// hashing castle
  move.castle_index_[0] = castle_index_[color_][0];
  move.castle_index_[1] = castle_index_[color_][1];

  if ( fig.getType() == Figure::TypeKing )
  {
    if ( castle_index_[color_][0] )
    {
      castle_index_[color_][0] = false;
      fmgr_.hashCastling(color_, 0);
    }

    if ( castle_index_[color_][1] )
    {
      castle_index_[color_][1] = false;
      fmgr_.hashCastling(color_, 1);
    }
  }
  else if ( fig.getType() == Figure::TypeRook && (fig.where() & 7) == 7 && castle_index_[color_][0] )
  {
    THROW_IF( !fig.isFirstStep(), "castling's possibility doesn't correspond to the flag" );

    castle_index_[color_][0] = false;
    fmgr_.hashCastling(color_, 0);
  }
  else if ( fig.getType() == Figure::TypeRook && (fig.where() & 7) == 0 && castle_index_[color_][1] )
  {
    THROW_IF( !fig.isFirstStep(), "castling's possibility doesn't correspond to the flag" );

    castle_index_[color_][1] = false;
    fmgr_.hashCastling(color_, 1);
  }

  en_passant_ = -1;

  if ( move.rindex_ >= 0 )
  {
    Figure & rfig = getFigure(ocolor, move.rindex_);
    move.eaten_type_ = rfig.getType();
    getField(rfig.where()).clear();
    fmgr_.decr(rfig);
    rfig.setType(Figure::TypeNone);
  }

  int pw_dy = move.to_ - fig.where();
  if ( Figure::TypePawn == fig.getType() && fig.isFirstStep() && (16 == pw_dy || -16 == pw_dy) )
  {
    en_passant_ = fig.getIndex();
    fmgr_.hashEnPassant(fig.where(), color_);
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

  if ( Figure::TypePawn == fig.getType() || move.rindex_ >= 0 )
  {
    move.irreversible_ = true;
    fiftyMovesCount_ = 0;
  }
  else
  {
    move.irreversible_ = false;
    fiftyMovesCount_++;
  }

  // remember hash code for threefold repetition detection
  fmgr_.hashColor();
  move.zcode_ = fmgr_.hashCode();

  if ( !color_ )
    movesCounter_++;

  return true;
}

void Board::undoMove()
{
  MoveCmd & move = moves_[halfmovesCounter_];

  // always restore state, because we have changed it
  state_ = (State)move.old_state_;

  if ( !move.need_undo_ )
    return;

  if ( !color_ )
    movesCounter_--;

  // restore hash color. we change it early for 3-fold repetition detection
  fmgr_.hashColor();

  Figure & fig = getFigure(color_, getField(move.to_).index());

  // restore figure

  // restore old type
  if ( move.new_type_ > 0 )
  {
    fmgr_.decr(fig);
    fig.go(move.from_);
    fig.setType(Figure::TypePawn);
    fmgr_.incr(fig);
  }
  else
  {
    fmgr_.move(fig, move.from_);
  }

  // restore position and field
  getField(move.to_) = move.field_to_;
  fig.setFirstStep(move.first_move_);
  getField(fig.where()).set(fig);

  // restore en-passant hash code
  if ( en_passant_ >= 0 )
    fmgr_.hashEnPassant(fig.where(), color_);

  // restore prev. en-passant index
  en_passant_ = move.en_passant_;

  // restore eaten figure
  if ( move.rindex_ >= 0 )
  {
    Figure::Color ocolor = Figure::otherColor(color_);
    Figure & rfig = getFigure(ocolor, move.rindex_);

    THROW_IF( move.eaten_type_ <= 0, "type of eaten figure is invalid" );
    rfig.setType((Figure::Type)move.eaten_type_);
    getField(rfig.where()).set(rfig);
    fmgr_.incr(rfig);
  }

  // restore castle possibility
  if ( move.castle_index_[0] != castle_index_[color_][0] )
  {
    fmgr_.hashCastling(color_, 0);
    castle_index_[color_][0] = move.castle_index_[0];
  }
  if ( move.castle_index_[1] != castle_index_[color_][1] )
  {
    fmgr_.hashCastling(color_, 1);
    castle_index_[color_][1] = move.castle_index_[1];
  }

  // restore king and rook after castling
  if ( move.rook_index_ >= 0 )
  {
    int d = (move.to_ - move.from_) >> 1;

    // restore rook's field. copy old one to restore all bits
    getField(move.rook_to_) = move.field_rook_to_;

    Figure & rook = getFigure(color_, move.rook_index_);
    fmgr_.move(rook, move.rook_from_);

    rook.setFirstStep(true);
    Field & rfield = getField(rook.where());
    rfield.set(rook);

    castle_[color_] = 0;
  }

  fiftyMovesCount_ = move.fifty_moves_;
}

bool Board::makeMove(const Move & mv)
{
  THROW_IF(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  halfmovesCounter_++;

  MoveCmd & move = moves_[halfmovesCounter_-1];
  move.clearUndo();
  move = mv;

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

  MoveCmd & move = moves_[halfmovesCounter_];

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
                ( fmgr_.bishops_b(Figure::ColorBlack) > 0 && fmgr_.bishops_w(Figure::ColorBlack) > 0 );

  can_win_[1] = ( fmgr_.pawns(Figure::ColorWhite) > 0 || fmgr_.rooks(Figure::ColorWhite) > 0 || fmgr_.queens(Figure::ColorWhite) > 0 ) ||
                ( fmgr_.knights(Figure::ColorWhite) > 0 && fmgr_.bishops(Figure::ColorWhite) > 0 ) ||
                ( fmgr_.bishops_b(Figure::ColorWhite) > 0 && fmgr_.bishops_w(Figure::ColorWhite) > 0 );

  if ( !can_win_[0] && !can_win_[1] )
  {
    state_ = DrawInsuf;
    return true;
  }

  // we don't need to verify threefold repetition if there was capture or pawn's move
  if ( 0 == fiftyMovesCount_ )
    return false;

  int reps = 1;
  for (int i = halfmovesCounter_-3; i >= 0; i -= 2)
  {
    if ( moves_[i].zcode_ == fmgr_.hashCode() )
      reps++;

    if ( reps >= 3 )
    {
      state_ = DrawReps;
      return true;
    }

    if ( moves_[i].irreversible_ )
      break;
  }

  return false;
}
