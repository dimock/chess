/*************************************************************
  CapsGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

//////////////////////////////////////////////////////////////////////////
CapsGenerator::CapsGenerator(const Move & hcap, Board & board, Figure::Type minimalType) :
  MovesGeneratorBase(board), hcap_(hcap)
{
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();
}

int CapsGenerator::generate()
{
  int m = 0;

  Figure::Color ocolor = Figure::otherColor(board_.color_);

  // generate only promotions to Queen with capture
  if ( minimalType_ > Figure::TypeQueen )
  {
    BitMask promo_mask = board_.g_movesTable->promote_o(board_.color_) & board_.fmgr().pawn_mask_o(board_.color_);
    for ( ; promo_mask; )
    {
      int from = clear_lsb(promo_mask);
      BitMask cap_mask = board_.g_movesTable->pawnCaps_o(board_.color_, from) & board_.fmgr().mask(ocolor);
      for ( ; cap_mask; )
      {
        int to = clear_lsb(cap_mask);
        const Field & field = board_.getField(to);
        THROW_IF( !field || field.color() != ocolor, "invalid promotion with capture in caps generator" );

        add(m, from, to, Figure::TypeQueen, true);
      }
    }
    return m;
  }

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

  // generate pawn promotions
  const uint64 & pawn_msk = board_.fmgr_.pawn_mask_o(board_.color_);

  {
    static int pw_delta[] = { -8, +8 };
    uint64 promo_msk = board_.g_movesTable->promote_o(board_.color_);
    promo_msk &= pawn_msk;

    for ( ; promo_msk; )
    {
      int from = clear_lsb(promo_msk);

      THROW_IF( (unsigned)from > 63, "invalid promoted pawn's position" );

      const Field & field = board_.getField(from);

      THROW_IF( !field || field.color() != board_.color_ || field.type() != Figure::TypePawn, "there is no pawn to promote" );

      int to = from + pw_delta[board_.color_];

      THROW_IF( (unsigned)to > 63, "pawn tries to go to invalid field" );

      if ( board_.getField(to) )
        continue;

      add(m, from, to, Figure::TypeQueen, false);
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
    pawns_eat = (pawn_eat_msk & (1ULL << board_.en_passant_)) != 0;

  // generate captures

  // 1. Pawns
  if ( pawns_eat )
  {
    BitMask pw_mask = board_.fmgr().pawn_mask_o(board_.color_);

    for ( ; pw_mask; )
    {
      int pw_pos = clear_msb(pw_mask);

      uint64 p_caps = board_.g_movesTable->pawnCaps_o(board_.color_, pw_pos) & oppenent_mask_p;

      for ( ; p_caps; )
      {
        THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

        int to = clear_lsb(p_caps);

        bool promotion = to > 55 || to < 8; // 1st || last line

        THROW_IF( (unsigned)to > 63, "invalid pawn's capture position" );

        const Field & field = board_.getField(to);
        if ( !field || field.color() != ocolor || (field.type() < minimalType_ && !promotion) )
          continue;

        add(m, pw_pos, to, promotion ? Figure::TypeQueen : Figure::TypeNone, true);
      }

      if ( board_.en_passant_ >= 0 && minimalType_ <= Figure::TypePawn )
      {
        THROW_IF( board_.getField(board_.enpassantPos()).type() != Figure::TypePawn || board_.getField(board_.enpassantPos()).color() != ocolor, "there is no en passant pawn" );

        int dir = board_.g_figureDir->dir(Figure::TypePawn, board_.color_, pw_pos, board_.en_passant_);
        if ( 0 == dir || 1 == dir )
        {
          THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

          add(m, pw_pos, board_.en_passant_, Figure::TypeNone, true);
        }
      }
    }
  }

  // 2. Knights
  BitMask kn_mask = board_.fmgr().knight_mask(board_.color_);
  for ( ; kn_mask; )
  {
    int kn_pos = clear_msb(kn_mask);

    // don't need to verify capture possibility by mask
    uint64 f_caps = board_.g_movesTable->caps(Figure::TypeKnight, kn_pos) & oppenent_mask;
    for ( ; f_caps; )
    {
      int to = clear_lsb(f_caps);

      THROW_IF( (unsigned)to > 63, "invalid field index while capture" );

      const Field & field = board_.getField(to);

      THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

      THROW_IF( field.type() < minimalType_, "try to capture figure with score lower than required" );

      add(m, kn_pos, to, Figure::TypeNone, true);
    }
  }

  // 3. Bishops + Rooks + Queens
  for (int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
  {
    BitMask fg_mask = board_.fmgr().type_mask((Figure::Type)type, board_.color_);
    for ( ; fg_mask; )
    {
      int fg_pos = clear_msb(fg_mask);

      uint64 f_caps = board_.g_movesTable->caps((Figure::Type)type, fg_pos) & oppenent_mask;
      for ( ; f_caps; )
      {
        int8 to = clear_lsb(f_caps);

        const Field & field = board_.getField(to);
        THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

        THROW_IF( field.type() < minimalType_, "try to capture figure " );

        // can't go here
        const uint64 & btw_msk = board_.g_betweenMasks->between(fg_pos, to);
        if ( (btw_msk & mask_all_inv) != btw_msk )
          continue;

        add(m, fg_pos, to, Figure::TypeNone, true);
      }
    }
  }

  // 4. King
  {
    BitMask ki_mask = board_.fmgr().king_mask(board_.color_);

    THROW_IF( ki_mask == 0, "invalid position - no king" );

    int ki_pos = clear_lsb(ki_mask);

    // don't need to verify capture possibility by mask
    uint64 f_caps = board_.g_movesTable->caps(Figure::TypeKing, ki_pos) & oppenent_mask;
    for ( ; f_caps; )
    {
      int to = clear_lsb(f_caps);

      THROW_IF( (unsigned)to > 63, "invalid field index while capture" );

      const Field & field = board_.getField(to);

      THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

      THROW_IF( field.type() < minimalType_, "try to capture figure with score lower than required" );

      add(m, ki_pos, to, Figure::TypeNone, true);
    }
  }

  return m;
}
