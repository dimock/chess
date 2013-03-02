#pragma once
/*************************************************************
  Evaluator.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Board.h"

class Evaluator
{

  enum GamePhase { Opening = 0, MiddleGame, EndGame };

public:

  static int score_ex_max_;

  // position evaluation. 0 - opening, 1 - endgame; color,type,pos
  static const ScoreType positionEvaluations_[2][8][64];

  static const ScoreType positionGain_;
  static const ScoreType lazyThreshold_, lazyThresholdEg_;

  // evaluation constants
  static const ScoreType bishopKnightMat_[64];
  static const ScoreType pawnDoubled_, pawnIsolated_, pawnBackward_, pawnDisconnected_, pawnBlocked_, defendedBonus_;
  static const ScoreType groupsPenalty_;
  static const ScoreType assistantBishop_, rookBehindBonus_;
  static const ScoreType semiopenRook_, openRook_, winloseBonus_;
  static const ScoreType fakecastlePenalty_;
  static const ScoreType castleImpossiblePenalty_;
  static const ScoreType bishopBonus_;
  static const ScoreType pawnEndgameBonus_;
  static const ScoreType unstoppablePawn_;
  static const ScoreType kingFarBonus_;
  static const ScoreType pawnPassed_[8], defendedPasser_[8], passerCandidate_[8], pawnCanGo_[8];
  static const ScoreType passersGroup_[8];
  static const ScoreType mobilityBonus_[8][32];
  static const ScoreType kingDistanceBonus_[8][8];
  static const ScoreType attackedByWeakBonus_;
  static const ScoreType forkBonus_;
  static const ScoreType fianchettoBonus_;
  static const ScoreType rookToKingBonus_;

  /// material difference
  static const ScoreType figureAgainstPawnBonus_[16];
  static const ScoreType rookAgainstFigureBonus_[16];

  // blocked bishop & knight
  static const ScoreType bishopBlocked_;
  static const ScoreType knightBlocked_;

  // pinned
  static const ScoreType pinnedKnight_;
  static const ScoreType pinnedBishop_;
  static const ScoreType pinnedRook_;

  // pawns shield. penalty for absent pawn
  static const ScoreType cf_columnOpened_;
  static const ScoreType bg_columnOpened_;
  static const ScoreType ah_columnOpened_;

  static const ScoreType cf_columnSemiopened_;
  static const ScoreType bg_columnSemiopened_;
  static const ScoreType ah_columnSemiopened_;

  static const ScoreType cf_columnCracked_;
  static const ScoreType bg_columnCracked_;
  static const ScoreType ah_columnCracked_;

  static const ScoreType pawnBeforeKing_;

  // pressure to opponent's king
  static const ScoreType kingPawnPressure_;
  static const ScoreType kingKnightPressure_;
  static const ScoreType kingBishopPressure_;
  static const ScoreType kingRookPressure_;
  static const ScoreType kingQueenPressure_;
  static const ScoreType kingFieldAttackBonus_;
  
  Evaluator();

  void initialize(const Board * board, EHashTable * ehash);

  ScoreType operator () (ScoreType alpha, ScoreType betta);

private:

  void prepare();

  /// calculates absolute position evaluation
  ScoreType evaluate();

  ScoreType evaluatePawns(Figure::Color color, ScoreType * score_eg);
  ScoreType evaluatePawnShield(Figure::Color color);
  ScoreType evaluatePasserAdditional(Figure::Color color, ScoreType & pw_score_eg);

  ScoreType evaluateBlockedBishops();
  ScoreType evaluateBlockedKnights();

  ScoreType evaluateMaterialDiff();
  ScoreType evaluateCastlePenalty(Figure::Color color);
  ScoreType evaluateFianchetto() const;

  ScoreType analyzePasserGroup(ScoreType * score_eg, Figure::Color color, int (&group_x)[8], int n, int (&passers_y)[8]);

  // search path from opponent king to pawn's promotion of given color
  bool findRootToPawn(Figure::Color color, int promo_pos) const;

  // special cases

  enum SpecialCases
  {
    SC_None, SC_RBR_W, SC_RNR_W, SC_RBR_B, SC_RNR_B
  };

  SpecialCases findSpecialCase() const;
  ScoreType evaluateSpecial(SpecialCases sc) const;
  ScoreType evaluateWinnerLoser();

  // multiple coefficients for opening/endgame
  GamePhase detectPhase(int & coef_o, int & coef_e);

  // calculate or take from hash - pawns structure for middle & end game; king's pawn shield
  void hashedEvaluation(ScoreType & pwscore, ScoreType & pwscore_eg, ScoreType & score_ps);

  // 0 - short, 1 - long, -1 - no castle
  int getCastleType(Figure::Color color) const;

  /// bishops mobility and attacks
  ScoreType evaluateBishops();

  /// calculate field (bit-mask) attacked by knights, and mobility
  ScoreType evaluateKnights();

  // evaluate rooks mobility and open column
  void evaluateRooks();

  // queens mobility
  void evaluateQueens();

  /// find knight and pawn forks
  ScoreType evaluateForks(Figure::Color color);

  /// lazy eval.
  ScoreType evaluateExpensive(int coef_o, int coef_e);

  struct FieldsInfo
  {
    void reset()
    {
      king_pos_ = -1;
      
      pw_attack_mask_ = 0;
      kn_attack_mask_ = 0;
      attack_mask_ = 0;

      knightMobility_ = 0;
      bishopMobility_ = 0;
      rookMobility_ = 0;
      queenMobility_ = 0;

      knightPressure_ = 0;
      bishopPressure_ = 0;
      rookPressure_ = 0;
      queenPressure_ = 0;

      rookOpenScore_ = 0;
      kingAttackersN_ = 0;
    }

    int king_pos_;
    ScoreType knightMobility_, bishopMobility_, rookMobility_, queenMobility_;
    ScoreType knightPressure_, bishopPressure_, rookPressure_, queenPressure_;
    ScoreType rookOpenScore_;
    BitMask pw_attack_mask_;
    BitMask kn_attack_mask_;
    BitMask attack_mask_;

    // attacking line 0
    int kingAttackersN_;
  } finfo_[2];

  template <int DIR>
  void mobility_masks(int from, BitMask & mob_mask, BitMask & att_mask, const BitMask & di_mask) const
  {
  }

  template <>
  void mobility_masks<0>(int from, BitMask & mob_mask, BitMask & att_mask, const BitMask & di_mask) const
  {
    BitMask mask_from = di_mask & mask_all_;
    if ( mask_from )
    {
      int to = find_lsb(mask_from);
      mob_mask |= board_->g_betweenMasks->between(from, to);
      att_mask |= mob_mask | set_mask_bit(to);
    }
    else
    {
      mob_mask |= di_mask;
      att_mask |= di_mask;
    }
  }

  template <>
  void mobility_masks<1>(int from, BitMask & mob_mask, BitMask & att_mask, const BitMask & di_mask) const
  {
    BitMask mask_from = di_mask & mask_all_;
    if ( mask_from )
    {
      int to = find_msb(mask_from);
      mob_mask |= board_->g_betweenMasks->between(from, to);
      att_mask |= mob_mask | set_mask_bit(to);
    }
    else
    {
      mob_mask |= di_mask;
      att_mask |= di_mask;
    }
  }

  // used to find pinned figures
  enum PinType { ptAll, ptOrtho, ptDiag };
  // acolor - color of attacking side, ki_pos - attacked king pos
  bool discoveredCheck(int pt, Figure::Color acolor, const BitMask & brq_mask, int ki_pos, enum PinType pinType) const;


  // sum of weights of all figures
  int weightMax_;

  const Board * board_;
  EHashTable  * ehash_;

  BitMask mask_all_;
  BitMask inv_mask_all_;
  ScoreType alpha_, betta_;
};