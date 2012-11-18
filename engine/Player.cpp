/*************************************************************
  Player.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


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
  analyze_mode_(false),
  counter_(0),
  numOfMoves_(0),
  out_(0),
  callback_(0),
  givetime_(0),
  posted_command_(Posted_NONE),
  stop_(false),
  timeLimitMS_(0),
  tstart_(0),
  nodesCount_(0),
  totalNodes_(0),
  depthMax_(2),
  depth_(0),
  plyMax_(0),
  initial_material_balance_(0),
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
  posted_fen_[0] = 0;

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

//////////////////////////////////////////////////////////////////////////
void Player::postUndo()
{
  if ( posted_command_ )
    return;

  posted_command_ = Posted_UNDO;
}

void Player::postNew()
{
  if ( posted_command_ )
    return;

  posted_command_ = Posted_NEW;
}

void Player::postFEN(const char * fen)
{
  if ( fen )
    strncpy(posted_fen_, fen, sizeof(posted_fen_));
}

void Player::postStatus()
{
  if ( posted_command_ )
    return;

  posted_command_ = Posted_UPDATE;
}

void Player::postHint()
{
  if ( posted_command_ )
    return;

  posted_command_ = Posted_HINT;
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
  ghash_.resize(hsize-1);
  use_pv_ = false;
#endif

#ifdef USE_HASH_TABLE_CAPTURE
  chash_.resize(hsize+1);
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

  MovesGenerator::clear_history();

#ifdef USE_HASH_TABLE_GENERAL
  ghash_.clear();
#endif

#ifdef USE_HASH_TABLE_CAPTURE
  chash_.clear();
#endif

  return board_.fromFEN(fen);
}

bool Player::toFEN(char * fen) const
{
  return board_.toFEN(fen);
}

void Player::printPV(Board & pv_board, SearchResult & sres)
{
  if ( !out_ )
    return;

  *out_ << sres.depth_ << " " << sres.score_ << " " << (int)sres.dt_ << " " << sres.totalNodes_;
  for (int i = 0; i < sres.depth_ && sres.pv_[i]; ++i)
  {
    *out_ << " ";

    Move pv = sres.pv_[i];
    uint8 captured = pv.capture_;
    pv.clearFlags();
    pv.capture_ = captured;

    if (!pv_board.possibleMove(pv) )
      break;

    char str[64];
    if ( !printSAN(pv_board, pv, str) )
      break;

    THROW_IF( !pv_board.validateMove(pv), "move is invalid but it is not detected by printSAN()");

    pv_board.makeMove(pv);

    *out_ << str;
  }
  *out_ << std::endl;
}

bool Player::findMove(SearchResult & sres, std::ostream * out)
{
  out_ = out;
  bool ok = false;
  for ( ;; )
  {
    ok = search(sres, out);

    if ( !posted_command_ )
      break;

    if ( posted_command_ == Posted_NEW )
      fromFEN(0);
    else if ( posted_command_ == Posted_FEN && posted_fen_[0] )
    {
      fromFEN(posted_fen_);
      posted_fen_[0] = 0;
    }
    else if ( posted_command_ == Posted_UNDO && board_.halfmovesCount() > 0 )
      board_.unmakeMove();

    posted_command_ = Posted_NONE;
  }
  out_ = 0;
  return ok;
}

bool Player::search(SearchResult & sres, std::ostream * out)
{
  sres = SearchResult();

  stop_ = false;
  totalNodes_ = 0;
  firstIter_ = true;
  tprev_ = tstart_ = clock();
  numOfMoves_ = 0;
  
  // for recapture extension
  initial_material_balance_ = board_.fmgr().weight();

  MovesGenerator::clear_history();

  before_.clear();
  contexts_[0].killer_.clear();

  contexts_[0].clearPV(depthMax_);

  {
    ScoreType alpha = -std::numeric_limits<ScoreType>::max();
    ScoreType betta = +std::numeric_limits<ScoreType>::max();
    int counter = 0;
    MovesGenerator mg(board_);
    for ( ;; )
    {
      const Move & move = mg.move();
      if ( !move )
        break;
      if ( board_.validateMove(move) )
        numOfMoves_++;
    }
  }


  for (depth_ = 1; !stop_ && depth_ <= depthMax_; ++depth_)
  {
    pv_board_ = board_;
    pv_board_.set_moves(pv_moves_);

    best_.clear();
    beforeFound_ = false;
    nodesCount_ = 0;
	  plyMax_ = 0;
    counter_ = 0;

    ScoreType alpha = -std::numeric_limits<ScoreType>::max();
    ScoreType betta = +std::numeric_limits<ScoreType>::max();

    ScoreType score = alphaBetta(depth_, 0, alpha, betta, false);

    if ( best_ && (!stop_ || (stop_ && beforeFound_) || (2 == depth_)) )
    {
      if ( stop_ && depth_ > 2 &&
        (abs(score-sres.score_) >= Figure::figureWeight_[Figure::TypePawn]/2 || best_ != sres.best_ && abs(score-sres.score_) >= 5)&&
        givetime_ )
      {
        int t_add = (givetime_)();
        if ( t_add > 0 )
        {
          stop_ = false;
          timeLimitMS_ += t_add;
          if ( counter_ < numOfMoves_ )
            depth_--;
          continue;
        }
      }

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

      printPV(pv_board_, sres);
    }

    firstIter_ = false;

    if ( !best_ || (score >= Figure::WeightMat-MaxPly || score <= MaxPly-Figure::WeightMat) && !analyze_mode_ )
      break;
  }

  sres.totalNodes_ = totalNodes_;

  return sres.best_;
}

void Player::setCallback(PLAYER_CALLBACK cbk)
{
  callback_ = cbk;
}

void Player::setGiveTimeCbk(GIVE_MORE_TIME gvt)
{
  givetime_ = gvt;
}

void Player::setAnalyzeMode(bool analyze)
{
  analyze_mode_ = analyze;
}

void Player::testTimer()
{
  int t = clock();
  stop_ = stop_ || ( (t - tstart_) > timeLimitMS_);

  if ( callback_ )
    (callback_)();

  if ( !posted_command_ )
    return;

  if ( posted_command_ == Posted_NEW || posted_command_ == Posted_UNDO || posted_command_ == Posted_FEN )
  {
    stop_ = true;
    return;
  }

  processPosted(t - tstart_);
}

void Player::testInput()
{
  if ( callback_ )
    (callback_)();

  if ( !posted_command_ )
    return;

  if ( posted_command_ == Posted_NEW || posted_command_ == Posted_UNDO || posted_command_ == Posted_FEN )
  {
    stop_ = true;
    return;
  }

  processPosted( clock() - tstart_ );
}

void Player::processPosted(int t)
{
  if ( !posted_command_ )
    return;

  if ( posted_command_ == Posted_UPDATE )
  {
    if ( out_ && depth_ > 0 )
    {
      posted_command_ = Posted_NONE;

      char outstr[1024];
      sprintf(outstr, "stat01: %d %d %d %d %d", t/10, totalNodes_, depth_-1, numOfMoves_-counter_, numOfMoves_);

      Move mv = best_;
      if ( !mv )
        mv = before_;
      uint8 captured = mv.capture_;
      mv.clearFlags();
      mv.capture_ = captured;

      char str[64];
      if ( mv && pv_board_.validateMove(mv) && printSAN(pv_board_, mv, str) )
      {
        strcat(outstr, " ");
        strcat(outstr, str);
      }

      *out_ << outstr << std::endl;
    }
  }
  else if ( posted_command_ == Posted_HINT )
  {
    posted_command_ = Posted_NONE;
  }
}

//////////////////////////////////////////////////////////////////////////
ScoreType Player::nullMove(int depth, int ply, ScoreType alpha, ScoreType betta)
{
  if (   board_.underCheck() ||
        !board_.allowNullMove() ||
        (ply < 1) ||
        (ply >= MaxPly-1) ||
        (depth < NullMove_DepthMin+1) ||
        (depth_ < NullMove_DepthStart) ||
        betta >= Figure::WeightMat+MaxPly ||
        alpha <= -Figure::WeightMat-MaxPly  )
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

  Move pv;
  pv.clear();

  // clear context for current ply
  // copy extensions counters from prev.ply context
  if ( ply < MaxPly-1 )
  {
    if ( use_pv_ )
    {
      // we use PV only up to max-depth
      if ( ply < depthMax_ )
      {
        pv = contexts_[0].pv_[ply];
        pv.checkVerified_ = 0;

        if ( !board_.possibleMove(pv) )
          pv.clear();
      }
    }

    contexts_[ply+1].killer_.clear();
    contexts_[ply].clear(ply);

    if ( ply > 0 )
      contexts_[ply].ext_data_.copy_counters(contexts_[ply-1].ext_data_);
  }

#ifdef VERIFY_ESCAPE_GENERATOR
  if ( board_.underCheck() )
    verifyEscapeGen();
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  if ( !board_.underCheck() )
    verifyChecksGenerator(Figure::TypeKing);
#endif

#ifdef VERIFY_CAPS_GENERATOR
  if ( !board_.underCheck() )
    verifyCapsGenerator();
#endif

  // first of all try null-move
#ifdef USE_NULL_MOVE
  if ( !null_move 
#ifndef MAT_THREAT_EXTENSION
    && betta == alpha+1
#endif
    )
  {
    ScoreType nullScore = nullMove(depth, ply, alpha, betta);

    if ( nullScore >= betta
#ifdef MAT_THREAT_EXTENSION
      && betta == alpha+1
#endif
      )
    {
      if ( board_.shortNullMoveReduction() )
        depth--;
      else if ( board_.limitedNullMoveReduction() )
      {
        depth -= 4;
        null_move = true;
      }
      else
      {
        depth = nullMove_depth(depth);
        null_move = true;
      }

      if ( depth < NullMove_DepthMin )
        depth = NullMove_DepthMin;
    }
    // mat threat extension
#ifdef MAT_THREAT_EXTENSION
    else if ( betta > alpha+1 && ply > 1 && contexts_[ply].ext_data_.mat_threats_found_ > 1 &&
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
  if ( !board_.underCheck() &&
       alpha > -Figure::WeightMat+MaxPly &&
       alpha < Figure::WeightMat-MaxPly &&
       depth == 1 && ply > 1 && !board_.isWinnerLoser() )
  {
    ScoreType score = board_.evaluate();
    int delta = (int)alpha - (int)score - (int)Figure::positionGain_;
    if ( delta > 0 )
      return captures(1, ply, alpha, betta, delta);
  }
#endif

  // collect all hashed moves together
  Move hmoves[HashedMoves_Size];
  int hmovesN = 0;
  
  if ( !use_pv_ )
    hmovesN = collectHashMoves(depth, ply, null_move, alpha, betta, hmoves);
  else if ( pv )
  {
    hmoves[0] = pv;
    hmoves[1].clear();
    hmovesN = 1;
  }
  else
    hmoves[0].clear();

  if ( board_.underCheck() )
  {
    EscapeGenerator eg(hmoves[0], board_);

    // additional check extension
    depth += extend_check(depth, ply, eg, alpha, betta);

    for ( ; !stop_ && alpha < betta; )
    {
      Move & move = eg.escape();
      if ( !move )
        break;

      checkForStop();

      if ( stop_ )
        break;

      movement(depth, ply, alpha, betta, move, counter, null_move);

      if ( ply == 0 )
        counter_ = counter;
    }
  }
  else
  {
    Move killer = contexts_[ply].killer_;

#ifdef VERIFY_FAST_GENERATOR
    verifyFastGenerator(hmoves[0], killer);
#endif

    // first of all try moves, collected from hash
    for (Move * m = hmoves; !stop_ && alpha < betta && *m; ++m)
    {
      // if movement return true we were reduced threat move and need to recalculate it with full depth
      if ( movement(depth, ply, alpha, betta, *m, counter, null_move) )
      {
        THROW_IF( !stop_ && (betta < -32760 || betta > 32760), "invalid score" );
        return betta - 1;
      }

      if ( ply == 0 )
        counter_ = counter;

      checkForStop();
    }

    if ( stop_ || alpha >= betta )
    {
      THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
      return alpha;
    }

    MovesGenerator mg(board_, killer);

    for ( ; !stop_ && alpha < betta ; )
    {
      Move & move = mg.move();
      if ( !move )
        break;

      if ( find_move(hmoves, hmovesN, move) )
        continue;

      checkForStop();

      if ( stop_ )
        break;

      if ( movement(depth, ply, alpha, betta, move, counter, null_move) )
      {
        THROW_IF( !stop_ && (betta < -32760 || betta > 32760), "invalid score" );
        return betta - 1;
      }

      if ( ply == 0 )
        counter_ = counter;
    }
  }

  if ( stop_ )
    return alpha;

  if ( 0 == counter )
  {
    board_.setNoMoves();
    ScoreType s = board_.evaluate();
    if ( board_.matState() )
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

  if ( 0 == ply && counter == 1 && !analyze_mode_ && !stop_ )
  {
    beforeFound_ = true;
    stop_ = true;
  }

  // we haven't found best move
#ifdef USE_HASH_TABLE_GENERAL
  if ( alpha == savedAlpha && !stop_ )
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
  if ( !board_.validateMove(move) )
    return false;

  totalNodes_++;
  nodesCount_++;

#ifndef NDEBUG
  Board board0 = board_;
#endif

  uint64 hcode = board_.hashCode();
  int depth0 = depth;
  Figure::Color color = board_.getColor();
  bool check_esc = board_.underCheck();
  contexts_[ply].threat_ = false;

  // previous move was reduced
  bool reduced = board_.halfmovesCount() > 0 && board_.getMoveRev(0).reduced_;
  bool was_winnerloser = board_.isWinnerLoser();
  int initial_balance = board_.fmgr().weight();

  board_.makeMove(move);
  {
    MoveCmd & mv_cmd = board_.getMoveRev(0);
    mv_cmd.extended_ = false;
    History & hist = MovesGenerator::history(move.from_, move.to_);

    bool haveCheck = board_.underCheck();
    move.checkFlag_ = haveCheck;
    int ext_ply = do_extension(depth, ply, alpha, betta, was_winnerloser, initial_balance);
    if ( ext_ply )
    {
      mv_cmd.extended_ = true;
      depth += ext_ply;
    }

    counter++;
    ScoreType score = alpha;
    if ( board_.drawState() )
      score = 0;
    else if ( depth <= 1 )
    {
      score = -board_.evaluate();
      ScoreType sc0 = score;
      int delta = (int)score - (int)betta - (int)Figure::positionGain_;
      if ( haveCheck || score > alpha )
      {
        ScoreType betta1 = score < betta && !haveCheck ? score : betta;
        score = -captures(depth-1, ply+1, -betta1, -alpha, delta);
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
              board_.canBeReduced(move) &&
              !move.threat_ &&
               mv_cmd.castle_ == 0 &&
              !haveCheck &&
              !check_esc &&
              !null_move &&
              !mv_cmd.extended_ &&
              alpha > -Figure::WeightMat+MaxPly &&
              ((hist.good_count_<<4) <= hist.bad_count_) )
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

        if ( !move.capture_ && !move.new_type_ )
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
#if ((defined USE_HASH_TABLE_GENERAL) && (defined USE_THREAT_MOVE))
      else if ( contexts_[ply].threat_ )
      {
        ghash_[hcode].tmove_ = board_.pack(move);
      }
#endif
      if ( !move.capture_ && !move.new_type_ )
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
  if ( !stop_ && contexts_[ply].null_move_threat_ && alpha >= betta && ply > 0 && isRealThreat(move) )
  {
    contexts_[ply-1].threat_ = true;
    if ( reduced )
      return true;

#ifdef USE_HASH_TABLE_GENERAL
    else if ( ghash_[board_.hashCode()].hcode_ == board_.hashCode() )
      ghash_[board_.hashCode()].threat_ = 1;
#endif
  }

  return false;
}


//////////////////////////////////////////////////////////////////////////
ScoreType Player::captures(int depth, int ply, ScoreType alpha, ScoreType betta, int delta)
{
	if ( ply > plyMax_ )
		plyMax_ = ply;

  if ( stop_ || ply >= MaxPly )
  {
    if ( alpha < -Figure::WeightMat-MaxPly )
      return board_.evaluate();
    else
      return alpha;
  }
  else if ( alpha >= Figure::WeightMat-ply )
    return alpha;

  if ( ply < MaxPly )
    contexts_[ply+1].killer_.clear();

#ifdef VERIFY_CAPS_GENERATOR
  if ( !board_.underCheck() )
    verifyCapsGenerator();
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  if ( !board_.underCheck() )
  {
    verifyChecksGenerator(delta2type(delta));
  }
#endif

#ifdef VERIFY_ESCAPE_GENERATOR
  if ( board_.underCheck() )
    verifyEscapeGen();
#endif

  int counter = 0;
  ScoreType saveAlpha = alpha;

  Figure::Type minimalType = board_.isWinnerLoser() ? Figure::TypePawn : delta2type(delta);

  Move hcap;

#ifdef USE_HASH_TABLE_CAPTURE
  CapturesHashTable::Flag flag = testCaptureHashItem(depth, ply, alpha, betta, minimalType, hcap);

#ifdef RETURN_IF_ALPHA_BETTA_CAPTURES
  if ( flag == CapturesHashTable::Alpha )
    return alpha;
  else if ( flag == CapturesHashTable::Betta )
    return betta;
#endif

#else // !USE_HASH_TABLE_CAPTURE
  hcap.clear();
#endif //USE_HASH_TABLE_CAPTURE

  if ( board_.underCheck() )
  {
#ifdef USE_HASH_TABLE_CAPTURE
    EscapeGenerator eg(hcap, board_);
#else
    EscapeGenerator eg(board_, 0, ply, *this, alpha, betta, counter);
#endif

    depth += extend_check(depth, ply, eg, alpha, betta);

    for ( ; !stop_ && alpha < betta ; )
    {
      const Move & move = eg.escape();
      if ( !move )
        break;

      checkForStop();

      if ( stop_ )
        break;

      THROW_IF( !board_.possibleMove(move), "move validation failed" );

      capture(depth, ply, alpha, betta, move, counter);
    }

    if ( stop_ )
      return alpha;

    if ( !counter )
    {
      board_.setNoMoves();
      ScoreType s = board_.evaluate();
      if ( board_.matState() )
        s += ply;

#ifdef USE_HASH_TABLE_CAPTURE
      CapturesHashTable::Flag flag;
      if ( s <= saveAlpha )
        flag = CapturesHashTable::Alpha;
      else if ( s >= betta )
        flag = CapturesHashTable::Betta;
      else
        flag = CapturesHashTable::AlphaBetta;
      chash_.push(board_.hashCode(), s, board_.getColor(), flag, depth, ply, PackedMove());
#endif

      THROW_IF( !stop_ && (s < -32760 || s > 32760), "invalid score" );
      return s;
    }
  }
  else
  {
    if ( hcap )
      capture(depth, ply, alpha, betta, hcap, counter);

    if ( stop_ || alpha >= betta )
      return alpha;

    // generate only suitable captures
    CapsGenerator cg(hcap, board_, minimalType);
    for ( ; !stop_ && alpha < betta; )
    {
      const Move & cap = cg.capture();
      if ( !cap )
        break;

      checkForStop();

      if ( stop_ )
        break;

      THROW_IF( !board_.possibleMove(cap), "move validation failed" );

      capture(depth, ply, alpha, betta, cap, counter);
    }

#ifdef PERFORM_CHECKS_IN_CAPTURES
    // generate check only on 1st iteration under horizon
    if ( depth >= 0 && !stop_ && alpha < betta )
    {
      ChecksGenerator ckg(hcap, board_, minimalType);

      for ( ; !stop_ && alpha < betta ; )
      {
        Move & check = ckg.check();
        if ( !check )
          break;

        checkForStop();

        if ( stop_ )
          break;

        THROW_IF( !board_.possibleMove(check), "move validation failed" );

        capture(depth, ply, alpha, betta, check, counter);
      }
    }
#endif // PERFORM_CHECKS_IN_CAPTURES

  }

#ifdef USE_HASH_TABLE_CAPTURE
  if ( alpha == saveAlpha && !stop_ )
  {
    chash_.push(board_.hashCode(), alpha, board_.getColor(), CapturesHashTable::Alpha, depth, ply, PackedMove());
  }
#endif

  THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
  return alpha;
}

//////////////////////////////////////////////////////////////////////////
void Player::capture(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & cap, int & counter)
{
  if ( !board_.validateMove(cap) )
    return;

  totalNodes_++;
  nodesCount_++;

#ifndef NDEBUG
  Board board0 = board_;
#endif

  uint64 hcode = board_.hashCode();
  Figure::Color color = board_.getColor();

  board_.makeMove(cap);
  {
    History & hist = MovesGenerator::history(cap.from_, cap.to_);

    bool haveCheck = board_.underCheck();
    if ( haveCheck )
      depth++;

    counter++;
    ScoreType s = alpha;
    if ( board_.drawState() )
      s = 0;
    else
    {
      s = -board_.evaluate();
      int delta = s - betta - Figure::positionGain_;
      bool b = s <= alpha;
      if ( haveCheck || (s > alpha && (delta <= Figure::figureWeight_[Figure::TypeQueen] || depth >= 0)) )
      {
        ScoreType betta1 = s < betta && !haveCheck ? s : betta;
        s = -captures(depth-1, ply+1, -betta1, -alpha, delta);
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

      if ( !cap.capture_ && !cap.new_type_ )
      {
        hist.score_++;
        if ( hist.score_ > History::history_max_ )
          History::history_max_ = hist.score_;
      }
    }

    if ( !cap.capture_ && !cap.new_type_ )
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

#ifdef USE_HASH_TABLE_GENERAL
  {
    GeneralHItem & hgitem = ghash_[board_.hashCode()];
    if ( hgitem.move_ && hgitem.hcode_ == board_.hashCode() )
    {
      board_.unpack(hgitem.move_, pv);
    }

#if (defined USE_HASH_TABLE_ADV) && (defined USE_HASH_TABLE_CAPTURE)
    // if we haven't found pv in general hash, lets try captures hash
    if ( !pv )
    {
      CaptureHItem & hcitem = chash_[board_.hashCode()];
      if ( hcitem.move_ && hcitem.hcode_ == board_.hashCode() )
      {
        board_.unpack(hcitem.move_, pv);
      }
    }
#endif // USE_HASH_TABLE_ADV
  }

#ifdef USE_IID
  // internal iterative deeping
  // if we don't have move in hashes, let's try to calculate to reduced depth to find some appropriate move
  if ( ply > 0 && depth > 1 && !null_move && !pv && ghash_[board_.hashCode()].flag_ != GeneralHashTable::Alpha && alpha > -Figure::WeightMat+MaxPly )
  {
    depth -= 3;
    if ( depth < 1 )
      depth = 1;

    ScoreType score = alphaBetta(depth, ply+1, alpha, alpha+1, false);
    GeneralHItem & hitem = ghash_[board_.hashCode()];
    if ( hitem.move_ && hitem.hcode_ == board_.hashCode() )
    {
      board_.unpack(hitem.move_, pv);
    }

#ifdef USE_HASH_TABLE_CAPTURE
    else
    {
      CaptureHItem & citem = chash_[board_.hashCode()];
      if ( citem.move_ && citem.hcode_ == board_.hashCode() )
      {
        board_.unpack(citem.move_, pv);
      }
    }
#endif
  }
#endif

#endif USE_HASH_TABLE_GENERAL

  // write 1st move from hash (or from PV)
  if ( pv )
    moves[num++] = pv;

  if ( board_.underCheck() )
  {
    moves[num].clear();
    return num;
  }


#ifdef USE_HASH_TABLE_GENERAL
  GeneralHItem & hitem = ghash_[board_.hashCode()];
  if ( hitem.hcode_ == board_.hashCode() )
  {
#ifdef USE_HASH_MOVE_EX
  // extra moves from hash
    for (int i = 0; i < 2; ++i)
    {
      Move hmove_ex;
      if ( !board_.unpack(hitem.move_ex_[i], hmove_ex) || find_move(moves, num, hmove_ex) )
        continue;

      moves[num++] = hmove_ex;
    }
#endif

#ifdef USE_THREAT_MOVE
    // threat move, if we have one
    Move htmove;
    if ( board_.unpack(hitem.tmove_, htmove) && !find_move(moves, num, htmove) )
    {
      htmove.threat_ = 1;
      moves[num++] = htmove;
    }
#endif
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
    if ( !board_.unpack(chitem.move_, hmove) )
      hmove.clear();
  }

#if ((defined USE_GENERAL_HASH_IN_CAPS) && (defined USE_HASH_TABLE_GENERAL))
  if ( !hmove )
  {
    GeneralHItem & ghitem = ghash_[board_.hashCode()];
    if ( ghitem.move_ && ghitem.hcode_ == board_.hashCode() )
    {
      if ( !board_.unpack(ghitem.move_, hmove) || !hmove.capture_ || board_.getField(hmove.to_).type() < minimalType )
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
  if ( move.capture_ >= 0 || move.checkFlag_ )
    return false;

  const MoveCmd & prev = board_.getMoveRev(0);
  Figure::Color ocolor = Figure::otherColor(board_.getColor());

  const Field & pfield = board_.getField(prev.to_);
  THROW_IF( !pfield || pfield.color() != ocolor, "no figure of required color on the field it was move to while detecting threat" );

  // don't need forbid reduction of captures, checks, promotions and pawn's attack because we've already done it
  if ( prev.capture_ || prev.new_type_ > 0 || prev.checkingNum_ > 0 || board_.isDangerPawn(prev) /*|| pawnBeforePromotion(prev)*/)
  {
    return false;
  }

  const Field & cfield = board_.getField(move.from_);
  THROW_IF( !cfield || cfield.color() != board_.getColor(), "no figure of required color in while detecting threat" );

  // we have to put figure under attack
  if ( board_.ptAttackedBy(move.to_, prev.to_) 
#ifdef ONLY_LEQ_THREAT
    && typeLEQ(pfig.getType(), cfig.getType())
#endif
    )
    return true;

  // put our figure under attack
  int tindex = board_.getAttackedFrom(ocolor, move.to_, prev.from_);
  if ( tindex >= 0 )
  {
#ifdef ONLY_LEQ_THREAT
    const Figure & afig = board_.getFigure(ocolor, tindex);
    if ( typeLEQ(afig.getType(), cfig.getType()))
#endif
      return true;
  }

  // prev move was attack, and we should escape from it
  if ( board_.ptAttackedBy(move.from_, prev.to_) 
#ifdef ONLY_LEQ_THREAT
    && typeLEQ(cfig.getType(), pfig.getType())
#endif
    )
    return true;

  // our figure was attacked from direction, opened by prev movement
  int findex = board_.getAttackedFrom(ocolor, move.from_, prev.from_);
  if ( findex >= 0 )
  {
#ifdef ONLY_LEQ_THREAT
    const Figure & afig = board_.getFigure(ocolor, findex);
    if ( typeLEQ(cfig.getType(), afig.getType()))
#endif
      return true;
  }

  return false;
}
//////////////////////////////////////////////////////////////////////////
int Player::do_extension(int depth, int ply, ScoreType alpha, ScoreType betta, bool was_winnerloser, int initial_balance)
{
  if ( depth <= 0 || alpha >= Figure::WeightMat-MaxPly || board_.halfmovesCount() < 1 )
    return 0;

  const MoveCmd & move = board_.getMoveRev(0);

  if ( board_.underCheck() )
      return 1;

  // other extensions will be done only in PV
  if ( betta == alpha+1 )
    return 0;

  // we look from side, that moved recently. we should adjust sing of initial mat-balance
  initial_balance = initial_material_balance_;
  if ( board_.getColor() )
    initial_balance = -initial_balance;

#ifdef EXTEND_PROMOTION
  if ( move.new_type_ == Figure::TypeQueen && move.recapture_ )
  {
    return 1;
  }
#endif

#ifdef EXTEND_PASSED_PAWN
   if ( board_.pawnPassed(move) )
   {
     Move next;
     int rdepth = 0;
     int score_see = board_.see(initial_balance, next, rdepth);
     if ( score_see >= 0 )
       return 1;
   }
#endif

#ifdef RECAPTURE_EXTENSION
  if ( alpha < Figure::figureWeight_[Figure::TypeKnight] && recapture(ply, depth, initial_balance) )
      return 1;
#endif

  // go to winner-loser state. extend to be sure that loser has at least 1 movement
#ifdef EXTEND_WINNER_LOSER
  if (  betta > alpha+1 && depth <= 1 && !was_winnerloser && board_.isWinnerLoser() )
    return 1;
#endif

  return 0;
}

