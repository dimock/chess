#pragma once

#include "Board.h"
#include "HashTable.h"
#include <time.h>

class SearchResult
{
public:

  SearchResult();

  /// statistic
  int nodesCount_;
  int totalNodes_;
  int forcedNodes_;
  int additionalNodes_;
  int nullMovesCount_;
  int plyMax_;
  clock_t dt_;

  /// result
  Move best_;
  Move pv_[MaxPly+1];
  int  depth_;

  ScoreType score_;
//  bool wminus_;
};

class CapsGenerator;
class MovesGenerator;
class EscapeGenerator;
class ChecksGenerator;
class CapsChecksGenerator;

struct PlyContext
{
  struct ExtData
  {
    ExtData() :
        recapture_count_(0),
        mat_threat_count_(0),
        mbe_count_(0),
        mat_threats_found_(0),
        mbe_threat_(false)
      /*singular_count_(0), checks_count_(0), doublechecks_count_(0),*/ 
    {
      mbe_move_.clear();
      recap_curr_.clear();
      recap_next_.clear();
    }

    void clear()
    {
      recapture_count_ = 0;
      mat_threat_count_ = 0;
      mbe_count_ = 0;
      mat_threats_found_ = 0;
      mbe_threat_ = false;
      mbe_move_.clear();
      recap_curr_.clear();
      recap_next_.clear();
      //singular_count_ = 0;
      //checks_count_ = 0;
      //doublechecks_count_ = 0;
    }

    void copy_counters(const ExtData & e)
    {
      recapture_count_ = e.recapture_count_;
      mat_threat_count_ = e.mat_threat_count_;
      mbe_count_ = e.mbe_count_;
      mat_threats_found_ = e.mat_threats_found_;
      //singular_count_ = e.singular_count_;
      //checks_count_ = e.checks_count_;
      //doublechecks_count_ = e.doublechecks_count_;
    }

    int recapture_count_;
    int mat_threat_count_;
    int mbe_count_;
    bool mbe_threat_;
    int mat_threats_found_;
    /*, singular_count_, checks_count_, doublechecks_count_*/

    Move recap_curr_, recap_next_;
    Move mbe_move_;
  };


  PlyContext() : threat_(0), null_move_threat_(0) {}

  Move killer_;
  Move pv_[MaxPly+1];
  uint16 threat_ : 1,
         null_move_threat_ : 1;

  void clearPV(int depth)
  {
    for (int i = 0; i < depth; ++i)
      pv_[i].clear();
  }

  void clear(int ply)
  {
    pv_[ply].clear();
    threat_ = 0;
    null_move_threat_ = 0;
    ext_data_.clear();
  }

  ExtData ext_data_;
};

class Player
{
  friend class CapsGenerator;
  friend class MovesGenerator;
  friend class EscapeGenerator;
  friend class ChecksGenerator;
  friend class CapsChecksGenerator;

public:

  // initialize global arrays, tables, masks, etc. write them to it's board_
  Player();
  ~Player();

  void setMemory(int mb);

  bool fromFEN(const char * fen);
  bool toFEN(char * fen) const;

  void saveHash(const char * fname) const;
  void loadHash(const char * fname);

  bool findMove(SearchResult & , std::ostream * out = 0);
  
  Board & getBoard()
  {
    return board_;
  }
  
  const Board & getBoard() const
  {
    return board_;
  }

  void pleaseStop()
  {
    stop_ = true;
  }
  
  void setTimeLimit(int ms)
  {
    timeLimitMS_ = ms;
  }

  void setMaxDepth(int d)
  {
    if ( d >= 1 && d <= 32 )
      depthMax_ = d;
  }

private:

  void printPV(Board & pv_board, SearchResult & sres, std::ostream * out);

