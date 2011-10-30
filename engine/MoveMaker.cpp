#include "Board.h"


bool Board::doMove(MoveCmd & move)
{
  Figure & fig = getFigure(color_, getField(move.from_).index());
  Figure::Color ocolor = Figure::otherColor(color_);
  move.en_passant_ = en_passant_;
  move.old_state_ = state_;
  en_passant_ = -1;

  state_ = Ok;
  if ( fig.getType() == Figure::TypeKing )
  {
    int d = move.to_ - move.from_;
    if ( 2 == d || -2 == d ) // castle
    {
      if ( checkingNum_ > 0 )
        return false;

      d >>= 1;
      if ( getField(move.from_+d) || getField(move.to_) )
        return false;

      move.rook_index_ = getField(d < 0 ? move.from_ - 4 : move.from_ + 3).index();
      Figure & rook = getFigure(color_, move.rook_index_);

      getField(rook.where()).clear();
      fmgr_.move(rook, move.from_+d);
      rook.setMoved();
      getField(rook.where()).set(rook);

      castle_[color_] = d > 0 ? 1 : 2;
      state_ = Castle;
    }
  }

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

  getField(fig.where()).set(fig);
  move.need_undo_ = true;

  return true;
}

void Board::undoMove(MoveCmd & move)
{
  // restore state, because we have changed it
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
  getField(move.to_).clear();
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

    // clear rook's field
    getField(move.from_+d).clear();

    Figure & rook = getFigure(color_, move.rook_index_);

    fmgr_.move(rook, d < 0 ? move.from_ - 4 : move.from_ + 3);

    rook.setUnmoved();
    Field & rfield = getField(rook.where());
    rfield.set(rook);

    castle_[color_] = 0;
  }
}
