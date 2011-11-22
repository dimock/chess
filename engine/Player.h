#pragma once

#include "Board.h"

class SearchResult
{
public:

  SearchResult();

  /// statistic
  int movesCount_;
  int totalMoves_;
  int forcedMoves_;
  int additionalMoves_;
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

  Player();

  bool initialize(const char * fen);
  bool findMove(SearchResult & );
  Board & getBoard() { return board_; }
  const Board & getBoard() const { return board_; }
  void stop() { stop_ = true; }

private:

  volatile bool stop_;
  Board board_;
};