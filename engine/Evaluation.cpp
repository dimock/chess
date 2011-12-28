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
       7,   7,   7,   7,   7,   7,   7,   7,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -5,  -5,   3,   3,   3,   3,  -5,  -5
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
      12,  12,   6,   0,   0,   0,  16,  16
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
ScoreType Figure::pawnBackward_ = -8;
ScoreType Figure::openRook_     =  8;
ScoreType Figure::assistantBishop_ = 5;
ScoreType Figure::onlineRooks_ = 8;

//ScoreType Figure::pawnPassed_[2][8] = {
//  { 0, 40, 25, 20, 15, 10, 5, 0 },
//  { 0, 5, 10, 15, 20, 25, 40, 0 }
//};

#define PAWN_PROMOTION_MAX_ 60

ScoreType Figure::pawnPassed_[2][8] = {
	{ 0, PAWN_PROMOTION_MAX_, 40, 25, 15, 10, 5, 0 },
	{ 0, 5, 10, 15, 25, 40, PAWN_PROMOTION_MAX_, 0 }
};

#define FAST_ROOK_PAWN_EVAL

//////////////////////////////////////////////////////////////////////////
#define EVALUATE_OPEN_ROOKS(clr, score)\
{\
  uint64 rook_mask = fmgr_.rook_mask(clr);\
  Figure::Color oclr = Figure::otherColor(clr);\
  uint64 pmsk  = fmgr_.pawn_mask_t(clr) | fmgr_.pawn_mask_t(oclr);\
  int8 rfields[64] = {-1, -1};\
  int num = 0;\
  for ( ; rook_mask; )\
  {\
    int n = least_bit_number(rook_mask);\
    rfields[num++] = n;\
    THROW_IF( (unsigned)n > 63, "invalid rook index" );\
    THROW_IF( getField(n).color() != clr || getField(n).type() != Figure::TypeRook, "there should be rook on given field" );\
    int x = n & 7;\
    int16 column = ((uint8*)&pmsk)[x];\
    int16 col_msk = (column-1) >> 15;\
    score += Figure::openRook_ & col_msk;\
  }\
  /*if ( num > 1 )\
  {\
    THROW_IF( rfields[0] < 0 || rfields[1] < 0, "invalid rook fields" );\
    const Field & field = getField(rfields[0]);\
    const Figure & rook0 = getFigure(clr, field.index());\
    int dir = g_figureDir->dir(rook0, rfields[1]);\
    if ( dir >= 0 )\
    {\
      const uint64 & btw_msk = g_betweenMasks->between(rfields[0], rfields[1]);\
      const uint64 & black = fmgr_.mask(Figure::ColorBlack);\
      const uint64 & white = fmgr_.mask(Figure::ColorWhite);\
      uint64 all_msk_inv = ~(black | white);\
      if ( (btw_msk & all_msk_inv) == btw_msk )\
        score += Figure::onlineRooks_;\
    }\
  }*/\
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
      Figure::Color dst_color = (Figure::Color)g_pawnMasks->pawn_dst_color(color, pawn.where());\
      int16 bnum = fmgr_.bishops_c(color, dst_color);\
      ScoreType bsp_msk = ~((bnum-1) >> 15);\
      wght += Figure::assistantBishop_ & bsp_msk;\
      int obnum = fmgr_.bishops_c(ocolor, dst_color);\
      ScoreType obsp_msk = ~((obnum-1) >> 15);\
      wght -= Figure::assistantBishop_ * obsp_msk;\
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
  
  ScoreType score = -std::numeric_limits<ScoreType>::max();
  if ( can_win_[0] != can_win_[1] )
    score = evaluateWinnerLoser();
  else
    score = calculateEval();

  if ( Figure::ColorBlack  == color_ )
    score = -score;

  return score;
}

ScoreType Board::expressEval() const
{
	if ( ChessMat == state_ )
		return -Figure::WeightMat;
	else if ( drawState() )
		return Figure::WeightDraw;

	if ( can_win_[0] != can_win_[1] )
		return evaluateWinnerLoser();

	ScoreType weight = fmgr_.weight();
	weight -= fmgr_.eval(Figure::ColorBlack, stages_[0]);
	weight += fmgr_.eval(Figure::ColorWhite, stages_[1]);

	if ( Figure::ColorBlack  == color_ )
		weight = -weight;

	return weight;
}

