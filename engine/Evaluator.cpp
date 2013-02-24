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

const ScoreType Evaluator::lazyThresholds_[3] = { 300, 300, 150 };

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
      7,   7,   7,   7,   7,   7,   7,   7,
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
      -16, -16, -16, -20, -20, -16, -16, -16,
      -16, -16, -16, -20, -20, -16, -16, -16,
      -16, -16, -16, -20, -20, -16, -16, -16,
      -16, -16, -16, -20, -20, -16, -16, -16,
      -8,  -12, -12, -16, -16, -12, -12,  -8,
      -4,  -8,   -8, -10, -10,  -8,  -8,  -4,
       0,   0,   -2,  -8,  -8,  -2,   0,   0,
       8,  12,    5,   0,   0,   5,  12,   8
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
const ScoreType Evaluator::pawnIsolated_ = -20;
const ScoreType Evaluator::pawnBackward_ = -10;
const ScoreType Evaluator::pawnDisconnected_ = -5;
const ScoreType Evaluator::pawnBlocked_ = 0;
const ScoreType Evaluator::assistantBishop_ = 6;
const ScoreType Evaluator::rookBehindBonus_ = 7;
const ScoreType Evaluator::semiopenRook_ =  10;
const ScoreType Evaluator::openRook_ =  10;
const ScoreType Evaluator::winloseBonus_ =  25;
const ScoreType Evaluator::bishopBonus_ = 10;
const ScoreType Evaluator::figureAgainstPawnBonus_ = 20;
const ScoreType Evaluator::rookAgainstFigureBonus_ = 30;
const ScoreType Evaluator::pawnEndgameBonus_ = 20;
const ScoreType Evaluator::unstoppablePawn_ = 50;
const ScoreType Evaluator::kingFarBonus_ = 20;
const ScoreType Evaluator::fakecastlePenalty_ = 20;
const ScoreType Evaluator::castleImpossiblePenalty_ = 20;
const ScoreType Evaluator::attackedByWeakBonus_ = 10;
const ScoreType Evaluator::forkBonus_ = 60;
const ScoreType Evaluator::fianchettoBonus_ = 6;
const ScoreType Evaluator::rookToKingBonus_ = 6;

const ScoreType Evaluator::pinnedKnight_ = 0;//-5;
const ScoreType Evaluator::pinnedBishop_ = 0;//-5;
const ScoreType Evaluator::pinnedRook_ = 0;//-5;

// pawns shield
const ScoreType Evaluator::cf_columnOpened_ = 8;
const ScoreType Evaluator::bg_columnOpened_ = 25;
const ScoreType Evaluator::ah_columnOpened_ = 20;

const ScoreType Evaluator::cf_columnSemiopened_ = 4;
const ScoreType Evaluator::bg_columnSemiopened_ = 8;
const ScoreType Evaluator::ah_columnSemiopened_ = 8;

const ScoreType Evaluator::cf_columnCracked_ = 1;
const ScoreType Evaluator::bg_columnCracked_ = 4;
const ScoreType Evaluator::ah_columnCracked_ = 2;

// pressure to king by opponents pawn
const ScoreType Evaluator::opponentPawnsToKing_ = 10;

//pressure to king by opponents bishop & knight
const ScoreType Evaluator::kingbishopPressure_ = 8;
const ScoreType Evaluator::kingknightPressure_ = 8;

// queen attacks opponent's king (give only if supported by other figure)
const ScoreType Evaluator::queenAttackBonus_ = 10;

/// pawns evaluation
#define MAX_PASSED_SCORE 80

const ScoreType Evaluator::pawnPassed_[8] = { 0, 5, 10, 20, 40, 60, MAX_PASSED_SCORE, 0 };
const ScoreType Evaluator::pawnGuarded_[8] = { 0, 0, 4, 6, 10, 12, 15, 0 };
const ScoreType Evaluator::passerCandidate_[8] =  { 0, 2, 5, 8, 10, 12, 15, 0 };
const ScoreType Evaluator::pawnOnOpenColumn_[8] = { 0, 2, 3, 5, 7, 9, 12, 0 };
const ScoreType Evaluator::pawnCanGo_[8] = { 0, 5, 7, 9, 12, 15, 20, 0 };

const ScoreType Evaluator::mobilityBonus_[8][32] = {
  {},
  {},
  {-40, -15, 0, 3, 5, 7, 9, 11},
  {-35, -12, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4},
  {-20, -8, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4},
  {-40, -35, -15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12},
};

const ScoreType Evaluator::kingDistanceBonus_[8][8] = {
  {},
  {},
  {15, 12, 10, 7, 6, 1, 0, 0},
  {15, 12, 10, 7, 5, 3, 1, 0},
  {20, 18, 13, 9, 7, 3, 1, 0},
  {40, 55, 45, 25, 12, 3, 1, 0},
};

const ScoreType Evaluator::kingAttackBonus_[8] = {
  0, 5, 25, 60, 80, 120, 150, 200
};

const ScoreType Evaluator::kingImmobility_[10] = {
  10, 5, 2
};


//////////////////////////////////////////////////////////////////////////
Evaluator::Evaluator() :
  board_(0), ehash_(0)
{
  weightMax_ = 2*(Figure::figureWeight_[Figure::TypeQueen] +
    2*Figure::figureWeight_[Figure::TypeRook] + 2*Figure::figureWeight_[Figure::TypeBishop] + 2*Figure::figureWeight_[Figure::TypeKnight]);

  for (int i = 0; i < sizeof(alpha_)/sizeof(*alpha_); ++i)
  {
    alpha_[i] = -ScoreMax;
    betta_[i] = +ScoreMax;
  }
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


  // pawns attacks
  {
    const BitMask & pawn_msk_w = board_->fmgr().pawn_mask_o(Figure::ColorWhite);
    const BitMask & pawn_msk_b = board_->fmgr().pawn_mask_o(Figure::ColorBlack);

    finfo_[Figure::ColorWhite].pw_attack_mask_ = ((pawn_msk_w << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_w << 7) & Figure::pawnCutoffMasks_[1]);
    finfo_[Figure::ColorBlack].pw_attack_mask_ = ((pawn_msk_b >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_b >> 9) & Figure::pawnCutoffMasks_[1]);

    finfo_[0].attack_mask_ = finfo_[0].pw_attack_mask_;
    finfo_[1].attack_mask_ = finfo_[1].pw_attack_mask_;
  }

  for (int i = 0; i < sizeof(alpha_)/sizeof(*alpha_); ++i)
  {
    alpha_[i] = -ScoreMax;
    betta_[i] = +ScoreMax;
  }
}

bool Evaluator::discoveredCheck(int pt, Figure::Color acolor, const BitMask & brq_mask, int ki_pos, enum PinType pinType) const
{
  const BitMask & from_msk = board_->g_betweenMasks->from(ki_pos, pt);
  BitMask mask_all_ex = mask_all_ & ~set_mask_bit(pt);
  mask_all_ex &= from_msk;
  if ( (mask_all_ex & brq_mask) == 0 )
    return false;

  int apos = ki_pos < pt ? find_lsb(mask_all_ex) : find_msb(mask_all_ex);
  if ( (set_mask_bit(apos) & brq_mask) == 0 ) // no BRQ on this field
    return false;

  const Field & afield = board_->getField(apos);
  THROW_IF( afield.color() != acolor || afield.type() < Figure::TypeBishop || afield.type() > Figure::TypeQueen, "discoveredCheck() - attacking figure isn't BRQ" );

  nst::dirs d = board_->g_figureDir->dir(ki_pos, pt);
  if ( d == nst::no_dir )
    return false;

  if ( pinType == ptAll )
    return true;

  static PinType pin_types[10] = { ptAll, ptDiag, ptOrtho, ptDiag, ptOrtho, ptDiag, ptOrtho, ptDiag, ptOrtho };
  
  return pin_types[d] == pinType;
}


