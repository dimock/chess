#include "Board.h"

// static exchange evaluation
int Board::see()
{
  if ( halfmovesCounter_ < 1 )
    return 0;

  THROW_IF( state_ == UnderCheck, "can't use see under check" );

  const MoveCmd & move = getMoveRev(0);
  
  const Field & tfield = getField(move.to_);

  THROW_IF( !tfield || tfield.color() == color_, "no figure on field after movement" );

  Figure::Color ocolor = Figure::otherColor(color_);

  Figure::Type ftype =  tfield.type();
  
  ScoreType fscore = Figure::figureWeight_[ftype];

  // collect all attackers for each side

  // lo-byte = type, hi-byte = pos
  uint16 attackers[2][NumOfFigures];
  int figsN[2] = {0, 0}; 
  bool king_found[2] = { false, false };

  for (int c = 0; c < 2; ++c)
  {
    int & num = figsN[c];

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
      Figure & king = getFigure((Figure::Color)c, Figure::TypeKing);
      attackers[c][num++] = Figure::TypeKing | (king.where() << 8);
      king_found[c] = true;
    }

    attackers[c][num] = (uint16)-1;
  }

  // if there are both kings they couldn't capture
  if ( king_found[0] && king_found[1] )
  {
    attackers[0][--figsN[0]] = (uint16)-1;
    attackers[1][--figsN[1]] = (uint16)-1;
  }

  // calculate recapture result, starting from side to move (color_)

  int score = move.eaten_type_ > 0 ? -Figure::figureWeight_[move.eaten_type_] : 0;
  if ( move.new_type_ )
    score += Figure::figureWeight_[Figure::TypePawn] - Figure::figureWeight_[move.new_type_];

  int col = color_;
  uint64 all_mask_inv = ~(fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite));
  bool promotion = (move.to_ >> 3) == 0 || (move.to_ >> 3) == 7;

  for ( ;; )
  {
    // find least attacker
    uint16 attc = 0;
    for (int i = 0; i < figsN[col]; ++i)
    {
      if ( !attackers[col][i] )
        continue;

      Figure::Type t =  (Figure::Type)(attackers[col][i] & 255);
      uint8 pos = (attackers[col][i] >> 8) & 255;

      if ( t == Figure::TypePawn || t == Figure::TypeKnight )
      {
        attc = attackers[col][i];
        attackers[col][i] = 0;
        break;
      }

      // only king left. need to verify check on target field
      if ( t == Figure::TypeKing )
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

        break;
      }

      // bishop | rook | queen
      {
        const uint64 & btw_mask = g_betweenMasks->between(pos, move.to_);
        if ( (btw_mask & all_mask_inv) != btw_mask )
          continue;

        // is valid?
        //bool valid = false;
        //Figure & king = getFigure(Figure::TypeKing, (Figure::Color)c);
        //uint64 & btw_king_msk = g_betweenMasks->between(king.where(), pos);
        //uint64 all_mask_inv2 = (all_mask_inv ^ (1ULL << pos)) | (1ULL << move.to_);
        //if ( (btw_king_msk & all_mask_inv2) != btw_king_msk )
        //  valid = true;
        //else
        //{
        //  uint64 from_msk = g_betweenMasks->from(king.where(), pos);
        //}

        {
          attc = attackers[col][i];
          attackers[col][i] = 0;
          break;
        }
      }

    }

    if ( !attc )
      break;

    Figure::Type t =  (Figure::Type)(attc & 255);
    uint8 pos = (attc >> 8) & 255;

    // remove from (inverted) mask
    all_mask_inv |= (1ULL << pos);

    score += fscore;
    fscore = (col == color_) ? -Figure::figureWeight_[t] : Figure::figureWeight_[t];
    if ( t == Figure::TypePawn && promotion )
    {
      score += col == color_ ? Figure::figureWeight_[Figure::TypeQueen]-Figure::figureWeight_[Figure::TypePawn] :
                              -Figure::figureWeight_[Figure::TypeQueen]+Figure::figureWeight_[Figure::TypePawn];
    }

    col = (col + 1) & 1;
  }

  return score;
}
