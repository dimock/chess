#pragma once


/*************************************************************
  Player.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
  Player class implements the main search routine (alpha-betta algorithm)
 *************************************************************/

#include "Board.h"
#include "HashTable.h"
#include "Evaluator.h"
#include <time.h>
#include <queue>

class SearchResult
{
public:

  SearchResult();

  /// statistic
  clock_t dt_;
  int nodesCount_;
  int totalNodes_;
  int depthMax_;
  int numOfMoves_;
  int counter_;
  Board board_;

  /// result
  Move best_;
  Move pv_[MaxPly+1];
  int  depth_;

  ScoreType score_;
};

struct SearchData
{
  SearchData();

  void reset();
  void restart();

  void inc_nc()
  {
    totalNodes_++;
    nodesCount_++;
  }

  Board board_;
  int counter_;
  int numOfMoves_;
  int depth_;
  int nodesCount_;
  int totalNodes_;
  int plyMax_;
  clock_t tstart_;
  clock_t tprev_;
  Move best_;
};


typedef void (*QueryInputCommand)();
typedef int  (*GiveMoreTime)();
typedef void (*SendOutput)(SearchResult *);
typedef void (*SendStats)(SearchData *);

struct CallbackStruct
{
  CallbackStruct() :
    queryInput_(0), giveTime_(0), sendOutput_(0), sendStats_(0), sendFinished_(0)
  {
  }

  QueryInputCommand queryInput_;
  GiveMoreTime      giveTime_;
  SendOutput        sendOutput_;
  SendStats         sendStats_;
  SendOutput        sendFinished_;
};


struct PostedCommand
{
  enum CommandType
  { ctNONE, ctUNDO, ctUPDATE, ctHINT, ctNEW, ctFEN };

  PostedCommand(CommandType t = ctNONE) : type_(t)
  {
  }

  std::string fen_;
  CommandType type_;
};



class CapsGenerator;
class MovesGenerator;
class EscapeGenerator;
class ChecksGenerator;

struct PlyStack
{
  PlyStack() {}

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

struct SearchParams
{
  SearchParams();

  void reset();

  int  timeLimitMS_;
  int  depthMax_;
  bool analyze_mode_;
};

class Player
{
  friend class CapsGenerator;
  friend class MovesGenerator;
  friend class EscapeGenerator;
  friend class ChecksGenerator;

public:

  // initialize global arrays, tables, masks, etc. write them to it's board_
  Player();
  ~Player();

  void clearHash();

  void setMemory(int mb);
  void setCallbacks(CallbackStruct cs);

  void postCommand(const PostedCommand & cmd);

  bool fromFEN(const char * fen);
  bool toFEN(char * fen) const;

  // call it to start search
  bool findMove(SearchResult * );
  
  Board & getBoard() { return scontexts_[0].board_; }  
  const Board & getBoard() const { return scontexts_[0].board_; }

  void pleaseStop();
  
  void setAnalyzeMode(bool);
  void setTimeLimit(int ms);
  void setMaxDepth(int d);

private:

  bool checkForStop();
  void reset();

  // time control
  void testTimer();
  bool stopped() const { return stop_; }
  void testInput();

  // start point of search algorithm
  bool search(SearchResult * sres);

  // testing of move generator
  void enumerate();
  void enumerate(int depth);

  // search routine
  ScoreType alphaBetta0();
  ScoreType alphaBetta(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool nm = false);
  ScoreType captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0 = -ScoreMax);
  int depthIncrement(int ictx, Move & move, bool pv) const;
  void assemblePV(int ictx, const Move & move, bool checking, int ply);


  // is given movement caused by previous. this mean that if we don't do this move we loose
  // we actually check if moved figure was attacked by previously moved one or from direction it was moved from
  bool isRealThreat(int ictx, const Move & move);

#ifdef USE_HASH
  void prefetchHash(int ictx);
  // we should return alpha if flag is Alpha, or Betta if flag is Betta
  GHashTable::Flag getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv);
  void putHash(int ictx, const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat);
#endif // USE_HASH

  // analyze mode support
  std::queue<PostedCommand> posted_;
  CallbackStruct callbacks_;

  // search data
  static const int depth0_ = 1; // start depth
  volatile bool stop_;

  struct SearchContext
  {
    SearchContext();
    ~SearchContext();

    UndoInfo * undoStack_;
    Evaluator eval_;
    Board board_;
    PlyStack plystack_[MaxPly+1];
    Move moves_[Board::MovesMax];
  };

  // will be used in multi-threading mode???
  SearchContext scontexts_[1];

  SearchData sdata_;
  SearchParams sparams_;

  // global data
  MovesTable * g_movesTable;
  FigureDir  * g_figureDir;
  PawnMasks  * g_pawnMasks_;
  BetweenMask * g_betweenMasks;
  DeltaPosCounter * g_deltaPosCounter;
  DistanceCounter * g_distanceCounter;

#ifdef USE_HASH
  GHashTable hash_;
  EHashTable ehash_;
#endif


/// for DEBUG, verification

#ifdef VERIFY_ESCAPE_GENERATOR
  void verifyEscapeGen(int ictx, const Move & hmove);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  void verifyChecksGenerator(int ictx);
#endif

#ifdef VERIFY_CAPS_GENERATOR
  void verifyCapsGenerator(int ictx);
#endif

#ifdef VERIFY_FAST_GENERATOR
  void verifyFastGenerator(int ictx, const Move & hmove, const Move & killer);
#endif

#ifdef VERIFY_TACTICAL_GENERATOR
  void verifyTacticalGenerator(int ictx);
#endif

  void findSequence(int ictx, const Move & move, int ply, int depth, int counter, ScoreType alpha, ScoreType betta) const;
  void verifyGenerators(int ictx, const Move & hmove);

public:
  void saveHash(const char * fname) const;
  void loadHash(const char * fname);
};