//////////////////////////////////////////////////////////////////////////
ScoreType Evaluator::operator () (ScoreType alpha, ScoreType betta)
{
  THROW_IF( !board_, "Evaluator wasn't properly initialized" );

  if ( board_->matState() )
    return -Figure::MatScore;
  else if ( board_->drawState() )
    return Figure::DrawScore;

  prepare();

  // prepare lazy evaluation
  if ( alpha > -Figure::MatScore )
  {
    for (int i = 0; i < sizeof(alpha_)/sizeof(*alpha_); ++i)
      alpha_[i] = alpha - lazyThresholds_[i];
  }

  if ( betta < +Figure::MatScore )
  {
    for (int i = 0; i < sizeof(betta_)/sizeof(*betta_); ++i)
      betta_[i] = betta + lazyThresholds_[i];
  }

  ScoreType score = -ScoreMax;

  if ( board_->isWinnerLoser() )
    score = evaluateWinnerLoser();
  else
  {
    SpecialCases sc = findSpecialCase();
    if ( sc != SC_None )
      score = evaluateSpecial(sc);
    else
      score = evaluate();
  }

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
  
  ///// 1st level
  //if ( score < alpha_[0] || score > betta_[0] )
  //  return score;

  //score += evaluateKingPressure(phase, coef_o);

  // 2nd level
  if ( score < alpha_[1] || score > betta_[1] )
    return score;

  score += evaluateWorth(phase, coef_o, coef_e);

  // 3rd level
  if ( score < alpha_[2] || score > betta_[2] )
    return score;

  score += evaluateExpensive(phase, coef_o, coef_e);

  return score;
}

ScoreType Evaluator::evaluateKingPressure(GamePhase phase, int coef_o)
{
  if ( phase == EndGame )
    return 0;

  ScoreType score = evaluateKingPressure(Figure::ColorWhite) - evaluateKingPressure(Figure::ColorBlack);

  // consider current move side
  if ( Figure::ColorBlack  == board_->getColor() )
    score = -score;

  if ( phase == MiddleGame )
    score = (score * coef_o) / weightMax_;

  return score;
}

ScoreType Evaluator::evaluateKingPressure(Figure::Color color)
{
  Figure::Color ocolor = Figure::otherColor(color);
  int oki_pos = finfo_[ocolor].king_pos_;
  const BitMask & oki_mask = board_->g_movesTable->caps(Figure::TypeKing, oki_pos);
  
  BitMask mask_diag  = board_->fmgr().queen_mask(color) | board_->fmgr().bishop_mask(color);
  BitMask mask_ortho = board_->fmgr().queen_mask(color) | board_->fmgr().rook_mask(color);

  BitMask xray_masks[2] = { ~(~mask_diag & mask_all_), ~(~mask_ortho & mask_all_) };

  int attacked_fields[64] = {};
  uint8 attacked_field_types[64] = {};
  BitMask attacked_mask = 0;
  int attackersN = 0;
  uint8 attackerTypes = 0;
  int fieldAttackersN = 0;

  BitMask pw_mask = finfo_[color].pw_attack_mask_ & oki_mask;
  for ( ; pw_mask; )
  {
    int to = clear_lsb(pw_mask);
    attacked_mask |= set_mask_bit(to);
    uint8 a_mask = (uint8)set_bit(Figure::TypePawn);
    attackerTypes |= a_mask;
    attacked_field_types[to] |= a_mask;
    attacked_fields[to] = 1;
    attackersN++;
  }

  BitMask kn_mask = board_->fmgr().knight_mask(color);
  for ( ; kn_mask; )
  {
    int from = clear_lsb(kn_mask);
    const BitMask & kn_cap = board_->g_movesTable->caps(Figure::TypeKnight, from);
    BitMask oki_attack_mask = kn_cap & oki_mask;
    if ( oki_attack_mask )
      attackersN++;
    for ( ; oki_attack_mask; )
    {
      int to = clear_lsb(oki_attack_mask);
      attacked_mask |= set_mask_bit(to);
      uint8 a_mask = (uint8)set_bit(Figure::TypeKnight);
      attackerTypes |= a_mask;
      attacked_fields[to]++;
      attacked_field_types[to] |= a_mask;
      if ( attacked_fields[to] > fieldAttackersN )
        fieldAttackersN = attacked_fields[to];
    }
  }

  for (int type = Figure::TypeBishop; type <= Figure::TypeQueen; ++type)
  {
    BitMask mask = board_->fmgr().type_mask((Figure::Type)type, color);
    for ( ; mask; )
    {
      int from = clear_lsb(mask);
      
      const BitMask & cap_mask = board_->g_movesTable->caps((Figure::Type)type, from);
      BitMask oki_attack_mask = cap_mask & oki_mask;

      bool haveAttack = false;

      for ( ; oki_attack_mask; )
      {
        int to = clear_lsb(oki_attack_mask);
        const BitMask & btw_mask = board_->g_betweenMasks->between(from, to);

        bool attackFound = false;
        bool directAttack = false;

        // 1. nothing between
        if ( (btw_mask & inv_mask_all_) == btw_mask )
        {
          attackFound = true;
          directAttack = true;
        }

        // 2. X-ray attack
        if ( !attackFound )
        {
          nst::dirs dir = board_->g_figureDir->dir(from, to);

          THROW_IF(nst::no_dir == dir, "no direction between fields");

          int xray_type = (dir-1) & 1;
          const BitMask & xray_mask = xray_masks[xray_type];

          if ( (btw_mask & xray_mask) == btw_mask )
            attackFound = true;
        }

        if ( attackFound )
        {
          haveAttack = true;
          attacked_mask |= set_mask_bit(to);
          attacked_fields[to]++;
          if ( attacked_fields[to] > fieldAttackersN )
            fieldAttackersN = attacked_fields[to];
          if ( directAttack )
          {
            uint8 a_mask = (uint8)set_bit(type);
            attackerTypes |= a_mask;
            attacked_field_types[to] |= a_mask;
          }
        }
      }

      if ( haveAttack )
        attackersN++;
    }
  }

  BitMask oki_mob = oki_mask & ~(mask_all_ | attacked_mask);
  int movesN = pop_count(oki_mob);

  ScoreType score = 0;

  score += kingImmobility_[movesN];

  if ( 0 == attackersN )
    return score;

  score += kingAttackBonus_[attackersN & 7];

  // queen's attack
  if ( (attackerTypes & set_bit(Figure::TypeQueen)) && attackersN > 1 )
  {
    score += queenAttackBonus_;

    // give additional bonus for double attacked field that isn't protected by pawn
    BitMask a_mask = attacked_mask;
    for ( ; a_mask; )
    {
      int n = clear_lsb(a_mask);
      if ( attacked_fields[n] > 1 &&
          (attacked_field_types[n] & set_bit(Figure::TypeQueen)) &&
          !(finfo_[ocolor].pw_attack_mask_ & set_mask_bit(n)) )
      {
        score += (queenAttackBonus_<<1) * attacked_fields[n];
        break;
      }
    }
  }

  return score;
}

