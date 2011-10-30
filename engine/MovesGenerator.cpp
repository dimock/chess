#include "Board.h"
#include "MovesTable.h"

int Board::generateMoves(Move (&moves)[MovesMax])
{
  int m = 0;
  Figure::Color ocolor = Figure::otherColor(color_);
  for (int i = 0; i < NumOfFigures; ++i)
  {
    const Figure & fig = getFigure(color_, i);
    switch ( fig.getType() )
    {
    case Figure::TypePawn:
      {
        int8 * table = MovesTable::pawn(color_, fig.where());

        for (int i = 0; i < 2; ++i)
        {
          if ( table[i] < 0 )
            continue;

          const Field & field = getField(table[i]);
          int rindex = -1;
          if ( field && field.color() == ocolor )
            rindex = field.index();
          else if ( en_passant_ >= 0 )
          {
            const Figure & rfig = getFigure(ocolor, en_passant_);
            int8 to = rfig.where();
            static const int8 delta_pos[] = {-8, +8};
            to += delta_pos[ocolor];
            if ( to == table[i] )
              rindex = en_passant_;
          }

          if ( rindex < 0 )
            continue;

          const Figure & rfig = getFigure(ocolor, rindex);

          Move & move = moves[m++];
          move.from_ = fig.where();
          move.to_ = table[i];
          move.rindex_ = rfig.getIndex();

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

        for (int i = 2; i < 4; ++i)
        {
          if ( table[i] < 0 || getField(table[i]) )
            break;

          Move & move = moves[m++];
          move.from_ = fig.where();
          move.to_ = table[i];
          move.rindex_ = -1;

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
  }

  return m;
}