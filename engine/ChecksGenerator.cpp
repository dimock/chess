#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

//////////////////////////////////////////////////////////////////////////
ChecksGenerator::ChecksGenerator(Board & board, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), player_(player), ply_(ply), numOfMoves_(0)
{
  numOfMoves_ = generate(alpha, betta, counter);
  checks_[numOfMoves_].clear();
}

int ChecksGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = 0;

  Figure::Color color = board_.getColor();
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
    bool checking = maybeCheck(fig, mask_all, brq_mask, oking);

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
            if ( field )
              continue;

#ifdef DO_CHECK_IMMEDIATELY
            if ( do_check(alpha, betta, fig.where(), *table, counter) )
              return m;
#else
            add_check(m, fig.where(), *table);
#endif
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
            add_check(m, fig.where(), fig.where()+2);
        }

        if ( board_.castling(board_.color_, 1) && !board_.getField(fig.where()-2) ) // long
        {
          const Field & rfield = board_.getField(board_.color_ ? 0 : 56);
          THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
          Figure rook = board_.getFigure(board_.color_, rfield.index());
          rook.go(rook.where()+3);
          uint64 rk_mask = board_.g_betweenMasks->between(rook.where(), oking.where());
          if ( board_.g_figureDir->dir(rook, oking.where()) >= 0 && (rk_mask & ~mask_all) == rk_mask )
            add_check(m, fig.where(), fig.where()-2);
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
              if ( field )
                break;

#ifdef DO_CHECK_IMMEDIATELY
              if ( do_check(alpha, betta, fig.where(), p, counter) )
                return m;
#else
              add_check(m, fig.where(), p);
#endif
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
            if ( field )
              continue;

            // can we check from this position?
            const uint64 & btw_msk_k = board_.g_betweenMasks->between(to, oking.where());
            if ( (btw_msk_k & mask_all_inv_ex) != btw_msk_k )
              continue;

            // can we go to this position?
            const uint64 & btw_msk_f = board_.g_betweenMasks->between(to, fig.where());
            if ( (btw_msk_f & mask_all_inv_ex) != btw_msk_f )
              continue;

#ifdef DO_CHECK_IMMEDIATELY
            if ( do_check(alpha, betta, fig.where(), to, counter) )
              return m;
#else
            add_check(m, fig.where(), to);
#endif
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
            if ( field )
              continue;

#ifdef DO_CHECK_IMMEDIATELY
            if ( do_check(alpha, betta, fig.where(), *table, counter) )
              return m;
#else
			      add_check(m, fig.where(), *table);
#endif
          }
        }
        else
        {
          uint64 kn_msk = board_.g_movesTable->caps(Figure::TypeKnight, fig.where()) & knight_check_mask;
          for ( ; kn_msk; )
          {
            int to = least_bit_number(kn_msk);

            const Field & field = board_.getField(to);
            if ( field )
              continue;

#ifdef DO_CHECK_IMMEDIATELY
            if ( do_check(alpha, betta, fig.where(), to, counter) )
              return m;
#else
			      add_check(m, fig.where(), to);
#endif
          }
        }
      }
      break;

    case Figure::TypePawn:
      {
        const int8 * table = board_.g_movesTable->pawn(color, fig.where()) + 2;
        for (; *table >= 0 && !board_.getField(*table); ++table)
        {
          uint64 pw_msk_to = 1ULL << *table;
          checking = checking || (pw_msk_to & pw_check_mask);

          if ( !checking )
          {
            // if it's not check, it could be promotion to knight
            if ( !((*table < 8 || *table > 55) && (knight_check_mask & pw_msk_to)) )
              continue;

#ifdef DO_CHECK_IMMEDIATELY
            if ( do_check(alpha, betta, fig.where(), *table, counter) )
              return m;
#else
            add_check_knight(m, fig.where(), *table);
#endif
          }

#ifdef DO_CHECK_IMMEDIATELY
          if ( do_check(alpha, betta, fig.where(), *table, counter) )
            return m;
#else
          add_check(m, fig.where(), *table);
#endif
        }
      }
      break;
    }
  }

  return m;
}

bool ChecksGenerator::do_check(ScoreType & alpha, ScoreType betta, int8 from, int8 to, int & counter)
{
  Move move;
  move.set(from, to, -1, 0, 0);

#ifdef USE_KILLER
  const Move & killer = player_.contexts_[ply_].killer_;
  if ( move == killer )
    return false;
#endif

  player_.capture(ply_, alpha, betta, move, counter);
  return alpha >= betta;
}