ScoreType Evaluator::evaluateWorth(GamePhase phase, int coef_o, int coef_e)
{
  ScoreType score = 0;

  // knight mobility
  score += evaluateKnights();

  // bishops mobility
  score += evaluateBishops();

  score -= evaluateForks(Figure::ColorBlack);
  score += evaluateForks(Figure::ColorWhite);

  ScoreType pw_score_eg = 0;
  score -= evaluatePasserAdditional(Figure::ColorBlack, pw_score_eg, phase);
  score += evaluatePasserAdditional(Figure::ColorWhite, pw_score_eg, phase);

  if ( phase == MiddleGame )
    pw_score_eg = (pw_score_eg * coef_e) / weightMax_;

  score += pw_score_eg;

  if ( Figure::ColorBlack  == board_->getColor() )
    score = -score;

  return score;
}

ScoreType Evaluator::evaluateKnights()
{
  ScoreType score = 0;

  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color = (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor(color);
    BitMask not_occupied = ~finfo_[ocolor].pw_attack_mask_ & inv_mask_all_;
    BitMask brq_mask = board_->fmgr().bishop_mask(ocolor) | board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor);
    const int &  ki_pos = finfo_[ color].king_pos_;
    const int & oki_pos = finfo_[ocolor].king_pos_;

    BitMask kn_mask = board_->fmgr().knight_mask(color);
    for ( ; kn_mask; )
    {
      int from = clear_lsb(kn_mask);
      const BitMask & kn_cap = board_->g_movesTable->caps(Figure::TypeKnight, from);
      finfo_[c].kn_attack_mask_ |= kn_cap;

      int ki_dist = board_->g_distanceCounter->getDistance(from, oki_pos);
      finfo_[c].knightPressure_ += kingDistanceBonus_[Figure::TypeKnight][ki_dist];

      finfo_[c].attack_mask_ |= kn_cap;

      BitMask kmob_mask = kn_cap & not_occupied;
      int movesN = pop_count(kmob_mask);
      finfo_[c].knightMobility_ += mobilityBonus_[Figure::TypeKnight][movesN];

      //// give penalty for pinned knight
      //if ( discoveredCheck(from, ocolor, brq_mask, ki_pos, ptAll) )
      //  finfo_[c].knightMobility_ += pinnedKnight_;
    }
  }

  score -= finfo_[0].knightPressure_;
  score += finfo_[1].knightPressure_;

  score -= finfo_[0].knightMobility_;
  score += finfo_[1].knightMobility_;

  return score;
}

ScoreType Evaluator::evaluateBishops()
{
  ScoreType score = 0;

  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color = (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor(color);
    BitMask not_attacked = ~finfo_[ocolor].pw_attack_mask_;
    BitMask rq_mask = board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor);
    const int &  ki_pos = finfo_[ color].king_pos_;
    const int & oki_pos = finfo_[ocolor].king_pos_;

    BitMask bimask = board_->fmgr().bishop_mask(color);
    for ( ; bimask; )
    {
      int from = clear_lsb(bimask);

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

      finfo_[c].attack_mask_ |= bmob_mask;

      bmob_mask &= not_attacked;

      int ki_dist = board_->g_distanceCounter->getDistance(from, oki_pos);
      finfo_[c].bishopPressure_ = kingDistanceBonus_[Figure::TypeBishop][ki_dist];

      int movesN = pop_count(bmob_mask);
      finfo_[c].bishopMobility_ += mobilityBonus_[Figure::TypeBishop][movesN];

      //// give penalty for pinned bishop
      //if ( discoveredCheck(from, ocolor, rq_mask, ki_pos, ptOrtho) )
      //  finfo_[c].bishopMobility_ += pinnedBishop_;
    }
  }

  score -= finfo_[0].bishopPressure_;
  score += finfo_[1].bishopPressure_;

  score -= finfo_[0].bishopMobility_;
  score += finfo_[1].bishopMobility_;

  return score;
}

// most expensive part: mobility of rooks and queens, rooks on open columns
ScoreType Evaluator::evaluateExpensive(GamePhase phase, int coef_o, int coef_e)
{
  ScoreType score = 0;

  evaluateRooks(phase != EndGame);

  evaluateQueens();

  ScoreType score_r = finfo_[1].rookOpenScore_ - finfo_[0].rookOpenScore_;

  if ( phase == MiddleGame )
    score_r = (score_r * coef_o) / weightMax_;

  score += score_r;

  score -= finfo_[0].rookMobility_;
  score += finfo_[1].rookMobility_;

  score -= finfo_[0].queenMobility_;
  score += finfo_[1].queenMobility_;

  score -= finfo_[0].rookPressure_;
  score += finfo_[1].rookPressure_;

  score -= finfo_[0].queenPressure_;
  score += finfo_[1].queenPressure_;

  if ( Figure::ColorBlack  == board_->getColor() )
    score = -score;

  return score;
}

void Evaluator::evaluateRooks(bool eval_open)
{
  BitMask rattack_mask[2] = { 0, 0 };

  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color = (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor(color);
    BitMask not_attacked = ~finfo_[ocolor].attack_mask_;
    const BitMask & q_mask = board_->fmgr().queen_mask(ocolor);
    const int &  ki_pos = finfo_[ color].king_pos_;
    const int & oki_pos = finfo_[ocolor].king_pos_;

    Index oki_p(oki_pos);
    int okx = oki_p.x();
    int oky = oki_p.y();

    const uint8 * pmsk_t  = (const uint8*)&board_->fmgr().pawn_mask_t(color);
    const uint8 * opmsk_t = (const uint8*)&board_->fmgr().pawn_mask_t(ocolor);
    BitMask ro_mask = board_->fmgr().rook_mask(color);
    for ( ; ro_mask; )
    {
      int from = clear_lsb(ro_mask);

      const BitMask & di_mask_no = board_->g_betweenMasks->from_dir(from, nst::no);
      const BitMask & di_mask_ea = board_->g_betweenMasks->from_dir(from, nst::ea);
      const BitMask & di_mask_so = board_->g_betweenMasks->from_dir(from, nst::so);
      const BitMask & di_mask_we = board_->g_betweenMasks->from_dir(from, nst::we);

      BitMask rmob_mask = 0;
      BitMask mask_from = di_mask_no & mask_all_;
      rmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_lsb(mask_from)) : di_mask_no);

      mask_from = di_mask_ea & mask_all_;
      rmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_lsb(mask_from)) : di_mask_ea);

      mask_from = di_mask_so & mask_all_;
      rmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_msb(mask_from)) : di_mask_so);

      mask_from = di_mask_we & mask_all_;
      rmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_msb(mask_from)) : di_mask_we);

      rattack_mask[c] |= rmob_mask;

      rmob_mask &= not_attacked;

      int ki_dist = board_->g_distanceCounter->getDistance(from, oki_pos);
      finfo_[c].rookPressure_ = kingDistanceBonus_[Figure::TypeRook][ki_dist];

      int movesN = pop_count(rmob_mask);
      finfo_[c].rookMobility_ += mobilityBonus_[Figure::TypeRook][movesN];

      //// give penalty for pinned rook
      //if ( discoveredCheck(from, ocolor, q_mask, ki_pos, ptDiag) )
      //  finfo_[c].rookMobility_ += pinnedRook_;

      // rook on open column
      if ( eval_open )
      {
        Index rp(from);
        int x = rp.x();
        int y = rp.y();

        // no pawns of some color
        if ( !opmsk_t[rp.x()] || !pmsk_t[rp.x()] )
        {
          finfo_[c].rookOpenScore_ += semiopenRook_;

          // near opponent's king
          if ( x == okx || x == okx-1 || x == okx+1 )
            finfo_[c].rookOpenScore_ += rookToKingBonus_;
        }

        // no pawns at all
        if ( !(opmsk_t[x] | pmsk_t[x]) )
          finfo_[c].rookOpenScore_ += openRook_;

        // rooks to the opponent king
        if ( y == oky || y == oky-1 || y == oky+1 )
          finfo_[c].rookOpenScore_ += rookToKingBonus_;
      }
    }
  }

  finfo_[0].attack_mask_ |= rattack_mask[0];
  finfo_[1].attack_mask_ |= rattack_mask[1];
}

