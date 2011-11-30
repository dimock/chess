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

ScoreType Figure::pawnGuarded_  =  8;
ScoreType Figure::pawnDoubled_  = -8;
ScoreType Figure::pawnIsolated_ = -8;

ScoreType Figure::pawnPassed_[2][8] = {
  { 0, 40, 25, 20, 15, 10, 5, 0 },
  { 0, 5, 10, 15, 20, 25, 40, 0 }
};
//ScoreType Figure::pawnPassed_[2][8] = {
//	{ 0, 50, 30, 25, 20, 15, 10, 0 },
//	{ 0, 10, 15, 20, 25, 30, 50, 0 }
//};

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
      wght += Figure::pawnPassed_[color][y]/* << stage*/;\
      const uint64 & guardmsk = g_pawnMasks->mask_guarded(color, pawn.where());\
      if ( pmsk & guardmsk )\
        wght += Figure::pawnGuarded_;\
    }\
    const uint64 & isomask = g_pawnMasks->mask_isolated(pawn.where() & 7);\
    if ( !(pmsk & isomask) && fmgr_.pawns(color) > 1 )\
      wght += Figure::pawnIsolated_;\
  }\
  uint8 column = ((uint8*)&pmsk)[(icol)];\
  int dblNum = BitsCounter::numBitsInByte(column);\
  ScoreType dblMask = ~((dblNum-1) >> 31);\
  wght -= ((dblNum-1) << 3) & dblMask;/* *Figure::pawnDoubled_;*/\
}

#define EVALUATE_PAWNS(wght, color)\
{\
  const uint64 & pmsk = fmgr_.pawn_mask(color);\
  Figure::Color ocolor = Figure::otherColor(color);\
  const uint64 & opmsk = fmgr_.pawn_mask(ocolor);\
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

      const uint8 * pmsk = (const uint8*)&fmgr_.pawn_mask((Figure::Color)color);

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

      if ( !castle_index_[color][0] && !castle_index_[color][1] /*!king.isFirstStep() || (!rook1 && !rook2)*/ ) // castle impossible
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
  //  weight -= evaluatePawns(Figure::ColorBlack, stages_[0]);
  //  weight += evaluatePawns(Figure::ColorWhite, stages_[1]);
  //}

  //if ( UnderCheck == state_ )
  //{
  //  static ScoreType s_checkWeight[2] = { 2, -2 };
  //  weight += s_checkWeight[color_];
  //}

  return weight;
}



//ScoreType Board::evaluatePawns(Figure::Color color, int stage) const
//{
//  const uint64 & pmsk = fmgr_.pawn_mask(color);
//
//  if ( !pmsk )
//    return 0;
//
//  ScoreType weight = 0;
//  Figure::Color ocolor = Figure::otherColor(color);
//  const uint64 & opmsk = fmgr_.pawn_mask(ocolor);
//
//  EVALUATE_PAWN_COLUMN(0);
//  EVALUATE_PAWN_COLUMN(1);
//  EVALUATE_PAWN_COLUMN(2);
//  EVALUATE_PAWN_COLUMN(3);
//  EVALUATE_PAWN_COLUMN(4);
//  EVALUATE_PAWN_COLUMN(5);
//  EVALUATE_PAWN_COLUMN(6);
//  EVALUATE_PAWN_COLUMN(7);
//
//  //for (int i = 0; i < 8; ++i)
//  //{
//  //  const Figure & pawn = getFigure(color, i);
//  //  if ( pawn.getType() == Figure::TypePawn )
//  //  {
//  //    const uint64 & passmsk = PawnMasks::mask_passed(color, pawn.where());
//  //    const uint64 & blckmsk = PawnMasks::mask_blocked(color, pawn.where());
//  //    if ( !(opmsk & passmsk) && !(pmsk & blckmsk) )
//  //    {
//  //      int y = pawn.where() >> 3;
//  //      weight += Figure::pawnPassed_[color][y]/* << stage*/;
//
//		//    const uint64 & guardmsk = PawnMasks::mask_guarded(color, pawn.where());
//		//    if ( pmsk & guardmsk )
//		//      weight += Figure::pawnGuarded_;
//  //    }
//
//  //    const uint64 & isomask = PawnMasks::mask_isolated(pawn.where() & 7);
//  //    if ( !(pmsk & isomask) && fmgr_.pawns(color) > 1 )
//  //      weight += Figure::pawnIsolated_;
//  //  }
//
//  //  uint8 column = ((uint8*)&pmsk)[i];
//  //  int dblNum = numBitsInByte(column);
//  //  ScoreType dblMask = ~((dblNum-1) >> 31);
//  //  weight -= ((dblNum-1) << 3) & dblMask;//* Figure::pawnDoubled_;
//  //}
//
//  return weight;
//}

ScoreType Board::evaluateWinnerLoser() const
{
  Figure::Color win_color = can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  ScoreType weight = fmgr_.weight(win_color);

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
