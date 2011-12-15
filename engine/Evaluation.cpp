#include "Figure.h"
#include "Board.h"

// TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing
ScoreType Figure::figureWeight_[7] = { 0, 100, 320, 330, 500, 950, 0 };

ScoreType Figure::positionEvaluations_[2][8][64] = {
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
       3,   5,   5,   5,   5,   5,   5,   3,
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

ScoreType Figure::pawnGuarded_  =  8;
ScoreType Figure::pawnDoubled_  = -8;
ScoreType Figure::pawnIsolated_ =-10;
ScoreType Figure::pawnBackward_ = -6;
ScoreType Figure::openRook_     =  8;

ScoreType Figure::pawnPassed_[2][8] = {
  { 0, 40, 25, 20, 15, 10, 5, 0 },
  { 0, 5, 10, 15, 20, 25, 40, 0 }
};

//ScoreType Figure::pawnPassed_[2][8] = {
//	{ 0, 50, 30, 25, 15, 10, 5, 0 },
//	{ 0, 5, 10, 15, 25, 30, 50, 0 }
//};

//////////////////////////////////////////////////////////////////////////
#define EVALUATE_OPEN_ROOKS(clr, score)\
{\
  uint64 rook_mask = fmgr_.rook_mask(clr);\
  Figure::Color oclr = Figure::otherColor(clr);\
  uint64 pmsk  = fmgr_.pawn_mask_t(clr) | fmgr_.pawn_mask_t(oclr);\
  for ( ; rook_mask; )\
  {\
    int n = least_bit_number(rook_mask);\
    THROW_IF( (unsigned)n > 63, "invalid rook index" );\
    THROW_IF( getField(n).color() != clr || getField(n).type() != Figure::TypeRook, "there should be rook on given field" );\
    int x = n & 7;\
    uint8 column = ((uint8*)&pmsk)[x];\
    if ( !column )\
      score += Figure::openRook_;\
  }\
}

//////////////////////////////////////////////////////////////////////////
#define EVALUATE_PAWN_COLUMN(wght, color, icol)\
{\
  const Figure & pawn = getFigure(color, (icol));\
  if ( pawn.getType() == Figure::TypePawn )\
  {\
    const uint64 & passmsk = g_pawnMasks->mask_passed(color, pawn.where());\
    const uint64 & blckmsk = g_pawnMasks->mask_blocked(color, pawn.where());\
    if ( !(opmsk & passmsk) && !(pmsk & blckmsk) )\
    {\
      int y = pawn.where() >> 3;\
      wght += Figure::pawnPassed_[color][y];\
      const uint64 & guardmsk = g_pawnMasks->mask_guarded(color, pawn.where());\
      if ( pmsk & guardmsk )\
        wght += Figure::pawnGuarded_;\
    }\
    const uint64 & isomask = g_pawnMasks->mask_isolated(pawn.where() & 7);\
    const uint64 & bkwmask = g_pawnMasks->mask_backward(pawn.where());\
    if ( !(pmsk & isomask) )\
      wght += Figure::pawnIsolated_;\
    else if ( !(pmsk & bkwmask) )\
      wght += Figure::pawnBackward_;\
  }\
  uint8 column = ((uint8*)&pmsk)[(icol)];\
  int dblNum = BitsCounter::numBitsInByte(column);\
  ScoreType dblMask = ~((dblNum-1) >> 31);\
  wght -= ((dblNum-1) << 3) & dblMask;\
}

#define EVALUATE_PAWNS(wght, color)\
{\
  const uint64 & pmsk = fmgr_.pawn_mask_t(color);\
  Figure::Color ocolor = Figure::otherColor(color);\
  const uint64 & opmsk = fmgr_.pawn_mask_t(ocolor);\
  EVALUATE_PAWN_COLUMN(wght, color, 0);\
  EVALUATE_PAWN_COLUMN(wght, color, 1);\
  EVALUATE_PAWN_COLUMN(wght, color, 2);\
  EVALUATE_PAWN_COLUMN(wght, color, 3);\
  EVALUATE_PAWN_COLUMN(wght, color, 4);\
  EVALUATE_PAWN_COLUMN(wght, color, 5);\
  EVALUATE_PAWN_COLUMN(wght, color, 6);\
  EVALUATE_PAWN_COLUMN(wght, color, 7);\
}

//////////////////////////////////////////////////////////////////////////
ScoreType Board::evaluate() const
{
  if ( ChessMat == state_ )
    return -Figure::WeightMat;
  else if ( drawState() )
    return Figure::WeightDraw;

  ScoreType weight = calculateEval();

  if ( Figure::ColorBlack  == color_ )
    weight = -weight;

  return weight;
}

ScoreType Board::calculateEval() const
{

  if ( can_win_[0] != can_win_[1] )
  {
    return evaluateWinnerLoser();
  }

  static int castleWeights[] = { -8, 8, 4 };

  ScoreType weight = fmgr_.weight();

  ScoreType kingEval[2] = { 0, 0 };
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
      int dist  = g_distanceCounter->getDistance(queen.where(), king.where());
      int odist = g_distanceCounter->getDistance(queen.where(), oking.where());
      kingEval[color] += (7 - odist);
      kingEval[color] += (7 - dist);
    }

    kingEval[color] += castleWeights[castle_[color]];

    if ( castle_[color] )
    {
      static int pawns_x[2][4] = { {5, 6, 7, -1}, {2, 1, 0, -1} };// for left/right castle

      static uint8 pmask_king[2] = { 96, 6 };
      static uint8 shifts[2] = { 5, 1 };
      static ScoreType king_penalties[2][4] = { {10, 5, 0, 0}, {10, 0, 5, 0} };

      const uint8 * pmsk = (const uint8*)&fmgr_.pawn_mask_t((Figure::Color)color);

      for (int i = 0; i < 3; ++i)
      {
        int x = pawns_x[castle_[color]-1][i];
        int m = ((pmsk[x] & pmask_king[color]) >> shifts[color]) & 3;
        kingEval[color] -= king_penalties[color][m];
      }
    }
    else
    {
      const Figure & king = getFigure((Figure::Color)color, KingIndex);
      const Figure & rook1 = getFigure((Figure::Color)color, RookIndex);
      const Figure & rook2 = getFigure((Figure::Color)color, RookIndex+1);

	  if ( !castling((Figure::Color)color, 0) && !castling((Figure::Color)color, 1)) // castle impossible
      {
        bool penalty = !((rook1 && (rook1.where()&7) > (king.where()&7)) || (rook2 && (rook2.where()&7) < (king.where()&7)));
        if ( penalty )
          kingEval[color] -= 10;
      }
    }
  }

  weight -= fmgr_.eval(Figure::ColorBlack, stages_[0]);
  weight += fmgr_.eval(Figure::ColorWhite, stages_[1]);

  weight -= kingEval[0];
  weight += kingEval[1];

  if ( fmgr_.pawns(Figure::ColorBlack) )
  {
    ScoreType pweight0 = 0;
    EVALUATE_PAWNS(pweight0, Figure::ColorBlack);
    weight -= pweight0;
  }

  if ( fmgr_.pawns(Figure::ColorWhite) )
  {
    ScoreType pweight1 = 0;
    EVALUATE_PAWNS(pweight1, Figure::ColorWhite);
    weight += pweight1;
  }

  //if ( fmgr_.pawns() > 0 )
  //{
  //  weight -= evaluatePawns(Figure::ColorBlack);
  //  weight += evaluatePawns(Figure::ColorWhite);
  //}

  //{
	 // weight -= evaluateRooks(Figure::ColorBlack);
	 // weight += evaluateRooks(Figure::ColorWhite);
  //}

  if ( fmgr_.rooks(Figure::ColorBlack) )
  {
    ScoreType score0 = 0;
    EVALUATE_OPEN_ROOKS(Figure::ColorBlack, score0);
    weight -= score0;
  }

  if ( fmgr_.rooks(Figure::ColorWhite) )
  {
    ScoreType score1 = 0;
    EVALUATE_OPEN_ROOKS(Figure::ColorWhite, score1);
    weight += score1;
  }

  //if ( UnderCheck == state_ )
  //{
  //  static ScoreType s_checkWeight[2] = { 2, -2 };
  //  weight += s_checkWeight[color_];
  //}

  return weight;
}

