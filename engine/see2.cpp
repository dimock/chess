/*************************************************************
  see.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Board.h"

// static exchange evaluation
int Board::see_before2(int initial_value, const Move & move) const
{
  if ( state_ != Ok )
    return 0;

  const Field & ffield = getField(move.from_);
  const Field & tfield = getField(move.to_);

  Figure::Color color = ffield.color();
  Figure::Color ocolor = Figure::otherColor(color);
  Figure::Type ftype =  ffield.type();

  // king
  if ( ffield.type() == Figure::TypeKing )
    return -Figure::WeightMat;

  // en-passant
  if ( !tfield && ffield.type() == Figure::TypePawn && move.rindex_ >= 0 )
    return 0;

  int current_value = fmgr_.weight();
  if ( !color )
    current_value = -current_value;

  ScoreType fscore = 0;
  int score_gain = current_value - initial_value;

  if ( tfield )
    fscore = Figure::figureWeightSEE_[tfield.type()];

  // collect all attackers for each side
  // lo-byte = type, hi-byte = pos
  uint16 attackers[2][NumOfFields];
  int figsN[2] = {0, 0}; 
  bool king_found[2] = { false, false };
  uint64 brq_masks[2] = {0ULL, 0ULL};

  // pawn's movement without capture
  //if ( move.rindex_ < 0 && ffield.type() == Figure::TypePawn )

  // put 'move' to 1st position
  attackers[color][figsN[color]++] = ffield.type() | (move.from_ << 8);

  bool promotion = (move.to_ >> 3) == 0 || (move.to_ >> 3) == 7;
  BitMask all_mask = fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite);

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
  }

  // look along directions.
  // long-attacking figures for both colors
  uint8 long_figures[2][3][NumOfFields];
  uint8 long_N[2][3] = { {0, 0, 0}, {0, 0, 0} };

  // nw
  {
    BitMask from_mask = g_betweenMasks->from_dir(move.to_, nst::nw) & all_mask;
    for ( ; from_mask; )
    {
      int n = clear_lsb(from_mask);
      if ( n == move.from_ )
        continue;

      const Field & field = getField(n);
      if ( field.type() == Figure::TypeBishop || field.type() == Figure::TypeQueen )
      {
        int ty = field.type()-Figure::TypeBishop;
        long_figures[field.color()][ty][long_N[field.color()][ty]] = n;
        long_N[field.color()][ty]++;
      }
      else
      {
        if ( field.type() != Figure::TypePawn )
          break;

        int dir = g_figureDir->dir(field.type(), field.color(), n, move.to_);
        if ( dir != 0 && dir != 1 )
          break;
      }
    }
  }

  // no
  {
    BitMask from_mask = g_betweenMasks->from_dir(move.to_, nst::no) & all_mask;
    for ( ; from_mask; )
    {
      int n = clear_lsb(from_mask);
      if ( n == move.from_ )
        continue;

      const Field & field = getField(n);
      if ( field.type() == Figure::TypeRook || field.type() == Figure::TypeQueen )
      {
        int ty = field.type()-Figure::TypeBishop;
        long_figures[field.color()][ty][long_N[field.color()][ty]] = n;
        long_N[field.color()][ty]++;
      }
      else if ( n != move.from_ ) // it isn't figure, that makes first 'move'
        break;
    }
  }

  // ne
  {
    BitMask from_mask = g_betweenMasks->from_dir(move.to_, nst::ne) & all_mask;
    for ( ; from_mask; )
    {
      int n = clear_lsb(from_mask);
      if ( n == move.from_ )
        continue;

      const Field & field = getField(n);
      if ( field.type() == Figure::TypeBishop || field.type() == Figure::TypeQueen )
      {
        int ty = field.type()-Figure::TypeBishop;
        long_figures[field.color()][ty][long_N[field.color()][ty]] = n;
        long_N[field.color()][ty]++;
      }
      else
      {
        if ( field.type() != Figure::TypePawn )
          break;

        // capturing pawn
        int dir = g_figureDir->dir(field.type(), field.color(), n, move.to_);
        if ( dir != 0 && dir != 1 )
          break;
      }
    }
  }

  // ea
  {
    BitMask from_mask = g_betweenMasks->from_dir(move.to_, nst::ea) & all_mask;
    for ( ; from_mask; )
    {
      int n = clear_lsb(from_mask);
      if ( n == move.from_ )
        continue;

      const Field & field = getField(n);
      if ( field.type() != Figure::TypeRook && field.type() != Figure::TypeQueen )
        break;

      int ty = field.type()-Figure::TypeBishop;
      long_figures[field.color()][ty][long_N[field.color()][ty]] = n;
      long_N[field.color()][ty]++;
    }
  }

  // se
  {
    BitMask from_mask = g_betweenMasks->from_dir(move.to_, nst::se) & all_mask;
    for ( ; from_mask; )
    {
      int n = clear_msb(from_mask);
      if ( n == move.from_ )
        continue;

      const Field & field = getField(n);
      if ( field.type() == Figure::TypeBishop || field.type() == Figure::TypeQueen )
      {
        int ty = field.type()-Figure::TypeBishop;
        long_figures[field.color()][ty][long_N[field.color()][ty]] = n;
        long_N[field.color()][ty]++;
      }
      else
      {
        if ( field.type() != Figure::TypePawn )
          break;

        // capturing pawn
        int dir = g_figureDir->dir(field.type(), field.color(), n, move.to_);
        if ( dir != 0 && dir != 1 )
          break;
      }
    }
  }

  // so
  {
    BitMask from_mask = g_betweenMasks->from_dir(move.to_, nst::so) & all_mask;
    for ( ; from_mask; )
    {
      int n = clear_msb(from_mask);
      if ( n == move.from_ )
        continue;

      const Field & field = getField(n);
      if ( field.type() == Figure::TypeRook || field.type() == Figure::TypeQueen )
      {
        int ty = field.type()-Figure::TypeBishop;
        long_figures[field.color()][ty][long_N[field.color()][ty]] = n;
        long_N[field.color()][ty]++;
      }
      else if ( n != move.from_ ) // it isn't figure, that makes first 'move'
        break;
    }
  }

  // sw
  {
    BitMask from_mask = g_betweenMasks->from_dir(move.to_, nst::sw) & all_mask;
    for ( ; from_mask; )
    {
      int n = clear_msb(from_mask);
      if ( n == move.from_ )
        continue;

      const Field & field = getField(n);
      if ( field.type() == Figure::TypeBishop || field.type() == Figure::TypeQueen )
      {
        int ty = field.type()-Figure::TypeBishop;
        long_figures[field.color()][ty][long_N[field.color()][ty]] = n;
        long_N[field.color()][ty]++;
      }
      else
      {
        if ( field.type() != Figure::TypePawn )
          break;

        // capturing pawn
        int dir = g_figureDir->dir(field.type(), field.color(), n, move.to_);
        if ( dir != 0 && dir != 1 )
          break;
      }
    }
  }

  // we
  {
    BitMask from_mask = g_betweenMasks->from_dir(move.to_, nst::we) & all_mask;
    for ( ; from_mask; )
    {
      int n = clear_msb(from_mask);
      if ( n == move.from_ )
        continue;

      const Field & field = getField(n);
      if ( field.type() != Figure::TypeRook && field.type() != Figure::TypeQueen )
        break;

      int ty = field.type()-Figure::TypeBishop;
      long_figures[field.color()][ty][long_N[field.color()][ty]] = n;
      long_N[field.color()][ty]++;
    }
  }

  // now add them to attackers list
  for (int c = 0; c < 2; ++c)
  {
    int & num = figsN[c];

    // bishops, rooks, queens
    for (int t = 0; t < 3; ++t)
    {
      for (int i = 0; i < long_N[c][t]; ++i)
      {
        int & num = figsN[c];
        THROW_IF(long_figures[c][t][i] < 0 || long_figures[c][t][i] > 63, "see2: invalid figure position" );
        attackers[c][num++] = (t+Figure::TypeBishop) | (long_figures[c][t][i] << 8);
      }
    }

    // at the last add kings
    uint64 kmask = fmgr_.king_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeKing, move.to_);
    if ( kmask )
    {
      const Figure & king = getFigure((Figure::Color)c, KingIndex);
      if ( king.where() != move.from_ )
      {
        attackers[c][num++] = Figure::TypeKing | (king.where() << 8);
        king_found[c] = true;
      }
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

  // find 'move' and put it to the 1st position
  //bool found = false;
  //for (int i = 0; i < figsN[color]; ++i)
  //{
  //  Figure::Type t =  (Figure::Type)(attackers[color][i] & 255);
  //  uint8 pos = (attackers[color][i] >> 8) & 255;
  //  if ( pos == move.from_ )
  //  {
  //    uint16 attc0 = attackers[color][i];
  //    for (int j = i; j > 0; --j)
  //      attackers[color][j] = attackers[color][j-1];
  //    attackers[color][0] = attc0;
  //    found = true;
  //    break;
  //  }
  //}

  //THROW_IF( !found, "move wasn't found in list of moves" );

  // starting calculation
  int col = color;
  uint64 all_mask_inv = ~all_mask;

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
          bool is_checking = see_check2((Figure::Color)col, (attackers[col][i] >> 8) & 255, all_mask_inv, brq_masks[(col+1)&1]);

#ifndef NDEBUG
          bool is_checking_o = see_check((Figure::Color)col, (attackers[col][i] >> 8) & 255, all_mask_inv, brq_masks[(col+1)&1]);
#endif
          THROW_IF( is_checking != is_checking_o, "error in see_check2()" );

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
          const uint64 & btw_mask = g_betweenMasks->between(pos, move.to_);
          if ( (btw_mask & all_mask_inv) != btw_mask )
            continue;

          bool is_checking = see_check2((Figure::Color)col, (attackers[col][i] >> 8) & 255, all_mask_inv, brq_masks[(col+1)&1]);

#ifndef NDEBUG
          bool is_checking_o = see_check((Figure::Color)col, (attackers[col][i] >> 8) & 255, all_mask_inv, brq_masks[(col+1)&1]);
#endif
          THROW_IF( is_checking != is_checking_o, "error in see_check2()" );

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
              const uint64 & btw_mask = g_betweenMasks->between(opos, move.to_);
              if ( (btw_mask & all_mask_inv) == btw_mask )
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
    bool give_check = see_check2( (Figure::Color)((col+1)&1), pos, all_mask_inv, brq_masks[col]);

#ifndef NDEBUG
    bool give_check_o = see_check( (Figure::Color)((col+1)&1), pos, all_mask_inv, brq_masks[col]);
#endif
    THROW_IF( give_check != give_check_o, "error in see_check2()" );

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
