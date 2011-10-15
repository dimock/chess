#include "Helpers.h"
#include "ChessBoard.h"
#include <time.h>

using namespace std;

//////////////////////////////////////////////////////////////////////////
unsigned long xorshf96()
{
  static unsigned long x= time(0) ^ clock(), y=362436069, z=521288629;

  unsigned long t;
  x ^= x << 16;
  x ^= x >> 5;
  x ^= x << 1;

  t = x;
  x = y;
  y = z;
  z = t ^ x ^ y;

  return z;
}

//////////////////////////////////////////////////////////////////////////

DebutsTable::DebutsTable() :
  current_(-1)
{
}

bool DebutsTable::readTable(const char * fname)
{
  if ( !fname )
  {
    initStatic();
    return true;
  }

  FILE * f = fopen(fname, "rt");
  if ( !f )
  {
    initStatic();
    return true;
  }

  tlines_.clear();

  char line[4096];

  for ( ; fgets(line, sizeof(line), f); )
  {
    if ( strlen(line) < 4 )
      continue;

    strlwr(line);

    tlines_.push_back(StepsList());
    StepsList & tline = tlines_.back();

    const char * sepr = ", \t\r\n";
    char * s = strtok(line, sepr);
    int i = 0;
    for (; s; s = strtok(0, sepr), ++i)
    {
      if ( strlen(s) >= 4 && isalpha(s[0]) && isdigit(s[1]) && isalpha(s[2]) && isdigit(s[3]) )
        tline.push_back(s);
    }
  }

  fclose(f);

  return true;
}

void DebutsTable::initStatic()
{
  tlines_.clear();
  for (int i = 0; i < N; ++i)
  {
    tlines_.push_back(StepsList());
    StepsList & tline = tlines_.back();

    for (int j = 0; j < M; ++j)
    {
      if ( s_table_[i][j] == 0 )
        break;

      tline.push_back(s_table_[i][j]);
    }
  }
}

StepId DebutsTable::findStep2(const std::vector<StepId> & steps, Board * board)
{
  StepId step0;
  step0.clear();

  if ( (size_t)current_ >= tlines_.size() )
    return step0;

  StepsList & tline = tlines_[current_];
  size_t n = steps.size();

  if ( n >= tline.size() )
    return step0;

  char smove[32];
  strcpy(smove, tline[n].c_str());

  StepId sid;
  if ( !strToMove(smove, board, sid) )
    return step0;

  return sid;
}

StepId DebutsTable::findStep(const std::vector<StepId> & steps, Board * board)
{
  StepId step0;
  step0.clear();

  StepsList slist;
  for (size_t i = 0; i < steps.size(); ++i)
  {
    const StepId & sid = steps[i];
    char str[16];

    if ( !moveToStrShort(sid, str) )
      return step0;

    slist.push_back(str);
  }

  vector<size_t> possible;

  for (size_t i = 0; i < tlines_.size(); ++i)
  {
    StepsList & tline = tlines_[i];
    if ( tline.size() <= slist.size() )
      continue;

    size_t j = 0;
    for (; j < slist.size(); ++j)
    {
      string & str1 = slist[j];
      string & str2 = tline[j];

      if ( str1 != str2 )
        break;
    }

    if ( j < slist.size() )
      continue;

    possible.push_back(i);
  }

  if ( !possible.size() )
    return step0;

  unsigned long x = xorshf96();
  int n = x % possible.size();
  StepsList & psteps = tlines_[possible[n]];

  char smove[32];
  strcpy(smove, psteps.at(slist.size()).c_str());

  StepId sid;
  if ( !strToMove(smove, board, sid) )
    return step0;

  return sid;
}

//////////////////////////////////////////////////////////////////////////
bool moveToStrShort(const StepId & sid, char * str)
{
	if ( sid.index_ < 0 || sid.from_ < 0 || sid.to_ < 0 )
		return false;

	Index from(sid.from_);
	Index to(sid.to_);

	str[0] = 'a' + from.x();
	str[1] = '1' + from.y();
	str[2] = 'a' + to.x();
	str[3] = '1' + to.y();
	str[4] = 0;

	if ( sid.newType_ > 0 )
	{
		switch ( sid.newType_ )
		{
		case Figure::TypeBishop:
			str[4] = 'b';
			break;

		case Figure::TypeKnight:
			str[4] = 'n';
			break;

		case Figure::TypeRook:
			str[4] = 'r';
			break;

		case Figure::TypeQueen:
			str[4] = 'q';
			break;
		}

		str[5] = 0;
	}

	return true;
}

bool moveToStr(const StepId & sid, char * str)
{
	if ( sid.index_ < 0 || sid.from_ < 0 || sid.to_ < 0 )
		return false;

	Index from(sid.from_);
	Index to(sid.to_);

	strcpy(str, "move ");

	str[5] = 'a' + from.x();
	str[6] = '1' + from.y();
	str[7] = 'a' + to.x();
	str[8] = '1' + to.y();
	str[9] = 0;

	if ( sid.newType_ > 0 )
	{
		switch ( sid.newType_ )
		{
		case Figure::TypeBishop:
			str[9] = 'b';
			break;

		case Figure::TypeKnight:
			str[9] = 'n';
			break;

		case Figure::TypeRook:
			str[9] = 'r';
			break;

		case Figure::TypeQueen:
			str[9] = 'q';
			break;
		}

		str[10] = 0;
	}

	return true;
}

