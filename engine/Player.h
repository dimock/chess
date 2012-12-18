#pragma once


/*************************************************************
  Player.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
  Player class implements the main search routine (alpha-betta algorithm)
 *************************************************************/

#include "Board.h"
#include "HashTable.h"
#include "HashTable2.h"
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

  // write result here
  std::ostream * out_;

  /// result
  Move best_;
  Move pv_[MaxPly+1];
  int  depth_;

  ScoreType score_;
};

class CapsGenerator;
class MovesGenerator;
class EscapeGenerator;
class ChecksGenerator;

// implementation isn't perfect )) have to rewrite
typedef void (*PLAYER_CALLBACK)();
typedef int  (*GIVE_MORE_TIME)();

struct PlyContext
{
  PlyContext() {}

  Move killer_;
  Move pv_[MaxPly+1];

  void clearPV(int depth)
  {
    for (int i = 0; i < depth; ++i)
      pv_[i].clear();
  }

  void clear(int ply)
  {
    pv_[ply].clear();
  }

  void setKiller(const Move & killer)
  {
    if ( !killer.capture_ && !killer.new_type_ )
      killer_ = killer;
  }

  void clearKiller()
  {
    killer_.clear();
  }
};

inline Figure::Type delta2type(int delta)
{
  Figure::Type minimalType = Figure::TypePawn;

#ifdef USE_DELTA_PRUNING
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

  bool findMove(SearchResult * );
  
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

  bool checkForStop()
  {
    if ( totalNodes_ && !(totalNodes_ & TIMING_FLAG) )
    {
      if ( timeLimitMS_ > 0 )
        testTimer();
      else
        testInput();
    }
    return stop_;
  }

  void reset();

  bool stopped() const
  {
    return stop_;
  }

  void testInput();
  void processPosted(int t);
  void printPV();

  // start point of search algorithm
  bool search();

  // new search routine
  Move moves_[Board::MovesMax];
  static const int depth0_ = 1;

  void inc_nc()
  {
    totalNodes_++;
    nodesCount_++;
  }

  int nextDepth(int depth, const Move & move, bool pv) const
  {
    if ( board_.underCheck() )
      return depth;

    depth--;

    if ( !move.see_good_ || !pv )
      return depth;

    if ( move.new_type_ == Figure::TypeQueen )
      return depth+1;

    if ( board_.halfmovesCount() > 1 )
    {
      const UndoInfo & prev = board_.undoInfoRev(-1);
      const UndoInfo & curr = board_.undoInfoRev(0);

      if ( move.capture_ && prev.to_ == curr.to_ || curr.en_passant_ == curr.to_ )
        return depth+1;
    }

    return depth;
  }

  // search routine
  ScoreType alphaBetta0();
  ScoreType alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv);
  ScoreType captures(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0 = -ScoreMax);

  // time control
  void testTimer();

  volatile bool stop_;
  Board board_;
  int timeLimitMS_;
  int depthMax_;
  int depth_;
  int nodesCount_, totalNodes_, plyMax_;
  clock_t tstart_, tprev_;
  Move best_;
  SearchResult * sres_;

  PlyContext contexts_[MaxPly+1];
  UndoInfo * pvundoStack_;

  // global data
  UndoInfo * g_undoStack;
  MovesTable * g_movesTable;
  FigureDir * g_figureDir;
  PawnMasks * g_pawnMasks_;
  BetweenMask * g_betweenMasks;
  DeltaPosCounter * g_deltaPosCounter;
  DistanceCounter * g_distanceCounter;

#ifdef USE_HASH
  GHashTable hash_;
#endif

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

  // is given movement caused by previous. this mean that if we don't do this move we loose
  // we actually check if moved figure was attacked by previously moved one or from direction it was moved from
  bool isRealThreat(const Move & move);

#ifdef USE_HASH
  // we should return alpha if flag is Alpha, or Betta if flag is Betta
  GHashTable::Flag getHash(int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv);
  void putHash(const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat);
#endif // USE_HASH


/// verification
#ifdef VERIFY_ESCAPE_GENERATOR
  void verifyEscapeGen(const Move & hmove);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  void verifyChecksGenerator();
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

  // for DEBUG
  void findSequence(const Move & move, int ply, int depth, int counter, ScoreType alpha, ScoreType betta) const;
};
