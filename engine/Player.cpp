
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
  depth_(0),
#ifdef USE_HASH_TABLE_CAPTURE
  chash_(16),
#endif
#ifdef USE_HASH_TABLE_GENERAL
  ghash_(16),
  use_pv_(false)
#else
  use_pv_(true)
#endif
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

void Player::setMemory(int mb)
{
  int gsize = 0;

  if ( mb > 4 ) // we need at least 4mb of memory
  {
    mb -= 4;
    gsize = 16;
    for ( ; mb > 1; mb >>= 1, gsize++);
    gsize--;
  }

#ifdef USE_HASH_TABLE_GENERAL
  ghash_.resize(gsize);
#endif

#ifdef USE_HASH_TABLE_CAPTURE
  chash_.resize(gsize);
#endif

  use_pv_ = gsize == 0;
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

    ScoreType score = alphaBetta(depth_, 0, alpha, betta, false, false);

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
ScoreType Player::nullMove(int depth, int ply, ScoreType alpha, ScoreType betta)
{
  if ( (board_.getState() == Board::UnderCheck) || !board_.allowNullMove() || (ply < 1) || (depth < 2) || (depth_ < 3) || betta >= Figure::WeightMat-MaxPly )
    return alpha;

#ifndef NDEBUG
  Board board0(board_);
#endif

  MoveCmd move;
  board_.makeNullMove(move);

  int depth1 = depth-4;
  depth >>= 1;
  if ( depth1 < depth && depth1 > 0 )
    depth = depth1;
  if ( depth < 1 )
    depth = 1;

  ScoreType nullScore = -alphaBetta(depth, ply+1, -betta, -(betta-1), true /* null-move */, false /* extension */);

  board_.unmakeNullMove(move);

  THROW_IF( board0 != board_, "nullMove wasn't correctly unmake" );

  return nullScore;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, bool null_move, bool extension)
{
  if ( stop_ || ply >= MaxPly || alpha >= Figure::WeightMat-ply )
    return alpha;


#ifdef USE_FUTILITY_PRUNING
  if ( Board::UnderCheck != board_.getState() && alpha > -Figure::WeightMat+MaxPly && alpha < Figure::WeightMat-MaxPly && depth == 1 && ply > 1 )
  {
    ScoreType score = board_.evaluate();
    int delta = (int)alpha - (int)score - (int)Figure::positionGain_;
    if ( delta > 0 )
      return captures(depth, ply, alpha, betta, delta);
  }
#endif


  int counter = 0;
  
  // if we haven't found best move, we write alpha-flag to hash
  ScoreType savedAlpha = alpha;

  Move pv;
  pv.clear();

  if ( use_pv_ )
  {
    // we use PV only up to max-depth
    if ( ply < depthMax_ )
    {
      pv = contexts_[0].pv_[ply];
      pv.checkVerified_ = 0;

      THROW_IF( pv.rindex_ == 100, "invalid pv move" );
    }
  }
#ifdef USE_HASH_TABLE_GENERAL
  else // use hash table
  {
    GeneralHItem & hitem = ghash_[board_.hashCode()];
    if ( hitem.hcode_ == board_.hashCode() )
    {
      ScoreType hscore = hitem.score_;
      if ( hscore >= Figure::WeightMat-MaxPly )
      {
        hscore += hitem.ply_;
        hscore -= ply;
      }
      else if ( hscore <= MaxPly-Figure::WeightMat )
      {
        hscore -= hitem.ply_;
        hscore += ply;
      }

      if ( GeneralHashTable::Alpha != hitem.flag_ )
      {
        pv = board_.unpack(hitem.move_);
      }

      if ( hitem.depth_ >= depth )
      {
        if ( GeneralHashTable::Alpha == hitem.flag_ && hscore <= alpha )
          return alpha;

#ifdef RETURN_IF_BETTA
        if ( (GeneralHashTable::Betta == hitem.flag_ || GeneralHashTable::Betta == hitem.flag_) && pv && hscore >= betta )
        {
          totalNodes_++;
          nodesCount_++;

#ifndef NDEBUG
          Board board0 = board_;
#endif

          bool retBetta = false;

          if ( board_.makeMove(pv) )
          {
            if ( board_.drawState() )
            {
              if ( 0 >= betta )
                retBetta = true;
            }
            else if ( board_.repsCount() < 2 )
            {
              assemblePV(pv, ply);
              retBetta = true;
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

          if ( retBetta )
          {
            return betta;
          }
        }
#endif // RETURN_IF_BETTA
      }
    }

#ifdef USE_HASH_TABLE_ADV
    // if we haven't found pv in general hash, lets try captures hash
    if ( !pv )
    {
      CaptureHItem & hitem = chash_[board_.hashCode()];
      if ( hitem.hcode_ == board_.hashCode() )
      {
        if ( hitem.move_ )
          pv = board_.unpack(hitem.move_);
      }
    }
#endif // USE_HASH_TABLE_ADV

  }
#endif // USE_HASH_TABLE_GENERAL

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
  if ( board_.getState() != Board::UnderCheck )
    verifyChecksGenerator(depth, ply, alpha, betta, Figure::TypeKing);
#endif

  // first of all try null-move
#ifdef USE_NULL_MOVE
  if ( !null_move )
  {
    ScoreType nullScore = nullMove(depth, ply, alpha, betta);
    if ( nullScore >= betta )
    {
      int depth1 = depth-4;
      depth >>= 1;
      if ( depth1 < depth && depth1 > 0 )
        depth = depth1;
      if ( depth < 1 )
        depth = 1;
      null_move = true;
    }
  }
#endif

  // try move from hash and killer (only if we are not under check)
  if ( board_.getState() != Board::UnderCheck && pv && board_.validMove(pv) )
    movement(depth, ply, alpha, betta, pv, counter, null_move, extension);

  if ( alpha >= betta )
  {
    return alpha;
  }

  if ( Board::UnderCheck == board_.getState() )
  {
    EscapeGenerator eg(pv, board_, depth, ply, *this, alpha, betta, counter);

    if ( (eg.count() == 1 || board_.getNumOfChecking() == 2)&& alpha < Figure::WeightMat-MaxPly )
      depth++;

    for ( ; !stop_ && alpha < betta ; )
    {
      const Move & move = eg.escape();
      if ( !move || stop_ )
        break;

      if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
        testTimer();

      movement(depth, ply, alpha, betta, move, counter, null_move, extension);
    }
  }
  else
  {
#ifdef USE_KILLER_ADV
	  Move killer = contexts_[ply].killer_;
	  if ( killer && killer != pv && killer.rindex_ >= 0 && board_.validMove(killer) )
	  {
		  movement(depth, ply, alpha, betta, killer, counter, null_move, extension);
		  if ( alpha >= betta )
			  return alpha;
	  }
	  else
		  killer.clear();
#endif

	  MovesGenerator mg(board_, depth, ply, this, alpha, betta, counter, null_move, extension);
	  for ( ; !stop_ && alpha < betta ; )
	  {
	    const Move & move = mg.move();
	    if ( !move || stop_ )
		    break;

	    if ( move == pv 
#ifdef USE_KILLER_ADV
			|| move == killer
#endif
			)
		    continue;

	    if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
		    testTimer();

      if ( stop_ )
        break;
  	  
	    movement(depth, ply, alpha, betta, move, counter, null_move, extension);
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

  // we haven't found best move
#ifdef USE_HASH_TABLE_GENERAL
  if ( alpha == savedAlpha )
  {
    ghash_.push(board_.hashCode(), alpha, depth, ply, board_.getColor(), GeneralHashTable::Alpha, PackedMove());
  }
#endif

  return alpha;
}
//////////////////////////////////////////////////////////////////////////
void Player::movement(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & move, int & counter, bool null_move, bool extension)
{
  totalNodes_++;
  nodesCount_++;

#ifndef NDEBUG
  Board board0 = board_;
#endif

  uint64 hcode = board_.hashCode();

  if ( board_.makeMove(move) )
  {
    bool ext = false;
    History & hist = MovesGenerator::history(move.from_, move.to_);

    bool haveCheck = board_.getState() == Board::UnderCheck;
    if ( (haveCheck || Figure::TypeQueen == move.new_type_) && depth > 0 &&
          alpha < Figure::WeightMat-MaxPly )
    {
      ext = extension = true;
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
        score = -captures(depth-1, ply+1, -betta1, -alpha, delta);
      }
    }
    else
    {
#ifdef USE_ZERO_WINDOW
      if ( counter > 1 )
      {
        int R = 1;

#ifdef USE_LMR
        if ( counter > 3 && depth > 3 &&
             !null_move && !ext &&
             hist.bad_count_ >= (hist.good_count_ << 1) &&
             board_.canBeReduced(move) )
          R = 2;
#endif

        score = -alphaBetta(depth-R, ply+1, -(alpha+1), -alpha, null_move, extension);

#ifdef USE_LMR

        //if ( score <= alpha && R > 1 && betta>alpha+1) // verify LMR
        //{
        //  ScoreType score1 = -alphaBetta(depth-1, ply+1, -betta, -alpha, null_move, extension);
        //  if ( score1 >= betta )
        //  {
        //    Board::ticks_++;
        //    Board::tcounter_ += hist.score_;
        //  }
        //}

        if ( score > alpha && R > 1 ) // was LMR
          score = -alphaBetta(depth-1, ply+1, -(alpha+1), -alpha, null_move, extension);
#endif
      }
      if ( counter < 2  || (score > alpha && score < betta) )
#endif
        score = -alphaBetta(depth-1, ply+1, -betta, -score, null_move, extension);
    }

    if ( !stop_ )
    {
      if ( score > alpha )
      {
        alpha = score;

        assemblePV(move, ply);

#ifdef USE_HASH_TABLE_GENERAL
        if ( depth >= 1 )
          updateGeneralHash(move, depth, ply, score, betta, hcode);
#ifdef USE_HASH_TABLE_CAPTURE
        else
          updateCaptureHash(move, score, betta, hcode);
#endif
#endif

        if ( move.rindex_ < 0 && !move.new_type_ )
          hist.score_ += depth;

        if ( 0 == ply )
        {
          best_ = move;
          if ( before_ == best_ )
            beforeFound_ = true;
        }

#ifdef USE_KILLER
        Move & killer = contexts_[ply].killer_;
        //if ( score > killer.score_ )
        {
          killer = move;
          killer.score_ = score;
        }
#endif
      }
      if ( move.rindex_ < 0 && !move.new_type_ )
      {
        if ( alpha >= betta )
          hist.good_count_++;
        else
          hist.bad_count_++;
      }
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
ScoreType Player::captures(int depth, int ply, ScoreType alpha, ScoreType betta, int delta)
{
	if ( stop_ || ply >= MaxPly )
		return alpha;

  if ( ply < MaxPly )
    contexts_[ply+1].killer_.clear();

#ifdef VERIFY_CAPS_GENERATOR
  if ( board_.getState() != Board::UnderCheck )
    verifyCapsGenerator(ply, alpha, betta, delta);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  if ( board_.getState() != Board::UnderCheck )
    verifyChecksGenerator(depth, ply, alpha, betta, Figure::TypeKing);
#endif

  int counter = 0;
  ScoreType saveAlpha = alpha;

  Move hmove;
  hmove.clear();

#ifdef USE_HASH_TABLE_CAPTURE
  CaptureHItem & hitem = chash_[board_.hashCode()];
  if ( hitem.hcode_ == board_.hashCode() )
  {
    if ( CapturesHashTable::Alpha == hitem.flag_ && hitem.score_ <= alpha )
      return alpha;

    if ( CapturesHashTable::Alpha != hitem.flag_ )
      hmove = board_.unpack(hitem.move_);

#ifdef RETURN_IF_BETTA
    if ( (GeneralHashTable::Betta == hitem.flag_ || GeneralHashTable::Betta == hitem.flag_) && hmove && hitem.score_ >= betta )
    {
      totalNodes_++;
      nodesCount_++;

#ifndef NDEBUG
      Board board0 = board_;
#endif

      bool retBetta = false;

      if ( board_.makeMove(hmove) )
      {
        if ( board_.drawState() )
        {
          if ( 0 >= betta )
            retBetta = true;
        }
        else
        {
          retBetta = true;
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

      if ( retBetta )
      {
        return betta;
      }
    }
#endif // RETURN_IF_BETTA

    if ( hmove )
    {
      hmove.checkVerified_ = 0;
      capture(depth, ply, alpha, betta, hmove, counter);
  
      if ( alpha >= betta )
	  {
		return alpha;
	  }
    }
  }

#ifdef USE_GENERAL_HASH_IN_CAPS
  if ( !hmove )
  {
    GeneralHItem & hitem = ghash_[board_.hashCode()];
    if ( hitem.hcode_ == board_.hashCode() && hitem.move_ )
    {
      hmove = board_.unpack(hitem.move_);
      if ( hmove && hmove.rindex_ >= 0 )
      {
        hmove.checkVerified_ = 0;
        capture(depth, ply, alpha, betta, hmove, counter);

        if ( alpha >= betta )
		{
			return alpha;
		}
      }
      else
        hmove.clear();
    }
  }
#endif

#endif //USE_HASH_TABLE_CAPTURE

  Figure::Type minimalType = delta2type(delta);

#ifdef USE_KILLER_CAPS
  Move killer = contexts_[ply].killer_;
  if ( killer && killer != hmove && killer.rindex_ >= 0 && board_.validMove(killer) &&
	  board_.getFigure(Figure::otherColor(board_.getColor()), killer.rindex_).getType() >= minimalType)
  {
	  capture(depth, ply, alpha, betta, killer, counter);
	  if ( alpha >= betta )
		  return alpha;
  }
  else
	  killer.clear();
#endif


  if ( board_.getState() == Board::UnderCheck )
  {
	  EscapeGenerator eg(board_, 0, ply, *this, alpha, betta, counter);

	  for ( ; !stop_ && alpha < betta ; )
	  {
		  const Move & move = eg.escape();
		  if ( !move || stop_ )
			  break;

		  if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
			  testTimer();

		  if ( stop_ )
			break;

		  if ( hmove == move
#ifdef USE_KILLER_CAPS
			  || move == killer
#endif
			  )
			  continue;

		  THROW_IF( !board_.validMove(cap), "move validation failed" );

		  capture(depth, ply, alpha, betta, move, counter);
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
    // generate only suitable captures
    CapsGenerator cg(board_, minimalType, ply, *this, alpha, betta, counter);
    for ( ; !stop_ && alpha < betta ; )
    {
      const Move & cap = cg.capture();
      if ( !cap || stop_ )
        break;

      if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
        testTimer();

      if ( hmove == cap 
#ifdef USE_KILLER_CAPS
		  || cap == killer
#endif
		  )
        continue;

      THROW_IF( !board_.validMove(cap), "move validation failed" );

      capture(depth, ply, alpha, betta, cap, counter);
    }

#ifdef PERFORM_CHECKS_IN_CAPTURES
    // generate check only on 1st iteration under horizon
    if ( depth >= 0 && !stop_ && alpha < betta )
    {
      ChecksGenerator ckg(&cg, board_, ply, *this, alpha, betta, minimalType, counter);

      for ( ; !stop_ && alpha < betta ; )
      {
        const Move & check = ckg.check();
        if ( !check || stop_ )
          break;

        if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
          testTimer();

        if ( hmove == check
#ifdef USE_KILLER_CAPS
			|| check == killer
#endif
			)
          continue;

        THROW_IF( !board_.validMove(check), "move validation failed" );

        capture(depth, ply, alpha, betta, check, counter);
      }
    }
#endif // PERFORM_CHECKS_IN_CAPTURES

  }

#ifdef USE_HASH_TABLE_CAPTURE
  if ( alpha == saveAlpha )
  {
    chash_.push(board_.hashCode(), alpha, board_.getColor(), CapturesHashTable::Alpha, PackedMove());
  }
#endif

  return alpha;
}

//////////////////////////////////////////////////////////////////////////
void Player::capture(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & cap, int & counter)
{
	totalNodes_++;
	nodesCount_++;

#ifndef NDEBUG
	Board board0 = board_;
#endif

  uint64 hcode = board_.hashCode();

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
				s = -captures(depth-1, ply+1, -betta1, -alpha, delta);
			}
		}
		if ( !stop_ && s > alpha )
		{
			alpha = s;

#ifdef USE_KILLER_CAPS
			//if ( s > contexts_[ply].killer_.score_ )
			{
				contexts_[ply].killer_ = cap;
				contexts_[ply].killer_.score_ = s;
			}
#endif

#ifdef USE_HASH_TABLE_CAPTURE
			updateCaptureHash(cap, s, betta, hcode);
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
