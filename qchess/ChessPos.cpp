#include "ChessPos.h"
#include <QDir>
#include <QFileDialog>
#include <QWidget>
#include <QPainter>
#include "selfiguredlg.h"
#include <limits>
#include <fstream>
#include <string>
#include <iostream>

std::auto_ptr<QImage> ChessPosition::fimages_[12];

using namespace std;

ChessPosition::ChessPosition() : board_(Figure::ColorWhite), working_(false), turned_(false)
{
  lastStep_.clear();

  squareSize_ = 44;
  borderWidth_ = 16;
  boardSize_ = QSize(squareSize_*8+borderWidth_*2, squareSize_*8+borderWidth_*2);
}


bool ChessPosition::initialize(bool enableBook, int depthMax)
{
  if ( working_)
    return false;

  alg_.init(Figure::ColorWhite);
  alg_.enableBook(enableBook);
  alg_.setDepth(depthMax);

  clearSteps();

  Board & board = *alg_.getCurrent();

  for (char i = 'a'; i <= 'h'; ++i)
  {
    Figure pawn(Figure::TypePawn, i, 7, Figure::ColorBlack, true);
    board.addFigure(pawn);
  }

  for (char i = 'a'; i <= 'h'; ++i)
  {
    Figure pawn(Figure::TypePawn, i, 2, Figure::ColorWhite, true);
    board.addFigure(pawn);
  }

  {
    Figure knight1(Figure::TypeKnight, 'b', 8, Figure::ColorBlack, true);
    Figure knight2(Figure::TypeKnight, 'g', 8, Figure::ColorBlack, true);

    Figure bishop1(Figure::TypeBishop, 'c', 8, Figure::ColorBlack, true);
    Figure bishop2(Figure::TypeBishop, 'f', 8, Figure::ColorBlack, true);

    Figure rook1(Figure::TypeRook, 'a', 8, Figure::ColorBlack, true);
    Figure rook2(Figure::TypeRook, 'h', 8, Figure::ColorBlack, true);

    Figure queen(Figure::TypeQueen, 'd', 8, Figure::ColorBlack, true);
    Figure king(Figure::TypeKing, 'e', 8, Figure::ColorBlack, true);

    board.addFigure(knight1);
    board.addFigure(knight2);

    board.addFigure(bishop1);
    board.addFigure(bishop2);

    board.addFigure(rook1);
    board.addFigure(rook2);

    board.addFigure(queen);
    board.addFigure(king);
  }
  {
    Figure knight1(Figure::TypeKnight, 'b', 1, Figure::ColorWhite, true);
    Figure knight2(Figure::TypeKnight, 'g', 1, Figure::ColorWhite, true);

    Figure bishop1(Figure::TypeBishop, 'c', 1, Figure::ColorWhite, true);
    Figure bishop2(Figure::TypeBishop, 'f', 1, Figure::ColorWhite, true);

    Figure rook1(Figure::TypeRook, 'a', 1, Figure::ColorWhite, true);
    Figure rook2(Figure::TypeRook, 'h', 1, Figure::ColorWhite, true);

    Figure queen(Figure::TypeQueen, 'd', 1, Figure::ColorWhite, true);
    Figure king(Figure::TypeKing, 'e', 1, Figure::ColorWhite, true);

    board.addFigure(knight1);
    board.addFigure(knight2);

    board.addFigure(bishop1);
    board.addFigure(bishop2);

    board.addFigure(rook1);
    board.addFigure(rook2);

    board.addFigure(queen);
    board.addFigure(king);
  }

  board_ = *alg_.getCurrent();
  lastStep_.clear();

  return true;
}

void ChessPosition::draw(QWidget * view, const QPoint & cursorPt) const
{
	QPainter painter(view); 

	QSize sz = view->size();
	painter.fillRect(QRect(0, 0, sz.width(), sz.height()), Qt::white);

  if ( boardSize_.width()+upleft_.x() > sz.width() || boardSize_.height()+upleft_.y() > sz.height() )
    return;

	drawBoard(&painter, sz);
	drawFigures(&painter, sz);
  //drawFake(&painter, sz);
  drawCurrentMoving(&painter, sz, cursorPt);
}

