#pragma once

#include "Board.h"
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
  Move pv_[MaxDepth];
  int  depth_;

  ScoreType score_;
//  bool wminus_;
};

class CapsGenerator;
class MovesGenerator;
class EscapeGenerator;

struct PlyContext
{
  Move killer_;

  void clear()
  {
    killer_.clear();
  }
};

class Player
{
  friend class CapsGenerator;
  friend class MovesGenerator;
  friend class EscapeGenerator;

  enum { MaxPly = 40 };

public:

  // initialize global arrays, tables, masks, etc. write them to it's board_
  Player();
  ~Player();

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

  void printPV(SearchResult & sres, std::ostream * out);

  ScoreType alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta);
  ScoreType captures(int ply, ScoreType alpha, ScoreType betta, int delta);

  void testTimer();

  volatile bool stop_;
  Board board_;
  int timeLimitMS_;
  int depthMax_;
  int nodesCount_, totalNodes_;
  bool firstIter_;
  clock_t tstart_, tprev_;
  Move before_, best_;
  bool beforeFound_;

  PlyContext contexts_[MaxPly];

  // global data
  MoveCmd * g_moves;
  MovesTable * g_movesTable;
  FigureDir * g_figureDir;
  PawnMasks * g_pawnMasks_;
  BetweenMask * g_betweenMasks;
  DeltaPosCounter * g_deltaPosCounter;
  DistanceCounter * g_distanceCounter;


  //////////////////////////////////////////////////////////////////////////
  void movement(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & move, int & counter);
  void capture(int ply, ScoreType & alpha, ScoreType betta, const Move & cap, int & counter);

};