ScoreType Board::calculateEval() const
{
  ScoreType weight = fmgr_.weight();

  ScoreType kingEval[2] = { 0, 0 };
  for (int color = 0; color < 2; ++color)
  {
    Figure::Color ocolor = Figure::otherColor((Figure::Color)color);

    if ( stages_[color] > 0 )
    {
      if ( fmgr_.pawns(ocolor) )
        kingEval[color] = evalPawnsEndgame((Figure::Color)color);

      continue;
    }

    const Figure & queen = getFigure((Figure::Color)color, QueenIndex);
    const Figure & king = getFigure((Figure::Color)color, KingIndex);
    if ( queen )
    {
      const Figure & oking = getFigure(ocolor, KingIndex);
      int dist  = g_distanceCounter->getDistance(queen.where(), king.where());
      int odist = g_distanceCounter->getDistance(queen.where(), oking.where());
      kingEval[color] += (7 - odist);
      kingEval[color] += (7 - dist);
    }

    static int8 castle_mask[8] = { 2,2,2, 0,0, 1,1,1 }; // 1 - short (K); 2 - long (Q)
    int8 castle = castle_mask[king.where() & 7]; // determine by king's x-position
    int8 ky = king.where() >> 3;
    if ( !castle || (ky > 1 && color) || (ky < 6 && !color) )
      continue;

    castle--;

    static int8 pawns_x[2][4] = { {5, 6, 7, -1}, {2, 1, 0, -1} };// for left/right castle
    static uint8 pmask_king[2] = { 96, 6 };
    static uint8 opmask_king[2] = { 48, 6 };
    static uint8 shifts[2] = { 5, 1 };
    static uint8 oshifts[2] = { 4, 2 };
    static ScoreType king_penalties[2][4] = { {10, 3, 0, 0}, {10, 0, 3, 0} };
    static ScoreType king_o_penalties[2][4] = { {0, 3, 8, 8}, {0, 8, 3, 8} };

    const uint8 * pmsk = (const uint8*)&fmgr_.pawn_mask_t((Figure::Color)color);
    const uint8 * opmsk = (const uint8*)&fmgr_.pawn_mask_t(ocolor);

    for (int i = 0; i < 3; ++i)
    {
      // my pawns shield
      int x = pawns_x[castle][i];
      int m = ((pmsk[x] & pmask_king[color]) >> shifts[color]) & 3;
      kingEval[color] -= king_penalties[color][m];

      // attacker pawns
      int o = ((opmsk[x] & opmask_king[color]) >> oshifts[color]) & 3;
      kingEval[color] -= king_o_penalties[color][o];
    }
  }

  weight -= fmgr_.eval(Figure::ColorBlack, stages_[0]);
  weight += fmgr_.eval(Figure::ColorWhite, stages_[1]);

  weight -= kingEval[0];
  weight += kingEval[1];

#ifdef FAST_ROOK_PAWN_EVAL
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
#else
  if ( fmgr_.pawns() > 0 )
  {
    weight -= evaluatePawns(Figure::ColorBlack);
    weight += evaluatePawns(Figure::ColorWhite);
  }

  {
	  weight -= evaluateRooks(Figure::ColorBlack);
	  weight += evaluateRooks(Figure::ColorWhite);
  }
#endif

  return weight;
}

ScoreType Board::evalPawnsEndgame(Figure::Color color) const
{
  Figure::Color ocolor = Figure::otherColor(color);
  const Figure & king = getFigure(color, KingIndex);
  ScoreType score = 0;
  for (int i = 0; i < 8; ++i)
  {
    const Figure & pawn = getFigure(ocolor, i);
    if ( Figure::TypePawn != pawn.getType() )
      continue;

    int dist = g_distanceCounter->getDistance(king.where(), pawn.where());
    score += (7 - dist);
  }
  return score;
}