void Evaluator::evaluateQueens()
{
  BitMask qattack_mask[2] = { 0, 0 };

  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color = (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor(color);
    BitMask not_attacked = ~finfo_[ocolor].attack_mask_;
    const int & oki_pos = finfo_[ocolor].king_pos_;

    BitMask q_mask = board_->fmgr().queen_mask(color);
    for ( ; q_mask; )
    {
      int from = clear_lsb(q_mask);

      BitMask qmob_mask = 0;

      const BitMask & di_mask_nw = board_->g_betweenMasks->from_dir(from, nst::nw);
      const BitMask & di_mask_ne = board_->g_betweenMasks->from_dir(from, nst::ne);
      const BitMask & di_mask_se = board_->g_betweenMasks->from_dir(from, nst::se);
      const BitMask & di_mask_sw = board_->g_betweenMasks->from_dir(from, nst::sw);

      const BitMask & di_mask_no = board_->g_betweenMasks->from_dir(from, nst::no);
      const BitMask & di_mask_ea = board_->g_betweenMasks->from_dir(from, nst::ea);
      const BitMask & di_mask_so = board_->g_betweenMasks->from_dir(from, nst::so);
      const BitMask & di_mask_we = board_->g_betweenMasks->from_dir(from, nst::we);

      
      BitMask mask_from = di_mask_nw & mask_all_;
      qmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_lsb(mask_from)) : di_mask_nw);

      mask_from = di_mask_ne & mask_all_;
      qmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_lsb(mask_from)) : di_mask_ne);

      mask_from = di_mask_se & mask_all_;
      qmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_msb(mask_from)) : di_mask_se);

      mask_from = di_mask_sw & mask_all_;
      qmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_msb(mask_from)) : di_mask_sw);

      
      mask_from = di_mask_no & mask_all_;
      qmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_lsb(mask_from)) : di_mask_no);

      mask_from = di_mask_ea & mask_all_;
      qmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_lsb(mask_from)) : di_mask_ea);

      mask_from = di_mask_so & mask_all_;
      qmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_msb(mask_from)) : di_mask_so);

      mask_from = di_mask_we & mask_all_;
      qmob_mask |= ((mask_from) ? board_->g_betweenMasks->between(from, find_msb(mask_from)) : di_mask_we);

      qattack_mask[c] |= qmob_mask;

      qmob_mask &= not_attacked;

      int ki_dist = board_->g_distanceCounter->getDistance(from, oki_pos);
      finfo_[c].queenPressure_ = kingDistanceBonus_[Figure::TypeQueen][ki_dist];

      int movesN = pop_count(qmob_mask);
      finfo_[c].queenMobility_ += mobilityBonus_[Figure::TypeQueen][movesN];
    }
  }

  finfo_[0].attack_mask_ |= qattack_mask[0];
  finfo_[1].attack_mask_ |= qattack_mask[1];
}

//////////////////////////////////////////////////////////////////////////
/// common  part
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
  HEval * heval = 0;

  const uint64 & code = board_->pawnCode();
  uint32 hkey = (uint32)(code >> 32);

  if ( ehash_ )
  {
    heval = ehash_->get(code);

    if ( heval->hkey_ == hkey && heval->initizalized_ )
    {
      pwscore = heval->pwscore_;
      pwscore_eg = heval->pwscore_eg_;
      score_ps = heval->score_ps_;
      return;
    }
  }

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

  if ( heval )
  {
    heval->hkey_     = hkey;
    heval->pwscore_  = pwscore;
    heval->pwscore_eg_ = pwscore_eg;
    heval->score_ps_ = score_ps;
    heval->initizalized_ = 1;
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

  BitMask o_rq_mask = board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor);
  BitMask o_mask = board_->fmgr().knight_mask(ocolor) | board_->fmgr().bishop_mask(ocolor) | o_rq_mask;

  BitMask pawn_fork = o_mask & finfo_[color].pw_attack_mask_;
  int pawnsN = pop_count(pawn_fork);
  if ( pawnsN > 1 )
    return forkBonus_;

  BitMask kn_fork = o_rq_mask & finfo_[color].kn_attack_mask_;
  int knightsN = pop_count(kn_fork);
  if ( pawnsN+knightsN > 1 )
    return forkBonus_;

  if ( pawnsN+knightsN > 0 && color == board_->getColor() )
    return attackedByWeakBonus_;

  return 0;
}

ScoreType Evaluator::evaluateMaterialDiff()
{
  ScoreType score = 0;

  const FiguresManager & fmgr = board_->fmgr();

  Figure::Color color = board_->getColor();
  Figure::Color ocolor = Figure::otherColor(color);
  
  // 1. Knight - Bishop disbalance
  score += (fmgr.bishops(Figure::ColorWhite) - fmgr.bishops(Figure::ColorBlack))*bishopBonus_;

  // 3. Knight or Bishop against 3 pawns
  int figuresDiff = (fmgr.bishops(Figure::ColorWhite)+fmgr.knights(Figure::ColorWhite)) -
    (fmgr.bishops(Figure::ColorBlack)+fmgr.knights(Figure::ColorBlack));

  int pawnsDiff  = fmgr.pawns(Figure::ColorWhite) - fmgr.pawns(Figure::ColorBlack);
  int rooksDiff  = fmgr.rooks(Figure::ColorWhite) - fmgr.rooks(Figure::ColorBlack);
  int queensDiff = fmgr.queens(Figure::ColorWhite) - fmgr.queens(Figure::ColorBlack);

  if ( figuresDiff*pawnsDiff < 0 && !rooksDiff && !queensDiff )
    score += figuresDiff * figureAgainstPawnBonus_;

  // 4. Knight|Bishop+2Pawns vs. Rook
  else if ( !queensDiff && rooksDiff*figuresDiff == -1 )
    score += rooksDiff * rookAgainstFigureBonus_;

  return score;
}

int Evaluator::getCastleType(Figure::Color color) const
{
  Index ki_pos(finfo_[color].king_pos_);

  // short
  if ( ki_pos.x() > 4 /*&& (ki_pos.y() < 2 && color || ki_pos.y() > 5 && !color)*/ )
    return 0;

  // long
  if ( ki_pos.x() < 3 /*&& (ki_pos.y() < 2 && color || ki_pos.y() > 5 && !color)*/ )
    return 1;

  return -1;
}

