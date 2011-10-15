#pragma once

#include "ChessBoard.h"
#include <vector>
#include <limits>
#include "HashTable.h"

struct CalcResult
{
  CalcResult();

  WeightType weight_;
  bool wminus_;

  int movesCount_;
  int totalMoves_;
  int forcedMoves_;
  int additionalMoves_;
  int nullMovesCount_;

  Step best_;
  StepId steps_[MaxDepth];
  int depth_;
};


class ChessAlgorithm
{
public:

  ChessAlgorithm();
  ~ChessAlgorithm();

  void enableBook(bool enable);
  void setMemory(int mb);
  void setMode(int mode);
  void setDepth(int depth);
  void init(Figure::Color = Figure::ColorWhite);
  bool calculateSteps(std::vector<Step> & steps);
  void applyStep(Step & );
  int  doBestStep(CalcResult & cres, std::ostream & sout);
  void stop();
  const Board * getCurrent() const;
  Board * getCurrent();
  const Step * lastStep() const;
  int stepsCount() const { return (positions_.size())/2; }
  void nextPos();
  void prevPos();

  bool load(std::istream &);
  void save(std::ostream &) const;

  struct Level
  {
    Level() : board_(Figure::ColorWhite) {}
    Level(const Board & board, const Step & step) : board_(board), step_(step) {}

    Board board_;
    Step  step_;
  };

private:

  struct Reps
  {
    Reps() : hcode_(0), fcount_(0), pawn_or_eat_(false) {}
    Reps(const uint64 & hc, int fc, bool e, bool p) : hcode_(hc), fcount_(fc)
    {
      pawn_or_eat_ = e || p;
    }

    const uint64 & hcode() const { return hcode_; }
    int fcount() const { return fcount_; }
    bool eat() const { return pawn_or_eat_; }

    void set(const uint64 & hc, int fc, bool e, bool p)
    {
      hcode_ = hc;
      fcount_ = fc;
      pawn_or_eat_ = e || p;
    }

    void clear()
    {
      hcode_ = 0;
      fcount_ = 0;
      pawn_or_eat_ = false;
    }

  private:

    uint64 hcode_;
    int fcount_;
    bool pawn_or_eat_;
  };

  void outputPV(CalcResult & cres, int dt, std::ostream & sout);

  WeightType alphaBetta(Board & board, Step & step, StepId * pv_steps, const Step & prev, int depth, int ply, WeightType alpha, WeightType betta, bool nullMove);
  WeightType captureForce(const Step & prev, Step & killerId, int ply, Board & board, WeightType alpha, WeightType betta);
  WeightType doNullMove(Board & board, const Step & prev, int depth, int ply, WeightType alpha, WeightType betta);

