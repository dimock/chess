/*************************************************************
  CapsGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

//////////////////////////////////////////////////////////////////////////
CapsGenerator::CapsGenerator(Board & board, Figure::Type minimalType, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), minimalType_(minimalType), player_(player), ply_(ply)
{
  numOfMoves_ = generate(alpha, betta, counter);
  captures_[numOfMoves_].clear();
}

Move & CapsGenerator::capture()
{
  for ( ;; )
  {
    Move * move = captures_ + numOfMoves_;
    Move * mv = captures_;
    for ( ; *mv; ++mv)
    {
      if ( mv->alreadyDone_ || mv->srt_score_ < move->srt_score_ )
        continue;

      move = mv;
    }
    if ( !*move )
      return *move;

    int see_gain = 0;
    if ( !move->seen_ && !see(*move, see_gain) )
    {
      move->seen_ = 1;

#ifdef USE_SEE_PRUNING
      move->alreadyDone_ = 1;
#else
      if ( move->rindex_ >= 0 )
        move->srt_score_ = see_gain + 10000;
      else
        move->srt_score_ = see_gain + 5000;
#endif

      continue;
    }

    move->alreadyDone_ = 1;
    return *move;
  }
}

bool CapsGenerator::see(Move & move, int & see_gain)
{
  THROW_IF(move.rindex_ < 0 && move.new_type_ == 0, "try to see() move that isn't capture or promotion");

  if ( move.rindex_ >= 0 )
  {
    // victim >= attacker
    Figure::Type vtype = board_.getFigure(Figure::otherColor(board_.getColor()), move.rindex_).getType();
    Figure::Type atype = board_.getField(move.from_).type();
    if ( Figure::figureWeightSEE_[vtype] >= Figure::figureWeightSEE_[atype] )
      return true;
  }

  // we look from side, that goes to move. we should adjust sing of initial mat-balance
  int initial_balance = board_.fmgr().weight();
  if ( !board_.getColor() )
    initial_balance = -initial_balance;

  see_gain = board_.see_before(initial_balance, move);
  return see_gain >= 0;
}

int CapsGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = 0;
  if ( minimalType_ > Figure::TypeQueen )
    return 0;

  Figure::Color ocolor = Figure::otherColor(board_.color_);

  const uint64 exclude_mask = 0xffffffffffff00;//72057594037927680;
  uint64 oppenent_mask = board_.fmgr_.mask(ocolor) ^ board_.fmgr_.king_mask(ocolor);
  uint64 oppenent_mask_p = oppenent_mask;

  if ( minimalType_ > Figure::TypePawn)
  {
    oppenent_mask ^= board_.fmgr_.pawn_mask_o(ocolor);
    oppenent_mask_p = oppenent_mask;
  }
  if ( minimalType_ > Figure::TypeKnight )
  {
    oppenent_mask ^= board_.fmgr_.knight_mask(ocolor);
    oppenent_mask_p ^= board_.fmgr_.knight_mask(ocolor) & exclude_mask;
  }
  if ( minimalType_ > Figure::TypeBishop )
  {
    oppenent_mask ^= board_.fmgr_.bishop_mask(ocolor);
    oppenent_mask_p ^= board_.fmgr_.bishop_mask(ocolor) & exclude_mask;
  }
  if ( minimalType_ > Figure::TypeRook )
  {
    oppenent_mask ^= board_.fmgr_.rook_mask(ocolor);
    oppenent_mask_p ^= board_.fmgr_.rook_mask(ocolor) & exclude_mask;
  }

  const uint64 & black = board_.fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr_.mask(Figure::ColorWhite);
  uint64 mask_all = white | black;
  uint64 mask_all_inv = ~mask_all;
  uint64 brq_mask = board_.fmgr_.bishop_mask(board_.color_) | board_.fmgr_.rook_mask(board_.color_) | board_.fmgr_.queen_mask(board_.color_);

  // generate pawn promotions (only if < 2 checking figures)
  const uint64 & pawn_msk = board_.fmgr_.pawn_mask_o(board_.color_);
  static int pw_delta[] = { -8, 8 };

  const Figure & oking = board_.getFigure(ocolor, Board::KingIndex);

  if ( minimalType_ <= Figure::TypeQueen )
  {
    uint64 promo_msk = board_.g_movesTable->promote_o(board_.color_);
    promo_msk &= pawn_msk;

    for ( ; promo_msk; )
    {
      int from = least_bit_number(promo_msk);

      THROW_IF( (unsigned)from > 63, "invalid promoted pawn's position" );

      const Field & field = board_.getField(from);

      THROW_IF( !field || field.color() != board_.color_ || field.type() != Figure::TypePawn, "there is no pawn to promote" );

      int to = from + pw_delta[board_.color_];

      THROW_IF( (unsigned)to > 63, "pawn tries to go to invalid field" );

      if ( board_.getField(to) )
        continue;

      add_capture(m, from, to, -1, Figure::TypeQueen);
    }
  }

  // firstly check if have at least 1 attacking pawn
  bool pawns_eat = false;

  uint64 pawn_eat_msk = 0;
  if ( board_.color_ )
    pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
  else
    pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);

  pawns_eat = (pawn_eat_msk & oppenent_mask_p) != 0;

  if ( !pawns_eat && board_.en_passant_ >= 0 && (minimalType_ <= Figure::TypePawn) )
  {
    const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
    THROW_IF( !epawn, "there is no en passant pawn" );

    int to = epawn.where() + pw_delta[board_.color_];
    THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

    pawns_eat = (pawn_eat_msk & (1ULL << to)) != 0;
  }

  // generate captures

  if ( board_.checkingNum_ < 2 )
  {
    // 1. Pawns
    if ( pawns_eat )
    {
      BitMask pw_mask = board_.fmgr().pawn_mask_o(board_.color_);

      for ( ; pw_mask; )
      {
        int pw_pos = most_bit_number(pw_mask);

        uint64 p_caps = board_.g_movesTable->pawnCaps_o(board_.color_, pw_pos) & oppenent_mask_p;

        for ( ; p_caps; )
        {
          THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

          int to = least_bit_number(p_caps);

          bool promotion = to > 55 || to < 8; // 1st || last line

          THROW_IF( (unsigned)to > 63, "invalid pawn's capture position" );

          const Field & field = board_.getField(to);
          if ( !field || field.color() != ocolor || (field.type() < minimalType_ && !promotion) )
            continue;

          add_capture(m, pw_pos, to, field.index(), promotion ? Figure::TypeQueen : 0);
        }

        if ( board_.en_passant_ >= 0 && minimalType_ <= Figure::TypePawn )
        {
          const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
          THROW_IF( !epawn, "there is no en passant pawn" );

          int to = epawn.where() + pw_delta[board_.color_];
          THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

          int dir = board_.g_figureDir->dir(Figure::TypePawn, board_.color_, pw_pos, to);
          if ( 0 == dir || 1 == dir )
          {
            THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

            add_capture(m, pw_pos, to, board_.en_passant_, 0);
          }
        }
      }
    }

    // 2. Knights
    BitMask kn_mask = board_.fmgr().knight_mask(board_.color_);
    for ( ; kn_mask; )
    {
      int kn_pos = most_bit_number(kn_mask);

      // don't need to verify capture possibility by mask
      uint64 f_caps = board_.g_movesTable->caps(Figure::TypeKnight, kn_pos) & oppenent_mask;
      for ( ; f_caps; )
      {
        int to = least_bit_number(f_caps);

        THROW_IF( (unsigned)to > 63, "invalid field index while capture" );

        const Field & field = board_.getField(to);

        THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

        THROW_IF( field.type() < minimalType_, "try to capture figure with score lower than required" );

        add_capture(m, kn_pos, to, field.index(), 0);
      }
    }

    // 3. Bishops + Rooks + Queens
    for (int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
    {
      BitMask fg_mask = board_.fmgr().type_mask((Figure::Type)type, board_.color_);
      for ( ; fg_mask; )
      {
        int fg_pos = most_bit_number(fg_mask);

        uint64 f_caps = board_.g_movesTable->caps((Figure::Type)type, fg_pos) & oppenent_mask;
        for ( ; f_caps; )
        {
          int8 to = least_bit_number(f_caps);

          const Field & field = board_.getField(to);
          THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

          THROW_IF( field.type() < minimalType_, "try to capture figure " );

          // can't go here
          const uint64 & btw_msk = board_.g_betweenMasks->between(fg_pos, to);
          if ( (btw_msk & mask_all_inv) != btw_msk )
            continue;

          add_capture(m, fg_pos, to, field.index(), 0);
        }
      }
    }
  }

  // 4. King
  {
    BitMask ki_mask = board_.fmgr().king_mask(board_.color_);

    THROW_IF( ki_mask == 0, "invalid position - no king" );

    int ki_pos = least_bit_number(ki_mask);

    // don't need to verify capture possibility by mask
    uint64 f_caps = board_.g_movesTable->caps(Figure::TypeKing, ki_pos) & oppenent_mask;
    for ( ; f_caps; )
    {
      int to = least_bit_number(f_caps);

      THROW_IF( (unsigned)to > 63, "invalid field index while capture" );

      const Field & field = board_.getField(to);

      THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

      THROW_IF( field.type() < minimalType_, "try to capture figure with score lower than required" );

      add_capture(m, ki_pos, to, field.index(), 0);
    }
  }

  return m;
}
