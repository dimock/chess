#include "MovesGenerator.h"

MovesGenerator::MovesGenerator(Board & board) :
  board_(board), current_(-1), numOfMoves_(0)
{
  numOfMoves_ = generateMoves();
  current_ = numOfMoves_ > 0 ? 0 : -1;
  moves_[numOfMoves_].clear();
}

int MovesGenerator::generateMoves()
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
        int8 * table = board_.g_movesTable->king(fig.where());

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
        uint16 * table = board_.g_movesTable->move(fig.getType()-Figure::TypeBishop, fig.where());

        for (; *table; ++table)
        {
          int8 * packed = reinterpret_cast<int8*>(table);
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
        int8 * table = board_.g_movesTable->knight(fig.where());

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
        int8 * table = board_.g_movesTable->pawn(color, fig.where());

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