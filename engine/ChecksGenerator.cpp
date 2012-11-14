/*************************************************************
  ChecksGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "MovesGenerator.h"
#include "MovesTable.h"
#include <fstream>

//////////////////////////////////////////////////////////////////////////
ChecksGenerator::ChecksGenerator(const Move & hmove, Board & board, Figure::Type minimalType) :
  MovesGeneratorBase(board), minimalType_(minimalType), hmove_(hmove)
{
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();
}

int ChecksGenerator::generate()
{
  int m = 0;

  Figure::Color color  = board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  int oki_pos = board_.kingPos(ocolor);

  BitMask brq_mask = board_.fmgr_.bishop_mask(board_.color_) | board_.fmgr_.rook_mask(board_.color_) | board_.fmgr_.queen_mask(board_.color_);
  const BitMask & knight_check_mask = board_.g_movesTable->caps(Figure::TypeKnight, oki_pos);
  const BitMask & black = board_.fmgr_.mask(Figure::ColorBlack);
  const BitMask & white = board_.fmgr_.mask(Figure::ColorWhite);
  BitMask mask_all = white | black;


  // 1. King
  {
    int ki_pos = board_.kingPos(color);

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

        add(m, ki_pos, *table, Figure::TypeNone, discovered, field);
      }
    }

    // castling also could be with check
    BitMask all_but_king_mask = ~mask_all | (1ULL << ki_pos);
    if ( board_.castling(board_.color_, 0) && !board_.getField(ki_pos+2) ) // short
    {
      static int rook_positions[] = { 63, 7 };
      int & r_pos = rook_positions[board_.color_];
      const Field & rfield = board_.getField(r_pos);
      THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
      int r_pos_to = r_pos - 2;
      BitMask rk_mask = board_.g_betweenMasks->between(r_pos_to, oki_pos);
      if ( board_.g_figureDir->dir(Figure::TypeRook, board_.color_, r_pos_to, oki_pos) >= 0 && (rk_mask & all_but_king_mask) == rk_mask )
      {
        add(m, ki_pos, ki_pos+2, Figure::TypeNone, discovered, false);
      }
    }

    if ( board_.castling(board_.color_, 1) && !board_.getField(ki_pos-2) ) // long
    {
      static int rook_positions[] = { 56, 0 };
      int & r_pos = rook_positions[board_.color_];
      const Field & rfield = board_.getField(r_pos);

      //if ( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_ )
      //{
      //  std::ofstream ofs("D:\\Projects\\git_tests\\temp\\crash.pgn");
      //  Board::save(board_, ofs);
      //}

      THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
      int r_pos_to = r_pos + 3;
      BitMask rk_mask = board_.g_betweenMasks->between(r_pos_to, oki_pos);
      if ( board_.g_figureDir->dir(Figure::TypeRook, board_.color_, r_pos_to, oki_pos) >= 0 && (rk_mask & all_but_king_mask) == rk_mask )
        add(m, ki_pos, ki_pos-2, Figure::TypeNone, discovered, false);
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
          discovered = board_.discoveredCheck(fg_pos, color, mask_all, brq_mask, oki_pos);

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

              add(m, fg_pos, p, Figure::TypeNone, discovered, field);

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

            add(m, fg_pos, to, Figure::TypeNone, discovered, field);
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

          add(m, kn_pos, *table, Figure::TypeNone, discovered, field);
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

          add(m, kn_pos, to, Figure::TypeNone, discovered, field);
        }
      }
    }
  }

  // 4. Pawn
  {
#ifndef NDEBUG
    char fen[256];
    board_.toFEN(fen);
#endif

    int ep_pos = board_.en_passant_;

    // 1st find immediate checks with or without capture
    BitMask pw_check_mask = board_.g_movesTable->pawnCaps_o(ocolor, oki_pos);
    BitMask looked_up = 0;

    for ( ; pw_check_mask; )
    {
      int to = clear_lsb(pw_check_mask);
      const Field & tfield = board_.getField(to);

      // caps
      if ( tfield )
      {
        if ( tfield.color() != ocolor || tfield.type() >= minimalType_ )
          continue;

        BitMask pw_from = board_.g_movesTable->pawnCaps_o(ocolor, to) & board_.fmgr().pawn_mask_o(color);
        BitMask pw_from_save = pw_from;

        for ( ; pw_from; )
        {
          int from = clear_lsb(pw_from);
          bool discovered = board_.discoveredCheck(from, color, mask_all, brq_mask, oki_pos);

          add(m, from, to, Figure::TypeNone, discovered, true);

          // add another moves of this pawn if there is discovered check
          // and we haven't already processed this pawn
          if ( discovered && !(looked_up & (1ULL << from)) )
            add_other_pawn_moves(m, from, to, oki_pos);
        }

        looked_up |= pw_from_save;
        continue;
      }

      // en-passant
      if ( to == ep_pos && minimalType_ > Figure::TypePawn )
      {
        BitMask pw_from = board_.g_movesTable->pawnCaps_o(ocolor, to) & board_.fmgr().pawn_mask_o(color);
        BitMask pw_from_save = pw_from;

        for ( ; pw_from; )
        {
          int from = clear_lsb(pw_from);
          add(m, from, board_.en_passant_, Figure::TypeNone, false, true);

          // other moves of this pawn were already processed
          BitMask pw_msk_from = 1ULL << from;
          if ( pw_msk_from & looked_up )
            continue;

          // add other moves if pawn discovers check
          bool discovered = board_.discoveredCheck(from, color, mask_all, brq_mask, oki_pos);
          if ( discovered )
            add_other_pawn_moves(m, from, to, oki_pos);
        }

        looked_up |= pw_from_save;
        ep_pos = -1;
        continue;
      }

      // normal moves
      {
        BitMask inv_mask_all = ~mask_all;
        BitMask pw_from = board_.g_movesTable->pawnFrom(color, to) & board_.fmgr().pawn_mask_o(color);
        looked_up |= pw_from;

        for ( ; pw_from; )
        {
          int from = clear_lsb(pw_from);
          if ( board_.is_something_between(from, to, inv_mask_all) )
            break;

          add(m, from, to, Figure::TypeNone, false, false);
        }
      }
    }

    // 2nd find promotions
    {
      BitMask promo_mask = board_.g_movesTable->promote_o(color) & board_.fmgr().pawn_mask_o(color);
      looked_up |= promo_mask;

      for ( ; promo_mask; )
      {
        int from = clear_lsb(promo_mask);
        bool discovered = board_.discoveredCheck(from, color, mask_all, brq_mask, oki_pos);

        {
          static const int pw_delta[] = {-8, 8};
          int to = from + pw_delta[color];
          THROW_IF( (unsigned)to > 63, "invalid promotion field" );

          const Field & field_to = board_.getField(to);
          if ( !field_to )
          {
            // usual promotion to queen
            if ( minimalType_ > Figure::TypeQueen )
            {
              if ( discovered )
                add(m, from, to, Figure::TypeQueen, true, false);
              else
              {
                // could we check from this position?
                int dir = board_.g_figureDir->dir(Figure::TypeQueen, color, to, oki_pos);
                if ( dir >= 0 )
                {
                  BitMask mask_all_inv_ex = ~(mask_all & ~(1ULL << from));
                  if ( !board_.is_something_between(to, oki_pos, mask_all_inv_ex) )
                    add(m, from, to, Figure::TypeQueen, false, false);
                }
              }
            }

            // promotion to checking knight without capture
            if ( knight_check_mask & (1ULL << to) )
              add(m, from, to, Figure::TypeKnight, discovered, false);
          }
        }

        // promotion to checking knight with capture
        BitMask pw_mask = board_.g_movesTable->pawnCaps_o(color, from) & board_.fmgr().mask(ocolor);
        for ( ; pw_mask; )
        {
          int to = clear_lsb(pw_mask);
          const Field & field = board_.getField(to);
          THROW_IF( !field || field.color() == color, "couldn't capture on promotion in checks generator" );

          if ( knight_check_mask & (1ULL << to) )
            add(m, from, to, Figure::TypeKnight, discovered, field);
        }
      }
    }

    // 3rd en-passant
    if ( ep_pos >= 0 && minimalType_ > Figure::TypePawn )
    {
      BitMask pw_from = board_.g_movesTable->pawnCaps_o(ocolor, ep_pos) & board_.fmgr().pawn_mask_o(color);
      looked_up |= pw_from;

      for ( ; pw_from; )
      {
        int from = clear_lsb(pw_from);
        bool discovered = board_.discoveredCheck(from, color, mask_all, brq_mask, oki_pos);
        if ( discovered )
        {
          add(m, from, ep_pos, Figure::TypeNone, true, true);
          add_other_pawn_moves(m, from, ep_pos, oki_pos);
        }
        else
        {
          BitMask pw_msk_to = 1ULL << ep_pos;
          BitMask pw_msk_from = 1ULL << from;
          BitMask mask_all_pw = (mask_all ^ pw_msk_from) | pw_msk_to;

          bool ep_checking = board_.discoveredCheck(board_.enpassantPos(), color, mask_all_pw, brq_mask, oki_pos);
          if ( ep_checking )
            add(m, from, ep_pos, Figure::TypeNone, true, true);
        }
      }

      ep_pos = -1;
    }

    // 4th discovered checks
    {
      BitMask disc_mask = board_.g_movesTable->caps(Figure::TypeQueen, oki_pos) & board_.fmgr().pawn_mask_o(color);
      disc_mask &= ~looked_up;

      for ( ; disc_mask; )
      {
        int from = clear_lsb(disc_mask);
        bool discovered = board_.discoveredCheck(from, color, mask_all, brq_mask, oki_pos);
        if ( discovered )
          add_other_pawn_moves(m, from, -1, oki_pos);
      }
    }
  }

  return m;
}
