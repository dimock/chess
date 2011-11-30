#include "Board.h"
#include "fpos.h"
#include "FigureDirs.h"

//////////////////////////////////////////////////////////////////////////
bool Board::isChecking(MoveCmd & move) const
{
  int8 idx0 = -1;
  move.checkingNum_ = 0;
  Figure::Color ocolor = Figure::otherColor(color_);
  const Figure & fig = getFigure(color_, move.index_);
  if ( move.castle_ )
  {
    const Figure & rook = getFigure(color_, move.rook_index_);
    idx0 = isAttackedBy(ocolor, rook);
    THROW_IF( getAttackedFrom(ocolor, move.rook_to_) != idx0, "castling rook attack isn't correctly detected" );

//    idx0 = getAttackedFrom(ocolor, move.rook_to_);
  }
  else
  {
    if ( Figure::TypePawn == fig.getType() || Figure::TypeKnight == fig.getType() )
    {
      const Figure & king = getFigure(ocolor, KingIndex);
      int dir = g_figureDir->dir(fig, king.where());
      if ( (Figure::TypePawn == fig.getType() && 0 == dir || 1 == dir) || (Figure::TypeKnight == fig.getType() && dir >= 0) )
        idx0 = move.index_;
    }
    else
    {
      idx0 = isAttackedBy(ocolor, fig);

      THROW_IF( getAttackedFrom(ocolor, move.to_) != idx0, "index of attacking figure isn't detected correctly" );

      //idx0 = getAttackedFrom(ocolor, move.to_);
    }

    THROW_IF( idx0 >= 0 && idx0 != move.index_, "attacked by wrong figure" );
  }

  if ( idx0 >= 0 )
    move.checking_[move.checkingNum_++] = idx0;

  if ( !move.castle_ )
  {
    //QpfTimer qpt;

    int idx1 = fastAttackedFrom( ocolor, move.from_ );

    //ticks_ += qpt.dt();
    //tcounter_++;

    THROW_IF( idx1 != getAttackedFrom( ocolor, move.from_ ), "fastAttackedFrom() failed" );

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
        int idx2 = fastAttackedFrom(ocolor, fake.where());
        THROW_IF( idx2 != getAttackedFrom( ocolor, fake.where() ), "fastAttackedFrom() failed" );

        if ( idx2 >= 0 && idx2 != idx0 && idx2 != idx1 )
          move.checking_[move.checkingNum_++] = idx2;
      }
    }
  }

  THROW_IF( move.checkingNum_ > 2, "more than 2 figures attacking king" );
  THROW_IF( !move.checkingNum_ && wasMoveValid(move) && isAttacked(color_, getFigure(ocolor, KingIndex).where()), "ckeck wasn't detected" );

  return move.checkingNum_ > 0;
}

//////////////////////////////////////////////////////////////////////////
bool Board::wasValidUnderCheck(const MoveCmd & move) const
{
  const Figure & fig = getFigure(color_, move.index_);
  Figure::Color ocolor = Figure::otherColor(color_);
  if ( Figure::TypeKing == fig.getType() )
  {
    THROW_IF(fastAttacked(ocolor, fig.where()) != isAttacked(ocolor, fig.where()), "fast attacked returned wrong result");
    return !move.castle_ && !fastAttacked(ocolor, fig.where());
  }

  if ( 1 == checkingNum_ )
  {
    THROW_IF( checking_[0] < 0 || checking_[0] >= NumOfFigures, "invalid checking figure index" );

    const Figure & king = getFigure(color_, KingIndex);

    if ( move.rindex_ == checking_[0] )
    {
      // maybe attacked from direction, my figure goes from
      int idx = fastAttackedFrom(color_, move.from_);
      THROW_IF( idx != getAttackedFrom(color_, move.from_), "fastAttackedFrom() failed" );
      return idx < 0;
    }

    const Figure & afig = getFigure(ocolor, checking_[0]);
    THROW_IF( Figure::TypeKing == afig.getType(), "king is attacking king" );
    THROW_IF( !afig, "king is attacked by non existing figure" );

    // Pawn and Knight could be only removed to escape from check
    if ( Figure::TypeKnight == afig.getType() || Figure::TypePawn == afig.getType() )
      return false;

    FPos dp1 = g_deltaPosCounter->getDeltaPos(move.to_, afig.where());
    FPos dp2 = g_deltaPosCounter->getDeltaPos(king.where(), move.to_);

    // we can protect king by putting figure between it and attacking figure.
    if ( FPos(0, 0) != dp1 && dp1 == dp2 )
    {
      // at the last we have to check if it's safe to move this figure
      int idx = fastAttackedFrom(color_, move.from_);
      THROW_IF( idx != getAttackedFrom(color_, move.from_), "fastAttackedFrom() failed" );
      return idx < 0;
    }

    return false;
  }

  THROW_IF(2 != checkingNum_, "invalid number of checking figures");
  return false;
}