  //////////////////////////////////////////////////////////////////////////
  inline WeightType doMove(Board & board, StepId * pv_steps, const Step & prev, Step & step,
      int depth, int ply, WeightType alpha, WeightType betta, bool first, bool & valid, bool nullMove)
  {
    valid = false;

    if ( ply >= depthLimit_ )
      additionalCount_++;

    if ( ply >= MaxDepth )
      return board.evaluate();

    movesCount_++;
    Board board2(board);
    Board::State state = board2.makeStep(step);

    if ( Board::Invalid == state )
      return alpha;

    valid = true;

    repetitions_[ply+1].set(board2.hashCode(), board2.getFiguresCount(), step.rindex_ >= 0, Figure::TypePawn == (Figure::Type)step.ftype_);

    int n_reps;
    board2.setChessDraw(checkForDraw(ply+1, repetitions_[ply+1], n_reps));

    bool pv_saved = false;
    Step s;
    s.clear();

    WeightType w = -std::numeric_limits<WeightType>::max();
    if ( !board2.drawState() && Board::ChessMat != board2.getState() )
    {

      if ( board2.getState() == Board::UnderCheck )
      {
        if ( !nullMove )
          depth++;
      }
      else if ( depth == 2 )
      {
        WeightType x = board2.evaluate();
        if ( x < alpha-(Figure::figureWeight_[Figure::TypeQueen]+50) )
          return x;
      }
#ifdef USE_EXTRA_QUIS_
      else if ( depth == 2 )
      {
        WeightType x = board2.evaluate();
        if ( x >= betta + 60 )
          depth = 1;
      }
#endif

      if ( depth <= 1 )
      {
        Step ki;
        ki.clear();

        w = board2.evaluate();

        if ( ply < MaxDepth && w > alpha && w < betta+Figure::figureWeight_[Figure::TypeQueen]+50 )
        {
            WeightType w0 = w < betta ? -w : -betta;
            w = -captureForce(step, ki, ply+1, board2, w0, -alpha);
        }

        if ( ply < depthLimit_ && pv_steps && w > alpha )
        {
          pv_steps[ply] = step;
          if ( ply < MaxDepth-1 )
            pv_steps[ply+1].clear();

          pv_saved = true;
        }
        else if ( pv_saved && ply < MaxDepth && ply >= depthLimit_ )
        {
          pv_steps[ply].clear();
          pv_saved = true;
        }
      }
      else
      {
        StepId pv_steps1[MaxDepth];
        if ( ply < MaxDepth-1 )
          pv_steps1[ply+1].clear();

        WeightType alpha0 = alpha;

#ifndef DONT_USE_EXTS_
        if ( !first )
        {
          w = -alphaBetta(board2, s, pv_steps1, step, depth-1, ply+1, -(alpha+1), -alpha, nullMove);
          if ( w > alpha0 )
            alpha0 = w;
        }

        if ( first || (w > alpha && w < betta) )
#endif
          w = -alphaBetta(board2, s, pv_steps1, step, depth-1, ply+1, -betta, -alpha0, nullMove);

        if ( pv_steps && w > alpha )
        {
          pv_steps[ply] = step;
          for (int j = ply+1; j < depthLimit_; ++j)
          {
            pv_steps[j] = pv_steps1[j];
            if ( !pv_steps1[j] )
              break;
          }
          pv_saved = true;
        }
      }
    }

    if ( w == -std::numeric_limits<WeightType>::max() )
    {
      w = board2.evaluate();
    }

    if ( Figure::WeightMat == w )
      w -= ply;

    if ( pv_steps && w > alpha && !pv_saved )
    {
      pv_steps[ply] = step;
      if ( ply < MaxDepth-1 )
        pv_steps[ply+1].clear();
    }

    return w;
  }
  //////////////////////////////////////////////////////////////////////////
  WeightType doCaptureMove(Step & step, Step & killer, int ply, Board & board, WeightType alpha, WeightType betta, bool first, bool & valid)
  {
    valid = false;

    forcedCount_++;
    movesCount_++;

    Board board2(board);
    Board::State state = board2.makeStep(step);

    if ( Board::Invalid == state )
      return alpha;

    valid = true;

    WeightType w = board2.evaluate();
    if ( board2.drawState() || Board::ChessMat == board2.getState() )
      return w;

    if ( w >= betta+Figure::figureWeight_[Figure::TypeQueen]+50 )
      return w;

    if ( ply < MaxDepth && w > alpha )
    {
      WeightType w0 = w < betta ? -w : -betta;
      WeightType alpha0 = alpha;
#ifndef DONT_USE_EXTS_
      if ( !first )
      {
        w = -captureForce(step, killer, ply+1, board2, -(alpha+1), -alpha);
        if ( w > alpha0 )
          alpha0 = w;
      }

      if ( first || (w > alpha && w < betta) )
#endif
        w = -captureForce(step, killer, ply+1, board2, w0, -alpha0);
    }

    return w;
  }
  //////////////////////////////////////////////////////////////////////////
  Board::State checkForDraw(int ply, const Reps & rcur, int & reps) const
  {
    reps = checkForRepetitions(ply, rcur);
    if ( reps >= 3 )
      return Board::DrawReps;

    if ( checkFiftySteps(ply) )
      return Board::Draw50Moves;

    return Board::Invalid;
  }

  int checkForRepetitions(int ply, const Reps & rcur) const
  {
    int reps = 1;
    ply -= 2;
    for (; ply > 0; ply -= 2)
    {
      const Reps & rep = repetitions_[ply];
      if ( rep.fcount() > rcur.fcount() )
        return reps;
      if ( repetitions_[ply].hcode() == rcur.hcode() )
        reps++;
      if ( reps >= 3 )
        return reps;
    }
    ply = currentIdx_ + ply;
    for (; ply >= 0; ply -= 2)
    {
      if ( positions_[ply].board_.getFiguresCount() > rcur.fcount() )
        return reps;
      if ( positions_[ply].board_.hashCode() == rcur.hcode() )
        reps++;
      if ( reps >= 3 )
        return reps;
    }
    return reps;
  }

  bool checkFiftySteps(int ply) const
  {
    int num = currentIdx_ - lastEatNum_;
    if ( num >= 100 )
      return true;

    if ( ply <= 0 || ply + num < 100 )
      return false;

    int eatIndex = -1;
    for ( ; ply > 0; --ply)
    {
      if ( repetitions_[ply].eat() )
      {
        eatIndex = ply;
        break;
      }
    }

    if ( eatIndex >= 0 )
      return false;

    return num + ply >= 100;
  }

