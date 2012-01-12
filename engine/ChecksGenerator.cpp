#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

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

  const Figure & oking = board_.getFigure(ocolor, Board::KingIndex);
  uint64 brq_mask = board_.fmgr_.bishop_mask(board_.color_) | board_.fmgr_.rook_mask(board_.color_) | board_.fmgr_.queen_mask(board_.color_);
  const uint64 & pw_check_mask = board_.g_movesTable->pawnCaps_o(ocolor, oking.where());
  const uint64 & knight_check_mask = board_.g_movesTable->caps(Figure::TypeKnight, oking.where());
  const uint64 & black = board_.fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr_.mask(Figure::ColorWhite);
  uint64 mask_all = white | black;

  const uint64 & kn_check_mask = board_.g_movesTable->caps(Figure::TypeKnight, oking.where());
  for (int i = 0; i < Board::NumOfFigures; ++i)
  {
    const Figure & fig = board_.getFigure(board_.color_, i);
    if ( !fig )
      continue;

    // may be figure opens line between attacker and king
    bool checking = maybeCheck(fig.where(), mask_all, brq_mask, oking);

    switch ( fig.getType() )
    {
    case Figure::TypeKing:
      {
        if ( checking )
        {
          const int8 * table = board_.g_movesTable->king(fig.where());

          for (; *table >= 0; ++table)
          {
            const Field & field = board_.getField(*table);
            if ( field && (field.color() == color || field.type() >= minimalType_) )
              continue;

            add_check(m, fig.where(), *table, field ? field.index() : -1, Figure::TypeNone);
          }
        }

        // castling also could be with check
        if ( board_.castling(board_.color_, 0) && !board_.getField(fig.where()+2) ) // short
        {
          const Field & rfield = board_.getField(board_.color_ ? 7 : 63);
          THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
          Figure rook = board_.getFigure(board_.color_, rfield.index());
          rook.go(rook.where()-2);
          uint64 rk_mask = board_.g_betweenMasks->between(rook.where(), oking.where());
          if ( board_.g_figureDir->dir(rook, oking.where()) >= 0 && (rk_mask & ~mask_all) == rk_mask )
            add_check(m, fig.where(), fig.where()+2, -1, Figure::TypeNone);
        }

        if ( board_.castling(board_.color_, 1) && !board_.getField(fig.where()-2) ) // long
        {
          const Field & rfield = board_.getField(board_.color_ ? 0 : 56);
          THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
          Figure rook = board_.getFigure(board_.color_, rfield.index());
          rook.go(rook.where()+3);
          uint64 rk_mask = board_.g_betweenMasks->between(rook.where(), oking.where());
          if ( board_.g_figureDir->dir(rook, oking.where()) >= 0 && (rk_mask & ~mask_all) == rk_mask )
            add_check(m, fig.where(), fig.where()-2, -1, Figure::TypeNone);
        }
      }
      break;

    case Figure::TypeBishop:
    case Figure::TypeRook:
    case Figure::TypeQueen:
      {
        if ( checking )
        {
          const uint16 * table = board_.g_movesTable->move(fig.getType()-Figure::TypeBishop, fig.where());

          for (; *table; ++table)
          {
            const int8 * packed = reinterpret_cast<const int8*>(table);
            int8 count = packed[0];
            int8 delta = packed[1];

            int8 p = fig.where();
            for ( ; count; --count)
            {
              p += delta;

              const Field & field = board_.getField(p);
              if ( field && (field.color() == color || field.type() >= minimalType_) )
                break;

              add_check(m, fig.where(), p, field ? field.index() : -1, Figure::TypeNone);

              if ( field )
                break;
            }
          }
        }
        else
        {
          uint64 mask_all_inv_ex = ~(mask_all & ~(1ULL << fig.where()));
          uint64 f_msk = board_.g_movesTable->caps(fig.getType(), fig.where()) & board_.g_movesTable->caps(fig.getType(), oking.where());
          for ( ; f_msk; )
          {
            int to = least_bit_number(f_msk);

            const Field & field = board_.getField(to);
            if ( field && (field.color() == color || field.type() >= minimalType_) )
              continue;

            // can we check from this position?
            const uint64 & btw_msk_k = board_.g_betweenMasks->between(to, oking.where());
            if ( (btw_msk_k & mask_all_inv_ex) != btw_msk_k )
              continue;

            // can we go to this position?
            const uint64 & btw_msk_f = board_.g_betweenMasks->between(to, fig.where());
            if ( (btw_msk_f & mask_all_inv_ex) != btw_msk_f )
              continue;

            add_check(m, fig.where(), to, field ? field.index() : -1, Figure::TypeNone);
          }
        }
      }
      break;

    case Figure::TypeKnight:
      {
        if ( checking )
        {
          const int8 * table = board_.g_movesTable->knight(fig.where());

          for (; *table >= 0; ++table)
          {
            const Field & field = board_.getField(*table);
            if ( field && (field.color() == color || field.type() >= minimalType_) )
              continue;

            add_check(m, fig.where(), *table, field ? field.index() : -1, Figure::TypeNone);
          }
        }
        else
        {
          uint64 kn_msk = board_.g_movesTable->caps(Figure::TypeKnight, fig.where()) & knight_check_mask;
          for ( ; kn_msk; )
          {
            int to = least_bit_number(kn_msk);

            const Field & field = board_.getField(to);
            if ( field && (field.color() == color || field.type() >= minimalType_))
              continue;

            add_check(m, fig.where(), to, field ? field.index() : -1, Figure::TypeNone);
          }
        }
      }
      break;

    case Figure::TypePawn:
      {
        const int8 * table = board_.g_movesTable->pawn(color, fig.where());
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
                uint64 pw_msk_to = 1ULL << *table;
                uint64 pw_msk_from = 1ULL << fig.where();
                uint64 mask_all_pw = (mask_all ^ pw_msk_from) | pw_msk_to;
                ep_checking = maybeCheck(rfig.where(), mask_all_pw, brq_mask, oking);
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

          uint64 pw_msk_to = 1ULL << *table;
          bool promotion = *table > 55 || *table < 8;

          if ( checking  || ep_checking|| (pw_msk_to & pw_check_mask) )
            add_check(m, fig.where(), *table, rindex, promotion ? Figure::TypeQueen : Figure::TypeNone);
          // if it's not check, it could be promotion to knight
          else if ( promotion )
          {
            if ( knight_check_mask & pw_msk_to )
              add_check(m, fig.where(), *table, rindex, Figure::TypeKnight);
            // may be we haven't generated promotion to checking queen yet
            else if ( minimalType_ > Figure::TypeQueen )
            {
              // can we check from this position?
              uint64 mask_all_inv_ex = ~(mask_all & ~(1ULL << fig.where()));
              const uint64 & btw_msk = board_.g_betweenMasks->between(*table, oking.where());
              if ( (btw_msk & mask_all_inv_ex) == btw_msk )
                add_check(m, fig.where(), *table, rindex, Figure::TypeQueen);
            }
          }
        }
      }
      break;
    }
  }

  return m;
}
//////////////////////////////////////////////////////////////////////////
