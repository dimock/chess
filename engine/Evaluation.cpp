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

ScoreType Figure::pawnGuarded_  =  8;
ScoreType Figure::pawnDoubled_  = -8;
ScoreType Figure::pawnIsolated_ = -8;
ScoreType Figure::pawnBackward_ = -8;
ScoreType Figure::openRook_     =  8;
ScoreType Figure::winloseBonus_ =  0;
ScoreType Figure::kingbishopPressure_ = 8;
ScoreType Figure::fianchettoBonus_ = 4;
ScoreType Figure::queenMobilityBonus_[32] = { -8/* blocked */, -4/* immobile */, -2, 0, 1, 2, 3, 4, 5 };
ScoreType Figure::knightMobilityBonus_[16] = { -6 /* blocked */, -3 /* immobile */, 2, 3, 3, 4, 4, 4, 4 };
ScoreType Figure::bishopMobilityBonus_[16] = { -4 /* blocked */, -1 /* immobile */, 1, 2 };
ScoreType Figure::knightDistBonus_[8] = { 0, 2, 4, 3, 1, 0, 0, 0 };
ScoreType Figure::bishopDistBonus_[8] = { 0, 2, 4, 2, 1, 0, 0, 0 };
ScoreType Figure::fakecastlePenalty_ = 10;
ScoreType Figure::promoToKingPenalty_[8] = {10, 8, 7, 5, 3, 1, 0, 0};

#define MAX_PASSED_SCORE 60