  ScoreType nullMove(int depth, int ply, ScoreType alpha, ScoreType betta);
  ScoreType alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, bool null_move);
  ScoreType captures(int depth, int ply, ScoreType alpha, ScoreType betta, int delta, bool do_checks);

  void testTimer();

  volatile bool stop_;
  Board board_;
  int timeLimitMS_;
  int depthMax_;
  int depth_;
  int nodesCount_, totalNodes_, plyMax_;
  bool firstIter_;
  clock_t tstart_, tprev_;
  Move before_, best_;
  bool beforeFound_;

  // mat-balance at the root node (before starting calculation)
  int initial_material_balance_;

  PlyContext contexts_[MaxPly+1];
  MoveCmd * pv_moves_;

  // global data
  MoveCmd * g_moves;
  MovesTable * g_movesTable;
  FigureDir * g_figureDir;
  PawnMasks * g_pawnMasks_;
  BetweenMask * g_betweenMasks;
  DeltaPosCounter * g_deltaPosCounter;
  DistanceCounter * g_distanceCounter;

#ifdef USE_HASH_TABLE_GENERAL
  GeneralHashTable ghash_;
#endif

#ifdef USE_HASH_TABLE_CAPTURE
  CapturesHashTable chash_;
#endif

  bool use_pv_;

  //////////////////////////////////////////////////////////////////////////
  // return true if we have to return betta-1 to recalculate with full depth
  bool movement(int depth, int ply, ScoreType & alpha, ScoreType betta, Move & move, int & counter, bool null_move);

  void capture(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & cap, int & counter, bool do_checks);
  int collectHashMoves(int depth, int ply, bool null_move, ScoreType alpha, ScoreType betta, Move (&moves)[HashedMoves_Size]);
  int collectHashCaps(int ply, Figure::Type minimalType, Move (&caps)[HashedMoves_Size]);

  void assemblePV(const Move & move, bool checking, int ply)
  {
    if ( ply > depth_ || ply >= MaxPly )
      return;

    contexts_[ply].pv_[ply] = move;
    contexts_[ply].pv_[ply].checkFlag_ = checking;
    contexts_[ply].pv_[ply+1].clear();

    for (int i = ply+1; i < depth_; ++i)
    {
      contexts_[ply].pv_[i] = contexts_[ply+1].pv_[i];
      if ( !contexts_[ply].pv_[i] )
        break;
    }
  }

