#include "Board.h"
#include "fpos.h"
#include "FigureDirs.h"

bool Board::isChecking(MoveCmd & move) const
{
  int idx0 = -1;
  move.checkingNum_ = 0;
  Figure::Color ocolor = Figure::otherColor(color_);
  const Figure & fig = getFigure(color_, move.index_);
  if ( move.castle_ )
    idx0 = getAttackedFrom(ocolor, move.rook_to_);
  else
  {
    if ( Figure::TypePawn == fig.getType() || Figure::TypeKnight == fig.getType() )
    {
      const Figure & king = getFigure(ocolor, KingIndex);
      int dir = FigureDir::dir(fig, king.where());
      if ( (Figure::TypePawn == fig.getType() && 0 == dir || 1 == dir) || (Figure::TypeKnight == fig.getType() && dir >= 0) )
        idx0 = move.index_;
    }
    else
      idx0 = getAttackedFrom(ocolor, move.to_);

    THROW_IF( idx0 >= 0 && idx0 != move.index_, "attacked by wrong figure" );
  }

  if ( idx0 >= 0 )
    move.checking_[move.checkingNum_++] = idx0;

  if ( !move.castle_ )
  {
    int idx1 = getAttackedFrom( ocolor, move.from_ );
    if ( idx1 >= 0 && idx1 != idx0 )
      move.checking_[move.checkingNum_++] = idx1;

    if ( move.rindex_ >= 0 && move.rindex_ == move.en_passant_ && Figure::TypePawn == fig.getType() )
    {
      const Figure & fake = getFigure(ocolor, move.rindex_);
      int fakePos = fake.where();
      static int fake_dp[2] = { 8, -8 };
      fakePos += fake_dp[ocolor];

      if ( fakePos == move.to_ )
      {
        int idx2 = getAttackedFrom(ocolor, fake.where());
        if ( idx2 >= 0 && idx2 != idx0 && idx2 != idx1 )
          move.checking_[move.checkingNum_++] = idx2;
      }
    }
  }

  THROW_IF( move.checkingNum_ > 2, "more than 2 figures attacking king" );
  THROW_IF( !move.checkingNum_ && wasMoveValid(move) && isAttacked(color_, getFigure(ocolor, KingIndex).where()), "ckeck wasn't detected" );

  return move.checkingNum_ > 0;
}

int Board::getAttackedFrom(Figure::Color color, int apt) const
{
  const Figure & king = getFigure(color, KingIndex);
  FPos dp = getDeltaPos(apt, king.where());
  if ( FPos(0, 0) == dp )
    return -1;

  FPos p = FPosIndexer::get(king.where()) + dp;
  for ( ; p; p += dp)
  {
    const Field & field = getField(p.index());
    if ( !field )
      continue;

    if ( field.color() == color )
      return -1;

    const Figure & afig = getFigure(field.color(), field.index());
    if ( Figure::TypeBishop != afig.getType() && Figure::TypeRook != afig.getType() && Figure::TypeQueen != afig.getType() )
      return -1;

    int dir = FigureDir::dir(afig, king.where());
    return dir >= 0 ? afig.getIndex() : -1;
  }
  return -1;
}


bool Board::wasValidUnderCheck(const MoveCmd & move) const
{
  const Figure & fig = getFigure(color_, move.index_);
  Figure::Color ocolor = Figure::otherColor(color_);
  if ( Figure::TypeKing == fig.getType() )
    return !move.castle_ && !isAttacked(ocolor, fig.where());

  if ( 1 == checkingNum_ )
  {
    THROW_IF( checking_[0] < 0 || checking_[0] >= NumOfFigures, "invalid checking figure index" );

    const Figure & king = getFigure(color_, KingIndex);

    if ( move.rindex_ == checking_[0] )
      return getAttackedFrom(color_, move.from_) < 0;

    const Figure & afig = getFigure(ocolor, checking_[0]);
    THROW_IF( Figure::TypeKing == afig.getType(), "king is attacking king" );
    THROW_IF( !afig, "king is attacked by non existing figure" );

    if ( Figure::TypeKnight == afig.getType() || Figure::TypePawn == afig.getType() )
      return false;

    FPos dp1 = getDeltaPos(move.to_, afig.where());
    FPos dp2 = getDeltaPos(king.where(), move.to_);

    // can protect king. now check if we can move this figure
    if ( FPos(0, 0) != dp1 && dp1 == dp2 )
      return getAttackedFrom(color_, move.from_) < 0;

    return false;
  }

  THROW_IF(2 != checkingNum_, "invalid number of checking figures");
  return false;
}

bool Board::wasValidWithoutCheck(const MoveCmd & move) const
{
  const Figure & fig = getFigure(color_, move.index_);
  Figure::Color ocolor = Figure::otherColor(color_);
  if ( Figure::TypeKing == fig.getType() )
    return !isAttacked(ocolor, fig.where());

  return getAttackedFrom(color_, move.from_) < 0;
}


/// is field 'pos' attacked by given color?
bool Board::isAttacked(const Figure::Color c, int pos) const
{
  for (int i = KingIndex; i >= 0; --i)
  {
    const Figure & fig = getFigure(c, i);
    if ( !fig )
      continue;

    int dir = FigureDir::dir(fig, pos);
    if ( (dir < 0) || (Figure::TypePawn == fig.getType() && (2 == dir || 3 == dir)) || (Figure::TypeKing == fig.getType() && dir > 7) )
      continue;

    if ( Figure::TypePawn == fig.getType() || Figure::TypeKnight == fig.getType() || Figure::TypeKing == fig.getType() )
      return true;

    FPos dp = getDeltaPos(fig.where(), pos);

    THROW_IF( FPos(0, 0) == dp, "invalid attacked position" );

    FPos p = FPosIndexer::get(pos) + dp;
    const FPos & figp = FPosIndexer::get(fig.where());
    bool have_fig = false;
    for ( ; p != figp; p += dp)
    {
      const Field & field = getField(p.index());
      if ( !field )
        continue;

      if ( field.color() == c && Figure::TypeQueen == field.type() )
      {
        return true;
      }

      if ( field.color() == c && (Figure::TypeBishop == field.type() || Figure::TypeRook == field.type()) )
      {
        const Figure & afig = getFigure(c, field.index());
        int dir = FigureDir::dir(afig, pos);
        if ( dir >= 0 )
          return true;

        have_fig = true;
        break;
      }

      if ( (Figure::TypeKnight == field.type()) || (field.color() != c && Figure::TypeKing != field.type()) || (field.color() == c && Figure::TypeKing == field.type()) )
      {
        have_fig = true;
        break;
      }

      if ( Figure::TypePawn == field.type() )
      {
        const Figure & pawn = getFigure(field.color(), field.index());
        int d = FigureDir::dir(pawn, pos);
        if ( 0 == d || 1 == d )
          return true;

        have_fig = true;
        break;
      }
    }

    if ( !have_fig )
      return true;
  }

  return false;
}
