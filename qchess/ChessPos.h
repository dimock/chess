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
  //bool makeFigureStep(const QPoint & pt);
  bool selectFigure(const QPoint & pt);
  void setTurned(bool t) { turned_ = t; }
  void clearSelected();

  Board getBoard() const;
  //int doStep(CalcResult & cres);
  //bool applyStep(const Step & step);
  int movesCount() const;
  //void stop();
  //bool calculateSteps(std::vector<Step> & steps);
  //void prevPos();
  //void nextPos();
  bool save() const;
  bool load();


private:

	bool doSave() const;
	bool doLoad();

  QPoint coordByField(int) const;
  bool getFigureOnPt(const QPoint & pt, Figure & fig) const;
  int  getPositionOnPt(const QPoint & pt) const;
  //const Figure * getSelection() const;

  //void clearSteps();

  //ChessAlgorithm alg_;

	void drawBoard(QPainter *, QSize & ) const;
	void drawFigures(QPainter *, QSize & ) const;
  void drawCurrentMoving(QPainter * , QSize & , const QPoint & cursorPt) const;

  //std::vector<Step> steps_; 
  //std::vector<Step> selectedSteps_;
  std::set<int> selectedPositions_;
  //Step lastStep_;
  Board board_;

  mutable Figure selectedFigure_;
  mutable QPoint upleft_;

  static std::auto_ptr<QImage> fimages_[12];

  int squareSize_;
  int borderWidth_;
  QSize boardSize_;
  bool turned_;

  volatile bool working_;
};