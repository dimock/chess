#include "Board.h"
#include "fpos.h"
#include "FigureDirs.h"

Board::Board()
{
  castle_[0] = castle_[1] = 0;
  checking_[0] = checking_[1] = 0;
  can_win_[0] = can_win_[1] = true;
  checkingNum_ = 0;
  en_passant_ = -1;
  state_ = Invalid;
  color_ = Figure::ColorBlack;
  fiftyMovesCount_ = 0;
  movesCounter_ = 1;
  stages_[0] = stages_[1] = 0;
}

void Board::clear()
{
  fmgr_.clear();

  castle_[0] = castle_[1] = 0;
  checking_[0] = checking_[1] = 0;
  checkingNum_ = 0;
  en_passant_ = -1;
  state_ = Invalid;
  color_ = Figure::ColorBlack;
  fiftyMovesCount_ = 0;
  movesCounter_ = 1;

  for (int i = 0; i < NumOfFields; ++i)
    fields_[i].clear();

  for (int i = 0; i < 2; ++i)
  {
    for (int j = 0; j < NumOfFigures; ++j)
      figures_[i][j].clear();
  }
}

/* rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 */

bool Board::initialize(const char * fen)
{
  clear();

  const char * s = fen;
  int x = 0, y = 7;
  for ( ; s && y >= 0; ++s)
  {
    char c = *s;
    if ( '/' == c )
      continue;
    else if ( c >= '0' && c <= '9')
    {
      int dx = c - '0';
      x += dx;
      if ( x > 7 )
      {
        --y;
        x = 0;
      }
    }
    else
    {
      Figure::Type  ftype = Figure::TypeNone;
      Figure::Color color = Figure::ColorBlack;

      if ( c >= 'A' && c <= 'Z' )
        color = Figure::ColorWhite;

      switch ( c )
      {
      case 'r':
      case 'R':
        ftype = Figure::TypeRook;
        break;

      case 'p':
      case 'P':
        ftype = Figure::TypePawn;
        break;

      case 'n':
      case 'N':
        ftype = Figure::TypeKnight;
        break;

      case 'b':
      case 'B':
        ftype = Figure::TypeBishop;
        break;

      case 'q':
      case 'Q':
        ftype = Figure::TypeQueen;
        break;

      case 'k':
      case 'K':
        ftype = Figure::TypeKing;
        break;
      }

      if ( !ftype )
        return false;

      bool firstStep = false;
      if ( Figure::TypePawn == ftype && (6 == y && Figure::ColorBlack == color || 1 == y && Figure::ColorWhite == color) )
        firstStep = true;

      Figure fig(ftype, color, x, y, firstStep);

      addFigure(fig);

      if ( ++x > 7 )
      {
        --y;
        x = 0;
      }
    }
  }

  int i = 0;
  for ( ; *s; )
  {
    char c = *s;
    if ( c <= ' ' )
    {
      ++s;
      continue;
    }

    if ( 0 == i ) // process move color
    {
      if ( 'w' == c )
        color_ = Figure::ColorWhite;
      else if ( 'b' == c )
        color_ = Figure::ColorBlack;
      else
        return false;
      ++i;
      ++s;
      continue;
    }

    if ( 1 == i ) // process castling possibility
    {
      if ( '-' == c )
      {
        ++i;
        ++s;
        continue;
      }

      for ( ; *s && *s > 32; ++s)
      {
        Field fk, fr;

        switch ( *s )
        {
        case 'k':
          fk = getField(60);
          if ( fk.type() != Figure::TypeKing || fk.color() != Figure::ColorBlack )
            return false;
          fr = getField(63);
          if ( fr.type() != Figure::TypeRook || fr.color() != Figure::ColorBlack )
            return false;
          break;

        case 'K':
          fk = getField(4);
          if ( fk.type() != Figure::TypeKing || fk.color() != Figure::ColorWhite )
            return false;
          fr = getField(7);
          if ( fr.type() != Figure::TypeRook || fr.color() != Figure::ColorWhite )
            return false;
          break;

        case 'q':
          fk = getField(60);
          if ( fk.type() != Figure::TypeKing || fk.color() != Figure::ColorBlack )
            return false;

          fr = getField(56);
          if ( fr.type() != Figure::TypeRook || fr.color() != Figure::ColorBlack )
            return false;
          break;

        case 'Q':
          fk = getField(4);
          if ( fk.type() != Figure::TypeKing || fk.color() != Figure::ColorWhite )
            return false;

          fr = getField(0);
          if ( fr.type() != Figure::TypeRook || fr.color() != Figure::ColorWhite )
            return false;
          break;

        default:
          return false;
        }

        Figure & king = getFigure(fk.color(), fk.index());
        Figure & rook = getFigure(fr.color(), fr.index());

        king.setFirstStep(true);
        rook.setFirstStep(true);
      }

      ++i;
      continue;
    }

    if ( 2 == i )
    {
      ++i;
      char cx = *s++;
      if ( !*s )
        return false;

      if ( '-' == cx ) // no en-passant pawn
        continue;

      char cy = *s++;
      if ( color_ )
        cy--;
      else
        cy++;

      Index pos(cx, cy);

      Field & fp = getField(pos);
      if ( fp.type() != Figure::TypePawn || fp.color() == color_ )
        return false;

      en_passant_ = fp.index();
      continue;
    }

    if ( 3 == i ) // fifty moves rule
    {
      char str[4];
      str[0] = 0;
      for (int j = 0; *s && j < 4; ++j, ++s)
      {
        if ( *s > 32 )
          str[j] = *s;
        else
        {
          str[j] = 0;
          break;
        }
      }

      fiftyMovesCount_ = atoi(str);
      ++i;
      continue;
    }

    if ( 4 == i ) // moves counter
    {
      movesCounter_ = atoi(s);
      break;
    }
  }

  if ( !invalidate() )
    return false;

  return true;
}