void ChessPosition::drawBoard(QPainter * painter, QSize & ) const
{
  QPen penBlack(Qt::black);
  painter->setPen(penBlack);
  painter->drawRect(QRect(upleft_, boardSize_));
  painter->drawRect(QRect(upleft_ + QPoint(borderWidth_-1, borderWidth_-1), QSize(squareSize_*8+1, squareSize_*8+1)));

	QFont font(QObject::tr("Time New Roman"), 10);
	QPen pen(Qt::blue);
	painter->setFont(font);
	painter->setPen(pen);

	QPoint p(upleft_);
  p.rx() += borderWidth_;
  p.ry() += boardSize_.height()-borderWidth_;
	for (int i = 0; i < 8; ++i, p.rx() += squareSize_)
	{
		QRect r(p.x(), p.y(), squareSize_, borderWidth_);
		QString text;
    char c = turned_ ? 'H' - i : 'A' + i;
		text.sprintf("%c", c);
		painter->drawText(r, Qt::AlignCenter, text);
	}

	p = upleft_;
  p.ry() += boardSize_.height()-borderWidth_-squareSize_;
	for (int i = 0; i < 8; ++i, p.ry() -= squareSize_)
	{
		QRect r(p.x(), p.y(), borderWidth_, squareSize_);
		QString text;
    text.sprintf("%d", turned_ ? 8-i : i+1);
		painter->drawText(r, Qt::AlignCenter, text);
	}

	p = upleft_;
	p.rx() += borderWidth_;
	p.ry() += boardSize_.height()-borderWidth_-squareSize_;

	QColor cwhite(240,240,240);
	QColor cblack(100,100,100);

  QColor lwhite(240,200,200);
  QColor lblack(150,110,110);

  QColor swhite(200,240,200);
  QColor sblack(110,150,110);

	for (int y = 0; y < 8; ++y)
	for (int x = 0; x < 8; ++x)
	{
    bool wht = (x+y) & 1;
		QColor color = wht ? cwhite : cblack;

    FPos fpos = turned_ ? FPos(7-x, 7-y) : FPos(x, y);

    if ( lastStep_ && fpos.index() == lastStep_.to_ )
      color = wht ? lwhite : lblack;

    if ( selectedPositions_.find(fpos.index()) != selectedPositions_.end() )
      color = wht ? swhite : sblack;

		QRect r(p.x()+squareSize_*x, p.y()-squareSize_*y, squareSize_, squareSize_);
		painter->fillRect(r, color);

  }
}

void ChessPosition::drawFigures(QPainter * painter, QSize & ) const
{
  QSize boardSize(squareSize_*8+borderWidth_*2, squareSize_*8+borderWidth_*2);

  for (int color = 0; color < 2; ++color)
  {
    for (int index = 0; index < Board::NumOfFigures; ++index)
    {
      Figure fig = board_.getFigure((Figure::Color)color, index);
      if ( !fig || fig == selectedFigure_ )
        continue;

  	  int inum = fig.getColor()*6+(fig.getType()-1);
  	  if ( inum < 0 || inum > 11 )
  		  continue;

  	  if ( !fimages_[inum].get() )
  	  {
  		  QString imgName;
  		  imgName.sprintf(":/images/%s_%c.png", fig.name(), fig.getColor() ? 'w' : 'b');
  		  fimages_[inum].reset( new QImage(imgName) );
  	  }

  	  if ( !fimages_[inum].get() )
  		  continue;

      FPos fpos = FPosIndexer::get(fig.where());
      QPoint p = coordByFPos(fpos);
      QRect r(p.x(), p.y(), squareSize_, squareSize_);

      painter->drawImage(r, *fimages_[inum].get(), fimages_[inum]->rect());
    }
  }
}

void ChessPosition::drawCurrentMoving(QPainter * painter, QSize & , const QPoint & cursorPt) const
{
  if ( selectedFigure_.getType() == Figure::TypeNone || !FPosIndexer::get(selectedFigure_.where()) )
    return;

  int inum = selectedFigure_.getColor()*6+(selectedFigure_.getType()-1);
  if ( inum < 0 || inum > 11 )
    return;

  if ( !fimages_[inum].get() )
  {
    QString imgName;
    imgName.sprintf(":/images/%s_%c.png", selectedFigure_.name(), selectedFigure_.getColor() ? 'w' : 'b');
    fimages_[inum].reset( new QImage(imgName) );
  }

  if ( !fimages_[inum].get() )
    return;

  QRect r(cursorPt.x()-squareSize_/2, cursorPt.y()-squareSize_/2, squareSize_, squareSize_);
  painter->drawImage(r, *fimages_[inum].get(), fimages_[inum]->rect());

}

