#pragma once

#include "Player.h"
#include <QPoint>
#include <QSize>
#include <set>

class QWidget;
class QPainter;
class QSize;
class QImage;


class ChessPosition
{
public:

	ChessPosition();

  void pleaseStop();
  void setMaxDepth(int d);
  void setTimeLimit(int ms);
  bool initialize(bool enableBook, int depthMax);
  void setUpLeft(const QPoint & upleft) const { upleft_ = upleft; }
	void draw(QWidget * view, const QPoint & cursorPt) const;
  bool makeMovement(const QPoint & pt);
  bool selectFigure(const QPoint & pt);
  void setTurned(bool t) { turned_ = t; }
  void clearSelected();

  // making move
  bool findMove(SearchResult & sres);

  const Board & getBoard() const;
  Board & getBoardT() { return player_.getBoard(); }
  int movesCount() const;

  bool fromFEN(const char * fen);
  bool toFEN(char * fen) const;

  void undo();
  void redo();
  bool save() const;
  bool load();


private:

  bool applyMove(const Move &);
  void setLastMove(const Board & board);

	bool doSave() const;
	bool doLoad();

  QPoint coordByField(int) const;
  bool getFigureOnPt(const QPoint & pt, Figure & fig) const;
  int  getPositionOnPt(const QPoint & pt) const;


	void drawBoard(QPainter *, QSize & ) const;
	void drawFigures(QPainter *, QSize & ) const;
  void drawCurrentMoving(QPainter * , QSize & , const QPoint & cursorPt) const;

  std::vector<Move> selectedMoves_;
  std::set<int> selectedPositions_;
  Board vboard_;
  Move  vmove_;
  int halfmovesNumber_;

  mutable Figure selectedFigure_;
  mutable QPoint upleft_;

  static std::auto_ptr<QImage> fimages_[12];

  int squareSize_;
  int borderWidth_;
  QSize boardSize_;
  bool turned_;


  /*! Player is the main class of engine
      it performs search of best move
   */
  Player player_;

  /// indicates that search is in progress
  volatile bool working_;
};