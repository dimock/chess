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
  Move killer_;
  Move pv_[MaxPly+1];

  void clearPV(int depth)
  {
    for (int i = 0; i < depth; ++i)
      pv_[i].clear();
  }
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
  void movement(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & move, int & counter, bool null_move, int history_max, bool threat);
  void capture(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & cap, int & counter, bool do_checks);
  
  void assemblePV(const Move & move, bool checking, int ply)
  {
    if ( ply > depth_ || ply >= MaxPly )
      return;

    contexts_[ply].pv_[ply] = move;
    contexts_[ply].pv_[ply].checking_ = checking;
    contexts_[ply].pv_[ply+1].clear();

    for (int i = ply+1; i < depth_; ++i)
    {
      contexts_[ply].pv_[i] = contexts_[ply+1].pv_[i];
      if ( !contexts_[ply].pv_[i] )
        break;
    }
  }

#ifdef USE_HASH_TABLE_GENERAL
  void updateGeneralHash(const Move & move, int depth, int ply, const ScoreType score, const ScoreType betta, const uint64 & hcode, Figure::Color color, bool threat)
  {
    PackedMove pm = board_.pack(move);
    ghash_.push(hcode, score, depth, ply, board_.halfmovesCount()-1, color, score >= betta ? GeneralHashTable::Betta : GeneralHashTable::AlphaBetta, pm, threat);
  }

  // we should return alpha if flag is Alpha, or betta if flag is Betta
  GeneralHashTable::Flag getGeneralHashItem(int depth, int ply, ScoreType alpha, ScoreType betta, Move & pv)
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

    if ( GeneralHashTable::Alpha != hitem.flag_ )
    {
      pv = board_.unpack(hitem.move_);
    }

    if ( hitem.depth_ >= depth )
    {
      if ( GeneralHashTable::Alpha == hitem.flag_ && hscore <= alpha )
        return GeneralHashTable::Alpha;

#ifdef RETURN_IF_BETTA
      if ( (GeneralHashTable::Betta == hitem.flag_ || GeneralHashTable::AlphaBetta == hitem.flag_) && pv && hscore >= betta )
      {
#ifndef NDEBUG
        Board board0 = board_;
#endif

        bool retBetta = pv.rindex_ >= 0 || pv.new_type_;
        bool checking = false;

        if ( !retBetta )
        {
          totalNodes_++;
          nodesCount_++;

          if ( board_.makeMove(pv) )
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
          assemblePV(pv, checking, ply);
          return GeneralHashTable::Betta;
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

};