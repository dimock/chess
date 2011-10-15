#pragma once

#include <QMainWindow>
#include <QThread>
#include "ChessPos.h"

class ChessWidget;
class ChessAlgThread : public QThread
{
  Q_OBJECT

public:
  ChessAlgThread(ChessWidget * widget);

protected:

  void run();

  ChessWidget * widget_;
};

class ChessWidget : public QMainWindow
{
	Q_OBJECT;

  friend class ChessAlgThread;

public:
	ChessWidget(QWidget * parent = 0);
  ~ChessWidget();

  void closeEvent(QCloseEvent *event);
	void paintEvent(QPaintEvent * e);
	void mouseDoubleClickEvent(QMouseEvent * e);
  void mousePressEvent(QMouseEvent * e);
  void mouseReleaseEvent(QMouseEvent * e);
  void mouseMoveEvent(QMouseEvent * e);
  void keyReleaseEvent(QKeyEvent * e);

private:

  void createMenu();

private slots:

  void onNew();
  void onLoad();
  bool onSave();
  void onNext();
  void onPrev();
  void onGo();
  void onStepDone();
  void onTimeoutStop();
  void onTurnBoard(bool t);

private:

  void enableActions(bool on);
  void drawState();
  void drawInfo();
  void doBestStep();

  bool okToReset();

  QPoint curPt_;
  QPoint upleft_;
  ChessPosition cpos_;
  int full_t_, dt_;
  int bs_count_;
  int depth_;
  int movesCount_, totalCurr_, movesCurr_, forcesCount_, additionalCount_, nullMovesCount_;
  char pv_str_[256];
  double moves_avg_base_, depth_avg_, moves_base_;
  WeightType w_;
  int stepsCount_;
  bool goingToClose_;
  bool changed_;

  bool autoPlay_;
  int  timelimit_;
  bool useTimer_;
  bool computerAnswers_;
  int  depthMax_;
  bool enableBook_;

  QAction * onNewAction_;
  QAction * onLoadAction_;
  QAction * onSaveAction_;
  QAction * onNextAction_;
  QAction * onPrevAction_;
  QAction * onGoAction_;
  QAction * onTurnBoardAction_;

  ChessAlgThread thread_;
};