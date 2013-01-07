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
  static const ScoreType pawnDoubled_, pawnIsolated_, pawnBackward_, openRook_, semiopenRook_, winloseBonus_;
  static const ScoreType fakecastlePenalty_;
  static const ScoreType blockedKingPenalty_;
  static const ScoreType unstoppablePawn_;
  static const ScoreType kingbishopPressure_;
  static const ScoreType bishopBonus_;
  static const ScoreType figureAgainstPawnBonus_;
  static const ScoreType pawnEndgameBonus_;
  static const ScoreType pawnPassed_[2][8], pawnGuarded_[2][8];
  static const ScoreType mobilityBonus_[8][32];
  static const ScoreType kingDistanceBonus_[8][8];
  static const ScoreType nearKingAttackBonus_[8];

  Evaluator(const Board & board);

  ScoreType operator () ();

private:

  /// calculates absolute position evaluation
  ScoreType evaluate();
  ScoreType evaluateMaterialDiff();
  ScoreType evaluateMaterialPawnDiff();
  inline ScoreType evaluateKing(Figure::Color color);
  inline ScoreType evalKingPawns(Figure::Color color, int index, int coeff, int castle);
  ScoreType evaluatePawns(Figure::Color color);
  ScoreType evalPawnsEndgame(Figure::Color color);
  ScoreType evaluateRooks(Figure::Color color);
  ScoreType evaluateWinnerLoser();

  /// calculates attacked fields (masks) and figures mobility
  void collectFieldsInfo();

  struct FieldsInfo
  {
    void reset()
    {
      attacked_ = 0;
      figuresN_ = 0;
      king_pos_ = -1;
      kingMobility_ = 0;
      kingCaps_ = 0;
      pawn_attacked_ = 0;
    }

    BitMask attacked_;
    BitMask pawn_attacked_;
    Figure::Type types_[64];
    BitMask mobility_[64];
    BitMask caps_[64];
    BitMask kingMobility_;
    BitMask kingCaps_;
    int movesN_[64];
    int kingDist_[64];
    int figuresN_;
    int king_pos_;
  } finfo_[2];

  const Board & board_;
};