QPoint ChessPosition::coordByFPos(const FPos & fpos) const
{
  QPoint orig(upleft_.x()+borderWidth_, upleft_.y()+boardSize_.height()-borderWidth_-squareSize_);
  int x = fpos.x();
  int y = fpos.y();
  if ( turned_ )
  {
    x = 7-x;
    y = 7-y;
  }
  QPoint pos = QPoint(orig.x()+squareSize_*x, orig.y()-squareSize_*y);
  return pos;
}

int ChessPosition::getPositionOnPt(const QPoint & pt) const
{
  QPoint dp = pt - (upleft_ + QPoint(borderWidth_, borderWidth_));
  dp.ry() = squareSize_*8 - dp.y();

  if ( dp.x() < 0 || dp.x() >= squareSize_*8 || dp.y() < 0 || dp.y() >= squareSize_*8 )
    return FPos(-1, -1);

  int x = dp.x()/squareSize_;
  int y = dp.y()/squareSize_;

  FPos fpos = turned_ ? FPos(7-x, 7-y) : FPos(x, y);
  if ( !fpos )
    return -1;

  return fpos.index();
}

bool ChessPosition::getFigureOnPt(const QPoint & pt, Figure & fig) const
{
  int pos = getPositionOnPt(pt);
  if ( pos < 0 )
    return false;

  const Field & field = board_.getField(pos);
  if ( !field )
    return false;

  fig = board_.getFigure(field.color(), field.index());
  return true;
}

bool ChessPosition::selectFigure(const QPoint & pt)
{
  if ( working_ )
    return false;

  selectedPositions_.clear();
  if ( !getFigureOnPt(pt, selectedFigure_) || selectedFigure_.getColor() != board_.getColor() )
  {
    selectedFigure_.setType(Figure::TypeNone);
    return false;
  }

  if ( !calculateSteps(steps_) )
  {
    selectedFigure_.setType(Figure::TypeNone);
    return false;
  }

  for (size_t i = 0; i < steps_.size(); ++i)
  {
    const Step & step = steps_[i];
    if ( step.index_ != selectedFigure_.getIndex() )
      continue;

    selectedSteps_.push_back(step);
  }

  if ( selectedSteps_.size() == 0 )
  {
    selectedFigure_.setType(Figure::TypeNone);
    return false;
  }

  for (size_t i = 0; i < selectedSteps_.size(); ++i)
  {
    const Step & step = selectedSteps_[i];
    selectedPositions_.insert(step.to_);
  }

  return true;
}

bool ChessPosition::makeFigureStep(const QPoint & pt)
{
  if ( working_ )
    return false;

  int pos = getPositionOnPt(pt);
  if ( pos < 0 )
  {
    clearSteps();
    return false;
  }

  std::vector<Step> steps;
  for (size_t i = 0; i < selectedSteps_.size(); ++i)
  {
    const Step & step = selectedSteps_[i];
    if ( step.to_ == pos )
      steps.push_back(step);
  }
  clearSteps();

  if ( steps.size() == 0 )
    return false;

  int idx = -1;
  if ( steps.size() == 1 )
    idx = 0;
  else
  {
    SelectFigureDlg dlg;
    Figure::Type type = Figure::TypeNone;
    if ( dlg.exec() == QDialog::Accepted )
    {
      if ( dlg.rbBishop_->isChecked() )
        type = Figure::TypeBishop;
      else if ( dlg.rbKnight_->isChecked() )
        type = Figure::TypeKnight;
      else if ( dlg.rbRook_->isChecked() )
        type = Figure::TypeRook;
      else if ( dlg.rbQueen_->isChecked() )
        type = Figure::TypeQueen;
    }
    if ( Figure::TypeNone != type )
    {
      for (size_t i = 0; i < steps.size(); ++i)
      {
        Step & step = steps[i];
        if ( step.newType_ == type )
        {
          idx = i;
          break;
        }
      }
    }
  }

  clearSteps();

  if ( idx < 0 )
    return false;

  applyStep(steps[idx]);
  return true;
}

