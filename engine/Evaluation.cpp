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
       0,   0,   0,   0,   0,   0,   0,   0,
       1,   1,   1,   2,   2,   1,   1,   1,
       0,   0,   2,   2,   2,   2,   0,   0,
       0,   0,   2,  10,  10,   2,   0,   0,
       0,   0,   1,   2,   2,   1,   0,   0,
       2,   0,   2,  -8,  -8,   2,   0,   2,
       0,   0,   0,   0,   0,   0,   0,   0
    },

    // knight
    {
     -4,  -4,  -4,  -4,  -4,  -4,  -4, -4,
     -4,   0,   0,   0,   0,   0,   4, -4,
     -4,   0,   4,   6,   6,   4,   0, -4,
     -4,   2,   6,   8,   8,   6,   2, -4,
     -4,   0,   6,   8,   8,   6,   0, -4,
     -4,   2,   4,   6,   6,   4,   2, -4,
     -4,  -8,   0,   2,   2,   0,  -8, -4,
     -4, -12,  -4,  -4,  -4,  -4, -12, -4
    },

    // bishop
    {
      -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
      -2,   0,   0,   0,   0,   0,   0,  -2,
      -2,   0,   2,   4,   4,   2,   0,  -2,
      -2,   2,   2,   4,   4,   2,   2,  -2,
      -2,   0,   4,   4,   4,   4,   0,  -2,
      -2,   0,   4,   4,   4,   4,   0,  -2,
      -2,   2,   0,   0,   0,   0,   2,  -2,
      -4,  -4, -12,  -4,  -4, -12,  -4,  -4
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
       0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   0,   0,   0,   0,   0,   0,
       0,   0,   1,   2,   2,   1,   0,   0,
       0,   0,   2,   2,   2,   2,   0,   0,
       0,   0,   2,   2,   2,   2,   0,   0,
       0,   1,   2,   2,   2,   2,   1,   0,
      -4,   0,   2,   1,   1,   2,   0,  -4,
      -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4
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
     12,  12,  12,  12,  12,  12,  12,  12,
      8,   8,   8,   8,   8,   8,   8,   8,
      4,   4,   4,   4,   4,   4,   4,   4,
      2,   2,   2,   2,   2,   2,   2,   2,
      2,   1,   1,   1,   1,   1,   1,   1,
      2,   0,   0,   0,   0,   0,   0,   2,
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
ScoreType Figure::fianchettoBonus_ = 5;
ScoreType Figure::queenMobilityBonus_[32] = { -30/* blocked */, -5/* immobile */, 0 };
ScoreType Figure::knightMobilityBonus_[10] = { -20 /* blocked */, -5 /* immobile */, 5, 10, 10, 10, 10, 10, 10 };
ScoreType Figure::bishopMobilityBonus_[16] = { -10 /* blocked */, -4 /* immobile */, 0 };
ScoreType Figure::rookMobilityBonus_[16] = { -10 /* blocked */, -1 /* immobile */, 0};
ScoreType Figure::knightDistBonus_[8] = { 0, 4, 10, 6, 2, 0, 0, 0 };
ScoreType Figure::bishopDistBonus_[8] = { 0, 4, 4, 2, 2, 1, 0, 0 };
ScoreType Figure::rookDistBonus_[8] = {  0, 0, 8, 4, 2, 1, 0, 0 };
ScoreType Figure::queenDistBonus_[8] = {  0, 0, 16, 10, 4, 1, 0, 0 };
ScoreType Figure::queen2MeDistBonus_[8] = {  0, 3, 2, 1, 0, 0, 0, 0 };
ScoreType Figure::fakecastlePenalty_ = 8;

ScoreType Figure::pawnPassed_[2][8] = {
	{ 0, 80, 60, 40, 30, 10, 5, 0 },
	{ 0, 5, 10,  30, 40, 60, 80, 0 }
};

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
  //char fen[256];
  //toFEN(fen);

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
  evaluateMobility(Figure::ColorBlack, fmob_b);
  evaluateMobility(Figure::ColorWhite, fmob_w);

  score -= fmob_b.knightMob_ + fmob_b.bishopMob_ + fmob_b.rookMob_ + fmob_b.queenMob_;
  score += fmob_w.knightMob_ + fmob_w.bishopMob_ + fmob_w.rookMob_ + fmob_w.queenMob_;

  // black
  if ( stages[0] == 0 )
  {
    int king_score = evaluateKing(Figure::ColorBlack, fmob_w);
    king_score += fmob_b.queen2MeDist_;
    score -= (ScoreType)(king_score * fweight_w / fweight_max);
  }

  // white
  if ( stages[1] == 0 )
  {
    int king_score = evaluateKing(Figure::ColorWhite, fmob_b);
    king_score += fmob_w.queen2MeDist_;
    score += (ScoreType)(king_score * fweight_b / fweight_max);
  }

  if ( fmgr_.pawns(Figure::ColorBlack) > 0 )
    score -= evaluatePawns(Figure::ColorBlack);

  if ( fmgr_.pawns(Figure::ColorWhite) > 0 )
    score += evaluatePawns(Figure::ColorWhite);

  if ( fmgr_.rooks(Figure::ColorBlack) > 0 )
	  score -= evaluateRooks(Figure::ColorBlack);

  if ( fmgr_.rooks(Figure::ColorWhite) > 0 )
	  score += evaluateRooks(Figure::ColorWhite);

  return score;
}