ScoreType Figure::pawnPassed_[2][8] = {
  { 0, MAX_PASSED_SCORE, 40, 25, 15, 10, 5, 0 },
  { 0, 5, 10, 15, 25, 40, MAX_PASSED_SCORE, 0 }
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
    int16 col_msk = ((column|ocolumn)-1) >> 15;\
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

  static const ScoreType fweight_max = Figure::figureWeight_[Figure::TypeQueen] + 2*Figure::figureWeight_[Figure::TypeRook] + 2*Figure::figureWeight_[Figure::TypeBishop] + 2*Figure::figureWeight_[Figure::TypeKnight];

  ScoreType fweight_b = fmgr_.weight(Figure::ColorBlack) - fmgr_.pawns(Figure::ColorBlack)*Figure::figureWeight_[Figure::TypePawn];
  ScoreType fweight_w = fmgr_.weight(Figure::ColorWhite) - fmgr_.pawns(Figure::ColorWhite)*Figure::figureWeight_[Figure::TypePawn];

  score -= fmgr_.eval(Figure::ColorBlack, stages_[0]);
  score += fmgr_.eval(Figure::ColorWhite, stages_[1]);

  FiguresMobility fmob_b, fmob_w;
  score -= evaluateMobility(Figure::ColorBlack, fmob_b);
  score += evaluateMobility(Figure::ColorWhite, fmob_w);

  // king's safety
  if ( !stages_[Figure::ColorBlack] )
    score -= evaluateKing(Figure::ColorBlack, fmob_w)*fweight_w/fweight_max;

  if ( !stages_[Figure::ColorWhite] )
    score += evaluateKing(Figure::ColorWhite, fmob_b)*fweight_b/fweight_max;

  // king to pawns in endgame
  if ( stages_[Figure::ColorBlack] )
    score -= evalPawnsEndgame(Figure::ColorBlack);

  if ( stages_[Figure::ColorWhite] )
    score += evalPawnsEndgame(Figure::ColorWhite);


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

inline ScoreType Board::evaluateKing(Figure::Color color, const FiguresMobility & fmob /* opponent's color */) const
{
  ScoreType kingEval = 0;
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);

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
  static ScoreType king_o_penalties[2][4] = { {0, 5, 10, 10}, {0, 10, 5, 10} };

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

  // pressure to king from the opponent
  kingEval -= fmob.knightDist_ + fmob.bishopDist_;

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

    if ( !opmsk[x] && !pmsk[x]  )
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
  const uint64 & opmsk = fmgr_.pawn_mask_t(ocolor);
  if ( !opmsk )
    return 0;

  static ScoreType pwd_scores[8] = {6, 4, 2, 1, 0, 0, 0, 0};
  const Figure & king = getFigure(color, KingIndex);
  const Figure & oking = getFigure(ocolor, KingIndex);
  const uint64 & pmsk = fmgr_.pawn_mask_t(color);
  ScoreType score = 0;
  for (int i = 0; i < 8; ++i)
  {
    const Figure & pawn = getFigure(ocolor, i);
    if ( Figure::TypePawn != pawn.getType() )
      continue;

    int y = pawn.where() >> 3;
    int x = pawn.where() & 7;
    if ( !ocolor )
      y = 7 -y;

    int pr_pos = x | (ocolor ? A8 : A1);
    int pr_dist = g_distanceCounter->getDistance(king.where(), pr_pos);
    int opr_dist = g_distanceCounter->getDistance(oking.where(), pr_pos);
    int dist = g_distanceCounter->getDistance(king.where(), pawn.where());
    int odist = g_distanceCounter->getDistance(oking.where(), pawn.where());
    const uint64 & opassmsk = g_pawnMasks->mask_passed(ocolor, pawn.where());
    const uint64 & oblckmsk = g_pawnMasks->mask_blocked(ocolor, pawn.where());
    if ( !(pmsk & opassmsk) && !(opmsk & oblckmsk) )
    {
      score += Figure::promoToKingPenalty_[pr_dist & 7];
      score -= Figure::promoToKingPenalty_[opr_dist & 7];
      score += pwd_scores[dist & 7];
      score -= pwd_scores[odist & 7];
    }
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
      weight_lose_fig = Figure::figureWeight_[Figure::TypePawn] + (MAX_PASSED_SCORE) + 10;
      eval_pawns = false;
    }

    weight -= num_lose_figs * weight_lose_fig;
  }
  else
    weight -= fmgr_.weight(lose_color);

  // add small bonus for winner-loser state
  weight += Figure::winloseBonus_;

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
      int pp = least_bit_number(pwmask);
      int x = pp & 7;
      int y = pp >> 3;
      if ( !win_color )
        y = 7-y;
      if ( y < 6 )
      {
        int ep = x | (win_color ? 7 : 0);
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
    weight = 15;
  }

  if ( fmgr_.pawns(win_color) == 1 )
  {
    uint64 pwmsk = fmgr_.pawn_mask_o(win_color);
    int pp = least_bit_number(pwmsk);
    THROW_IF( (unsigned)pp > 63, "no pawn found" );

    int x = pp & 7;
    int y = pp >> 3;
    if ( !win_color )
      y = 7-y;

    int pr_pos = x | (win_color ? A8 : A1);
    Figure::Color pr_color = (Figure::Color)FiguresCounter::s_whiteColors_[pr_pos];

    int pr_moves = 7-y;
    if ( win_color == color_ )
      pr_moves--;

    int wk_pr_dist = g_distanceCounter->getDistance(king_w.where(), pr_pos);
    int lk_pr_dist = g_distanceCounter->getDistance(king_l.where(), pr_pos);

    int wdist = g_distanceCounter->getDistance(king_w.where(), pp);
    int ldist = g_distanceCounter->getDistance(king_l.where(), pp);

    // special case KPK
    if ( (fmgr_.weight(win_color) == Figure::figureWeight_[Figure::TypePawn] && fmgr_.weight(lose_color) == 0) )
    {
      const uint64 & pass_mask = g_pawnMasks->mask_kpk(win_color, pp);
      if ( (pass_mask & (1ULL << king_l.where())) && !(pass_mask & (1ULL << king_w.where())) ||
           (pr_moves <= lk_pr_dist && pr_moves > wk_pr_dist) )
      {
        weight = 20 + (y<<1);
      }
    }
    // KPBK. bishop color differs from promotion field color
    else if ( fmgr_.rooks(win_color) == 0 && fmgr_.queens(win_color) == 0 && fmgr_.knights(win_color) == 0 && fmgr_.bishops(win_color) == 1 && (x == 0 || x == 7) &&
              (fmgr_.bishops_b(win_color) && pr_color || fmgr_.bishops_w(win_color) && !pr_color) )
    {
      const uint64 & pass_mask = g_pawnMasks->mask_kpk(win_color, pp);
      if ( (pass_mask & (1ULL << king_l.where())) && !(pass_mask & (1ULL << king_w.where())) ||
           (pr_moves <= lk_pr_dist && pr_moves > wk_pr_dist) )
      {
        weight = 50;
      }
    }
    // opponent's king should be as far as possible from my pawn promo-field
    weight -= Figure::promoToKingPenalty_[lk_pr_dist & 7];

    // my king should be as near as possible to my pawn promo-field
    weight += Figure::promoToKingPenalty_[wk_pr_dist & 7];

    int yw = king_w.where() >> 3;
    int yl = king_l.where() >> 3;
    if ( !color_ )
    {
      yw = 7-yw;
      yl = 7-yl;
    }
    weight -= yl << 1;
    weight += yw << 1;
  }
  else if ( fmgr_.pawns(win_color) )
  {
    int yw = king_w.where() >> 3;
    int yl = king_l.where() >> 3;
    if ( !color_ )
    {
      yw = 7-yw;
      yl = 7-yl;
    }
    weight -= yl << 1;
    weight += yw << 1;

    uint64 pwmsk = fmgr_.pawn_mask_o(win_color);
    for (; pwmsk;)
    {
      int pp = least_bit_number(pwmsk);

      int x = pp & 7;
      int y = pp >> 3;
      if ( !win_color )
        y = 7-y;

      int pr_pos = x | (win_color ? A8 : A1);

      int wk_pr_dist = g_distanceCounter->getDistance(king_w.where(), pr_pos);
      int lk_pr_dist = g_distanceCounter->getDistance(king_l.where(), pr_pos);

      // opponent's king should be as far as possible from my pawn promo-field
      weight -= Figure::promoToKingPenalty_[lk_pr_dist & 7];

      // my king should be as near as possible to my pawn promo-field
      weight += Figure::promoToKingPenalty_[wk_pr_dist & 7];
    }
  }
  else
  {
    int dist  = g_distanceCounter->getDistance(king_w.where(), king_l.where());
    weight -= dist << 1;
    weight -= Figure::positionEvaluation(1, lose_color, Figure::TypeKing, king_l.where());
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

ScoreType Board::evaluateMobility(Figure::Color color, FiguresMobility & fmob) const
{
  Figure::Color ocolor = Figure::otherColor(color);
  const uint64 & opw_mask = fmgr_.pawn_mask_o(ocolor);
  const Figure & oking = getFigure(ocolor, KingIndex);

  // calculate fields, attacked by opponent's pawns
  uint64 opw_eat_msk = 0;
  if ( ocolor )
    opw_eat_msk = ((opw_mask << 9) & Figure::pawnCutoffMasks_[0]) | ((opw_mask << 7) & Figure::pawnCutoffMasks_[1]);
  else
    opw_eat_msk = ((opw_mask >> 7) & Figure::pawnCutoffMasks_[0]) | ((opw_mask >> 9) & Figure::pawnCutoffMasks_[1]);

  // calculate occupied fields mask
  const uint64 & black = fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = fmgr_.mask(Figure::ColorWhite);
  uint64 occupied_msk = ~(opw_eat_msk | black | white);

  const Figure & knight1 = getFigure(color, KnightIndex);
  if ( knight1 && knight1.getType() == Figure::TypeKnight )
  {
    int movesN = 0;
    int dist = g_distanceCounter->getDistance(knight1.where(), oking.where());
    const uint64 & kn_caps = g_movesTable->caps(Figure::TypeKnight, knight1.where());
    uint64 kn_go_msk = occupied_msk & kn_caps;
    movesN = pop_count( kn_go_msk );
    THROW_IF(movesN > 8, "invalid number of knight moves");
    fmob.knightMob_ += Figure::knightMobilityBonus_[movesN & 15];
    fmob.knightDist_ += Figure::knightDistBonus_[dist];
  }

  const Figure & knight2 = getFigure(color, KnightIndex+1);
  if ( knight2 && knight2.getType() == Figure::TypeKnight )
  {
    int movesN = 0;
    int dist = g_distanceCounter->getDistance(knight2.where(), oking.where());
    const uint64 & kn_caps = g_movesTable->caps(Figure::TypeKnight, knight2.where());
    uint64 kn_go_msk = occupied_msk & kn_caps;
    movesN = pop_count( kn_go_msk );
    THROW_IF(movesN > 8, "invalid number of knight moves");
    fmob.knightMob_ += Figure::knightMobilityBonus_[movesN & 15];
    fmob.knightDist_ += Figure::knightDistBonus_[dist];
  }

  const Figure & bishop1 = getFigure(color, BishopIndex);
  if ( bishop1 && bishop1.getType() == Figure::TypeBishop )
  {
    int movesN = 0;
    int dist = g_distanceCounter->getDistance(bishop1.where(), oking.where());
    const uint64 & bishop_mob = g_movesTable->bishop_mobility(bishop1.where());
    uint64 bishop_go_msk = occupied_msk & bishop_mob;
    movesN = pop_count( bishop_go_msk );
    THROW_IF(movesN > 8, "invalid number of bishop moves");
    fmob.bishopMob_ += Figure::bishopMobilityBonus_[movesN & 15];
    fmob.bishopDist_ += Figure::bishopDistBonus_[dist];
  }

  const Figure & bishop2 = getFigure(color, BishopIndex+1);
  if ( bishop2 && bishop2.getType() == Figure::TypeBishop )
  {
    int movesN = 0;
    int dist = g_distanceCounter->getDistance(bishop2.where(), oking.where());
    const uint64 & bishop_mob = g_movesTable->bishop_mobility(bishop2.where());
    uint64 bishop_go_msk = occupied_msk & bishop_mob;
    movesN = pop_count( bishop_go_msk );
    THROW_IF(movesN > 8, "invalid number of bishop moves");
    fmob.bishopMob_ += Figure::bishopMobilityBonus_[movesN & 15];
    fmob.bishopDist_ += Figure::bishopDistBonus_[dist];
  }

  const Figure & queen = getFigure(color, QueenIndex);
  if ( queen && queen.getType() == Figure::TypeQueen )
  {
    int movesN = 0;
    int dist = g_distanceCounter->getDistance(queen.where(), oking.where());
    // use king mask - test only neighbor fields !!!
    const uint64 & queen_mob = g_movesTable->caps(Figure::TypeKing, queen.where());
    uint64 queen_go_msk = occupied_msk & queen_mob;
    movesN = pop_count( queen_go_msk );
    THROW_IF(movesN > 8, "invalid number of bishop moves");
    fmob.queenMob_ += Figure::queenMobilityBonus_[movesN & 31];
  }

  return fmob.knightMob_ + fmob.bishopMob_ + fmob.queenMob_;
}