ScoreType Board::evaluateRooks(Figure::Color color) const
{
	uint64 rook_mask = fmgr_.rook_mask(color);
	if ( !rook_mask )
		return 0;

	Figure::Color ocolor = Figure::otherColor(color);
	uint64 pmsk  = fmgr_.pawn_mask_t(color) | fmgr_.pawn_mask_t(ocolor);

	ScoreType score = 0;

	for ( ; rook_mask; )
	{
		int n = least_bit_number(rook_mask);

		THROW_IF( (unsigned)n > 63, "invalid rook index" );

		THROW_IF( getField(n).color() != color || getField(n).type() != Figure::TypeRook, "there should be rook on given field" );

		int x = n & 7;

		uint8 column = ((uint8*)&pmsk)[x];
		if ( !column )
      score += Figure::openRook_;
	}

	return score;
}

ScoreType Board::evaluatePawns(Figure::Color color) const
{
  const uint64 & pmsk = fmgr_.pawn_mask_t(color);

  if ( !pmsk )
    return 0;

  ScoreType weight = 0;
  Figure::Color ocolor = Figure::otherColor(color);
  const uint64 & opmsk = fmgr_.pawn_mask_t(ocolor);

  //EVALUATE_PAWN_COLUMN(0);
  //EVALUATE_PAWN_COLUMN(1);
  //EVALUATE_PAWN_COLUMN(2);
  //EVALUATE_PAWN_COLUMN(3);
  //EVALUATE_PAWN_COLUMN(4);
  //EVALUATE_PAWN_COLUMN(5);
  //EVALUATE_PAWN_COLUMN(6);
  //EVALUATE_PAWN_COLUMN(7);

  for (int i = 0; i < 8; ++i)
  {
    const Figure & pawn = getFigure(color, i);
    if ( pawn.getType() == Figure::TypePawn )
    {
      const uint64 & passmsk = g_pawnMasks->mask_passed(color, pawn.where());
      const uint64 & blckmsk = g_pawnMasks->mask_blocked(color, pawn.where());
      if ( !(opmsk & passmsk) && !(pmsk & blckmsk) )
      {
        int y = pawn.where() >> 3;
        weight += Figure::pawnPassed_[color][y];

	    const uint64 & guardmsk = g_pawnMasks->mask_guarded(color, pawn.where());
	    if ( pmsk & guardmsk )
	      weight += Figure::pawnGuarded_;
      }

	  const uint64 & isomask = g_pawnMasks->mask_isolated(pawn.where() & 7);
	  const uint64 & bkwmask = g_pawnMasks->mask_backward(pawn.where());

	  // maybe isolated pawn
      if ( !(pmsk & isomask) )
        weight += Figure::pawnIsolated_;

	  // if no, it maybe backward
	  else if ( !(pmsk & bkwmask) )
		  weight += Figure::pawnBackward_;
    }

    uint8 column = ((uint8*)&pmsk)[i];
    int dblNum = BitsCounter::numBitsInByte(column);
    ScoreType dblMask = ~((dblNum-1) >> 31);
    weight -= ((dblNum-1) << 3) & dblMask;
  }

  return weight;
}

