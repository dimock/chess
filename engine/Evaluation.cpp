/*************************************************************
  Evaluation.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Figure.h"
#include "Board.h"

enum {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
};

extern const
ScoreType Figure::positionEvaluations_[2][8][64] = {
  // begin
  {
    // empty
    {},

      // pawn
    {
       0,   0,   0,   0,   0,   0,   0,   0,
      10,  10,  10,  10,  10,  10,  10,  10,
       4,   4,   6,   8,   8,   6,   4,   4,
       2,   2,   4,   6,   6,   4,   2,   2,
       0,   0,   0,   4,   4,   0,   0,   0,
       2,  -2,  -4,   0,   0,  -4,  -2,   2,
       2,   4,   4,  -8,  -8,   4,   4,   2,
       0,   0,   0,   0,   0,   0,   0,   0
    },

    // knight
    {
     -10, -10, -10, -10, -10, -10, -10, -10,
     -10,  -8,   0,   0,   0,   0,  -8, -10,
     -10,   0,   4,   6,   6,   4,   0, -10,
     -10,   2,   6,   8,   8,   6,   2, -10,
     -10,   0,   6,   8,   8,   6,   0, -10,
     -10,   2,   4,   6,   6,   4,   2, -10,
     -10,  -8,   0,   2,   2,   0,  -8, -10,
     -10, -12, -10, -10, -10, -10, -12, -10
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
      -8,  -4,  -8,  -4,  -4,  -8,  -4,  -8
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
      -4,   0,   2,   2,   2,   2,   0,  -4,
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
       5,   5,   0,   0,   0,   0,   5,   5,
      10,  12,   6,   0,   0,   0,  16,  14
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
      14,  16,  16,  18, 18,  16,  16,  14,
      10,  10,  12,  12, 12,  12,  10,  10,
      7,   7,   8,   8,   8,   8,   7,   7,
      5,   5,   6,   6,   6,   6,   5,   5,
      3,   3,   4,   4,   4,   4,   3,   3,
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
     -14, -12, -10, -10, -10, -10, -12, -14,
     -12,  -8,   0,   0,   0,   0,  -8, -12,
     -10,  -4,   6,   8,   8,   6,  -4, -10,
     -10,  -4,   8,  10,  10,   8,  -4, -10,
     -10,  -4,   8,  10,  10,   8,  -4, -10,
     -10,  -4,   6,   8,   8,   6,  -4, -10,
     -12,  -8,   0,   0,   0,   0,  -8, -12,
     -14, -12, -10, -10, -10, -10, -12, -14
    },

    {}
  }
};

// king position eval for BN-mat
extern const ScoreType Figure::bishopKnightMat_[64] =
{
  16,   10,  6,  1, -2, -5,  -12,  -16,
  10,   12,  5, -1, -3, -6,  -14,  -12,
   5,    5,  4, -2, -4, -8,   -8,  -10,
  -1,   -1, -2, -6, -6, -6,   -5,   -4,
  -4,   -5, -6, -6, -6, -2,   -1,   -1,
  -10,  -8, -8, -4, -2,  4,    5,    5,
  -12, -14, -6, -3, -1,  5,   12,   10,
  -16, -12, -5, -2,  1,  6,   10,   16
};

extern const ScoreType Figure::pawnDoubled_  = -10;
extern const ScoreType Figure::pawnIsolated_ = -10;
extern const ScoreType Figure::pawnBackward_ = -10;
extern const ScoreType Figure::openRook_     =  8;
extern const ScoreType Figure::semiopenRook_ =  4;
extern const ScoreType Figure::winloseBonus_ =  20;
extern const ScoreType Figure::kingbishopPressure_ = 10;
extern const ScoreType Figure::fianchettoBonus_ = 4;
extern const ScoreType Figure::queenToMyKingDistPenalty_[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
extern const ScoreType Figure::fakecastlePenalty_ = 10;
extern const ScoreType Figure::castleImpossiblePenalty_ = 10;

#define MAX_PASSED_SCORE 60

extern const ScoreType Figure::pawnPassed_[2][8] = {
  { 0, MAX_PASSED_SCORE, 40, 25, 15, 10, 5, 0 },
  { 0, 5, 10, 15, 25, 40, MAX_PASSED_SCORE, 0 }
};

extern const ScoreType Figure::pawnGuarded_[2][8] = {
  { 0, 16, 12, 10, 6, 4, 0, 0 },
  { 0, 0, 4, 6, 10, 12, 16, 0 },
};

#undef FAST_ROOK_PAWN_EVAL

//////////////////////////////////////////////////////////////////////////
#define EVALUATE_OPEN_ROOKS(clr, score)\
{\
  uint64 rook_mask = fmgr_.rook_mask(clr);\
  Figure::Color oclr = Figure::otherColor(clr);\
  const uint8 * pmsk  = (const uint8*)&fmgr_.pawn_mask_t(clr);\
  const uint8 * opmsk = (const uint8*)&fmgr_.pawn_mask_t(oclr);\
  const Figure & oking = getFigure(oclr, KingIndex);\
  int okx = oking.where() & 7;\
  for ( ; rook_mask; )\
  {\
    int n = clear_lsb(rook_mask);\
    THROW_IF( (unsigned)n > 63, "invalid rook index" );\
    THROW_IF( getField(n).color() != clr || getField(n).type() != Figure::TypeRook, "there should be rook on given field" );\
    int x = n & 7;\
    if ( !opmsk[x] )\
    {\
      if ( !pmsk[x] )\
        score += Figure::openRook_;\
      else\
        score += Figure::semiopenRook_;\
    }\
    if ( x == okx || x == okx-1 || x == okx+1 )\
      score <<= 1;\
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
        wght += Figure::pawnGuarded_[color][y];\
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

#define EVALUATE_KING_PAWNS(index, coeff)\
{\
  int x = pawns_x[castle][index];\
  int m = ((pmsk[x] & pmask_king[color]) >> shifts[color]) & 3;\
  kingEval -= ((king_penalties[color][m])<<(coeff));\
  int o = ((opmsk[x] & opmask_king[color]) >> oshifts[color]) & 3;\
  kingEval -= king_o_penalties[color][o];\
}

//////////////////////////////////////////////////////////////////////////
ScoreType Board::evaluate() const
{
  if ( matState() )
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

  THROW_IF( score < -32760 || score > 32760, "invalid score" );

  if ( repsCount() > 1 )
    score >>= 1;

  return score;
}

ScoreType Board::calculateEval() const
{
  ScoreType score = fmgr_.weight();
  score -= fmgr_.eval(Figure::ColorBlack, 0);
  score += fmgr_.eval(Figure::ColorWhite, 0);

  return score;

  static const ScoreType fweight_max = Figure::figureWeight_[Figure::TypeQueen] + 2*Figure::figureWeight_[Figure::TypeRook] + 2*Figure::figureWeight_[Figure::TypeBishop] + 2*Figure::figureWeight_[Figure::TypeKnight];

  ScoreType fweight_b = fmgr_.weight(Figure::ColorBlack) - fmgr_.pawns(Figure::ColorBlack)*Figure::figureWeight_[Figure::TypePawn];
  ScoreType fweight_w = fmgr_.weight(Figure::ColorWhite) - fmgr_.pawns(Figure::ColorWhite)*Figure::figureWeight_[Figure::TypePawn];

  int stage_coef = 128 * (fweight_b + fweight_w) / fweight_max;

  score -= (stage_coef * fmgr_.eval(Figure::ColorBlack, 0) + (256-stage_coef) * fmgr_.eval(Figure::ColorBlack, 1)) >> 8;
  score += (stage_coef * fmgr_.eval(Figure::ColorWhite, 0) + (256-stage_coef) * fmgr_.eval(Figure::ColorWhite, 1)) >> 8;

  // king's safety
  score -= (stage_coef * evaluateKing(Figure::ColorBlack) + (256-stage_coef) * evalPawnsEndgame(Figure::ColorBlack)) >> 8;
  score += (stage_coef * evaluateKing(Figure::ColorWhite) + (256-stage_coef) * evalPawnsEndgame(Figure::ColorWhite)) >> 8;

  // fianchetto
  score += evaluateFianchetto();

#ifdef FAST_ROOK_PAWN_EVAL
  ScoreType pws_score = 0;
  if ( fmgr_.pawns(Figure::ColorBlack) )
  {
    ScoreType pweight0 = 0;
    EVALUATE_PAWNS(pweight0, Figure::ColorBlack);
    pws_score -= pweight0;
  }

  if ( fmgr_.pawns(Figure::ColorWhite) )
  {
    ScoreType pweight1 = 0;
    EVALUATE_PAWNS(pweight1, Figure::ColorWhite);
    pws_score += pweight1;
  }
  score += pws_score;

  if ( fmgr_.rooks(Figure::ColorBlack) )
  {
    ScoreType score0 = 0;
    EVALUATE_OPEN_ROOKS(Figure::ColorBlack, score0);
    score0 = (score0 * stage_coef) >> 8;
    score -= score0;
  }

  if ( fmgr_.rooks(Figure::ColorWhite) )
  {
    ScoreType score1 = 0;
    EVALUATE_OPEN_ROOKS(Figure::ColorWhite, score1);
    score1 = (score1 * stage_coef) >> 8;
    score += score1;
  }
#else
  if ( fmgr_.pawns() > 0 )
  {
    score -= evaluatePawns(Figure::ColorBlack);
    score += evaluatePawns(Figure::ColorWhite);
  }

  {
    score -= (evaluateRooks(Figure::ColorBlack) * stage_coef) >> 8;
    score += (evaluateRooks(Figure::ColorWhite) * stage_coef) >> 8;
  }
#endif

  return score;
}

inline ScoreType Board::evaluateKing(Figure::Color color) const
{
  ScoreType kingEval = 0;
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);

  BitMask queen_mask = fmgr_.queen_mask(color);
  Index ki_pos(kingPos(color));
  for ( ; queen_mask; )
  {
    int n = clear_lsb(queen_mask);
    int dist  = g_distanceCounter->getDistance(n, ki_pos);
    kingEval -= Figure::queenToMyKingDistPenalty_[dist];
  }

  static int8 castle_mask[8] = { 2,2,2, 0,0, 1,1,1 }; // 1 - short (K); 2 - long (Q)
  int8 castle = castle_mask[ki_pos.x()]; // determine by king's x-position
  int8 ky = ki_pos.y();
  bool bCastle = castle && !(ky > 1 && color) && !(ky < 6 && !color);
  if ( !bCastle )
  {
    //if ( !castling(color, 0) && !castling(color, 1) )
    //  kingEval -= Figure::castleImpossiblePenalty_;

    return kingEval;
  }

  castle--;

  static int8 pawns_x[2][4] = { {5, 6, 7, -1}, {2, 1, 0, -1} };// for left/right castle
  static uint8 pmask_king[2] = { 96, 6 };
  static uint8 opmask_king[2] = { 48, 6 };
  static uint8 shifts[2] = { 5, 1 };
  static uint8 oshifts[2] = { 4, 2 };
  static ScoreType king_penalties[2][4] = { {10, 2, 0, 0}, {10, 0, 2, 0} };
  static ScoreType king_o_penalties[2][4] = { {0, 5, 10, 10}, {0, 10, 5, 10} };

  const uint8 * pmsk  = (const uint8*)&fmgr_.pawn_mask_t((Figure::Color)color);
  const uint8 * opmsk = (const uint8*)&fmgr_.pawn_mask_t(ocolor);
  
  int ry = color ? A1 : A8;

  EVALUATE_KING_PAWNS(0, 0);
  EVALUATE_KING_PAWNS(1, 1);
  EVALUATE_KING_PAWNS(2, 1);

  // penalty for fake castle
  int rp = pawns_x[castle][1], kp = pawns_x[castle][2];
  rp = rp | ry;
  kp = kp | ry;
  const Field & rfield = getField(rp);
  const Field & kfield = getField(kp);
  if ( kfield.type() == Figure::TypeRook ||
      (rfield.type() == Figure::TypeRook && kfield.type() != Figure::TypeKing) )
    kingEval -= Figure::fakecastlePenalty_;

  // opponent bishop near king
  int bp0 = 0, bp1 = 0;
  if ( color )
  {
    if ( castle )
    {
      bp0 = A3;
      bp1 = C3;
    }
    else
    {
      bp0 = F3;
      bp1 = H3;
    }
  }
  else
  {
    if ( castle )
    {
      bp0 = A6;
      bp1 = C6;
    }
    else
    {
      bp0 = F6;
      bp1 = H6;
    }
  }

  const Field & fb0 = getField(bp0);
  const Field & fb1 = getField(bp1);

  if ( fb0.type() == Figure::TypeBishop && fb0.color() == ocolor || fb1.type() == Figure::TypeBishop && fb1.color() == ocolor )
    kingEval -= Figure::kingbishopPressure_;

  return kingEval;
}

// fianchetto
ScoreType Board::evaluateFianchetto() const
{
  ScoreType score = 0;

  const Field & fb2 = getField(B2);
  const Field & fb3 = getField(B3);

  const Field & fg2 = getField(G2);
  const Field & fg3 = getField(G3);
  
  const Field & fb7 = getField(B7);
  const Field & fb6 = getField(B6);
  
  const Field & fg7 = getField(G7);
  const Field & fg6 = getField(G6);

  // white
  if ( fb3.color() && fb3.type() == Figure::TypePawn )
  {
    if ( fb2.color() && fb2.type() == Figure::TypeBishop )
      score += Figure::fianchettoBonus_;
    else
      score -= Figure::fianchettoBonus_;
  }

  if ( fg3.color() && fg3.type() == Figure::TypePawn )
  {
    if ( fg2.color() && fg2.type() == Figure::TypeBishop )
      score += Figure::fianchettoBonus_;
    else
      score -= Figure::fianchettoBonus_;
  }

  // black
  if ( !fb6.color() && fb6.type() == Figure::TypePawn )
  {
    if ( !fb7.color() && fb7.type() == Figure::TypeBishop )
      score -= Figure::fianchettoBonus_;
    else
      score += Figure::fianchettoBonus_;
  }

  if ( !fg6.color() && fg6.type() == Figure::TypePawn )
  {
    if ( !fg7.color() && fg7.type() == Figure::TypeBishop )
      score -= Figure::fianchettoBonus_;
    else
      score += Figure::fianchettoBonus_;
  }

  return score;
}

ScoreType Board::evaluateRooks(Figure::Color color) const
{
	uint64 rook_mask = fmgr_.rook_mask(color);
	if ( !rook_mask )
		return 0;

	Figure::Color ocolor = Figure::otherColor(color);
	const uint8 * pmsk  = (const uint8*)&fmgr_.pawn_mask_t(color);
  const uint8 * opmsk = (const uint8*)&fmgr_.pawn_mask_t(ocolor);

  int ok_pos = kingPos(ocolor);
  int okx = ok_pos & 7;

	ScoreType score = 0;
	for ( ; rook_mask; )
	{
		int n = clear_lsb(rook_mask);

		THROW_IF( (unsigned)n > 63, "invalid rook index" );
		THROW_IF( getField(n).color() != color || getField(n).type() != Figure::TypeRook, "there should be rook on given field" );

		int x = n & 7;

    if ( !opmsk[x] )
    {
      if ( !pmsk[x]  )
        score += Figure::openRook_;
      else
        score += Figure::semiopenRook_;
    }

    if ( x == okx || x == okx-1 || x == okx+1 )
      score <<= 1;
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

  BitMask pawn_mask = fmgr_.pawn_mask_o(color);
  for ( ; pawn_mask; )
  {
    int n = clear_lsb(pawn_mask);

    const uint64 & passmsk = g_pawnMasks->mask_passed(color, n);
    const uint64 & blckmsk = g_pawnMasks->mask_blocked(color, n);

    // passed pawn evaluation
    if ( !(opmsk & passmsk) && !(pmsk & blckmsk) )
    {
      int y = n >> 3;
      weight += Figure::pawnPassed_[color][y];

      // guarded by neighbor pawn
      const uint64 & guardmsk = g_pawnMasks->mask_guarded(color, n);
      if ( pmsk & guardmsk )
        weight += Figure::pawnGuarded_[color][y];
    }

    const uint64 & isomask = g_pawnMasks->mask_isolated(n & 7);
    const uint64 & bkwmask = g_pawnMasks->mask_backward(n);

    // maybe isolated pawn
    if ( !(pmsk & isomask) )
      weight += Figure::pawnIsolated_;

    // if no, it maybe backward
    else if ( !(pmsk & bkwmask) )
      weight += Figure::pawnBackward_;
  }

  for (int i = 0; i < 8; ++i)
  {
    uint8 column = ((uint8*)&pmsk)[i];
    int16 dblNum = BitsCounter::numBitsInByte(column);
    ScoreType dblMask = ~((dblNum-1) >> 15);
    weight -= ((dblNum-1) << 3) & dblMask;
  }

  return weight;
}

ScoreType Board::evalPawnsEndgame(Figure::Color color) const
{
  ScoreType score = 0;
  Figure::Color ocolor = Figure::otherColor(color);
  Index ki_pos(kingPos(color));
  const BitMask & pmsk = fmgr_.pawn_mask_t(color);
  int kx = ki_pos.x();
  int ky = ki_pos.y();
  const BitMask & opmsk = fmgr_.pawn_mask_t(ocolor);

  // opponent has pawns
  if ( opmsk )
  {
    BitMask opawn_mask = opmsk;
    for ( ; opawn_mask; )
    {
      int n = clear_lsb(opawn_mask);

      int py = n >> 3;
      int y = py;
      if ( !ocolor )
        y = 7 -y;

      y |= 1;

      const uint64 & opassmsk = g_pawnMasks->mask_passed(ocolor, n);
      const uint64 & oblckmsk = g_pawnMasks->mask_blocked(ocolor, n);
      if ( !(pmsk & opassmsk) && !(opmsk & oblckmsk) )
      {
        int dist = g_distanceCounter->getDistance(ki_pos, n);

        int px = n & 7;
        int xdist = kx - px;
        if ( xdist < 0 )
          xdist = -xdist;

        ScoreType s = 0;

        if ( ocolor && py < ky || !ocolor && py > ky )
        {
          s += ((7 - xdist)*y) >> 1;
          s += ((7 - dist)*y) >> 1;
        }
        else
          s += ((7 - dist)*y);

        score += s;
      }
    }
  }
  // i have pawns
  else if ( pmsk )
  {
    BitMask pawn_mask = pmsk;
    for ( ; pawn_mask; )
    {
      int n = clear_lsb(pawn_mask);

      int py = n >> 3;
      int y = py;
      if ( !color )
        y = 7 -y;

      y |= 1;

      const uint64 & passmsk = g_pawnMasks->mask_passed(color, n);
      const uint64 & blckmsk = g_pawnMasks->mask_blocked(color, n);
      if ( !(pmsk & passmsk) )
      {
        int dist = g_distanceCounter->getDistance(ki_pos, n);
        score += ((7 - dist)*y);

        int px = n & 7;
        int xdist = kx - px;
        if ( xdist < 0 )
          xdist = -xdist;

        if ( color && py < ky || !color && py > ky )
          score += ((7 - xdist)*y) >> 1;
      }
    }
  }

  return score;
}


ScoreType Board::evaluateWinnerLoser() const
{
  Figure::Color win_color = can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  ScoreType weight = fmgr_.weight(win_color);

  Index king_pos_w = kingPos(win_color);
  Index king_pos_l = kingPos(lose_color);

  bool eval_pawns = true;

  if ( fmgr_.rooks(win_color) == 0 && fmgr_.queens(win_color) == 0 && fmgr_.pawns(win_color) > 0 )
  {
    int num_lose_figs = fmgr_.knights(lose_color) + fmgr_.bishops(lose_color);
    ScoreType weight_lose_fig = 10;

    // if winner has more pawns than loser and also has some figure he must exchange all loser figures to pawns
    if ( fmgr_.knights(lose_color)+fmgr_.bishops(lose_color) > 0  &&
         fmgr_.knights(win_color)+fmgr_.bishops(win_color) > 0 &&
         fmgr_.knights(lose_color)+fmgr_.bishops(lose_color) < fmgr_.pawns(win_color) )
    {
      weight_lose_fig = Figure::figureWeight_[Figure::TypePawn] + (MAX_PASSED_SCORE);
      eval_pawns = false;
    }

    weight -= num_lose_figs * weight_lose_fig;
  }
  else
    weight -= fmgr_.weight(lose_color);

  // add small bonus for winner-loser state
  weight += Figure::winloseBonus_;

  // BN-mat case
  if ( fmgr_.weight(lose_color) == 0 && fmgr_.knights(win_color) == 1 && fmgr_.bishops(win_color) == 1 &&
       fmgr_.rooks(win_color) == 0 && fmgr_.queens(win_color) == 0 && fmgr_.pawns(win_color) == 0 )
  {
    int dist  = g_distanceCounter->getDistance(king_pos_w, king_pos_l);
    weight -= dist;

    int kp = king_pos_l;
    if ( fmgr_.bishops_w(win_color) )
    {
      int kx = king_pos_l.x();
      int ky = king_pos_l.y();
      kp = ((7-ky)<<3)| kx;
    }
    weight += Figure::bishopKnightMat_[kp];

    uint64 n_mask = fmgr_.knight_mask(win_color);
    int np = clear_lsb(n_mask);
    THROW_IF( (unsigned)np > 63, "no knigt found" );
    int ndist = g_distanceCounter->getDistance(np, king_pos_l);
    weight -= ndist >> 1;

    // add more bonus to be sure that we go to this state
    weight += Figure::winloseBonus_;
  }
  else
  {
    // some special almost-draw cases
    if ( fmgr_.rooks(win_color) == 0 && fmgr_.queens(win_color) == 0 && fmgr_.pawns(win_color) == 0 &&
         fmgr_.weight(win_color)-fmgr_.weight(lose_color) < Figure::figureWeight_[Figure::TypeBishop]+Figure::figureWeight_[Figure::TypeKnight] )
    {
      weight = 10;
    }
    else if ( fmgr_.rooks(win_color) == 0 && fmgr_.queens(win_color) == 0 && fmgr_.pawns(win_color) == 1 &&
              fmgr_.knights(lose_color)+fmgr_.bishops(lose_color) > 0 )
    {
      if ( fmgr_.knights(win_color)+fmgr_.bishops(win_color) <= fmgr_.knights(lose_color)+fmgr_.bishops(lose_color) )
      {
        weight = (MAX_PASSED_SCORE);
        uint64 pwmask = fmgr_.pawn_mask_o(win_color);
        int pp = clear_lsb(pwmask);
        int x = pp & 7;
        int y = pp >> 3;
        if ( !win_color )
          y = 7-y;
        if ( y < 6 )
        {
          int ep = x | (win_color ? A8 : A1);
          int8 pwhite = FiguresCounter::s_whiteColors_[ep];
          if ( pwhite && fmgr_.bishops_w(lose_color) || !pwhite && fmgr_.bishops_b(lose_color) > 0 || y < 5 )
            weight = 10;
        }
        else if ( color_ == win_color )
          weight = Figure::figureWeight_[Figure::TypePawn];
        weight += y << 1;
        eval_pawns = false;
      }
    }
    else if ( fmgr_.queens(win_color) == 0 && fmgr_.bishops(win_color) == 0 &&
              fmgr_.knights(win_color) == 0 && fmgr_.pawns(win_color) == 0 && fmgr_.rooks(win_color) == 1 &&
              fmgr_.knights(lose_color)+fmgr_.bishops(lose_color) > 0 )
    {
      if ( fmgr_.knights(lose_color)+fmgr_.bishops(lose_color) == 1 )
        weight = fmgr_.weight();
      else
        weight = 15;
    }

    if ( fmgr_.pawns(win_color) == 1 )
    {
      uint64 pwmsk = fmgr_.pawn_mask_o(win_color);
      int pp = clear_lsb(pwmsk);
      THROW_IF( (unsigned)pp > 63, "no pawn found" );

      int ykl = king_pos_l.y();
      int ykw = king_pos_w.y();

      int xkl = king_pos_l.x();
      int xkw = king_pos_w.x();

      int x = pp & 7;
      int y = pp >> 3;
      int y_under = y;
      if ( win_color )
        y_under++;
      else
        y_under--;
      int pp_under = (x | (y_under << 3)) & 63;

      if ( !win_color )
      {
        y = 7-y;
        ykl = 7-ykl;
        ykw = 7-ykw;
      }

      int pr_pos = x | (win_color ? A8 : A1);
      Figure::Color pr_color = (Figure::Color)FiguresCounter::s_whiteColors_[pr_pos];

      int pr_moves = 7-y;

      int wk_pr_dist = g_distanceCounter->getDistance(king_pos_w, pr_pos);
      int lk_pr_dist = g_distanceCounter->getDistance(king_pos_l, pr_pos);

      int wdist = g_distanceCounter->getDistance(king_pos_w, pp);
      int ldist = g_distanceCounter->getDistance(king_pos_l, pp);

      int wudist = g_distanceCounter->getDistance(king_pos_w, pp_under);
      int ludist = g_distanceCounter->getDistance(king_pos_l, pp_under);

      int xwdist = xkw > x ? xkw-x : x-xkw;
      int xldist = xkl > x ? xkl-x : x-xkl;

      // special case KPK
      if ( (fmgr_.weight(win_color) == Figure::figureWeight_[Figure::TypePawn] && fmgr_.weight(lose_color) == 0) )
      {
        bool almost_draw = false;
        if ( x == 0 || x == 7 )
        {
          if ( (lk_pr_dist + ludist <= wk_pr_dist + wudist) ||
               ((pr_moves >= lk_pr_dist && y > 1 || pr_moves > lk_pr_dist && y == 1) && wudist >= ludist) )
            almost_draw = true;
        }
        else
        {
          if ( (pr_moves >= lk_pr_dist && y > 1 || pr_moves > lk_pr_dist && y == 1) &&
               ((wudist > ludist || wudist == ludist && lose_color == color_) && y >= ykw || (wudist > ludist+1 && y < ykw)) )
            almost_draw = true;
        }

        if ( almost_draw )
        {
          weight = 30 + (y<<1);
          eval_pawns = false;
        }
      }
      // KPBK. bishop color differs from promotion field color
      else if ( ( fmgr_.rooks(win_color) == 0 && fmgr_.queens(win_color) == 0 && fmgr_.knights(win_color) == 0 && fmgr_.bishops(win_color) && (x == 0 || x == 7) &&
                 (!fmgr_.bishops_w(win_color) && pr_color || !fmgr_.bishops_b(win_color) && !pr_color) ) )
      {
        if ( (pr_moves > lk_pr_dist && lk_pr_dist <= wk_pr_dist) || (lk_pr_dist < 2 && pr_moves > 0) )
        {
          weight = 30 + (y<<1);
          eval_pawns = false;
        }
      }

      // opponent's king should be as far as possible from my pawn
      weight -= (7-ldist);

      // my king should be as near as possible to my pawn
      weight -= wdist;
    }
    else
    {
      int dist  = g_distanceCounter->getDistance(king_pos_w, king_pos_l);
      weight -= dist << 1;
      weight -= Figure::positionEvaluation(1, lose_color, Figure::TypeKing, king_pos_l);
    }
  }

  if ( win_color == Figure::ColorBlack )
    weight = -weight;

  if ( eval_pawns )
  {
    if ( fmgr_.pawns(Figure::ColorBlack) > 0 )
      weight -= evaluatePawns(Figure::ColorBlack);

    if ( fmgr_.pawns(Figure::ColorWhite) > 0 )
      weight += evaluatePawns(Figure::ColorWhite);
  }

  return weight;
}
