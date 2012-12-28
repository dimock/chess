#pragma once


/*************************************************************
  Player.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
  Player class implements the main search routine (alpha-betta algorithm)
 *************************************************************/

#include "Board.h"
#include "HashTable.h"
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
  int plyMax_;
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
typedef void (*SendStatus)(SearchData *);

struct CallbackStruct
{
  CallbackStruct() :
    queryInput_(0), giveTime_(0), sendOutput_(0), sendStatus_(0)
  {
  }

  QueryInputCommand queryInput_;
  GiveMoreTime      giveTime_;
  SendOutput        sendOutput_;
  SendStatus        sendStatus_;
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

  int timeLimitMS_;
  int depthMax_;
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

  void setMemory(int mb);
  void setCallbacks(CallbackStruct cs);

  void postCommand(const PostedCommand & cmd);

  bool fromFEN(const char * fen);
  bool toFEN(char * fen) const;

  // call it to start search
  bool findMove(SearchResult * );
  
  Board & getBoard()
  {
    return board_;
  }
  
  const Board & getBoard() const
  {
    return board_;
  }

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
  ScoreType alphaBetta(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv);
  ScoreType captures(int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0 = -ScoreMax);
  int nextDepth(int depth, const Move & move, bool pv) const;
  void assemblePV(const Move & move, bool checking, int ply);


  // is given movement caused by previous. this mean that if we don't do this move we loose
  // we actually check if moved figure was attacked by previously moved one or from direction it was moved from
  bool isRealThreat(const Move & move);

#ifdef USE_HASH
  // we should return alpha if flag is Alpha, or Betta if flag is Betta
  GHashTable::Flag getHash(int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv);
  void putHash(const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat);
#endif // USE_HASH

  // analyze mode support
  std::queue<PostedCommand> posted_;
  CallbackStruct callbacks_;

  // search data
  volatile bool stop_;
  Board board_;
  SearchData sdata_;
  SearchParams sparams_;
  PlyStack plystack_[MaxPly+1];
  Move moves_[Board::MovesMax];
  static const int depth0_ = 1;

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


/// for DEBUG, verification

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

#ifdef VERIFY_TACTICAL_GENERATOR
  void verifyTacticalGenerator();
#endif

  void findSequence(const Move & move, int ply, int depth, int counter, ScoreType alpha, ScoreType betta) const;
  void verifyGenerators(const Move & hmove);

public:
  void saveHash(const char * fname) const;
  void loadHash(const char * fname);
};
