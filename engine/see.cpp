#include "Board.h"

// static exchange evaluation
int Board::see(int initial_value, Move & next, int & rdepth) const
{
  rdepth = 0;
  next.clear();

  if ( halfmovesCounter_ < 1 || (state_ != Ok && state_ != UnderCheck) )
    return 0;

  const MoveCmd & move = getMoveRev(0);
  const Field & tfield = getField(move.to_);
  THROW_IF( !tfield || tfield.color() == color_, "no figure on field after movement" );
  Figure::Color ocolor = Figure::otherColor(color_);
  Figure::Type ftype =  tfield.type();


  // calculate recapture result, starting from side to move (color_)
  // looking from side, that made 'move' recently
  int current_value = fmgr_.weight();
  if ( color_ )
    current_value = -current_value;

  int score_gain = current_value - initial_value;
  //if ( move.eaten_type_ )
  //  score_gain = Figure::figureWeight_[move.eaten_type_];
  //if ( move.new_type_ )
  //  score_gain += Figure::figureWeight_[move.new_type_]-Figure::figureWeight_[Figure::TypePawn];
  ScoreType fscore = -Figure::figureWeight_[ftype];

  // we don't need to continue if eaten figure is greater than attacker
  if ( score_gain > -fscore )
    return score_gain + fscore;

  // collect all attackers for each side
  // lo-byte = type, hi-byte = pos
  uint16 attackers[2][NumOfFigures];
  int figsN[2] = {0, 0}; 
  bool king_found[2] = { false, false };
  uint64 brq_masks[2] = {0ULL, 0ULL};

  for (int c = 0; c < 2; ++c)
  {
    int & num = figsN[c];
    brq_masks[c] = fmgr_.bishop_mask((Figure::Color)c) | fmgr_.rook_mask((Figure::Color)c) | fmgr_.queen_mask((Figure::Color)c);
    brq_masks[c] &= ~(1ULL << move.to_);

    // pawns
    uint64 pmask = fmgr_.pawn_mask_o((Figure::Color)c) & g_movesTable->pawnCaps_o((Figure::Color)((c+1)&1), move.to_);
    for ( ; pmask; )
    {
      int n = least_bit_number(pmask);
      attackers[c][num++] = Figure::TypePawn | (n << 8);
    }

    // knights
    uint64 nmask = fmgr_.knight_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeKnight, move.to_);
    for ( ; nmask; )
    {
      int n = least_bit_number(nmask);
      attackers[c][num++] = Figure::TypeKnight | (n << 8);
    }

    // bishops
    uint64 bmask = fmgr_.bishop_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeBishop, move.to_);
    for ( ; bmask; )
    {
      int n = least_bit_number(bmask);
      attackers[c][num++] = Figure::TypeBishop | (n << 8);
    }

    // rooks
    uint64 rmask = fmgr_.rook_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeRook, move.to_);
    for ( ; rmask; )
    {
      int n = least_bit_number(rmask);
      attackers[c][num++] = Figure::TypeRook | (n << 8);
    }

    // queens
    uint64 qmask = fmgr_.queen_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeQueen, move.to_);
    for ( ; qmask; )
    {
      int n = least_bit_number(qmask);
      attackers[c][num++] = Figure::TypeQueen | (n << 8);
    }

    // king
    uint64 kmask = fmgr_.king_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeKing, move.to_);
    if ( kmask )
    {
      const Figure & king = getFigure((Figure::Color)c, KingIndex);
      attackers[c][num++] = Figure::TypeKing | (king.where() << 8);
      king_found[c] = true;
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

  if ( figsN[color_] < 1 )
    return score_gain;


  // starting calculation
  int col = color_;
  uint64 all_mask_inv = ~(fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite));
  bool promotion = (move.to_ >> 3) == 0 || (move.to_ >> 3) == 7;

  THROW_IF( all_mask_inv & (1ULL << move.to_), "see: there no figure on move.to_ field" );

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
          if ( !see_check((Figure::Color)col, (attackers[col][i] >> 8) & 255, all_mask_inv, brq_masks[(col+1)&1]) )
          {
            attc = attackers[col][i];
            attackers[col][i] = 0;
          }
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

          if ( !see_check((Figure::Color)col, (attackers[col][i] >> 8) & 255, all_mask_inv, brq_masks[(col+1)&1]) )
          {
            attc = attackers[col][i];
            attackers[col][i] = 0;
          }
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

    if ( !next && col == color_ ) // save answer to current move
    {
      next.from_ = pos;
      next.to_ = move.to_;
      next.rindex_ = tfield.index();
      if ( promotion && t == Figure::TypePawn )
        next.new_type_ = Figure::TypeQueen;
    }

    rdepth++;
    score_gain += fscore;
    if ( t == Figure::TypePawn && promotion )
    {
      int dscore = Figure::figureWeight_[Figure::TypeQueen]-Figure::figureWeight_[Figure::TypePawn];
      if ( col == color_ )
        dscore = -dscore;
      score_gain += dscore;
      fscore = (col == color_) ? Figure::figureWeight_[Figure::TypeQueen] : -Figure::figureWeight_[Figure::TypeQueen];
    }
    else
      fscore = (col == color_) ? Figure::figureWeight_[t] : -Figure::figureWeight_[t];

    // don't need to continue if we haven't won material after capture
    if ( score_gain > 0 && col == color_ || score_gain < 0 && col != color_ )
      break;

    // if we give check we don't need to continue
    if ( see_check( (Figure::Color)((col+1)&1), (attc >> 8) & 255, all_mask_inv, brq_masks[col]) )
      break;

    // remove from (inverted) mask
    all_mask_inv |= (1ULL << pos);

    // remove from brq mask
    brq_masks[col] &= ~(1ULL << pos);

    // change color
    col = (col + 1) & 1;
  }

  return score_gain;
}

