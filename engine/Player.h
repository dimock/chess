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

  enum { MaxPly = 128 };

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
  inline void movement(int depth, int ply, ScoreType & alpha, ScoreType betta, const Move & move, int & counter)
  {
	  totalNodes_++;
	  nodesCount_++;

#ifndef NDEBUG
    Board board0 = board_;
#endif

    if ( board_.makeMove(move) )
    {
      counter++;
      ScoreType s = alpha;
      if ( board_.drawState() )
        s = 0;
      else if ( depth <= 1 )
      {
        s = -board_.evaluate();
#ifdef PERFORM_CAPTURES
        int delta = (int)s - (int)betta - (int)Figure::positionGain_;
        if ( s > alpha && delta < Figure::figureWeight_[Figure::TypeQueen] )
		    {
          //QpfTimer qpt;
			    ScoreType betta1 = s < betta ? s : betta;
			    s = -captures(ply+1, -betta1, -alpha, delta);
          //Board::ticks_ += qpt.ticks();
		    }
#endif
      }
      else
        s = -alphaBetta(depth-1, ply+1, -betta, -alpha);

      if ( !stop_ && s > alpha )
      {
        Move & killer = contexts_[ply].killer_;
        alpha = s;

        if ( 0 == ply )
        {
          best_ = move;
          if ( before_ == best_ )
            beforeFound_ = true;
        }

        if ( s > killer.score_ )
        {
          killer = move;
          killer.score_ = s;
        }
      }
    }

#ifndef NDEBUG
    board_.verifyMasks();
#endif

    board_.unmakeMove();

    THROW_IF( board0 != board_, "board unmake wasn't correct applied" );

#ifndef NDEBUG
    board_.verifyMasks();
#endif
  }

  //////////////////////////////////////////////////////////////////////////
  inline void capture(int ply, ScoreType & alpha, ScoreType betta, const Move & cap)
  {
	  totalNodes_++;
	  nodesCount_++;

#ifndef NDEBUG
    Board board0 = board_;
#endif

	  if ( board_.makeMove(cap) )
	  {
		  ScoreType s = alpha;
		  if ( board_.drawState() )
			  s = 0;
		  else
		  {
			  s = -board_.evaluate();
        int delta = s - betta - Figure::positionGain_;
        if ( s > alpha && delta < Figure::figureWeight_[Figure::TypeQueen] )
			  {
				  ScoreType betta1 = s < betta ? s : betta;
				  s = -captures(ply+1, -betta1, -alpha, delta);
			  }
		  }
		  if ( !stop_ && s > alpha )
      {
        Move killer = contexts_[ply].killer_;

			  alpha = s;
        if ( s > killer.score_ )
        {
          killer = cap;
          killer.score_ = s;
        }
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
};