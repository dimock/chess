/*************************************************************
  ChecksGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

#define USE_DISCOVERED2

//////////////////////////////////////////////////////////////////////////
ChecksGenerator::ChecksGenerator(CapsGenerator * cg, Board & board, int ply, Player & player, ScoreType & alpha, ScoreType betta, Figure::Type minimalType, int & counter) :
  board_(board), player_(player), ply_(ply), numOfMoves_(0), cg_(cg), minimalType_(minimalType)
{
#ifdef USE_KILLER
	if ( player_.contexts_[ply_].killer_ )
		killer_ = player_.contexts_[ply_].killer_;
	else
#endif
		killer_.clear();

  numOfMoves_ = generate(alpha, betta, counter);
  checks_[numOfMoves_].clear();
}

int ChecksGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = 0;

  Figure::Color color  = board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  const BitMask & oki_mask = board_.fmgr().king_mask(ocolor);
  int oki_pos = find_lsb(oki_mask);

  BitMask brq_mask = board_.fmgr_.bishop_mask(board_.color_) | board_.fmgr_.rook_mask(board_.color_) | board_.fmgr_.queen_mask(board_.color_);
  const BitMask & pw_check_mask = board_.g_movesTable->pawnCaps_o(ocolor, oki_pos);
  const BitMask & knight_check_mask = board_.g_movesTable->caps(Figure::TypeKnight, oki_pos);
  const BitMask & black = board_.fmgr_.mask(Figure::ColorBlack);
  const BitMask & white = board_.fmgr_.mask(Figure::ColorWhite);
  BitMask mask_all = white | black;

  const BitMask & kn_check_mask = board_.g_movesTable->caps(Figure::TypeKnight, oki_pos);


  // 1. King
  {
    const BitMask & ki_mask = board_.fmgr().king_mask(color);
    int ki_pos = find_lsb(ki_mask);

    // figure opens line between attacker and king

#ifdef USE_DISCOVERED2
    bool discovered = board_.discoveredCheck(ki_pos, color, mask_all, brq_mask, oki_pos);
#else
    bool discovered = discoveredCheck(ki_pos, mask_all, brq_mask, oki_pos);
#endif

    const BitMask & from_mask = board_.g_betweenMasks->from(oki_pos, ki_pos);

    if ( discovered )
    {
      const int8 * table = board_.g_movesTable->king(ki_pos);

      for (; *table >= 0; ++table)
      {
        const Field & field = board_.getField(*table);
        if ( field && (field.color() == color || field.type() >= minimalType_) )
          continue;

        // if king moves along line of attack, it covers opponent from check
        if ( from_mask & (1ULL << *table) )
          continue;

        add_check(m, ki_pos, *table, field ? field.index() : -1, Figure::TypeNone, discovered);
      }
    }

    // castling also could be with check
    BitMask all_but_king_mask = ~mask_all | (1ULL << ki_pos);
    if ( board_.castling(board_.color_, 0) && !board_.getField(ki_pos+2) ) // short
    {
      int r_pos = board_.color_ ? 7 : 63;
      const Field & rfield = board_.getField(r_pos);
      THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
      int r_pos_to = r_pos - 2;
      BitMask rk_mask = board_.g_betweenMasks->between(r_pos_to, oki_pos);
      if ( board_.g_figureDir->dir(Figure::TypeRook, board_.color_, r_pos_to, oki_pos) >= 0 && (rk_mask & all_but_king_mask) == rk_mask )
      {
        add_check(m, ki_pos, ki_pos+2, -1, Figure::TypeNone, discovered);
      }
    }

    if ( board_.castling(board_.color_, 1) && !board_.getField(ki_pos-2) ) // long
    {
      int r_pos = board_.color_ ? 0 : 56;
      const Field & rfield = board_.getField(r_pos);
      THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
      int r_pos_to = r_pos + 3;
      BitMask rk_mask = board_.g_betweenMasks->between(r_pos_to, oki_pos);
      if ( board_.g_figureDir->dir(Figure::TypeRook, board_.color_, r_pos_to, oki_pos) >= 0 && (rk_mask & all_but_king_mask) == rk_mask )
        add_check(m, ki_pos, ki_pos-2, -1, Figure::TypeNone, discovered);
    }
  }

  // 2. Bishop + Rook + Queen
  {
    for (int t = Figure::TypeBishop; t < Figure::TypeKing; ++t)
    {
      Figure::Type type = (Figure::Type)t;
      BitMask fg_mask = board_.fmgr().type_mask(type, color);

      for ( ; fg_mask; )
      {
        int fg_pos = clear_lsb(fg_mask);

#ifdef USE_DISCOVERED2
        bool discovered = board_.discoveredCheck(fg_pos, color, mask_all, brq_mask, oki_pos);
#else
        bool discovered = discoveredCheck(fg_pos, mask_all, brq_mask, oki_pos);
#endif

        if ( discovered )
        {
          const uint16 * table = board_.g_movesTable->move(type-Figure::TypeBishop, fg_pos);

          for (; *table; ++table)
          {
            const int8 * packed = reinterpret_cast<const int8*>(table);
            int8 count = packed[0];
            int8 delta = packed[1];

            int8 p = fg_pos;
            for ( ; count; --count)
            {
              p += delta;

              const Field & field = board_.getField(p);
              if ( field && (field.color() == color || field.type() >= minimalType_) )
                break;

              add_check(m, fg_pos, p, field ? field.index() : -1, Figure::TypeNone, discovered);

              if ( field )
                break;
            }
          }
        }
        else
        {
          BitMask mask_all_inv_ex = ~(mask_all & ~(1ULL << fg_pos));
          BitMask f_msk = board_.g_movesTable->caps(type, fg_pos) & board_.g_movesTable->caps(type, oki_pos);
          for ( ; f_msk; )
          {
            int to = clear_lsb(f_msk);

            const Field & field = board_.getField(to);
            if ( field && (field.color() == color || field.type() >= minimalType_) )
              continue;

            // can we check from this position?
            if ( board_.is_something_between(to, oki_pos, mask_all_inv_ex) )
              continue;

            // can we go to this position?
            if ( board_.is_something_between(to, fg_pos, mask_all_inv_ex) )
              continue;

            add_check(m, fg_pos, to, field ? field.index() : -1, Figure::TypeNone, discovered);
          }
        }
      }
    }
  }

  // 3. Knight
  {
    BitMask kn_mask = board_.fmgr().knight_mask(color);
    for ( ; kn_mask; )
    {
      int kn_pos = clear_lsb(kn_mask);

#ifdef USE_DISCOVERED2
      bool discovered = board_.discoveredCheck(kn_pos, color, mask_all, brq_mask, oki_pos);
#else
      bool discovered = discoveredCheck(kn_pos, mask_all, brq_mask, oki_pos);
#endif

      if ( discovered )
      {
        const int8 * table = board_.g_movesTable->knight(kn_pos);

        for (; *table >= 0; ++table)
        {
          const Field & field = board_.getField(*table);
          if ( field && (field.color() == color || field.type() >= minimalType_) )
            continue;

          add_check(m, kn_pos, *table, field ? field.index() : -1, Figure::TypeNone, discovered);
        }
      }
      else
      {
        BitMask kn_msk = board_.g_movesTable->caps(Figure::TypeKnight, kn_pos) & knight_check_mask;
        for ( ; kn_msk; )
        {
          int to = clear_lsb(kn_msk);

          const Field & field = board_.getField(to);
          if ( field && (field.color() == color || field.type() >= minimalType_))
            continue;

          add_check(m, kn_pos, to, field ? field.index() : -1, Figure::TypeNone, discovered);
        }
      }
    }
  }

  // 4. Pawn
  {
    BitMask pw_mask = board_.fmgr().pawn_mask_o(color);
    for ( ; pw_mask; )
    {
      int pw_pos = clear_lsb(pw_mask);

#ifdef USE_DISCOVERED2
      bool discovered = board_.discoveredCheck(pw_pos, color, mask_all, brq_mask, oki_pos);
#else
      bool discovered = discoveredCheck(pw_pos, mask_all, brq_mask, oki_pos);
#endif

      const BitMask & from_mask = board_.g_betweenMasks->from(oki_pos, pw_pos);

      const int8 * table = board_.g_movesTable->pawn(color, pw_pos);
      for (int i = 0; i < 4; ++i, ++table)
      {
        bool ep_checking = false;
        int rindex = -1;
        if ( i < 2 )
        {
          if ( *table < 0 )
            continue;

          const Field & field = board_.getField(*table);
          if ( field && field.color() == ocolor )
            rindex = field.index();
          else if ( board_.en_passant_ >= 0 )
          {
            const Figure & rfig = board_.getFigure(ocolor, board_.en_passant_);
            int8 to = rfig.where();
            static const int8 delta_pos[] = {8, -8};
            to += delta_pos[ocolor];
            if ( to == *table )
            {
              rindex = board_.en_passant_;
              BitMask pw_msk_to = 1ULL << *table;
              BitMask pw_msk_from = 1ULL << pw_pos;
              BitMask mask_all_pw = (mask_all ^ pw_msk_from) | pw_msk_to;

#ifdef USE_DISCOVERED2
              ep_checking = board_.discoveredCheck(rfig.where(), color, mask_all_pw, brq_mask, oki_pos);
#else
              ep_checking = discoveredCheck(rfig.where(), mask_all_pw, brq_mask, oki_pos);
#endif
            }
          }

          if ( rindex < 0 )
            continue;

          const Figure & rfig = board_.getFigure(ocolor, rindex);

          if ( rfig.getType() >= minimalType_ )
            continue;
        }
        else if ( *table < 0 || board_.getField(*table) )
          break;

        BitMask pw_msk_to = 1ULL << *table;
        bool promotion = *table > 55 || *table < 8;

        // pawn doesn't cover opponent's king in its new position
        bool doesnt_cover = (from_mask & (1ULL << *table)) == 0;

        if ( (discovered && doesnt_cover) || ep_checking || (pw_msk_to & pw_check_mask) )
          add_check(m, pw_pos, *table, rindex, promotion ? Figure::TypeQueen : Figure::TypeNone, discovered);
        // if it's not check, it could be promotion to knight
        else if ( promotion )
        {
          if ( knight_check_mask & pw_msk_to )
            add_check(m, pw_pos, *table, rindex, Figure::TypeKnight, discovered);
          // may be we haven't generated promotion to checking queen yet
          else if ( minimalType_ > Figure::TypeQueen )
          {
            // could we check from this position?
            int dir = board_.g_figureDir->dir(Figure::TypeQueen, color, *table, oki_pos);
            if ( dir >= 0 )
            {
              BitMask mask_all_inv_ex = ~(mask_all & ~(1ULL << pw_pos));
              if ( !board_.is_something_between(*table, oki_pos, mask_all_inv_ex) )
                add_check(m, pw_pos, *table, rindex, Figure::TypeQueen, discovered);
            }
          }
        }
      }
    }
  }

  return m;
}
//////////////////////////////////////////////////////////////////////////

ChecksGenerator2::ChecksGenerator2(CapsGenerator * cg, Board & board, int ply, Player & player, ScoreType & alpha, ScoreType betta, Figure::Type minimalType, int & counter) :
board_(board), player_(player), ply_(ply), numOfMoves_(0), cg_(cg), minimalType_(minimalType)
{
#ifdef USE_KILLER
  if ( player_.contexts_[ply_].killer_ )
    killer_ = player_.contexts_[ply_].killer_;
  else
#endif
    killer_.clear();

  numOfMoves_ = generate(alpha, betta, counter);
  checks_[numOfMoves_].clear();
}

int ChecksGenerator2::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = 0;

  Figure::Color color  = board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  const BitMask & oki_mask = board_.fmgr().king_mask(ocolor);
  int oki_pos = find_lsb(oki_mask);

  BitMask brq_mask = board_.fmgr_.bishop_mask(board_.color_) | board_.fmgr_.rook_mask(board_.color_) | board_.fmgr_.queen_mask(board_.color_);
  const BitMask & knight_check_mask = board_.g_movesTable->caps(Figure::TypeKnight, oki_pos);
  const BitMask & black = board_.fmgr_.mask(Figure::ColorBlack);
  const BitMask & white = board_.fmgr_.mask(Figure::ColorWhite);
  BitMask mask_all = white | black;

  const BitMask & kn_check_mask = board_.g_movesTable->caps(Figure::TypeKnight, oki_pos);


  // 1. King
  {
    const BitMask & ki_mask = board_.fmgr().king_mask(color);
    int ki_pos = find_lsb(ki_mask);

    // figure opens line between attacker and king
    bool discovered = board_.discoveredCheck(ki_pos, color, mask_all, brq_mask, oki_pos);

    const BitMask & from_mask = board_.g_betweenMasks->from(oki_pos, ki_pos);

    if ( discovered )
    {
      const int8 * table = board_.g_movesTable->king(ki_pos);

      for (; *table >= 0; ++table)
      {
        const Field & field = board_.getField(*table);
        if ( field && (field.color() == color || field.type() >= minimalType_) )
          continue;

        // if king moves along line of attack, it covers opponent from check
        if ( from_mask & (1ULL << *table) )
          continue;

        add_check(m, ki_pos, *table, field ? field.index() : -1, Figure::TypeNone, discovered);
      }
    }

    // castling also could be with check
    BitMask all_but_king_mask = ~mask_all | (1ULL << ki_pos);
    if ( board_.castling(board_.color_, 0) && !board_.getField(ki_pos+2) ) // short
    {
      int r_pos = board_.color_ ? 7 : 63;
      const Field & rfield = board_.getField(r_pos);
      THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
      int r_pos_to = r_pos - 2;
      BitMask rk_mask = board_.g_betweenMasks->between(r_pos_to, oki_pos);
      if ( board_.g_figureDir->dir(Figure::TypeRook, board_.color_, r_pos_to, oki_pos) >= 0 && (rk_mask & all_but_king_mask) == rk_mask )
      {
        add_check(m, ki_pos, ki_pos+2, -1, Figure::TypeNone, discovered);
      }
    }

    if ( board_.castling(board_.color_, 1) && !board_.getField(ki_pos-2) ) // long
    {
      int r_pos = board_.color_ ? 0 : 56;
      const Field & rfield = board_.getField(r_pos);
      THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
      int r_pos_to = r_pos + 3;
      BitMask rk_mask = board_.g_betweenMasks->between(r_pos_to, oki_pos);
      if ( board_.g_figureDir->dir(Figure::TypeRook, board_.color_, r_pos_to, oki_pos) >= 0 && (rk_mask & all_but_king_mask) == rk_mask )
        add_check(m, ki_pos, ki_pos-2, -1, Figure::TypeNone, discovered);
    }
  }

  // 2. Bishop + Rook + Queen
  {
    for (int t = Figure::TypeBishop; t < Figure::TypeKing; ++t)
    {
      Figure::Type type = (Figure::Type)t;
      BitMask fg_mask = board_.fmgr().type_mask(type, color);

      for ( ; fg_mask; )
      {
        int fg_pos = clear_lsb(fg_mask);

        bool discovered = false;
        if ( type != Figure::TypeQueen )
          board_.discoveredCheck(fg_pos, color, mask_all, brq_mask, oki_pos);

        if ( discovered )
        {
          const uint16 * table = board_.g_movesTable->move(type-Figure::TypeBishop, fg_pos);

          for (; *table; ++table)
          {
            const int8 * packed = reinterpret_cast<const int8*>(table);
            int8 count = packed[0];
            int8 delta = packed[1];

            int8 p = fg_pos;
            for ( ; count; --count)
            {
              p += delta;

              const Field & field = board_.getField(p);
              if ( field && (field.color() == color || field.type() >= minimalType_) )
                break;

              add_check(m, fg_pos, p, field ? field.index() : -1, Figure::TypeNone, discovered);

              if ( field )
                break;
            }
          }
        }
        else
        {
          BitMask mask_all_inv_ex = ~(mask_all & ~(1ULL << fg_pos));
          BitMask f_msk = board_.g_movesTable->caps(type, fg_pos) & board_.g_movesTable->caps(type, oki_pos);
          for ( ; f_msk; )
          {
            int to = clear_lsb(f_msk);

            const Field & field = board_.getField(to);
            if ( field && (field.color() == color || field.type() >= minimalType_) )
              continue;

            // can we check from this position?
            if ( board_.is_something_between(to, oki_pos, mask_all_inv_ex) )
              continue;

            // can we go to this position?
            if ( board_.is_something_between(to, fg_pos, mask_all_inv_ex) )
              continue;

            add_check(m, fg_pos, to, field ? field.index() : -1, Figure::TypeNone, discovered);
          }
        }
      }
    }
  }

  // 3. Knight
  {
    BitMask kn_mask = board_.fmgr().knight_mask(color);
    for ( ; kn_mask; )
    {
      int kn_pos = clear_lsb(kn_mask);

      bool discovered = board_.discoveredCheck(kn_pos, color, mask_all, brq_mask, oki_pos);
      if ( discovered )
      {
        const int8 * table = board_.g_movesTable->knight(kn_pos);

        for (; *table >= 0; ++table)
        {
          const Field & field = board_.getField(*table);
          if ( field && (field.color() == color || field.type() >= minimalType_) )
            continue;

          add_check(m, kn_pos, *table, field ? field.index() : -1, Figure::TypeNone, discovered);
        }
      }
      else
      {
        BitMask kn_msk = board_.g_movesTable->caps(Figure::TypeKnight, kn_pos) & knight_check_mask;
        for ( ; kn_msk; )
        {
          int to = clear_lsb(kn_msk);

          const Field & field = board_.getField(to);
          if ( field && (field.color() == color || field.type() >= minimalType_))
            continue;

          add_check(m, kn_pos, to, field ? field.index() : -1, Figure::TypeNone, discovered);
        }
      }
    }
  }

  // 4. Pawn
  {
    // caps
    {
      BitMask pw_check_mask = board_.g_movesTable->pawnCaps_o(ocolor, oki_pos);
      BitMask cap_mask = 0;

      // collect all figures, which we haven't eaten yet
      for (int type = Figure::TypePawn; type < minimalType_; ++type)
        cap_mask |= board_.fmgr().type_mask((Figure::Type)type, ocolor);

      cap_mask &= pw_check_mask;

      BitMask looked_up = 0;
      for ( ; cap_mask; )
      {
        int to = clear_lsb(cap_mask);
        const Field & tfield = board_.getField(to);
        THROW_IF( !tfield || tfield.color() != ocolor || tfield.type() >= minimalType_, "invalid pawns capture in checks generator" );
        BitMask pw_from = board_.g_movesTable->pawnCaps_o(ocolor, to) & board_.fmgr().pawn_mask_o(color);

        // exclude pawns, that we already added
        pw_from &= ~looked_up;
        looked_up |= pw_from;

        for ( ; pw_from; )
        {
          int from = clear_lsb(pw_from);
          add_check(m, from, to, tfield.index(), Figure::TypeNone, false);
        }
      }
    }

    // normal moves
    {
      BitMask inv_mask_all = ~mask_all;
      BitMask move_mask = board_.g_movesTable->pawnCaps_o(ocolor, oki_pos) & inv_mask_all;
      for ( ; move_mask; )
      {
        int to = clear_lsb(move_mask);
        const Field & tfield = board_.getField(to);
        THROW_IF( tfield , "invalid pawns movement in checks generator" );
        BitMask pw_from = board_.g_movesTable->pawnFrom(color, to) & board_.fmgr().pawn_mask_o(color);

        for ( ; pw_from; )
        {
          int from = clear_lsb(pw_from);
          if ( board_.is_something_between(from, to, inv_mask_all) )
            break;

          add_check(m, from, to, -1, Figure::TypeNone, false);
        }
      }
    }

    // promotions
  }
  //{
  //  BitMask pw_mask = board_.fmgr().pawn_mask_o(color);
  //  for ( ; pw_mask; )
  //  {
  //    int pw_pos = clear_lsb(pw_mask);

  //    bool discovered = board_.discoveredCheck(pw_pos, color, mask_all, brq_mask, oki_pos);

  //    const BitMask & from_mask = board_.g_betweenMasks->from(oki_pos, pw_pos);

  //    const int8 * table = board_.g_movesTable->pawn(color, pw_pos);
  //    for (int i = 0; i < 4; ++i, ++table)
  //    {
  //      bool ep_checking = false;
  //      int rindex = -1;
  //      if ( i < 2 )
  //      {
  //        if ( *table < 0 )
  //          continue;

  //        const Field & field = board_.getField(*table);
  //        if ( field && field.color() == ocolor )
  //          rindex = field.index();
  //        else if ( board_.en_passant_ >= 0 )
  //        {
  //          const Figure & rfig = board_.getFigure(ocolor, board_.en_passant_);
  //          int8 to = rfig.where();
  //          static const int8 delta_pos[] = {8, -8};
  //          to += delta_pos[ocolor];
  //          if ( to == *table )
  //          {
  //            rindex = board_.en_passant_;
  //            BitMask pw_msk_to = 1ULL << *table;
  //            BitMask pw_msk_from = 1ULL << pw_pos;
  //            BitMask mask_all_pw = (mask_all ^ pw_msk_from) | pw_msk_to;

  //            ep_checking = board_.discoveredCheck(rfig.where(), color, mask_all_pw, brq_mask, oki_pos);
  //          }
  //        }

  //        if ( rindex < 0 )
  //          continue;

  //        const Figure & rfig = board_.getFigure(ocolor, rindex);

  //        if ( rfig.getType() >= minimalType_ )
  //          continue;
  //      }
  //      else if ( *table < 0 || board_.getField(*table) )
  //        break;

  //      BitMask pw_msk_to = 1ULL << *table;
  //      bool promotion = *table > 55 || *table < 8;

  //      // pawn doesn't cover opponent's king in its new position
  //      bool doesnt_cover = (from_mask & (1ULL << *table)) == 0;

  //      if ( (discovered && doesnt_cover) || ep_checking || (pw_msk_to & pw_check_mask) )
  //        add_check(m, pw_pos, *table, rindex, promotion ? Figure::TypeQueen : Figure::TypeNone, discovered);
  //      // if it's not check, it could be promotion to knight
  //      else if ( promotion )
  //      {
  //        if ( knight_check_mask & pw_msk_to )
  //          add_check(m, pw_pos, *table, rindex, Figure::TypeKnight, discovered);
  //        // may be we haven't generated promotion to checking queen yet
  //        else if ( minimalType_ > Figure::TypeQueen )
  //        {
  //          // could we check from this position?
  //          int dir = board_.g_figureDir->dir(Figure::TypeQueen, color, *table, oki_pos);
  //          if ( dir >= 0 )
  //          {
  //            BitMask mask_all_inv_ex = ~(mask_all & ~(1ULL << pw_pos));
  //            if ( !board_.is_something_between(*table, oki_pos, mask_all_inv_ex) )
  //              add_check(m, pw_pos, *table, rindex, Figure::TypeQueen, discovered);
  //          }
  //        }
  //      }
  //    }
  //  }
  //}

  return m;
}
