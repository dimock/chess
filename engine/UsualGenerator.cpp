/*************************************************************
  MovesGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "MovesGenerator.h"
#include "MovesTable.h"

//////////////////////////////////////////////////////////////////////////

UsualGenerator::UsualGenerator(Board & board) :
  MovesGeneratorBase(board)
{
  killer_.clear();
}

int UsualGenerator::generate(const Move & killer)
{
  killer_ = killer;
  numOfMoves_ = 0;

  const Figure::Color & color = board_.color_;
  const Figure::Color ocolor = Figure::otherColor(color);
  const BitMask & omask = board_.fmgr_.mask(ocolor);
  BitMask mask_all_inv = ~(board_.fmgr_.mask(Figure::ColorWhite) | board_.fmgr_.mask(Figure::ColorBlack));

  // pawns movements
  if ( board_.fmgr().pawn_mask_o(color) )
  {
    BitMask pw_mask = board_.fmgr().pawn_mask_o(color);
    for ( ; pw_mask; )
    {
      int pw_pos = clear_lsb(pw_mask);

      int y = Index(pw_pos).y();
      bool promotion = ((y == 1 && !color) || (y == 6 && color));

      BitMask pw_caps = board_.g_movesTable->pawnCaps_o(color, pw_pos) * omask;
      if ( promotion && (pw_caps != 0) )
      {
        for ( ; pw_caps; )
        {
          int to = clear_lsb(pw_caps);
          const Field & tfield = board_.getField(to);
          THROW_IF( !tfield || tfield.color() != ocolor, "UsualGenerator: no figure to capture" );

          // don't promote to knight if it doesn't check
          if ( !(board_.g_movesTable->caps(Figure::TypeKnight, to) & board_.fmgr_.king_mask(ocolor)) )
            continue;

          add(numOfMoves_, pw_pos, to, Figure::TypeKnight, true);
        }
      }

      const int8 * table = board_.g_movesTable->pawn(color, pw_pos) + 2; // skip captures
      for (; *table >= 0 && !board_.getField(*table); ++table)
      {
        // don't promote to knight if it doesn't check
        if ( promotion )
        {
          if ( !(board_.g_movesTable->caps(Figure::TypeKnight, *table) & board_.fmgr_.king_mask(ocolor)) )
            continue;

          Move move;
          move.clear();
          move.clearFlags();
          move.from_ = pw_pos;
          move.to_ = *table;
          move.new_type_ = Figure::TypeKnight;

          if ( board_.see(move) < 0 )
            continue;

          moves_[numOfMoves_++] = move;
        }
        else
          add(numOfMoves_, pw_pos, *table, promotion ? Figure::TypeKnight : Figure::TypeNone, false);
      }
    }
  }

  // knights movements
  if ( board_.fmgr().knight_mask(color) )
  {
    BitMask kn_mask = board_.fmgr().knight_mask(color);
    for ( ; kn_mask; )
    {
      int kn_pos = clear_lsb(kn_mask);
      BitMask kn_caps = board_.g_movesTable->caps(Figure::TypeKnight, kn_pos) & mask_all_inv;

      for ( ; kn_caps; )
      {
        int to = clear_lsb(kn_mask);

        const Field & field = board_.getField(to);
        THROW_IF( field, "try to generate capture" );

        add(numOfMoves_, kn_pos, to, Figure::TypeNone, false);
      }
    }
  }

  // bishops, rooks and queens movements
  for (int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
  {
    BitMask fg_mask = board_.fmgr().type_mask((Figure::Type)type, color);

    for ( ; fg_mask; )
    {
      int fg_pos = clear_lsb(fg_mask);

      const uint16 * table = board_.g_movesTable->move(type-Figure::TypeBishop, fg_pos);

      for (; *table; ++table)
      {
        const int8 * packed = reinterpret_cast<const int8*>(table);
        int8 count = packed[0];
        int8 delta = packed[1];

        int8 p = fg_pos;
        bool capture = false;
        for ( ; count; --count)
        {
          p += delta;

          const Field & field = board_.getField(p);
          if ( field )
            break;

          add(numOfMoves_, fg_pos, p, Figure::TypeNone, false);
        }
      }
    }
  }

  // kings movements
  {
    int ki_pos = board_.kingPos(color);
    BitMask ki_mask = board_.g_movesTable->caps(Figure::TypeKing, ki_pos) & mask_all_inv;

    if ( ki_mask )
    {
      // short castle
      if ( board_.castling(board_.color_, 0) && !board_.getField(ki_pos+2) && !board_.getField(ki_pos+1) )
        add(numOfMoves_, ki_pos, ki_pos+2, Figure::TypeNone, false);

      // long castle
      if ( board_.castling(board_.color_, 1) && !board_.getField(ki_pos-2) && !board_.getField(ki_pos-1) && !board_.getField(ki_pos-3) )
        add(numOfMoves_, ki_pos, ki_pos-2, Figure::TypeNone, false);

      for ( ; ki_mask; )
      {
        int to = clear_lsb(ki_mask);

        const Field & field = board_.getField(to);
        THROW_IF( field, "try to capture by king" );

        add(numOfMoves_, ki_pos, to, Figure::TypeNone, false);
      }
    }
  }

  moves_[numOfMoves_].clear();
  return numOfMoves_;
}
