#include "FigureDirs.h"


int FigureDir::s_dirs_[8*2*4096];

namespace { FigureDir s_figureDir; }

FigureDir::FigureDir()
{
	for (int i = 0; i < 8*2*4096; ++i)
		s_dirs_[i] = -1;

	for (int i = 0; i < 4096; ++i)
	{
		calcPawnDir(i);
		calcKnightDir(i);
		calcBishopDir(i);
		calcRookDir(i);
		calcQueenDir(i);
		calcKingDir(i);
	}
}

void FigureDir::calcPawnDir(int idp)
{
  FPos dp(FPosIndexer::get(idp >> 6) - FPosIndexer::get(idp & 63));
	for (int color = 0; color < 2; ++color)
	{
		int step_y = Figure::ColorWhite == color ? 1 : -1;
		int d = -1;

		if ( dp.x() == -1 && dp.y() == step_y )
			d = 0;
		else if ( dp.x() == 1 && dp.y() == step_y )
			d = 1;
		else if ( dp.x() == 0 && dp.y() == step_y )
			d = 2;
		else if ( dp.x() == 0 && dp.y() == (step_y<<1) )
			d = 3;

		dir_ref(Figure::TypePawn, (Figure::Color)color, idp) = d;
	}
}

void FigureDir::calcKnightDir(int idp)
{
  FPos dp(FPosIndexer::get(idp >> 6) - FPosIndexer::get(idp & 63));

	int d = -1;

	if ( dp.x() == -2 && dp.y()  == 1 )
		d = 0;
	else if ( dp.x() == -1 && dp.y() == 2 )
		d = 1;
	else if ( dp.x() == 1 && dp.y() == 2 )
		d = 2;
	else if ( dp.x() == 2 && dp.y() == 1 )
		d = 3;
	else if ( dp.x() == 2 && dp.y() == -1 )
		d = 4;
	else if ( dp.x() == 1 && dp.y() == -2 )
		d = 5;
	else if ( dp.x() == -1 && dp.y() == -2 )
		d = 6;
	else if ( dp.x() == -2 && dp.y() == -1 )
		d = 7;

	for (int color = 0; color < 2; ++color)
	{
		dir_ref(Figure::TypeKnight, (Figure::Color)color, idp) = d;
	}
}

void FigureDir::calcBishopDir(int idp)
{
  FPos dp(FPosIndexer::get(idp >> 6) - FPosIndexer::get(idp & 63));

	if ( dp.x() == 0 || dp.y() == 0 )
		return;

	int d = -1;

	if ( dp.x() < 0 && dp.y() < 0 )
		d = 0;
	else if ( dp.x() < 0 && dp.y() > 0 )
		d = 1;
	else if ( dp.x() > 0 && dp.y() > 0 )
		d = 2;
	else if ( dp.x() > 0 && dp.y() < 0 )
		d = 3;

	int x = dp.x(), y = dp.y();
	if ( x < 0 )
		x = -x;
	if ( y < 0 )
		y = -y;

	if ( x != y )
		return;

	for (int color = 0; color < 2; ++color)
	{
		dir_ref(Figure::TypeBishop, (Figure::Color)color, idp) = d;
	}

}

void FigureDir::calcRookDir(int idp)
{
  FPos dp(FPosIndexer::get(idp >> 6) - FPosIndexer::get(idp & 63));

	int d = -1;

	if ( dp.x() < 0 && dp.y() == 0 )
		d = 0;
	else if ( dp.x() == 0 && dp.y() > 0 )
		d = 1;
	else if ( dp.x() > 0 && dp.y() == 0 )
		d = 2;
	else if ( dp.x() == 0 && dp.y() < 0 )
		d = 3;

	for (int color = 0; color < 2; ++color)
	{
		dir_ref(Figure::TypeRook, (Figure::Color)color, idp) = d;
	}
}

void FigureDir::calcQueenDir(int idp)
{
  FPos dp(FPosIndexer::get(idp >> 6) - FPosIndexer::get(idp & 63));

	if ( dp.x() == 0 && dp.y() == 0 )
		return;

	int d = -1;

	int x = dp.x(), y = dp.y();
	if ( x < 0 )
		x = -x;
	if ( y < 0 )
		y = -y;

	if ( (x != y && y != 0 && x != 0 ) )
		return;

	if ( dp.x() < 0 && dp.y() == 0 )
		d = 0;
	else if ( dp.x() < 0 && dp.y() > 0 )
		d = 1;
	else if ( dp.x() == 0 && dp.y() > 0 )
		d = 2;
	else if ( dp.x() > 0 && dp.y() > 0 )
		d = 3;
	else if ( dp.x() > 0 && dp.y() == 0 )
		d = 4;
	else if ( dp.x() > 0 && dp.y() < 0 )
		d = 5;
	else if ( dp.x() == 0 && dp.y() < 0 )
		d = 6;
	else if ( dp.x() < 0 && dp.y() < 0 )
		d = 7;

	for (int color = 0; color < 2; ++color)
	{
		dir_ref(Figure::TypeQueen, (Figure::Color)color, idp) = d;
	}
}

void FigureDir::calcKingDir(int idp)
{
  FPos dp(FPosIndexer::get(idp >> 6) - FPosIndexer::get(idp & 63));

	int d = -1;

	if ( dp.x() == -1 && dp.y() == 0 )
		d = 0;
	if ( dp.x() == -1 && dp.y() == 1 )
		d = 1;
	if ( dp.x() == 0 && dp.y() == 1 )
		d = 2;
	if ( dp.x() == 1 && dp.y() == 1 )
		d = 3;
	if ( dp.x() == 1 && dp.y() == 0 )
		d = 4;
	if ( dp.x() == 1 && dp.y() == -1 )
		d = 5;
	if ( dp.x() == 0 && dp.y() == -1 )
		d = 6;
	if ( dp.x() == -1 && dp.y() == -1)
		d = 7;
	if ( dp.x() == -2 && dp.y() == 0 )
		d = 8;
	if ( dp.x() == 2 && dp.y() == 0 )
		d = 9;

	for (int color = 0; color < 2; ++color)
	{
		dir_ref(Figure::TypeKing, (Figure::Color)color, idp) = d;
	}
}