ScoreType Board::evaluateRooks(Figure::Color color) const
{
	uint64 rook_mask = fmgr_.rook_mask(color);
	if ( !rook_mask )
		return 0;

	Figure::Color ocolor = Figure::otherColor(color);
	uint64 pmsk  = fmgr_.pawn_mask_t(color) | fmgr_.pawn_mask_t(ocolor);

	ScoreType score = 0;
  int8 rfields[64] = {-1, -1};
  int num = 0;

	for ( ; rook_mask; )
	{
		int n = least_bit_number(rook_mask);
    rfields[num++] = n;

		THROW_IF( (unsigned)n > 63, "invalid rook index" );

		THROW_IF( getField(n).color() != color || getField(n).type() != Figure::TypeRook, "there should be rook on given field" );

		int x = n & 7;

		int16 column = ((uint8*)&pmsk)[x];
    int16 col_msk = (column-1) >> 15;
    score += Figure::openRook_ & col_msk;
	}

  // do they see each other?
  if ( num > 1 )
  {
    THROW_IF( rfields[0] < 0 || rfields[1] < 0, "invalid rook fields" );
    const Field & field = getField(rfields[0]);
    const Figure & rook0 = getFigure(color, field.index());
    int dir = g_figureDir->dir(rook0, rfields[1]);
    if ( dir >= 0 )
    {
      const uint64 & btw_msk = g_betweenMasks->between(rfields[0], rfields[1]);
      const uint64 & black = fmgr_.mask(Figure::ColorBlack);
      const uint64 & white = fmgr_.mask(Figure::ColorWhite);
      uint64 all_msk_inv = ~(black | white);
      if ( (btw_msk & all_msk_inv) == btw_msk )
        score += Figure::onlineRooks_;
    }
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

  for (int i = 0; i < 8; ++i)
  {
    const Figure & pawn = getFigure(color, i);
    if ( pawn.getType() == Figure::TypePawn )
    {
      const uint64 & passmsk = g_pawnMasks->mask_passed(color, pawn.where());
      const uint64 & blckmsk = g_pawnMasks->mask_blocked(color, pawn.where());

      // passed pawn evaluation
      if ( !(opmsk & passmsk) && !(pmsk & blckmsk) )
      {
        int y = pawn.where() >> 3;
        weight += Figure::pawnPassed_[color][y];

        // guarded by neighbour pawn
        const uint64 & guardmsk = g_pawnMasks->mask_guarded(color, pawn.where());
        if ( pmsk & guardmsk )
          weight += Figure::pawnGuarded_;

        // have bishop with the same field's color as pawn's destination field
        Figure::Color dst_color = (Figure::Color)g_pawnMasks->pawn_dst_color(color, pawn.where());
        int16 bnum = fmgr_.bishops_c(color, dst_color);
        ScoreType bsp_msk = ~((bnum-1) >> 15);
        weight += Figure::assistantBishop_ & bsp_msk;

        // have enemy's bishop
        int obnum = fmgr_.bishops_c(ocolor, dst_color);
        ScoreType obsp_msk = ~((obnum-1) >> 15);
        weight -= Figure::assistantBishop_ * obsp_msk;
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
    int16 dblNum = BitsCounter::numBitsInByte(column);
    ScoreType dblMask = ~((dblNum-1) >> 15);
    weight -= ((dblNum-1) << 3) & dblMask;
  }

  return weight;
}

ScoreType Board::evaluateWinnerLoser() const
{
  Figure::Color win_color = can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  ScoreType weight = fmgr_.weight(win_color);

  const Figure & king_w = getFigure(win_color, KingIndex);
  const Figure & king_l = getFigure(lose_color, KingIndex);

  ScoreType kingEval = 0;
  if ( fmgr_.pawns(win_color) > 0 )//&& fmgr_.weight(win_color)-fmgr_.weight(lose_color) < Figure::figureWeight_[Figure::TypeRook] )
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
    kingEval -= evalPawnsEndgame(lose_color);
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

  weight += fmgr_.pawns(win_color) * PAWN_PROMOTION_MAX_;

  //if ( fmgr_.pawns(Figure::ColorBlack) )
  //{
  //  ScoreType pweight0 = 0;
  //  EVALUATE_PAWNS(pweight0, Figure::ColorBlack);
  //  weight -= pweight0;
  //}

  //if ( fmgr_.pawns(Figure::ColorWhite) )
  //{
  //  ScoreType pweight1 = 0;
  //  EVALUATE_PAWNS(pweight1, Figure::ColorWhite);
  //  weight += pweight1;
  //}

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