// static exchange evaluation
// almost the same as see(), need to be refactored
int Board::see_before(int initial_value, const Move & move) const
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
  uint16 attackers[2][NumOfFigures];
  int figsN[2] = {0, 0}; 
  bool king_found[2] = { false, false };
  uint64 brq_masks[2] = {0ULL, 0ULL};

  // pawn's movement without capture
  if ( move.rindex_ < 0 && ffield.type() == Figure::TypePawn )
    attackers[color][figsN[color]++] = Figure::TypePawn | (move.from_ << 8);

  for (int c = 0; c < 2; ++c)
  {
    int & num = figsN[c];
    brq_masks[c] = fmgr_.bishop_mask((Figure::Color)c) | fmgr_.rook_mask((Figure::Color)c) | fmgr_.queen_mask((Figure::Color)c);
    brq_masks[c] &= ~(1ULL << move.to_);

    // pawns
    uint64 pmask = fmgr_.pawn_mask_o((Figure::Color)c) & g_movesTable->pawnCaps_o((Figure::Color)((c+1)&1), move.to_);
    for ( ; pmask; )
    {
      int n = least_bit_number(pmask);
      attackers[c][num++] = Figure::TypePawn | (n << 8);
    }

    // knights
    uint64 nmask = fmgr_.knight_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeKnight, move.to_);
    for ( ; nmask; )
    {
      int n = least_bit_number(nmask);
      attackers[c][num++] = Figure::TypeKnight | (n << 8);
    }

    // bishops
    uint64 bmask = fmgr_.bishop_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeBishop, move.to_);
    for ( ; bmask; )
    {
      int n = least_bit_number(bmask);
      attackers[c][num++] = Figure::TypeBishop | (n << 8);
    }

    // rooks
    uint64 rmask = fmgr_.rook_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeRook, move.to_);
    for ( ; rmask; )
    {
      int n = least_bit_number(rmask);
      attackers[c][num++] = Figure::TypeRook | (n << 8);
    }

    // queens
    uint64 qmask = fmgr_.queen_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeQueen, move.to_);
    for ( ; qmask; )
    {
      int n = least_bit_number(qmask);
      attackers[c][num++] = Figure::TypeQueen | (n << 8);
    }

    // king
    uint64 kmask = fmgr_.king_mask((Figure::Color)c) & g_movesTable->caps(Figure::TypeKing, move.to_);
    if ( kmask )
    {
      const Figure & king = getFigure((Figure::Color)c, KingIndex);
      attackers[c][num++] = Figure::TypeKing | (king.where() << 8);
      king_found[c] = true;
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
  bool found = false;
  for (int i = 0; i < figsN[color]; ++i)
  {
    Figure::Type t =  (Figure::Type)(attackers[color][i] & 255);
    uint8 pos = (attackers[color][i] >> 8) & 255;
    if ( pos == move.from_ )
    {
      uint16 attc0 = attackers[color][i];
      for (int j = i; j > 0; --j)
        attackers[color][j] = attackers[color][j-1];
      attackers[color][0] = attc0;
      found = true;
      break;
    }
  }

  THROW_IF( !found, "move wasn't found in list of moves" );

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
          if ( !see_check((Figure::Color)col, (attackers[col][i] >> 8) & 255, all_mask_inv, brq_masks[(col+1)&1]) )
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

          if ( !see_check((Figure::Color)col, (attackers[col][i] >> 8) & 255, all_mask_inv, brq_masks[(col+1)&1]) )
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
    if ( see_check( (Figure::Color)((col+1)&1), pos, all_mask_inv, brq_masks[col]) )
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

// could we move this figure
bool Board::see_check(Figure::Color kc, uint8 from, const uint64 & all_mask_inv, const uint64 & a_brq_mask) const
{
//  Figure::Type t =  (Figure::Type)(attc & 255);
//  uint8 pos = (attc >> 8) & 255;

  const Figure & king = getFigure((Figure::Color)kc, KingIndex);

  // are king and field we move from on the same line
  Figure queen(king);
  queen.setType(Figure::TypeQueen);
  if ( g_figureDir->dir(queen, from) < 0 )
    return false;

  // is there some figure between king and field that we move from
  const uint64 & btw_king_msk = g_betweenMasks->between(king.where(), from);
  uint64 all_mask_inv2 = (all_mask_inv | (1ULL << from));
  if ( (btw_king_msk & all_mask_inv2) != btw_king_msk )
    return false;

  // then we need to check if there is some attacker on line to king
  uint64 from_msk = g_betweenMasks->from(king.where(), from) & a_brq_mask;
  for ( ; from_msk; )
  {
    int n = least_bit_number(from_msk);
    const Field & afield = getField(n);
    THROW_IF( !afield, "see: no BRQ of opponents color" );
    const Figure & fig = getFigure(afield.color(), afield.index());
    int dir = g_figureDir->dir(fig, king.where());
    if ( dir < 0 )
      continue;

    // no figure between king and attacker
    const uint64 & btw_mask = g_betweenMasks->between(king.where(), n);
    if ( (btw_mask & all_mask_inv2) == btw_mask )
      return true;
  }

  return false;
}
