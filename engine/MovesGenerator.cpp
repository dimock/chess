#include "Board.h"
#include "MovesTable.h"

int Board::generateMoves(MoveCmd (&moves)[MovesMax])
{
  int m = 0;
  Figure::Color ocolor = Figure::otherColor(color_);
  for (int n = KingIndex; n >= 0; --n)
  {
    const Figure & fig = getFigure(color_, n);

    switch ( fig.getType() )
    {
    case Figure::TypeKing:
      {
        int8 * table = MovesTable::king(fig.where());

        for (; *table >= 0; ++table)
        {
          const Field & field = getField(*table);
          int rindex = -1;
          if ( field )
          {
            if ( field.color() == color_ )
              continue;
            rindex = field.index();
          }

          MoveCmd & move = moves[m++];

          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = rindex;
          move.new_type_ = 0;
        }

        if ( fig.isFirstStep() && state_ != UnderCheck )
        {
          {
            const Field & kfield = getField(fig.where()+3);
            if ( kfield.type() == Figure::TypeRook && kfield.color() == color_ )
            {
              const Figure & krook = getFigure(color_, kfield.index());
              if ( krook.isFirstStep() )
              {
                MoveCmd & move = moves[m++];

                move.from_ = fig.where();
                move.to_ = fig.where() + 2;
                move.new_type_ = 0;
                move.rindex_ = -1;
              }
            }
          }

          {
            const Field & qfield = getField(fig.where()-4);
            if ( qfield.type() == Figure::TypeRook && qfield.color() == color_ )
            {
              const Figure & qrook = getFigure(color_, qfield.index());
              if ( qrook.isFirstStep() )
              {
                MoveCmd & move = moves[m++];

                move.from_ = fig.where();
                move.to_ = fig.where() - 2;
                move.new_type_ = 0;
                move.rindex_ = -1;
              }
            }
          }
        }
      }
      break;

    case Figure::TypeBishop:
    case Figure::TypeRook:
    case Figure::TypeQueen:
      {
        uint16 * table = MovesTable::move(fig.getType()-Figure::TypeBishop, fig.where());

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

            const Field & field = getField(p);
            if ( field )
            {
              if ( field.color() == color_ )
                break;

              rindex = field.index();
            }

            MoveCmd & move = moves[m++];

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
        int8 * table = MovesTable::knight(fig.where());

        for (; *table >= 0; ++table)
        {
          const Field & field = getField(*table);
          int rindex = -1;
          if ( field )
          {
            if ( field.color() == color_ )
              continue;
            rindex = field.index();
          }

          MoveCmd & move = moves[m++];

          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = rindex;
          move.new_type_ = 0;
        }
      }
      break;

    case Figure::TypePawn:
      {
        int8 * table = MovesTable::pawn(color_, fig.where());

        for (int i = 0; i < 2; ++i, ++table)
        {
          if ( *table < 0 )
            continue;

          const Field & field = getField(*table);
          int rindex = -1;
          if ( field && field.color() == ocolor )
            rindex = field.index();
          else if ( en_passant_ >= 0 )
          {
            const Figure & rfig = getFigure(ocolor, en_passant_);
            int8 to = rfig.where();
            static const int8 delta_pos[] = {8, -8};
            to += delta_pos[ocolor];
            if ( to == *table )
              rindex = en_passant_;
          }

          if ( rindex < 0 )
            continue;

          const Figure & rfig = getFigure(ocolor, rindex);

          MoveCmd & move = moves[m++];
          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = rfig.getIndex();
          move.new_type_ = 0;

          if ( move.to_ > 55 || move.to_ < 8 ) // 1st || last line
          {
            move.new_type_ = Figure::TypeQueen;

            moves[m] = move;
            moves[m++].new_type_ = Figure::TypeRook;

            moves[m] = move;
            moves[m++].new_type_ = Figure::TypeBishop;

            moves[m] = move;
            moves[m++].new_type_ = Figure::TypeKnight;
          }
        }

        for (; *table >= 0 && !getField(*table); ++table)
        {
          MoveCmd & move = moves[m++];
          move.from_ = fig.where();
          move.to_ = *table;
          move.rindex_ = -1;
          move.new_type_ = 0;

          if ( move.to_ > 55 || move.to_ < 8 ) // 1st || last line
          {
            move.new_type_ = Figure::TypeQueen;

            moves[m] = move;
            moves[m++].new_type_ = Figure::TypeRook;

            moves[m] = move;
            moves[m++].new_type_ = Figure::TypeBishop;

            moves[m] = move;
            moves[m++].new_type_ = Figure::TypeKnight;
          }
        }
      }
      break;
    }

    // only king's movements are available
    if ( checkingNum_ > 1 )
      break;
  }

  return m;
}