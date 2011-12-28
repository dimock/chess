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
            if ( do_check(alpha, betta, fig.where(), *table, Figure::TypeNone, counter) )
              return m;
#else
            add_check(m, fig.where(), *table, Figure::TypeNone);
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
            add_check(m, fig.where(), fig.where()+2, Figure::TypeNone);
        }

        if ( board_.castling(board_.color_, 1) && !board_.getField(fig.where()-2) ) // long
        {
          const Field & rfield = board_.getField(board_.color_ ? 0 : 56);
          THROW_IF( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
          Figure rook = board_.getFigure(board_.color_, rfield.index());
          rook.go(rook.where()+3);
          uint64 rk_mask = board_.g_betweenMasks->between(rook.where(), oking.where());
          if ( board_.g_figureDir->dir(rook, oking.where()) >= 0 && (rk_mask & ~mask_all) == rk_mask )
            add_check(m, fig.where(), fig.where()-2, Figure::TypeNone);
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
              if ( do_check(alpha, betta, fig.where(), p, Figure::TypeNone, counter) )
                return m;
#else
              add_check(m, fig.where(), p, Figure::TypeNone);
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
            if ( do_check(alpha, betta, fig.where(), to, Figure::TypeNone, counter) )
              return m;
#else
            add_check(m, fig.where(), to, Figure::TypeNone);
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
            if ( do_check(alpha, betta, fig.where(), *table, Figure::TypeNone, counter) )
              return m;
#else
			      add_check(m, fig.where(), *table, Figure::TypeNone);
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
            if ( do_check(alpha, betta, fig.where(), to, Figure::TypeNone, counter) )
              return m;
#else
            add_check(m, fig.where(), to, Figure::TypeNone);
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
          bool promotion = *table < 8 || *table > 55;

          if ( !checking )
          {
            // if it's not check, it could be promotion to knight
            if ( !(promotion && (knight_check_mask & pw_msk_to)) )
              continue;

#ifdef DO_CHECK_IMMEDIATELY
            if ( do_check(alpha, betta, fig.where(), *table, Figure::TypeKnight, counter) )
              return m;
#else
            add_check(m, fig.where(), *table, Figure::TypeKnight);
#endif
          }

#ifdef DO_CHECK_IMMEDIATELY
          if ( do_check(alpha, betta, fig.where(), *table, promotion ? Figure::TypeQueen : Figure::TypeNone, counter) )
            return m;
#else
          add_check(m, fig.where(), *table, promotion ? Figure::TypeQueen : Figure::TypeNone);
#endif
        }
      }
      break;
    }
  }

  return m;
}

bool ChecksGenerator::do_check(ScoreType & alpha, ScoreType betta, int8 from, int8 to, Figure::Type new_type, int & counter)
{
  Move move;
  move.set(from, to, -1, new_type, 0);

#ifdef USE_KILLER
  const Move & killer = player_.contexts_[ply_].killer_;
  if ( move == killer )
    return false;
#endif

  player_.capture(ply_, alpha, betta, move, counter);
  return alpha >= betta;
}

//////////////////////////////////////////////////////////////////////////
void CapsChecksGenerator::calculateWeight(Move & move)
{
  const Field & ffield = player_.board_.getField(move.from_);
  THROW_IF( !ffield, "no figure on field we move from" );
  if ( move.rindex_ >= 0 )
  {
    const Figure & rfig = player_.board_.getFigure(Figure::otherColor(player_.board_.color_), move.rindex_);
    move.score_ = Figure::figureWeight_[rfig.getType()] - Figure::figureWeight_[ffield.type()] + rfig.getType() + 10000;
  }
  else if ( move.new_type_ > 0 )
  {
    move.score_ = Figure::figureWeight_[move.new_type_] - Figure::figureWeight_[Figure::TypePawn] + 10000;
  }
}

//////////////////////////////////////////////////////////////////////////
CapsChecksGenerator::CapsChecksGenerator(Board & board, Figure::Type minimalType, int depth, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), minimalType_(minimalType), depth_(depth), player_(player), ply_(ply)
{
  numOfMoves_ = generate(alpha, betta, counter);
  captures_[numOfMoves_].clear();
}

int CapsChecksGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = 0;

  Figure::Color ocolor = Figure::otherColor(board_.color_);
  uint64 oppenent_mask = board_.fmgr_.mask(ocolor) & ~board_.fmgr_.king_mask(ocolor);
  const uint64 & black = board_.fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr_.mask(Figure::ColorWhite);
  uint64 mask_all = white | black;
  uint64 mask_all_inv = ~mask_all;
  uint64 brq_mask = board_.fmgr_.bishop_mask(board_.color_) | board_.fmgr_.rook_mask(board_.color_) | board_.fmgr_.queen_mask(board_.color_);

  // generate pawn promotions (only if < 2 checking figures)
  const uint64 & pawn_msk = board_.fmgr_.pawn_mask_o(board_.color_);
  static int pw_delta[] = { -8, 8 };

  const Figure & oking = board_.getFigure(ocolor, Board::KingIndex);

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

      // promotion doesn't give enough material, but it could be check
      if ( minimalType_ > Figure::TypeQueen )
      {
        // 1 - pawn between attacker (BRQ) and king
        const uint64 & from_msk = board_.g_betweenMasks->from(oking.where(), from);
        if ( !(from_msk & brq_mask) )
        {
          // 2 - new queen checks
          Figure queen = board_.getFigure(board_.color_, field.index());
          queen.go(to);
          queen.setType(Figure::TypeQueen);
          int dir = board_.g_figureDir->dir(queen, oking.where());
          if ( dir < 0 )
            continue;

          const uint64 & btw_msk = board_.g_betweenMasks->between(to, oking.where());
          uint64 mask_all_inv_ex = mask_all_inv | (1ULL << from);
          if ( (btw_msk && mask_all_inv_ex) != btw_msk )
            continue;
        }
      }

#ifdef GO_IMMEDIATELY_CC
      Move move;
#else
      Move & move = captures_[m++];
      move.alreadyDone_ = 0;
#endif

      move.set(from, to, -1, Figure::TypeQueen, 0);

#ifdef GO_IMMEDIATELY_CC
      if ( movement(alpha, betta, move, counter) )
        return m;
#else
      calculateWeight(move);
