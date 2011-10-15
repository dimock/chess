#include "ChessBoard.h"

// TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing
WeightType Figure::figureWeight_[7] = { 0, 100, 320, 330, 500, 950, 0 };

WeightType Figure::positionEvaluations_[2][8][64] = {
  // begin
  {
    // empty
    {},

      // pawn
    {
       0,   0,   0,   0,   0,   0,   0,   0,
      20,  20,  20,  20,  20,  20,  20,  20,
       4,   4,   8,  12,  12,   8,   4,   4,
       2,   2,   4,  10,  10,   4,   2,   2,
       0,   0,   0,   8,   8,   0,   0,   0,
       2,  -2,  -4,   0,   0,  -4,  -2,   2,
       2,   4,   4,  -8,  -8,   4,   4,   2,
       0,   0,   0,   0,   0,   0,   0,   0
    },

    // knight
    {
     -12, -12, -12, -12, -12, -12, -12, -12,
     -12,  -8,   0,   0,   0,   0,  -8, -12,
     -12,   0,   4,   6,   6,   4,   0, -12,
     -12,   2,   6,   8,   8,   6,   2, -12,
     -12,   0,   6,   8,   8,   6,   0, -12,
     -12,   2,   4,   6,   6,   4,   2, -12,
     -12,  -8,   0,   2,   2,   0,  -8, -12,
     -12, -16, -12, -12, -12, -12, -12, -12
    },

    // bishop
    {
      -8,  -4,  -4,  -4,  -4,  -4,  -4,  -8,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   2,   8,   8,   2,   0,  -2,
      -2,   2,   2,   8,   8,   2,   2,  -2,
      -2,   0,   8,   8,   8,   8,   0,  -2,
      -2,   8,   8,   8,   8,   8,   8,  -2,
      -2,   2,   0,   0,   0,   0,   2,  -2,
      -8,  -4,  -4,  -4,  -4,  -4,  -4,  -8
    },

    // rook
    {
       0,   0,   0,   0,   0,   0,   0,   0,
       2,   4,   4,   4,   4,   4,   4,   2,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   0,   0,   0,   0,   0,  -2,
       0,   0,   0,   2,   2,   0,   0,   0
    },

    // queen
    {
      -8,  -4,  -4,  -2,  -2,  -4,  -4,  -8,
      -4,   0,   0,   0,   0,   0,   0,  -4,
      -4,   0,   2,   2,   2,   2,   0,  -4,
      -2,   0,   2,   2,   2,   2,   0,  -2,
       0,   0,   2,   2,   2,   2,   0,  -2,
      -4,   2,   2,   2,   2,   2,   0,  -4,
      -4,   0,   2,   0,   0,   0,   0,  -4,
      -8,  -4,  -4,  -2,  -2,  -4,  -4,  -8
    },

    // king
    {
     -12, -16, -16, -20, -20, -16, -16, -12,
     -12, -16, -16, -20, -20, -16, -16, -12,
     -12, -16, -16, -20, -20, -16, -16, -12,
     -12, -16, -16, -20, -20, -16, -16, -12,
      -8, -12, -12, -16, -16, -12, -12,  -8,
      -4,  -8,  -8,  -8,  -8,  -8,  -8,  -4,
       8,   8,   0,   0,   0,   0,   8,   8,
       8,  12,   4,   0,   0,   4,  12,   8
    },

    {}
  },

    // end
  {
    // empty
    {},

      // pawn
    {
       0,   0,   0,   0,   0,   0,   0,   0,
      20,  20,  20,  20,  20,  20,  20,  20,
      16,  16,  16,  16,  16,  16,  16,  16,
      12,  12,  12,  12,  12,  12,  12,  12,
       8,   8,   8,   8,   8,   8,   8,   8,
       4,   4,   4,   4,   4,   4,   4,   4,
       0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0
    },

    // knight
    {},

    // bishop
    {},

    // rook
    {},

    // queen
    {},

    // king
    {
     -20, -16, -12, -12, -12, -12, -16, -20,
     -16,  -8,   0,   0,   0,   0,  -8, -16,
     -12,  -4,   8,  12,  12,   8,  -4, -12,
     -12,  -4,  12,  16,  16,  12,  -4, -12,
     -12,  -4,  12,  16,  16,  12,  -4, -12,
     -12,  -4,   8,  12,  12,   8,  -4, -12,
     -16,  -8,   0,   0,   0,   0,  -8, -16,
     -20, -16, -12, -12, -12, -12, -16, -20
    },

    {}
  }
};

