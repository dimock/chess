#pragma once

/*************************************************************
  ChessPos.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


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
  ~ChessPosition();

  void pleaseStop();
  void setMaxDepth(int d);
  void setTimeLimit(int ms);
  bool initialize(int depthMax);
  void setUpLeft(const QPoint & upleft) const { upleft_ = upleft; }
	void draw(QWidget * view, const QPoint & cursorPt) const;
  bool makeMovement(const QPoint & pt);
  bool selectFigure(const QPoint & pt);
  void setTurned(bool t) { turned_ = t; }
  void clearSelected();
  int  getDiffHeight() const { return diff_hei_; }
  int  getBoardHeight() const { return boardSize_.height(); }
  int  getBoardWidth() const { return boardSize_.width(); }

  // making move
  bool findMove(SearchResult * sres);
  bool applyMove(const Move &);

  const Board & getBoard() const;
  Board & getBoardT() { return player_.getBoard(); }
  int movesCount() const;

  bool fromFEN(const char * fen);
  bool toFEN(char * fen) const;

  void undo();
  void redo();
  bool save() const;
  bool load();

  void setUpdateCallback(SendOutput callback);

private:

  void setLastMove(const Board & board);

	bool doSave() const;
	bool doLoad();

  QPoint coordByField(int) const;
  bool getFieldByPt(const QPoint & pt, Field & field) const;
  int  getPositionOnPt(const QPoint & pt) const;

  const QImage * figImage(Figure::Type, Figure::Color) const;

	void drawBoard(QPainter *, QSize & ) const;
	void drawFigures(QPainter *, QSize & ) const;
  void drawCurrentMoving(QPainter * , QSize & , const QPoint & cursorPt) const;
  void drawMaterialDifference(QPainter * , QSize & ) const;

  std::vector<Move> selectedMoves_;
  std::set<int> selectedPositions_;
  Board vboard_;
  Move  vmove_;
  int halfmovesNumber_;

  mutable int selectedPos_;
  mutable QPoint upleft_;
  int diff_hei_, diff_margin_;

  static std::auto_ptr<QImage> fimages_[12];

  int squareSize_;
  int squareDiffSize_;
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