ScoreType Evaluator::evaluatePawnShield2(Figure::Color color)
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

  // whole line is opened
  // color, castle type, column number (abc, hgf)
  static const BitMask abc_masks_full[2][2][3] = {
    { { 0x8080808000000000ULL, 0x4040404000000000ULL, 0x2020202000000000ULL }, { 0x0101010100000000ULL, 0x0202020200000000ULL, 0x0404040400000000ULL } },
    { { 0x0000000080808080ULL, 0x0000000040404040ULL, 0x0000000020202020ULL }, { 0x0000000001010101ULL, 0x0000000002020202ULL, 0x0000000004040404ULL } }
  };

  // first 2 squares are opened
  // color, castle type, column number
  static const BitMask abc_mask_two[2][2][3] = {
    { { 0x0080800000000000ULL, 0x0040400000000000ULL, 0x0020200000000000ULL }, { 0x0001010000000000ULL, 0x0002020000000000ULL, 0x0004040000000000ULL } },
    { { 0x0000000000808000ULL, 0x0000000000404000ULL, 0x0000000000202000ULL }, { 0x0000000000010100ULL, 0x0000000000020200ULL, 0x0000000000040400ULL } }
  };

  // only nearest square is opened
  // color, castle type, column number
  static const BitMask abc_mask_one[2][2][3] = {
    { { set_mask_bit(H7), set_mask_bit(G7), set_mask_bit(F7) }, { set_mask_bit(A7), set_mask_bit(B7), set_mask_bit(C7) } },
    { { set_mask_bit(H2), set_mask_bit(G2), set_mask_bit(F2) }, { set_mask_bit(A2), set_mask_bit(B2), set_mask_bit(C2) } }
  };

  // first 2 lines empty, full line empty
  static const ScoreType kingPenalties[3][3] = {
    {ah_columnOpened_, bg_columnOpened_, cf_columnOpened_},
    {ah_columnSemiopened_, bg_columnSemiopened_, cf_columnSemiopened_},
    {ah_columnCracked_, bg_columnCracked_, cf_columnCracked_}
  };

  // color, castle type
  static const BitMask opponent_pressure_masks[2][2] = {
    {set_mask_bit(F6)|set_mask_bit(G6)|set_mask_bit(H6), set_mask_bit(A6)|set_mask_bit(B6)|set_mask_bit(C6)},
    {set_mask_bit(F3)|set_mask_bit(G3)|set_mask_bit(H3), set_mask_bit(A3)|set_mask_bit(B3)|set_mask_bit(C3)}
  };

  // pawn shield ab, gh, cf columns
  for (int i = 0; i < 3; ++i)
  {
    const BitMask & full_mask = abc_masks_full[color][ctype][i];
    const BitMask & two_mask  = abc_mask_two[color][ctype][i];
    const BitMask & one_mask  = abc_mask_one[color][ctype][i];

    if ( !(full_mask & pw_mask) )
      score -= kingPenalties[0][i];
    else if ( !(two_mask & pw_mask) )
      score -= kingPenalties[1][i];
    else if ( !(one_mask & pw_mask) )
      score -= kingPenalties[2][i];
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
    { set_mask_bit(G8)|set_mask_bit(H8)|set_mask_bit(G7)|set_mask_bit(H7),
      set_mask_bit(A8)|set_mask_bit(B8)|set_mask_bit(C8)|set_mask_bit(A7)|set_mask_bit(B7)|set_mask_bit(C7)},

    { set_mask_bit(G1)|set_mask_bit(H1)|set_mask_bit(G2)|set_mask_bit(H2),
      set_mask_bit(A1)|set_mask_bit(B1)|set_mask_bit(C1)|set_mask_bit(A2)|set_mask_bit(B2)|set_mask_bit(C2)} };

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
  const BitMask & oqueen_mask  = fmgr.queen_mask(ocolor);
  const BitMask & oknight_mask = fmgr.knight_mask(ocolor);

  // color, castle type
  static const BitMask opponent_bishop_masks[2][2] = {
    {set_mask_bit(F6)|set_mask_bit(H6), set_mask_bit(C6)|set_mask_bit(A6)},
    {set_mask_bit(F3)|set_mask_bit(H3), set_mask_bit(C3)|set_mask_bit(A3)}
  };

  static const BitMask opponent_knight_masks[2][2] = {
    {set_mask_bit(G5), set_mask_bit(B5)},
    {set_mask_bit(G4), set_mask_bit(B4)}
  };

  //static const BitMask opponent_queen_masks[2][2] = {
  //  {set_mask_bit(F6)|set_mask_bit(H6)|set_mask_bit(H5)|set_mask_bit(H4), set_mask_bit(C6)|set_mask_bit(A6)|set_mask_bit(A5)|set_mask_bit(A4)},
  //  {set_mask_bit(F3)|set_mask_bit(H3)|set_mask_bit(H4)|set_mask_bit(H5), set_mask_bit(C3)|set_mask_bit(A3)|set_mask_bit(A4)|set_mask_bit(A5)}
  //};

  // some attack patterns
  if ( ki_pos.y() < 2 && color || ki_pos.y() > 5 && !color )
  {
    bool kn_or_bi = false;
    // opponent bishop pressure
    if ( opponent_bishop_masks[color][ctype] & obishop_mask )
    {
      kn_or_bi = true;
      score -= kingbishopPressure_;
    }

    // opponent's knight pressure
    if ( opponent_knight_masks[color][ctype] & oknight_mask )
    {
      kn_or_bi = true;
      score -= kingknightPressure_;
    }

    // opponent queen pressure
    if ( kn_or_bi && (opponent_bishop_masks[color][ctype] & oqueen_mask) )
      score -= queenAttackBonus_;
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

  BitMask attacked_mask = (finfo_[ocolor].pw_attack_mask_ & ~finfo_[color].pw_attack_mask_);

  int py = promo_y[color];

  // columns, occupied by pawns
  uint8 cols_visited = 0;

  const BitMask & pw_mask_o = fmgr.pawn_mask_o(color);
  BitMask pawn_mask = pw_mask_o;
  for ( ; pawn_mask; )
  {
    int n = clear_lsb(pawn_mask);

    int x = n & 7;
    int y = n >>3;

    int cy = color ? y : 7-y;

    // double pawns
    if ( !(cols_visited & (1<<x)) )
    {
      cols_visited |= 1<<x;
      uint8 column = ((uint8*)&pmsk_t)[x];
      int dblNum = BitsCounter::numBitsInByte(column);
      if ( dblNum > 1 )
      {

        // iterate from upper pawn to lower
        for ( ; column && dblNum > 1; --dblNum )
        {
          int py = color ? find_msb_32(column) : find_lsb_32(column);
          int p = x | (py << 3);

          THROW_IF( board_->getField(p).color() != color || board_->getField(p).type() != Figure::TypePawn, "no pawn found on double line" );

          const BitMask & pw_cap = board_->g_movesTable->pawnCaps_o(ocolor, p);

          /// give big penalty if upper pawn isn't defended or small otherwise
          if ( pw_cap & pw_mask_o ) // defended
            score += pawnDoubled_ >> 1;
          else
            score += pawnDoubled_;
        }
      }
    }

    const uint64 & passmsk = board_->g_pawnMasks->mask_passed(color, n);
    const uint64 & blckmsk = board_->g_pawnMasks->mask_blocked(color, n);

    bool passed = false;

    // passed pawn evaluation
    if ( !(opmsk_t & passmsk) && !(pmsk_t & blckmsk) )
    {
      passed = true;
      score += pawnPassed_[cy];

      // guarded by neighbor pawn
      const BitMask & guardmsk = board_->g_pawnMasks->mask_guarded(color, n);
      if ( pmsk_t & guardmsk )
        score += pawnGuarded_[cy];

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
          *score_eg += kingFarBonus_;
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
    // or disconnected
    else if ( !(pmsk_t & dismask) )
      score += pawnDisconnected_;

    // backward - ie pawn isn't on last line and not guarded by other pawn and can't safely go to the next line
    if ( (color && y < 6 || !color && y > 1) && !(finfo_[color].pw_attack_mask_ & set_mask_bit(n)) )
    {
      int to = x | ((y+delta_y[color]) << 3);
      BitMask to_mask = set_mask_bit(to);

      if ( to_mask & opmsk_o )
        score += pawnBlocked_;
      else if ( to_mask & attacked_mask )
        score += pawnBackward_;
    }

    // check if it is passer candidate - column after pawn is opened and has more defenders than opponent's sentries
    if ( !passed && !(pmsk_t & blckmsk) && !(opmsk_t & blckmsk) )
    {
      THROW_IF( color && y > 6 || !color && y < 2, "passed pawn wasn't detected" );

      const uint8 *  pcol_t = (const uint8*)&pmsk_t;
      const uint8 * opcol_t = (const uint8*)&opmsk_t;

      int sentriesN = 0;
      int defendersN = 0;

      if ( x > 0 )
      {
        // sentries
        {
          int le = Index(x-1, y);
          const uint8 * oblock_le = (const uint8 *)&board_->g_pawnMasks->mask_blocked(color, le);
          int sentry_le = BitsCounter::numBitsInByte(opcol_t[x-1] & oblock_le[x-1]);
          sentriesN += sentry_le;
        }

        // defenders
        {
          int le = Index(x-1, y+delta_y[color]);
          const uint8 * block_le = (const uint8 *)&board_->g_pawnMasks->mask_blocked(ocolor, le);
          int defender_le = BitsCounter::numBitsInByte(pcol_t[x-1] & block_le[x-1]);
          defendersN += defender_le;
        }
      }

      if ( x < 7 )
      {
        // sentries
        {
          int ri = Index(x+1, y);
          const uint8 * oblock_ri = (const uint8 *)&board_->g_pawnMasks->mask_blocked(color, ri);
          int sentry_ri = BitsCounter::numBitsInByte(opcol_t[x+1] & oblock_ri[x+1]);
          sentriesN += sentry_ri;
        }

        // defenders
        {
          int ri = Index(x+1, y+delta_y[color]);
          const uint8 * block_ri = (const uint8 *)&board_->g_pawnMasks->mask_blocked(ocolor, ri);
          int defender_ri = BitsCounter::numBitsInByte(pcol_t[x+1] & block_ri[x+1]);
          defendersN += defender_ri;
        }
      }

      // passer candidate
      if ( defendersN >= sentriesN )
        score += passerCandidate_[cy];
      else // give bonus for pawn on open column
        score += pawnOnOpenColumn_[cy];
    }
  }

  return score;
}

ScoreType Evaluator::evaluatePasserAdditional(Figure::Color color, ScoreType & pw_score_eg, GamePhase phase)
{
  const FiguresManager & fmgr = board_->fmgr();
  Figure::Color ocolor = Figure::otherColor(color);

  const BitMask & opmsk_t = fmgr.pawn_mask_t(ocolor);
  const BitMask & pmsk_t = fmgr.pawn_mask_t(color);
  if ( !pmsk_t )
    return 0;

  ScoreType score = 0, score_eg = 0;

  static int delta_y[] = { -1, 1 };
  static int promo_y[] = {  0, 7 };
  
  int py = promo_y[color];
  int dy = delta_y[color];

  nst::dirs dir_behind[] = {nst::no, nst::so};

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
      int pawn_dist_promo = py - y;
      int cy = color ? y : 7-y;

      // have bishop with color the same as promotion field's color
      Figure::Color pcolor = ((Figure::Color)FiguresCounter::s_whiteColors_[promo_pos]);
      if ( pcolor && fmgr.bishops_w(color) || !pcolor && fmgr.bishops_b(color) )
        score += assistantBishop_;

      // have rook behind passed pawn
      BitMask behind_msk = board_->g_betweenMasks->from_dir(n, dir_behind[color]) & fmgr.rook_mask(color) ;
      if ( behind_msk )
      {
        int rpos = color ? find_msb(behind_msk) : find_lsb(behind_msk);
        if ( board_->is_nothing_between(n, rpos, inv_mask_all_) )
          score += rookBehindBonus_;
      }

      // pawn can go to the next line
      int next_pos = x | ((y+dy)<<3);
      THROW_IF( ((y+dy)) > 7 || ((y+dy)) < 0, "pawn goes to invalid line" );

      bool can_go = false;

      // field is empty
      // check is it attacked by opponent
      if ( !board_->getField(next_pos) )
      {
        Move move;
        Figure::Type type = Figure::TypeNone;
        if ( next_pos == promo_pos )
          type = Figure::TypeQueen;

        move.set(n, next_pos, type, false);
        ScoreType see_score = board_->see(move);
        can_go = see_score >= 0;
      }

      // additional bonus if opponent's king is too far from my pawn's promotion field
      if ( can_go )
      {
        score += pawnCanGo_[cy];

        if ( phase != Opening )
        {
          if ( pawn_dist_promo < 0 )
            pawn_dist_promo = -pawn_dist_promo;

          int o_dist_promo = board_->g_distanceCounter->getDistance(finfo_[ocolor].king_pos_, promo_pos);
          if ( board_->color_ == ocolor )
            pawn_dist_promo++;

          if ( pawn_dist_promo < o_dist_promo || !findRootToPawn(color, pawn_dist_promo, promo_pos) )
            score_eg += unstoppablePawn_;
        }
      }
    }
  }

  if ( !color )
    score_eg = -score_eg;

  pw_score_eg += score_eg;

  return score;
}