ScoreType Board::evaluateKing(Figure::Color color, const FiguresMobility & fmob /* opponent's color */) const
{
  Figure::Color ocolor = Figure::otherColor(color);
  const Figure & king = getFigure(color, KingIndex);

  int kx = king.where() & 7;
  int ky = king.where() >>3;

  if ( (color && ky > 1) || (!color && ky < 6) )
    return 0;

  ScoreType score = 0;

  const uint8 * pw_mask = (const uint8*)&fmgr_.pawn_mask_t(color);

  int iA2,iA3, iB2,iB3, iC2,iC3, iF2,iF3, iG2,iG3, iH2,iH3;
  int iA1,iB1, iG1,iH1;
  if ( color )
  {
    iA2 = A2;
    iA3 = A3;
    iB2 = B2;
    iB3 = B3;
    iC2 = C2;
    iC3 = C3;
    iF2 = F2;
    iF3 = F3;
    iG2 = G2;
    iG3 = G3;
    iH2 = H2;
    iH3 = H3;
    iA1 = A1;
    iB1 = B1;
    iG1 = G1;
    iH1 = H1;
  }
  else
  {
    iA2 = A7;
    iA3 = A6;
    iB2 = B7;
    iB3 = B6;
    iC2 = C7;
    iC3 = C6;
    iF2 = F7;
    iF3 = F6;
    iG2 = G7;
    iG3 = G6;
    iH2 = H7;
    iH3 = H6;
    iA1 = A8;
    iB1 = B8;
    iG1 = G8;
    iH1 = H8;
  }

  // queen side castle
  if ( kx < 3 )
  {
    const Field & fa2 = getField(iA2);
    const Field & fa3 = getField(iA3);

    const Field & fb2 = getField(iB2);
    const Field & fb3 = getField(iB3);

    const Field & fc2 = getField(iC2);
    const Field & fc3 = getField(iC3);

    if ( fa2.color() == color && fa2.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[0];
    else if ( fa3.color() == color && fa3.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[1];
    else if ( !pw_mask[0] )
      score += Figure::kingpawnsBonus_[2]<<1;

    // opponent's pawn near
    if ( (fa2.type() == Figure::TypePawn && fa2.color() != color) || (fa3.type() == Figure::TypePawn && fa3.color() != color) )
      score += Figure::kingpawnsBonus_[3];

    if ( fb2.color() == color && fb2.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[0];
    else if ( fb3.color() == color && fb3.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[1];
    else if ( !pw_mask[1] )
      score += Figure::kingpawnsBonus_[2]<<1;

    if ( (fb2.type() == Figure::TypePawn && fb2.color() != color) || (fb3.type() == Figure::TypePawn && fb3.color() != color) )
      score += Figure::kingpawnsBonus_[3];

    if ( fc2.color() == color && fc2.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[0];
    else if ( fc3.color() == color && fc3.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[1];
    else if ( !pw_mask[2] )
      score += Figure::kingpawnsBonus_[2];

    if ( (fc2.type() == Figure::TypePawn && fc2.color() != color) || (fc3.type() == Figure::TypePawn && fc3.color() != color) )
      score += Figure::kingpawnsBonus_[3];

    // fianchetto
    if ( fb3.color() == color && fb3.type() == Figure::TypePawn )
    {
      if ( fb2.color() == color && fb2.type() == Figure::TypeBishop )
        score += Figure::fianchettoBonus_;
      else
      {
        if ( (color && fmgr_.bishops_b(ocolor)) || (!color && fmgr_.bishops_w(ocolor)) )
          score -= Figure::fianchettoBonus_;
      }
    }

    // fake castle
    const Field & fa1 = getField(iA1);
    const Field & fb1 = getField(iB1);
    if ( fa1.type() == Figure::TypeRook || fb1.type() == Figure::TypeRook )
      score -= Figure::fakecastlePenalty_;
  }
  // king side castle
  else if ( kx > 4 )
  {
    const Field & ff2 = getField(iF2);
    const Field & ff3 = getField(iF3);

    const Field & fg2 = getField(iG2);
    const Field & fg3 = getField(iG3);

    const Field & fh2 = getField(iH2);
    const Field & fh3 = getField(iH3);

    if ( ff2.color() == color && ff2.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[0];
    else if ( ff3.color() == color && ff3.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[1];
    else if ( !pw_mask[5] )
      score += Figure::kingpawnsBonus_[2];

    if ( (ff2.type() == Figure::TypePawn && ff2.color() != color) || (ff3.type() == Figure::TypePawn && ff3.color() != color) )
      score += Figure::kingpawnsBonus_[3];

    if ( fg2.color() == color && fg2.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[0];
    else if ( fg3.color() == color && fg3.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[1];
    else if ( !pw_mask[6] )
      score += Figure::kingpawnsBonus_[2]<<1;

    if ( (fg2.type() == Figure::TypePawn && fg2.color() != color) || (fg3.type() == Figure::TypePawn && fg3.color() != color) )
      score += Figure::kingpawnsBonus_[3];

    if ( fh2.color() == color && fh2.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[0];
    else if ( fh3.color() == color && fh3.type() == Figure::TypePawn )
      score += Figure::kingpawnsBonus_[1];
    else if ( !pw_mask[7] )
      score += Figure::kingpawnsBonus_[2]<<1;

    if ( (fh2.type() == Figure::TypePawn && fh2.color() != color) || (fh3.type() == Figure::TypePawn && fh3.color() != color) )
      score += Figure::kingpawnsBonus_[3];

    // fianchetto
    if ( fg3.color() == color && fg3.type() == Figure::TypePawn )
    {
      if ( fg2.color() == color && fg2.type() == Figure::TypeBishop )
        score += Figure::fianchettoBonus_;
      else
      {
        if ( (color && fmgr_.bishops_w(ocolor)) || (!color && fmgr_.bishops_b(ocolor)) )
          score -= Figure::fianchettoBonus_;
      }
    }

    // fake castle
    const Field & fg1 = getField(iG1);
    const Field & fh1 = getField(iH1);
    if ( fg1.type() == Figure::TypeRook || fh1.type() == Figure::TypeRook )
      score -= Figure::fakecastlePenalty_;
  }

  // pressing to king from opponent

  score -= (fmob.knightDist_ + fmob.bishopDist_ + fmob.rookDist_ + fmob.queenDist_);

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
  int8 rfields[64] = {-1, -1};
  int num = 0;

	for ( ; rook_mask; )
	{
		int n = least_bit_number(rook_mask);
    rfields[num++] = n;

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

ScoreType Board::evaluateWinnerLoser() const
{
  Figure::Color win_color = can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  ScoreType weight = fmgr_.weight(win_color);

  const Figure & king_w = getFigure(win_color, KingIndex);
  const Figure & king_l = getFigure(lose_color, KingIndex);

  if ( fmgr_.pawns(win_color) == 1 )
  {
    // king should be as far as possible from my pawn
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

void Board::evaluateMobility(Figure::Color color, FiguresMobility & fmob) const
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
          fmob.queen2MeDist_ += Figure::queen2MeDistBonus_[dist2me];
        }
      }
      break;
    }
  }
}
