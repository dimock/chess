#include "Board.h"


bool Board::doMove(MoveCmd & move)
{
  Figure & fig = getFigure(color_, getField(move.from_).index());
  Figure::Color ocolor = Figure::otherColor(color_);
  move.en_passant_ = en_passant_;
  move.old_state_ = state_;
  move.index_ = fig.getIndex();

  state_ = Ok;

  int d = move.to_ - move.from_;
  if ( fig.getType() == Figure::TypeKing && (2 == d || -2 == d) )// castle
  {
    if ( checkingNum_ > 0 )
      return false;

    d >>= 1;

    move.rook_to_ = move.from_ + d;

    if ( getField(move.rook_to_) || getField(move.to_) )
      return false;

    THROW_IF( isAttacked(ocolor, fig.where()), "can't do castling under check" );
    THROW_IF( move.rindex_ >= 0, "can't eat while castling" );

    move.rook_from_ = move.from_ + ((d>>1) ^ 3);//d < 0 ? move.from_ - 4 : move.from_ + 3
    move.rook_index_ = getField(move.rook_from_).index();
    Figure & rook = getFigure(color_, move.rook_index_);

    Field & field_rook_from = getField(rook.where());
    THROW_IF( !rook.isFirstStep(), "rook was already moved" );
    THROW_IF( !field_rook_from, "there is no rook for castling" );

    if ( isAttacked(ocolor, move.rook_to_) )
      return false;

    field_rook_from.clear();
    fmgr_.move(rook, move.rook_to_);
    rook.setMoved();
    Field & field_rook_to  = getField(rook.where());
    move.field_rook_to_ = field_rook_to;
    field_rook_to.set(rook);

    castle_[color_] = 1 + (((unsigned)d & 0x80000000) >> 31);//d > 0 ? 1 : 2;
    state_ = Castle;
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

  if ( Figure::TypePawn == fig.getType() && fig.isFirstStep() )
  {
    int dy = move.to_ - fig.where();
    if ( 16 == dy || -16 == dy )
    {
      en_passant_ = fig.getIndex();
      fmgr_.hashEnPassant(fig.where(), color_);
    }
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

  movesCounter_++;

  move.fifty_moves_ = fiftyMovesCount_;

  if ( Figure::TypePawn == fig.getType() || move.rindex_ >= 0 )
    fiftyMovesCount_ = 0;
  else
    fiftyMovesCount_++;

  return true;
}

void Board::undoMove(MoveCmd & move)
{
  // always restore state, because we have changed it
  state_ = (State)move.old_state_;

  if ( !move.need_undo_ )
    return;

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
  if ( move.first_move_ )
    fig.setUnmoved();
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

  // restore king and rook after castling
  if ( move.rook_index_ >= 0 )
  {
    int d = (move.to_ - move.from_) >> 1;

    // restore rook's field
    getField(move.rook_to_) = move.field_rook_to_;

    Figure & rook = getFigure(color_, move.rook_index_);

    fmgr_.move(rook, move.rook_from_);

    rook.setUnmoved();
    Field & rfield = getField(rook.where());
    rfield.set(rook);

    castle_[color_] = 0;
  }

  movesCounter_--;
  fiftyMovesCount_ = move.fifty_moves_;
}

bool Board::makeMove(MoveCmd & move)
{
  if ( !doMove(move) )
  {
    state_ = Invalid;
    return false;
  }

  if ( !wasMoveValid(move) )
  {
    state_ = Invalid;
    return false;
  }

  isChecking(move);

  move.can_win_[0] = can_win_[0];
  move.can_win_[1] = can_win_[1];

  verifyChessDraw();

  if ( drawState() )
    return true;

  move.need_unmake_ = true;

  move.stage_ = stages_[color_];

  if ( move.rindex_ >= 0 && 0 == stages_[color_] ) // do we need to go to endgame if there is capture
  {
    Figure::Color ocolor = Figure::otherColor((Figure::Color)color_);
    if ( !( (fmgr_.queens(ocolor) > 0 && fmgr_.rooks(ocolor)+fmgr_.knights(ocolor)+fmgr_.bishops(ocolor) > 0) ||
            (fmgr_.rooks(ocolor) > 1 && fmgr_.bishops(ocolor) + fmgr_.knights(ocolor) > 1) ||
            (fmgr_.rooks(ocolor) > 0 && ((fmgr_.bishops_w(ocolor) > 0 && fmgr_.bishops_b(ocolor) > 0) || (fmgr_.bishops(ocolor) + fmgr_.knights(ocolor) > 2))) ) )
    {
      stages_[color_] = 1;
    }
  }

  move.old_checkingNum_ = checkingNum_;
  if ( checkingNum_ > 0 )
  {
    move.old_checking_[0] = checking_[0];
    move.old_checking_[1] = checking_[1];
  }

  checkingNum_ = move.checkingNum_;
  if ( checkingNum_ > 0 )
  {
    checking_[0] = move.checking_[0];
    checking_[1] = move.checking_[1];
    state_ = UnderCheck;
  }

  THROW_IF( isAttacked(color_, getFigure(Figure::otherColor(color_), KingIndex).where()) && UnderCheck != state_, "check isn't detected" );

  // now change color
  color_ = Figure::otherColor(color_);
  fmgr_.hashColor();

  return true;
}

void Board::unmakeMove(MoveCmd & move)
{
  if ( move.need_unmake_ )
  {
    fmgr_.hashColor();
    color_ = Figure::otherColor(color_);
    checkingNum_ = move.old_checkingNum_;

    can_win_[0] = move.can_win_[0];
    can_win_[1] = move.can_win_[1];

    if ( checkingNum_ > 0 )
    {
      checking_[0] = move.old_checking_[0];
      checking_[1] = move.old_checking_[1];
    }
  }

  undoMove(move);
}

void Board::verifyChessDraw()
{
  if ( drawState() )
    return;

  if ( fiftyMovesCount_ == 100 && UnderCheck != state_ || fiftyMovesCount_ > 100 )
  {
    state_ = Draw50Moves;
    return;
  }

  can_win_[0] = can_win_[1] = true;
  for (int n = 0; n < 2; ++n)
  {
    Figure::Color c = (Figure::Color)n;
    Figure::Color oc = (Figure::Color)((n + 1) & 1);
    if ( fmgr_.pawns(c) > 0 || fmgr_.rooks(c) > 0 || fmgr_.queens(c) > 0 )
      continue;

    if ( (fmgr_.count(c) == 0) ||
         (fmgr_.knights(c) <= 2 &&  fmgr_.bishops(c) == 0) ||
         (fmgr_.knights(c) == 0 && (fmgr_.bishops_b(c) == 0 || fmgr_.bishops_w(c) == 0)) )
    {
      can_win_[n] = false;
      continue;
    }
  }

  if ( !can_win_[0] && !can_win_[1] )
    state_ = DrawInsuf;
}
