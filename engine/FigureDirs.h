#pragma once

/*************************************************************
  FigureDirs.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Figure.h"

class FigureDir
{
	int s_dirs_[8*2*4096];

public:

	//static inline int dir(Figure::Type type, Figure::Color color, const Pos8 & dp)
	//{
	//	THROW_IF( (unsigned)type > Figure::TypeKing || (unsigned)color > 1, "try to get invalid direction" );
	//	return *(s_dirs_ + ((type<<9) | (color<<8) | dp.to_byte()));
	//}

 // static inline int dir(Figure::Type type, Figure::Color color, uint8 u)
 // {
 //   THROW_IF( (unsigned)type > Figure::TypeKing || (unsigned)color > 1, "try to get invalid direction" );
 //   return *(s_dirs_ + ((type<<9) | (color<<8) | u));
 // }

  // from 'figure' to 'p'
  inline int dir(const Figure & fig, int p) const
	{
    THROW_IF( p < 0 || p > 63, "invalid point to get direction" );
    unsigned u = (p << 6) | fig.where();
		return *(s_dirs_ + ((fig.getType()<<13) | (fig.getColor()<<12) | u));
	}

	FigureDir();

private:

	inline int & dir_ref(Figure::Type type, Figure::Color color, int idp)
	{
		THROW_IF( (unsigned)type > Figure::TypeKing || (unsigned)color > 1, "try to get invalid direction" );
		return *(s_dirs_ + ((type<<13) | (color<<12) | idp));
	}


	void calcPawnDir(int idp);
	void calcKnightDir(int idp);
	void calcBishopDir(int idp);
	void calcRookDir(int idp);
	void calcQueenDir(int idp);
	void calcKingDir(int idp);
};