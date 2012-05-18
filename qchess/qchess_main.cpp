
/*************************************************************
  qchess_main.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <QApplication>
#include "ChessWidget.h"

int main(int argn, char * argv[])
{
	QApplication app(argn, argv);
	ChessWidget * w = new ChessWidget;
	//w->resize(500, 550);
	w->show();
	app.exec();
}