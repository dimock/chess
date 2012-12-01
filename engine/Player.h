#pragma once


/*************************************************************
  Player.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
  Player class implements the main search routine (alpha-betta algorithm)
 *************************************************************/

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

// implementation isn't perfect )) have to rewrite
typedef void (* PLAYER_CALLBACK)();
typedef int (*GIVE_MORE_TIME)();

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


  // analyze move support
  enum PostedCommand { Posted_NONE, Posted_UNDO, Posted_UPDATE, Posted_HINT, Posted_NEW, Posted_FEN };
  PostedCommand posted_command_;
  char posted_fen_[256];
  PLAYER_CALLBACK callback_;
  GIVE_MORE_TIME givetime_;
  std::ostream * out_;
  int counter_, numOfMoves_;
  Board pv_board_;
  bool analyze_mode_;

public:

  // initialize global arrays, tables, masks, etc. write them to it's board_
  Player();
  ~Player();

  void setMemory(int mb);
  void setCallback(PLAYER_CALLBACK);
  void setGiveTimeCbk(GIVE_MORE_TIME);
  void setAnalyzeMode(bool);

  void postUndo();
  void postNew();
  void postFEN(const char *);
  void postStatus();
  void postHint();

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

  void checkForStop()
  {
    if ( totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
    {
      if ( timeLimitMS_ > 0 )
        testTimer();
      else
        testInput();
    }
  }

  void testInput();
  void processPosted(int t);
  void printPV(Board & pv_board, SearchResult & sres);

  // start point of search algorithm
  bool search(SearchResult & , std::ostream * out = 0);

  // core of search algorithm
  ScoreType alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, bool null_move, int singularCount);

  // only tactical moves (captures + winning checks) after horizon
  ScoreType captures(int depth, int ply, ScoreType alpha, ScoreType betta, int delta);

  // null-move heuristic
  ScoreType nullMove(int depth, int ply, ScoreType alpha, ScoreType betta);

  // time control
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

  //////////////////////////////////////////////////////////////////////////
  // return true if we have to return betta-1 to recalculate with full depth
  bool movement(int depth, int ply, ScoreType & alpha, ScoreType betta, Move & move, int & counter, bool null_move, int singularCount);

  void capture(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & cap, int & counter);
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
    if ( board_.repsCount() > 2 )
      return;

    PackedMove pm = board_.pack(move);
    ghash_.push(hcode, score, depth, ply, board_.halfmovesCount()-1,
      color, score >= betta ? GeneralHashTable::Betta : GeneralHashTable::AlphaBetta, pm);
  }

  // we should return alpha if flag is Alpha, or betta if flag is Betta
  GeneralHashTable::Flag getGeneralHashFlag(int depth, int ply, ScoreType alpha, ScoreType betta)
  {
    if ( betta > alpha+1 )
      return GeneralHashTable::AlphaBetta;

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
        Move hmove;
        if ( board_.unpack(hitem.move_, hmove) )
        {
#ifndef NDEBUG
          Board board0 = board_;
#endif

          bool retBetta = hmove.capture_ || hmove.new_type_;
          bool checking = false;

          if ( !retBetta )
          {
            int reps = board_.repsCount();
            for (int i = 1; reps < 2 && i < board_.halfmovesCount()-1; i += 2)
            {
              const MoveCmd & mv = board_.getMoveRev(-i);
              if ( mv.irreversible_ )
                break;

              if ( mv.from_ == hmove.to_ && mv.to_ == hmove.from_ )
                reps++;
            }
            retBetta = reps < 2;
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
  CapturesHashTable::Flag testCaptureHashItem(int depth, int ply, ScoreType alpha, ScoreType betta, Figure::Type minimalType, Move & hmove)
  {
    hmove.clear();
    CaptureHItem & hitem = chash_[board_.hashCode()];

    if ( hitem.hcode_ == board_.hashCode() )
    {
      board_.unpack(hitem.move_, hmove);

      // ensure that we already verified checking moves
      if ( depth < 0 || !(hitem.depth_ & 32) /* HACK */ || board_.underCheck() )
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

        THROW_IF( (Figure::Color)hitem.color_ != board_.getColor(), "identical hash code but different color in captures" );

        if ( CapturesHashTable::Alpha == hitem.flag_ && hscore <= alpha )
        {
          THROW_IF( !stop_ && (alpha < -32760 || alpha > 32760), "invalid score" );
          return CapturesHashTable::Alpha;
        }

#ifdef RETURN_IF_BETTA
        if ( (GeneralHashTable::Betta == hitem.flag_ || GeneralHashTable::AlphaBetta == hitem.flag_) &&
              hmove && (hmove.capture_ || hmove.new_type_) && hscore >= betta )
        {
          THROW_IF( !stop_ && (betta < -32760 || betta > 32760), "invalid score" );
          return CapturesHashTable::Betta;
        }
#endif // RETURN_IF_BETTA
      }
    }

#if ((defined USE_GENERAL_HASH_IN_CAPS) && (defined USE_HASH_TABLE_GENERAL))
    if ( !hmove )
    {
      GeneralHItem & ghitem = ghash_[board_.hashCode()];
      if ( ghitem.move_ && ghitem.hcode_ == board_.hashCode() )
      {
        board_.unpack(ghitem.move_, hmove);
        if ( !board_.underCheck() && board_.getField(hmove.to_).type() < minimalType )
          hmove.clear();
      }
    }
#endif

    return CapturesHashTable::AlphaBetta;
  }
#endif

#ifdef USE_HASH_TABLE_CAPTURE
  void updateCaptureHash(int depth, int ply, const Move & move, const ScoreType score, const ScoreType betta, const uint64 & hcode, Figure::Color color)
  {
    if ( board_.repsCount() < 2 )
    {
      PackedMove pm = board_.pack(move);
      chash_.push(hcode, score, color, score >= betta ? CapturesHashTable::Betta : CapturesHashTable::AlphaBetta, depth, ply, pm);
    }
  }
#endif // USE_HASH_TABLE_CAPTURE

#ifdef VERIFY_ESCAPE_GENERATOR
  void verifyEscapeGen(const Move & hmove);
#endif

#ifdef VERIFY_ESCAPE_GENERATOR_LIMITED
  void verifyEscapeGenLimited(const Move & hmove, Figure::Type minimalType);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  void verifyChecksGenerator(Figure::Type minimalType);
#endif

#ifdef VERIFY_CAPS_GENERATOR
  void verifyCapsGenerator();
#endif

#ifdef VERIFY_FAST_GENERATOR
  void verifyFastGenerator(const Move & hmove, const Move & killer);
#endif

#ifdef VERIFY_QUIES_GENERATOR
  void verifyQuiesGenerator(const Move & hmove);
#endif

#ifdef RECAPTURE_EXTENSION
  bool recapture(int ply, int depth, int initial_balance);
#endif //RECAPTURE_EXTENSION

  inline bool pawnBeforePromotion(const MoveCmd & move) const
  {
    const Field & field = board_.getField(move.to_);
    if ( field.type() == Figure::TypePawn )
    {
      // before promotion
      int8 y = move.to_ >> 3;
      return 1 == y || 6 == y;
    }
    return false;
  }

  /// returns numer of ply to extend
  int do_extension(int depth, int ply, ScoreType alpha, ScoreType betta, bool was_winnerloser, int initial_balance);

  // is given movement caused by previous. this mean that if we don't do this move we loose
  // we actually check if moved figure was attacked by previously moved one or from direction it was moved from
  bool isRealThreat(const Move & move);

  // do we need additional check extension
  int extend_check(int depth, int ply, EscapeGenerator & eg, ScoreType alpha, ScoreType betta);
};
