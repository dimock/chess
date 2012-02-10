
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

inline int nullMove_depth(int depth)
{
  int depth1 = depth - NullMove_PlyReduce;
  depth >>= 1;
  if ( depth > depth1 )
    depth = depth1;
  //depth -= NullMove_PlyReduce;
  if ( depth < NullMove_DepthMin )
    depth = NullMove_DepthMin;
  return depth;
}

inline bool find_move(const Move * moves, int n, const Move & m)
{
  for (int i = 0; i < n; ++i)
  {
    if ( moves[i] == m )
      return true;
  }
  return false;
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
  plyMax_(0),
  original_material_balance_(0),
#ifdef USE_HASH_TABLE_CAPTURE
  chash_(20),
#endif
#ifdef USE_HASH_TABLE_GENERAL
  ghash_(20),
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
  use_pv_ = true;
  if ( mb < 1 )
    return;

  int ghitemSize = sizeof(GeneralHItem);
  int chitemSize = sizeof(CaptureHItem);
  int bytesN = mb*1024*1024;

  int hsize = log2(bytesN/ghitemSize) - 1;
  if ( hsize < 10 )
    return;

#ifdef USE_HASH_TABLE_GENERAL
  ghash_.resize(hsize);
  use_pv_ = false;
#endif

#ifdef USE_HASH_TABLE_CAPTURE
  chash_.resize(hsize);
  use_pv_ = false;
#endif

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

  *out << sres.depth_ << " " << sres.score_ << " " << (int)sres.dt_ << " " << sres.totalNodes_;
  for (int i = 0; i < sres.depth_ && sres.pv_[i]; ++i)
  {
    *out << " ";

    Move pv = sres.pv_[i];
    pv.clearFlags();

    if ( !pv_board.validMove(pv) || !pv_board.makeMove(pv) )
      break;

    pv_board.unmakeMove();

    char str[64];
    if ( !printSAN(pv_board, pv, str) )
      break;

    pv_board.makeMove(pv);

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
  
  // for recapture extension
  original_material_balance_ = board_.material();

  MovesGenerator::clear_history();

  before_.clear();
  contexts_[0].killer_.clear();

  contexts_[0].clearPV(depthMax_);

  for (depth_ = 1; !stop_ && depth_ <= depthMax_; ++depth_)
  {
    Board pv_board(board_);
    pv_board.set_moves(pv_moves_);

    best_.clear();
    beforeFound_ = false;
    nodesCount_ = 0;
	  plyMax_ = 0;

    ScoreType alpha = -std::numeric_limits<ScoreType>::max();
    ScoreType betta = +std::numeric_limits<ScoreType>::max();

    ScoreType score = alphaBetta(depth_, 0, alpha, betta, false);

    if ( best_ && (!stop_ || (stop_ && beforeFound_) || (2 == depth_)) )
    {
      clock_t t  = clock();
      clock_t dt = (t - tstart_) / 10;
      tprev_ = t;

      sres.score_ = score;
      sres.best_  = best_;
      sres.depth_ = depth_;
      sres.nodesCount_ = nodesCount_;
      sres.totalNodes_ = totalNodes_;
	    sres.plyMax_ = plyMax_;
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
    {
      break;
    }
  }

  sres.totalNodes_ = totalNodes_;

  return sres.best_;
}

void Player::testTimer()
{
#ifndef NO_TIME_LIMIT
  int t = clock();
  stop_ = stop_ || ( (t - tstart_) > timeLimitMS_);
#endif
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::nullMove(int depth, int ply, ScoreType alpha, ScoreType betta)
{
  if ( (board_.getState() == Board::UnderCheck) || !board_.allowNullMove() ||
       (ply < 1) || (ply >= MaxPly-1) || (depth < 2) || (depth_ < 4) || betta >= Figure::WeightMat-MaxPly ||
        alpha <= -Figure::WeightMat+MaxPly || board_.getMoveRev(0).rindex_ >= 0 || board_.getMoveRev(0).new_type_ == Figure::TypeQueen )
    return alpha;

#ifndef NDEBUG
  Board board0(board_);
#endif

  MoveCmd move;
  board_.makeNullMove(move);

  depth = nullMove_depth(depth);

  ScoreType nullScore = -alphaBetta(depth, ply+1, -betta, -(betta-1), true /* null-move */);

  board_.unmakeNullMove(move);

  THROW_IF( board0 != board_, "nullMove wasn't correctly unmake" );

  // set threat flag in current context if null-move failed
  // we need to verify this position more carefully to prevent reduction of dangerous movement on ply-1
  if ( nullScore <= alpha )
  {
    contexts_[ply].ext_data_.mbe_move_ = contexts_[ply+1].pv_[ply+1];
    contexts_[ply].null_move_threat_ = 1;
  }

#ifdef MAT_THREAT_EXTENSION
  if ( nullScore <= -Figure::WeightMat+MaxPly && nullScore > -std::numeric_limits<ScoreType>::max() )
    contexts_[ply].ext_data_.mat_threats_found_++;
#endif

#ifdef MARKOFF_BOTVINNIK_EXTENSION
  // Markoff-Botvinnik extension ??? don't completely understand the reason to do it. just an experiment
  if ( ply > 2 && contexts_[ply].ext_data_.mbe_move_ &&
       contexts_[ply-2].ext_data_.mbe_move_ == contexts_[ply].ext_data_.mbe_move_ )
  {
    contexts_[ply].ext_data_.mbe_threat_ = true;
  }
#endif

  return nullScore;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, bool null_move)
{
	if ( ply > plyMax_ )
		plyMax_ = ply;

  if ( stop_ || ply >= MaxPly || alpha >= Figure::WeightMat-ply )
  {
    THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
    return alpha;
  }

  int counter = 0;

  // if we haven't found best move, we write alpha-flag to hash
  ScoreType savedAlpha = alpha;
  bool inPvNode = betta > alpha+1;
  int goodCount = 0;

#ifdef USE_HASH_TABLE_GENERAL
  if ( !use_pv_ ) // use hash table
  {
    GeneralHashTable::Flag flag = getGeneralHashFlag(depth, ply, alpha, betta);
    if ( GeneralHashTable::Alpha == flag )
    {
      THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
      return alpha;
    }

    else if ( GeneralHashTable::Betta == flag )
    {
      if ( ghash_[board_.hashCode()].threat_ && board_.halfmovesCount() > 0 && board_.getMoveRev(0).reduced_ )
        return betta - 1;

      THROW_IF( !stop_ && (betta < -32760 || betta > 32760), "invalid score" );
      return betta;
    }
  }
#endif // USE_HASH_TABLE_GENERAL

  // clear context for current ply
  // copy extensions counters from prev.ply context
  if ( ply < MaxPly-1 )
  {
    contexts_[ply+1].killer_.clear();
    contexts_[ply].clear(ply);

    if ( ply > 0 )
      contexts_[ply].ext_data_.copy_counters(contexts_[ply-1].ext_data_);
  }

#ifdef VERIFY_ESCAPE_GENERATOR
  if ( board_.getState() == Board::UnderCheck )
    verifyEscapeGen(depth, ply, alpha, betta);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  if ( board_.getState() != Board::UnderCheck )
    verifyChecksGenerator(depth, ply, alpha, betta, Figure::TypeKing);
#endif

#ifdef VERIFY_CAPS_GENERATOR
  if ( board_.getState() != Board::UnderCheck )
    verifyCapsGenerator(ply, alpha, betta, 0);
#endif

  // first of all try null-move
#ifdef USE_NULL_MOVE
  if ( !null_move )
  {
    ScoreType nullScore = nullMove(depth, ply, alpha, betta);

    if ( nullScore >= betta )
    {
      if ( board_.material(board_.getColor()) <= Figure::figureWeight_[Figure::TypeQueen] + Figure::figureWeight_[Figure::TypeRook] + Figure::figureWeight_[Figure::TypeKnight] )
      {
        depth--;
        if ( depth < NullMove_DepthMin )
          depth = NullMove_DepthMin;
      }
      else
      {
        depth = nullMove_depth(depth);
        null_move = true;
      }
    }
    // mat threat extension
#ifdef MAT_THREAT_EXTENSION
    else if ( ply > 1 && contexts_[ply].ext_data_.mat_threats_found_ > 1 &&
              contexts_[ply].ext_data_.mat_threat_count_ < MatThreatExtension_Limit )
    {
      contexts_[ply].ext_data_.mat_threat_count_++;
      depth++;
    }
#endif
#ifdef MARKOFF_BOTVINNIK_EXTENSION
    // Markoff-Botvinnic extension
    else if ( ply > 2 && contexts_[ply].ext_data_.mbe_threat_  &&
              contexts_[ply].ext_data_.mbe_count_ < MbeExtension_Limit )
    {
      contexts_[ply].ext_data_.mbe_count_++;
      depth++;
    }
#endif
  }
#endif

#ifdef USE_FUTILITY_PRUNING
  if ( Board::UnderCheck != board_.getState() && alpha > -Figure::WeightMat+MaxPly && alpha < Figure::WeightMat-MaxPly &&
       depth == 1 && ply > 1 )
  {
    ScoreType score = board_.evaluate();
    int delta = (int)alpha - (int)score - (int)Figure::positionGain_;
    if ( delta > 0 )
      return captures(1, ply, alpha, betta, delta, true);
  }
#endif

  // collect all hashed moves together
  Move hmoves[HashedMoves_Size];
  int hmovesN = collectHashMoves(depth, ply, null_move, alpha, betta, hmoves);

  if ( Board::UnderCheck == board_.getState() )
  {
    EscapeGenerator eg(hmoves[0], board_, depth, ply, *this, alpha, betta, counter);

    if ( (eg.count() == 1 || board_.getNumOfChecking() == 2)&& alpha < Figure::figureWeight_[Figure::TypeRook] && alpha > -Figure::figureWeight_[Figure::TypeRook] )
    {
      //contexts_[ply].ext_data_.doublechecks_count_++;
      depth++;
    }

    for ( ; !stop_ && alpha < betta; )
    {
      Move & move = eg.escape();
      if ( !move || stop_ )
        break;

      if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
        testTimer();

      movement(depth, ply, alpha, betta, move, counter, null_move);
    }
  }
  else
  {
    //Move firstMove;
    //Move singleMove;
    //singleMove.clear();
    //firstMove.clear();

    // first of all try moves, collected from hash
    for (Move * m = hmoves; !stop_ && alpha < betta && *m; ++m)
    {
      //ScoreType alpha0 = alpha;
      // if movement return true we were reduced threat move and need to recalculate it with full depth
      if ( movement(depth, ply, alpha, betta, *m, counter, null_move) )
      {
        THROW_IF( !stop_ && (betta < -32760 || betta > 32760), "invalid score" );
        return betta - 1;
      }

      //if ( !firstMove && counter == 1 )
      //  firstMove = *m;

      //if ( alpha0 > alpha+50 && alpha > -Figure::WeightMat+MaxPly )
      //{
      //  goodCount++;
      //  singleMove = *m;
      //}

      //alpha = alpha0;
    }

    if ( stop_ || alpha >= betta )
    {
      THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
      return alpha;
    }

    MovesGenerator mg(board_, depth, ply, this, alpha, betta, counter);
    for ( ; !stop_ && alpha < betta ; )
    {
      Move & move = mg.move();
      if ( !move || stop_ )
        break;

      if ( find_move(hmoves, hmovesN, move) )
        continue;

      if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
        testTimer();

      if ( stop_ )
        break;

      //ScoreType alpha0 = alpha;
      if ( movement(depth, ply, alpha, betta, move, counter, null_move) )
      {
        THROW_IF( !stop_ && (betta < -32760 || betta > 32760), "invalid score" );
        return betta - 1;
      }

      //if ( !firstMove && counter == 1 )
      //  firstMove = move;

      //if ( alpha0 > alpha+50 && alpha > -Figure::WeightMat+MaxPly )
      //{
      //  goodCount++;
      //  singleMove = move;
      //}

      //alpha = alpha0;
    }

//    if ( !stop_ && !null_move && inPvNode && alpha < betta )
//    {
///*      if ( goodCount == 1 && contexts_[ply].ext_data_.singular_count_ < SingularExtension_Limit && singleMove )
//      {
//        alpha = savedAlpha;
//        int counter1 = 0;
//        contexts_[ply].ext_data_.singular_count_++;
//        movement(depth+1, ply, alpha, betta, singleMove, counter1, null_move);
//      }
//      else */if ( counter == 1 && firstMove && contexts_[ply].ext_data_.singular_count_ < SingularExtension_Limit )
//      {
//        alpha = savedAlpha;
//        int counter1 = 0;
//        contexts_[ply].ext_data_.singular_count_++;
//        movement(depth+1, ply, alpha, betta, firstMove, counter1, null_move);
//      }
//    }
  }

  if ( stop_ )
    return alpha;

  if ( 0 == counter )
  {
    board_.setNoMoves();
    ScoreType s = board_.evaluate();
    if ( Board::ChessMat == board_.getState() )
      s += ply;

#ifdef USE_HASH_TABLE_GENERAL
    GeneralHashTable::Flag flag;
    if ( s <= savedAlpha )
      flag = GeneralHashTable::Alpha;
    else if ( s >= betta )
      flag = GeneralHashTable::Betta;
    else
      flag = GeneralHashTable::AlphaBetta;

    ghash_.push(board_.hashCode(), s, depth, ply, board_.halfmovesCount(), board_.getColor(),  flag, PackedMove());
#endif

    THROW_IF( !stop_ && (s < -32760 || s > 32760), "invalid score" );
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
    ghash_.push(board_.hashCode(), alpha, depth, ply, board_.halfmovesCount(), board_.getColor(), GeneralHashTable::Alpha, PackedMove());
  }
#endif

  THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
  return alpha;
}
//////////////////////////////////////////////////////////////////////////
bool Player::movement(int depth, int ply, ScoreType & alpha, ScoreType betta, Move & move, int & counter, bool null_move)
{
  totalNodes_++;
  nodesCount_++;

#ifndef NDEBUG
  Board board0 = board_;
#endif

  uint64 hcode = board_.hashCode();
  int depth0 = depth;
  Figure::Color color = board_.getColor();
  bool check_esc = board_.getState() == Board::UnderCheck;
  contexts_[ply].threat_ = false;

  // previous move was reduced
  bool reduced = board_.halfmovesCount() > 0 && board_.getMoveRev(0).reduced_;

  if ( board_.makeMove(move) )
  {
    MoveCmd & mv_cmd = board_.getMoveRev(0);
    mv_cmd.extended_ = false;
    History & hist = MovesGenerator::history(move.from_, move.to_);

    bool haveCheck = board_.getState() == Board::UnderCheck;
    move.checkFlag_ = haveCheck;
    if ( depth > 0 && alpha < Figure::WeightMat-MaxPly && 
         (haveCheck || Figure::TypeQueen == move.new_type_ || pawnBeforePromotion(move) 
#ifdef RECAPTURE_EXTENSION
         || recapture(ply)
#endif
         ) )
    {
      mv_cmd.extended_ = true;
      depth++;

      //if ( haveCheck )
      //  contexts_[ply].ext_data_.checks_count_++;

      // extend one more ply to be sure that opponent doesn't capture promoted queen (?)
      if ( move.new_type_ == Figure::TypeQueen )
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
      if ( haveCheck || score > alpha )
      {
        ScoreType betta1 = score < betta && !haveCheck ? score : betta;
        score = -captures(depth-1, ply+1, -betta1, -alpha, delta, true);
        THROW_IF( !stop_ && (score < -32760 || score > 32760), "invalid score" );
      }
    }
    else
    {
#ifdef USE_ZERO_WINDOW
      if ( counter > 1 )
      {
        int R = 1;

#ifdef USE_LMR
        if (  counter > LMR_Counter &&
              depth_ > LMR_MinDepthLimit &&
              depth > LMR_DepthLimit &&
              !move.threat_ &&
              !move.strong_ &&
              !check_esc &&
              !null_move &&
              !mv_cmd.extended_ &&
              alpha > -Figure::WeightMat+MaxPly &&
              ((hist.good_count_<<1) <= hist.bad_count_) &&
              board_.canBeReduced(move) )
        {
          R = LMR_PlyReduce;
          mv_cmd.reduced_ = true;
        }
#endif

        score = -alphaBetta(depth-R, ply+1, -(alpha+1), -alpha, null_move);

        THROW_IF( !stop_ && (score < -32760 || score > 32760), "invalid score" );
        mv_cmd.reduced_ = false;

#ifdef USE_LMR

        if ( !stop_ && score > alpha && R > 1 ) // was LMR
          score = -alphaBetta(depth-1, ply+1, -(alpha+1), -alpha, null_move);

        THROW_IF( !stop_ && (score < -32760 || score > 32760), "invalid score" );
#endif
      }

      if ( !stop_ && counter < 2  || (score > alpha && score < betta) )
#endif
        score = -alphaBetta(depth-1, ply+1, -betta, -score, null_move);

      THROW_IF( !stop_ && (score < -32760 || score > 32760), "invalid score" );
    }

    if ( !stop_ )
    {
      if ( score > alpha )
      {
        alpha = score;

        assemblePV(move, haveCheck, ply);

#ifdef USE_HASH_TABLE_GENERAL
        updateGeneralHash(move, depth0, ply, score, betta, hcode, color);
#endif

        if ( move.rindex_ < 0 && !move.new_type_ )
        {
          hist.score_ ++;
          if ( hist.score_ > History::history_max_ )
            History::history_max_ = hist.score_;
        }

        if ( 0 == ply )
        {
          best_ = move;
          if ( before_ == best_ )
            beforeFound_ = true;
        }

#ifdef USE_KILLER
        Move & killer = contexts_[ply].killer_;
        killer = move;
#endif
      }
      else if ( contexts_[ply].threat_ )
      {
        ghash_[hcode].tmove_ = board_.pack(move);
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

  // force to recalculate again with full depth
  if ( alpha >= betta && contexts_[ply].null_move_threat_ && ply > 0 && isRealThreat(move) )
  {
    contexts_[ply-1].threat_ = true;
    if ( reduced )
      return true;
    else if ( ghash_[board_.hashCode()].hcode_ == board_.hashCode() )
      ghash_[board_.hashCode()].threat_ = 1;
  }

  return false;
}


//////////////////////////////////////////////////////////////////////////
ScoreType Player::captures(int depth, int ply, ScoreType alpha, ScoreType betta, int delta, bool do_checks)
{
	if ( ply > plyMax_ )
		plyMax_ = ply;

  if ( stop_ || ply >= MaxPly || alpha >= Figure::WeightMat-ply )
  {
    //THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
    return board_.evaluate();
  }

  if ( ply < MaxPly )
    contexts_[ply+1].killer_.clear();

  bool extend_check = (board_.getState() == Board::UnderCheck && depth >= 0) || depth > 0;

#ifdef VERIFY_CAPS_GENERATOR
  if ( board_.getState() != Board::UnderCheck )
    verifyCapsGenerator(ply, alpha, betta, delta);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  if ( board_.getState() != Board::UnderCheck )
    verifyChecksGenerator(depth, ply, alpha, betta, Figure::TypeKing);
#endif

#ifdef VERIFY_ESCAPE_GENERATOR
  if ( board_.getState() == Board::UnderCheck )
    verifyEscapeGen(depth, ply, alpha, betta);
#endif

  int counter = 0;
  ScoreType saveAlpha = alpha;

#ifdef USE_HASH_TABLE_CAPTURE
  CaptureHItem & hitem = chash_[board_.hashCode()];
  if ( hitem.hcode_ == board_.hashCode() )
  {
    THROW_IF( (Figure::Color)hitem.color_ != board_.getColor(), "identical hash code but different color in captures" );

    if ( CapturesHashTable::Alpha == hitem.flag_ && hitem.score_ <= alpha )
    {
      THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
      return alpha;
    }

#ifdef RETURN_IF_BETTA
    if ( (GeneralHashTable::Betta == hitem.flag_ || GeneralHashTable::AlphaBetta == hitem.flag_) && hitem.move_ && hitem.score_ >= betta )
    {
      THROW_IF( !stop_ && (betta < -32760 || betta > 32760), "invalid score" );
      return betta;
    }
#endif // RETURN_IF_BETTA
  }
#endif //USE_HASH_TABLE_CAPTURE

  Figure::Type minimalType = delta2type(delta);

  Move hcaps[HashedMoves_Size];
  int hcapsN = collectHashCaps(ply, minimalType, hcaps);

  for (Move * c = hcaps; alpha < betta && !stop_ && *c; ++c)
  {
    capture(depth, ply, alpha, betta, *c, counter, extend_check);
  }

  if ( stop_ || alpha >= betta )
  {
    THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
    return alpha;
  }

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

      if ( find_move(hcaps, hcapsN, move) )
        continue;

      THROW_IF( !board_.validMove(move), "move validation failed" );

      capture(depth, ply, alpha, betta, move, counter, extend_check);
    }

    if ( !counter )
    {
      board_.setNoMoves();
      ScoreType s = board_.evaluate();
      if ( Board::ChessMat == board_.getState() )
        s += ply;

#ifdef USE_HASH_TABLE_CAPTURE
      CapturesHashTable::Flag flag;
      if ( s <= saveAlpha )
        flag = CapturesHashTable::Alpha;
      else if ( s >= betta )
        flag = CapturesHashTable::Betta;
      else
        flag = CapturesHashTable::AlphaBetta;
      chash_.push(board_.hashCode(), s, board_.getColor(), flag, depth, ply, board_.halfmovesCount(), PackedMove());
#endif

      THROW_IF( !stop_ && (s < -32760 || s > 32760), "invalid score" );
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

      if ( find_move(hcaps, hcapsN, cap) )
        continue;

      THROW_IF( !board_.validMove(cap), "move validation failed" );

      capture(depth, ply, alpha, betta, cap, counter, extend_check);
    }

#ifdef PERFORM_CHECKS_IN_CAPTURES
    // generate check only on 1st iteration under horizon
    if ( alpha < Figure::figureWeight_[Figure::TypeRook] && do_checks && !stop_ && alpha < betta )
    {
      ChecksGenerator ckg(&cg, board_, ply, *this, alpha, betta, minimalType, counter);

      for ( ; !stop_ && alpha < betta ; )
      {
        const Move & check = ckg.check();
        if ( !check || stop_ )
          break;

        if ( timeLimitMS_ > 0 && totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
          testTimer();

        if ( find_move(hcaps, hcapsN, check ) /*|| !isCheckDangerous(check, minimalType) */)
          continue;

        THROW_IF( !board_.validMove(check), "move validation failed" );

        capture(depth, ply, alpha, betta, check, counter, extend_check);
      }
    }
#endif // PERFORM_CHECKS_IN_CAPTURES

  }

#ifdef USE_HASH_TABLE_CAPTURE
  if ( alpha == saveAlpha )
  {
    chash_.push(board_.hashCode(), alpha, board_.getColor(), CapturesHashTable::Alpha, depth, ply, board_.halfmovesCount(), PackedMove());
  }
#endif

  THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
  return alpha;
}

//////////////////////////////////////////////////////////////////////////
void Player::capture(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & cap, int & counter, bool do_checks)
{
  totalNodes_++;
  nodesCount_++;

#ifndef NDEBUG
  Board board0 = board_;
#endif

  uint64 hcode = board_.hashCode();
  Figure::Color color = board_.getColor();

  if ( board_.makeMove(cap) )
  {
    History & hist = MovesGenerator::history(cap.from_, cap.to_);

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
      if ( haveCheck || (s > alpha && (delta < Figure::figureWeight_[Figure::TypeQueen] || do_checks)) )
      {
        ScoreType betta1 = s < betta && !haveCheck ? s : betta;
        s = -captures(depth-1, ply+1, -betta1, -alpha, delta, do_checks);
        THROW_IF( !stop_ && (s < -32760 || s > 32760), "invalid score" );
      }
    }
    if ( !stop_ && s > alpha )
    {
      alpha = s;

      assemblePV(cap, haveCheck, ply);

#ifdef USE_KILLER_CAPS
      contexts_[ply].killer_ = cap;
#endif

#ifdef USE_HASH_TABLE_CAPTURE
      updateCaptureHash(depth, ply, cap, s, betta, hcode, color);
#endif

      if ( cap.rindex_ < 0 && !cap.new_type_ )
      {
        hist.score_++;
        if ( hist.score_ > History::history_max_ )
          History::history_max_ = hist.score_;
      }
    }

    if ( cap.rindex_ < 0 && !cap.new_type_ )
    {
      if ( alpha >= betta )
        hist.good_count_++;
      else
        hist.bad_count_++;
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
int Player::collectHashMoves(int depth, int ply, bool null_move, ScoreType alpha, ScoreType betta, Move (&moves)[HashedMoves_Size])
{
  moves[0].clear();

  if ( ply >= MaxPly )
    return 0;

  int num = 0;

  Move pv;
  pv.clear();

  if ( use_pv_ )
  {
    // we use PV only up to max-depth
    if ( ply < depthMax_ )
    {
      pv = contexts_[0].pv_[ply];
      pv.checkVerified_ = 0;

      if ( !board_.validMove(pv) )
        pv.clear();

      THROW_IF( pv.rindex_ == 100, "invalid pv move" );
    }
  }
#ifdef USE_HASH_TABLE_GENERAL
  else
  {
    GeneralHItem & hgitem = ghash_[board_.hashCode()];
    if ( hgitem.move_ && hgitem.hcode_ == board_.hashCode() )
      pv = board_.unpack(hgitem.move_);

#if (defined USE_HASH_TABLE_ADV) && (defined USE_HASH_TABLE_CAPTURE)
    // if we haven't found pv in general hash, lets try captures hash
    if ( !pv )
    {
      CaptureHItem & hcitem = chash_[board_.hashCode()];
      if ( hcitem.move_ && hcitem.hcode_ == board_.hashCode() )
        pv = board_.unpack(hcitem.move_);
    }
#endif // USE_HASH_TABLE_ADV
  }

#ifdef USE_IID
  // internal iterative deeping
  // if we don't have move in hashes, let's try to calculate to reduced depth to find some appropriate move
  if ( ply > 0 && depth > 1 && !null_move && !pv && ghash_[board_.hashCode()].flag_ != GeneralHashTable::Alpha && alpha > -Figure::WeightMat+MaxPly )
  {
    depth--;
    if ( depth > 1 )
      depth--;

    ScoreType score = alphaBetta(depth, ply+1, alpha, alpha+1, false);
    GeneralHItem & hitem = ghash_[board_.hashCode()];
    if ( hitem.move_ && hitem.hcode_ == board_.hashCode() )
      pv = board_.unpack(hitem.move_);

#ifdef USE_HASH_TABLE_CAPTURE
    else
    {
      CaptureHItem & citem = chash_[board_.hashCode()];
      if ( citem.move_ && citem.hcode_ == board_.hashCode() )
        pv = board_.unpack(citem.move_);
    }
#endif
  }
#endif

#endif USE_HASH_TABLE_GENERAL

  // write 1st move from hash (or from PV)
  if ( pv )
    moves[num++] = pv;

  if ( board_.getState() == Board::UnderCheck )
  {
    moves[num].clear();
    return num;
  }


#if ((defined USE_HASH_MOVE_EX) && (defined USE_HASH_TABLE_GENERAL))
  GeneralHItem & hitem = ghash_[board_.hashCode()];
  if ( hitem.hcode_ == board_.hashCode() )
  {
  // extra moves from hash
    for (int i = 0; i < 2; ++i)
    {
      Move hmove_ex = board_.unpack(hitem.move_ex_[i]);
      if ( !hmove_ex || find_move(moves, num, hmove_ex) )
        continue;

      moves[num++] = hmove_ex;
    }

    // threat move, if we have one
    Move htmove = board_.unpack(hitem.tmove_);
    if ( htmove && !find_move(moves, num, htmove) )
    {
      htmove.threat_ = 1;
      moves[num++] = htmove;
    }
  }
#endif

#ifdef USE_KILLER_ADV
  // at the last push killer (only if it is capture)
  Move killer = contexts_[ply].killer_;
  if ( killer && killer.rindex_ >= 0 && !find_move(moves, num, killer) && board_.validMove(killer) )
  {
    killer.fkiller_ = 1;
    moves[num++] = killer;
  }
#endif

  moves[num].clear();

  return num;
}
//////////////////////////////////////////////////////////////////////////
int Player::collectHashCaps(int ply, Figure::Type minimalType, Move (&caps)[HashedMoves_Size])
{
  caps[0].clear();

  if ( ply >= MaxPly )
    return 0;

  int num = 0;

  Move hmove;
  hmove.clear();

#ifdef USE_HASH_TABLE_CAPTURE

  CaptureHItem & chitem = chash_[board_.hashCode()];
  if ( chitem.move_ && chitem.hcode_ == board_.hashCode() )
  {
    THROW_IF( (Figure::Color)chitem.color_ != board_.getColor(), "identical hash code but different color in captures" );
    hmove = board_.unpack(chitem.move_);
  }

#if ((defined USE_GENERAL_HASH_IN_CAPS) && (defined USE_HASH_TABLE_GENERAL))
  if ( !hmove )
  {
    GeneralHItem & ghitem = ghash_[board_.hashCode()];
    if ( ghitem.move_ && ghitem.hcode_ == board_.hashCode() )
    {
      hmove = board_.unpack(ghitem.move_);
      if ( hmove.rindex_ < 0 || board_.getFigure(Figure::otherColor(board_.getColor()), hmove.rindex_).getType() < minimalType )
        hmove.clear();
    }
  }
#endif

#endif //USE_HASH_TABLE_CAPTURE

  if ( hmove )
    caps[num++] = hmove;

#ifdef USE_KILLER_CAPS
  Move killer = contexts_[ply].killer_;
  if ( killer && killer != hmove && killer.rindex_ >= 0 && board_.validMove(killer) &&
       board_.getFigure(Figure::otherColor(board_.getColor()), killer.rindex_).getType() >= minimalType)
  {
    killer.checkVerified_ = 0;
    caps[num++] = killer;
  }
#endif

  caps[num].clear();

  return num;
}

//////////////////////////////////////////////////////////////////////////
// is given movement caused by previous? this mean that if we don't do this move we loose
// we actually check if moved figure was/willbe attacked by previously moved one or from direction it was moved from
//////////////////////////////////////////////////////////////////////////
bool Player::isRealThreat(const Move & move)
{
  // don't need to forbid if our answer is capture or check ???
  if ( move.rindex_ >= 0 || move.checkFlag_ )
    return false;

  const MoveCmd & prev = board_.getMoveRev(0);
  Figure::Color ocolor = Figure::otherColor(board_.getColor());

  const Field & pfield = board_.getField(prev.to_);
  THROW_IF( !pfield || pfield.color() != ocolor, "no figure of required color on the field it was move to while detecting threat" );
  const Figure & pfig = board_.getFigure(ocolor, pfield.index());
  THROW_IF( !pfig, "field is occupied but there is no figure in the list in threat detector" );

  // don't need forbid reduction of captures, checks, promotions and pawn's attack because we've already done it
  if ( prev.rindex_ >= 0 || prev.new_type_ > 0 || prev.checkingNum_ > 0 ||
       board_.isDangerPawn(prev) || pawnBeforePromotion(prev) )
  {
    return false;
  }

  const Field & cfield = board_.getField(move.from_);
  THROW_IF( !cfield || cfield.color() != board_.getColor(), "no figure of required color in while detecting threat" );
  const Figure & cfig = board_.getFigure(cfield.color(), cfield.index());
  THROW_IF( !cfig, "field is occupied but there is no figure in the list in threat detector" );

  // we have to put figure under attack
  if ( board_.ptAttackedBy(move.to_, pfig) )
    return true;

  // put our figure under attack
  int tindex = board_.getAttackedFrom(ocolor, move.to_, prev.from_);
  if ( tindex >= 0 )
    return true;

  // prev move was attack, and we should escape from it
  if ( board_.ptAttackedBy(move.from_, pfig) )
    return true;

  // our figure was attacked from direction, opened by prev movement
  int findex = board_.getAttackedFrom(ocolor, move.from_, prev.from_);
  if ( findex >= 0 )
    return true;

#ifdef EXTENDED_THREAT_DETECTION
  // we have to protect figure from attack of previous one (only if it isn't pawn)
  if ( pfig.getType() != Figure::TypePawn )
  {
    const uint64 & black = board_.fmgr().mask(Figure::ColorBlack);
    const uint64 & white = board_.fmgr().mask(Figure::ColorWhite);
    uint64 mask_all_inv = ~(black | white);
    uint64 amask = g_movesTable->caps(pfig.getType(), prev.to_);
    for ( ; amask; )
    {
      int n = least_bit_number(amask);     
      const Field & field = board_.getField(n);
      if ( !field || field.color() != board_.getColor() )
        continue;

      if ( pfig.getType() != Figure::TypeKnight && pfig.getType() != Figure::TypeKing )
      {
        const uint64 & btw_msk = g_betweenMasks->between(prev.to_, n);
        if ( (btw_msk & mask_all_inv) != btw_msk )
          continue;
      }

      // field is attacked by pfig. now test if it is also attacked by cfig
      Figure cfig_to(cfig);
      cfig_to.go(move.to_);

      if ( g_figureDir->dir(cfig_to, n) < 0 )
        continue;

      // may be it was already attacked by this figure
      if ( g_figureDir->dir(cfig, n) >= 0 )
        continue;

      if ( cfig.getType() == Figure::TypeKnight || cfig.getType() == Figure::TypePawn || cfig.getType() == Figure::TypeKing )
        return true;

      // can cfig go to n-field
      const uint64 & btw_msk_cfig = g_betweenMasks->between(move.to_, n);
      if ( (btw_msk_cfig & mask_all_inv) == btw_msk_cfig )
        return true;
    }
  }

  // we have to protect from figure, attacked through position, that prev figure left
  {
    const uint64 & black = board_.fmgr().mask(Figure::ColorBlack);
    const uint64 & white = board_.fmgr().mask(Figure::ColorWhite);
    uint64 mask_all_inv = ~(black | white);
    uint64 cmask = g_movesTable->caps(cfig.getType(), move.to_);
    Figure::Color ocolor = Figure::otherColor(board_.getColor());
    uint64 brq_msk = board_.fmgr().bishop_mask(ocolor) | board_.fmgr().rook_mask(ocolor) | board_.fmgr().queen_mask(ocolor);
    for ( ; cmask; )
    {
      int n = least_bit_number(cmask);
      const Field & field = board_.getField(n);
      if ( !field || field.color() != board_.getColor() )
        continue;

      if ( cfig.getType() != Figure::TypeKing && cfig.getType() != Figure::TypePawn && cfig.getType() != Figure::TypeKnight )
      {
        const uint64 & btw_msk = g_betweenMasks->between(move.to_, n);
        if ( (btw_msk & mask_all_inv) != btw_msk )
          continue;
      }

      const uint64 & msk_from = g_betweenMasks->from(n, prev.from_);
      if ( !msk_from )
        continue;

      uint64 att_msk = brq_msk & msk_from;
      for ( ; att_msk; )
      {
        int a = least_bit_number(att_msk);
        const Field & afield = board_.getField(a);
        if ( !afield )
          continue;

        THROW_IF( afield.color() != ocolor, "invalid figure color" );

        const Figure & afig = board_.getFigure(afield.color(), afield.index());

        THROW_IF( afig.getColor() != ocolor || (afig.getType() != Figure::TypeBishop && afig.getType() != Figure::TypeQueen && afig.getType() != Figure::TypeRook),
          "invalid figure color or type" );

        if ( g_figureDir->dir(afig, n) < 0 )
          continue;

        const uint64 & btw_msk = g_betweenMasks->between(afig.where(), n);
        if ( (btw_msk & mask_all_inv) == btw_msk )
          return true;
      }
    }
  }
#endif // EXTENDED_THREAT_DETECTION

  return false;
}
//////////////////////////////////////////////////////////////////////////
// do we need extent move if there is great pressure on king
int Player::need_extension(int counter)
{
  // get current move
  MoveCmd & move = board_.getMoveRev(0);

  const Figure & fig = board_.getFigure(move.to_);
  if ( fig.getType() == Figure::TypePawn )
  {
    // before promotion
    int8 y = move.to_ >> 3;
    if ( 1 == y || 6 == y )
      return 1;
  }

  // Attention - 'Ocolor' is attacker !!!

  Figure::Color color = board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  // don't need extension if there is already plenty of material
  if ( (board_.fmgr().weight() > +Figure::figureWeight_[Figure::TypeBishop] && ocolor) ||
       (board_.fmgr().weight() < -Figure::figureWeight_[Figure::TypeBishop] && color) ||
       (board_.fmgr().weight(ocolor) < (Figure::figureWeight_[Figure::TypePawn]*20)) )
       return -1;

  const Figure & king = board_.getFigure(color, Board::KingIndex);
  THROW_IF( !king || king.getType() != Figure::TypeKing, "no king found" );

  const uint64 & king_mask = g_movesTable->caps(Figure::TypeKing, king.where());
  const uint64 & fig_mask = g_movesTable->caps(fig.getType(), fig.where());

  uint64 around = king_mask & fig_mask;
  if ( !around )
    return -1;

  const uint64 & black = board_.fmgr().mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr().mask(Figure::ColorWhite);
  uint64 mask_all_inv = ~(black | white);

  const uint64 & b_mask  = board_.fmgr().bishop_mask(ocolor);
  const uint64 & r_mask  = board_.fmgr().rook_mask(ocolor);
  const uint64 & q_mask  = board_.fmgr().queen_mask(ocolor);
  const uint64 & kn_mask = board_.fmgr().knight_mask(ocolor);
  const uint64 & pw_mask = board_.fmgr().pawn_mask_o(ocolor);
  const uint64 & ki_mask = board_.fmgr().king_mask(ocolor);

  uint64 fig_mask_i = ~(1ULL << fig.where());

  bool npk = fig.getType() == Figure::TypeKnight || fig.getType() == Figure::TypePawn || fig.getType() == Figure::TypeKing;
  int attackers = 0;
  if ( npk )
    attackers++;

  Figure ffig = fig;
  ffig.go(move.from_);
  if ( move.new_type_ )
    ffig.setType(Figure::TypePawn);

  while ( around )
  {
    int n = least_bit_number(around);

    if ( !npk )
    {
      const uint64 & btw_msk = g_betweenMasks->between(fig.where(), n);
      if ( (btw_msk & mask_all_inv) != btw_msk )
        continue;
    }

    // field found. test if this field was attacked by our figure before movement?
    int fdir = g_figureDir->dir(ffig, n);
    if ( fdir >= 0 )
    {
      if ( npk )
        continue;

      const uint64 & btw_msk = g_betweenMasks->between(move.from_, n);
      if ( (btw_msk & mask_all_inv) == btw_msk )
        continue;
    }

    const uint64 & p_caps = g_movesTable->pawnCaps_o(color, n);
    const uint64 & n_caps = g_movesTable->caps(Figure::TypeKnight, n);
    const uint64 & q_caps = g_movesTable->caps(Figure::TypeQueen, n);
    const uint64 & b_caps = g_movesTable->caps(Figure::TypeBishop, n);
    const uint64 & r_caps = g_movesTable->caps(Figure::TypeRook, n);
    const uint64 & k_caps = g_movesTable->caps(Figure::TypeKing, n);

    // pawn, knight or king
    if ( (p_caps & fig_mask_i & pw_mask) || (n_caps & fig_mask_i & kn_mask) || (k_caps & fig_mask_i & ki_mask) )
      attackers++;

    if ( attackers > 2 )
      return 0;

    // bishop
    uint64 amask = b_caps & b_mask & fig_mask_i;
    while ( amask )
    {
      int x = least_bit_number(amask);
      const Figure & bishop = board_.getFigure(x);
      THROW_IF( bishop.getType() != Figure::TypeBishop, "no bishop found, but mask isn't empty" );

      const uint64 & btw_msk = g_betweenMasks->between(n, x);
      if ( (btw_msk & mask_all_inv) != btw_msk )
        continue;

      attackers++;
      if ( attackers > 2 )
        return 0;
    }

    // rook
    amask = r_caps & r_mask & fig_mask_i;
    while ( amask )
    {
      int x = least_bit_number(amask);
      const Figure & rook = board_.getFigure(x);
      THROW_IF( rook.getType() != Figure::TypeRook, "no rook found, but mask isn't empty" );

      const uint64 & btw_msk = g_betweenMasks->between(n, x);
      if ( (btw_msk & mask_all_inv) != btw_msk )
        continue;

      attackers++;
      if ( attackers > 2 )
        return 0;
    }

    // queen
    amask = q_caps & q_mask & fig_mask_i;
    while ( amask )
    {
      int x = least_bit_number(amask);
      const Figure & queen = board_.getFigure(x);
      THROW_IF( queen.getType() != Figure::TypeQueen, "no rook found, but mask isn't empty" );

      const uint64 & btw_msk = g_betweenMasks->between(n, x);
      if ( (btw_msk & mask_all_inv) != btw_msk )
        continue;

      attackers++;
      if ( attackers > 2 )
        return 0;
    }
  }

  //if ( attackers > 2 )
  //  return 1;

  if ( attackers > 2 )
    return 0;

  return -1;
}

//////////////////////////////////////////////////////////////////////////
bool Player::isCheckDangerous(const Move & move, Figure::Type minimalType)
{
  const Field & field = board_.getField(move.from_);
  const Figure & fig = board_.getFigure(field.color(), field.index());

  THROW_IF( !fig || fig.getColor() != board_.getColor(), "invalid figure gives check");

  const Figure::Color ocolor = Figure::otherColor(board_.getColor());
  const Figure & oking = board_.getFigure(ocolor, Board::KingIndex);
  THROW_IF( !oking || oking.getType() != Figure::TypeKing, "no opponents king" );

  // if checking figure is attacked but not protected we skip this move
  bool attacked = board_.fastAttacked(ocolor, move.to_);
  bool guarded = board_.fastAttacked(board_.getColor(), move.to_, move.from_);
  if ( attacked && !guarded )
    return false;

  if ( move.rindex_ >= 0 || move.new_type_ > 0 || fig.getType() == Figure::TypePawn || fig.getType() == Figure::TypeQueen )
    return true;

  // opponent's figures
  const uint64 & omask = board_.fmgr().mask(ocolor);
  uint64 all_inv = board_.fmgr().mask(Figure::ColorWhite) | board_.fmgr().mask(Figure::ColorBlack);
  all_inv ^= (1ULL << move.from_);
  all_inv = ~all_inv;

  // check with attack
  uint64 amask = g_movesTable->caps(fig.getType(), move.to_) & omask ^ (1ULL << oking.where());
  if ( amask )
  {
    if ( fig.getType() == Figure::TypeKnight )
      return true;

    for ( ; amask; )
    {
      int n = least_bit_number(amask);
      const Field & afield = board_.getField(n);

      if ( !afield || afield.type() < minimalType )
        continue;

      Figure afig = fig;
      afig.go(move.to_);
      if ( g_figureDir->dir(afig, n) < 0 )
        continue;

      // can capture n-position
      const uint64 & btw_msk = g_betweenMasks->between(move.to_, n);
      if ( (btw_msk & all_inv) == btw_msk )
        return true;
    }
  }

  // too few available kings movements
  uint64 k_caps = g_movesTable->caps(Figure::TypeKing, oking.where());
  uint64 all_mask = ~all_inv | (1ULL << move.to_) ;
  k_caps &= all_mask;

  int num = 0;
  for ( ; k_caps; )
  {
    int n = least_bit_number(k_caps);
    if ( !board_.fastAttacked(board_.getColor(), n) )
      num++;
  }

  return num < 3;
}
