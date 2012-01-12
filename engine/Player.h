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
    if ( d > 1 && d < 17 )
      depthMax_ = d;
  }

private:

  void printPV(Board & pv_board, SearchResult & sres, std::ostream * out);

  ScoreType nullMove(int depth, int ply, ScoreType alpha, ScoreType betta);
  ScoreType alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, bool null_move, bool extension);
  ScoreType captures(int depth, int ply, ScoreType alpha, ScoreType betta, int delta, bool do_checks);

  void testTimer();

  volatile bool stop_;
  Board board_;
  int timeLimitMS_;
  int depthMax_;
  int depth_;
  int nodesCount_, totalNodes_;
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
  void movement(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & move, int & counter, bool null_move, bool extension, int history_max);
  void capture(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & cap, int & counter, bool do_checks);
  
  void assemblePV(const Move & move, int ply)
  {
    if ( ply >= depth_ )
      return;

    contexts_[ply].pv_[ply] = move;
    contexts_[ply].pv_[ply+1].clear();

    for (int i = ply+1; i < depth_; ++i)
    {
      contexts_[ply].pv_[i] = contexts_[ply+1].pv_[i];
      if ( !contexts_[ply].pv_[i] )
        break;
    }
  }

#ifdef USE_HASH_TABLE_GENERAL
  void updateGeneralHash(const Move & move, int depth, int ply, const ScoreType score, const ScoreType betta, const uint64 & hcode)
  {
    Figure::Color color = Figure::otherColor(board_.getColor());
    PackedMove pm = board_.pack(move);
    ghash_.push(hcode, score, depth, ply, color, score >= betta ? GeneralHashTable::Betta : GeneralHashTable::AlphaBetta, pm);
  }
#endif // USE_HASH_TABLE_GENERAL

#ifdef USE_HASH_TABLE_CAPTURE
  void updateCaptureHash(const Move & move, const ScoreType score, const ScoreType betta, const uint64 & hcode)
  {
    Figure::Color color = Figure::otherColor(board_.getColor());
    PackedMove pm = board_.pack(move);
    chash_.push(hcode, score, color, score >= betta ? CapturesHashTable::Betta : CapturesHashTable::AlphaBetta, pm);
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