//////////////////////////////////////////////////////////////////////////
void Board::zeroMovesFound()
{
  if ( UnderCheck == state_ )
    state_ = ChessMat;
  else if ( !drawState() )
    state_ = Stalemat;
}

//////////////////////////////////////////////////////////////////////////
/// verification of move/unmove methods
bool Board::operator != (const Board & other) const
{
  const char * buf0 = reinterpret_cast<const char*>(this);
  const char * buf1 = reinterpret_cast<const char*>(&other);

  for (int i = 0; i < sizeof(Board); ++i)
  {
    if ( buf0[i] != buf1[i] )
      return true;
  }

  return false;
}

bool Board::invalidate()
{
  Figure::Color ocolor = Figure::otherColor(color_);

  Figure & king1 = getFigure(color_, KingIndex);
  Figure & king2 = getFigure(ocolor, KingIndex);

  state_ = Ok;

  if ( isAttacked(color_, king2.where()) )
  {
    state_ = Invalid;
    return false;
  }

  if ( isAttacked(ocolor, king1.where()) )
    state_ = UnderCheck;

  verifyChessDraw();

  if ( drawState() )
    return true;

  int cnum = findCheckingFigures(ocolor, king1.where());
  if ( cnum > 2 )
  {
    state_ = Invalid;
    return false;
  }
  else if ( cnum > 0 )
  {
    state_ = UnderCheck;
  }

  // setup stages for each color
  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color =  (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
    stages_[color] = 0;
    if ( !( (fmgr_.queens(ocolor) > 0 && fmgr_.rooks(ocolor)+fmgr_.knights(ocolor)+fmgr_.bishops(ocolor) > 0) ||
            (fmgr_.rooks(ocolor) > 1 && fmgr_.bishops(ocolor) + fmgr_.knights(ocolor) > 1) ||
            (fmgr_.rooks(ocolor) > 0 && ((fmgr_.bishops_w(ocolor) > 0 && fmgr_.bishops_b(ocolor) > 0) || (fmgr_.bishops(ocolor) + fmgr_.knights(ocolor) > 2))) ) )
    {
      stages_[color] = 1;
    }
  }


  MoveCmd moves[MovesMax];
  int snum = generateMoves(moves);

  bool found = false;
  for (int i = 0; !found && i < snum; ++i)
  {
    MoveCmd & move = moves[i];
    move.clearUndo();

#ifndef NDEBUG
    Board board0(*this);
#endif

    if ( makeMove(move) )
      found = true;

    unmakeMove(move);

    THROW_IF(board0 != *this, "board is not restored by undo move method");
  }

  if ( !found )
    state_ = UnderCheck == state_ ? ChessMat : Stalemat;

  return true;
}

//////////////////////////////////////////////////////////////////////////

void Board::setFigure(const Figure & fig)
{
  getFigure(fig.getColor(), fig.getIndex()) = fig;
  fields_[fig.where()].set(fig);
  fmgr_.incr(fig);
}

bool Board::addFigure(const Figure & fig)
{
  if ( !fig || fig.where() < 0 )
    return false;

  Field & field = getField(fig.where());
  if ( field )
    return false;

  if ( Figure::TypePawn == fig.getType() )
  {
    int j = fig.where() & 7;
    Figure & f = getFigure(fig.getColor(), j);
    if ( !f )
    {
      f = fig;
      f.setIndex(j);
      field.set(f);
      fmgr_.incr(f);
      return true;
    }
  }

  int jfrom = -1, jto = -1;
  switch ( fig.getType() )
  {
  case Figure::TypePawn:
    jfrom = PawnIndex;
    jto = KnightIndex;
    break;

  case Figure::TypeKnight:
    jfrom = KnightIndex;
    jto = BishopIndex;
    break;

  case Figure::TypeBishop:
    jfrom = BishopIndex;
    jto = RookIndex;
    break;

  case Figure::TypeRook:
    jfrom = RookIndex;
    jto = QueenIndex;
    break;

  case Figure::TypeQueen:
    jfrom = QueenIndex;
    jto = KingIndex;
    break;

  case Figure::TypeKing:
    jfrom = KingIndex;
    jto = NumOfFigures;
    break;
  }

  for (int i = 0; i < 2; ++i)
  {
    for (int j = jfrom; j < jto; ++j)
    {
      Figure & f = getFigure(fig.getColor(), j);
      if ( !f )
      {
        f = fig;
        f.setIndex(j);
        field.set(f);
        fmgr_.incr(f);
        return true;
      }
    }

    // if we haven't found empty slot yet, lets try to put anywhere :)
    jfrom = PawnIndex;
    jto = KingIndex;
  }

  return false;
}
