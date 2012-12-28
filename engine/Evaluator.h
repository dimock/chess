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
  static const ScoreType unstoppablePawn_;
  static const ScoreType kingbishopPressure_;
  static const ScoreType bishopBonus_;
  static const ScoreType figureAgainstPawnBonus_;
  static const ScoreType pawnEndgameBonus_;
  static const ScoreType pawnPassed_[2][8], pawnGuarded_[2][8];

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


  const Board & board_;
};