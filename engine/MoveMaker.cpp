/*************************************************************
  MoveMaker.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Board.h"
#include "FigureDirs.h"

// unpack from hash, verify move's physical possibility
bool Board::unpack(const PackedMove & pm, Move & move) const
{
  move.clear();

  move.from_ = pm.from_;
  move.to_ = pm.to_;
  move.new_type_ = pm.new_type_;

  THROW_IF( (unsigned)move.to_ > 63 || (unsigned)move.from_ > 63, "invalid move given" );

  const Field & fto = getField(move.to_);
  if ( fto && (fto.color() == color_ || fto.type() == Figure::TypeKing) )
    return false;

  const Field & ffrom = getField(move.from_);
  if ( !ffrom || ffrom.color() != color_ )
    return false;

  const MoveCmd & mv_last = lastMove();

  // only kings move is possible
  if ( mv_last.checkingNum_ > 1 && ffrom.type() != Figure::TypeKing )
    return false;

  if ( ffrom.type() != Figure::TypePawn && move.new_type_ > 0 )
    return false;

  int dir = g_figureDir->dir(ffrom.type(), color_, move.from_, move.to_);
  if ( dir < 0 )
    return false;

  if ( fto || (en_passant_ == move.to_ && ffrom.type() == Figure::TypePawn) )
    move.capture_ = true;

  // castle
  if ( ffrom.type() == Figure::TypeKing && (8 == dir || 9 == dir) )
  {
    if ( mv_last.checkingNum_ )
      return false;

    if ( !castling(color_, dir-8) ) // 8 means short castling, 9 - long
      return false;

    if ( color_ && move.from_ != 4 || !color_ && move.from_ != 60 )
      return false;

    int d = (move.to_ - move.from_) >> 1; // +1 short, -1 long
    if ( move.capture_ || getField(move.to_) || getField(move.from_ + d) )
      return false;

    // long castling - field right to rook (or left to king) is occupied
    if ( dir == 9 && getField(move.to_-1) )
      return false;
  }

  switch ( ffrom.type() )
  {
  case Figure::TypePawn:
    {
      int8 y = move.to_ >> 3;
      if ( ((7 == y || 0 == y) && !move.new_type_) || (0 != y && 7 != y && move.new_type_) )
        return false;

      THROW_IF( move.new_type_ < 0 || move.new_type_ > Figure::TypeQueen, "invalid promotion piece" );

      if ( fto || move.to_ == en_passant_ )
      {
        return (0 == dir || 1 == dir);
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
      const BitMask & mask  = g_betweenMasks->between(move.from_, move.to_);
      const BitMask & black = fmgr_.mask(Figure::ColorBlack);
      const BitMask & white = fmgr_.mask(Figure::ColorWhite);

      bool ok = (mask & ~(black | white)) == mask;
      return ok;
    }
    break;
  }

  return true;
}

// verify move by rules
bool Board::validateMove(const Move & move) const
{  
  const Field & ffrom = getField(move.from_);
  const Field & fto   = getField(move.to_);

  Figure::Color ocolor = Figure::otherColor(color_);

  const MoveCmd & prev = lastMove();

  // validate under check
  if ( prev.checkingNum_ )
  {
#ifdef NDEBUG
    if ( move.checkVerified_ )
      return true;
#endif

    {
        if ( Figure::TypeKing == ffrom.type() )
        {
            THROW_IF((move.from_&7)-(move.to_&7) > 1 || (move.from_&7)-(move.to_&7) < -1, "try to castle under check");

            return !detectCheck(ocolor, move.to_, move.from_);
        }
        else if ( prev.checkingNum_ > 1 )
            return false;

        THROW_IF( (unsigned)prev.checking_[0] >= NumOfFields, "invalid checking figure" );

        // regular capture
        if ( move.to_ == prev.checking_[0] )
        {
            // maybe moving figure discovers check
            BitMask mask_all = fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack);
            mask_all |= (1ULL << move.to_);

            BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);
            brq_mask &= ~(1ULL << move.to_);

            int ki_pos = kingPos(color_);

            return !discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos);
        }

        // en-passant capture. we have to check the direction from king to en-passant pawn
        if ( move.to_ == en_passant_ && Figure::TypePawn == ffrom.type() )
        {
            int ep_pos = enpassantPos();
            BitMask mask_all = fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack);
            mask_all |= (1ULL << move.to_);
            mask_all ^= (1ULL << move.from_);
            mask_all &= ~(1ULL << ep_pos);

            BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);

            int ki_pos = kingPos(color_);

            // through removed en-passant pawn's field
            if ( discoveredCheck(ep_pos, ocolor, mask_all, brq_mask, ki_pos) )
                return false;

            // through moved pawn's field
            return !discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos);
        }

        // moving figure covers king
        {
            const Field & cfield = getField(prev.checking_[0]);
            THROW_IF( Figure::TypeKing == cfield.type(), "king is attacking king" );
            THROW_IF( !cfield, "king is attacked by non existing figure" );

            // Pawn and Knight could be only removed to escape from check
            if ( Figure::TypeKnight == cfield.type() || Figure::TypePawn == cfield.type() )
                return false;

            int ki_pos = kingPos(color_);
            const BitMask & protect_king_msk = g_betweenMasks->between(ki_pos, prev.checking_[0]);
            if ( (protect_king_msk & (1ULL << move.to_)) == 0 )
                return false;

            BitMask mask_all = fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack);
            mask_all |= (1ULL << move.to_);

            BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);
            brq_mask &= ~(1ULL << move.to_);

            return !discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos);
        }
    }

    return false;
  }

  // without check
  if ( ffrom.type() == Figure::TypeKing  )
  {
    // castling
    int d = move.to_ - move.from_;
    if ( (2 == d || -2 == d) )
    {
        THROW_IF( !castling(color_, d > 0 ? 0 : 1), "castling impossible" );
        THROW_IF( !verifyCastling(color_, d > 0 ? 0 : 1), "castling flag is invalid" );

        // don't do castling under check
        THROW_IF( prev.checkingNum_ > 0, "can not castle under check" );
        THROW_IF( move.capture_, "can't capture while castling" );
        THROW_IF( !(move.from_ == 4 && color_ || move.from_ == 60 && !color_), "kings position is wrong" );
        THROW_IF( getField(move.to_), "king position after castle is occupied" );

        d >>= 1;

        // verify if there is suitable rook for castling
        int rook_from = move.from_ + ((d>>1) ^ 3); //d < 0 ? move.from_ - 4 : move.from_ + 3

        THROW_IF( (rook_from != 0 && color_ && d < 0) || (rook_from != 7 && color_ && d > 0), "invalid castle rook position" );
        THROW_IF( (rook_from != 56 && !color_ && d < 0) || (rook_from != 63 && !color_ && d > 0), "invalid castle rook position" );

        int rook_to = move.from_ + d;

        THROW_IF( getField(rook_to), "field, that rook is going to move to, is occupied" );

        THROW_IF ( !(rook_from & 3) && getField(rook_from+1), "long castling impossible" );

        if ( detectCheck(ocolor, move.to_) || detectCheck(ocolor, rook_to) )
          return false;

        return true;
     }
    else
    {
        // other king's movements - don't put it under check
        return detectCheck(ocolor, move.to_, move.from_);
    }
  }
  else
  {
    BitMask mask_all = fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack);
    BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);
    int ki_pos = kingPos(color_);

    if ( discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos) )
      return false;

    // en-passant check
    if ( move.to_ == en_passant_ )
    {
      int ep_pos = enpassantPos();
      BitMask mask_all_ep = (mask_all ^ (1ULL << move.from_)) | (1ULL << en_passant_);
      return !discoveredCheck(ep_pos, ocolor, mask_all_ep, brq_mask, ki_pos);
    }

    return true;
  }
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
  if ( drawState() || matState() )
    return false;

  THROW_IF(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  MoveCmd & prev = lastMove();

  halfmovesCounter_++;

  MoveCmd & move = lastMove();
  move.clearUndo();
  move = mv;

  // store Zobrist key and state
  move.old_state_ = state_;
  move.zcode_old_ = fmgr_.hashCode();

  state_ = Ok;

  Figure::Color ocolor = Figure::otherColor((Figure::Color)color_);

  //////////////////////////////////////////////////////////////////////////
  /// do move here

  // store masks - don't undo it to save time
  move.mask_[0] = fmgr_.mask(Figure::ColorBlack);
  move.mask_[1] = fmgr_.mask(Figure::ColorWhite);

  if ( move.en_passant_ >= 0 )
    fmgr_.hashEnPassant(move.en_passant_, ocolor);

  /// hashing castle possibility
  int d = move.to_ - move.from_;
  if ( fig.getType() == Figure::TypeKing && (2 == d || -2 == d) )// castle
  {
    // don't do castling under check
    THROW_IF( prev.checkingNum_ > 0, "can not castle under check" );

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

  /// end of do move
  //////////////////////////////////////////////////////////////////////////


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

  THROW_IF( isAttacked(color_, kingPos(Figure::otherColor(color_))) && !underCheck(), "check isn't detected" );

  // now change color
  color_ = ocolor;

  return true;
}

void Board::unmakeMove()
{
  THROW_IF(halfmovesCounter_ <= 0 || halfmovesCounter_ > GameLength, "number of halfmoves is invalid");

  halfmovesCounter_--;

  MoveCmd & move = getMove(halfmovesCounter_);

  color_ = Figure::otherColor(color_);

  checkingNum_ = move.old_checkingNum_;
  checking_[0] = move.old_checking_[0];
  checking_[1] = move.old_checking_[1];

  can_win_[0] = move.can_win_[0];
  can_win_[1] = move.can_win_[1];

  //////////////////////////////////////////////////////////////////////////
  /// undo move here
  //////////////////////////////////////////////////////////////////////////

  undoMove();

  // always restore hash code and state
  state_ = (State)move.old_state_;
  fmgr_.restoreHash(move.zcode_old_);
}

bool Board::verifyChessDraw()
{
  if ( (fiftyMovesCount_ == 100 && !underCheck()) || (fiftyMovesCount_ > 100) )
  {
    state_ |= Draw50Moves;
    return true;
  }

  can_win_[0] = ( fmgr_.pawns(Figure::ColorBlack) || fmgr_.rooks(Figure::ColorBlack) || fmgr_.queens(Figure::ColorBlack) ) ||
                ( fmgr_.knights(Figure::ColorBlack) && fmgr_.bishops(Figure::ColorBlack) ) ||
                ( fmgr_.bishops_b(Figure::ColorBlack) && fmgr_.bishops_w(Figure::ColorBlack) );

  can_win_[1] = ( fmgr_.pawns(Figure::ColorWhite) || fmgr_.rooks(Figure::ColorWhite) || fmgr_.queens(Figure::ColorWhite) ) ||
                ( fmgr_.knights(Figure::ColorWhite) && fmgr_.bishops(Figure::ColorWhite) ) ||
                ( fmgr_.bishops_b(Figure::ColorWhite) && fmgr_.bishops_w(Figure::ColorWhite) );

  if ( !can_win_[0] && !can_win_[1] )
  {
    state_ |= DrawInsuf;
    return true;
  }

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
	  state_ |= DrawReps;
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
    fmgr_.hashEnPassant(en_passant_, color_);
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
