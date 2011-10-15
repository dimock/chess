#include "ChessAlg.h"
#include "Helpers.h"
#include <string>
#include <time.h>

using namespace std;

#undef SAVE_HTABLE_

CalcResult::CalcResult() :
  weight_(0), wminus_(false), movesCount_(0), totalMoves_(0), forcedMoves_(0), additionalMoves_(0), nullMovesCount_(0), depth_(0)
{
  best_.clear();

  for (int i = 0; i < sizeof(steps_)/sizeof(steps_[0]); ++i)
    steps_[i].clear();
}


ChessAlgorithm::ChessAlgorithm() : bookEnabled_(true), mode_(0), currentIdx_((size_t)-1), depthMax_(16), htable_(22), capturesHash_(22), stopMe_(false), stopAfterMove_(false), lastEatNum_(0)
{
  debutsTable_.readTable("debut.tbl");
}

ChessAlgorithm::~ChessAlgorithm()
{
}

void ChessAlgorithm::enableBook(bool enable)
{
  bookEnabled_ = enable;
}

void ChessAlgorithm::setMemory(int mb)
{
  int size1 = 0, size2 = 0;

  if ( mb > 4 ) // we need at least 4mb of memory
  {
    mb -= 4;
    size1 = 16;
    for ( ;mb > 1; mb >>= 1, size1++);
    size1--;
    size2 = size1;
  }

  htable_.resize(size1);
  capturesHash_.resize(size2);
}

void ChessAlgorithm::setMode(int mode)
{
  mode_ = mode;
}

void ChessAlgorithm::setStopFlag(bool stop)
{
  if ( !mode_ )
    stopMe_ = stop;
  else
    stopAfterMove_ = true;
}

void ChessAlgorithm::resetStopFlags()
{
  stopMe_ = stopAfterMove_ = false;
}

void ChessAlgorithm::stop()
{
  setStopFlag(true);
}

void ChessAlgorithm::setDepth(int depth)
{
  if ( depth > 0 && depth < 32 )
    depthMax_ = depth;
}

void ChessAlgorithm::init(Figure::Color color)
{
  positions_.clear();
  positions_.push_back( Level(Board(color), Step()) );
  positions_.back().step_.clear();
  currentIdx_ = 0;
  movesCount_ = 0;
  lastEatNum_ = 0;
  setStopFlag(false);
  Board::StepSorter::clearHistory();
}

void ChessAlgorithm::updateLastEat()
{
  lastEatNum_ = 0;
  for (int i = currentIdx_; i >= 0; --i)
  {
    const Step & step = positions_[i].step_;
    if ( step.rindex_ >= 0 || Figure::TypePawn == (Figure::Type)step.ftype_ )
    {
      lastEatNum_ = i;
      break;
    }
  }
}

const Board * ChessAlgorithm::getCurrent() const
{
  if ( currentIdx_ >= positions_.size() )
    return 0;

  return &positions_[currentIdx_].board_;
}

Board * ChessAlgorithm::getCurrent()
{
  if ( currentIdx_ >= positions_.size() )
    return 0;

  return &positions_[currentIdx_].board_;
}

const Step * ChessAlgorithm::lastStep() const
{
  if ( currentIdx_ >= positions_.size() )
    return 0;

  return &positions_[currentIdx_].step_;
}

void ChessAlgorithm::nextPos()
{
  if ( ++currentIdx_ >= positions_.size() )
    currentIdx_ = positions_.size()-1;

  updateLastEat();
}

void ChessAlgorithm::prevPos()
{
  if ( --currentIdx_ >= positions_.size() )
    currentIdx_ = 0;

  updateLastEat();
}

bool ChessAlgorithm::calculateSteps(std::vector<Step> & steps_arr)
{
  if ( currentIdx_ >= positions_.size() )
    return false;

  if ( steps_arr.size() != 0 )
    return true;

  Board & board = positions_[currentIdx_].board_;

  if ( board.drawState() || Board::ChessMat == board.getState() )
    return false;

  Step prev;
  prev.clear();

  StepId killerId;
  killerId.clear();

  Step steps[Board::NumOfSteps];
  int ifirst = board.getSteps(steps, false, prev, killerId);
  if ( ifirst < 0 )
    return false;

  for (int i = ifirst; i >= 0; i = steps[i].next_)
  {
    Step & step = steps[i];

    bool valid = false;
    if ( board.doStep(step) )
    {
      valid = board.wasStepValid(step);
      board.undoStep(step);
    }

    if ( !valid )
      continue;

    steps_arr.push_back(step);
  }

  return steps_arr.size() > 0;
}

