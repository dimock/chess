#include "MovesGenerator.h"
#include "MovesTable.h"

//////////////////////////////////////////////////////////////////////////

MovesGenerator::MovesGenerator(Board & board) :
  board_(board), current_(-1), numOfMoves_(0)
{
  numOfMoves_ = generate();
  current_ = numOfMoves_ > 0 ? 0 : -1;
  moves_[numOfMoves_].clear();
}

bool MovesGenerator::verify(const Move (&caps)[Board::MovesMax], const Move (&quiets)[Board::MovesMax]) const
{
  if ( board_.checkingNum_ > 1 )
    return true;

  for (int i = 0; i < numOfMoves_; ++i)
  {
    const Move & move = moves_[i];
    bool found = false;
    if ( move.rindex_ >= 0 || move.new_type_ > 0 )
    {
      for (int j = 0; caps[j]; ++j)
      {
        if ( caps[j] == move )
        {
          found = true;
          break;
        }
      }
      if ( !found )
      {
        throw std::runtime_error("capture wasn't found in captures generator");
      }
    }
    if ( found )
      continue;
    for (int j = 0; quiets[j]; ++j)
    {
      if ( quiets[j] == move )
      {
        found = true;
        break;
      }
    }
    if ( !found )
    {
      throw std::runtime_error("move wasn't found in quiets generator");
    }
  }

  for (int i = 0; caps[i]; ++i)
  {
    bool found = false;
    for (int j = 0; j < numOfMoves_; ++j)
    {
      if ( caps[i] == moves_[j] )
      {
        found = true;
        break;
      }
    }
    if ( !found )
    {
      throw std::runtime_error("capture wasn't found in moves");
    }
  }

  for (int i = 0; quiets[i]; ++i)
  {
    bool found = false;
    for (int j = 0; j < numOfMoves_; ++j)
    {
      if ( quiets[i] == moves_[j] )
      {
        found = true;
        break;
      }
    }
    if ( !found )
    {
      throw std::runtime_error("quiet wasn't found in moves");
    }
  }

  return true;
}

int MovesGenerator::generate()
{
  int m = 0;
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);
  for (int n = Board::KingIndex; n >= 0; --n)
  {
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

            Move & move = moves_[m++];

            move.from_ = fig.where();
            move.to_ = p;
            move.rindex_ = rindex;
            move.new_type_ = 0;
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

          Move & move = moves_[m++];

          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = rindex;
          move.new_type_ = 0;
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

          Move & move = moves_[m++];
          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = rfig.getIndex();
          move.new_type_ = 0;

          if ( move.to_ > 55 || move.to_ < 8 ) // 1st || last line
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

        for (; *table >= 0 && !board_.getField(*table); ++table)
        {
          Move & move = moves_[m++];
          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = -1;
          move.new_type_ = 0;

          if ( move.to_ > 55 || move.to_ < 8 ) // 1st || last line
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
      break;
    }

    // only king's movements are available
    if ( board_.checkingNum_ > 1 )
      break;
  }

  return m;
}

//////////////////////////////////////////////////////////////////////////
CapsGenerator::CapsGenerator(Board & board) :
  board_(board), current_(-1), numOfMoves_(0)
{
  numOfMoves_ = generate();
  current_ = numOfMoves_ > 0 ? 0 : -1;
  captures_[numOfMoves_].clear();
}