bool strToMove(char * str, Board * board, StepId & sid)
{
	if ( !str || !board || strlen(str) < 4 )
		return false;

	strlwr(str);
	sid.clear();

	Figure::Color color = board->getColor();
	Figure::Color ocolor = Figure::otherColor(color);

	if ( isalpha(str[0]) && isdigit(str[1]) && isalpha(str[2]) && isdigit(str[3]) )
	{
		int xfrom = str[0] - 'a';
		int yfrom = str[1] - '1';
		int xto = str[2] - 'a';
		int yto = str[3] - '1';

		int promote = 0;
		if ( strlen(str) > 4 && isalpha(str[4]) )
		{
			if ( 'b' == str[4] )
				promote = Figure::TypeBishop;
			else if ( 'n' == str[4] )
				promote = Figure::TypeKnight;
			else if ( 'r' == str[4] )
				promote = Figure::TypeRook;
			else if ( 'q' == str[4] )
				promote = Figure::TypeQueen;
		}

		FPos fpos(xfrom, yfrom);
		Figure fig;

		if ( !board->getFigure(fpos, fig) || fig.getColor() != color )
			return false;

		sid.index_ = fig.getIndex();
		sid.ftype_ = fig.getType();
		sid.from_ = fpos.index();
		sid.to_ = FPos(xto, yto).index();
		if ( sid.to_ < 0 || sid.to_ > 63 )
			return false;

		int dx =  Index(sid.from_).x() - Index(sid.to_).x();
		if ( dx < 0 )
			dx = -dx;

		if ( fig.getType() == Figure::TypeKing && 2 == dx )
			sid.castle_ = 1;

		const Field & field = board->getField(sid.to_);
		if ( field.index() >= 0 && field.color() == ocolor )
			sid.rindex_ = field.index();
		else if ( fig.getType() == Figure::TypePawn )
		{
			Figure fake;
			if ( board->getFake(fake) && fake.where() == sid.to_ )
				sid.rindex_ = fake.getIndex();
		}

		if ( promote > 0 )
			sid.newType_ = promote;

		return true;
	}

	return false;
}

bool formatMove(StepId & sid, char * str)
{
	if ( !sid || !str )
		return false;

	Index from(sid.from_);
	Index to(sid.to_);

	if ( sid.castle_ )
	{
		int dx = to.x() - from.x();
		if ( 2 == dx )
			strcpy(str, "O-O");
		else
			strcpy(str, "O-O-O");
		return true;
	}

	int i = 0;
	switch ( sid.ftype() )
	{
	case Figure::TypePawn:
		//str[i++] = 'P';
		break;

	case Figure::TypeBishop:
		str[i++] = 'B';
		break;

	case Figure::TypeKnight:
		str[i++] = 'N';
		break;

	case Figure::TypeRook:
		str[i++] = 'R';
		break;

	case Figure::TypeQueen:
		str[i++] = 'Q';
		break;

	case Figure::TypeKing:
		str[i++] = 'K';
		break;
	}

	str[i++] = 'a' + from.x();
	str[i++] = '1' + from.y();

	if ( sid.rindex_ >= 0 )
		str[i++] = 'x';

	str[i++] = 'a' + to.x();
	str[i++] = '1' + to.y();

	if ( sid.newType_ > 0 )
	{
		switch ( sid.newType_ )
		{
		case Figure::TypeBishop:
			str[i++] = 'b';
			break;

		case Figure::TypeKnight:
			str[i++] = 'n';
			break;

		case Figure::TypeRook:
			str[i++] = 'r';
			break;

		case Figure::TypeQueen:
			str[i++] = 'q';
			break;
		}
	}

	str[i++] = 0;
	return true;
}

//////////////////////////////////////////////////////////////////////////
uint64 PawnMasks::pmasks_guarded_[2][64];
uint64 PawnMasks::pmasks_passed_[2][64];
uint64 PawnMasks::pmask_isolated_[8];
uint64 PawnMasks::pmasks_backward_[2][64];
uint64 PawnMasks::pmasks_blocked_[2][64];

PawnMasks s_pawnMask;

PawnMasks::PawnMasks()
{
  for (int color = 0; color < 2; ++color)
  {
    for (int i = 0; i < 64; ++i)
    {
      int dy = color ? -1 : +1;
      int x = i & 7;
      int y = i >> 3;

      if ( y > 0 && y < 7 )
      {
        if ( x < 7 )
          pmasks_guarded_[color][i] |= 1ULL << ((y+dy) | ((x+1) << 3));
        if ( x > 0 )
          pmasks_guarded_[color][i] |= 1ULL << ((y+dy) | ((x-1) << 3));
      }

      uint8 pm = 0;
      uint8 bm = 0;
      if ( color )
      {
        for (int j = y+1; j < 7; ++j)
          pm |= 1 << j;

        if ( y < 6 )
          bm = (1<<y) | (1<<(y+1));
      }
      else
      {
        for (int j = y-1; j > 0; --j)
          pm |= 1 << j;

        if ( y > 1 )
          bm = (1<<y) | (1<<(y-1));
      }

      uint8 * ppmask = (uint8*)&pmasks_passed_[color][i];
      uint8 * bpmask = (uint8*)&pmasks_backward_[color][i];
      uint8 * blkmask = (uint8*)&pmasks_blocked_[color][i];

      ppmask[x] = pm;
      blkmask[x] = pm;
      if ( x > 0 )
      {
        ppmask[x-1] = pm;
        bpmask[x-1] = bm;
      }
      if ( x < 7 )
      {
        ppmask[x+1] = pm;
        bpmask[x+1] = bm;
      }
    }
  }

  for (int x = 0; x < 8; ++x)
  {
    uint8 * ppmask = (uint8*)&pmask_isolated_[x];
    if ( x > 0 )
      ppmask[x-1] = 0xff;
    if ( x < 7 )
      ppmask[x+1] = 0xff;
  }
}