void ChessAlgorithm::applyStep(Step & step)
{
  if ( !step || currentIdx_ >= positions_.size() )
    return;

  Board & board = positions_[currentIdx_].board_;
  Board nextBoard(board);

  Board::State state = nextBoard.makeStep(step);

  if ( Board::Invalid == state )
    return;

  positions_.erase(positions_.begin()+currentIdx_+1, positions_.end());

  Reps rcur(nextBoard.hashCode(), nextBoard.getFiguresCount(), step.rindex_ >= 0, Figure::TypePawn == (Figure::Type)step.ftype_ );

  positions_.push_back( Level(nextBoard, step) );
  currentIdx_ = positions_.size()-1;
  if ( step.rindex_ >= 0 || Figure::TypePawn == (Figure::Type)step.ftype_ )
    lastEatNum_ = currentIdx_;

  int n_reps;
  positions_.back().board_.setChessDraw(checkForDraw(0, rcur, n_reps));
}

//////////////////////////////////////////////////////////////////////////
void ChessAlgorithm::outputPV(CalcResult & cres, int dt, std::ostream & sout)
{
  WeightType w = cres.wminus_ ? -cres.weight_ : cres.weight_;
  sout << cres.depth_ << " " << w << " " << dt << " " << cres.movesCount_ << " ";
  for (int i = 0; i < cres.depth_; ++i)
  {
    char str[32];
    if ( !formatMove(cres.steps_[i], str) )
      break;

    sout << str;
    if ( i < cres.depth_-1 )
      sout << " ";
  }
  sout << endl;
}
//////////////////////////////////////////////////////////////////////////
int ChessAlgorithm::doBestStep(CalcResult & cres, std::ostream & sout)
{
  resetStopFlags();

  if ( currentIdx_ >= positions_.size() )
    return 0;

  Board & board = positions_[currentIdx_].board_;
  const Step & prev = positions_[currentIdx_].step_;

  if ( Board::ChessMat == board.getState() || board.drawState() )
    return 0;


  if ( bookEnabled_ )
  {
    std::vector<StepId> steps;
    for (size_t i = 1; i <= currentIdx_; ++i)
      steps.push_back(positions_[i].step_);

    StepId stepId = debutsTable_.findStep(steps, &board);
    if ( stepId && board.stepById(stepId, cres.best_) )
    {
      cres.depth_ = 1;
      cres.steps_[0] = cres.best_;
      cres.weight_ = 0;
    }
    else
      cres.best_.clear();
  }

  if ( !cres.best_ )
  {
    vector<Step> steps;
    if ( !calculateSteps(steps) )
      return 0;

    if ( steps.size() == 1 )
    {
      cres.steps_[0] = cres.best_ = steps[0];
      cres.depth_ =  1;
      cres.weight_ = 0;
    }
  }

  if ( !cres.best_ )
  {
    WeightType alpha = -std::numeric_limits<WeightType>::max();
    WeightType betta = +std::numeric_limits<WeightType>::max();

    cres.weight_ = -std::numeric_limits<WeightType>::max();

    int depth = 1;
    cres.totalMoves_ = 0;

    Board::StepSorter::clearHistory();

    for ( ; !stopMe_ && !stopAfterMove_; ++depth)
    {
      StepId pv_steps[MaxDepth];

      for (int i = 0; i < MaxDepth; ++i)
      {
        killers_[i].clear();
        pv_steps[i].clear();
      }

      movesCount_ = 0;
      forcedCount_ = 0;
      additionalCount_ = 0;
      nullMovesCount_ = 0;

      depthLimit_ = depth;

      Step best;
      best.clear();

      clock_t t0 = clock();

      WeightType w = alphaBetta(board, best, pv_steps, prev, depth, 0, alpha, betta, false);

      double dt = 100.0*(double(clock() - t0))/CLOCKS_PER_SEC;

      cres.totalMoves_ += movesCount_;
      if ( stopMe_ )
      {
        break;
      }

      THROW_IF(best != pv_steps[0], "wrong step found");

      cres.weight_ = w;
      cres.wminus_ = Figure::ColorBlack == board.getColor();
      cres.best_ = best;
      cres.depth_ = depth;
      cres.movesCount_ = movesCount_;
      cres.forcedMoves_ = forcedCount_;
      cres.additionalMoves_ = additionalCount_;
      cres.nullMovesCount_ = nullMovesCount_;

      for (int i = 0; i < depth; ++i)
      {
        cres.steps_[i] = pv_steps[i];
        if ( !pv_steps[i] )
          break;
      }

      outputPV(cres, (int)dt, sout);

      if ( depth >= depthMax_ || (w >= Figure::WeightMat-MaxDepth || w <= MaxDepth-Figure::WeightMat) )
        break;
    }
  }

  if ( !cres.best_ )
    return 0;

  applyStep(cres.best_);

  return cres.depth_;
}

