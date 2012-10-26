/*************************************************************
  see.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Board.h"

// static exchange evaluation
// have to be called before doing move
int Board::see(const Move & move) const
{
  if ( state_ == Invalid )
    return 0;

  const Field & ffield = getField(move.from_);
  const Field & tfield = getField(move.to_);

  int score_gain = 0;

  // victim >= attacker
  if ( tfield.type() )
  {
    score_gain = Figure::figureWeightSEE_[tfield.type()] - Figure::figureWeightSEE_[ffield.type()];
    if ( score_gain >= 0 )
      return score_gain;
  }
  // en-passant
  else if ( !tfield && ffield.type() == Figure::TypePawn && move.to_ == en_passant_ )
  {
    THROW_IF(board_.getField(enpassantPos()).type() != Figure::TypePawn || board_.getField(enpassantPos()).color() == color_, "no en-passant pawn");
    return 0;
  }


  Figure::Color color = ffield.color();
  Figure::Color ocolor = Figure::otherColor(color);
  Figure::Type  ftype =  ffield.type();

  ScoreType fscore = 0;

  if ( tfield )
    fscore = Figure::figureWeightSEE_[tfield.type()];

  // collect all attackers for each side
  // lo-byte = type, hi-byte = pos
  uint16 attackers[2][NumOfFields];
  int figsN[2] = {0, 0}; 
  bool king_found[2] = { false, false };
  uint64 brq_masks[2] = {0ULL, 0ULL};
  int ki_pos[2] = { -1, -1 };

  // push 1st move
  attackers[color][figsN[color]++] = ffield.type() | (move.from_ << 8);

  for (int c = 0; c < 2; ++c)
  {
    int & num = figsN[c];
    brq_masks[c] = fmgr_.bishop_mask((Figure::Color)c) | fmgr_.rook_mask((Figure::Color)c) | fmgr_.queen_mask((Figure::Color)c);
    brq_masks[c] &= ~(1ULL << move.to_);

    // pawns
    uint64 pmask = fmgr_.pawn_mask_o((Figure::Color)c) & g_movesTable->pawnCaps_o((Figure::Color)((c+1)&1), move.to_);
    for ( ; pmask; )
    {
      int n = clear_lsb(pmask);
      if ( n != move.from_ )
        attackers[c][num++] = Figure::TypePawn | (n << 8);
    }

    // knights
    uint64 nmask = fmgr_.knight_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeKnight, move.to_);
    for ( ; nmask; )
    {
      int n = clear_lsb(nmask);
      if ( n != move.from_ )
        attackers[c][num++] = Figure::TypeKnight | (n << 8);
    }

    // bishops
    uint64 bmask = fmgr_.bishop_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeBishop, move.to_);
    for ( ; bmask; )
    {
      int n = clear_lsb(bmask);
      if ( n != move.from_ )
        attackers[c][num++] = Figure::TypeBishop | (n << 8);
    }

    // rooks
    uint64 rmask = fmgr_.rook_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeRook, move.to_);
    for ( ; rmask; )
    {
      int n = clear_lsb(rmask);
      if ( n != move.from_ )
        attackers[c][num++] = Figure::TypeRook | (n << 8);
    }

    // queens
    uint64 qmask = fmgr_.queen_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeQueen, move.to_);
    for ( ; qmask; )
    {
      int n = clear_lsb(qmask);
      if ( n != move.from_ )
        attackers[c][num++] = Figure::TypeQueen | (n << 8);
    }

    // king
    BitMask kmask = fmgr_.king_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeKing, move.to_);
    if ( kmask )
    {
      int n = find_lsb(kmask);

      // save kings positions
      ki_pos[c] = n;

      if ( n != move.from_ )
      {
        attackers[c][num++] = Figure::TypeKing | (n << 8);
        king_found[c] = true;
      }
      else
      {
        // if king's movement is 1st its is last. we can't make recapture after it
        num = 1;
      }
    }
    else // no king's attack found
    {
      // save kings positions
      ki_pos[c] = kingPos((Figure::Color)c);
    }

    attackers[c][num] = (uint16)-1;
  }

  // if there are both kings they can't capture
  if ( king_found[0] && king_found[1] )
  {
    THROW_IF( figsN[0] < 1 || figsN[1] < 1 , "see: no figures but both kings found?" );
    attackers[0][--figsN[0]] = (uint16)-1;
    attackers[1][--figsN[1]] = (uint16)-1;
  }

  if ( figsN[color] < 1 )
    return score_gain;

  // starting calculation
  int col = color;
  uint64 all_mask_inv = ~(fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite));
  bool promotion = (move.to_ >> 3) == 0 || (move.to_ >> 3) == 7;

  for ( ;; )
  {
    // find attacker with minimal value
    uint16 attc = 0;
    for (int i = 0; !attc && i < figsN[col]; ++i)
    {
      if ( !attackers[col][i] )
        continue;

      Figure::Type t =  (Figure::Type)(attackers[col][i] & 255);
      uint8 pos = (attackers[col][i] >> 8) & 255;

      switch ( t )
      {
      case Figure::TypePawn:
      case Figure::TypeKnight:
        {
          bool is_checking = see_check((Figure::Color)col, (attackers[col][i] >> 8) & 255, ki_pos[col], all_mask_inv, brq_masks[(col+1)&1]);
          if ( !is_checking )
          {
            attc = attackers[col][i];
            attackers[col][i] = 0;
          }
          // illegal move
          else if ( col == color && 0 == i )
            return -Figure::WeightMat;
        }
        break;

        // bishop | rook | queen
      case Figure::TypeBishop:
      case Figure::TypeRook:
      case Figure::TypeQueen:
        {
          // can go to target field
          if ( is_something_between(pos, move.to_, all_mask_inv) )
            continue;

          bool is_checking = see_check((Figure::Color)col, (attackers[col][i] >> 8) & 255, ki_pos[col], all_mask_inv, brq_masks[(col+1)&1]);
          if ( !is_checking )
          {
            attc = attackers[col][i];
            attackers[col][i] = 0;
          }
          // illegal move
          else if ( col == color && 0 == i )
            return -Figure::WeightMat;
        }
        break;

        // only king left. need to verify check on target field
      case Figure::TypeKing:
        {
          bool check = false;
          int oc = (col+1) & 1;
          for (int j = 0; j < figsN[oc] && !check; ++j)
          {
            if ( !attackers[oc][j] )
              continue;

            Figure::Type ot = (Figure::Type)(attackers[oc][j] & 255);
            uint8 opos = (attackers[oc][j] >> 8) & 255;
            if ( ot == Figure::TypePawn || ot == Figure::TypeKnight )
              check = true;
            else
            {
              if ( !is_something_between(opos, move.to_, all_mask_inv) )
                check = true;
            }
          }
          if ( !check )
          {
            attc = attackers[col][i];
            attackers[col][i] = 0;
          }
        }
        break;
      }
    }

    if ( !attc )
      break;

    Figure::Type t =  (Figure::Type)(attc & 255);
    uint8 pos = (attc >> 8) & 255;

    score_gain += fscore;
    if ( t == Figure::TypePawn && promotion )
    {
      int dscore = Figure::figureWeightSEE_[Figure::TypeQueen]-Figure::figureWeightSEE_[Figure::TypePawn];
      if ( col != color )
        dscore = -dscore;
      score_gain += dscore;
      fscore = (col != color) ? Figure::figureWeightSEE_[Figure::TypeQueen] : -Figure::figureWeightSEE_[Figure::TypeQueen];
    }
    else
      fscore = (col != color) ? Figure::figureWeightSEE_[t] : -Figure::figureWeightSEE_[t];

    // don't need to continue if we haven't won material after capture
    if ( score_gain > 0 && col != color || score_gain < 0 && col == color )
      break;

    // if we give check we don't need to continue
    int ki_col = (col+1) & 1;
    bool give_check = see_check( (Figure::Color)ki_col, pos, ki_pos[ki_col], all_mask_inv, brq_masks[col]);
    if ( give_check )
      break;

    // remove from (inverted) mask
    all_mask_inv |= (1ULL << pos);

    // add to move.to_ field
    all_mask_inv &= ~(1ULL << move.to_);

    // remove from brq mask
    brq_masks[col] &= ~(1ULL << pos);

    // change color
    col = (col + 1) & 1;
  }

  return score_gain;
}
