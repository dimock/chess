#pragma once
/*************************************************************
  Evaluator.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Board.h"

class Evaluator
{
public:

  static const ScoreType positionGain_;

  // position evaluation. 0 - opening, 1 - endgame; color,type,pos
  static const ScoreType positionEvaluations_[2][8][64];
  static const ScoreType bishopKnightMat_[64];
  static const ScoreType pawnDoubled_, pawnIsolated_, pawnBackward_, pawnDisconnected_, pawnBlocked_;
  static const ScoreType assistantBishop_, rookBehindPenalty_;
  static const ScoreType semiopenRook_, winloseBonus_;
  static const ScoreType fakecastlePenalty_;
  static const ScoreType castleImpossiblePenalty_;
  static const ScoreType blockedKingPenalty_;
  static const ScoreType unstoppablePawn_;
  static const ScoreType kingbishopPressure_;
  static const ScoreType bishopBonus_;
  static const ScoreType figureAgainstPawnBonus_;
  static const ScoreType rookAgainstFigureBonus_;
  static const ScoreType pawnEndgameBonus_;
  static const ScoreType pawnPassed_[2][8], pawnGuarded_[2][8];
  static const ScoreType mobilityBonus_[8][32];
  static const ScoreType kingDistanceBonus_[8][8];
  static const ScoreType nearKingAttackBonus_[8];
  static const ScoreType attackedByWeakBonus_;
  static const ScoreType forkBonus_;
  static const ScoreType fianchettoBonus_;

  static const ScoreType rookToKingBonus_;

  // pawns shield. penalty for absent pawn
  static const ScoreType cf_columnOpened_;
  static const ScoreType bg_columnOpened_;
  static const ScoreType ah_columnOpened_;

  static const ScoreType cf_columnSemiopened_;
  static const ScoreType bg_columnSemiopened_;
  static const ScoreType ah_columnSemiopened_;

  static const ScoreType opponentPawnsToKing_;

  Evaluator(const Board & board, EHashTable * ehash);

  ScoreType operator () ();

private:

  /// calculates absolute position evaluation
  ScoreType evaluate();
  ScoreType evaluateMaterialDiff();
  ScoreType evaluatePawnShield(Figure::Color color);
  ScoreType evaluateCastlePenalty(Figure::Color color);
  ScoreType evalKingPawns(Figure::Color color, int index, int coeff, int castle);
  ScoreType evaluatePawns(Figure::Color color, ScoreType * score_eg);
  ScoreType evaluateFianchetto() const;
  ScoreType evalPawnsEndgame(Figure::Color color);
  ScoreType evaluateRooks(Figure::Color color);
  ScoreType evaluateWinnerLoser();

  // 0 - short, 1 - long, -1 - no castle
  int getCastleType(Figure::Color color) const;

  /// calculates figures mobility
  void calculateMobility();

  /// find knight and pawn forks
  ScoreType evaluateForks(Figure::Color color);

  struct FieldsInfo
  {
    void reset()
    {
      king_pos_ = -1;
      pawn_attacked_ = 0;
      mobilityBonus_ = 0;
      kingPressureBonus_ = 0;
      kn_caps_ = 0;
    }

    int king_pos_;
    ScoreType mobilityBonus_;
    ScoreType kingPressureBonus_;
    BitMask pawn_attacked_;
    BitMask kn_caps_;
  } finfo_[2];

  const Board & board_;
  EHashTable * ehash_;
};