int CapsGenerator::generate()
{
  int m = 0;

  // generate pawn promotions
  const uint64 & pawn_msk = board_.fmgr_.pawn_mask(board_.color_);
  uint64 promo_msk = board_.g_movesTable->promote_t(board_.color_);
  promo_msk &= pawn_msk;

  static int pw_delta[] = { -8, 8 };

  if ( promo_msk )
  {
    for ( ; promo_msk; )
    {
      int n = least_bit_number(promo_msk);

      THROW_IF( (unsigned)n > 63, "invalid promoted pawn's position" );

      int from = FiguresCounter::s_transposeIndex_[n];
      const Field & field = board_.getField(from);

      THROW_IF( !field || field.color() != board_.color_ || field.type() != Figure::TypePawn, "there is no pawn to promote" );

      int to = from + pw_delta[board_.color_];

      THROW_IF( (unsigned)to > 63, "pawn tries to go to invalid field" );

      if ( board_.getField(to) )
        continue;

      Move & move = captures_[m++];

      move.from_ = from;
      move.to_ = to;
      move.rindex_ = -1;
      move.new_type_ = Figure::TypeQueen;

      captures_[m] = move;
      captures_[m++].new_type_ = Figure::TypeRook;

      captures_[m] = move;
      captures_[m++].new_type_ = Figure::TypeBishop;

      captures_[m] = move;
      captures_[m++].new_type_ = Figure::TypeKnight;
    }
  }

  Figure::Color ocolor = Figure::otherColor(board_.color_);
  uint64 oppenent_mask = board_.fmgr_.mask(ocolor) & ~board_.fmgr_.king_mask(ocolor);
  const uint64 & black = board_.fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr_.mask(Figure::ColorWhite);
  uint64 mask_all_inv = ~(white | black);

  // generate captures
  for (int i = 0; i < Board::NumOfFigures; ++i)
  {
    const Figure & fig = board_.getFigure(board_.color_, i);
    if ( !fig )
      continue;

    if ( fig.getType() == Figure::TypePawn )
    {
      uint64 p_caps = board_.g_movesTable->pawnCaps_o(board_.color_, fig.where()) & oppenent_mask;
      if ( p_caps )
      {
        for ( ; p_caps; )
        {
          int to = least_bit_number(p_caps);

          THROW_IF( (unsigned)to > 63, "invalid pawn's capture position" );

          const Field & field = board_.getField(to);
          if ( !field || field.color() != ocolor )
            continue;

          Move & move = captures_[m++];
          move.from_ = fig.where();
          move.to_ = to;
          move.rindex_ = field.index();
          move.new_type_ = 0;

          if ( move.to_ > 55 || move.to_ < 8 ) // 1st || last line
          {
            move.new_type_ = Figure::TypeQueen;

            captures_[m] = move;
            captures_[m++].new_type_ = Figure::TypeRook;

            captures_[m] = move;
            captures_[m++].new_type_ = Figure::TypeBishop;

            captures_[m] = move;
            captures_[m++].new_type_ = Figure::TypeKnight;
          }
        }
      }

      if ( board_.en_passant_ >= 0 )
      {
        const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
        THROW_IF( !epawn, "there is no en passant pawn" );

        int to = epawn.where() + pw_delta[board_.color_];
        THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

        int dir = board_.g_figureDir->dir(fig, to);
        if ( 0 == dir || 1 == dir )
        {
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

        Move & move = captures_[m++];
        move.from_ = fig.where();
        move.to_ = to;
        move.rindex_ = field.index();
        move.new_type_ = 0;
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

        const uint64 & btw_msk = board_.g_betweenMasks->between(fig.where(), to);
        if ( (btw_msk & mask_all_inv) != btw_msk )
          continue;

        Move & move = captures_[m++];
        move.from_ = fig.where();
        move.to_ = to;
        move.rindex_ = field.index();
        move.new_type_ = 0;
      }
    }
  }

  return m;
}

//////////////////////////////////////////////////////////////////////////
QuietGenerator::QuietGenerator(Board & board) :
  board_(board), current_(-1), numOfMoves_(0)
{
  numOfMoves_ = generate();
  current_ = numOfMoves_ > 0 ? 0 : -1;
  quiets_[numOfMoves_].clear();
}

int QuietGenerator::generate()
{
  int m = 0;
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);
  for (int n = Board::KingIndex; n >= 0; --n)
  {
    const Figure & fig = board_.getFigure(color, n);

    switch ( fig.getType() )
    {
    case Figure::TypeKing:
      {
        const int8 * table = board_.g_movesTable->king(fig.where());

        for (; *table >= 0; ++table)
        {
          const Field & field = board_.getField(*table);
          if ( field )
            continue;

          Move & move = quiets_[m++];

          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = -1;
          move.new_type_ = 0;
        }

        // castling
        if ( fig.isFirstStep() && board_.state_ != Board::UnderCheck )
        {
          {
            Move & move = quiets_[m++];

            move.from_ = fig.where();
            move.to_ = fig.where() + 2;
            move.new_type_ = 0;
            move.rindex_ = -1;
          }

          {
            Move & move = quiets_[m++];

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
          for ( ; count; --count)
          {
            p += delta;

            const Field & field = board_.getField(p);
            if ( field )
              break;

            Move & move = quiets_[m++];

            move.from_ = fig.where();
            move.to_ = p;
            move.rindex_ = -1;
            move.new_type_ = 0;
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
          if ( field )
            continue;

          Move & move = quiets_[m++];

          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = -1;
          move.new_type_ = 0;
        }
      }
      break;

    case Figure::TypePawn:
      {
        // skip captures/promotions - add 2 to *table
        uint64 p_msk = 1ULL << FiguresCounter::s_transposeIndex_[fig.where()];
        if ( !(p_msk & board_.g_movesTable->promote_t(board_.color_)) )
        {
          const int8 * table = board_.g_movesTable->pawn(color, fig.where()) + 2;
          for (; *table >= 0 && !board_.getField(*table); ++table)
          {
            THROW_IF( *table > 55 || *table < 8, "try to promote pawn while quiet move" );

            Move & move = quiets_[m++];
            move.from_ = fig.where();
            move.to_ = *table;
            move.rindex_ = -1;
            move.new_type_ = 0;
          }
        }
      }
      break;
    }
  }

  return m;
}