//////////////////////////////////////////////////////////////////////////
bool Board::wasValidWithoutCheck(const MoveCmd & move) const
{
  const Figure & fig = getFigure(color_, move.index_);
  Figure::Color ocolor = Figure::otherColor(color_);
  if ( Figure::TypeKing == fig.getType() )
  {

    QpfTimer qpt;

    bool a = fastAttacked(ocolor, fig.where());

    ticks_ += qpt.dt();
    tcounter_++;

    THROW_IF( a != isAttacked(ocolor, fig.where()), "fast attacked returned wrong result");

    return !a;
  }

  int idx = fastAttackedFrom(color_, move.from_);
  THROW_IF( idx != getAttackedFrom(color_, move.from_), "fastAttackedFrom() failed" );
  return idx < 0;
}

//////////////////////////////////////////////////////////////////////////

/// gets index of figure, attacking from given direction
/// check only bishops, rook and queens
int Board::getAttackedFrom(Figure::Color color, int apt) const
{
  const Figure & king = getFigure(color, KingIndex);
  FPos dp = g_deltaPosCounter->getDeltaPos(apt, king.where());
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

    int dir = g_figureDir->dir(afig, king.where());
    return dir >= 0 ? afig.getIndex() : -1;
  }
  return -1;
}

int Board::fastAttackedFrom(Figure::Color color, int apt) const
{
  const Figure & king = getFigure(color, KingIndex);
  const uint64 & mask = g_betweenMasks->from(king.where(), apt);
  if ( !mask )
    return -1;

  Figure::Color ocolor = Figure::otherColor(color);
  const uint64 & bishops = fmgr_.bishop_mask(ocolor);
  const uint64 & rooks = fmgr_.rook_mask(ocolor);
  const uint64 & queens = fmgr_.queen_mask(ocolor);

  uint64 all = (bishops | rooks | queens) & mask;
  if ( !all )
    return -1;

  const uint64 & black = fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = fmgr_.mask(Figure::ColorWhite);

  uint64 figs_msk_inv = ~(black | white);

  for ( ; all; )
  {
    int n = least_bit_number(all);
    
    THROW_IF( (unsigned)n > 63, "least siginficant bit is invalid" );

    const Field & field = getField(n);
    THROW_IF( !field || field.color() == color, "there should be some figure of opponent's color on this field" );
    
    THROW_IF( field.type() != Figure::TypeBishop && field.type() != Figure::TypeRook && field.type() != Figure::TypeQueen, "figure should be bishop, rook or queen" );

    const Figure & afig = getFigure(ocolor, field.index());
    THROW_IF( !afig, "no figure, but bit in mask isn't zero" );

    int dir = g_figureDir->dir(afig, king.where());
    if ( dir < 0 )
      continue;

    const uint64 & btw_msk = g_betweenMasks->between(afig.where(), king.where());

    if ( (figs_msk_inv & btw_msk) != btw_msk )
      continue;

    return afig.getIndex();
  }

  return -1;
}

