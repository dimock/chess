
/*************************************************************
  ChessPos.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "ChessPos.h"
#include "MovesGenerator.h"
#include <QDir>
#include <QFileDialog>
#include <QWidget>
#include <QPainter>
#include "selfiguredlg.h"
#include <limits>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#undef LOAD_HASH

std::auto_ptr<QImage> ChessPosition::fimages_[12];

using namespace std;

ChessPosition::ChessPosition() : working_(false), turned_(false)
{
  squareSize_ = 44;
  squareDiffSize_ = squareSize_ / 2;
  borderWidth_ = 16;
  diff_margin_ = 4;
  diff_hei_ = squareDiffSize_ + diff_margin_*2;
  boardSize_ = QSize(squareSize_*8+borderWidth_*2, squareSize_*8+borderWidth_*2);
  halfmovesNumber_ = 0;
  vmove_.clear();
  player_.setMemory(256);
}

void ChessPosition::setMaxDepth(int d)
{
  player_.setMaxDepth(d);
}

bool ChessPosition::initialize(int depthMax)
{
  if ( working_)
    return false;

  player_.setMaxDepth(depthMax);

  return fromFEN(0);
}

bool ChessPosition::fromFEN(const char * fen)
{
  if ( working_)
    return false;

  if ( !player_.fromFEN(fen) )
    return false;

  vmove_.clear();
  halfmovesNumber_ = 0;
  vboard_ = player_.getBoard();

#ifdef LOAD_HASH
  player_.loadHash("hash");
  halfmovesNumber_ = player_.getBoard().halfmovesCount();
  vboard_ = player_.getBoard();
#endif

  int counter = 0;
  ScoreType a = -10000;
  CapsGenerator cg(vboard_, Figure::TypeKing, 1, player_, a, +10000, counter);
  ChecksGenerator2 ckg(vboard_, 1, player_, Figure::TypeKing);

  //int imb = vboard_.fmgr().weight();
  //if ( !vboard_.getColor() )
  //  imb = -imb;
  //Move m;
  //m.set(19, 36, 3, 0, 0);

  //int score = vboard_.see_before(imb, m);
  //int score2 = vboard_.see_before2(imb, m);

  //ScoreType s = vboard_.evaluate();

  return true;
}

bool ChessPosition::toFEN(char * fen) const
{
  if ( working_)
    return false;

  return player_.toFEN(fen);
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
  drawCurrentMoving(&painter, sz, cursorPt);
  drawMaterialDifference(&painter, sz);
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

    Index fidx = turned_ ? Index(7-x, 7-y) : Index(x, y);

    if ( vmove_.to_ == fidx )
      color = wht ? lwhite : lblack;

    if ( selectedPositions_.find(fidx) != selectedPositions_.end() )
      color = wht ? swhite : sblack;

		QRect r(p.x()+squareSize_*x, p.y()-squareSize_*y, squareSize_, squareSize_);
		painter->fillRect(r, color);

  }
}

const QImage * ChessPosition::figImage(Figure::Type t, Figure::Color c) const
{
  Figure fig(t, c, 0, 0);
  return figImage(fig);
}

const QImage * ChessPosition::figImage(const Figure & fig) const
{
  unsigned idx = fig.getColor()*6 + (fig.getType() - 1);
  if ( idx > 11 )
    return 0;

  if ( !fimages_[idx].get() )
  {
    QString imgName;
    imgName.sprintf(":/images/%s_%c.png", fig.name(), fig.getColor() ? 'w' : 'b');
    fimages_[idx].reset( new QImage(imgName) );
  }

  return fimages_[idx].get();
}

void ChessPosition::drawMaterialDifference(QPainter * painter, QSize & ) const
{
  const FiguresManager & fmgr = vboard_.fmgr();
  int diff_count[Figure::TypeKing] = {};

  diff_count[Figure::TypePawn] = fmgr.pawns(Figure::ColorWhite) - fmgr.pawns(Figure::ColorBlack);
  diff_count[Figure::TypeKnight] = fmgr.knights(Figure::ColorWhite) - fmgr.knights(Figure::ColorBlack);
  diff_count[Figure::TypeBishop] = fmgr.bishops(Figure::ColorWhite) - fmgr.bishops(Figure::ColorBlack);
  diff_count[Figure::TypeRook] = fmgr.rooks(Figure::ColorWhite) - fmgr.rooks(Figure::ColorBlack);
  diff_count[Figure::TypeQueen] = fmgr.queens(Figure::ColorWhite) - fmgr.queens(Figure::ColorBlack);

  int x0 = upleft_.x() + boardSize_.width() - borderWidth_ - squareDiffSize_,
      y0 = upleft_.y() - diff_hei_ + diff_margin_;

  std::pair<int, int> mdifferences[2][Figure::TypeKing];
  int mdiffN[2] = { 0, 0 };

  for (int t = Figure::TypePawn; t < Figure::TypeKing; ++t)
  {
    if ( !diff_count[t] )
      continue;

    int cnt = diff_count[t];
    Figure::Color c = Figure::ColorWhite;
    if ( cnt < 0 )
    {
      cnt = -cnt;
      c = Figure::ColorBlack;
    }

    mdifferences[c][mdiffN[c]++] = std::make_pair(cnt, t);
  }

  int x = x0, y = y0;

  for (int c = 0; c < 2; ++c)
  {
    for (int i = 0; i < mdiffN[c]; ++i)
    {
      int cnt = mdifferences[c][i].first;
      int t = mdifferences[c][i].second;
      const QImage * img = figImage((Figure::Type)t, (Figure::Color)c);
      if ( !img )
        continue;

      for (int j = 0; j < cnt; ++j, x -= squareDiffSize_)
      {
        QRect r(x, y, squareDiffSize_, squareDiffSize_);
        painter->drawImage(r, *img, img->rect());
      }
    }
  }
}

void ChessPosition::drawFigures(QPainter * painter, QSize & ) const
{
  QSize boardSize(squareSize_*8+borderWidth_*2, squareSize_*8+borderWidth_*2);

  for (int color = 0; color < 2; ++color)
  {
    for (int index = 0; index < Board::NumOfFigures; ++index)
    {
      Figure fig = vboard_.getFigure((Figure::Color)color, index);
      if ( !fig || fig == selectedFigure_ )
        continue;

      const QImage * img = figImage(fig);
  	  if ( !img )
  		  continue;

      QPoint p = coordByField(fig.where());
      QRect r(p.x(), p.y(), squareSize_, squareSize_);

      painter->drawImage(r, *img, img->rect());
    }
  }
}

void ChessPosition::drawCurrentMoving(QPainter * painter, QSize & , const QPoint & cursorPt) const
{
  if ( selectedFigure_.getType() == Figure::TypeNone )
    return;

  const QImage * img = figImage(selectedFigure_);
  if ( !img )
    return;

  QRect r(cursorPt.x()-squareSize_/2, cursorPt.y()-squareSize_/2, squareSize_, squareSize_);
  painter->drawImage(r, *img, img->rect());

}

QPoint ChessPosition::coordByField(int f) const
{
  QPoint orig(upleft_.x()+borderWidth_, upleft_.y()+boardSize_.height()-borderWidth_-squareSize_);
  Index index(f);
  int x = index.x();
  int y = index.y();
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
    return -1;

  int x = dp.x()/squareSize_;
  int y = dp.y()/squareSize_;

  if ( turned_ )
  {
    x = 7 - x;
    y = 7 - y;
  }

  if ( (unsigned)x > 7 || (unsigned)y > 7 )
    return -1;

  return Index(x, y);
}

bool ChessPosition::getFigureOnPt(const QPoint & pt, Figure & fig) const
{
  int pos = getPositionOnPt(pt);
  if ( pos < 0 )
    return false;

  const Field & field = vboard_.getField(pos);
  if ( !field )
    return false;

  fig = vboard_.getFigure(field.color(), field.index());
  return true;
}

bool ChessPosition::selectFigure(const QPoint & pt)
{
  if ( working_ )
    return false;

  selectedMoves_.clear();
  selectedPositions_.clear();

  Board & board = player_.getBoard();

  if ( !getFigureOnPt(pt, selectedFigure_) || selectedFigure_.getColor() != board.getColor() )
  {
    selectedFigure_.clear();
    return false;
  }

  MovesGenerator mg(board);

  if ( !mg )
  {
    selectedFigure_.setType(Figure::TypeNone);
    return false;
  }

  for ( ;; )
  {
    Move & move = mg.move();
    if ( !move )
      break;

#ifndef NDEBUG
    Board board0 = board;
#endif

    int index = board.getField(move.from_).index();
    if ( board.makeMove(move) )
    {
      if ( index == selectedFigure_.getIndex() )
      {
        selectedPositions_.insert(move.to_);
        selectedMoves_.push_back(move);
      }
    }
    board.unmakeMove();

    THROW_IF(board0 != board, "board is not restored by undo move method");
  }

  if ( selectedPositions_.size() == 0 )
  {
    selectedFigure_.setType(Figure::TypeNone);
    return false;
  }
  return true;
}

bool ChessPosition::makeMovement(const QPoint & pt)
{
  if ( working_ )
    return false;

  int pos = getPositionOnPt(pt);
  if ( pos < 0 )
  {
    clearSelected();
    return false;
  }

  std::vector<Move> moves;
  for (size_t i = 0; i < selectedMoves_.size(); ++i)
  {
    const Move & move = selectedMoves_[i];
    if ( move.to_ == pos )
      moves.push_back(move);
  }

  clearSelected();

  if ( moves.size() == 0 )
    return false;

  int idx = -1;
  if ( moves.size() == 1 )
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
      for (size_t i = 0; i < moves.size(); ++i)
      {
        Move & move = moves[i];
        if ( move.new_type_ == type )
        {
          idx = (int)i;
          break;
        }
      }
    }
  }

  if ( idx < 0 )
    return false;

  return applyMove(moves[idx]);
}

void ChessPosition::clearSelected()
{
  selectedMoves_.clear();
  selectedFigure_.clear();
  selectedPositions_.clear();
}


//////////////////////////////////////////////////////////////////////////
const Board & ChessPosition::getBoard() const
{
  return vboard_;
}

bool ChessPosition::findMove(SearchResult & sres)
{
  if ( working_ )
    return false;

  working_ = true;

  bool b = player_.findMove(sres);
  if ( b )
    b = applyMove(sres.best_);

  working_ = false;

  return b;
}

bool ChessPosition::applyMove(const Move & move)
{
  Board & board = player_.getBoard();

  if ( !board.validMove(move) )
    return false;

  if ( board.makeMove(move) )
  {
    halfmovesNumber_ = board.halfmovesCount();
    board.verifyState();
    vboard_ = board;
    vmove_  = move;
    return true;
  }
  else
  {
    board.unmakeMove();
    vmove_.clear();
    return false;
  }
}

void ChessPosition::setLastMove(const Board & board)
{
  if ( board.halfmovesCount() > 0 )
    vmove_  = board.getMove(board.halfmovesCount()-1);
  else
    vmove_.clear();
}

int ChessPosition::movesCount() const
{
  if ( working_ )
    return -1;

  const Board & board = player_.getBoard();
  return board.movesCount();
}

void ChessPosition::setTimeLimit(int ms)
{
  player_.setTimeLimit(ms);
}

void ChessPosition::pleaseStop()
{
  player_.pleaseStop();
}

void ChessPosition::undo()
{
  if ( working_ )
    return;

  Board & board = player_.getBoard();
  if ( board.halfmovesCount() > 0 )
    board.unmakeMove();

  setLastMove(board);

  vboard_ = board;
}


void ChessPosition::redo()
{
  if ( working_ )
    return;

  Board & board = player_.getBoard();
  int i = board.halfmovesCount();
  if ( i >= halfmovesNumber_ )
    return;

  const MoveCmd & move = ((const Board &)board).getMove(i);

  if ( !board.makeMove(move) )
  {
    board.unmakeMove();
    vmove_.clear();
    return;
  }

  // it could be draw or mat if there is last move
  if ( halfmovesNumber_ == board.halfmovesCount() )
    board.verifyState();

  setLastMove(board);

  vboard_ = board;
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

  return true;
}

//////////////////////////////////////////////////////////////////////////
bool ChessPosition::doSave() const
{
	QDir dir(QObject::tr(".\\games"), QObject::tr("*.pgn"));
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
			QRegExp re(QObject::tr("(game_)(\\d+)"), Qt::CaseInsensitive);
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
		d.mkdir(QObject::tr(".\\games"));
	}
	QString name;
	name.sprintf( "game_%03d.pgn", num + 1 );
	fname = dir.filePath(name);

	std::ofstream out(fname.toAscii());

  const Board & board = player_.getBoard();
  bool res = Board::save(board, out);

  return res;
}

bool ChessPosition::doLoad()
{
	QString fname = QFileDialog::getOpenFileName(0, QObject::tr("Read game in PGN format"), QObject::tr(".\\games"), QObject::tr("PGN files (*.pgn)"));
	if ( fname.size() == 0 )
		return false;

	std::ifstream in(fname.toAscii());

	if ( !in )
		return false;

  Board & board = player_.getBoard();
  bool res = Board::load(board, in);
  halfmovesNumber_ = board.halfmovesCount();
  vboard_ = board;

  setLastMove(board);

  return res;
}
