#include "Figure.h"
#include "Board.h"

// TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing
ScoreType Figure::figureWeight_[7] = { 0, 100, 320, 330, 500, 950, 0 };

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
       2,   2,   2,   2,   2,   2,   2,   2,
       8,   8,   8,   8,   8,   8,   8,   8,
      -4,   0,   0,   0,   0,   0,   0,  -4,
      -4,   0,   0,   0,   0,   0,   0,  -4,
      -4,   0,   0,   0,   0,   0,   0,  -4,
      -4,   0,   0,   0,   0,   0,   0,  -4,
      -4,   0,   0,   0,   0,   0,   0,  -4,
      -8,  -4,   0,   2,   2,   0,  -4,  -8
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
     -20, -20, -20, -20, -20, -20, -20, -20,
     -20, -20, -20, -20, -20, -20, -20, -20,
     -20, -20, -20, -20, -20, -20, -20, -20,
     -20, -20, -20, -20, -20, -20, -20, -20,
     -20, -20, -20, -20, -20, -20, -20, -20,
     -20, -20, -20, -20, -20, -20, -20, -20,
     -12, -12, -12, -12, -12, -12, -12, -12,
       0,  12,  16, -10,   0, -10,  20,  16
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

ScoreType Figure::pawnGuarded_  =  10;
ScoreType Figure::pawnDoubled_  = -12;
ScoreType Figure::pawnIsolated_ = -12;
ScoreType Figure::pawnBackward_ = -10;
ScoreType Figure::openRook_     =   5;
ScoreType Figure::semiopenRook_ =   5;
ScoreType Figure::winloseBonus_ =  50;
ScoreType Figure::kingpawnsBonus_[4] = {12, 4, -12, -12};
ScoreType Figure::fianchettoBonus_ = 4;
ScoreType Figure::queenMobilityBonus_[32] = { -10/* blocked */, -6/* immobile */, 0 };
ScoreType Figure::knightMobilityBonus_[10] = { -8 /* blocked */, -4 /* immobile */, 3, 4, 4, 4, 4, 4, 4 };
ScoreType Figure::bishopMobilityBonus_[16] = { -4 /* blocked */, -4 /* immobile */, 0 };
ScoreType Figure::rookMobilityBonus_[16] = { -4 /* blocked */, -1 /* immobile */, 0};
ScoreType Figure::knightDistBonus_[8] = { 0, 4, 10, 6, 2, 0, 0, 0 };
ScoreType Figure::bishopDistBonus_[8] = { 0, 4, 4, 2, 2, 1, 0, 0 };
ScoreType Figure::rookDistBonus_[8] = {  0, 0, 8, 4, 2, 1, 0, 0 };
ScoreType Figure::queenDistBonus_[8] = {  0, 0, 16, 10, 4, 1, 0, 0 };
ScoreType Figure::fakecastlePenalty_ = 12;

ScoreType Figure::pawnPassed_[2][8] = {
	{ 0, 60, 40, 25, 15, 10, 5, 0 },
	{ 0, 5, 10, 15, 25, 40, 60, 0 }
};

#define FAST_ROOK_PAWN_EVAL