/// is field 'pos' attacked by given color?
bool Board::isAttacked(const Figure::Color c, int pos) const
{
  for (int i = KingIndex; i >= 0; --i)
  {
    const Figure & fig = getFigure(c, i);
    if ( !fig )
      continue;

    int dir = g_figureDir->dir(fig, pos);
    if ( (dir < 0) || (Figure::TypePawn == fig.getType() && (2 == dir || 3 == dir)) || (Figure::TypeKing == fig.getType() && dir > 7) )
      continue;

    if ( Figure::TypePawn == fig.getType() || Figure::TypeKnight == fig.getType() || Figure::TypeKing == fig.getType() )
      return true;

    FPos dp = g_deltaPosCounter->getDeltaPos(fig.where(), pos);

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
        return true;

      if ( field.color() == c && (Figure::TypeBishop == field.type() || Figure::TypeRook == field.type()) )
      {
        const Figure & afig = getFigure(c, field.index());
        int dir = g_figureDir->dir(afig, pos);
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
        int d = g_figureDir->dir(pawn, pos);
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

/// is field 'pos' attacked by given color?
bool Board::fastAttacked(const Figure::Color c, int8 pos) const
{
  Figure::Color ocolor = Figure::otherColor(c);

  // all long-range figures
  const uint64 & q_caps = g_movesTable->caps(Figure::TypeQueen, pos);
  uint64 attack_msk = fmgr_.bishop_mask(c) | fmgr_.rook_mask(c) | fmgr_.queen_mask(c);
  attack_msk &= q_caps;

  // do we have at least 1 attacking figure
  if ( attack_msk )
  {
    const uint64 & black = fmgr_.mask(Figure::ColorBlack);
    const uint64 & white = fmgr_.mask(Figure::ColorWhite);
    uint64 figs_msk_inv = ~(black | white);

    // queens
    uint64 queen_msk = fmgr_.queen_mask(c) & q_caps;
    for ( ; queen_msk; )
    {
      int n = least_bit_number(queen_msk);

      THROW_IF( (unsigned)n > 63, "invalid bit found in attack detector" );

      const Field & field = getField(n);

      THROW_IF( !field || field.type() != Figure::TypeQueen, "no figure but mask bit is 1" );
      THROW_IF( field.color() != c, "invalid figure color in attack detector" );
      THROW_IF( !getFigure(c, field.index()), "no figure in occupied field" );

      const uint64 & btw_msk = g_betweenMasks->between(n, pos);
      if ( (figs_msk_inv & btw_msk) == btw_msk )
        return true;
    }

    // rooks
    const uint64 & r_caps = g_movesTable->caps(Figure::TypeRook, pos);
    uint64 rook_msk = fmgr_.rook_mask(c) & r_caps;
    for ( ; rook_msk; )
    {
      int n = least_bit_number(rook_msk);

      THROW_IF( (unsigned)n > 63, "invalid bit found in attack detector" );

      const Field & field = getField(n);

      THROW_IF( !field || field.type() != Figure::TypeRook, "no figure but mask bit is 1" );
      THROW_IF( field.color() != c, "invalid figure color in attack detector" );
      THROW_IF( !getFigure(c, field.index()), "no figure in occupied field" );

      const uint64 & btw_msk = g_betweenMasks->between(n, pos);
      if ( (figs_msk_inv & btw_msk) == btw_msk )
        return true;
    }

    // bishops
    const uint64 & b_caps = g_movesTable->caps(Figure::TypeBishop, pos);
    uint64 bishop_msk = fmgr_.bishop_mask(c) & b_caps;
    for ( ; bishop_msk; )
    {
      int n = least_bit_number(bishop_msk);

      THROW_IF( (unsigned)n > 63, "invalid bit found in attack detector" );

      const Field & field = getField(n);

      THROW_IF( !field || field.type() != Figure::TypeBishop, "no figure but mask bit is 1" );
      THROW_IF( field.color() != c, "invalid figure color in attack detector" );
      THROW_IF( !getFigure(c, field.index()), "no figure in occupied field" );

      const uint64 & btw_msk = g_betweenMasks->between(n, pos);
      if ( (figs_msk_inv & btw_msk) == btw_msk )
        return true;
    }
  }

  // knights
  const uint64 & n_caps = g_movesTable->caps(Figure::TypeKnight, pos);
  const uint64 & knight_msk = fmgr_.knight_mask(c);
  if ( n_caps & knight_msk )
    return true;

  // pawns. masks are transposed
  const uint64 & p_caps = g_movesTable->pawnCaps_t(ocolor, pos);
  const uint64 & pawn_msk = fmgr_.pawn_mask(c);
  if ( p_caps & pawn_msk )
    return true;

  // king
  const uint64 & k_caps = g_movesTable->caps(Figure::TypeKing, pos);
  const uint64 & king_msk = fmgr_.king_mask(c);
  if ( k_caps & king_msk )
    return true;


  // at the last other figures
  //const uint64 & q_caps = g_movesTable->caps(Figure::TypeQueen, pos);
  //uint64 attack_msk = fmgr_.bishop_mask(c) | fmgr_.rook_mask(c) | fmgr_.queen_mask(c);
  //attack_msk &= q_caps;
  //if ( !attack_msk )
  //  return false;

  //for ( ; attack_msk; )
  //{
  //  int n = least_bit_number(attack_msk);

  //  THROW_IF( (unsigned)n > 63, "invalid bit found in attack detector" );

  //  const Field & field = getField(n);

  //  THROW_IF( !field, "no figure but mask bit is 1" );
  //  THROW_IF( field.color() != c, "invalid figure color in attack detector" );

  //  const Figure & fig = getFigure(c, field.index());

  //  THROW_IF( !fig, "no figure in occupied field" );

  //  // can current figure attack given field?
  //  int dir = g_figureDir->dir(fig, pos);
  //  if ( dir < 0 )
  //    continue;

  //  const uint64 & btw_msk = g_betweenMasks->between(n, pos);
  //  if ( (figs_msk_inv & btw_msk) == btw_msk )
  //    return true;
  //}

  return false;
}

//////////////////////////////////////////////////////////////////////////
// returns number of checking figures.
// very slow. used only for initial validation
int Board::findCheckingFigures(Figure::Color color, int pos)
{
  checkingNum_ = 0;
  for (int i = 0; i < KingIndex; ++i)
  {
    const Figure & fig = getFigure(color, i);
    if ( !fig )
      continue;

    int dir = g_figureDir->dir(fig, pos);
    if ( (dir < 0) || (Figure::TypePawn == fig.getType() && (2 == dir || 3 == dir)) || (Figure::TypeKing == fig.getType() && dir > 7) )
      continue;

    if ( Figure::TypePawn == fig.getType() || Figure::TypeKnight == fig.getType() )
    {
      if ( checkingNum_ > 1 )
        return ++checkingNum_;

      checking_[checkingNum_++] = i;
    }

    FPos dp = g_deltaPosCounter->getDeltaPos(fig.where(), pos);

    THROW_IF( FPos(0, 0) == dp, "invalid attacked position" );

    FPos p = FPosIndexer::get(pos) + dp;
    const FPos & figp = FPosIndexer::get(fig.where());
    bool have_figure = false;
    for ( ; p != figp; p += dp)
    {
      const Field & field = getField(p.index());
      if ( field )
      {
        have_figure = true;
        break;
      }
    }

    if ( !have_figure )
    {
      if ( checkingNum_ > 1 )
        return ++checkingNum_;

      checking_[checkingNum_++] = i;
    }
  }

  return checkingNum_;
}
