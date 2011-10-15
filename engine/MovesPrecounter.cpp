#include "MovesPrecounter.h"

int MovesTable::s_moveIndices_[64*2*8*8*8];
uint64 MovesTable::s_eatMasks_[64*2*8];
int MovesTable::s_attackIndices[64*2*8*8];

FPos MovesTable::delta_pos_pawn_[2][4] = { { FPos(-1, -1), FPos(1, -1), FPos(0, -1), FPos(0, -2) }, { FPos(-1, 1), FPos(1, 1), FPos(0, 1), FPos(0, 2) } }; // b/w
FPos MovesTable::delta_pos_bishop_[4] = { FPos(-1, -1), FPos(-1, 1), FPos(1, 1), FPos(1, -1) };
FPos MovesTable::delta_pos_knight_[8] = { FPos(-2, 1), FPos(-1, 2), FPos(1, 2), FPos(2, 1), FPos(2, -1), FPos(1, -2), FPos(-1, -2), FPos(-2, -1) };
FPos MovesTable::delta_pos_rook_[4] = { FPos(-1, 0), FPos(0, 1), FPos(1, 0), FPos(0, -1) };
FPos MovesTable::delta_pos_queen_[8] = { FPos(-1, 0), FPos(-1, 1), FPos(0, 1), FPos(1, 1), FPos(1, 0), FPos(1, -1), FPos(0, -1), FPos(-1, -1) };
FPos MovesTable::delta_pos_king_[8] = { FPos(-1, 0), FPos(-1, 1), FPos(0, 1), FPos(1, 1), FPos(1, 0), FPos(1, -1), FPos(0, -1), FPos(-1, -1) };


namespace { MovesTable s_movesTbl_; }

MovesTable::MovesTable()
{
  for (int i = 0; i < 64*2*8*8*8; ++i)
    s_moveIndices_[i] = -1;

  for (int i = 0; i < 64*2*8*8; ++i)
	  s_attackIndices[i] = -1;

  for (int i = 0; i < 64; ++i)
  {
    calcPawn(i);
    calcBishop(i);
    calcKnight(i);
    calcRook(i);
    calcQueen(i);
    calcKing(i);

	calcPawnAttack(i);
	calcBishopAttack(i);
	calcKnightAttack(i);
	calcRookAttack(i);
	calcQueenAttack(i);
	calcKingAttack(i);

  }

  for (int i = 0; i < 64; ++i)
  {
    calcPawnEat(i);
    calcBishopEat(i);
    calcKnightEat(i);
    calcRookEat(i);
    calcQueenEat(i);
    calcKingEat(i);
  }
}

void MovesTable::calcPawn(int pos_index)
{
  FPos pos = FPosIndexer::get(pos_index);

  for (int color = 0; color < 2; ++color)
  {
    if ( 0 == pos.y() || 7 == pos.y() )
      continue;

    bool firstStep = color ? 1 == pos.y() : 6 == pos.y();
    int max_dir = firstStep ? 4 : 3;
    for (int dir = 0; dir < max_dir; ++dir)
    {
      FPos dp = delta_pos_pawn_[color][dir];
      FPos p  = pos + dp; 

      if ( !p )
        continue;

      *inds_intr(pos_index, color, Figure::TypePawn, dir) = p.index();
    }
  }
}

void MovesTable::calcBishop(int pos_index)
{
  FPos pos = FPosIndexer::get(pos_index);

  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 4; ++dir)
  {
    FPos dp = delta_pos_bishop_[dir];
    FPos p = pos + dp;
    for (int i = 0; p ; p += dp, ++i)
    {
      inds_intr(pos_index, color, Figure::TypeBishop, dir)[i] = p.index();
    }
  }
}

void MovesTable::calcKnight(int pos_index)
{
  FPos pos = FPosIndexer::get(pos_index);

  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 8; ++dir)
  {
    FPos dp = delta_pos_knight_[dir];
    FPos p = pos + dp;
    if ( !p )
      continue;
    *inds_intr(pos_index, color, Figure::TypeKnight, dir) = p.index();
  }
}

void MovesTable::calcRook(int pos_index)
{
  FPos pos = FPosIndexer::get(pos_index);

  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 4; ++dir)
  {
    FPos dp = delta_pos_rook_[dir];
    FPos p = pos + dp;
    for (int i = 0; p ; p += dp, ++i)
    {
      inds_intr(pos_index, color, Figure::TypeRook, dir)[i] = p.index();
    }
  }
}

void MovesTable::calcQueen(int pos_index)
{
  FPos pos = FPosIndexer::get(pos_index);

  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 8; ++dir)
  {
    FPos dp = delta_pos_queen_[dir];
    FPos p = pos + dp;
    for (int i = 0; p ; p += dp, ++i)
    {
      inds_intr(pos_index, color, Figure::TypeQueen, dir)[i] = p.index();
    }
  }
}

void MovesTable::calcKing(int pos_index)
{
  FPos pos = FPosIndexer::get(pos_index);

  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 8; ++dir)
  {
    FPos dp = delta_pos_king_[dir];
    FPos p = pos + dp;
    if ( !p )
      continue;

    *inds_intr(pos_index, color, Figure::TypeKing, dir) = p.index();
  }
}