//////////////////////////////////////////////////////////////////////////
#define EVALUATE_OPEN_ROOKS(clr, score)\
{\
  uint64 rook_mask = fmgr_.rook_mask(clr);\
  Figure::Color oclr = Figure::otherColor(clr);\
  const uint8 * pmsk  = (const uint8*)&fmgr_.pawn_mask_t(clr);\
  const uint8 * opmsk = (const uint8*)&fmgr_.pawn_mask_t(oclr);\
  for ( ; rook_mask; )\
  {\
    int n = least_bit_number(rook_mask);\
    THROW_IF( (unsigned)n > 63, "invalid rook index" );\
    THROW_IF( getField(n).color() != clr || getField(n).type() != Figure::TypeRook, "there should be rook on given field" );\
    int x = n & 7;\
    int16 column = (int16)(pmsk[x]);\
    int16 ocolumn = (int16)(opmsk[x]);\
    int16 col_msk = (column-1) >> 15;\
    int16 ocol_msk = (ocolumn-1) >> 15;\
    score += Figure::semiopenRook_ & ocol_msk;\
    score += Figure::openRook_ & col_msk;\
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

#define EVALUATE_KING_PAWNS(index)\
{\
  int x = pawns_x[castle][index];\
  int m = ((pmsk[x] & pmask_king[color]) >> shifts[color]) & 3;\
  kingEval -= king_penalties[color][m];\
  int o = ((opmsk[x] & opmask_king[color]) >> oshifts[color]) & 3;\
  kingEval -= king_o_penalties[color][o];\
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

  THROW_IF( score < -32760 || score > 32760, "invalid score" );


  return score;
}

ScoreType Board::calculateEval() const
{
  ScoreType score = fmgr_.weight();

  // 1 means endgame
  // if there is lack of opponent's material
  int stages[2] = {0, 0};

  static const ScoreType fweight_max = Figure::figureWeight_[Figure::TypeQueen] + 2*Figure::figureWeight_[Figure::TypeRook] + 2*Figure::figureWeight_[Figure::TypeBishop] + 2*Figure::figureWeight_[Figure::TypeKnight];

  ScoreType fweight_b = fmgr_.weight(Figure::ColorBlack) - fmgr_.pawns(Figure::ColorBlack)*Figure::figureWeight_[Figure::TypePawn];
  ScoreType fweight_w = fmgr_.weight(Figure::ColorWhite) - fmgr_.pawns(Figure::ColorWhite)*Figure::figureWeight_[Figure::TypePawn];

  // endgame for black
  if ( (fweight_w < Figure::figureWeight_[Figure::TypeQueen]+Figure::figureWeight_[Figure::TypeRook] && !fmgr_.queens(Figure::ColorWhite)) ||
       (fweight_w < Figure::figureWeight_[Figure::TypeQueen]+Figure::figureWeight_[Figure::TypeKnight]) )
    stages[0] = 1;

  // endgame for white
  if ( (fweight_b < Figure::figureWeight_[Figure::TypeQueen]+Figure::figureWeight_[Figure::TypeRook] && !fmgr_.queens(Figure::ColorBlack)) ||
       (fweight_b < Figure::figureWeight_[Figure::TypeQueen]+Figure::figureWeight_[Figure::TypeKnight]) )
    stages[1] = 1;

  score -= fmgr_.eval(Figure::ColorBlack, stages[0]);
  score += fmgr_.eval(Figure::ColorWhite, stages[1]);

  FiguresMobility fmob_b, fmob_w;
  //score -= evaluateMobility(Figure::ColorBlack, fmob_b);
  //score += evaluateMobility(Figure::ColorWhite, fmob_w);

  // king's safety
  if ( !stages[0] )
    score -= evaluateKing(Figure::ColorBlack, fmob_w, stages[0])*fweight_w/fweight_max;

  if ( !stages[1] )
    score += evaluateKing(Figure::ColorWhite, fmob_b, stages[1])*fweight_b/fweight_max;

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
    score -= score0;
  }

  if ( fmgr_.rooks(Figure::ColorWhite) )
  {
    ScoreType score1 = 0;
    EVALUATE_OPEN_ROOKS(Figure::ColorWhite, score1);
    score += score1;
  }
#else
  if ( fmgr_.pawns() > 0 )
  {
    score -= evaluatePawns(Figure::ColorBlack);
    score += evaluatePawns(Figure::ColorWhite);
  }

  {
	  score -= evaluateRooks(Figure::ColorBlack);
	  score += evaluateRooks(Figure::ColorWhite);
  }
#endif

  return score;
}

inline ScoreType Board::evaluateKing(Figure::Color color, const FiguresMobility & fmob /* opponent's color */, int stage) const
{
  ScoreType kingEval = 0;
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);

  if ( stage > 0 )
  {
    if ( fmgr_.pawns(ocolor) )
      kingEval = evalPawnsEndgame((Figure::Color)color);

    return kingEval;
  }

  const Figure & queen = getFigure((Figure::Color)color, QueenIndex);
  const Figure & king = getFigure((Figure::Color)color, KingIndex);
  if ( queen )
  {
    const Figure & oking = getFigure(ocolor, KingIndex);
    int dist  = g_distanceCounter->getDistance(queen.where(), king.where());
    int odist = g_distanceCounter->getDistance(queen.where(), oking.where());
    kingEval += (7 - odist) << 1;
    kingEval += (7 - dist);
  }

  static int8 castle_mask[8] = { 2,2,2, 0,0, 1,1,1 }; // 1 - short (K); 2 - long (Q)
  int8 castle = castle_mask[king.where() & 7]; // determine by king's x-position
  int8 ky = king.where() >> 3;
  bool bCastle = castle && !(ky > 1 && color) && !(ky < 6 && !color);
  if ( !bCastle )
    return kingEval;

  castle--;

  static int8 pawns_x[2][4] = { {5, 6, 7, -1}, {2, 1, 0, -1} };// for left/right castle
  static uint8 pmask_king[2] = { 96, 6 };
  static uint8 opmask_king[2] = { 48, 6 };
  static uint8 shifts[2] = { 5, 1 };
  static uint8 oshifts[2] = { 4, 2 };
  static ScoreType king_penalties[2][4] = { {10, 3, 0, 0}, {10, 0, 3, 0} };
  static ScoreType king_o_penalties[2][4] = { {0, 3, 10, 12}, {0, 10, 3, 12} };

  const uint8 * pmsk  = (const uint8*)&fmgr_.pawn_mask_t((Figure::Color)color);
  const uint8 * opmsk = (const uint8*)&fmgr_.pawn_mask_t(ocolor);
  
  int ry = color ? A1 : A8;

  EVALUATE_KING_PAWNS(0);
  EVALUATE_KING_PAWNS(1);
  EVALUATE_KING_PAWNS(2);

  // penalty for fake castle
  int rp = pawns_x[castle][1], kp = pawns_x[castle][2];
  rp = rp | ry;
  kp = kp | ry;
  const Field & rfield = getField(rp);
  const Field & kfield = getField(kp);
  if ( kfield.type() == Figure::TypeRook ||
      (rfield.type() == Figure::TypeRook && kfield.type() != Figure::TypeKing) )
    kingEval -= Figure::fakecastlePenalty_;

  // pressure to king from the opponent
  kingEval -= fmob.knightDist_ + fmob.bishopDist_ + fmob.rookDist_ + fmob.queenDist_;

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

	ScoreType score = 0;
	for ( ; rook_mask; )
	{
		int n = least_bit_number(rook_mask);

		THROW_IF( (unsigned)n > 63, "invalid rook index" );
		THROW_IF( getField(n).color() != color || getField(n).type() != Figure::TypeRook, "there should be rook on given field" );

		int x = n & 7;

    if ( !opmsk[x] )
    {
      score += Figure::semiopenRook_;
      if ( !pmsk[x] )
        score += Figure::openRook_;
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

        // guarded by neighbor pawn
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
    int16 dblNum = BitsCounter::numBitsInByte(column);
    ScoreType dblMask = ~((dblNum-1) >> 15);
    weight -= ((dblNum-1) << 3) & dblMask;
  }

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


ScoreType Board::evaluateWinnerLoser() const
{
  Figure::Color win_color = can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  ScoreType weight = fmgr_.weight(win_color);

  const Figure & king_w = getFigure(win_color, KingIndex);
  const Figure & king_l = getFigure(lose_color, KingIndex);

  if ( fmgr_.pawns(win_color) == 1 )
  {
    // opponent's king should be as far as possible from my pawn
    uint64 msk = fmgr_.pawn_mask_o(win_color);
    int pp = least_bit_number(msk);
    THROW_IF( (unsigned)pp > 63, "no pawn found" );

    int dist = g_distanceCounter->getDistance(king_l.where(), pp);
    weight -= (7-dist) << 3;
  }
  else
  {
    int dist  = g_distanceCounter->getDistance(king_w.where(), king_l.where());
    weight -= dist << 3;
    weight -= Figure::positionEvaluation(1, lose_color, Figure::TypeKing, king_l.where());
  }

  if ( fmgr_.rooks(win_color) == 0 && fmgr_.queens(win_color) == 0 )
  {
    int num_figs = fmgr_.knights(lose_color) + fmgr_.bishops(lose_color);
    weight -= (num_figs<<3);
  }
  else
    weight -= fmgr_.weight(lose_color);

  // add small bonus for winner-loser state
  weight += Figure::winloseBonus_;

  if ( win_color == Figure::ColorBlack )
    weight = -weight;

  if ( fmgr_.pawns(Figure::ColorBlack) > 0 )
    weight -= evaluatePawns(Figure::ColorBlack);

  if ( fmgr_.pawns(Figure::ColorWhite) > 0 )
    weight += evaluatePawns(Figure::ColorWhite);

  return weight;
}

ScoreType Board::evaluateMobility(Figure::Color color, FiguresMobility & fmob) const
{
  Figure::Color ocolor = Figure::otherColor(color);
  const uint64 & opw_mask = fmgr_.pawn_mask_o(ocolor);

  const Figure & king = getFigure(color, KingIndex);
  const Figure & oking = getFigure(ocolor, KingIndex);

  // calculate fields, attacked by opponent's pawns
  uint64 opw_eat_msk = 0;
  if ( ocolor )
    opw_eat_msk = ((opw_mask << 9) & Figure::pawnCutoffMasks_[0]) | ((opw_mask << 7) & Figure::pawnCutoffMasks_[1]);
  else
    opw_eat_msk = ((opw_mask >> 7) & Figure::pawnCutoffMasks_[0]) | ((opw_mask >> 9) & Figure::pawnCutoffMasks_[1]);

  uint64 okn_msk = fmgr_.knight_mask(ocolor);
  uint64 okn_eat_msk = 0ULL;
  for ( ; okn_msk; )
  {
    int n = least_bit_number(okn_msk);
    const Field & kfield = getField(n);
    okn_eat_msk |= g_movesTable->caps(Figure::TypeKnight, n);
  }

  // calculate occupied fields mask
  const uint64 & black = fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = fmgr_.mask(Figure::ColorWhite);
  uint64 all_mask_inv = ~(black | white);
  uint64 o_brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);
  uint64 occupied_msk = opw_eat_msk | black | white;

  //uint64 occupied_msk = opw_eat_msk | fmgr_.mask(color);
  //int initial_balance = fmgr_.weight();
  //if ( !color )
  //  initial_balance = -initial_balance;

  for (int i = 0; i < KingIndex; ++i)
  {
    const Figure & fig = getFigure(color, i);
    if ( !fig || fig.getType() == Figure::TypePawn )
      continue;

    bool blocked = false;
    if ( fig.getType() != Figure::TypeQueen && fig.getType() != Figure::TypeRook && see_check(color, fig.where(), all_mask_inv, o_brq_mask) )
      blocked = true;

    int dist = g_distanceCounter->getDistance(fig.where(), oking.where());
    int dist2me = g_distanceCounter->getDistance(fig.where(), king.where());

    switch ( fig.getType() )
    {
    case Figure::TypeKnight:
      {
        int movesN = 0;
        if ( !blocked )
        {
          const uint64 & kn_caps = g_movesTable->caps(Figure::TypeKnight, fig.where());
          uint64 kn_go_msk = ~occupied_msk & kn_caps;
          movesN = pop_count( kn_go_msk );
          //for ( ; movesN < 2 && kn_go_msk; )
          //{
          //  int n = least_bit_number(kn_go_msk);
          //  const Field & tfield = getField(n);
          //  Move move;
          //  move.from_ = fig.where();
          //  move.to_ = n;
          //  if ( tfield && tfield.color() == ocolor )
          //    move.rindex_ = tfield.index();
          //  int gain = see_before(initial_balance, move);
          //  if ( gain >= 0 )
          //    movesN++;
          //}

          THROW_IF(movesN > 8, "invalid number of knight moves");
        }
        fmob.knightMob_ += Figure::knightMobilityBonus_[movesN];
        fmob.knightDist_ += Figure::knightDistBonus_[dist];
      }
      break;

    case Figure::TypeBishop:
    case Figure::TypeQueen:
    case Figure::TypeRook:
      {
        uint64 eat_msk = opw_eat_msk | okn_eat_msk;
        int movesN = 0;
        if ( !blocked )
        {
          const uint16 * table = g_movesTable->move(fig.getType()-Figure::TypeBishop, fig.where());
          for ( ; movesN < 2 && *table; ++table)
          {
            const int8 * packed = reinterpret_cast<const int8*>(table);
            int8 count = packed[0];
            int8 delta = packed[1];

            int8 p = fig.where();
            for ( ; movesN < 2 && count; --count)
            {
              p += delta;

              const Field & field = getField(p);
              if ( field /*&& field.color() == color*/ )
                break;

              // field is attacked by opponent's pawn | knight
              if ( (1ULL << p) & eat_msk )
                continue;

              //// can we go here
              //Move move;
              //move.from_ = fig.where();
              //move.to_ = p;
              //move.rindex_ = field.index();
              //int gain = see_before(initial_balance, move);
              //if ( gain >= 0 )
              //  movesN++;

              movesN++;

              //if ( field )
              //  break;
            }
          }
        }

        if ( fig.getType() == Figure::TypeBishop )
        {
          fmob.bishopMob_ += Figure::bishopMobilityBonus_[movesN];
          fmob.bishopDist_ += Figure::bishopDistBonus_[dist];
        }
        else if ( fig.getType() == Figure::TypeRook )
        {
          fmob.rookMob_ = Figure::rookMobilityBonus_[movesN];
          fmob.rookDist_ = Figure::rookDistBonus_[dist];
        }
        else if ( fig.getType() == Figure::TypeQueen )
        {
          fmob.queenMob_ = Figure::queenMobilityBonus_[movesN];
          fmob.queenDist_ += Figure::queenDistBonus_[dist];
        }
      }
      break;
    }
  }

  return fmob.knightMob_ + fmob.bishopMob_ + fmob.rookMob_ + fmob.queenMob_;
}