//////////////////////////////////////////////////////////////////////////
WeightType ChessAlgorithm::doNullMove(Board & board, const Step & prev, int depth, int ply, WeightType alpha, WeightType betta)
{
  if ( (board.getState() != Board::UnderCheck) &&
        board.getStage(Figure::ColorBlack) != 2 && board.getStage(Figure::ColorWhite) != 2 &&
        board.allowNullMove() &&
       (ply > 0) &&
       (depth > 1) &&
       (depthLimit_ > 3) )
  {
    Board nullBoard(board);
    nullBoard.makeNullMove(prev);

    Step nullStep, nullPrev;
    nullStep.clear();
    nullPrev.clear();

    StepId nullPV[MaxDepth];

    int R = 1;
    if ( depth > 4 )
      R = 4;
    else if ( depth > 3 )
      R = 3;
    else if ( depth > 2 )
      R = 2;

    repetitions_[ply+1].clear();
    WeightType nullEval = -alphaBetta(nullBoard, nullStep, nullPV, nullPrev, depth-R, ply+1, -betta, -(betta-1), true);
    return nullEval;
  }

  return alpha;
}

//////////////////////////////////////////////////////////////////////////
WeightType ChessAlgorithm::alphaBetta(Board & board, Step & step, StepId * pv_steps, const Step & prev, int depth, int ply, WeightType alpha, WeightType betta, bool nullMove)
{
  THROW_IF( depth < 1, "zero depth" );

  WeightType savedAlpha = alpha;

  Step hstep;
  hstep.clear();

  bool retAlpha = false;
  bool valid = false;

#ifndef DONT_USE_EXTS_
  alpha = processHitem(board, prev, step, pv_steps, hstep, depth, ply, alpha, betta, retAlpha, valid, nullMove);
#endif

  if ( stopMe_ )
    return alpha;

  if ( retAlpha || alpha >= betta )
    return alpha;

#ifndef DONT_USE_EXTS_
  if ( !nullMove )
  {
	  WeightType nullEval = doNullMove(board, prev, depth, ply, alpha, betta);
	  if ( nullEval >= betta )
	  {
		  nullMovesCount_++;
		  depth -= 3;
		  if ( depth < 1 )
			  depth = 1;
		  nullMove = true;
	  }
  }
#endif

#ifndef DONT_USE_EXTS_
  if ( hstep && valid )
  {
	  WeightType w = doMove(board, pv_steps, prev, hstep, depth, ply, alpha, betta, true, valid, nullMove);

	  if ( valid )
	  {
		  hstep.weight_ = w;

		  if ( w > alpha )
		  {
			  step = hstep;
			  alpha = w;
		  }

		  if ( alpha >= betta )
			  return alpha;
	  }
  }
#endif

  bool first = !valid;

  StepId killerId;
  Step   killer;

  killer.clear();
  killerId = killers_[ply];

  if ( killerId != hstep && killerId.rindex_ >= 0 && board.stepById(killerId, killer) )
  {
    THROW_IF(killer.invalid(), "invalid step");

    WeightType w = doMove(board, pv_steps, prev, killer, depth, ply, alpha, betta, true, valid, nullMove);
    
    if ( valid )
    {
      first = false;
      killer.weight_ = w;
    }

    if ( valid && w > alpha )
    {
      alpha = w;
      step  = killer;

      if ( !stopMe_ )
      {
        HashTable::Flag flag = alpha >= betta ? HashTable::Betta : HashTable::AlphaBetta;
        htable_.write(board.hashCode(), alpha, depth, ply, board.getColor(), flag, killer);
      }
    }

    if ( stopMe_ || alpha >= betta )
    {
      return alpha;
    }
  }

  Step steps[Board::NumOfSteps];
  int ifirst = board.getSteps(steps, false, prev, killerId);

  for (int i = ifirst; i >= 0 && alpha < betta; i = steps[i].next_)
  {
    if ( steps[i] == hstep || steps[i] == killer )
      continue;

    WeightType w = doMove(board, pv_steps, prev, steps[i], depth, ply, alpha, betta, first, valid, nullMove);

    if ( stopMe_ )
      break;

    if ( !valid )
      continue;

    first = false;
    steps[i].weight_  = w;

    if ( w > alpha )
    {
      step = steps[i];
      alpha = w;

      if ( steps[i].rindex_ < 0 )
        Board::StepSorter::history()[step.from_][step.to_] += depth*depth;
    }

    if ( w > killers_[ply].weight_ )
      killers_[ply] = steps[i];
  }


  HashTable::Flag flag = HashTable::None;

  if ( alpha == savedAlpha )
    flag = HashTable::Alpha;
  else if ( alpha >= betta )
    flag = HashTable::Betta;
  else if ( alpha > savedAlpha )
    flag = HashTable::AlphaBetta;

  if ( HashTable::None != flag && !stopMe_ )
  {
    htable_.write(board.hashCode(), alpha, depth, ply, board.getColor(), flag, step);
  }

  return alpha;
}