// additional check extension
int Player::extend_check(int depth, int ply, EscapeGenerator & eg, ScoreType alpha, ScoreType betta)
{
  THROW_IF( !board_.underCheck(), "try to extend check but there is no one" );

  if ( board_.halfmovesCount() < 1 || eg.count() < 1 )
  {
    return 0;
  }

  // one reply - always extend
  if ( eg.count() == 1 )
    return 1;

  const Move & first = eg[0];
  MoveCmd & move = board_.getMoveRev(0);

  // double check and first move isn't capture of checking figure
  if ( move.checkingNum_ == 2 && first.to_ != move.checking_[0] && first.to_ != move.checking_[1] )
    return 1;

  return 0;
}
//#include <fstream>

#ifdef RECAPTURE_EXTENSION
bool Player::recapture(int ply, int depth, int initial_balance)
{
  if ( board_.halfmovesCount() < 1 )
    return false;

  const MoveCmd & move = board_.getMoveRev(0);
  if ( !move.capture_ )
    return false;

  // this is not a recapture (?) but capture of strong figure by weaker one.
  // it usually means that previous move was stupid )
  if ( board_.halfmovesCount() > 1 )
  {
    const MoveCmd & prev = board_.getMoveRev(-1);
    if ( !prev.capture_&& !typeLEQ( (Figure::Type)board_.getField(move.to_).type(), (Figure::Type)move.eaten_type_) )
    {
      //char fen[256];
      //board_.toFEN(fen);
      return false;
    }
  }

  if ( move.recapture_ ) // good recapture
    return true;

  return false;
}
#endif //RECAPTURE_EXTENSION