#ifdef USE_HASH_TABLE_GENERAL
  void updateGeneralHash(const Move & move, int depth, int ply, const ScoreType score, const ScoreType betta, const uint64 & hcode, Figure::Color color)
  {
    PackedMove pm = board_.pack(move);
    ghash_.push(hcode, score, depth, ply, board_.halfmovesCount()-1,
      color, score >= betta ? GeneralHashTable::Betta : GeneralHashTable::AlphaBetta, pm);
  }

  // we should return alpha if flag is Alpha, or betta if flag is Betta
  GeneralHashTable::Flag getGeneralHashFlag(int depth, int ply, ScoreType alpha, ScoreType betta)
  {
    GeneralHItem & hitem = ghash_[board_.hashCode()];
    if ( hitem.hcode_ != board_.hashCode() )
      return GeneralHashTable::None;

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

    THROW_IF(hscore > 32760 || hscore < -32760, "invalid value in hash");

    if ( (int)hitem.depth_ >= depth && ply > 0 )
    {
      if ( GeneralHashTable::Alpha == hitem.flag_ && hscore <= alpha )
      {
        THROW_IF( !stop_ && alpha < -32760, "invalid hscore" );
        return GeneralHashTable::Alpha;
      }

#ifdef RETURN_IF_BETTA
      if ( (GeneralHashTable::Betta == hitem.flag_ || GeneralHashTable::AlphaBetta == hitem.flag_) &&
            hscore >= betta && hitem.move_ )
      {
        Move hmove = board_.unpack(hitem.move_);
        if ( hmove )
        {
#ifndef NDEBUG
          Board board0 = board_;
#endif

          bool retBetta = hmove.rindex_ >= 0 || hmove.new_type_;
          bool checking = false;

          if ( !retBetta )
          {
            totalNodes_++;
            nodesCount_++;

            if ( board_.makeMove(hmove) )
            {
              checking = board_.getState() == Board::UnderCheck;
              retBetta = (board_.drawState() && 0 >= betta) || board_.repsCount() < 2;
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

          if ( retBetta )
          {
            assemblePV(hmove, checking, ply);
            return GeneralHashTable::Betta;
          }
        }
      }
#endif // RETURN_IF_BETTA
    }

    return GeneralHashTable::AlphaBetta;
  }
#endif // USE_HASH_TABLE_GENERAL

#ifdef USE_HASH_TABLE_CAPTURE
  void updateCaptureHash(int depth, int ply, const Move & move, const ScoreType score, const ScoreType betta, const uint64 & hcode, Figure::Color color)
  {
    PackedMove pm = board_.pack(move);
    chash_.push(hcode, score, color, score >= betta ? CapturesHashTable::Betta : CapturesHashTable::AlphaBetta, depth, ply, board_.halfmovesCount(), pm);
  }
#endif // USE_HASH_TABLE_CAPTURE

#ifdef VERIFY_ESCAPE_GENERATOR
  void verifyEscapeGen(int depth, int ply, ScoreType alpha, ScoreType betta);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  void verifyChecksGenerator(int depth, int ply, ScoreType alpha, ScoreType betta, Figure::Type minimalType);
#endif

#ifdef VERIFY_CAPS_GENERATOR
  void verifyCapsGenerator(int ply, ScoreType alpha, ScoreType betta, int delta);
#endif

#ifdef RECAPTURE_EXTENSION
  bool recapture(int ply)
  {
    if ( board_.halfmovesCount() < 1 )
      return false;

    const MoveCmd & move = board_.getMoveRev(0);
    if ( move.rindex_ < 0 )
      return false;

    // don't extend pawn's recapture
    //if ( move.eaten_type_ == Figure::TypePawn && board_.getField(move.to_).type() == Figure::TypePawn )
    //  return false;

    // this is not a recapture (?) but capture of strong figure by weaker one.
    // it usually means that previous move was stupid )
    if ( board_.halfmovesCount() > 1 )
    {
      const MoveCmd & prev = board_.getMoveRev(-1);
      if ( prev.rindex_ < 0 &&
           !typeLEQ( (Figure::Type)board_.getField(move.to_).type(), (Figure::Type)move.eaten_type_) )
      {
        //char fen[256];
        //board_.toFEN(fen);
        return false;
      }
    }

    //if ( contexts_[ply].ext_data_.recapture_count_ >= RecaptureExtension_Limit )
    //  return false;

    Move next;
    int score_see = board_.see(initial_material_balance_, next);
    if ( score_see >= 0 )
    {
      contexts_[ply].ext_data_.recap_curr_ = move;
      contexts_[ply].ext_data_.recap_next_ = next;
      //contexts_[ply].ext_data_.recapture_count_++;
      return true;
    }
    else if ( ply > 1 )
    {
      const MoveCmd & prev = board_.getMoveRev(-1);
      if ( contexts_[ply-1].ext_data_.recap_curr_ == prev && contexts_[ply-1].ext_data_.recap_next_ == move )
        return true;
    }

    return false;
  }
#endif //RECAPTURE_EXTENSION

  inline bool pawnBeforePromotion(const Move & move) const
  {
    const Figure & fig = board_.getFigure(move.to_);
    if ( fig.getType() == Figure::TypePawn )
    {
      // before promotion
      int8 y = move.to_ >> 3;
      return 1 == y || 6 == y;
    }
    return false;
  }

  // is given movement caused by previous. this mean that if we don't do this move we loose
  // we actually check if moved figure was attacked by previously moved one or from direction it was moved from
  bool isRealThreat(const Move & move);

  // returns number of ply to extent
  int need_extension(int counter);

  // we only do dangerous check after horizont
  bool isCheckDangerous(const Move &, Figure::Type minimalType);
};