#endif
    }
  }

  // firstly check if have at least 1 attacking pawn
  bool pawns_eat = false;

  uint64 pawn_eat_msk = 0;
  if ( board_.color_ )
    pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
  else
    pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);

  pawns_eat = (pawn_eat_msk & oppenent_mask) != 0;

  // may be check if en-passant
  bool checkEP = false;
  if ( board_.en_passant_ >= 0 )
  {
    const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
    uint64 ep_mask = board_.g_betweenMasks->from(oking.where(), epawn.where()) & brq_mask;
    uint64 mask_all_inv_ep = mask_all_inv | (1ULL << epawn.where());
    for ( ; !checkEP && ep_mask; )
    {
      int8 n = least_bit_number(ep_mask);
      const Field & field = board_.getField(n);
      THROW_IF( field.color() != board_.color_, "invalid color of figure, checking from EP-position" );
      const Figure & cfig = board_.getFigure(board_.color_, field.index());
      if ( board_.g_figureDir->dir(cfig, oking.where()) < 0 )
        continue;

      const uint64 & btw_msk = board_.g_betweenMasks->between(oking.where(), n);
      if ( (btw_msk & mask_all_inv_ep) != btw_msk )
        continue;

      checkEP = true;
    }
  }

  if ( !pawns_eat && board_.en_passant_ >= 0 && (minimalType_ <= Figure::TypePawn || checkEP) )
  {
    const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
    THROW_IF( !epawn, "there is no en passant pawn" );

    int to = epawn.where() + pw_delta[board_.color_];
    THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

    pawns_eat = (pawn_eat_msk & (1ULL << to)) != 0;
  }

  static int s_findex[2][Board::NumOfFigures] = { {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}, {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0} };
  int v = board_.checkingNum_ > 1 ? 1 : 0;

  int i0 = 0;
  if ( !v && !pawns_eat )
  {
    for ( ;; ++i0)
    {
      const Figure & fig = board_.getFigure(board_.color_, i0);
      if ( fig.getType() > Figure::TypePawn )
        break;
    }
  }

  // if pawn doesn't eat enough material, it could check. so get pawns capture mask
  const uint64 & pw_check_msk = board_.g_movesTable->pawnCaps_o(ocolor, oking.where());

  // generate captures
  for (int i = i0; i < Board::NumOfFigures; ++i)
  {
    int n = s_findex[v][i];
    const Figure & fig = board_.getFigure(board_.color_, n);
    if ( !fig )
      continue;

    if ( fig.getType() == Figure::TypePawn )
    {
      uint64 p_caps = board_.g_movesTable->pawnCaps_o(board_.color_, fig.where()) & oppenent_mask;
      bool checking = p_caps && maybeCheck(fig, mask_all, brq_mask, oking);

      for ( ; p_caps; )
      {
        THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

        int to = least_bit_number(p_caps);

        bool promotion = to > 55 || to < 8; // 1st || last line

        THROW_IF( (unsigned)to > 63, "invalid pawn's capture position" );

        checking = checking || ((pw_check_msk & (1ULL << to)) != 0);

        const Field & field = board_.getField(to);
        if ( !field || field.color() != ocolor || (field.type() < minimalType_ && !promotion && !checking) )
          continue;

#ifdef GO_IMMEDIATELY_CC
        if ( promotion || field.type() > Figure::TypePawn || checking )
        {
          Move move;
          move.set(fig.where(), to, field.index(), promotion ? Figure::TypeQueen : 0, 0);

          if ( movement(alpha, betta, move, counter) )
            return m;
        }
        else
#endif
        {
          add_capture(m, fig.where(), to, field.index(), promotion ? Figure::TypeQueen : 0);
        }

        // may be we forgot to promote to checking knight
        if ( !checking && promotion )
        {
          const uint64 & kn_check_msk = board_.g_movesTable->caps(Figure::TypeKnight, oking.where());
          uint64 pw_msk_kn = (1ULL << to);
          if ( pw_msk_kn & kn_check_msk )
          {
            add_capture(m, fig.where(), to, field.index(), Figure::TypeKnight);
          }
        }
      }

      if ( board_.en_passant_ >= 0 && (minimalType_ <= Figure::TypePawn || checkEP) )
      {
        const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
        THROW_IF( !epawn, "there is no en passant pawn" );

        int to = epawn.where() + pw_delta[board_.color_];
        THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

        int dir = board_.g_figureDir->dir(fig, to);
        if ( 0 == dir || 1 == dir )
        {
          THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

          add_capture(m, fig.where(), to, board_.en_passant_, 0);
        }
      }
    }
    else if ( fig.getType() == Figure::TypeKnight || fig.getType() == Figure::TypeKing )
    {
      const uint64 & kn_check_msk = board_.g_movesTable->caps(Figure::TypeKnight, oking.where());

      // don't need to verify capture possibility by mask
      uint64 f_caps = board_.g_movesTable->caps(fig.getType(), fig.where()) & oppenent_mask;
      bool checking = f_caps && maybeCheck(fig, mask_all, brq_mask, oking);

      for ( ; f_caps; )
      {
        int to = least_bit_number(f_caps);

        THROW_IF( (unsigned)to > 63, "invalid field index while capture" );

        const Field & field = board_.getField(to);

        THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

        checking = checking || (fig.getType() == Figure::TypeKnight && (kn_check_msk & (1ULL << to)));

        if ( field.type() < minimalType_ && !checking )
          continue;

#ifdef GO_IMMEDIATELY_CC
        if ( field.type() > Figure::TypeBishop || checking )
        {
          Move move;
          move.set(fig.where(), to, field.index(), 0, 0);

          if ( movement(alpha, betta, move, counter) )
            return m;
        }
        else
#endif
        {
          add_capture(m, fig.where(), to, field.index(), 0);
        }
      }
    }
    else // other figures
    {
      uint64 f_caps = board_.g_movesTable->caps(fig.getType(), fig.where()) & oppenent_mask;
      bool checking = f_caps && maybeCheck(fig, mask_all, brq_mask, oking);

      for ( ; f_caps; )
      {
        int8 to = least_bit_number(f_caps);

        const Field & field = board_.getField(to);
        THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

        // can't go here
        const uint64 & btw_msk = board_.g_betweenMasks->between(fig.where(), to);
        if ( (btw_msk & mask_all_inv) != btw_msk )
          continue;

        // if there is not enough material, it could be check
        if ( !checking )
        {
          Figure cfig = fig;
          cfig.go(to);
          int dir = board_.g_figureDir->dir(cfig, oking.where());
          const uint64 & btw_msk_king = board_.g_betweenMasks->between(to, oking.where());
          checking = dir >= 0 && (btw_msk_king & mask_all_inv) == btw_msk_king;
        }

        if ( field.type() < minimalType_ && !checking )
          continue;

#ifdef GO_IMMEDIATELY_CC
        if ( checking || field.type() > fig.getType() )
        {
          Move move;
          move.set(fig.where(), to, field.index(), 0, 0);

          if ( movement(alpha, betta, move, counter) )
            return m;
        }
        else
#endif
        {
          add_capture(m, fig.where(), to, field.index(), 0);
        }
      }
    }

    // only king's movements are available
    if ( board_.checkingNum_ > 1 )
      break;
  }

  return m;
}

bool CapsChecksGenerator::movement(ScoreType & alpha, ScoreType betta, const Move & move, int & counter)
{
#ifdef USE_KILLER
  const Move & killer = player_.contexts_[ply_].killer_;
  if ( move == killer )
    return false;
#endif

  player_.movement(depth_, ply_, alpha, betta, move, counter);
  return alpha >= betta;
}
