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

  /// result
  Move best_;
  Move pv_[MaxDepth];
  int  depth_;

  ScoreType score_;
//  bool wminus_;
};

class Player
{
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

  ScoreType alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, const Move & before, Move & move, bool & found);

  void testTimer();

  volatile bool stop_;
  Board board_;
  int timeLimitMS_;
  int depthMax_;
  int nodesCount_, totalNodes_;
  clock_t tstart_, tprev_;


  // global data
  MoveCmd * g_moves;
  MovesTable * g_movesTable;
  FigureDir * g_figureDir;
  PawnMasks * g_pawnMasks_;
  BetweenMask * g_betweenMasks;
  DeltaPosCounter * g_deltaPosCounter;
  DistanceCounter * g_distanceCounter;


  //////////////////////////////////////////////////////////////////////////
  inline void movement(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & before, Move & b, const Move & mv, Move & move, bool & found, int & counter)
  {
	totalNodes_++;
	nodesCount_++;
    if ( board_.makeMove(mv) )
    {
      counter++;
      ScoreType s = alpha;
      if ( board_.drawState() )
        s = 0;
      else if ( depth <= 1 )
      {
        s = -board_.evaluate();
      }
      else
      {
        Move m;
        bool f = false;

        s = -alphaBetta(depth-1, ply+1, -betta, -alpha, b, m, f);
      }

      if ( !stop_ && s > alpha )
      {
        alpha = s;
        move = mv;
        if ( before == move )
          found = true;
      }
    }

#ifndef NDEBUG
    board_.verifyMasks();
#endif

    board_.unmakeMove();

#ifndef NDEBUG
    board_.verifyMasks();
#endif
  }

};