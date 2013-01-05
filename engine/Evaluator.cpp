/*************************************************************
  Evaluator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Evaluator.h"
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

const ScoreType Evaluator::positionGain_ = 100;


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
const ScoreType Evaluator::pawnBackward_ = -15;
const ScoreType Evaluator::openRook_     =  12;
const ScoreType Evaluator::semiopenRook_ =  6;
const ScoreType Evaluator::winloseBonus_ =  20;
const ScoreType Evaluator::kingbishopPressure_ = 15;
const ScoreType Evaluator::bishopBonus_ = 10;
const ScoreType Evaluator::figureAgainstPawnBonus_ = 10;
const ScoreType Evaluator::pawnEndgameBonus_ = 20;
const ScoreType Evaluator::fakecastlePenalty_ = 20;
const ScoreType Evaluator::unstoppablePawn_ = 60;

#define MAX_PASSED_SCORE 80

const ScoreType Evaluator::pawnPassed_[2][8] = {
  { 0, MAX_PASSED_SCORE, 60, 40, 20, 10, 5, 0 },
  { 0, 5, 10, 20, 40, 60, MAX_PASSED_SCORE, 0 }
};

const ScoreType Evaluator::pawnGuarded_[2][8] = {
  { 0, 20, 12, 10, 6, 4, 0, 0 },
  { 0, 0, 4, 6, 10, 12, 20, 0 },
};

const ScoreType Evaluator::mobilityBonus_[8][32] = {
  {},
  {},
  {-30, -10, -5, 0, 3, 5, 6, 7},
  {-30, -10, -5, 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
  {-15, -5, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14},
  {-50, -20, -5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
};

const ScoreType Evaluator::kingDistanceBonus_[8][8] = {
  {},
  {},
  {8, 8, 8, 7, 5, 1, 0, 0},
  {10, 10, 8, 7, 6, 3, 1, 0},
  {15, 15, 10, 7, 6, 3, 1, 0},
  {25, 25, 20, 10, 7, 3, 1, 0},
};
const ScoreType Evaluator::nearKingAttackBonus_[8] = {
  0, 5, 10, 15, 20, 30, 40, 50
};


//////////////////////////////////////////////////////////////////////////
Evaluator::Evaluator(const Board & board) : board_(board)
{
}

ScoreType Evaluator::operator () ()
{
  ScoreType score = -std::numeric_limits<ScoreType>::max();

  if ( board_.isWinnerLoser() )
    score = evaluateWinnerLoser();
  else
    score = evaluate();

  if ( Figure::ColorBlack  == board_.getColor() )
    score = -score;

  return score;
}
//////////////////////////////////////////////////////////////////////////
ScoreType Evaluator::evaluate()
{
  const FiguresManager & fmgr = board_.fmgr();

  ScoreType score = fmgr.weight();

  score += evaluateMaterialDiff();

  collectFieldsInfo();

  ScoreType scores_bw[2] = {};
  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color = (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor(color);

    BitMask opw_mask = board_.fmgr().pawn_mask_o(ocolor);
    opw_mask &= finfo_[c].attacked_;
    int pw_attacked_num = pop_count(opw_mask);
    scores_bw[c] += pw_attacked_num*3;

    int oki_pos = board_.kingPos(ocolor);
    BitMask oki_mask = board_.g_movesTable->caps(Figure::TypeKing, oki_pos);
    oki_mask &= finfo_[c].attacked_;
    int oki_num = pop_count(oki_mask);
    scores_bw[c] += nearKingAttackBonus_[oki_num];

    for(int i = 0; i < finfo_[c].figuresN_; ++i)
    {
      Figure::Type t = finfo_[c].types_[i];
      scores_bw[c] += mobilityBonus_[t][finfo_[c].movesN_[i] & 31];
      scores_bw[c] += kingDistanceBonus_[t][finfo_[c].kingDist_[i] & 7];
    }
  }

  score -= scores_bw[0];
  score += scores_bw[1];

  int wei[2] = {0, 0};
  for (int c = 0; c < 2; ++c)
  {
    for (int t = Figure::TypeKnight; t < Figure::TypeKing; ++t)
      wei[c] += fmgr.tcount((Figure::Type)t, (Figure::Color)c)*Figure::figureWeight_[t];
  }

  if ( wei[0] > 2*Figure::figureWeight_[Figure::TypeQueen] &&
       wei[1] > 2*Figure::figureWeight_[Figure::TypeQueen] )
  {
    score -= fmgr.eval(Figure::ColorBlack, 0);
    score += fmgr.eval(Figure::ColorWhite, 0);

    score -= evaluatePawns(Figure::ColorBlack);
    score += evaluatePawns(Figure::ColorWhite);

    score -= evaluateRooks(Figure::ColorBlack);
    score += evaluateRooks(Figure::ColorWhite);

    score -= evaluateKing(Figure::ColorBlack);
    score += evaluateKing(Figure::ColorWhite);

    score += evaluateMaterialPawnDiff();
  }
  else if ( wei[0] < Figure::figureWeight_[Figure::TypeQueen] &&
            wei[1] < Figure::figureWeight_[Figure::TypeQueen]  )
  {
    score -= fmgr.eval(Figure::ColorBlack, 1);
    score += fmgr.eval(Figure::ColorWhite, 1);

    score -= evaluatePawns(Figure::ColorBlack);
    score += evaluatePawns(Figure::ColorWhite);

    score -= evalPawnsEndgame(Figure::ColorBlack);
    score += evalPawnsEndgame(Figure::ColorWhite);
  }
  else
  {
    score -= evaluatePawns(Figure::ColorBlack);
    score += evaluatePawns(Figure::ColorWhite);

    int score0 = 0;
    int score1 = 0;

    score0 -= fmgr.eval(Figure::ColorBlack, 0);
    score0 += fmgr.eval(Figure::ColorWhite, 0);

    score0 -= evaluateRooks(Figure::ColorBlack);
    score0 += evaluateRooks(Figure::ColorWhite);

    score0 -= evaluateKing(Figure::ColorBlack);
    score0 += evaluateKing(Figure::ColorWhite);

    score0 += evaluateMaterialPawnDiff();

    score1 -= fmgr.eval(Figure::ColorBlack, 1);
    score1 += fmgr.eval(Figure::ColorWhite, 1);

    score1 -= evalPawnsEndgame(Figure::ColorBlack);
    score1 += evalPawnsEndgame(Figure::ColorWhite);

    static const int wei_max = 2*(Figure::figureWeight_[Figure::TypeQueen] +
      2*Figure::figureWeight_[Figure::TypeRook] + 2*Figure::figureWeight_[Figure::TypeBishop] + 2*Figure::figureWeight_[Figure::TypeKnight]);

    int w = wei[0] + wei[1];
    if ( w > wei_max )
      w = wei_max;
    int w1 = wei_max - w;
    int s = (score0 * w + score1 * w1) / wei_max;
    score += s;
  }

  return score;
}

void Evaluator::collectFieldsInfo()
{
  BitMask mask_all = board_.fmgr().mask(Figure::ColorWhite) | board_.fmgr().mask(Figure::ColorBlack);
  BitMask inv_mask_all = ~mask_all;

  // 1. Pawns
  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color = (Figure::Color)c;
    finfo_[c].reset();

    const BitMask & pawn_msk = board_.fmgr().pawn_mask_o(color);
    if ( color )
      finfo_[c].attacked_ = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
    else
      finfo_[c].attacked_ = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);
  }

  // 2. Knights
  BitMask kn_caps[2] = {};
  for (int c = 0; c < 2; ++c)
  {
    int & num = finfo_[c].figuresN_;
    Figure::Color color = (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor(color);
    BitMask kn_mask = board_.fmgr().knight_mask(color);
    for ( ; kn_mask; )
    {
      int n = clear_lsb(kn_mask);
      const BitMask & kn_cap = board_.g_movesTable->caps(Figure::TypeKnight, n);
      kn_caps[c] |= kn_cap;

      finfo_[c].types_[num] = Figure::TypeKnight;
      BitMask & mob_mask = finfo_[c].mobility_[num++];
      mob_mask = kn_cap & ~finfo_[ocolor].attacked_ & inv_mask_all;
    }
  }

  // 3. Bishops - Rooks - Queens
  static const nst::dirs t_dirs[4][8] = { {nst::nw, nst::ne, nst::se, nst::sw},
    {nst::no, nst::ea, nst::so, nst::we},
    {nst::nw, nst::no, nst::ne, nst::ea, nst::se, nst::so, nst::sw, nst::we} };

  static const int t_dirs_n[4] = { 4, 4, 8 };

  for (int t = Figure::TypeBishop; t <= Figure::TypeQueen; ++t)
  {
    BitMask t_caps[2] = {};

    for (int c = 0; c < 2; ++c)
    {
      int & num = finfo_[c].figuresN_;
      Figure::Color color = (Figure::Color)c;
      Figure::Color ocolor = Figure::otherColor(color);
      BitMask t_mask = board_.fmgr().type_mask((Figure::Type)t, color);
      int oki_pos = board_.kingPos(ocolor);
      for ( ; t_mask; )
      {
        int from = clear_lsb(t_mask);
        finfo_[c].kingDist_[num] = board_.g_distanceCounter->getDistance(from, oki_pos);
        finfo_[c].types_[num] = (Figure::Type)t;
        BitMask & mob_mask = finfo_[c].mobility_[num++];

        int n = t_dirs_n[t - Figure::TypeBishop];
        for (int i = 0; i < n; ++i)
        {
          nst::dirs dir = t_dirs[t - Figure::TypeBishop][i];
          const BitMask & di_mask = board_.g_betweenMasks->from_dir(from, dir);
          BitMask msk_from = di_mask & inv_mask_all;
          if ( msk_from )
          {
            int to = nst::get_bit_dir_[dir] ? find_lsb(msk_from) : find_msb(msk_from);
            const BitMask & btw_mask = board_.g_betweenMasks->between(from, to);
            mob_mask = btw_mask;
            t_caps[c] |= btw_mask;
            t_caps[c] |= set_mask_bit(to);
          }
          else
          {
            mob_mask = di_mask;
            t_caps[c] |= di_mask;
          }
        }

        mob_mask &= ~finfo_[ocolor].attacked_;
      }
    }

    if ( t == Figure::TypeBishop )
    {
      finfo_[0].attacked_ |= kn_caps[0];
      finfo_[1].attacked_ |= kn_caps[1];
    }

    finfo_[0].attacked_ |= t_caps[0];
    finfo_[1].attacked_ |= t_caps[1];
  }

  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color = (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor(color);

    BitMask o_attacked = finfo_[ocolor].attacked_ & ~finfo_[color].attacked_;

    for (int i = 0; i < finfo_[c].figuresN_; ++i)
    {
      BitMask & mob_mask = finfo_[c].mobility_[i];
      mob_mask &= ~o_attacked;
      finfo_[c].movesN_[i] = pop_count(mob_mask);
    }
  }
}

ScoreType Evaluator::evaluateMaterialDiff()
{
  ScoreType score = 0;

  const FiguresManager & fmgr = board_.fmgr();

  Figure::Color color = board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);
  
  // 1. Knight - Bishop disbalance
  score = (fmgr.bishops(Figure::ColorWhite) - fmgr.bishops(Figure::ColorBlack))*bishopBonus_;

  //// have 2 bishops with different field colors
  //if ( fmgr.bishops_b(Figure::ColorWhite) > 0 && fmgr.bishops_w(Figure::ColorWhite) > 0 )
  //  score += bishopBonus_;
  //if ( fmgr.bishops_b(Figure::ColorBlack) > 0 && fmgr.bishops_w(Figure::ColorBlack) > 0 )
  //  score -= bishopBonus_;

  return score;
}

ScoreType Evaluator::evaluateMaterialPawnDiff()
{
  ScoreType score = 0;

  const FiguresManager & fmgr = board_.fmgr();

  Figure::Color color = board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  // 2. Knight or Bishop against 3 pawns
  int figuresDiff = (fmgr.bishops(Figure::ColorWhite)+fmgr.knights(Figure::ColorWhite)) -
    (fmgr.bishops(Figure::ColorBlack)+fmgr.knights(Figure::ColorBlack));

  int pawnsDiff = fmgr.pawns(Figure::ColorWhite) - fmgr.pawns(Figure::ColorBlack);
  int rooksDiff = fmgr.rooks(Figure::ColorWhite) - fmgr.rooks(Figure::ColorBlack);
  int queensDiff = fmgr.queens(Figure::ColorWhite) - fmgr.queens(Figure::ColorBlack);

  if ( figuresDiff*pawnsDiff < 0 && !rooksDiff && !queensDiff )
    score += figuresDiff * figureAgainstPawnBonus_;

  return score;
}

inline ScoreType Evaluator::evalKingPawns(Figure::Color color, int index, int coeff, int castle)
{
  const FiguresManager & fmgr = board_.fmgr();

  ScoreType kingEval = 0;
  static int8 pawns_x[2][4] = { {5, 6, 7, -1}, {2, 1, 0, -1} };// for left/right castle
  static uint8 pmask_king[2] = { 96, 6 };
  static uint8 opmask_king[2] = { 48, 6 };
  static uint8 shifts[2] = { 5, 1 };
  static uint8 oshifts[2] = { 4, 2 };
  static ScoreType king_penalties[2][4] = { {10, 2, 0, 0}, {10, 0, 2, 0} };
  static ScoreType king_o_penalties[2][4] = { {0, 5, 10, 10}, {0, 10, 5, 10} };

  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);

  const uint8 * pmsk  = (const uint8*)&fmgr.pawn_mask_t((Figure::Color)color);
  const uint8 * opmsk = (const uint8*)&fmgr.pawn_mask_t(ocolor);

  int x = pawns_x[castle][index];
  int m = ((pmsk[x] & pmask_king[color]) >> shifts[color]) & 3;
  kingEval -= ((king_penalties[color][m])<<(coeff));
  int o = ((opmsk[x] & opmask_king[color]) >> oshifts[color]) & 3;
  kingEval -= king_o_penalties[color][o];

  return kingEval;
}

inline ScoreType Evaluator::evaluateKing(Figure::Color color)
{
  const FiguresManager & fmgr = board_.fmgr();

  ScoreType kingEval = 0;
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);

  Index ki_pos(board_.kingPos(color));

  static int8 castle_mask[8] = { 2,2,2, 0,0, 1,1,1 }; // 1 - short (K); 2 - long (Q)
  int8 castle = castle_mask[ki_pos.x()]; // determine by king's x-position
  int8 ky = ki_pos.y();
  bool bCastle = castle && !(ky > 1 && color) && !(ky < 6 && !color);
  if ( !bCastle )
    return kingEval;

  castle--;

  static int8 pawns_x[2][4] = { {5, 6, 7, -1}, {2, 1, 0, -1} };// for left/right castle
  static uint8 pmask_king[2] = { 96, 6 };
  static uint8 opmask_king[2] = { 48, 6 };
  static uint8 shifts[2] = { 5, 1 };
  static uint8 oshifts[2] = { 4, 2 };
  static ScoreType king_penalties[2][4] = { {8, 1, 0, 0}, {8, 0, 1, 0} };
  static ScoreType king_o_penalties[2][4] = { {0, 5, 10, 10}, {0, 10, 5, 10} };

  const uint8 * pmsk  = (const uint8*)&fmgr.pawn_mask_t((Figure::Color)color);
  const uint8 * opmsk = (const uint8*)&fmgr.pawn_mask_t(ocolor);
  
  kingEval += evalKingPawns(color, 0, 0, castle);
  kingEval += evalKingPawns(color, 1, 2, castle);
  kingEval += evalKingPawns(color, 2, 2, castle);

  // penalty for fake castle
  int ry = color ? A1 : A8;
  int rp = pawns_x[castle][1], kp = pawns_x[castle][2];
  rp = rp | ry;
  kp = kp | ry;
  const Field & rfield = board_.getField(rp);
  const Field & kfield = board_.getField(kp);
  if ( kfield.type() == Figure::TypeRook ||
      (rfield.type() == Figure::TypeRook && kfield.type() != Figure::TypeKing) )
  {
    kingEval -= fakecastlePenalty_;
  }

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

  const Field & fb0 = board_.getField(bp0);
  const Field & fb1 = board_.getField(bp1);

  if ( fb0.type() == Figure::TypeBishop && fb0.color() == ocolor || fb1.type() == Figure::TypeBishop && fb1.color() == ocolor )
    kingEval -= kingbishopPressure_;

  return kingEval;
}

ScoreType Evaluator::evaluateRooks(Figure::Color color)
{
  const FiguresManager & fmgr = board_.fmgr();

	uint64 rook_mask = fmgr.rook_mask(color);
	if ( !rook_mask )
		return 0;

	Figure::Color ocolor = Figure::otherColor(color);
	const uint8 * pmsk  = (const uint8*)&fmgr.pawn_mask_t(color);
  const uint8 * opmsk = (const uint8*)&fmgr.pawn_mask_t(ocolor);

  int ok_pos = board_.kingPos(ocolor);
  int okx = ok_pos & 7;

	ScoreType score = 0;
	for ( ; rook_mask; )
	{
		int n = clear_lsb(rook_mask);

		THROW_IF( (unsigned)n > 63, "invalid rook index" );
		THROW_IF( board_.getField(n).color() != color || board_.getField(n).type() != Figure::TypeRook,
      "there should be rook on given field" );

		int x = n & 7;

    if ( !opmsk[x] )
    {
      if ( !pmsk[x]  )
        score += openRook_;
      else
        score += semiopenRook_;
    }

    if ( x == okx || x == okx-1 || x == okx+1 )
      score <<= 1;
	}

	return score;
}

ScoreType Evaluator::evaluatePawns(Figure::Color color)
{
  const FiguresManager & fmgr = board_.fmgr();

  const uint64 & pmsk = fmgr.pawn_mask_t(color);

  if ( !pmsk )
    return 0;

  ScoreType weight = 0;
  Figure::Color ocolor = Figure::otherColor(color);
  const uint64 & opmsk = fmgr.pawn_mask_t(ocolor);

  BitMask pawn_mask = fmgr.pawn_mask_o(color);
  for ( ; pawn_mask; )
  {
    int n = clear_lsb(pawn_mask);

    const uint64 & passmsk = board_.g_pawnMasks->mask_passed(color, n);
    const uint64 & blckmsk = board_.g_pawnMasks->mask_blocked(color, n);

    // passed pawn evaluation
    if ( !(opmsk & passmsk) && !(pmsk & blckmsk) )
    {
      int y = n >> 3;
      weight += pawnPassed_[color][y];

      // guarded by neighbor pawn
      const uint64 & guardmsk = board_.g_pawnMasks->mask_guarded(color, n);
      if ( pmsk & guardmsk )
        weight += pawnGuarded_[color][y];
    }

    const uint64 & isomask = board_.g_pawnMasks->mask_isolated(n & 7);
    const uint64 & bkwmask = board_.g_pawnMasks->mask_backward(n);

    // maybe isolated pawn
    if ( !(pmsk & isomask) )
      weight += pawnIsolated_;

    // if no, it maybe backward
    else if ( !(pmsk & bkwmask) )
      weight += pawnBackward_;
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

ScoreType Evaluator::evalPawnsEndgame(Figure::Color color)
{
  const FiguresManager & fmgr = board_.fmgr();


  ScoreType score = 0;
  // bonus for each pawn in endgame
  score += fmgr.pawns(color)*pawnEndgameBonus_;

  Figure::Color ocolor = Figure::otherColor(color);
  Index ki_pos(board_.kingPos(color));
  const BitMask & pmsk = fmgr.pawn_mask_t(color);
  int kx = ki_pos.x();
  int ky = ki_pos.y();
  const BitMask & opmsk = fmgr.pawn_mask_t(ocolor);
  static int promo_y[] = { 0, 7 };

  bool opawn_found = false;

  //ScoreType score_op = 0, score_my = 0;
  //int passed_dist_op = 100, passed_dist_my = 100;

  // opponent has passed pawns?
  if ( opmsk )
  {
    BitMask opawn_mask = fmgr.pawn_mask_o(ocolor);
    for ( ; opawn_mask; )
    {
      int n = clear_lsb(opawn_mask);

      const uint64 & opassmsk = board_.g_pawnMasks->mask_passed(ocolor, n);
      const uint64 & oblckmsk = board_.g_pawnMasks->mask_blocked(ocolor, n);
      if ( !(pmsk & opassmsk) && !(opmsk & oblckmsk) )
      {
        int y = n >> 3;
        int px = n & 7;
        int py = promo_y[ocolor];
        int promo_pos = px | (py<<3);
        int pawn_dist_promo = py - y;
        if ( pawn_dist_promo < 0 )
          pawn_dist_promo = -pawn_dist_promo;

        //if ( passed_dist_op > pawn_dist_promo )
        //  passed_dist_op = pawn_dist_promo;

        int dist_promo = board_.g_distanceCounter->getDistance(ki_pos, promo_pos);
        if ( board_.color_ == color )
          dist_promo--;

        if ( pawn_dist_promo < dist_promo )
          score -= unstoppablePawn_;
        else
          score -= dist_promo<<1;

        opawn_found = true;
      }
    }
  }

  //// i have passed pawns?
  //if ( pmsk )
  //{
  //  BitMask pawn_mask = fmgr.pawn_mask_o(color);
  //  for ( ; pawn_mask; )
  //  {
  //    int n = clear_lsb(pawn_mask);

  //    const uint64 & passmsk = board_.g_pawnMasks->mask_passed(color, n);
  //    const uint64 & blckmsk = board_.g_pawnMasks->mask_blocked(color, n);
  //    if ( !(pmsk & passmsk) )
  //    {
  //      int px = n & 7;
  //      int y = n >> 3;
  //      int py = promo_y[color];
  //      int promo_pos = px | (py<<3);
  //      int pawn_dist_promo = py - y;
  //      if ( pawn_dist_promo < 0 )
  //        pawn_dist_promo = -pawn_dist_promo;

  //      if ( passed_dist_my > pawn_dist_promo )
  //        passed_dist_my = pawn_dist_promo;

  //      int dist_promo = board_.g_distanceCounter->getDistance(ki_pos, promo_pos);
  //      score_my -= dist_promo<<1;
  //    }
  //  }
  //}

  //if ( !opawn_found )
  //  score = score_my;
  //else
  //  score_my = score_op;

  //if ( passed_dist_my != 100 && !opawn_found )
  //{
  //  if ( color == board_.color_ )
  //    passed_dist_my--;
  //  else
  //    passed_dist_op--;

  //  if ( passed_dist_my < passed_dist_op )
  //    score += 20;
  //}

  return score;
}


ScoreType Evaluator::evaluateWinnerLoser()
{
  const FiguresManager & fmgr = board_.fmgr();

  Figure::Color win_color = board_.can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  ScoreType weight = fmgr.weight(win_color);

  // bonus for pawns
  weight += fmgr.pawns(win_color)*pawnEndgameBonus_;

  Index king_pos_w = board_.kingPos(win_color);
  Index king_pos_l = board_.kingPos(lose_color);

  bool eval_pawns = true;

  if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) > 0 )
  {
    int num_lose_figs = fmgr.knights(lose_color) + fmgr.bishops(lose_color);
    ScoreType weight_lose_fig = 10;

    // if winner has more pawns than loser and also has some figure he must exchange all loser figures to pawns
    if ( fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 0  &&
         fmgr.knights(win_color) +fmgr.bishops(win_color) > 0 &&
         fmgr.knights(lose_color)+fmgr.bishops(lose_color) < fmgr.pawns(win_color) )
    {
      weight_lose_fig = Figure::figureWeight_[Figure::TypePawn] + (MAX_PASSED_SCORE);
      eval_pawns = false;
    }

    weight -= num_lose_figs * weight_lose_fig;
  }
  else
    weight -= fmgr.weight(lose_color);

  // add small bonus for winner-loser state to force it
  weight += winloseBonus_;

  // BN-mat case
  if ( fmgr.weight(lose_color) == 0 && fmgr.knights(win_color) == 1 && fmgr.bishops(win_color) == 1 &&
       fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 0 )
  {
    int dist  = board_.g_distanceCounter->getDistance(king_pos_w, king_pos_l);
    weight -= dist;

    int kp = king_pos_l;
    if ( fmgr.bishops_w(win_color) )
    {
      int kx = king_pos_l.x();
      int ky = king_pos_l.y();
      kp = ((7-ky)<<3)| kx;
    }
    weight += bishopKnightMat_[kp];

    uint64 n_mask = fmgr.knight_mask(win_color);
    int np = clear_lsb(n_mask);
    THROW_IF( (unsigned)np > 63, "no knigt found" );
    int ndist = board_.g_distanceCounter->getDistance(np, king_pos_l);
    weight -= ndist >> 1;

    // add more bonus to be sure that we go to this state
    weight += winloseBonus_;
  }
  else
  {
    // some special almost-draw cases
    if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 0 &&
         fmgr.weight(win_color)-fmgr.weight(lose_color) < Figure::figureWeight_[Figure::TypeBishop]+Figure::figureWeight_[Figure::TypeKnight] )
    {
      weight = 10;
    }
    else if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 1 &&
              fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 0 )
    {
      if ( fmgr.knights(win_color)+fmgr.bishops(win_color) <= fmgr.knights(lose_color)+fmgr.bishops(lose_color) )
      {
        weight = (MAX_PASSED_SCORE);
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
            weight = 10;
        }
        else if ( board_.color_ == win_color )
          weight = Figure::figureWeight_[Figure::TypePawn];
        weight += y << 1;
        eval_pawns = false;
      }
    }
    else if ( fmgr.queens(win_color) == 0 && fmgr.bishops(win_color) == 0 &&
              fmgr.knights(win_color) == 0 && fmgr.pawns(win_color) == 0 && fmgr.rooks(win_color) == 1 &&
              fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 0 )
    {
      if ( fmgr.knights(lose_color)+fmgr.bishops(lose_color) == 1 )
        weight = fmgr.weight();
      else
        weight = 15;
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

      int wk_pr_dist = board_.g_distanceCounter->getDistance(king_pos_w, pr_pos);
      int lk_pr_dist = board_.g_distanceCounter->getDistance(king_pos_l, pr_pos);

      int wdist = board_.g_distanceCounter->getDistance(king_pos_w, pp);
      int ldist = board_.g_distanceCounter->getDistance(king_pos_l, pp);

      int wudist = board_.g_distanceCounter->getDistance(king_pos_w, pp_under);
      int ludist = board_.g_distanceCounter->getDistance(king_pos_l, pp_under);

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
               ( (wudist > ludist || wudist == ludist && lose_color == board_.color_) && y >= ykw ||
                 (wudist > ludist+1 && y < ykw) ) )
            almost_draw = true;
        }

        if ( almost_draw )
        {
          weight = 30 + (y<<1);
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
      int dist  = board_.g_distanceCounter->getDistance(king_pos_w, king_pos_l);
      weight -= dist << 1;
      weight -= positionEvaluation(1, lose_color, Figure::TypeKing, king_pos_l);
    }
  }

  if ( win_color == Figure::ColorBlack )
    weight = -weight;

  if ( eval_pawns )
  {
    if ( fmgr.pawns(Figure::ColorBlack) > 0 )
      weight -= evaluatePawns(Figure::ColorBlack);

    if ( fmgr.pawns(Figure::ColorWhite) > 0 )
      weight += evaluatePawns(Figure::ColorWhite);
  }

  return weight;
}