WeightType Figure::pawnGuarded_  =  8;
WeightType Figure::pawnDoubled_  = -8;
WeightType Figure::pawnIsolated_ = -8;

WeightType Figure::pawnPassed_[2][8] = {
  { 0, 40, 25, 20, 15, 10, 5, 0 },
  { 0, 5, 10, 15, 20, 25, 40, 0 }
};
//WeightType Figure::pawnPassed_[2][8] = {
//	{ 0, 50, 30, 25, 20, 15, 10, 0 },
//	{ 0, 10, 15, 20, 25, 30, 50, 0 }
//};

//////////////////////////////////////////////////////////////////////////

WeightType Figure::positionEvaluation(int stage, int color, int type, int pos)
{
  THROW_IF( stage < 0 || stage > 1 || color < 0 || color > 1 || type < 0 || type > 7 || pos < 0 || pos > 63, "invalid figure params" );

  if ( color )
  {
    int x = pos & 7;
    int y = 7 - (pos >> 3);
    pos = x | (y << 3);
  }

  WeightType e = positionEvaluations_[stage][type][pos];
  return e;
}

//////////////////////////////////////////////////////////////////////////

WeightType Board::evaluate() const
{
  WeightType weight = calculateEval();

  if ( Figure::ColorWhite  == color_ )
    weight = -weight;

  return weight;
}

WeightType Board::calculateEval() const
{
  if ( ChessMat == state_ )
  {
    static WeightType mat_weights[2] = { Figure::WeightMat, -Figure::WeightMat };
    return mat_weights[color_];
  }
  else if ( drawState() )
    return Figure::WeightDraw;

  if ( can_win_[0] != can_win_[1] )
  {
    return evaluateWinnerLoser();
  }

  static int castleWeights[] = { -8, 8, 4 };

  WeightType weight = fmgr_.weight();

  WeightType kingEval[2] = { 0, 0 };
  for (int color = 0; color < 2; ++color)
  {
    if ( stages_[color] > 0 )
      continue;

    Figure::Color ocolor = Figure::otherColor((Figure::Color)color);

    const Figure & queen = getFigure((Figure::Color)color, QueenIndex);
    if ( queen )
    {
      const Figure & king = getFigure((Figure::Color)color, KingIndex);
      const Figure & oking = getFigure(ocolor, KingIndex);
      int dist = getDistance(queen.where(), king.where());
      int odist = getDistance(queen.where(), oking.where());
      kingEval[color] += (7 - odist);
      kingEval[color] += (7 - dist);
    }

    kingEval[color] += castleWeights[castle_[color]];

    if ( castle_[color] )
    {
      static int pawns_x[2][4] = { {5, 6, 7, -1}, {2, 1, 0, -1} };// for left/right castle

      static uint8 pmask_king[2] = { 96, 6 };
      static uint8 shifts[2] = { 5, 1 };
      static WeightType king_penalties[2][4] = { {10, 5, 0, 0}, {10, 0, 5, 0} };

      const uint8 * pmsk = (const uint8*)&fmgr_.pawn_mask((Figure::Color)color);

      for (int i = 0; i < 3; ++i)
      {
        int x = pawns_x[castle_[color]-1][i];
        int m = ((pmsk[x] & pmask_king[color]) >> shifts[color]) & 3;
        kingEval[color] -= king_penalties[color][m];
      }
    }
  }

  weight -= fmgr_.eval(Figure::ColorBlack, stages_[0]);
  weight += fmgr_.eval(Figure::ColorWhite, stages_[1]);

  weight -= kingEval[0];
  weight += kingEval[1];

  if ( fmgr_.pawns() > 0 )
  {
    weight -= evaluatePawns(Figure::ColorBlack, stages_[0]);
    weight += evaluatePawns(Figure::ColorWhite, stages_[1]);
  }

  if ( UnderCheck == state_ )
  {
    static WeightType s_checkWeight[2] = { 2, -2 };
    weight += s_checkWeight[color_];
  }

  return weight;
}

