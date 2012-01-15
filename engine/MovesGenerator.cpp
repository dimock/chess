#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

//////////////////////////////////////////////////////////////////////////

History MovesGenerator::history_[64][64];
int History::history_max_;

MovesGenerator::MovesGenerator(Board & board, int depth, int ply, Player * player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), depth_(depth), ply_(ply), player_(player), history_max_(0)
{
#ifdef USE_KILLER
  if ( player_ && player_->contexts_[ply_].killer_ )
    killer_ = player_->contexts_[ply_].killer_;
  else
#endif
    killer_.clear();

  numOfMoves_ = generate(alpha, betta, counter);
  moves_[numOfMoves_].clear();
}

MovesGenerator::MovesGenerator(Board & board) :
  board_(board), current_(0), numOfMoves_(0), ply_(0), depth_(0), player_(0), history_max_(0)
{
  killer_.clear();
  ScoreType alpha = 0, betta = 0;
  int counter = 0;
  numOfMoves_ = generate(alpha, betta, counter);
  moves_[numOfMoves_].clear();
}

void MovesGenerator::clear_history()
{
  for (int i = 0; i < 64; ++i)
    for (int j = 0; j < 64; ++j)
      history_[i][j].clear();
  History::history_max_ = 0;
}

//////////////////////////////////////////////////////////////////////////
bool MovesGenerator::find(const Move & m) const
{
  for (int i = 0; i < numOfMoves_; ++i)
  {
    const Move & move = moves_[i];
    if ( m == move )
      return true;
  }
  return false;
}
//////////////////////////////////////////////////////////////////////////
int MovesGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = 0;
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);

  static int s_findex[2][Board::NumOfFigures] = { {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}, {15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0} };
  int v = board_.checkingNum_ > 1 ? 1 : 0;

  for (int j = 0; j < Board::NumOfFigures; ++j)
  {
    int n = s_findex[v][j];

    const Figure & fig = board_.getFigure(color, n);

    switch ( fig.getType() )
    {
    case Figure::TypeKing:
      {
        const int8 * table = board_.g_movesTable->king(fig.where());

        for (; *table >= 0; ++table)
        {
          const Field & field = board_.getField(*table);
          int rindex = -1;
          if ( field )
          {
            if ( field.color() == color )
              continue;
            rindex = field.index();
          }

          add_move(m, fig.where(), *table, rindex, 0);
        }

        if ( fig.isFirstStep() && board_.state_ != Board::UnderCheck )
        {
          add_move(m, fig.where(), fig.where()+2, -1, 0);
          add_move(m, fig.where(), fig.where()-2, -1, 0);
        }
      }
      break;

    case Figure::TypeBishop:
    case Figure::TypeRook:
    case Figure::TypeQueen:
      {
        const uint16 * table = board_.g_movesTable->move(fig.getType()-Figure::TypeBishop, fig.where());

        for (; *table; ++table)
        {
          const int8 * packed = reinterpret_cast<const int8*>(table);
          int8 count = packed[0];
          int8 delta = packed[1];

          int8 p = fig.where();
          int rindex = -1;
          for ( ; count && rindex < 0; --count)
          {
            p += delta;

            const Field & field = board_.getField(p);
            if ( field )
            {
              if ( field.color() == color )
                break;

              rindex = field.index();
            }

            add_move(m, fig.where(), p, rindex, 0);
          }
        }
      }
      break;

    case Figure::TypeKnight:
      {
        const int8 * table = board_.g_movesTable->knight(fig.where());

        for (; *table >= 0; ++table)
        {
          const Field & field = board_.getField(*table);
          int rindex = -1;
          if ( field )
          {
            if ( field.color() == color )
              continue;
            rindex = field.index();
          }

          add_move(m, fig.where(), *table, rindex, 0);
        }
      }
      break;

    case Figure::TypePawn:
      {
        const int8 * table = board_.g_movesTable->pawn(color, fig.where());

        for (int i = 0; i < 2; ++i, ++table)
        {
          if ( *table < 0 )
            continue;

          const Field & field = board_.getField(*table);
          int rindex = -1;
          if ( field && field.color() == ocolor )
            rindex = field.index();
          else if ( board_.en_passant_ >= 0 )
          {
            const Figure & rfig = board_.getFigure(ocolor, board_.en_passant_);
            int8 to = rfig.where();
            static const int8 delta_pos[] = {8, -8};
            to += delta_pos[ocolor];
            if ( to == *table )
              rindex = board_.en_passant_;
          }

          if ( rindex < 0 )
            continue;

          const Figure & rfig = board_.getFigure(ocolor, rindex);

          bool promotion = *table > 55 || *table < 8;

          Move & move = moves_[m++];
          move.alreadyDone_ = 0;
          move.set(fig.where(), *table, rfig.getIndex(), 0, 0);
          calculateWeight(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeRook;
            calculateWeight(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeBishop;
            calculateWeight(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeKnight;
            calculateWeight(moves_[m++]);
          }
        }

        for (; *table >= 0 && !board_.getField(*table); ++table)
        {
          bool promotion = *table > 55 || *table < 8;

          Move & move = moves_[m++];

          move.alreadyDone_ = 0;
          move.set(fig.where(), *table, -1, 0, 0);
          calculateWeight(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeRook;
            calculateWeight(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeBishop;
            calculateWeight(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeKnight;
            calculateWeight(moves_[m++]);
          }
        }
      }
      break;
    }

    // only king's movements are available
    if ( board_.checkingNum_ > 1 )
      break;
  }

  return m;
}

//////////////////////////////////////////////////////////////////////////
EscapeGenerator::EscapeGenerator(const Move & pv, Board & board, int depth, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), depth_(depth), ply_(ply), player_(player), pv_(pv)
{
  numOfMoves_ = push_pv();
  numOfMoves_ = generate(alpha, betta, counter);
  escapes_[numOfMoves_].clear();
}

EscapeGenerator::EscapeGenerator(Board & board, int depth, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), depth_(depth), ply_(ply), player_(player)
{
  pv_.clear();

  numOfMoves_ = generate(alpha, betta, counter);
  escapes_[numOfMoves_].clear();
}

int EscapeGenerator::push_pv()
{
  int m = 0;
  if ( pv_ && board_.validMove(pv_) && board_.isMoveValidUnderCheck(pv_) )
  {
    Move & move = escapes_[m];
    move = pv_;
    move.checkVerified_ = 1;
    move.alreadyDone_ = 0;
    ++m;
  }
  else
    pv_.clear();

  return m;
}

int EscapeGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  if ( board_.checkingNum_ == 1 )
    return generateUsual(alpha, betta, counter);
  else
    return generateKingonly(numOfMoves_, alpha, betta, counter);
}

int EscapeGenerator::generateUsual(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = numOfMoves_;
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);

  THROW_IF( board_.checking_[0] < 0, "there is no checking figure index" );

  const Figure & cfig = board_.getFigure(ocolor, board_.checking_[0]);

  THROW_IF( !cfig, "there is no checking figure" );
  THROW_IF( cfig.getType() == Figure::TypeKing, "king is attacking king" );

  const uint64 & black = board_.fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr_.mask(Figure::ColorWhite);
  const uint64 & pawn_msk = board_.fmgr_.pawn_mask_o(board_.color_);
  uint64 mask_all_inv = ~(white | black);

  static int pw_delta[] = { -8, 8 };

  // check if there is at least 1 pawns capture
  bool pawns_eat = false;
  bool ep_capture = false;

  {
    uint64 pawn_eat_msk = 0;
    if ( board_.color_ )
      pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
    else
      pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);

    // mask of attacking figure
    uint64 checking_fig_mask = 1ULL << cfig.where();

    pawns_eat = (pawn_eat_msk & checking_fig_mask) != 0;

    if ( cfig.getType() == Figure::TypePawn && board_.en_passant_ == board_.checking_[0] )
    {
      const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
      THROW_IF( !epawn, "there is no en passant pawn" );

      int to = epawn.where() + pw_delta[board_.color_];
      THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

      ep_capture = (pawn_eat_msk & (1ULL << to)) != 0;
    }

    pawns_eat = pawns_eat || ep_capture;
  }

  // 1st - get pawn's captures
  if ( pawns_eat )
  {
    // en-passant
    if ( ep_capture )
    {
      const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
      THROW_IF( !epawn, "there is no en passant pawn" );

      int8 to = epawn.where() + pw_delta[board_.color_];
      const uint64 & opawn_caps_ep = board_.g_movesTable->pawnCaps_o(ocolor, to);
      uint64 eat_msk_ep = pawn_msk & opawn_caps_ep;

      for ( ; eat_msk_ep; )
      {
        int n = least_bit_number(eat_msk_ep);

        const Field & fpawn = board_.getField(n);
        
        THROW_IF( !fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != board_.color_, "no pawn on field we are going to do capture from" );

        const Figure & pawn = board_.getFigure(board_.color_, fpawn.index());

        THROW_IF( !pawn, "capturing pawn not found" );

        add_escape(m, n, to, epawn.getIndex(), 0);
      }
    }


    const uint64 & opawn_caps = board_.g_movesTable->pawnCaps_o(ocolor, cfig.where());
    uint64 eat_msk = pawn_msk & opawn_caps;

    bool promotion = cfig.where() > 55 || cfig.where() < 8; // 1st || last line

    for ( ; eat_msk; )
    {
      int n = least_bit_number(eat_msk);

      const Field & fpawn = board_.getField(n);

      THROW_IF( !fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != board_.color_, "no pawn on field we are going to do capture from" );

      const Figure & pawn = board_.getFigure(board_.color_, fpawn.index());

      THROW_IF( !pawn, "capturing pawn not found" );

      int m0 = m;
      if ( add_escape(m, n, cfig.where(), cfig.getIndex(), promotion ? Figure::TypeQueen : 0) && promotion )
      {
        Move & move = escapes_[m0];

        escapes_[m] = move;
        escapes_[m++].new_type_ = Figure::TypeRook;

        escapes_[m] = move;
        escapes_[m++].new_type_ = Figure::TypeBishop;

        escapes_[m] = move;
        escapes_[m++].new_type_ = Figure::TypeKnight;
      }
    }
  }

  // 2nd - knight's captures
  {
    const uint64 & knight_caps = board_.g_movesTable->caps(Figure::TypeKnight, cfig.where());
    const uint64 & knight_msk = board_.fmgr_.knight_mask(color);
    uint64 eat_msk = knight_msk & knight_caps;

    for ( ; eat_msk; )
    {
      int n = least_bit_number(eat_msk);

      const Field & fknight = board_.getField(n);

      THROW_IF( !fknight || fknight.type() != Figure::TypeKnight || fknight.color() != board_.color_, "no knight on field we are going to do capture from" );

      const Figure & knight = board_.getFigure(board_.color_, fknight.index());

      THROW_IF( !knight, "capturing knight not found" );

      add_escape(m, n, cfig.where(), cfig.getIndex(), 0);
    }
  }

  // 3rd - bishops, rooks and queens
  {
    const uint64 & queen_caps = board_.g_movesTable->caps(Figure::TypeQueen, cfig.where());
    uint64 brq_msk = board_.fmgr_.bishop_mask(color) | board_.fmgr_.rook_mask(color) | board_.fmgr_.queen_mask(color);
    uint64 eat_msk = brq_msk & queen_caps;

    for ( ; eat_msk; )
    {
      int n = least_bit_number(eat_msk);

      const Field & field = board_.getField(n);

      THROW_IF( !field || field.color() != board_.color_, "no figure on field we are going to do capture from" );

      const Figure & fig = board_.getFigure(board_.color_, field.index());

      THROW_IF( !fig || !(fig.getType() == Figure::TypeBishop || fig.getType() == Figure::TypeRook || fig.getType() == Figure::TypeQueen), "capturing figure not found" );

      // can fig go to cfig's field
      int dir = board_.g_figureDir->dir(fig, cfig.where());
      if ( dir < 0 )
        continue;

      const uint64 & btw_msk = board_.g_betweenMasks->between(fig.where(), cfig.where());
      if ( (btw_msk & mask_all_inv) != btw_msk )
        continue;

      add_escape(m, n, cfig.where(), cfig.getIndex(), 0);
    }
  }

  // now try to protect king - put something between it and checking figure
  const Figure & king = board_.getFigure(color, Board::KingIndex);
  const uint64 & protect_king_msk = board_.g_betweenMasks->between(king.where(), cfig.where());

  if ( Figure::TypePawn != cfig.getType() && Figure::TypeKnight != cfig.getType()&& protect_king_msk )
  {
    for (int n = 0; n < Board::KingIndex; ++n)
    {
      const Figure & fig = board_.getFigure(color, n);
      if ( !fig )
        continue;

      if ( Figure::TypePawn == fig.getType() )
      {
        // +2 - skip captures
        const int8 * table = board_.g_movesTable->pawn(color, fig.where()) + 2;

        for (; *table >= 0 && !board_.getField(*table); ++table)
        {
          if ( !(protect_king_msk & (1ULL << *table)) )
            continue;

          bool promotion = *table > 55 || *table < 8;

          int m0 = m;
          if ( add_escape(m, fig.where(), *table, -1, promotion ? Figure::TypeQueen : 0) && promotion )
          {
            Move & move = escapes_[m0];

            escapes_[m] = move;
            escapes_[m++].new_type_ = Figure::TypeRook;

            escapes_[m] = move;
            escapes_[m++].new_type_ = Figure::TypeBishop;

            escapes_[m] = move;
            escapes_[m++].new_type_ = Figure::TypeKnight;
          }
        }
      }
      else if ( fig.getType() == Figure::TypeKnight )
      {
        const uint64 & knight_msk = board_.g_movesTable->caps(fig.getType(), fig.where());
        uint64 msk_protect = protect_king_msk & knight_msk;
        for ( ; msk_protect; )
        {
          int n = least_bit_number(msk_protect);

          const Field & field = board_.getField(n);

          THROW_IF( field, "there is something between king and checking figure" );

          add_escape(m, fig.where(), n, -1, 0);
        }
      }
      else
      {
        const uint64 & figure_msk = board_.g_movesTable->caps(fig.getType(), fig.where());
        uint64 msk_protect = protect_king_msk & figure_msk;

        for ( ; msk_protect; )
        {
          int n = least_bit_number(msk_protect);

          const Field & field = board_.getField(n);

          THROW_IF( field, "there is something between king and checking figure" );

          THROW_IF( board_.g_figureDir->dir(fig, n) < 0, "figure can't go to required field" );

          const uint64 & btw_msk = board_.g_betweenMasks->between(fig.where(), n);
          if ( (btw_msk & mask_all_inv) != btw_msk )
            continue;

          add_escape(m, fig.where(), n, -1, 0);
        }
      }
    }
  }

  // at the last generate all king's movements

  m = generateKingonly(m, alpha, betta, counter);

  return m;
}

int EscapeGenerator::generateKingonly(int m, ScoreType & alpha, ScoreType betta, int & counter)
{
  Figure::Color & color = board_.color_;

  const Figure & king = board_.getFigure(color, Board::KingIndex);

  const int8 * table = board_.g_movesTable->king(king.where());

  for (; *table >= 0; ++table)
  {
    const Field & field = board_.getField(*table);
    int rindex = -1;
    if ( field )
    {
      if ( field.color() == color )
        continue;

      rindex = field.index();
    }

	add_escape(m, king.where(), *table, rindex, 0);
  }

  return m;
}

//////////////////////////////////////////////////////////////////////////
bool EscapeGenerator::find(const Move & m) const
{
  for (int i = 0; i < numOfMoves_; ++i)
  {
    const Move & move = escapes_[i];
    if ( m == move )
      return true;
  }
  return false;
}