ScoreType Board::evaluateWinnerLoser() const
{
  Figure::Color win_color = can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  ScoreType weight = fmgr_.weight(win_color);
  //ScoreType weight = -fmgr_.weight(lose_color);
  //if ( !fmgr_.pawns(win_color) )
	 // weight += fmgr_.weight(win_color);
  //else
  //{
	 // weight += fmgr_.queens(win_color)*Figure::figureWeight_[Figure::TypeQueen] +
		//  fmgr_.knights(win_color)*Figure::figureWeight_[Figure::TypeKnight] +
		//  fmgr_.bishops(win_color)*Figure::figureWeight_[Figure::TypeBishop] +
		//  fmgr_.rooks(win_color)*Figure::figureWeight_[Figure::TypeRook] +
  //    fmgr_.pawns(win_color)*Figure::figureWeight_[Figure::TypeRook];
  //}

  const Figure & king_w = getFigure(win_color, KingIndex);
  const Figure & king_l = getFigure(lose_color, KingIndex);

  ScoreType kingEval = 0;
  if ( fmgr_.pawns(win_color) > 0 && fmgr_.weight(win_color)-fmgr_.weight(lose_color) < Figure::figureWeight_[Figure::TypeRook] )
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
    int dist  = g_distanceCounter->getDistance(king_w.where(), king_l.where());
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

  weight += fmgr_.pawns() << 6;

  if ( win_color == Figure::ColorBlack )
    weight = -weight;

  //if ( fmgr_.pawns() > 0 )
  //{
  //  weight -= evaluatePawns(Figure::ColorBlack, 1);
  //  weight += evaluatePawns(Figure::ColorWhite, 1);
  //}


  if ( fmgr_.pawns(Figure::ColorBlack) )
  {
    ScoreType pweight0 = 0;
    EVALUATE_PAWNS(pweight0, Figure::ColorBlack);
    weight -= pweight0;
  }

  if ( fmgr_.pawns(Figure::ColorWhite) )
  {
    ScoreType pweight1 = 0;
    EVALUATE_PAWNS(pweight1, Figure::ColorWhite);
    weight += pweight1;
  }

  //if ( UnderCheck == state_ )
  //{
  //  static ScoreType s_checkWeight[2] = { 5, -5 };
  //  weight += s_checkWeight[color_];
  //}

  return weight;
}

////////////////////////////////////////////////////////////////////////////
//// DEBUG ONLY
////////////////////////////////////////////////////////////////////////////
//
//#ifndef NDEBUG
//ScoreType Board::calculatePositionEval(int stage)
//{
//  ScoreType weight = 0;
//  for (int color = 0; color < 2; ++color)
//  {
//    for (int i = 0; i < NumOfFigures; ++i)
//    {
//      const Figure & fig = getFigure((Figure::Color)color, i);
//      if ( !fig )
//        continue;
//
//      ScoreType w = Figure::positionEvaluation(stage, color, fig.getType(), fig.where());
//
//      if ( !color )
//        w = -w;
//
//      weight += w;
//    }
//  }
//  return weight;
//}
//#endif