WeightType Board::evaluatePawns(Figure::Color color, int stage) const
{
  const uint64 & pmsk = fmgr_.pawn_mask(color);

  if ( !pmsk )
    return 0;

  WeightType weight = 0;
  Figure::Color ocolor = Figure::otherColor(color);
  const uint64 & opmsk = fmgr_.pawn_mask(ocolor);

  for (int i = 0; i < 8; ++i)
  {
    const Figure & pawn = getFigure(color, i);
    if ( pawn.getType() == Figure::TypePawn )
    {
      const uint64 & passmsk = PawnMasks::mask_passed(color, pawn.where());
      const uint64 & blckmsk = PawnMasks::mask_blocked(color, pawn.where());
      if ( !(opmsk & passmsk) && !(pmsk & blckmsk) )
      {
        int y = pawn.where() >> 3;
        weight += Figure::pawnPassed_[color][y]/* << stage*/;

		    const uint64 & guardmsk = PawnMasks::mask_guarded(color, pawn.where());
		    if ( pmsk & guardmsk )
		      weight += Figure::pawnGuarded_;
      }

      const uint64 & isomask = PawnMasks::mask_isolated(pawn.where() & 7);
      if ( !(pmsk & isomask) && fmgr_.pawns(color) > 1 )
        weight += Figure::pawnIsolated_;
    }

    uint8 column = ((uint8*)&pmsk)[i];
    int dblNum = numBitsInByte(column);
    if ( dblNum > 1 )
      weight += (dblNum-1) * Figure::pawnDoubled_;
  }
  return weight;
}

WeightType Board::evaluateWinnerLoser() const
{
  Figure::Color win_color = can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  WeightType weight = fmgr_.weight(win_color);

  const Figure & king_w = getFigure(win_color, KingIndex);
  const Figure & king_l = getFigure(lose_color, KingIndex);

  WeightType kingEval = 0;
  if ( fmgr_.pawns(win_color) > 0 )
  {
    int yw = king_w.where() >> 3;
    int yl = king_l.where() >> 3;

    if ( win_color ) // white
    {
      kingEval = (yw + (7-yl)) << 3;
    }
    else // black
    {
      kingEval = ((7-yw) + yl) << 3;
    }
  }
  else
  {
    int dist = getDistance(king_w.where(), king_l.where());
    kingEval -= dist << 3;
    kingEval -= Figure::positionEvaluation(1, lose_color, Figure::TypeKing, king_l.where());
  }

  weight += kingEval;

  if ( fmgr_.rooks(win_color) == 0 && fmgr_.queens(win_color) == 0 )
  {
    int num_figs = fmgr_.knights(lose_color) + fmgr_.bishops(lose_color);
    weight -= (num_figs<<3);
  }
  else
    weight -= fmgr_.weight(lose_color);

  if ( win_color == Figure::ColorBlack )
    weight = -weight;

  if ( fmgr_.pawns() > 0 )
  {
    weight -= evaluatePawns(Figure::ColorBlack, 1);
    weight += evaluatePawns(Figure::ColorWhite, 1);
  }

  if ( UnderCheck == state_ )
  {
    static WeightType s_checkWeight[2] = { 5, -5 };
    weight += s_checkWeight[color_];
  }

  return weight;
}

//////////////////////////////////////////////////////////////////////////
// DEBUG ONLY
//////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
WeightType Board::calculatePositionEval(int stage)
{
  WeightType weight = 0;
  for (int color = 0; color < 2; ++color)
  {
    for (int i = 0; i < NumOfFigures; ++i)
    {
      const Figure & fig = getFigure((Figure::Color)color, i);
      if ( !fig )
        continue;

      WeightType w = Figure::positionEvaluation(stage, color, fig.getType(), fig.where());

      if ( !color )
        w = -w;

      weight += w;
    }
  }
  return weight;
}
#endif