//////////////////////////////////////////////////////////////////////////
// attacked square 3x3
void MovesTable::calcPawnAttack(int pidx)
{
	FPos pos = FPosIndexer::get(pidx);

	for (int color = 0; color < 2; ++color)
	{
		if ( 0 == pos.y() || 7 == pos.y() )
			continue;

		int j = 0;
		for (int dir = 0; dir < 2; ++dir)
		{
			FPos dp = delta_pos_pawn_[color][dir];
			FPos p  = pos + dp; 

			if ( !p )
				continue;

			attack(pidx, color, Figure::TypePawn)[j++] = p.index();
		}
		for ( ; j < 8; ++j)
			attack(pidx, color, Figure::TypePawn)[j] = -1;
	}
}

void MovesTable::calcBishopAttack(int pidx)
{
	FPos pos = FPosIndexer::get(pidx);

	for (int color = 0; color < 2; ++color)
	{
		int j = 0;
		for (int dir = 0; dir < 4; ++dir)
		{
			FPos p = pos + delta_pos_bishop_[dir];
			if ( !p )
				continue;
			attack(pidx, color, Figure::TypeBishop)[j++] = p.index();
		}
		for ( ; j < 8; ++j)
			attack(pidx, color, Figure::TypeBishop)[j] = -1;
	}
}

void MovesTable::calcKnightAttack(int pidx)
{
	FPos pos = FPosIndexer::get(pidx);

	for (int color = 0; color < 2; ++color)
	{
		int j = 0;
		for (int dir = 0; dir < 8; ++dir)
		{
			FPos p = pos + delta_pos_knight_[dir];
			if ( !p )
				continue;
			attack(pidx, color, Figure::TypeKnight)[j++] = p.index();
		}
		for ( ; j < 8; ++j)
			attack(pidx, color, Figure::TypeKnight)[j] = -1;
	}
}

void MovesTable::calcRookAttack(int pidx)
{
	FPos pos = FPosIndexer::get(pidx);

	for (int color = 0; color < 2; ++color)
	{
		int j = 0;
		for (int dir = 0; dir < 4; ++dir)
		{
			FPos p = pos + delta_pos_rook_[dir];
			if ( !p )
				continue;
			attack(pidx, color, Figure::TypeRook)[j++] = p.index();
		}
		for ( ; j < 8; ++j)
			attack(pidx, color, Figure::TypeRook)[j] = -1;
	}
}

void MovesTable::calcQueenAttack(int pidx)
{
	FPos pos = FPosIndexer::get(pidx);

	for (int color = 0; color < 2; ++color)
	{
		int j = 0;
		for (int dir = 0; dir < 8; ++dir)
		{
			FPos p = pos + delta_pos_queen_[dir];
			if ( !p )
				continue;
			attack(pidx, color, Figure::TypeQueen)[j++] = p.index();
		}
		for ( ; j < 8; ++j)
			attack(pidx, color, Figure::TypeQueen)[j] = -1;
	}
}

void MovesTable::calcKingAttack(int pidx)
{
	FPos pos = FPosIndexer::get(pidx);

	for (int color = 0; color < 2; ++color)
	{
		int j = 0;
		for (int dir = 0; dir < 8; ++dir)
		{
			FPos p = pos + delta_pos_king_[dir];
			if ( !p )
				continue;
			attack(pidx, color, Figure::TypeKing)[j++] = p.index();
		}
		for ( ; j < 8; ++j)
			attack(pidx, color, Figure::TypeKing)[j] = -1;
	}
}


//////////////////////////////////////////////////////////////////////////
void MovesTable::calcPawnEat(int pos_index)
{
  FPos pos = FPosIndexer::get(pos_index);

  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 2; ++dir)
  {
    FPos dp = delta_pos_pawn_[(color+1)&1][dir];
    FPos p = pos + dp; 

    if ( !p || 0 == p.y() || 7 == p.y() )
      continue;

    mask(pos_index, color, Figure::TypePawn) |= 1ULL << p.index();
  }
}

void MovesTable::calcBishopEat(int pos_index)
{
  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 4; ++dir)
  {
    const int * pidx = inds(pos_index, color, Figure::TypeBishop, dir);
    for ( ; *pidx >= 0; ++pidx)
      mask(pos_index, color, Figure::TypeBishop) |= 1ULL << *pidx;
  }
}

void MovesTable::calcKnightEat(int pos_index)
{
  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 8; ++dir)
  {
    const int * pidx = inds(pos_index, color, Figure::TypeKnight, dir);
    for ( ; *pidx >= 0; ++pidx)
      mask(pos_index, color, Figure::TypeKnight) |= 1ULL << *pidx;
  }
}

void MovesTable::calcRookEat(int pos_index)
{
  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 4; ++dir)
  {
    const int * pidx = inds(pos_index, color, Figure::TypeRook, dir);
    for ( ; *pidx >= 0; ++pidx)
      mask(pos_index, color, Figure::TypeRook) |= 1ULL << *pidx;
  }
}

void MovesTable::calcQueenEat(int pos_index)
{
  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 8; ++dir)
  {
    const int * pidx = inds(pos_index, color, Figure::TypeQueen, dir);
    for ( ; *pidx >= 0; ++pidx)
      mask(pos_index, color, Figure::TypeQueen) |= 1ULL << *pidx;
  }
}

void MovesTable::calcKingEat(int pos_index)
{
  for (int color = 0; color < 2; ++color)
  for (int dir = 0; dir < 4; ++dir)
  {
    const int * pidx = inds(pos_index, color, Figure::TypeKing, dir);
    for ( ; *pidx >= 0; ++pidx)
      mask(pos_index, color, Figure::TypeKing) |= 1ULL << *pidx;
  }
}
