/*************************************************************
  Evaluator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Evaluator.h"
#include "Board.h"
#include "HashTable.h"

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

const ScoreType Evaluator::positionGain_ = 100;
const ScoreType Evaluator::mobilityGain_ = 100;


const ScoreType Evaluator::positionEvaluations_[2][8][64] = {
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
      -8,  -12, -12, -16, -16, -12, -12,  -8,
      -4,  -8,   -8,  -8,  -8,  -8,  -8,  -4,
       5,   5,    0,   0,   0,   0,   5,   5,
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
const ScoreType Evaluator::bishopKnightMat_[64] =
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

const ScoreType Evaluator::pawnDoubled_  = -15;
const ScoreType Evaluator::pawnIsolated_ = -15;
const ScoreType Evaluator::pawnBackward_ = -10;
const ScoreType Evaluator::pawnDisconnected_ = -5;
const ScoreType Evaluator::pawnBlocked_ = 0;
const ScoreType Evaluator::assistantBishop_ = 8;
const ScoreType Evaluator::rookBehindPenalty_ = 7;
const ScoreType Evaluator::semiopenRook_ =  6;
const ScoreType Evaluator::winloseBonus_ =  25;
const ScoreType Evaluator::kingbishopPressure_ = 10;
const ScoreType Evaluator::bishopBonus_ = 15;
const ScoreType Evaluator::figureAgainstPawnBonus_ = 20;
const ScoreType Evaluator::rookAgainstFigureBonus_ = 30;
const ScoreType Evaluator::pawnEndgameBonus_ = 20;
const ScoreType Evaluator::fakecastlePenalty_ = 20;
const ScoreType Evaluator::castleImpossiblePenalty_ = 20;
const ScoreType Evaluator::unstoppablePawn_ = 60;
const ScoreType Evaluator::blockedKingPenalty_ = 20;
const ScoreType Evaluator::attackedByWeakBonus_ = 10;
const ScoreType Evaluator::forkBonus_ = 50;
const ScoreType Evaluator::fianchettoBonus_ = 5;

const ScoreType Evaluator::rookToKingBonus_ = 7;

// pawns shield
const ScoreType Evaluator::cf_columnOpened_ = 6;
const ScoreType Evaluator::bg_columnOpened_ = 15;
const ScoreType Evaluator::ah_columnOpened_ = 12;

const ScoreType Evaluator::cf_columnSemiopened_ = 3;
const ScoreType Evaluator::bg_columnSemiopened_ = 7;
const ScoreType Evaluator::ah_columnSemiopened_ = 6;

// pressure to king by opponents pawn
const ScoreType Evaluator::opponentPawnsToKing_ = 10;

#define MAX_PASSED_SCORE 80

const ScoreType Evaluator::pawnPassed_[2][8] = {
  { 0, MAX_PASSED_SCORE, 60, 40, 20, 12, 5, 0 },
  { 0, 5, 10, 20, 40, 60, MAX_PASSED_SCORE, 0 }
};

const ScoreType Evaluator::pawnGuarded_[2][8] = {
  { 0, 15, 12, 10, 6, 4, 0, 0 },
  { 0, 0, 4, 6, 10, 12, 15, 0 },
};

const ScoreType Evaluator::mobilityBonus_[8][32] = {
  {},
  {},
  {-18, -8, 0, 3, 5, 7, 9, 11},
  {-15, -8, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4},
  {-15, -7, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4},
  {-35, -25, -10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12},
};

const ScoreType Evaluator::kingDistanceBonus_[8][8] = {
  {},
  {},
  {10, 9, 8, 7, 6, 1, 0, 0},
  {12, 10, 9, 7, 5, 3, 1, 0},
  {20, 18, 13, 9, 7, 3, 1, 0},
  {35, 35, 35, 25, 12, 3, 1, 0},
};
const ScoreType Evaluator::nearKingAttackBonus_[8] = {
  0, 10, 20, 30, 40, 50, 60, 70
};


//////////////////////////////////////////////////////////////////////////
Evaluator::Evaluator() :
  board_(0), ehash_(0), alpha_(-ScoreMax), betta_(-ScoreMax)
{
  weightMax_ = 2*(Figure::figureWeight_[Figure::TypeQueen] +
    2*Figure::figureWeight_[Figure::TypeRook] + 2*Figure::figureWeight_[Figure::TypeBishop] + 2*Figure::figureWeight_[Figure::TypeKnight]);
}

void Evaluator::initialize(const Board * board, EHashTable * ehash)
{
  board_ = board;
  ehash_ = ehash;
}

void Evaluator::prepare()
{
  mask_all_ = board_->fmgr().mask(Figure::ColorWhite) | board_->fmgr().mask(Figure::ColorBlack);
  inv_mask_all_ = ~mask_all_;

  finfo_[0].reset();
  finfo_[1].reset();

  finfo_[0].king_pos_ = board_->kingPos(Figure::ColorBlack);
  finfo_[1].king_pos_ = board_->kingPos(Figure::ColorWhite);

  alpha_ = -ScoreMax;
  betta_ = +ScoreMax;
}


ScoreType Evaluator::operator () (ScoreType alpha, ScoreType betta)
{
  THROW_IF( !board_, "Evaluator wasn't properly initialized" );

  if ( board_->matState() )
    return -Figure::MatScore;
  else if ( board_->drawState() )
    return Figure::DrawScore;

  prepare();

  // for lazy evaluation
  if ( alpha > -Figure::MatScore )
    alpha_ = alpha-mobilityGain_;
  if ( betta < +Figure::MatScore )
    betta_ = betta+mobilityGain_;

  ScoreType score = -ScoreMax;

  if ( board_->isWinnerLoser() )
    score = evaluateWinnerLoser();
  else
    score = evaluate();

  THROW_IF( score <= -ScoreMax || score >= ScoreMax, "invalid score" );

  return score;
}
//////////////////////////////////////////////////////////////////////////
ScoreType Evaluator::evaluate()
{
  const FiguresManager & fmgr = board_->fmgr();

  // 1. evaluate common features
  ScoreType score = fmgr.weight();

  score += evaluateMaterialDiff();

  calculatePawnsAndKnights();

  score -= finfo_[0].knightPressure_;
  score += finfo_[1].knightPressure_;

  score -= finfo_[0].knightMobility_;
  score += finfo_[1].knightMobility_;

  score -= evaluateForks(Figure::ColorBlack);
  score += evaluateForks(Figure::ColorWhite);

  // take pawns eval. from hash if possible
  ScoreType pwscore = -ScoreMax, pwscore_eg = -ScoreMax, score_ps = -ScoreMax;
  hashedEvaluation(pwscore, pwscore_eg, score_ps);

  score += pwscore;

  // 2. determine game phase (opening, middle or end game)
  int coef_o = 0, coef_e = 0;
  GamePhase phase = detectPhase(coef_o, coef_e);

  ScoreType score_o = 0, score_e = 0;

  // opening part
  if ( phase != EndGame )
  {
    // PSQ - evaluation
    score_o -= fmgr.eval(Figure::ColorBlack, 0);
    score_o += fmgr.eval(Figure::ColorWhite, 0);

    score_o -= evaluateRooks(Figure::ColorBlack);
    score_o += evaluateRooks(Figure::ColorWhite);

    ScoreType score_king = evaluateCastlePenalty(Figure::ColorWhite) - evaluateCastlePenalty(Figure::ColorBlack);
    score_king += score_ps;
    score_o += score_king;

    score_o += evaluateFianchetto();
  }

  if ( phase != Opening )
  {
    score_e -= fmgr.eval(Figure::ColorBlack, 1);
    score_e += fmgr.eval(Figure::ColorWhite, 1);

    score_e += pwscore_eg;
  }

  if ( phase == Opening )
    score += score_o;
  else if ( phase == EndGame )
    score += score_e;
  else // middle game
    score = score + (score_o*coef_o + score_e*coef_e) / weightMax_;

  // consider current move side
  if ( Figure::ColorBlack  == board_->getColor() )
    score = -score;

  /// use lazy evaluation
  if ( score < alpha_ || score > betta_ )
    return score;


  ScoreType score_add = 0;

  // most expensive part
  calculateMobility();

  score_add -= evaluatePasserAdditional(Figure::ColorBlack);
  score_add += evaluatePasserAdditional(Figure::ColorWhite);

  score_add += finfo_[1].mobilityBonus_ - finfo_[0].mobilityBonus_;
  score_add += finfo_[1].kingPressureBonus_ - finfo_[0].kingPressureBonus_;

  if ( Figure::ColorBlack  == board_->getColor() )
    score_add = -score_add;

  score += score_add;

  return score;
}

Evaluator::GamePhase Evaluator::detectPhase(int & coef_o, int & coef_e)
{
  const FiguresManager & fmgr = board_->fmgr();

  int wei[2] = {0, 0};
  for (int c = 0; c < 2; ++c)
  {
    for (int t = Figure::TypeKnight; t < Figure::TypeKing; ++t)
      wei[c] += fmgr.tcount((Figure::Type)t, (Figure::Color)c)*Figure::figureWeight_[t];
  }

  GamePhase phase = MiddleGame;

  if ( wei[0] > 2*Figure::figureWeight_[Figure::TypeQueen] &&
       wei[1] > 2*Figure::figureWeight_[Figure::TypeQueen] )
  {
    phase = Opening;
  }
  else if ( wei[0] < Figure::figureWeight_[Figure::TypeQueen] &&
            wei[1] < Figure::figureWeight_[Figure::TypeQueen] )
  {
    phase = EndGame;
  }

  coef_o = wei[0] + wei[1];
  if ( coef_o > weightMax_ )
    coef_o = weightMax_;
  coef_e = weightMax_ - coef_o;

  return phase;
}

void Evaluator::hashedEvaluation(ScoreType & pwscore, ScoreType & pwscore_eg, ScoreType & score_ps)
{
  if ( ehash_ )
  {
    const HEval * heval = ehash_->find(board_->pawnCode());
    if ( heval )
    {
      pwscore = heval->score_;
      pwscore_eg = heval->score_eg_;
      score_ps = heval->score_ps_;
    }
  }

  if ( pwscore == -ScoreMax || pwscore_eg == -ScoreMax || score_ps == -ScoreMax )
  {
    ScoreType pwscore_eg_b = 0, pwscore_eg_w = 0;
    ScoreType pwscore_b = 0, pwscore_w = 0;
    ScoreType score_ps_b = 0, score_ps_w = 0;

    pwscore_b = evaluatePawns(Figure::ColorBlack, &pwscore_eg_b);
    pwscore_w = evaluatePawns(Figure::ColorWhite, &pwscore_eg_w);

    score_ps_b = evaluatePawnShield(Figure::ColorBlack);
    score_ps_w = evaluatePawnShield(Figure::ColorWhite);

    pwscore = pwscore_w - pwscore_b;
    pwscore_eg = pwscore_eg_w - pwscore_eg_b;
    score_ps = score_ps_w - score_ps_b;

    if ( ehash_ )
      ehash_->push(board_->pawnCode(), pwscore, pwscore_eg, score_ps);
  }
}

ScoreType Evaluator::evaluateFianchetto() const
{
  ScoreType score = 0;

  const Field & fb2 = board_->getField(B2);
  const Field & fb3 = board_->getField(B3);

  const Field & fg2 = board_->getField(G2);
  const Field & fg3 = board_->getField(G3);

  const Field & fb7 = board_->getField(B7);
  const Field & fb6 = board_->getField(B6);

  const Field & fg7 = board_->getField(G7);
  const Field & fg6 = board_->getField(G6);

  // white
  if ( fb3.color() && fb3.type() == Figure::TypePawn && fb2.color() && fb2.type() == Figure::TypeBishop )
    score += fianchettoBonus_;

  if ( fg3.color() && fg3.type() == Figure::TypePawn && fg2.color() && fg2.type() == Figure::TypeBishop )
    score += fianchettoBonus_;

  // black
  if ( !fb6.color() && fb6.type() == Figure::TypePawn && !fb7.color() && fb7.type() == Figure::TypeBishop )
    score -= fianchettoBonus_;

  if ( !fg6.color() && fg6.type() == Figure::TypePawn && !fg7.color() && fg7.type() == Figure::TypeBishop )
    score -= fianchettoBonus_;

  return score;
}


ScoreType Evaluator::evaluateForks(Figure::Color color)
{
  Figure::Color ocolor = Figure::otherColor(color);

  BitMask rq_mask = board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor);
  BitMask o_mask = board_->fmgr().knight_mask(ocolor) | board_->fmgr().bishop_mask(ocolor) | rq_mask;

  BitMask pawn_fork = o_mask & finfo_[color].pawn_attacked_;
  BitMask kn_fork = rq_mask & finfo_[color].kn_caps_;

  int pawnsN = pop_count(pawn_fork);
  int knightsN = pop_count(kn_fork);

  if ( pawnsN + knightsN > 1 )
    return forkBonus_;
  else if ( pawnsN + knightsN > 0 )
    return ((color == board_->getColor()) ? attackedByWeakBonus_ : (attackedByWeakBonus_>>1));

  return 0;
}

void Evaluator::calculatePawnsAndKnights()
{
  // 1. Pawns
  {
    const BitMask & pawn_msk_w = board_->fmgr().pawn_mask_o(Figure::ColorWhite);
    const BitMask & pawn_msk_b = board_->fmgr().pawn_mask_o(Figure::ColorBlack);

    finfo_[Figure::ColorWhite].pawn_attacked_ = ((pawn_msk_w << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_w << 7) & Figure::pawnCutoffMasks_[1]);
    finfo_[Figure::ColorBlack].pawn_attacked_ = ((pawn_msk_b >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_b >> 9) & Figure::pawnCutoffMasks_[1]);
  }

  // 2. Knights
  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color = (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor(color);
    BitMask not_occupied = ~finfo_[ocolor].pawn_attacked_ & inv_mask_all_;
    const int & oki_pos = finfo_[ocolor].king_pos_;

    BitMask kn_mask = board_->fmgr().knight_mask(color);
    for ( ; kn_mask; )
    {
      int from = clear_lsb(kn_mask);
      const BitMask & kn_cap = board_->g_movesTable->caps(Figure::TypeKnight, from);
      finfo_[c].kn_caps_ |= kn_cap;

      int ki_dist = board_->g_distanceCounter->getDistance(from, oki_pos);
      finfo_[c].knightPressure_ += kingDistanceBonus_[Figure::TypeKnight][ki_dist];

      BitMask kmob_mask = kn_cap & not_occupied;
      int movesN = pop_count(kmob_mask);
      finfo_[c].knightMobility_ += mobilityBonus_[Figure::TypeKnight][movesN];
    }
  }
}

void Evaluator::calculateMobility()
{
  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color = (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor(color);
    BitMask not_occupied = ~finfo_[ocolor].pawn_attacked_ & inv_mask_all_;
    const int & oki_pos = finfo_[ocolor].king_pos_;

    // 1. Bishops
    BitMask bi_mask = board_->fmgr().bishop_mask(color);
    for ( ; bi_mask; )
    {
      int from = clear_lsb(bi_mask);

      const BitMask & di_mask_nw = board_->g_betweenMasks->from_dir(from, nst::nw);
      const BitMask & di_mask_ne = board_->g_betweenMasks->from_dir(from, nst::ne);
      const BitMask & di_mask_se = board_->g_betweenMasks->from_dir(from, nst::se);
      const BitMask & di_mask_sw = board_->g_betweenMasks->from_dir(from, nst::sw);

      BitMask bmob_mask = 0;

      BitMask mask_from = di_mask_nw & mask_all_;
      bmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_lsb(mask_from)) : di_mask_nw);

      mask_from = di_mask_ne & mask_all_;
      bmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_lsb(mask_from)) : di_mask_ne);

      mask_from = di_mask_se & mask_all_;
      bmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_msb(mask_from)) : di_mask_se);

      mask_from = di_mask_sw & mask_all_;
      bmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_msb(mask_from)) : di_mask_sw);

      bmob_mask &= not_occupied;

      int ki_dist = board_->g_distanceCounter->getDistance(from, oki_pos);
      finfo_[c].kingPressureBonus_ = kingDistanceBonus_[Figure::TypeBishop][ki_dist];

      int movesN = pop_count(bmob_mask);
      finfo_[c].mobilityBonus_ += mobilityBonus_[Figure::TypeBishop][movesN];
    }

    // 2. Rooks
    BitMask ro_mask = board_->fmgr().rook_mask(color);
    for ( ; ro_mask; )
    {
      int from = clear_lsb(ro_mask);
      BitMask r_mob = board_->g_movesTable->rook_mobility(from) & inv_mask_all_;
      int movesN = pop_count(r_mob);

      int ki_dist = board_->g_distanceCounter->getDistance(from, oki_pos);
      finfo_[c].kingPressureBonus_ = kingDistanceBonus_[Figure::TypeRook][ki_dist];
      finfo_[c].mobilityBonus_ += mobilityBonus_[Figure::TypeRook][movesN];
    }

    // 3. Queens
    BitMask q_mask = board_->fmgr().queen_mask(color);
    for ( ; q_mask; )
    {
      int from = clear_lsb(q_mask);
      BitMask q_mob = board_->g_movesTable->caps(Figure::TypeKing, from) & inv_mask_all_;
      int movesN = pop_count(q_mob);

      int ki_dist = board_->g_distanceCounter->getDistance(from, oki_pos);
      finfo_[c].kingPressureBonus_ = kingDistanceBonus_[Figure::TypeQueen][ki_dist];
      finfo_[c].mobilityBonus_ += mobilityBonus_[Figure::TypeQueen][movesN];
    }
  }
}

ScoreType Evaluator::evaluateMaterialDiff()
{
  ScoreType score = 0;

  const FiguresManager & fmgr = board_->fmgr();

  Figure::Color color = board_->getColor();
  Figure::Color ocolor = Figure::otherColor(color);
  
  // 1. Knight - Bishop disbalance
  score = (fmgr.bishops(Figure::ColorWhite) - fmgr.bishops(Figure::ColorBlack))*bishopBonus_;

  // 2. Knight or Bishop against 3 pawns
  int figuresDiff = (fmgr.bishops(Figure::ColorWhite)+fmgr.knights(Figure::ColorWhite)) -
    (fmgr.bishops(Figure::ColorBlack)+fmgr.knights(Figure::ColorBlack));

  int pawnsDiff  = fmgr.pawns(Figure::ColorWhite) - fmgr.pawns(Figure::ColorBlack);
  int rooksDiff  = fmgr.rooks(Figure::ColorWhite) - fmgr.rooks(Figure::ColorBlack);
  int queensDiff = fmgr.queens(Figure::ColorWhite) - fmgr.queens(Figure::ColorBlack);

  if ( figuresDiff*pawnsDiff < 0 && !rooksDiff && !queensDiff )
    score += figuresDiff * figureAgainstPawnBonus_;

  // 3. Knight|Bishop+2Pawns vs. Rook
  else if ( !queensDiff && rooksDiff*figuresDiff == -1 )
    score += rooksDiff * rookAgainstFigureBonus_;

  return score;
}

int Evaluator::getCastleType(Figure::Color color) const
{
  Index ki_pos(finfo_[color].king_pos_);

  // short
  if ( ki_pos.x() > 4 && (ki_pos.y() < 2 && color || ki_pos.y() > 5 && !color) )
    return 0;

  // long
  if ( ki_pos.x() < 3 && (ki_pos.y() < 2 && color || ki_pos.y() > 5 && !color) )
    return 1;

  return -1;
}

ScoreType Evaluator::evaluatePawnShield(Figure::Color color)
{
  const FiguresManager & fmgr = board_->fmgr();

  ScoreType score = 0;
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
  Index ki_pos(finfo_[color].king_pos_);

  int ctype = getCastleType(color);
  if ( ctype < 0 )
    return 0;

  const BitMask & pw_mask = fmgr.pawn_mask_o(color);
  const BitMask & opw_mask = fmgr.pawn_mask_o(ocolor);
  const BitMask & obishop_mask = fmgr.bishop_mask(ocolor);

  // castle type, column number (abc, hgf)
  static const BitMask abc_masks_full[2][3] = {
    {0x8080808080808080ULL, 0x4040404040404040ULL, 0x2020202020202020ULL},
    {0x0101010101010101ULL, 0x0202020202020202ULL, 0x0404040404040404ULL} };

  // color, castle type, column number
  static const BitMask abc_mask_two[2][2][3] = {
    { { 0x0080800000000000ULL, 0x0040400000000000ULL, 0x0020200000000000ULL }, { 0x0001010000000000ULL, 0x0002020000000000ULL, 0x0004040000000000ULL } },
    { { 0x0000000000808000ULL, 0x0000000000404000ULL, 0x0000000000202000ULL }, { 0x0000000000010100ULL, 0x0000000000020200ULL, 0x0000000000040400ULL } }
  };

  // first 2 lines empty, full line empty
  static const ScoreType kingPenalties[2][3] = {
    {ah_columnOpened_, bg_columnOpened_, cf_columnOpened_},
    {ah_columnSemiopened_, bg_columnSemiopened_, cf_columnSemiopened_}
  };

  // color, castle type
  static const BitMask opponent_pressure_masks[2][2] = {
    {set_mask_bit(F3)|set_mask_bit(H3), set_mask_bit(C3)|set_mask_bit(A3)},
    {set_mask_bit(F6)|set_mask_bit(H6), set_mask_bit(C6)|set_mask_bit(A6)}
  };

  // pawn shield ab, gh columns
  for (int i = 0; i < 2; ++i)
  {
    const BitMask & full_mask = abc_masks_full[ctype][i];
    const BitMask & two_mask  = abc_mask_two[color][ctype][i];

    if ( !(full_mask & pw_mask) )
      score -= kingPenalties[0][i];
    else if ( !(two_mask & pw_mask) )
      score -= kingPenalties[1][i];
  }

  // pawn shield cf columns
  {
    const BitMask & full_mask = abc_masks_full[ctype][2];
    const BitMask & two_mask  = abc_mask_two[color][ctype][2];

    if ( !(full_mask & pw_mask) )
      score -= kingPenalties[0][2];
    else if ( !(two_mask & pw_mask) )
      score -= kingPenalties[1][2];
  }

  // opponent pawns pressure
  if ( opponent_pressure_masks[color][ctype] & opw_mask )
    score -= opponentPawnsToKing_;

  return score;
}

ScoreType Evaluator::evaluateCastlePenalty(Figure::Color color)
{
  const FiguresManager & fmgr = board_->fmgr();

  ScoreType score = 0;
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
  Index ki_pos(finfo_[color].king_pos_);

  static const BitMask fake_castle_rook[2][3] = {
    { set_mask_bit(G8)|set_mask_bit(H8) | set_mask_bit(G7)|set_mask_bit(H7),
      set_mask_bit(A8)|set_mask_bit(B8)|set_mask_bit(C8) | set_mask_bit(A7)|set_mask_bit(B7)|set_mask_bit(C7)},

    { set_mask_bit(G1)|set_mask_bit(H1) | set_mask_bit(G2)|set_mask_bit(H2),
      set_mask_bit(A1)|set_mask_bit(B1)|set_mask_bit(C1) | set_mask_bit(A2)|set_mask_bit(B2)|set_mask_bit(C2)} };

  int ctype = getCastleType(color);
  if ( ctype < 0 && !board_->castling(color) )
    return -castleImpossiblePenalty_;

  // fake castle
  if ( !board_->castling(color) && ctype >= 0 )
  {
    BitMask r_mask = fmgr.rook_mask(color) & fake_castle_rook[color][ctype];
    if ( r_mask )
    {
      Index r_pos( find_lsb(r_mask) );
      if ( ctype == 0 && r_pos.x() > ki_pos.x() || ctype == 1 && r_pos.x() < ki_pos.x() )
        score = -fakecastlePenalty_;
    }
  }

  if ( ctype < 0 )
    return score;

  const BitMask & obishop_mask = fmgr.bishop_mask(ocolor);

  // color, castle type
  static const BitMask opponent_pressure_masks[2][2] = {
    {set_mask_bit(F3)|set_mask_bit(H3), set_mask_bit(C3)|set_mask_bit(A3)},
    {set_mask_bit(F6)|set_mask_bit(H6), set_mask_bit(C6)|set_mask_bit(A6)}
  };

  // opponent bishop pressure
  if ( opponent_pressure_masks[color][ctype] & obishop_mask )
    score -= kingbishopPressure_;

  return score;
}


ScoreType Evaluator::evaluateRooks(Figure::Color color)
{
  const FiguresManager & fmgr = board_->fmgr();

	uint64 rook_mask = fmgr.rook_mask(color);
	if ( !rook_mask )
		return 0;

	Figure::Color ocolor = Figure::otherColor(color);
	const uint8 * pmsk  = (const uint8*)&fmgr.pawn_mask_t(color);
  const uint8 * opmsk = (const uint8*)&fmgr.pawn_mask_t(ocolor);

  int ok_pos = board_->kingPos(ocolor);
  int okx = ok_pos & 7;
  int oky = ok_pos >>3;

	ScoreType score = 0;
	for ( ; rook_mask; )
	{
		int n = clear_lsb(rook_mask);

		THROW_IF( (unsigned)n > 63, "invalid rook index" );
		THROW_IF( board_->getField(n).color() != color || board_->getField(n).type() != Figure::TypeRook,
      "there should be rook on given field" );

		int x = n & 7;
    int y = n >>3;

    // no pawns of some color
    if ( !opmsk[x] || !pmsk[x] )
    {
      score += semiopenRook_;

      // near opponent's king
      if ( x == okx || x == okx-1 || x == okx+1 )
        score += rookToKingBonus_;
    }

    // no pawns at all
    if ( !(opmsk[x] | pmsk[x]) )
      score += semiopenRook_;

    if ( y == oky || y == oky-1 || y == oky+1 )
      score += rookToKingBonus_;
	}

	return score;
}

ScoreType Evaluator::evaluatePawns(Figure::Color color, ScoreType * score_eg)
{
  const FiguresManager & fmgr = board_->fmgr();
  const BitMask & pmsk_t = fmgr.pawn_mask_t(color);
  if ( !pmsk_t )
    return 0;

  if ( score_eg )
    *score_eg = fmgr.pawns(color)*pawnEndgameBonus_;

  static int promo_y[] = { 0, 7 };
  static int delta_y[] = { -1, +1 };

  ScoreType score = 0;
  Figure::Color ocolor = Figure::otherColor(color);
  const BitMask & opmsk_t = fmgr.pawn_mask_t(ocolor);
  const BitMask & opmsk_o = fmgr.pawn_mask_o(ocolor);

  BitMask attacked_mask = (finfo_[ocolor].pawn_attacked_ & ~finfo_[color].pawn_attacked_);

  int py = promo_y[color];

  // columns, occupied by pawns
  uint8 cols_visited = 0;

  BitMask pawn_mask = fmgr.pawn_mask_o(color);
  for ( ; pawn_mask; )
  {
    int n = clear_lsb(pawn_mask);

    int x = n & 7;
    int y = n >>3;

    // doubled pawns
    if ( !(cols_visited & (1<<x)) )
    {
      cols_visited |= 1<<x;
      uint8 & column = ((uint8*)&pmsk_t)[x];
      const ScoreType dblNum = BitsCounter::numBitsInByte(column);
      if ( dblNum > 1 )
        score += pawnDoubled_*(dblNum-1);
    }

    const uint64 & passmsk = board_->g_pawnMasks->mask_passed(color, n);
    const uint64 & blckmsk = board_->g_pawnMasks->mask_blocked(color, n);

    // passed pawn evaluation
    if ( !(opmsk_t & passmsk) && !(pmsk_t & blckmsk) )
    {
      int y = n >> 3;
      score += pawnPassed_[color][y];

      // guarded by neighbor pawn
      const BitMask & guardmsk = board_->g_pawnMasks->mask_guarded(color, n);
      if ( pmsk_t & guardmsk )
        score += pawnGuarded_[color][y];

      int promo_pos = x | (py<<3);

      if ( score_eg )
      {
        int pawn_dist_promo = py - y;
        if ( pawn_dist_promo < 0 )
          pawn_dist_promo = -pawn_dist_promo;

        int o_dist_promo = board_->g_distanceCounter->getDistance(finfo_[ocolor].king_pos_, promo_pos);
        if ( board_->color_ == ocolor )
          o_dist_promo--;

        if ( pawn_dist_promo < o_dist_promo )
          *score_eg += unstoppablePawn_;
        else
          *score_eg += o_dist_promo<<1;

        // give penalty for long distance to my pawns if opponent doesn't have any
        if ( !opmsk_o )
        {
          int dist_promo = board_->g_distanceCounter->getDistance(finfo_[color].king_pos_, promo_pos);
          *score_eg -= dist_promo<<1;
        }
      }
    }
    else if ( score_eg )
    {
      int o_dist = board_->g_distanceCounter->getDistance(finfo_[ocolor].king_pos_, n);
      *score_eg += o_dist;

      if ( !opmsk_o )
      {
        int dist = board_->g_distanceCounter->getDistance(finfo_[color].king_pos_, n);
        *score_eg -= dist;
      }
    }

    const BitMask & isomask = board_->g_pawnMasks->mask_isolated(n & 7);
    const BitMask & dismask = board_->g_pawnMasks->mask_disconnected(n);

    // maybe isolated pawn
    if ( !(pmsk_t & isomask) )
      score += pawnIsolated_;

    // backward - ie pawn isn't on last line and not guarded by other pawn and can't safely go to the next line
    if ( (color && y < 6 || !color && y > 1) && !(finfo_[color].pawn_attacked_ & set_mask_bit(n)) )
    {
      int to = x | ((y+delta_y[color]) << 3);
      BitMask to_mask = set_mask_bit(to);

      if ( to_mask & opmsk_o )
        score += pawnBlocked_;
      else if ( to_mask & attacked_mask )
        score += pawnBackward_;
    }

    // disconnected
    if ( !(pmsk_t & dismask) )
      score += pawnDisconnected_;
  }

  return score;
}

ScoreType Evaluator::evaluatePasserAdditional(Figure::Color color)
{
  const FiguresManager & fmgr = board_->fmgr();
  Figure::Color ocolor = Figure::otherColor(color);

  const BitMask & opmsk_t = fmgr.pawn_mask_t(ocolor);
  const BitMask & pmsk_t = fmgr.pawn_mask_t(color);
  if ( !pmsk_t )
    return 0;

  ScoreType score = 0;

  static int promo_y[] = { 0, 7 };
  int py = promo_y[color];

  BitMask pawn_mask = fmgr.pawn_mask_o(color);
  for ( ; pawn_mask; )
  {
    int n = clear_lsb(pawn_mask);

    int x = n & 7;
    int y = n >>3;

    const uint64 & passmsk = board_->g_pawnMasks->mask_passed(color, n);
    const uint64 & blckmsk = board_->g_pawnMasks->mask_blocked(color, n);

    if ( !(opmsk_t & passmsk) && !(pmsk_t & blckmsk) )
    {
      int promo_pos = x | (py<<3);

      // have bishop with color the same as promotion field's color
      Figure::Color pcolor = ((Figure::Color)FiguresCounter::s_whiteColors_[promo_pos]);
      if ( pcolor && fmgr.bishops_w(color) || !pcolor && fmgr.bishops_b(color) )
        score += assistantBishop_;
    }
  }

  return score;
}

ScoreType Evaluator::evaluateWinnerLoser()
{
  const FiguresManager & fmgr = board_->fmgr();

  Figure::Color win_color = board_->can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  ScoreType score = fmgr.weight(win_color);

  // bonus for pawns
  score += fmgr.pawns(win_color)*pawnEndgameBonus_;

  Index king_pos_w = board_->kingPos(win_color);
  Index king_pos_l = board_->kingPos(lose_color);

  bool eval_pawns = true;

  if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) > 0 )
  {
    int num_lose_figs = fmgr.knights(lose_color) + fmgr.bishops(lose_color);
    ScoreType weight_lose_fig = 10;

    // if winner doesn't have light figure and loser has more than 1 figure we don't want to evaluate them less than a pawn
    if ( fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 1 && fmgr.knights(win_color) +fmgr.bishops(win_color) == 0 )
      weight_lose_fig = Figure::figureWeight_[Figure::TypePawn] + (MAX_PASSED_SCORE) + pawnEndgameBonus_;

    // if winner has more pawns than loser and also has some figure he must exchange all loser figures to pawns
    else if ( fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 0  &&
         fmgr.knights(win_color) +fmgr.bishops(win_color) > 0 &&
         fmgr.knights(lose_color)+fmgr.bishops(lose_color) < fmgr.pawns(win_color) )
    {
      weight_lose_fig = Figure::figureWeight_[Figure::TypePawn] + (MAX_PASSED_SCORE) + pawnEndgameBonus_;
      eval_pawns = false;
    }

    score -= num_lose_figs * weight_lose_fig;
  }
  else
    score -= fmgr.weight(lose_color);

  // add small bonus for winner-loser state to force it
  score += winloseBonus_;

  // BN-mat case
  if ( fmgr.weight(lose_color) == 0 && fmgr.knights(win_color) == 1 && fmgr.bishops(win_color) == 1 &&
       fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 0 )
  {
    int dist  = board_->g_distanceCounter->getDistance(king_pos_w, king_pos_l);
    score -= dist;

    int kp = king_pos_l;
    if ( fmgr.bishops_w(win_color) )
    {
      int kx = king_pos_l.x();
      int ky = king_pos_l.y();
      kp = ((7-ky)<<3)| kx;
    }
    score += bishopKnightMat_[kp];

    uint64 n_mask = fmgr.knight_mask(win_color);
    int np = clear_lsb(n_mask);
    THROW_IF( (unsigned)np > 63, "no knigt found" );
    int ndist = board_->g_distanceCounter->getDistance(np, king_pos_l);
    score -= ndist >> 1;

    // add more bonus to be sure that we go to this state
    score += winloseBonus_;
  }
  else
  {
    // some special almost-draw cases
    if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 0 &&
         fmgr.weight(win_color)-fmgr.weight(lose_color) < Figure::figureWeight_[Figure::TypeBishop]+Figure::figureWeight_[Figure::TypeKnight] )
    {
      score = 10;
    }
    else if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 1 &&
              fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 0 )
    {
      if ( fmgr.knights(win_color)+fmgr.bishops(win_color) <= fmgr.knights(lose_color)+fmgr.bishops(lose_color) )
      {
        score = (MAX_PASSED_SCORE);
        uint64 pwmask = fmgr.pawn_mask_o(win_color);
        int pp = clear_lsb(pwmask);
        int x = pp & 7;
        int y = pp >> 3;
        if ( !win_color )
          y = 7-y;
        if ( y < 6 )
        {
          int ep = x | (win_color ? A8 : A1);
          int8 pwhite = FiguresCounter::s_whiteColors_[ep];
          if ( pwhite && fmgr.bishops_w(lose_color) || !pwhite && fmgr.bishops_b(lose_color) > 0 || y < 5 )
            score = 10;
        }
        else if ( board_->color_ == win_color )
          score = Figure::figureWeight_[Figure::TypePawn];
        score += y << 1;
        eval_pawns = false;
      }
    }
    else if ( fmgr.queens(win_color) == 0 && fmgr.bishops(win_color) == 0 &&
              fmgr.knights(win_color) == 0 && fmgr.pawns(win_color) == 0 && fmgr.rooks(win_color) == 1 &&
              fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 1 )
    {
      score = 25;
    }

    if ( fmgr.pawns(win_color) == 1 )
    {
      uint64 pwmsk = fmgr.pawn_mask_o(win_color);
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

      int wk_pr_dist = board_->g_distanceCounter->getDistance(king_pos_w, pr_pos);
      int lk_pr_dist = board_->g_distanceCounter->getDistance(king_pos_l, pr_pos);

      int wdist = board_->g_distanceCounter->getDistance(king_pos_w, pp);
      int ldist = board_->g_distanceCounter->getDistance(king_pos_l, pp);

      int wudist = board_->g_distanceCounter->getDistance(king_pos_w, pp_under);
      int ludist = board_->g_distanceCounter->getDistance(king_pos_l, pp_under);

      int xwdist = xkw > x ? xkw-x : x-xkw;
      int xldist = xkl > x ? xkl-x : x-xkl;

      // special case KPK
      if ( (fmgr.weight(win_color) == Figure::figureWeight_[Figure::TypePawn] && fmgr.weight(lose_color) == 0) )
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
               ( (wudist > ludist || wudist == ludist && lose_color == board_->color_) && y >= ykw ||
                 (wudist > ludist+1 && y < ykw) ) )
            almost_draw = true;
        }

        if ( almost_draw )
        {
          score = 30 + (y<<1);
          eval_pawns = false;
        }
      }
      // KPBK. bishop color differs from promotion field color
      else if ( ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.knights(win_color) == 0 &&
                  fmgr.bishops(win_color) && (x == 0 || x == 7) &&
                 (!fmgr.bishops_w(win_color) && pr_color || !fmgr.bishops_b(win_color) && !pr_color) ) )
      {
        if ( (pr_moves > lk_pr_dist && lk_pr_dist <= wk_pr_dist) || (lk_pr_dist < 2 && pr_moves > 0) )
        {
          score = 30 + (y<<1);
          eval_pawns = false;
        }
      }

      // opponent's king should be as far as possible from my pawn
      score -= (7-ldist);

      // my king should be as near as possible to my pawn
      score -= wdist;
    }
    else
    {
      int dist  = board_->g_distanceCounter->getDistance(king_pos_w, king_pos_l);
      score -= dist << 1;
      score -= positionEvaluation(1, lose_color, Figure::TypeKing, king_pos_l);
    }
  }

  if ( win_color == Figure::ColorBlack )
    score = -score;

  if ( eval_pawns )
  {
    ScoreType pwscore = -ScoreMax, pwscore_eg = -ScoreMax, score_ps = -ScoreMax;
    hashedEvaluation(pwscore, pwscore_eg, score_ps);

    score -= evaluatePasserAdditional(Figure::ColorBlack);
    score += evaluatePasserAdditional(Figure::ColorWhite);

    score += pwscore;
  }

  if ( Figure::ColorBlack  == board_->getColor() )
    score = -score;

  return score;
}


//////////////////////////////////////////////////////////////////////////
//inline ScoreType Evaluator::evalKingPawns(Figure::Color color, int index, int coeff, int castle)
//{
//  const FiguresManager & fmgr = board_->fmgr();
//
//  ScoreType kingEval = 0;
//  static int8 pawns_x[2][4] = { {5, 6, 7, -1}, {2, 1, 0, -1} };// for left/right castle
//  static uint8 pmask_king[2] = { 96, 6 };
//  static uint8 opmask_king[2] = { 48, 6 };
//  static uint8 shifts[2] = { 5, 1 };
//  static uint8 oshifts[2] = { 4, 2 };
//  static ScoreType king_penalties[2][4] = { {10, 2, 0, 0}, {10, 0, 2, 0} };
//  static ScoreType king_o_penalties[2][4] = { {0, 5, 10, 10}, {0, 10, 5, 10} };
//
//  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
//
//  const uint8 * pmsk  = (const uint8*)&fmgr.pawn_mask_t((Figure::Color)color);
//  const uint8 * opmsk = (const uint8*)&fmgr.pawn_mask_t(ocolor);
//
//  int x = pawns_x[castle][index];
//  int m = ((pmsk[x] & pmask_king[color]) >> shifts[color]) & 3;
//  kingEval -= ((king_penalties[color][m])<<(coeff));
//  int o = ((opmsk[x] & opmask_king[color]) >> oshifts[color]) & 3;
//  kingEval -= king_o_penalties[color][o];
//
//  return kingEval;
//}

//ScoreType Evaluator::evaluateKing(Figure::Color color)
//{
//  const FiguresManager & fmgr = board_.fmgr();
//
//  ScoreType kingEval = 0;
//  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
//
//  Index ki_pos(/*board_.kingPos(color)*/finfo_[color].king_pos_);
//
//  static int8 castle_mask[8] = { 2,2,2, 0,0, 1,1,1 }; // 1 - short (K); 2 - long (Q)
//  int8 castle = castle_mask[ki_pos.x()]; // determine by king's x-position
//  int8 ky = ki_pos.y();
//  bool bCastle = castle && !(ky > 1 && color) && !(ky < 6 && !color);
//  if ( !bCastle )
//    return kingEval;
//
//  castle--;
//
//  static int8 pawns_x[2][4] = { {5, 6, 7, -1}, {2, 1, 0, -1} };// for left/right castle
//  static uint8 pmask_king[2] = { 96, 6 };
//  static uint8 opmask_king[2] = { 48, 6 };
//  static uint8 shifts[2] = { 5, 1 };
//  static uint8 oshifts[2] = { 4, 2 };
//  static ScoreType king_penalties[2][4] = { {8, 1, 0, 0}, {8, 0, 1, 0} };
//  static ScoreType king_o_penalties[2][4] = { {0, 5, 10, 10}, {0, 10, 5, 10} };
//
//  const uint8 * pmsk  = (const uint8*)&fmgr.pawn_mask_t((Figure::Color)color);
//  const uint8 * opmsk = (const uint8*)&fmgr.pawn_mask_t(ocolor);
//  
//  kingEval += evalKingPawns(color, 0, 0, castle);
//  kingEval += evalKingPawns(color, 1, 2, castle);
//  kingEval += evalKingPawns(color, 2, 2, castle);
//
//  // penalty for fake castle
//  int ry = color ? A1 : A8;
//  int rp = pawns_x[castle][1], kp = pawns_x[castle][2];
//  rp = rp | ry;
//  kp = kp | ry;
//  const Field & rfield = board_.getField(rp);
//  const Field & kfield = board_.getField(kp);
//  if ( kfield.type() == Figure::TypeRook ||
//      (rfield.type() == Figure::TypeRook && kfield.type() != Figure::TypeKing) )
//  {
//    kingEval -= fakecastlePenalty_;
//  }
//
//  // opponent bishop near king
//  int bp0 = 0, bp1 = 0;
//  if ( color )
//  {
//    if ( castle )
//    {
//      bp0 = A3;
//      bp1 = C3;
//    }
//    else
//    {
//      bp0 = F3;
//      bp1 = H3;
//    }
//  }
//  else
//  {
//    if ( castle )
//    {
//      bp0 = A6;
//      bp1 = C6;
//    }
//    else
//    {
//      bp0 = F6;
//      bp1 = H6;
//    }
//  }
//
//  const Field & fb0 = board_.getField(bp0);
//  const Field & fb1 = board_.getField(bp1);
//
//  if ( fb0.type() == Figure::TypeBishop && fb0.color() == ocolor || fb1.type() == Figure::TypeBishop && fb1.color() == ocolor )
//    kingEval -= kingbishopPressure_;
//
//  // king is blocked
//  if ( !finfo_[color].kingMobility_ )
//  {
//    kingEval -= blockedKingPenalty_;
//  }
//
//  return kingEval;
//}