bool Evaluator::findRootToPawn(Figure::Color color, int pawn_dist_promo, int promo_pos) const
{
  Figure::Color ocolor = Figure::otherColor(color);
  int oki_pos = finfo_[ocolor].king_pos_;

  struct RootNode
  {
    int p;
    int d;
    BitMask vis;
  };

  RootNode looked[128];
  int lookedN = 1;
  looked[0].p = oki_pos;
  looked[0].d = 0;
  looked[0].vis = (mask_all_ | finfo_[color].attack_mask_) & ~set_mask_bit(promo_pos);

  for ( ; lookedN > 0; )
  {
    THROW_IF(lookedN > 128, "searching root. out of memory" );

    lookedN--;
    int pos = looked[lookedN].p;
    int dis = looked[lookedN].d;

    if ( dis > pawn_dist_promo ||
         dis + board_->g_distanceCounter->getDistance(promo_pos, pos) > pawn_dist_promo )
    {
      continue;
    }

    if ( pos == promo_pos )
      return true;

    BitMask visited = looked[lookedN].vis | set_mask_bit(pos);

    BitMask mask = board_->g_movesTable->caps(Figure::TypeKing, pos);
    mask &= ~visited;

    dis++;
    if ( dis > pawn_dist_promo )
      continue;

    for ( ; mask; )
    {
      int n = clear_lsb(mask);
      if ( dis + board_->g_distanceCounter->getDistance(promo_pos, n) > pawn_dist_promo )
        continue;

      looked[lookedN].p = n;
      looked[lookedN].d = dis;
      looked[lookedN].vis = visited;
      lookedN++;
    }
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////
/// special cases

// not winner-loser
Evaluator::SpecialCases Evaluator::findSpecialCase() const
{
  const FiguresManager & fmgr = board_->fmgr();

  if ( fmgr.queens(Figure::ColorBlack)+fmgr.queens(Figure::ColorWhite) > 0 ||
       fmgr.pawns(Figure::ColorBlack)+fmgr.pawns(Figure::ColorWhite) > 0 )
  {
    return SC_None;
  }

  if ( fmgr.rooks(Figure::ColorBlack) != 1 || fmgr.rooks(Figure::ColorWhite) != 1 )
  {
    return SC_None;
  }

  if ( fmgr.bishops(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 0 && 
       fmgr.bishops(Figure::ColorWhite) == 1 && fmgr.knights(Figure::ColorWhite) == 0 )
  {
    return SC_RBR_W;
  }

  if ( fmgr.bishops(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 0 && 
       fmgr.bishops(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 1 )
  {
    return SC_RNR_W;
  }

  if ( fmgr.bishops(Figure::ColorBlack) == 1 && fmgr.knights(Figure::ColorBlack) == 0 && 
       fmgr.bishops(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 0 )
  {
    return SC_RBR_B;
  }

  if ( fmgr.bishops(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 1 && 
       fmgr.bishops(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 0 )
  {
    return SC_RNR_B;
  }

  return SC_None;
}

ScoreType Evaluator::evaluateSpecial(SpecialCases sc) const
{
  ScoreType score = 0;

  switch ( sc )
  {
  case SC_RBR_B:
  case SC_RNR_B:
    score = -Figure::figureWeight_[Figure::TypePawn] >> 1;
    break;

  case SC_RBR_W:
  case SC_RNR_W:
    score = +Figure::figureWeight_[Figure::TypePawn] >> 1;
    break;

  default:
    THROW_IF(true, "invalid special case given");
    break;
  }

  const int & kpb = finfo_[0].king_pos_;
  const int & kpw = finfo_[1].king_pos_;

  const FiguresManager & fmgr = board_->fmgr();

  // opponent's NB should be as far as possible from my king
  BitMask fmsk = 0;
  switch ( sc )
  {
  case SC_RBR_W:
    fmsk = fmgr.bishop_mask(Figure::ColorWhite);
    break;

  case SC_RNR_W:
    fmsk = fmgr.knight_mask(Figure::ColorWhite);
    break;

  case SC_RBR_B:
    fmsk = fmgr.bishop_mask(Figure::ColorBlack);
    break;

  case SC_RNR_B:
    fmsk = fmgr.knight_mask(Figure::ColorBlack);
    break;
  }

  int pos = find_lsb(fmsk);

  // loser king's position should be near to center
  // winner king should be near to loser king
  int ki_dist = board_->g_distanceCounter->getDistance(kpw, kpb);

  if ( sc == SC_RBR_W || sc == SC_RNR_W ) // white is winner
  {
    int dist = board_->g_distanceCounter->getDistance(pos, kpb);
    score -= dist << 1;

    score -= positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpb);
    score += positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpw) >> 1;
    score -= ki_dist << 1;
  }
  else // back is winner
  {
    int dist = board_->g_distanceCounter->getDistance(pos, kpw);
    score += dist << 1;

    score -= positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpb) >> 1;
    score += positionEvaluation(1, Figure::ColorWhite, Figure::TypeKing, kpw);
    score += ki_dist << 1;
  }

  if ( Figure::ColorBlack  == board_->getColor() )
    score = -score;

  return score;
}

// winner-loser
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
    // Rook against 2 or more Bishops|Knights
    else if ( fmgr.queens(win_color) == 0 && fmgr.bishops(win_color) == 0 &&
              fmgr.knights(win_color) == 0 && fmgr.pawns(win_color) == 0 && fmgr.rooks(win_color) == 1 &&
              fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 1 )
    {
      score = 25;
    }
    // Rook against Knight|Bishop
    else if ( fmgr.queens(win_color) == 0 && fmgr.bishops(win_color) == 0 &&
              fmgr.knights(win_color) == 0 && fmgr.pawns(win_color) == 0 && fmgr.rooks(win_color) == 1 &&
              fmgr.knights(lose_color)+fmgr.bishops(lose_color) == 1 )
    {
      score = 35;
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
      score += positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, king_pos_w) >> 1;
    }
  }

  if ( win_color == Figure::ColorBlack )
    score = -score;

  if ( eval_pawns )
  {
    ScoreType pwscore = -ScoreMax, pwscore_eg = -ScoreMax, score_ps = -ScoreMax;
    hashedEvaluation(pwscore, pwscore_eg, score_ps);

    ScoreType score_eg = 0;
    score -= evaluatePasserAdditional(Figure::ColorBlack, score_eg, EndGame);
    score += evaluatePasserAdditional(Figure::ColorWhite, score_eg, EndGame);

    score += score_eg;
    score += pwscore;
  }

  if ( Figure::ColorBlack  == board_->getColor() )
    score = -score;

  return score;
}


//////////////////////////////////////////////////////////////////////////
/// experimental

ScoreType Evaluator::evaluatePawnShield(Figure::Color color)
{
  const FiguresManager & fmgr = board_->fmgr();
  const uint8 * pmsk_t  = (const uint8*)&board_->fmgr().pawn_mask_t(color);

  ScoreType score = 0;
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
  Index ki_pos(finfo_[color].king_pos_);

  int kx = ki_pos.x();
  int ky = ki_pos.y();

  int delta_y[] = {-1, +1};

  int ctype = getCastleType(color);

  // in castle
  if ( ctype >= 0 )
  {
    const BitMask & pw_mask = fmgr.pawn_mask_o(color);
    const BitMask & opw_mask = fmgr.pawn_mask_o(ocolor);

    int dy = delta_y[color];
    int pw_x[2][3] = { {5, 6, 7}, {2, 1, 0} };

    // first 2 lines empty, full line empty
    static const ScoreType kingPenalties[3][3] = {
      {cf_columnOpened_, bg_columnOpened_, ah_columnOpened_},
      {cf_columnSemiopened_, bg_columnSemiopened_, ah_columnSemiopened_},
      {cf_columnCracked_, bg_columnCracked_, ah_columnCracked_}
    };

    // f, g, h - short; b, c, a - long
    for (int i = 0; i < 3; ++i)
    {
      int & x = pw_x[ctype][i];
      Index ppy(x, ky), ppy1(x, ky+dy);
      BitMask y_mask = set_mask_bit(ppy) | set_mask_bit(ppy1);
      if ( pw_mask & y_mask )
        continue;

      Index ppy2(x, ky+dy+dy);
      BitMask y_mask2 = set_mask_bit(ppy2);
      if ( pw_mask & y_mask2 )
        score -= kingPenalties[2][i]; // cracked
      else if ( !pmsk_t[x] )
        score -= kingPenalties[0][i]; // completely opened
      else
        score -= kingPenalties[1][i]; // semi-opened (no pawns on 1st & 2nd lines after king)
    }
  }

  // give penalty if there is no pawn before king
  {
    int yplus  = ky + delta_y[color];
    int yplus2 = yplus + delta_y[color];

    if ( yplus < 1 )
      yplus = 1;
    else if ( yplus > 6 )
      yplus = 6;

    if ( yplus2 < 1 )
      yplus2 = 1;
    else if ( yplus2 > 6 )
      yplus2 = 6;

    uint8 pp_mask = set_bit(ky) | set_bit(yplus) | set_bit(yplus2);
    if ( (pmsk_t[kx] & pp_mask) == 0 )
      score -= bg_columnSemiopened_;
  }

  // try to put king under pawn's shield anyway
  {
    int yplus  = ky + delta_y[color];
    int yplus2 = yplus + delta_y[color];

    if ( yplus < 1 )
      yplus = 1;
    else if ( yplus > 6 )
      yplus = 6;

    if ( yplus2 < 1 )
      yplus2 = 1;
    else if ( yplus2 > 6 )
      yplus2 = 6;

    uint8 pp_mask = set_bit(ky) | set_bit(yplus) | set_bit(yplus2);

    int xle = kx-1;
    if ( xle < 0 )
      xle = 0;

    int xri = kx+1;
    if ( xri > 7 )
      xri = 7;

    int pw_count = 0;
    for (int x = xle; x <= xri; ++x)
    {
      if ( pmsk_t[x] & pp_mask )
        pw_count++;
    }

    // give small bonus if king is under pawn's shield
    static const int pawns_count_bonus[4] = { 0, 5, 10, 15 };
    score -= pawns_count_bonus[pw_count & 3];
  }

  // color, castle type
  static const BitMask opponent_pressure_masks[2][2] = {
    {set_mask_bit(F6)|set_mask_bit(G6)|set_mask_bit(H6), set_mask_bit(A6)|set_mask_bit(B6)|set_mask_bit(C6)},
    {set_mask_bit(F3)|set_mask_bit(G3)|set_mask_bit(H3), set_mask_bit(A3)|set_mask_bit(B3)|set_mask_bit(C3)}
  };

  if ( ki_pos.y() < 2 && color || ki_pos.y() > 5 && !color )
  {
    const BitMask & opw_mask = fmgr.pawn_mask_o(ocolor);

    // opponent pawns pressure
    if ( opponent_pressure_masks[color][ctype] & opw_mask )
      score -= opponentPawnsToKing_;
  }

  return score;
}

//int kx = ki_pos.x();
//int ky = ki_pos.y();
//
//int y0 = ky + delta_y[color];
//int y1 = y0 + delta_y[color];
//
//if ( y0 > 6 || y0 < 1 )
//return -20;
//
//if ( y1 > 6 || y1 < 1 )
//return -15;
//
//// check if there some pawn 2 lines higher than king
//uint8 half_mask = 0;
//int y2 = y1 + delta_y[color];
//if ( y2 < 7 && y2 > 0 )
//half_mask = set_bit(y2);
//
//// 1st - pawns under king
//int pp0 = kx | (y0 << 3);
//int pp1 = kx | (y1 << 3);
//
//int pawnsCount = 0;
//
//if ( board_->getField(pp0).type() != Figure::TypePawn || board_->getField(pp0).color() != color )
//{
//  if ( board_->getField(pp1).type() == Figure::TypePawn && board_->getField(pp1).color() == color )
//  {
//    score -= bg_columnCracked_;
//    pawnsCount++;
//  }
//  else if ( pmsk_t[kx] )
//  {
//    score -= bg_columnSemiopened_;
//    pawnsCount++;
//  }
//  else // no pawn in column
//    score -= bg_columnOpened_;
//}
//else
//pawnsCount++;
//
//// 2nd - pawn left to king
//int kx_le = kx-1;
//if ( kx_le >= 0 )
//{
//  int fullPenalty = 0, halfPenalty = 0, crackedPenalty = 0;
//
//  // in case of right castle give small penalty or big otherwise
//  if ( ctype == 0 )
//  {
//    fullPenalty = cf_columnOpened_;
//    halfPenalty = cf_columnCracked_;
//    crackedPenalty = cf_columnSemiopened_;
//  }
//  else
//  {
//    fullPenalty = ah_columnOpened_;
//    halfPenalty = ah_columnSemiopened_;
//    crackedPenalty = ah_columnCracked_;
//  }
//
//  int ppk = kx_le | (ky << 3);
//  pp0 = kx_le | (y0 << 3);
//  pp1 = kx_le | (y1 << 3);
//
//  // no pawn on upper or the same row
//  if ( (board_->getField(pp0).type() != Figure::TypePawn || board_->getField(pp0).color() != color) &&
//    (board_->getField(ppk).type() != Figure::TypePawn || board_->getField(ppk).color() != color) )
//  {
//    if ( board_->getField(pp1).type() == Figure::TypePawn && board_->getField(pp1).color() == color )
//    {
//      score -= crackedPenalty;
//      pawnsCount++;
//    }
//    else if ( pmsk_t[kx_le] )
//    {
//      score -= halfPenalty;
//      pawnsCount++;
//    }
//    else
//      score -= fullPenalty;
//  }
//  else
//    pawnsCount++;
//}
//
//// 3rd - pawn right to king
//int kx_ri = kx+1;
//if ( kx_ri < 8 )
//{
//  int fullPenalty = 0, halfPenalty = 0, crackedPenalty = 0;
//
//  // the same penalty but for left castle type
//  if ( ctype == 1 )
//  {
//    fullPenalty = cf_columnOpened_;
//    halfPenalty = cf_columnSemiopened_;
//    crackedPenalty = cf_columnCracked_;
//  }
//  else
//  {
//    fullPenalty = ah_columnOpened_;
//    halfPenalty = ah_columnSemiopened_;
//    crackedPenalty = ah_columnCracked_;
//  }
//
//  int ppk = kx_ri | (ky << 3);
//  pp0 = kx_ri | (y0 << 3);
//  pp1 = kx_ri | (y1 << 3);
//
//  // no pawn on upper or the same row
//  if ( (board_->getField(pp0).type() != Figure::TypePawn || board_->getField(pp0).color() != color) &&
//    (board_->getField(ppk).type() != Figure::TypePawn || board_->getField(ppk).color() != color) )
//  {
//    // no pawn at all
//    if ( board_->getField(pp1).type() == Figure::TypePawn && board_->getField(pp1).color() == color )
//    {
//      score -= crackedPenalty;
//      pawnsCount++;
//    }
//    else if ( pmsk_t[kx_ri] )
//    {
//      score -= halfPenalty;
//      pawnsCount++;
//    }
//    else
//      score -= fullPenalty;
//  }
//  else
//    pawnsCount++;
//}
//
//static int pawns_count_penalty[4] = { 25, 15, 4, 0 };
//score -= pawns_count_penalty[pawnsCount & 3];