  //////////////////////////////////////////////////////////////////////////
  WeightType processHitem(Board & board, const Step & prev, Step & step, StepId * pv_steps,
	Step & hstep, int depth, int ply,
    WeightType alpha, WeightType betta, bool & retAlpha, bool & valid, bool nullMove)
  {
    valid = false;

    HashTable::HItem & hitem = htable_[board.hashCode()];
    if ( hitem.hcode_ != board.hashCode() )
      return alpha;

    THROW_IF( (Figure::Color)hitem.color_ != board.getColor(), "colors in hash are different" );

    if ( HashTable::Alpha != hitem.flag_ )
    {
      board.stepById(hitem.stepId_, hstep);
      valid = hstep;

      THROW_IF(hstep.invalid(), "invalid hstep");
    }

    if ( hitem.depth_ >= depth )
    {
      WeightType hw = hitem.weight_;
      if ( hw >= Figure::WeightMat-MaxDepth )
      {
        hw += hitem.ply_;
        hw -= ply;
      }
      else if ( hw <= MaxDepth-Figure::WeightMat )
      {
        hw -= hitem.ply_;
        hw += ply;
      }

      switch ( hitem.flag_ )
      {
      case HashTable::Alpha:
        {
          if ( alpha >= hw )
          {
            retAlpha = true;
            return alpha;
          }
        }
        break;

      case HashTable::Betta:
      case HashTable::AlphaBetta:
        {
          if ( hw > alpha && hstep )
          {
            if ( ply >= depthLimit_ )
              additionalCount_++;

            movesCount_++;
            Board board2(board);
            Board::State state = board2.makeStep(hstep);

            if ( Board::Invalid != state )
            {
              repetitions_[ply+1].set(board2.hashCode(), board2.getFiguresCount(), hstep.rindex_ >= 0, Figure::TypePawn == (Figure::Type)hstep.ftype_);

              int n_reps;
              board2.setChessDraw(checkForDraw(ply+1, repetitions_[ply+1], n_reps));

              // if reps == 2 we have to check an answer to be sure that there is no draw
              if ( n_reps < 2 )
              {
                if ( board2.drawState() || Board::ChessMat == board2.getState() )
                {
                  hw = board2.evaluate();
                }

                if ( Figure::WeightMat == hw )
                  hw -= ply;

                hstep.weight_ = hw;

                if ( hw >= betta )
                {
                  step = hstep;
                  if ( pv_steps )
                  {
                    pv_steps[ply] = hstep;				
                    if ( ply < MaxDepth-1 )
                      pv_steps[ply+1].clear();
                  }
                  alpha = hw;
                  return alpha;
                }
              }
            }
            else
              valid = false;
          }
        }
        break;
      }
    }

    return alpha;
  }

  WeightType capturesHitem(Board & board, Step & killer, int ply, Step & step, Step & hstep,
    WeightType alpha, WeightType betta, bool & retAlpha, bool & valid)
  {
    valid = false;

    HashTable::HItem & hitem = capturesHash_[board.hashCode()];
    if ( hitem.hcode_ != board.hashCode() )
      return alpha;

    THROW_IF( (Figure::Color)hitem.color_ != board.getColor(), "colors in hash are different" );

    if ( HashTable::Alpha != hitem.flag_ )
    {
      board.stepById(hitem.stepId_, hstep);
      valid = hstep;

      THROW_IF(hstep.invalid(), "invalid hstep");
    }

    WeightType hw = hitem.weight_;

    switch ( hitem.flag_ )
    {
    case HashTable::Alpha:
      {
        if ( alpha >= hw )
        {
          retAlpha = true;
          return alpha;
        }
      }
      break;

    case HashTable::Betta:
    case HashTable::AlphaBetta:
      {
        if ( hw > alpha && hstep )
        {
          movesCount_++;
          Board board2(board);
          Board::State state = board2.makeStep(hstep);

          if ( Board::Invalid != state )
          {
            if ( board2.drawState() || Board::ChessMat == board2.getState() )
            {
              hw = board2.evaluate();
            }

            hstep.weight_ = hw;

            if ( hw >= betta )
            {
              step = hstep;
              alpha = hw;
              return alpha;
            }
          }
          else
            valid = false;
        }
      }
      break;
    }

    if ( !hstep || !valid )
      return alpha;

    WeightType w = doCaptureMove(hstep, killer, ply, board, alpha, betta, true, valid);
    if ( !valid )
      return alpha;

    hstep.weight_ = w;

    if ( w > alpha )
    {
      step = hstep;
      alpha = w;
    }

    return alpha;
  }

  //////////////////////////////////////////////////////////////////////////
  void updateLastEat();
  //////////////////////////////////////////////////////////////////////////

  typedef std::vector<Level> Levels;

  Levels positions_;
  size_t currentIdx_;

  Reps repetitions_[256];
  int depthMax_, depthLimit_;

  HashTable htable_, capturesHash_;

  int  movesCount_, forcedCount_, additionalCount_, nullMovesCount_;

  Step killers_[MaxDepth];

  void setStopFlag(bool);
  void resetStopFlags();

  volatile bool stopMe_, stopAfterMove_;

  int mode_;
  int lastEatNum_;

  bool bookEnabled_;

  DebutsTable debutsTable_;
};