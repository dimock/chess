#pragma once

#include "Board.h"
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

  void setMaxDepth(int d);
  bool initialize(bool enableBook, int depthMax);
  void setUpLeft(const QPoint & upleft) const { upleft_ = upleft; }
	void draw(QWidget * view, const QPoint & cursorPt) const;
  bool makeMovement(const QPoint & pt);
  bool selectFigure(const QPoint & pt);
  void setTurned(bool t) { turned_ = t; }
  void clearSelected();

  Board getBoard() const;
  //int doStep(CalcResult & cres);
  bool applyMove(const Move &);
  int movesCount() const;
  //void stop();
  //bool calculateSteps(std::vector<Step> & steps);
  void undo();
  void redo();
  bool save() const;
  bool load();

  long long getTicks() const { return ticks_; }
  int numOfMoves() const { return numOfMoves_; }


private:

	bool doSave() const;
	bool doLoad();

  QPoint coordByField(int) const;
  bool getFigureOnPt(const QPoint & pt, Figure & fig) const;
  int  getPositionOnPt(const QPoint & pt) const;
  //const Figure * getSelection() const;


  //ChessAlgorithm alg_;

	void drawBoard(QPainter *, QSize & ) const;
	void drawFigures(QPainter *, QSize & ) const;
  void drawCurrentMoving(QPainter * , QSize & , const QPoint & cursorPt) const;

  std::vector<Move> selectedMoves_;
  std::set<int> selectedPositions_;
  Board board_;
  int halfmovesNumber_;

  mutable Figure selectedFigure_;
  mutable QPoint upleft_;

  static std::auto_ptr<QImage> fimages_[12];

  int squareSize_;
  int borderWidth_;
  QSize boardSize_;
  bool turned_;

  long long ticks_;
  int numOfMoves_;
  WeightType wmax_;

  volatile bool working_;
};