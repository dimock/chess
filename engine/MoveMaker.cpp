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

  // only kings move is possible
  if ( checkingNum_ > 1 && ffrom.type() != Figure::TypeKing )
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
    if ( checkingNum_ )
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

  // validate under check
  if ( checkingNum_ )
  {
#ifdef NDEBUG
    if ( move.checkVerified_ )
      return true;
#endif

    {
        if ( Figure::TypeKing == ffrom.type() )
        {
            THROW_IF((move.from_&7)-(move.to_&7) > 1 || (move.from_&7)-(move.to_&7) < -1, "try to castle under check");

            return !fieldUnderCheck(ocolor, move.to_, move.from_);
        }
        else if ( checkingNum_ > 1 )
            return false;

        THROW_IF( (unsigned)checking_[0] >= NumOfFields, "invalid checking figure" );

        // regular capture
        if ( move.to_ == checking_[0] )
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
            const Field & cfield = getField(checking_[0]);
            THROW_IF( Figure::TypeKing == cfield.type(), "king is attacking king" );
            THROW_IF( !cfield, "king is attacked by non existing figure" );

            // Pawn and Knight could be only removed to escape from check
            if ( Figure::TypeKnight == cfield.type() || Figure::TypePawn == cfield.type() )
                return false;

            int ki_pos = kingPos(color_);
            const BitMask & protect_king_msk = g_betweenMasks->between(ki_pos, checking_[0]);
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
        THROW_IF( checkingNum_ > 0, "can not castle under check" );
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

        if ( fieldUnderCheck(ocolor, move.to_) || fieldUnderCheck(ocolor, rook_to) )
          return false;

        return true;
     }
    else
    {
        // other king's movements - don't put it under check
        return fieldUnderCheck(ocolor, move.to_, move.from_);
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

bool Board::makeMove(const Move & mv)
{
  if ( drawState() || matState() )
    return false;

  THROW_IF(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  halfmovesCounter_++;

  MoveCmd & move = lastMove();
  move.clearUndo();
  move = mv;

  // store Zobrist key and state
  move.old_state_ = state_;
  move.zcode_old_ = fmgr_.hashCode();

  // store checking info
  move.checking_figs_ = checking_figs_;
  move.checkingNum_ = checkingNum_;

  move.castling_ = castling_;

  state_ = Ok;

  Figure::Color ocolor = Figure::otherColor((Figure::Color)color_);

  // store masks - don't undo it to save time
  move.mask_[0] = fmgr_.mask(Figure::ColorBlack);
  move.mask_[1] = fmgr_.mask(Figure::ColorWhite);

  Field & ffrom = getField(move.from_);
  Field & fto = getField(move.to_);

  // castle
  bool castle_k = castling(color_, 0);
  bool castle_q = castling(color_, 1);
  bool castle_kk = castle_k, castle_qq = castle_q;

  if ( ffrom.type() == Figure::TypeKing )
  {
      int d = move.to_ - move.from_;
      if ( (2 == d || -2 == d) )
      {
        // don't do castling under check
        THROW_IF( checkingNum_ > 0, "can not castle under check" );

        d >>= 1;
        int rook_to = move.from_ + d;
        int rook_from = move.from_ + ((d>>1) ^ 3);//d < 0 ? move.from_ - 4 : move.from_ + 3

        Field & rf_field = getField(rook_from);
        THROW_IF( rf_field.type() != Figure::TypeRook || rf_field.color() != color_, "no rook for castle" );
        THROW_IF( !(rook_from & 3) && getField(rook_from+1), "long castle is impossible" );

        rf_field.clear();
        fmgr_.move(color_, Figure::TypeRook, rook_from, rook_to);
        Field & field_rook_to  = getField(rook_to);
        THROW_IF( field_rook_to, "field that rook is going to move to while castling is occupied" );
        field_rook_to.set(color_, Figure::TypeRook);
      }

      castle_kk = false;
      castle_qq = false;
  }
  else if ( ffrom.type() == Figure::TypeRook )
  {
      if ( (move.from_ & 7) == 7 ) // short castle
          castle_kk = false;

      if ( (move.from_ & 7) == 0 ) // long castle
          castle_qq = false;
  }

  // hash castle and change flags
  if ( castle_k && !castle_kk )
  {
      clear_castling(color_, 0);
      fmgr_.hashCastling(color_, 0);
  }

  if ( castle_q && !castle_qq )
  {
      clear_castling(color_, 1);
      fmgr_.hashCastling(color_, 1);
  }

  // remove captured figure
  if ( fto )
  {
    THROW_IF(fto.color() == color_, "invalid color of captured figure" );

    move.eaten_type_ = fto.type();
    fmgr_.decr(fto.color(), fto.type(), move.to_);
    fto.clear();
  }
  else if ( move.to_ == en_passant_ )
  {
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    THROW_IF( epfield.color() != ocolor || epfield.type() != Figure::TypePawn, "en-passant pawn is invalid" );
    fmgr_.decr(epfield.color(), epfield.type(), ep_pos);
  }

  // clear en-passant hash code
  if ( en_passant_ >= 0 )
    fmgr_.hashEnPassant(move.en_passant_, ocolor);

  // save en-passant field
  move.en_passant_ = en_passant_;
  en_passant_ = -1;


  // en-passant
  if ( Figure::TypePawn == ffrom.type() )
  {
    int dir = g_figureDir->dir(ffrom.type(), ffrom.color(), move.from_, move.to_);
    if ( 3 == dir )
    {
      en_passant_ = (move.to_ + move.from_) >> 1;
      fmgr_.hashEnPassant(move.to_, color_);
    }
  }

  // move figure 'from' -> 'to'
  if ( move.new_type_ > 0 )
  {
    // pawn promotion
    fmgr_.decr(ffrom.color(), ffrom.type(), move.from_);
    fto.set(color_, (Figure::Type)move.new_type_);
    fmgr_.incr(fto.color(), fto.type(), move.to_);
  }
  else
  {
    // usual movement
    fmgr_.move(ffrom.color(), ffrom.type(), move.from_, move.to_);
    fto.set(color_, ffrom.type());
  }

  // clear field, figure moved from
  ffrom.clear();

  // update counters
  move.fifty_moves_  = fiftyMovesCount_;
  move.reps_counter_ = repsCounter_;

  if ( Figure::TypePawn == fto.type() || move.capture_ || move.new_type_ > 0 )
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

  movesCounter_ += ~color_ & 1;

  move.can_win_[0] = can_win_[0];
  move.can_win_[1] = can_win_[1];

  verifyChessDraw();

  detectCheck(move);

  move.state_ = state_;

  //THROW_IF( isAttacked(color_, kingPos(Figure::otherColor(color_))) && !underCheck(), "check isn't detected" );

  // now change color
  color_ = ocolor;

  return true;
}

void Board::unmakeMove()
{
  THROW_IF(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  MoveCmd & move = lastMove();
  halfmovesCounter_--;

  color_ = Figure::otherColor(color_);

  // store checking info
  checking_figs_ = move.checking_figs_;
  checkingNum_ = move.checkingNum_;

  // restore castling flags
  castling_ = move.castling_;

  Figure::Color ocolor = Figure::otherColor((Figure::Color)color_);

  // store masks - don't undo it to save time
  move.mask_[0] = fmgr_.mask(Figure::ColorBlack);
  move.mask_[1] = fmgr_.mask(Figure::ColorWhite);

  can_win_[0] = move.can_win_[0];
  can_win_[1] = move.can_win_[1];

  movesCounter_ -= ~color_ & 1;

  Field & ffrom = getField(move.from_);
  Field & fto = getField(move.to_);

  // restore figure
  if ( move.new_type_ > 0 )
  {
    // restore old type
    fmgr_.u_decr(fto.color(), fto.type(), move.to_);
    fmgr_.u_incr(fto.color(), Figure::TypePawn, move.from_);
    ffrom.set(color_, Figure::TypePawn);
  }
  else
  {
    fmgr_.u_move(fto.color(), fto.type(), move.to_, move.from_);
    ffrom.set(fto.color(), fto.type());
  }
  fto.clear();

  // restore en-passant
  en_passant_ = move.en_passant_;
  if ( en_passant_ == move.to_ )
  {
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    epfield.set(ocolor, Figure::TypePawn);
  }

  // restore eaten figure
  if ( move.eaten_type_ > 0 )
  {
    fto.set(ocolor, (Figure::Type)move.eaten_type_);
    fmgr_.u_incr(fto.color(), fto.type(), move.to_);
  }

  // restore rook after castling
  if ( ffrom.type() == Figure::TypeKing )
  {
    int d = move.to_ - move.from_;
    if ( (2 == d || -2 == d) )
    {
      d >>= 1;
      int rook_to = move.from_ + d;
      int rook_from = move.from_ + ((d>>1) ^ 3);//d < 0 ? move.from_ - 4 : move.from_ + 3

      Field & rfrom = getField(rook_from);
      Field & rto = getField(rook_to);

      rto.clear();
      rfrom.set(color_, Figure::TypeRook);
      fmgr_.u_move(color_, Figure::TypeRook, rook_to, rook_from);
    }
  }

  // restore figures masks
  fmgr_.restoreMasks(move.mask_);

  fiftyMovesCount_ = move.fifty_moves_;
  repsCounter_ = move.reps_counter_;

  // restore hash code and state
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
