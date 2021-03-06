#pragma once

/*************************************************************
  FigureDirs.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Figure.h"

class FigureDir
{
	int s_dirs_[8*2*4096];
  nst::dirs s_ddirs_[4096];

public:

  // from 'figure' with 'pos' to 'p'
  inline int dir(const Figure::Type type, const Figure::Color color, int pos, int p) const
	{
    THROW_IF( p < 0 || p > 63, "invalid points to get direction" );
    unsigned u = (p << 6) | pos;
		return *(s_dirs_ + ((type<<13) | (color<<12) | u));
	}

  // get direction 'from' one square 'to' another
  inline nst::dirs dir(int from, int to) const
  {
    THROW_IF( from < 0 || from > 63 || to < 0 || to > 63, "invaid from or to point to get direction" );
    return *(s_ddirs_ + ((to << 6) | from));
  }

	FigureDir();

private:

	inline int & dir_ref(Figure::Type type, Figure::Color color, int idp)
	{
		THROW_IF( (unsigned)type > Figure::TypeKing || (unsigned)color > 1, "try to get invalid direction" );
		return *(s_dirs_ + ((type<<13) | (color<<12) | idp));
	}

  inline nst::dirs & dir_ref(int from, int to)
  {
    THROW_IF( from < 0 || from > 63 || to < 0 || to > 63, "invaid from or to point to get direction" );
    return *(s_ddirs_ + ((to << 6) | from));
  }


	void calcPawnDir(int idp);
	void calcKnightDir(int idp);
	void calcBishopDir(int idp);
	void calcRookDir(int idp);
	void calcQueenDir(int idp);
	void calcKingDir(int idp);

  void calcDDir(int i);
};