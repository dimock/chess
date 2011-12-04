#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

//////////////////////////////////////////////////////////////////////////

MovesGenerator::MovesGenerator(Board & board, int depth, int ply, Player * player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), depth_(depth), ply_(ply), player_(player)
{
  numOfMoves_ = generate(alpha, betta, counter);
  moves_[numOfMoves_].clear();
}

MovesGenerator::MovesGenerator(Board & board) :
  board_(board), current_(0), numOfMoves_(0), ply_(0), depth_(0), player_(0)
{
  ScoreType alpha = 0, betta = 0;
  int counter = 0;
  numOfMoves_ = generate(alpha, betta, counter);
  moves_[numOfMoves_].clear();
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

          Move & move = moves_[m++];

          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = rindex;
          move.new_type_ = 0;
        }

        if ( fig.isFirstStep() && board_.state_ != Board::UnderCheck )
        {
          {
            Move & move = moves_[m++];

            move.from_ = fig.where();
            move.to_ = fig.where() + 2;
            move.new_type_ = 0;
            move.rindex_ = -1;
          }

          {
            Move & move = moves_[m++];

            move.from_ = fig.where();
            move.to_ = fig.where() - 2;
            move.new_type_ = 0;
            move.rindex_ = -1;
          }
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

#ifdef GO_IMMEDIATELY
            if ( player_ && rindex >= 0 && field.type() > fig.getType() )
            {
              Move move;

              move.from_ = fig.where();
              move.to_ = p;
              move.rindex_ = rindex;
              move.new_type_ = 0;

              if ( movement(alpha, betta, move, counter) )
                return m;
            }
            else
#endif
            {
              Move & move = moves_[m++];
              move.from_ = fig.where();
              move.to_ = p;
              move.rindex_ = rindex;
              move.new_type_ = 0;
            }
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

#ifdef GO_IMMEDIATELY
          if ( player_ && rindex >= 0 && field.type() > fig.getType() )
          {
            Move move;

            move.from_ = fig.where();
            move.to_ = *table;
            move.rindex_ = rindex;
            move.new_type_ = 0;

            if ( movement(alpha, betta, move, counter) )
              return m;
          }
          else
#endif
          {
            Move & move = moves_[m++];

            move.from_ = fig.where();
            move.to_ = *table;
            move.rindex_ = rindex;
            move.new_type_ = 0;
          }
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

#ifdef GO_IMMEDIATELY
          if ( player_ && rfig.getType() > Figure::TypePawn )
          {
            Move move;

            move.from_ = fig.where();
            move.to_ = *table;
            move.rindex_ = rfig.getIndex();
            move.new_type_ = 0;

            if ( promotion )
              move.new_type_ = Figure::TypeQueen;

            if ( movement(alpha, betta, move, counter) )
              return m;

            if ( promotion )
            {
              moves_[m] = move;
              moves_[m++].new_type_ = Figure::TypeRook;

              moves_[m] = move;
              moves_[m++].new_type_ = Figure::TypeBishop;

              moves_[m] = move;
              moves_[m++].new_type_ = Figure::TypeKnight;
            }
          }
          else
#endif
          {
            Move & move = moves_[m++];
            move.from_ = fig.where();
            move.to_ = *table;
            move.rindex_ = rfig.getIndex();
            move.new_type_ = 0;

            if ( promotion )
            {
              move.new_type_ = Figure::TypeQueen;

              moves_[m] = move;
              moves_[m++].new_type_ = Figure::TypeRook;

              moves_[m] = move;
              moves_[m++].new_type_ = Figure::TypeBishop;

              moves_[m] = move;
              moves_[m++].new_type_ = Figure::TypeKnight;
            }
          }
        }

        for (; *table >= 0 && !board_.getField(*table); ++table)
        {
          bool promotion = *table > 55 || *table < 8;

#ifdef GO_IMMEDIATELY
          if ( player_ && promotion )
          {
            Move move;
            move.from_ = fig.where();
            move.to_ = *table;
            move.rindex_ = -1;
            move.new_type_ = Figure::TypeQueen;

            if ( movement(alpha, betta, move, counter) )
              return m;

            moves_[m] = move;
            moves_[m++].new_type_ = Figure::TypeRook;

            moves_[m] = move;
            moves_[m++].new_type_ = Figure::TypeBishop;

            moves_[m] = move;
            moves_[m++].new_type_ = Figure::TypeKnight;
          }
          else
#endif
          {
            Move & move = moves_[m++];
            move.from_ = fig.where();
            move.to_ = *table;
            move.rindex_ = -1;
            move.new_type_ = 0;

            if ( promotion )
            {
              move.new_type_ = Figure::TypeQueen;

              moves_[m] = move;
              moves_[m++].new_type_ = Figure::TypeRook;

              moves_[m] = move;
              moves_[m++].new_type_ = Figure::TypeBishop;

              moves_[m] = move;
              moves_[m++].new_type_ = Figure::TypeKnight;
            }
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

bool MovesGenerator::movement(ScoreType & alpha, ScoreType betta, const Move & move, int & counter)
{
  THROW_IF( !player_, "no player to make movement" );

  const Move & killer = player_->contexts_[ply_].killer_;
  if ( move == killer )
    return false;
  
  player_->movement(depth_, ply_, alpha, betta, move, counter);
  return alpha >= betta;
}

//////////////////////////////////////////////////////////////////////////
CapsGenerator::CapsGenerator(Board & board, Figure::Type minimalType, int ply, Player & player, ScoreType & alpha, ScoreType betta) :
  board_(board), current_(0), numOfMoves_(0), minimalType_(minimalType), player_(player), ply_(ply)
{
  numOfMoves_ = generate(alpha, betta);
  captures_[numOfMoves_].clear();
}

int CapsGenerator::generate(ScoreType & alpha, ScoreType betta)
{
  int m = 0;

  // generate pawn promotions (only if < 2 checking figures)
  const uint64 & pawn_msk = board_.fmgr_.pawn_mask_o(board_.color_);
  static int pw_delta[] = { -8, 8 };

  if ( board_.checkingNum_ < 2 )
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

      if ( board_.getField(to) || Figure::TypeQueen == minimalType_ )
        continue;

#ifdef GO_IMMEDIATELY
      Move move;
#else
      Move & move = captures_[m++];
#endif

      move.from_ = from;
      move.to_ = to;
      move.rindex_ = -1;
      move.new_type_ = Figure::TypeQueen;

#ifdef GO_IMMEDIATELY
      if ( capture(alpha, betta, move) )
        return m;
#endif
    }
  }

  Figure::Color ocolor = Figure::otherColor(board_.color_);
  uint64 oppenent_mask = board_.fmgr_.mask(ocolor) & ~board_.fmgr_.king_mask(ocolor);
  const uint64 & black = board_.fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr_.mask(Figure::ColorWhite);
  uint64 mask_all_inv = ~(white | black);

  // firstly check do we have at least 1 attacking pawn
  bool pawns_eat = false;

  uint64 pawn_eat_msk = 0;
  if ( board_.color_ )
    pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
  else
    pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);

  pawns_eat = (pawn_eat_msk & oppenent_mask) != 0;

  if ( !pawns_eat && board_.en_passant_ >= 0 && minimalType_ <= Figure::TypePawn )
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
      for ( ; p_caps; )
      {
        THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

        int to = least_bit_number(p_caps);

        bool promotion = to > 55 || to < 8; // 1st || last line

        THROW_IF( (unsigned)to > 63, "invalid pawn's capture position" );

        const Field & field = board_.getField(to);
        if ( !field || field.color() != ocolor || (field.type() < minimalType_ && !promotion) )
          continue;

#ifdef GO_IMMEDIATELY
        if ( promotion || field.type() > Figure::TypePawn )
        {
          Move move;

          move.from_ = fig.where();
          move.to_ = to;
          move.rindex_ = field.index();
          move.new_type_ = promotion ? Figure::TypeQueen : 0;

          if ( capture(alpha, betta, move) )
            return m;
        }
        else
#endif
        {
          Move & move = captures_[m++];
          move.from_ = fig.where();
          move.to_ = to;
          move.rindex_ = field.index();
          move.new_type_ = promotion ? Figure::TypeQueen : 0;
        }
      }

      if ( board_.en_passant_ >= 0 && minimalType_ <= Figure::TypePawn )
      {
        const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
        THROW_IF( !epawn, "there is no en passant pawn" );

        int to = epawn.where() + pw_delta[board_.color_];
        THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

        int dir = board_.g_figureDir->dir(fig, to);
        if ( 0 == dir || 1 == dir )
        {
          THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

          Move & move = captures_[m++];
          move.from_ = fig.where();
          move.to_ = to;
          move.rindex_ = board_.en_passant_;
          move.new_type_ = 0;
        }
      }
    }
    else if ( fig.getType() == Figure::TypeKnight || fig.getType() == Figure::TypeKing )
    {
      // don't need to verify capture possibility by mask
      uint64 f_caps = board_.g_movesTable->caps(fig.getType(), fig.where()) & oppenent_mask;
      for ( ; f_caps; )
      {
        int to = least_bit_number(f_caps);

        THROW_IF( (unsigned)to > 63, "invalid field index while capture" );

        const Field & field = board_.getField(to);

        THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

        if ( field.type() < minimalType_ )
          continue;

#ifdef GO_IMMEDIATELY
        if ( field.type() > Figure::TypeKnight )
        {
          Move move;
          move.from_ = fig.where();
          move.to_ = to;
          move.rindex_ = field.index();
          move.new_type_ = 0;

          if ( capture(alpha, betta, move) )
            return m;
        }
        else
#endif
        {
          Move & move = captures_[m++];
          move.from_ = fig.where();
          move.to_ = to;
          move.rindex_ = field.index();
          move.new_type_ = 0;
        }
      }
    }
    else // other figures
    {
      uint64 f_caps = board_.g_movesTable->caps(fig.getType(), fig.where()) & oppenent_mask;
      for ( ; f_caps; )
      {
        int to = least_bit_number(f_caps);

        THROW_IF( (unsigned)to > 63, "invalid field index while capture" );

        const Field & field = board_.getField(to);

        THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

        if ( field.type() < minimalType_ )
          continue;

        const uint64 & btw_msk = board_.g_betweenMasks->between(fig.where(), to);
        if ( (btw_msk & mask_all_inv) != btw_msk )
          continue;

#ifdef GO_IMMEDIATELY
        if ( field.type() > fig.getType() )
        {
          Move move;
          move.from_ = fig.where();
          move.to_ = to;
          move.rindex_ = field.index();
          move.new_type_ = 0;

          if ( capture(alpha, betta, move) )
            return m;
        }
        else
#endif
        {
          Move & move = captures_[m++];
          move.from_ = fig.where();
          move.to_ = to;
          move.rindex_ = field.index();
          move.new_type_ = 0;
        }
      }
    }

    // only king's movements are available
    if ( board_.checkingNum_ > 1 )
      break;
  }

  return m;
}

bool CapsGenerator::capture(ScoreType & alpha, ScoreType betta, const Move & move)
{
  const Move & killer = player_.contexts_[ply_].killer_;
  if ( move == killer )
    return false;

  player_.capture(ply_, alpha, betta, move);
  return alpha >= betta;
}

//////////////////////////////////////////////////////////////////////////
EscapeGenerator::EscapeGenerator(Board & board, int depth, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), depth_(depth), ply_(ply), player_(player)
{
  numOfMoves_ = generate(alpha, betta, counter);
  escapes_[numOfMoves_].clear();
}

int EscapeGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  if ( board_.checkingNum_ == 1 )
    return generateUsual(alpha, betta, counter);
  else
    return generateKingonly(alpha, betta, counter);
}

int EscapeGenerator::generateUsual(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = 0;
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);
  return m;
}

int EscapeGenerator::generateKingonly(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = 0;
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);
  return m;
}