//////////////////////////////////////////////////////////////////////////
WeightType ChessAlgorithm::captureForce(const Step & prev, Step & killerId, int ply, Board & board, WeightType alpha, WeightType betta)
{
  Step hstep;
  hstep.clear();

  Step step;
  step.clear();

  WeightType savedAlpha = alpha;
  bool retAlpha = false;

  Step ki;
  ki.clear();

  bool valid = false;

#ifndef DONT_USE_EXTS_
  alpha = capturesHitem(board, ki, ply, step, hstep, alpha, betta, retAlpha, valid);
#endif

  if ( stopMe_ || retAlpha || alpha >= betta )
    return alpha;

  bool first = !valid;

  Step killer;
  killer.clear();

  if ( killerId != hstep && killerId.rindex_ >= 0 && board.stepById(killerId, killer) )
  {
    WeightType w = doCaptureMove(killer, ki, ply, board, alpha, betta, first, valid);
    if ( valid )
      first = false;

    if ( valid && w > alpha )
    {
      step = killer;
      alpha = w;

      if ( !stopMe_ )
      {
        HashTable::Flag flag = alpha >= betta ? HashTable::Betta : HashTable::AlphaBetta;
        capturesHash_.write(board.hashCode(), alpha, 0, 0, board.getColor(), flag, step);
      }
    }
  }

  if ( stopMe_ || alpha >= betta )
    return alpha;

  Step steps[Board::NumOfSteps];
  int ifirst = board.getSteps(steps, true, prev, killerId);

  for (int i = ifirst; i >= 0 && alpha < betta; i = steps[i].next_)
  {
    if ( stopMe_ )
      break;

    Step & s = steps[i];

    if ( s == killer || s == hstep )
      continue;

#ifndef USE_EXTRA_QUIS_
    THROW_IF( s.rindex_ < 0, "step have to be capture");
#endif

    WeightType w = doCaptureMove(s, ki, ply, board, alpha, betta, first, valid);
    if ( valid )
      first = false;

    if ( valid && w > alpha )
    {
      step = s;
      alpha = w;
    }

    if ( valid && w > killerId.weight_ )
      killerId = s;
  }

  HashTable::Flag flag = HashTable::None;

  if ( alpha == savedAlpha )
    flag = HashTable::Alpha;
  else if ( alpha >= betta )
    flag = HashTable::Betta;
  else if ( alpha > savedAlpha )
    flag = HashTable::AlphaBetta;

  if ( HashTable::None != flag && !stopMe_ )
  {
    capturesHash_.write(board.hashCode(), alpha, 0, 0, board.getColor(), flag, step);
  }

  return alpha;
}

//////////////////////////////////////////////////////////////////////////
void ChessAlgorithm::save(std::ostream & out) const
{
  out << "steps " << positions_.size() << std::endl;

  for (size_t i = 0; i < positions_.size(); ++i)
  {
    positions_[i].board_.writeTo(out);
    positions_[i].step_.writeTo(out);
  }

#ifdef SAVE_HTABLE_
  htable_.save("htable.dat");
  capturesHash_.save("capture.dat");
#endif
}

bool ChessAlgorithm::load(std::istream & in)
{
  int nsteps = 0;
  std::string str;
  in >> str >> nsteps;

  if ( str != "steps" )
    return false;

  positions_.clear();
  for (int i = 0; i < nsteps; ++i)
  {
    Board board(Figure::ColorBlack);
    board.readFrom(in);
    Step step;
    step.clear();
    step.readFrom(in);
    positions_.push_back(Level(board, step));
  }
  currentIdx_ = positions_.size()-1;
  updateLastEat();

#ifdef SAVE_HTABLE_
  htable_.load("htable.dat");
  capturesHash_.load("capture.dat");
#endif

  return true;
}
