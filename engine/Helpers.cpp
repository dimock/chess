/*************************************************************
  Helpers.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Helpers.h"
#include "Board.h"
#include "MovesGenerator.h"
#include <time.h>

using namespace std;


//////////////////////////////////////////////////////////////////////////
int8 BitsCounter::s_array_[256] =
{
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

//////////////////////////////////////////////////////////////////////////
PawnMasks::PawnMasks()
{
  for (int pos = 0; pos < 64; ++pos)
    clearAll(pos);

  for (int i = 0; i < 8; ++i)
    pmask_isolated_[i] = 0;


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
      uint8 * blkmask = (uint8*)&pmasks_blocked_[color][i];

      ppmask[x] = pm;
      blkmask[x] = pm;
      if ( x > 0 )
      {
        ppmask[x-1] = pm;
      }
      if ( x < 7 )
      {
        ppmask[x+1] = pm;
      }

      uint64 & kpk_mask = pmask_kpk_[color][i];
      int x0 = x > 0 ? x-1 : 0;
      int x1 = x < 7 ? x+1 : 7;
      if ( color )
      {
        for (int j = y+1; j < 8; ++j)
        {
          for (int l = x0; l <= x1; ++l)
          {
            int kp = l | (j<<3);
            kpk_mask |= 1ULL << kp;
          }
        }
      }
      else
      {
        for (int j = y-1; j >= 0; --j)
        {
          for (int l = x0; l <= x1; ++l)
          {
            int kp = l | (j<<3);
            kpk_mask |= 1ULL << kp;
          }
        }
      }
    }
  }

  for (int i = 0; i < 64; ++i)
  {
	  int x = i & 7;
	  int y = i >> 3;

	  uint8 * bkmask = (uint8*)&pmasks_backward_[i];

	  if ( x > 0 )
	  {
		  bkmask[x-1] |= 1 << y;
		  if ( y > 0 )
			  bkmask[x-1] |= 1 << (y-1);
		  if ( y < 7 )
			  bkmask[x-1] |= 1 << (y+1);
	  }

	  if ( x < 7 )
	  {
		  bkmask[x+1] |= 1 << y;
		  if ( y > 0 )
			  bkmask[x+1] |= 1 << (y-1);
		  if ( y < 7 )
			  bkmask[x+1] |= 1 << (y+1);
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

  // destination color
  for (int color = 0; color < 2; ++color)
  {
    for (int i = 0; i < 64; ++i)
    {
      int x = i & 7;
      int8 dst_color = 0;
      if ( color ) // white
        dst_color = (x+1) & 1;
      else
        dst_color = x & 1;
      pawn_dst_color_[color][i] = dst_color;
    }
  }
}

void PawnMasks::clearAll(int pos)
{
  for (int color = 0; color < 2; ++color)
  {
    pmasks_guarded_[color][pos] = 0;
    pmasks_passed_[color][pos] = 0;
    pmasks_blocked_[color][pos] = 0;
    pmask_kpk_[color][pos] = 0;
  }
  pmasks_backward_[pos] = 0;
}

//////////////////////////////////////////////////////////////////////////
DeltaPosCounter::DeltaPosCounter()
{
  for (int i = 0; i < 4096; ++i)
  {
    FPos dp = FPosIndexer::get(i >> 6) - FPosIndexer::get(i & 63);
    s_array_[i] = deltaP(dp);
  }
}

//////////////////////////////////////////////////////////////////////////
BetweenMask::BetweenMask(DeltaPosCounter * deltaPoscounter)
{
  for (int i = 0; i < 64; ++i)
  {
    for (int j = 0; j < 64; ++j)
    {
      // clear it first
      s_between_[i][j] = 0;
      s_from_[i][j] = 0;

      ///  to <- from
      FPos dp = deltaPoscounter->getDeltaPos(j, i);
      if ( FPos(0, 0) == dp )
        continue;

      {
        FPos p = FPosIndexer::get(i) + dp;
        FPos q = FPosIndexer::get(j);

        for ( ; p && p != q; p += dp)
          s_between_[i][j] |= 1ULL << p.index();
      }

      {
        FPos p = FPosIndexer::get(i) + dp;

        for ( ; p; p += dp)
          s_from_[i][j] |= 1ULL << p.index();
      }
    }

    for (int j = 0; j < 8; ++j)
    {
      s_from_dir_[j][i] = 0;

      FPos dp;
      switch ( j+1 )
      {
      case nst::nw:
        dp = FPos(-1, 1);
        break;

      case nst::no:
        dp = FPos(0, 1);
        break;

      case nst::ne:
        dp = FPos(1, 1);
        break;

      case nst::ea:
        dp = FPos(1, 0);
        break;

      case nst::se:
        dp = FPos(1, -1);
        break;

      case nst::so:
        dp = FPos(0, -1);
        break;

      case nst::sw:
        dp = FPos(-1, -1);
        break;

      case nst::we:
        dp = FPos(-1, 0);
        break;
      }

      FPos p = FPosIndexer::get(i) + dp;
      for ( ; p; p += dp)
        s_from_dir_[j][i] |= 1ULL << p.index();
    }
  }
}

//////////////////////////////////////////////////////////////////////////
DistanceCounter::DistanceCounter()
{
  for (int i = 0; i < 4096; ++i)
  {
    FPos dp = FPosIndexer::get(i >> 6) - FPosIndexer::get(i & 63);
    s_array_[i] = dist_dP(dp);
  }
}

//////////////////////////////////////////////////////////////////////////

bool iscolumn(char c)
{
  return c >= 'a' && c <= 'h';
}

bool parseSAN(Board & board, const char * str, Move & move)
{
  if ( !str )
    return false;

  Figure::Type type = Figure::TypePawn;
  Figure::Type new_type = Figure::TypeNone;

  int xfrom = -1, yfrom = -1;
  int from  = -1, to = -1;
  bool capture = false;
  bool check = false;
  bool chessmat = false;

  const char * s = str;

  if ( strchr("PNBRQK", *s) )
  {
    type = toFtype(*s);
    s++;
  }
  else if ( strstr(s, "O-O-O") ) // long castle
  {
    from = board.getColor() ? 4 : 60;
    to   = board.getColor() ? 2 : 58;
    s += 5;
    type = Figure::TypeKing;
  }
  else if ( strstr(s, "O-O") ) // short castle
  {
    from = board.getColor() ? 4 : 60;
    to   = board.getColor() ? 6 : 62;
    s += 3;
    type = Figure::TypeKing;
  }

  if ( to < 0 ) // not found yet
  {
    // should be at least 2 chars
    size_t n = strlen(s);
    if ( n < 2 )
      return false;

    if ( isdigit(s[0]) && (iscolumn(s[1]) || 'x' == s[1]) ) // from row number
    {
      yfrom = s[0] - '1';
      s++;
    }
    else if ( iscolumn(s[0]) && (iscolumn(s[1]) || 'x' == s[1]) ) // from column number
    {
      xfrom = s[0] - 'a';
      s++;
    }
    else if ( n > 2 && iscolumn(s[0]) && isdigit(s[1]) && (iscolumn(s[2]) || 'x' == s[2]) ) // exact from point
    {
      xfrom = s[0] - 'a';
      yfrom = s[1] - '1';
      s += 2;
    }
    
    if ( 'x' == s[0] ) // capture
    {
      capture = true;
      s++;
    }

    n = strlen(s);
    if ( !*s || !iscolumn(s[0]) || n < 2 )
      return false;

    to = (s[0] - 'a') | ((s[1] - '1') << 3);
    s += 2;
    n = strlen(s);

    if ( '=' == s[0] )
    {
      if ( n < 2 )
        return false;
      new_type = toFtype(s[1]);
      if ( new_type < Figure::TypeKnight || new_type > Figure::TypeQueen )
        return false;
      s += 2;
      n = strlen(s);
    }

    if ( '+' == s[0] )
      check = true;
    else if ( '#' == s[0] )
      chessmat = true;
  }

  if ( to < 0 )
    return false;

  if ( xfrom >= 0 && yfrom >= 0 )
    from = xfrom | (yfrom << 3);

  MovesGenerator mg(board);
  for ( ;; )
  {
    const Move & m = mg.move();
    if ( !m )
      break;

    bool valid = false;
    if ( board.makeMove(m) )
      valid = true;
    board.unmakeMove();

    if ( !valid )
      continue;

    const Field & field = board.getField(m.from_);
    if ( to == m.to_ && m.new_type_ == new_type && field.type() == type && ((m.rindex_ >= 0) == capture) &&
        (from > 0 && from == m.from_ || xfrom >= 0 && (m.from_ & 7) == xfrom || yfrom >= 0 && (m.from_ >>3) == yfrom || xfrom < 0 && yfrom < 0))
    {
      move = m;
      return true;
    }
  }
  return false;
}

bool printSAN(Board & board, const Move & move, char * str)
{
  Field field = board.getField(move.from_);
  Board::State state = Board::Invalid;

  bool found = false;
  int disambiguations = 0;
  int same_x = 0, same_y = 0;
  int xfrom = move.from_ & 7;
  int yfrom = move.from_ >>3;
  int xto = move.to_ & 7;
  int yto = move.to_ >>3;

#ifndef NDEBUG
  Board board0(board);
#endif

  MovesGenerator mg(board);
  for ( ;; )
  {
    const Move & m = mg.move();
    if ( !m )
      break;

    bool valid = false;
    if ( board.makeMove(m) )
      valid = true;

    if ( m == move )
    {
      THROW_IF(!valid, "invalid move given to printSAN");
      board.verifyState();
      state = board.getState();
      found = true;
    }

    board.unmakeMove();

    THROW_IF(board0 != board, "board is not restored by undo move method");

    const Field & f = board.getField(m.from_);

    if ( !valid || m.to_ != move.to_ || f.type() != field.type() || m.new_type_ != move.new_type_ )
      continue;

    // check for disambiguation in 'from' position
    if ( (m.from_ & 7) == xfrom )
      same_x++;
    
    if ( (m.from_ >>3) == yfrom )
      same_y++;

    disambiguations++;
  }

  if ( !found )
    return false;

  char * s = str;
  if ( field.type() == Figure::TypeKing && (2 == move.to_ - move.from_ || -2 == move.to_ - move.from_) )// castle
  {
    if ( move.to_ > move.from_ ) // short castle
    {
      strcpy(s, "O-O");
      s += 3;
    }
    else
    {
      strcpy(s, "O-O-O");
      s += 5;
    }
  }
  else
  {
    if ( field.type() != Figure::TypePawn )
    {
      *s = fromFtype(field.type());
      ++s;
    }
    
    // Arena doesn't understand pawn's capture, even if other movies are illegal
    if ( disambiguations > 1 || (field.type() == Figure::TypePawn && move.rindex_ >= 0) )
    {
      if ( same_x <= 1 )
      {
        // x different
        *s = 'a' + xfrom;
        ++s;
      }
      else if ( same_y <= 1 )
      {
        // y different
        *s = '1' + yfrom;
        ++s;
      }
      else
      {
        // write both
        *s = 'a' + xfrom;
        s++;
        *s = '1' + yfrom;
        s++;
      }
    }
    // capture
    if ( move.rindex_ >= 0 )
    {
      *s = 'x';
      ++s;
    }

    *s = 'a' + xto;
    ++s;

    *s = '1' + yto;
    ++s;

    if ( move.new_type_ > 0 )
    {
      *s = '=';
      ++s;

      *s = fromFtype((Figure::Type)move.new_type_);
      ++s;
    }
  }

  if ( Board::UnderCheck == state )
  {
    *s = '+';
    ++s;
  }
  else if ( Board::ChessMat == state )
  {
    *s = '#';
    ++s;
  }

  *s = 0;

  return true;
}
//////////////////////////////////////////////////////////////////////////

bool moveToStr(const Move & move, char * str, bool full)
{
	if ( move.from_ < 0 || move.to_ < 0 )
		return false;

	Index from(move.from_);
	Index to(move.to_);

  size_t n = 0;
  if ( full )
  {
    strcpy(str, "move ");
    n = strlen(str);
  }

	str[n++] = 'a' + from.x();
	str[n++] = '1' + from.y();
	str[n++] = 'a' + to.x();
	str[n++] = '1' + to.y();
	str[n] = 0;

	if ( move.new_type_ <= 0 )
    return true;

	switch ( move.new_type_ )
	{
	case Figure::TypeBishop:
		str[n++] = 'b';
		break;

	case Figure::TypeKnight:
		str[n++] = 'n';
		break;

	case Figure::TypeRook:
		str[n++] = 'r';
		break;

	case Figure::TypeQueen:
		str[n++] = 'q';
		break;
	}

	str[n] = 0;

	return true;
}

bool strToMove(char * str, const Board & board, Move & move)
{
	if ( !str || strlen(str) < 4 )
		return false;

	_strlwr(str);
	move.clear();

	Figure::Color color = board.getColor();
	Figure::Color ocolor = Figure::otherColor(color);

	if ( !iscolumn(str[0]) || !isdigit(str[1]) && !iscolumn(str[2]) && !isdigit(str[3]) )
    return false;

	int xfrom = str[0] - 'a';
	int yfrom = str[1] - '1';
	int xto   = str[2] - 'a';
	int yto   = str[3] - '1';

	if ( strlen(str) > 4 && isalpha(str[4]) )
	{
		if ( 'b' == str[4] )
			move.new_type_ = Figure::TypeBishop;
		else if ( 'n' == str[4] )
			move.new_type_ = Figure::TypeKnight;
		else if ( 'r' == str[4] )
			move.new_type_ = Figure::TypeRook;
		else if ( 'q' == str[4] )
			move.new_type_ = Figure::TypeQueen;
	}

  move.from_ = Index(xfrom, yfrom);
  move.to_   = Index(xto, yto);

  int to = move.to_;

  const Field & ffrom = board.getField(move.from_);
  if ( !ffrom || ffrom.color() != color )
    return false;

  // maybe en-passant
  if ( ffrom.type() == Figure::TypePawn && board.getEnPassant() >= 0 )
  {
    int dy = (move.to_ >>3) - (move.from_ >>3);
    int dx = (move.to_ & 7) - (move.from_ & 7);
    if ( dx != 0 )
    {
      int yto = (move.to_ >> 3) - dy;
      int epos = (move.to_ & 7) | (yto << 3);
      const Figure & epawn = board.getFigure(ocolor, board.getEnPassant());
      if ( epawn.where() == epos )
        to = epos;
    }
  }

  const Field & fto = board.getField(to);
  if ( fto && fto.color() == ocolor )
    move.rindex_ = fto.index();

	if ( !board.validMove(move) )
		return false;

	return true;
}
