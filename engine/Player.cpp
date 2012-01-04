
#include "Player.h"
#include "MovesGenerator.h"

//////////////////////////////////////////////////////////////////////////
inline Figure::Type delta2type(int delta)
{
  Figure::Type minimalType = Figure::TypePawn;

#ifdef USE_DELTA_PRUNING_
  if ( delta > Figure::figureWeight_[Figure::TypeQueen] )
    minimalType = Figure::TypeKing;
  else if ( delta > Figure::figureWeight_[Figure::TypeRook] )
    minimalType = Figure::TypeQueen;
  else if ( delta > Figure::figureWeight_[Figure::TypeBishop] )
    minimalType = Figure::TypeRook;
  else if ( delta > Figure::figureWeight_[Figure::TypeKnight] )
    minimalType = Figure::TypeBishop;
  else if ( delta > Figure::figureWeight_[Figure::TypePawn] )
    minimalType = Figure::TypeKnight;
#endif

  return minimalType;
}

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
  for (int i = 0; i < MaxPly; ++i)
    pv_[i].clear();
}

Player::Player() :
  stop_(false),
  timeLimitMS_(0),
  tstart_(0),
  nodesCount_(0),
  totalNodes_(0),
  depthMax_(2),
  depth_(0)
{
  g_moves = new MoveCmd[Board::GameLength];
  g_deltaPosCounter = new DeltaPosCounter;
  g_betweenMasks = new BetweenMask(g_deltaPosCounter);
  g_distanceCounter = new DistanceCounter;
  g_movesTable = new MovesTable;
  g_figureDir = new FigureDir;
  g_pawnMasks_ = new PawnMasks;

  pv_moves_ = new MoveCmd[Board::GameLength];

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

  delete [] pv_moves_;

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

void Player::printPV(Board & pv_board, SearchResult & sres, std::ostream * out)
{
  if ( !out )
    return;

  *out << sres.depth_ << " " << sres.score_ << " " << (int)sres.dt_ << " " << sres.nodesCount_;
  for (int i = 0; i < sres.depth_ && sres.pv_[i]; ++i)
  {
    *out << " ";

    if ( !pv_board.validMove(sres.pv_[i]) || !pv_board.makeMove(sres.pv_[i]) )
      break;

    pv_board.unmakeMove();

    char str[64];
    if ( !printSAN(pv_board, sres.pv_[i], str) )
      break;

    pv_board.makeMove(sres.pv_[i]);

    *out << str;
  }
  *out << std::endl;
}

bool Player::findMove(SearchResult & sres, std::ostream * out)
{
  sres = SearchResult();

  stop_ = false;
  totalNodes_ = 0;
  firstIter_ = true;
  tprev_ = tstart_ = clock();

  MovesGenerator::clear_history();

  before_.clear();
  contexts_[0].killer_.clear();

  contexts_[0].clearPV(depthMax_);

  for (depth_ = 2; !stop_ && depth_ <= depthMax_; ++depth_)
  {
    Board pv_board(board_);
    pv_board.set_moves(pv_moves_);

    best_.clear();
    beforeFound_ = false;
    nodesCount_ = 0;

    ScoreType alpha = -std::numeric_limits<ScoreType>::max();
    ScoreType betta = +std::numeric_limits<ScoreType>::max();

    ScoreType score = alphaBetta(depth_, 0, alpha, betta);

    if ( best_ && (!stop_ || (stop_ && beforeFound_) || (2 == depth_)) )
    {
      clock_t t  = clock();
      clock_t dt = (t - tprev_) / 10;
      tprev_ = t;

      sres.score_ = score;
      sres.best_  = best_;
      sres.depth_ = depth_;
      sres.nodesCount_ = nodesCount_;
      sres.dt_ = dt;

      if ( before_ != best_ && before_ )
        contexts_[0].killer_ = before_;
      else
        contexts_[0].killer_.clear();

      for (int i = 0; i < depth_; ++i)
      {
        sres.pv_[i] = contexts_[0].pv_[i];
        if ( !sres.pv_[i] )
          break;
      }

      THROW_IF( sres.pv_[0] != best_, "invalid PV found" );

      before_ = best_;

      printPV(pv_board, sres, out);
    }

    firstIter_ = false;

    if ( !best_ || score >= Figure::WeightMat-MaxPly || score <= MaxPly-Figure::WeightMat )
      break;
  }

  sres.totalNodes_ = totalNodes_;

  return sres.best_;
}

void Player::testTimer()
{
	//if ( firstIter_ )
	//	return;

#ifndef NO_TIME_LIMIT
  int t = clock();
  stop_ = stop_ || ( (t - tstart_) > timeLimitMS_);
#endif
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta)
{
  if ( stop_ || ply >= MaxPly )
    return alpha;

  // there is no reason to go in this direction
  if ( alpha >= Figure::WeightMat-ply )
    return alpha;

  int counter = 0;

  Move pv;
  pv.clear();

  // we use PV only up to max-depth
  if ( ply < depthMax_ )
  {
    pv = contexts_[0].pv_[ply];
    pv.checkVerified_ = 0;

    THROW_IF( pv.rindex_ == 100, "invalid pv move" );
  }

  // clear context for current ply
  if ( ply < MaxPly )
  {
    contexts_[ply+1].killer_.clear();
    contexts_[ply].pv_[ply].clear();
  }

#ifdef VERIFY_ESCAPE_GENERATOR
  verifyEscapeGen(depth, ply, alpha, betta);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  verifyChecksGenerator(depth, ply, alpha, betta);
#endif


  // now use PV. it will be changed to hash soon
  if ( board_.validMove(pv) )
    movement(depth, ply, alpha, betta, pv, counter);

  Move & killer = contexts_[ply].killer_;

#ifdef USE_KILLER
  if ( killer && killer != pv && board_.validMove(killer) )
  {
#ifndef NDEBUG
    MovesGenerator mg(board_);
    if ( !mg.find(killer) )
    {
      board_.validMove(killer);
      THROW_IF( true, "move has passed validation but not found in generated moves list" );
    }
#endif

	  killer.checkVerified_ = 0;
    movement(depth, ply, alpha, betta, killer, counter);
  }

  if ( alpha >= betta )
  {
    return alpha;
  }
#endif


  if ( Board::UnderCheck == board_.getState() )
  {
    //QpfTimer qpt;
    EscapeGenerator eg(board_, depth, ply, *this, alpha, betta, counter);
    //Board::ticks_ += qpt.ticks();
    //Board::tcounter_++;

    int depthInc = 0;
    if ( ply > 0 && !firstIter_ && counter == 0 && eg.count() == 1 &&
         alpha < Figure::WeightMat-MaxPly
        /*(alpha < -Figure::WeightMat || alpha > -Figure::WeightMat+MaxPly) && (alpha > Figure::WeightMat || alpha < Figure::WeightMat-MaxPly)*/ )
    {
      depthInc = 1;
    }

    for ( ; !stop_ && alpha < betta ; )
    {
      const Move & move = eg.escape();
      if ( !move || stop_ )
        break;

      if ( move == pv
#ifdef USE_KILLER
        || move == killer
#endif
        )
        continue;

      if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
        testTimer();

      movement(depth+depthInc, ply, alpha, betta, move, counter);
    }

    THROW_IF( 2 == depthInc && counter != 1, "depth was increased by 2, but there is not exactly 1 move" );
  }
  else
  {
#ifdef USE_FUTILITY_PRUNING
	  if ( alpha > -Figure::WeightMat+MaxPly && alpha < Figure::WeightMat-MaxPly )
    {
      if ( depth == 1 && ply > 1 )
	  {
		  ScoreType score = board_.evaluate();
		  int delta = (int)alpha - (int)score - (int)Figure::positionGain_;
		  if ( delta > Figure::positionGain_ )
		  {
			  return captures_checks(depth, ply, alpha, betta, delta);
		  }
	  }
    }
#endif

	  MovesGenerator mg(board_, depth, ply, this, alpha, betta, counter);
	  for ( ; !stop_ && alpha < betta ; )
	  {
	    const Move & move = mg.move();
	    if ( !move || stop_ )
		    break;

      if ( depth_ == 4 && ply == 0 && move.from_ == 8 && move.to_ == 24 )
      {
        int ttt = 0;
      }

      if ( depth_ == 4 && ply == 2 && move.from_ == 24 && move.to_ == 32 )
      {
        int ttt = 0;
      }

	    if ( move == pv
  #ifdef USE_KILLER
        || move == killer
  #endif
        )
		    continue;

	    if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
		    testTimer();

      if ( stop_ )
        break;
  	  
	    movement(depth, ply, alpha, betta, move, counter);
	  }
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
    beforeFound_ = true;
    stop_ = true;
  }

  return alpha;
}
//////////////////////////////////////////////////////////////////////////
void Player::movement(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & move, int & counter)
{
  totalNodes_++;
  nodesCount_++;

#ifndef NDEBUG
  Board board0 = board_;
#endif

  if ( board_.makeMove(move) )
  {
    bool haveCheck = board_.getState() == Board::UnderCheck;
    if ( (haveCheck || Figure::TypeQueen == move.new_type_) && depth > 0 &&
          alpha < Figure::WeightMat-MaxPly &&
          depth > 0 )
    {
      depth++;
    }

    counter++;
    ScoreType score = alpha;
    if ( board_.drawState() )
      score = 0;
    else if ( depth <= 1 )
    {
      score = -board_.evaluate();
      int delta = (int)score - (int)betta - (int)Figure::positionGain_;
      if ( haveCheck || (score > alpha && delta < Figure::figureWeight_[Figure::TypeQueen]) )
      {
        ScoreType betta1 = score < betta && !haveCheck ? score : betta;
#ifdef PERFORM_CAPTURES_AND_CHECKS
        if ( depth < 1 )
#endif

          score = -captures(ply+1, -betta1, -alpha, delta);

#ifdef PERFORM_CAPTURES_AND_CHECKS
        else
          score = -captures_checks(0, ply+1, -betta1, -alpha, delta);
      }
#endif
    }
    else
    {
#ifdef USE_ZERO_WINDOW
      if ( counter > 1 )
        score = -alphaBetta(depth-1, ply+1, -(alpha+1), -alpha);
      if ( counter < 2  || (score > alpha && score < betta) )
#endif
        score = -alphaBetta(depth-1, ply+1, -betta, -score);
    }

    if ( !stop_ && score > alpha )
    {
      alpha = score;

      assemblePV(move, ply);

      if ( move.rindex_ < 0 )
        MovesGenerator::history(move.from_, move.to_) += depth;

      if ( 0 == ply )
      {
        best_ = move;
        if ( before_ == best_ )
          beforeFound_ = true;
      }
#ifdef USE_KILLER
      Move & killer = contexts_[ply].killer_;
      if ( score > killer.score_ )
      {
        killer = move;
        killer.score_ = score;
      }
#endif
    }
  }

#ifndef NDEBUG
  board_.verifyMasks();
#endif

  board_.unmakeMove();

  THROW_IF( board0 != board_, "board unmake wasn't correctly applied" );

#ifndef NDEBUG
  board_.verifyMasks();
#endif
}


//////////////////////////////////////////////////////////////////////////
ScoreType Player::captures(int ply, ScoreType alpha, ScoreType betta, int delta)
{
	if ( stop_ || ply >= MaxPly )
		return alpha;

  if ( ply < MaxPly )
    contexts_[ply+1].killer_.clear();

#ifdef VERIFY_CAPS_GENERATOR
  verifyCapsGenerator(ply, alpha, betta, delta);
#endif

  int counter = 0;
  Move & killer = contexts_[ply].killer_;

#ifdef USE_KILLER
  if ( killer && board_.validMove(killer) )
  {
#ifndef NDEBUG
    MovesGenerator mg(board_);
    if ( !mg.find(killer) )
    {
      board_.validMove(killer);
      THROW_IF( true, "move has passed validation but not found generated in moves list" );
    }
#endif

	  killer.checkVerified_ = 0;
      capture(ply, alpha, betta, killer, counter);
  }

  if ( alpha >= betta )
  {
    return alpha;
  }
#endif

  Figure::Type minimalType = delta2type(delta);

  //QpfTimer qpt;
  //Board::ticks_ += qpt.ticks();
  //Board::tcounter_ ++;//= cg.count();

  if ( board_.getState() == Board::UnderCheck )
  {
    //QpfTimer qpt;
	  EscapeGenerator eg(board_, 0, ply, *this, alpha, betta, counter);
    //Board::ticks_ += qpt.ticks();

	  for ( ; !stop_ && alpha < betta ; )
	  {
		  const Move & cap = eg.escape();
		  if ( !cap || stop_ )
			  break;

		  if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
			  testTimer();

      if ( stop_ )
        break;

#ifdef USE_KILLER
		  if ( killer == cap )
			  continue;
#endif

		  THROW_IF( !board_.validMove(cap), "move validation failed" );

		  capture(ply, alpha, betta, cap, counter);
	  }

    if ( !counter )
    {
      board_.setNoMoves();
      ScoreType s = board_.evaluate();
      if ( Board::ChessMat == board_.getState() )
        s += ply;
      return s;
    }
  }
  else
  {
	  CapsGenerator cg(board_, minimalType, ply, *this, alpha, betta, counter);

    for ( ; !stop_ && alpha < betta ; )
    {
      const Move & cap = cg.capture();
      if ( !cap || stop_ )
        break;

      if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
        testTimer();

#ifdef USE_KILLER
      if ( killer == cap )
        continue;
#endif

      THROW_IF( !board_.validMove(cap), "move validation failed" );

      capture(ply, alpha, betta, cap, counter);
    }
  }

  return alpha;
}

//////////////////////////////////////////////////////////////////////////
void Player::capture(int ply, ScoreType & alpha, ScoreType betta, const Move & cap, int & counter)
{
	totalNodes_++;
	nodesCount_++;

#ifndef NDEBUG
	Board board0 = board_;
#endif

	if ( board_.makeMove(cap) )
	{
    bool haveCheck = board_.getState() == Board::UnderCheck;

    counter++;
		ScoreType s = alpha;
		if ( board_.drawState() )
			s = 0;
		else
		{
			s = -board_.evaluate();
			int delta = s - betta - Figure::positionGain_;
			bool b = s <= alpha;
			if ( haveCheck || (s > alpha && delta < Figure::figureWeight_[Figure::TypeQueen]) )
			{
				ScoreType betta1 = s < betta && !haveCheck ? s : betta;
				s = -captures(ply+1, -betta1, -alpha, delta);
			}
		}
		if ( !stop_ && s > alpha )
		{
			alpha = s;

#ifdef USE_KILLER
			Move killer = contexts_[ply].killer_;
			if ( s > killer.score_ )
			{
				killer = cap;
				killer.score_ = s;
			}
#endif
		}
	}

#ifndef NDEBUG
	board_.verifyMasks();
#endif

	board_.unmakeMove();

	THROW_IF( board0 != board_, "board unmake wasn't applied correctly" );

#ifndef NDEBUG
	board_.verifyMasks();
#endif
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
ScoreType Player::captures_checks(int depth, int ply, ScoreType alpha, ScoreType betta, int delta)
{
	if ( stop_ || ply >= MaxPly )
		return alpha;

	if ( ply < MaxPly )
		contexts_[ply+1].killer_.clear();

	int counter = 0;

	Move & killer = contexts_[ply].killer_;

#ifdef USE_KILLER
	if ( killer && board_.validMove(killer) )
	{
#ifndef NDEBUG
		MovesGenerator mg(board_);
		if ( !mg.find(killer) )
		{
			board_.validMove(killer);
			THROW_IF( true, "move has passed validation but not found generated in moves list" );
		}
#endif

		killer.checkVerified_ = 0;
		movement(depth, ply, alpha, betta, killer, counter);
	}

	if ( alpha >= betta )
	{
		return alpha;
	}
#endif

	Figure::Type minimalType = delta2type(delta);

	//QpfTimer qpt;
	//Board::ticks_ += qpt.ticks();
	//Board::tcounter_ ++;//= cg.count();

	if ( board_.getState() == Board::UnderCheck )
	{
		//QpfTimer qpt;
		EscapeGenerator eg(board_, 0, ply, *this, alpha, betta, counter);
		//Board::ticks_ += qpt.ticks();

		for ( ; !stop_ && alpha < betta ; )
		{
			const Move & move = eg.escape();
			if ( !move || stop_ )
				break;

			if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
				testTimer();

			if ( stop_ )
				break;

#ifdef USE_KILLER
			if ( killer == move )
				continue;
#endif

			THROW_IF( !board_.validMove(move), "move validation failed" );

			movement(depth, ply, alpha, betta, move, counter);
		}

		if ( !counter )
		{
			board_.setNoMoves();
			ScoreType s = board_.evaluate();
			if ( Board::ChessMat == board_.getState() )
				s += ply;
			return s;
		}
	}
	else
	{
    //QpfTimer qpt;
		CapsChecksGenerator ccg(board_, minimalType, depth, ply, *this, alpha, betta, counter);
    //Board::ticks_ += qpt.ticks();

		for ( ; !stop_ && alpha < betta ; )
		{
			const Move & cap = ccg.capture();
			if ( !cap || stop_ )
				break;

			if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
				testTimer();

#ifdef USE_KILLER
			if ( killer == cap )
				continue;
#endif

			THROW_IF( !board_.validMove(cap), "move validation failed" );

			movement(depth, ply, alpha, betta, cap, counter);
		}

		if ( !stop_ && alpha < betta )
		{
      //QpfTimer qpt;
		  ChecksGenerator chkg(board_, ply, *this, alpha, betta, counter);
      //Board::ticks_ += qpt.ticks();
      //Board::tcounter_++;

		  for ( ; !stop_ && alpha < betta ; )
		  {
			  const Move & check = chkg.check();
			  if ( !check || stop_ )
				  break;

			  if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
				  testTimer();

#ifdef USE_KILLER
			  if ( killer == check )
				  continue;
#endif

			  THROW_IF( !board_.validMove(check), "move validation failed" );
			  //bool v = false;
			  //if ( !board_.validMove(check) )
			  //{
  			//	v = board_.validMove(check);
			  //}

			  movement(depth, ply, alpha, betta, check, counter);
		  }
		}
	}

	return alpha;
}
