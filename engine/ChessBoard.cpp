#include "ChessBoard.h"
#include "Helpers.h"

int Board::StepSorter::history_[64][64];

void Board::StepSorter::clearHistory()
{
  for (int i = 0; i < 64; ++i)
    for (int j = 0; j < 64; ++j)
      history_[i][j] = 0;
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
  case Figure::TypeBishop:
  case Figure::TypeKnight:
  case Figure::TypeRook:
  case Figure::TypeQueen:
    jfrom = PawnIndex;
    jto = KingIndex;
    break;

  case Figure::TypeKing:
    jfrom = KingIndex;
    jto = NumOfFigures;
    break;
  }

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

  return false;
}

void Board::setFigure(const Figure & fig)
{
  getFigure(fig.getColor(), fig.getIndex()) = fig;
  fields_[fig.where()].set(fig);
  fmgr_.incr(fig);
}

bool Board::validatePosition()
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

  if ( !hasSteps() )
  {
    if ( UnderCheck == state_ )
      state_ = ChessMat;
    else
      state_ = Stalemat;
  }

  return true;
}

int Board::findCheckingFigures(Figure::Color color, int pos)
{
  chkNum_ = 0;
  for (int i = 0; i < KingIndex; ++i)
  {
    const Figure & fig = getFigure(color, i);
    if ( !fig )
      continue;

    int dir = FigureDir::dir(fig, pos);
    if ( (dir < 0) || (Figure::TypePawn == fig.getType() && (2 == dir || 3 == dir)) || (Figure::TypeKing == fig.getType() && dir > 7) )
      continue;

    if ( Figure::TypePawn == fig.getType() || Figure::TypeKnight == fig.getType() )
    {
      if ( chkNum_ > 1 )
        return ++chkNum_;

      checking_[chkNum_++] = i;
    }

    FPos dp = getDeltaPos(fig.where(), pos);

    THROW_IF( FPos(0, 0) == dp, "invalid attacked position" );

    FPos p = FPosIndexer::get(pos) + dp;
    const FPos & figp = FPosIndexer::get(fig.where());
    for ( ; p != figp; p += dp)
    {
      const Field & field = getField(p);
      if ( !field )
        continue;

      if ( field.color() != color ||
        Figure::TypeKnight == field.type() || Figure::TypePawn == field.type() || Figure::TypeKing == field.type() )
      {
        break;
      }

      if ( Figure::TypeQueen == field.type() )
      {
        if ( chkNum_ > 1 )
          return ++chkNum_;

        checking_[chkNum_++] = i;
      }

      if ( Figure::TypeBishop == field.type() || Figure::TypeRook == field.type() )
      {
        const Figure & afig = getFigure(color, field.index());
        int dir = FigureDir::dir(afig, pos);
        if ( dir < 0 )
          break;

        if ( chkNum_ > 1 )
          return ++chkNum_;

        checking_[chkNum_++] = i;
      }
    }
  }

  return chkNum_;
}

//////////////////////////////////////////////////////////////////////////
void Board::writeTo(std::ostream & out) const
{
  out << (int)fakeIndex_ << " " << (int)color_ << " " << (int)state_ << " ";
  out << (int)chkNum_ << " " << (int)checking_[0] << " " << (int)checking_[1] << std::endl;

  for (int color = 0; color < 2; ++color)
  {
    int num = 0;
    for (int i = 0; i < NumOfFigures; ++i)
    {
      if ( getFigure((Figure::Color)color, i) )
        num++;
    }

    out << num << std::endl;
    for (int i = 0; i < NumOfFigures; ++i)
    {
      const Figure & fig = getFigure((Figure::Color)color, i);
      if ( !fig )
        continue;

      const FPos & fpos = FPosIndexer::get(fig.where());
      int x = fpos.x();
      int y = fpos.y();
      out << (int)fig.getColor() << " " << (int)fig.getType()<< " " << x << " " << y << " " << (int)fig.isFirstStep() << " " << fig.getIndex() << std::endl;
    }
  }
}

void Board::readFrom(std::istream & in)
{
  int color = 0, state = 0, fakeIndex = 0;
  int chkNum, checking[2];
  in >> fakeIndex >> color >> state >> chkNum >> checking[0] >> checking[1];

  color_ = (Figure::Color)color;
  state_ = (State)state;
  fakeIndex_ = fakeIndex;
  chkNum_ = chkNum;
  checking_[0] = checking[0];
  checking_[1] = checking[1];

  for (int color = 0; color < 2; ++color)
  {
    int num;
    in >> num;
    for (int i = 0; i < num; ++i)
    {
      int c, t, x, y, firstStep, idx;
      in >> c >> t >> x >> y >> firstStep >> idx;
      y++;
      char cx = x + 'a';
      Figure fig((Figure::Type)t, cx, y, (Figure::Color)color, firstStep != 0);
      fig.setIndex(idx);

      setFigure(fig);
    }
  }
}