const Figure * ChessPosition::getSelection() const
{
  if ( selectedFigure_.getType() == Figure::TypeNone || !selectedFigure_ )
    return 0;

  return &selectedFigure_;
}

void ChessPosition::clearSteps()
{
  steps_.clear();
  selectedSteps_.clear();
  selectedFigure_.clear();
  selectedPositions_.clear();
}


//////////////////////////////////////////////////////////////////////////
Board ChessPosition::getBoard() const
{
  return board_;
}

int ChessPosition::doStep(CalcResult & cres)
{
  if ( working_ )
    return 0;

  working_ = true;

  int depth = alg_.doBestStep(cres, cout);

  if ( alg_.getCurrent() )
    board_ = *alg_.getCurrent();

  if ( alg_.lastStep() )
    lastStep_ = *alg_.lastStep();
  else
    lastStep_.clear();

  working_ = false;

  return depth;
}

bool ChessPosition::applyStep(const Step & step)
{
  if ( working_ )
    return false;

  Step astep = step;
  alg_.applyStep(astep);
  if ( alg_.getCurrent() )
    board_ = *alg_.getCurrent();

  lastStep_.clear();
  if ( alg_.lastStep() )
    lastStep_ = *alg_.lastStep();

  return true;
}

int ChessPosition::stepsCount() const
{
  if ( working_ )
    return -1;

  return alg_.stepsCount();
}

void ChessPosition::stop()
{
  alg_.stop();
}

bool ChessPosition::calculateSteps(std::vector<Step> & steps)
{
  if ( working_ )
    return false;

  return alg_.calculateSteps(steps);
}

void ChessPosition::prevPos()
{
  if ( working_ )
    return;

  alg_.prevPos();
  if ( alg_.getCurrent() )
    board_ = *alg_.getCurrent();

  lastStep_.clear();
  if ( alg_.lastStep() )
    lastStep_ = *alg_.lastStep();
}

void ChessPosition::nextPos()
{
  if ( working_ )
    return;

  alg_.nextPos();
  if ( alg_.getCurrent() )
    board_ = *alg_.getCurrent();

  lastStep_.clear();
  if ( alg_.lastStep() )
    lastStep_ = *alg_.lastStep();
}

bool ChessPosition::save() const
{
  if ( working_ )
    return false;

  return doSave();
}

bool ChessPosition::load()
{
  if ( working_ )
    return false;

  if ( !doLoad() )
    return false;

  if ( alg_.getCurrent() )
    board_ = *alg_.getCurrent();

  lastStep_.clear();
  if ( alg_.lastStep() )
    lastStep_ = *alg_.lastStep();

  return true;
}

//////////////////////////////////////////////////////////////////////////
bool ChessPosition::doSave() const
{
	QDir dir(QObject::tr(".\\saved"), QObject::tr("*.pos"));
	QString fname;
	int num = 0;
	if ( dir.exists() )
	{
		dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
		dir.setSorting(QDir::Name);
		QFileInfoList list = dir.entryInfoList();
		for (int i = 0; i < list.size(); ++i)
		{
			QFileInfo fileInfo = list.at(i);
			fname = fileInfo.baseName();
		}
		if ( fname.size() > 0 )
		{
			QRegExp re(QObject::tr("(board_)(\\d+)"), Qt::CaseInsensitive);
			if ( re.indexIn(fname) != -1 )
			{
				QString str = re.cap(2);
				bool ok;
				long n = str.toLong(&ok);
				if ( ok )
					num = n;
			}
		}
	}
	else
	{
		QDir d(QObject::tr("."));
		d.mkdir(QObject::tr(".\\saved"));
	}
	QString name;
	name.sprintf( "board_%03d.pos", num + 1 );
	fname = dir.filePath(name);

	std::ofstream out(fname.toAscii());

	alg_.save(out);
	return true;
}

bool ChessPosition::doLoad()
{
	QString fname = QFileDialog::getOpenFileName(0, QObject::tr("Open position"), QObject::tr(".\\saved"), QObject::tr("Position Files (*.pos)"));
	if ( fname.size() == 0 )
		return false;

	std::ifstream in(fname.toAscii());

	if ( !in )
		return false;

	alg_.load(in);

	return true;
}
