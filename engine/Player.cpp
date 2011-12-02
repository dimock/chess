
#include "Player.h"
#include "MovesGenerator.h"

#undef NO_TIME_LIMIT

#ifndef NDEBUG
  #define TIMING_FLAG 0xFFF
#else
  #define TIMING_FLAG 0xFFFF
#endif

//////////////////////////////////////////////////////////////////////////

SearchResult::SearchResult() :
  nodesCount_(0),
  totalNodes_(0),
  forcedNodes_(0),
  additionalNodes_(0),
  nullMovesCount_(0),
  depth_(0),
  score_(0),
  dt_(0)
{
  best_.clear();
  for (int i = 0; i < MaxDepth; ++i)
    pv_[i].clear();
}

Player::Player() :
  stop_(false),
  timeLimitMS_(0),
  tstart_(0),
  nodesCount_(0),
  totalNodes_(0),
  depthMax_(2)
{
  g_moves = new MoveCmd[Board::GameLength];
  g_deltaPosCounter = new DeltaPosCounter;
  g_betweenMasks = new BetweenMask(g_deltaPosCounter);
  g_distanceCounter = new DistanceCounter;
  g_movesTable = new MovesTable;
  g_figureDir = new FigureDir;
  g_pawnMasks_ = new PawnMasks;

  board_.set_moves(g_moves);
  board_.set_MovesTable(g_movesTable);
  board_.set_FigureDir(g_figureDir);
  board_.set_DeltaPosCounter(g_deltaPosCounter);
  board_.set_DistanceCounter(g_distanceCounter);
  board_.set_BetweenMask(g_betweenMasks);
  board_.set_PawnMasks(g_pawnMasks_);
}

Player::~Player()
{
  board_.set_moves(0);
  board_.set_MovesTable(0);
  board_.set_FigureDir(0);
  board_.set_DeltaPosCounter(0);
  board_.set_DistanceCounter(0);
  board_.set_BetweenMask(0);
  board_.set_PawnMasks(0);

  delete [] g_moves;
  delete g_movesTable;
  delete g_figureDir;
  delete g_betweenMasks;
  delete g_deltaPosCounter;
  delete g_distanceCounter;
  delete g_pawnMasks_;
}

bool Player::fromFEN(const char * fen)
{
  stop_ = false;
  
  Board tboard(board_);
  MoveCmd tmoves[16];
  tboard.set_moves(tmoves);

  // verify FEN first
  if ( !tboard.fromFEN(fen) )
    return false;

  return board_.fromFEN(fen);
}

bool Player::toFEN(char * fen) const
{
  return board_.toFEN(fen);
}

void Player::printPV(SearchResult & sres, std::ostream * out)
{
  if ( !out )
    return;

  char str[64];
  if ( !printSAN(board_, sres.best_, str) )
    str[0] = 0;

  *out << sres.depth_ << " " << sres.score_ << " " << (int)sres.dt_ << " " << sres.nodesCount_ << " " << str << std::endl;
}

bool Player::findMove(SearchResult & sres, std::ostream * out)
{
  sres = SearchResult();

  stop_ = false;
  totalNodes_ = 0;
  firstIter_ = true;
  tprev_ = tstart_ = clock();

  Move before;
  before.clear();
  for (int depth = 2; !stop_ && depth <= depthMax_; ++depth)
  {
    Move best;
    bool found = false;
    nodesCount_ = 0;

    ScoreType score = alphaBetta(depth, 0, -std::numeric_limits<ScoreType>::max(), std::numeric_limits<ScoreType>::max(), before, best, found);

    if ( !stop_ || (stop_ && found) )
    {
      clock_t t  = clock();
      clock_t dt = (t - tprev_) / 10;
      tprev_ = t;

      sres.score_ = score;
      sres.best_  = best;
      sres.depth_ = depth;
      sres.nodesCount_ = nodesCount_;
      sres.dt_ = dt;
      before = best;

      printPV(sres, out);
    }

	firstIter_ = false;

    if ( score >= Figure::WeightMat-MaxDepth || score <= MaxDepth-Figure::WeightMat )
      break;
  }

  sres.totalNodes_ = totalNodes_;

  return sres.best_.to_ >= 0;
}

void Player::testTimer()
{
	if ( firstIter_ )
		return;

#ifndef NO_TIME_LIMIT
  int t = clock();
  stop_ = stop_ || ( (t - tstart_) > timeLimitMS_);
#endif
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, const Move & before, Move & move, bool & found)
{
  if ( stop_ )
    return alpha;

  int counter = 0;
  Move b;
  b.clear();

  if ( before && board_.validMove(before) )
  {
    movement(depth, ply, alpha, betta, before, b, before, move, found, counter);
  }

  //QpfTimer qpt;

	MovesGenerator mg(board_);

  //Board::ticks_ += qpt.ticks();
  //Board::tcounter_++;

	for ( ; !stop_ && alpha < betta ; )
	{
	  const Move & mv = mg.move();
	  if ( !mv )
		break;

	  if ( mv == before )
		  continue;

	  if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
		testTimer();

	  if ( stop_ )
		break;
	  
	  THROW_IF( !board_.validMove(mv), "move validation failed" );

	  movement(depth, ply, alpha, betta, before, b, mv, move, found, counter);
	}

  if ( stop_ )
    return alpha;

  if ( 0 == counter )
  {
    board_.setNoMoves();
    ScoreType s = board_.evaluate();
    if ( Board::ChessMat == board_.getState() )
      s += ply;
    return s;
  }

  if ( 0 == ply && counter == 1 )
  {
    found = true;
    stop_ = true;
  }

  return alpha;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::captures(Move & killer, ScoreType alpha, ScoreType betta, int delta)
{
	if ( stop_ )
		return alpha;

  Move ki;
  ki.clear();

  if ( killer && board_.validMove(killer) )
  {
    MovesGenerator mg(board_);
    if ( !mg.find(killer) )
    {
      board_.validMove(killer);
    }
    capture(killer, ki, alpha, betta, killer);
  }

  if ( alpha >= betta )
    return alpha;

  Figure::Type minimalType = Figure::TypePawn;
  if ( delta > Figure::figureWeight_[Figure::TypeRook] )
    minimalType = Figure::TypeQueen;
  else if ( delta > Figure::figureWeight_[Figure::TypeBishop] )
    minimalType = Figure::TypeRook;
  else if ( delta > Figure::figureWeight_[Figure::TypeKnight] )
    minimalType = Figure::TypeBishop;
  else if ( delta > Figure::figureWeight_[Figure::TypePawn] )
    minimalType = Figure::TypeKnight;

  //QpfTimer qpt;
  CapsGenerator cg(board_, minimalType, *this, killer, ki, alpha, betta);
  //Board::ticks_ += qpt.ticks();
  //Board::tcounter_ += cg.count();

	for ( ; !stop_ && alpha < betta ; )
	{
		const Move & cap = cg.capture();
		if ( !cap )
			break;

		if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
			testTimer();

		if ( stop_ )
			break;

		THROW_IF( !board_.validMove(cap), "move validation failed" );

		capture(killer, ki, alpha, betta, cap);
	}

	return alpha;
}
