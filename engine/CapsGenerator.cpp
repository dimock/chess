#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

//////////////////////////////////////////////////////////////////////////
CapsGenerator::CapsGenerator(Board & board, Figure::Type minimalType, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), minimalType_(minimalType), player_(player), ply_(ply)
{
  numOfMoves_ = generate(alpha, betta, counter);
  captures_[numOfMoves_].clear();
}

int CapsGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = 0;
  if ( minimalType_ > Figure::TypeQueen )
    return 0;

  Figure::Color ocolor = Figure::otherColor(board_.color_);

  const uint64 exclude_mask = 0xffffffffffff00;//72057594037927680;
  uint64 oppenent_mask = board_.fmgr_.mask(ocolor) ^ board_.fmgr_.king_mask(ocolor);
  uint64 oppenent_mask_p = oppenent_mask;

  if ( minimalType_ > Figure::TypePawn)
  {
    oppenent_mask ^= board_.fmgr_.pawn_mask_o(ocolor);
    oppenent_mask_p = oppenent_mask;
  }
  if ( minimalType_ > Figure::TypeKnight )
  {
    oppenent_mask ^= board_.fmgr_.knight_mask(ocolor);
    oppenent_mask_p ^= board_.fmgr_.knight_mask(ocolor) & exclude_mask;
  }
  if ( minimalType_ > Figure::TypeBishop )
  {
    oppenent_mask ^= board_.fmgr_.bishop_mask(ocolor);
    oppenent_mask_p ^= board_.fmgr_.bishop_mask(ocolor) & exclude_mask;
  }
  if ( minimalType_ > Figure::TypeRook )
  {
    oppenent_mask ^= board_.fmgr_.rook_mask(ocolor);
    oppenent_mask_p ^= board_.fmgr_.rook_mask(ocolor) & exclude_mask;
  }

  const uint64 & black = board_.fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr_.mask(Figure::ColorWhite);
  uint64 mask_all = white | black;
  uint64 mask_all_inv = ~mask_all;
  uint64 brq_mask = board_.fmgr_.bishop_mask(board_.color_) | board_.fmgr_.rook_mask(board_.color_) | board_.fmgr_.queen_mask(board_.color_);

  // generate pawn promotions (only if < 2 checking figures)
  const uint64 & pawn_msk = board_.fmgr_.pawn_mask_o(board_.color_);
  static int pw_delta[] = { -8, 8 };

  const Figure & oking = board_.getFigure(ocolor, Board::KingIndex);

  if ( minimalType_ <= Figure::TypeQueen )
  {
    uint64 promo_msk = board_.g_movesTable->promote_o(board_.color_);
    promo_msk &= pawn_msk;

    for ( ; promo_msk; )
    {
      int from = least_bit_number(promo_msk);

      THROW_IF( (unsigned)from > 63, "invalid promoted pawn's position" );

      const Field & field = board_.getField(from);

      THROW_IF( !field || field.color() != board_.color_ || field.type() != Figure::TypePawn, "there is no pawn to promote" );

      int to = from + pw_delta[board_.color_];

      THROW_IF( (unsigned)to > 63, "pawn tries to go to invalid field" );

      if ( board_.getField(to) )
        continue;

      add_capture(m, from, to, -1, Figure::TypeQueen);
    }
  }

  // firstly check if have at least 1 attacking pawn
  bool pawns_eat = false;

  uint64 pawn_eat_msk = 0;
  if ( board_.color_ )
    pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
  else
    pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);

  pawns_eat = (pawn_eat_msk & oppenent_mask_p) != 0;

  if ( !pawns_eat && board_.en_passant_ >= 0 && (minimalType_ <= Figure::TypePawn) )
  {
    const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
    THROW_IF( !epawn, "there is no en passant pawn" );

    int to = epawn.where() + pw_delta[board_.color_];
    THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

    pawns_eat = (pawn_eat_msk & (1ULL << to)) != 0;
  }

  // generate captures
  for (int i = 0; i < Board::NumOfFigures; ++i)
  {
    const Figure & fig = board_.getFigure(board_.color_, i);
    if ( !fig || (fig.getType() == Figure::TypePawn && !pawns_eat) )
      continue;

  	if ( fig.getType() == Figure::TypePawn )
    {
      uint64 p_caps = board_.g_movesTable->pawnCaps_o(board_.color_, fig.where()) & oppenent_mask_p;

      for ( ; p_caps; )
      {
        THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

        int to = least_bit_number(p_caps);

        bool promotion = to > 55 || to < 8; // 1st || last line

        THROW_IF( (unsigned)to > 63, "invalid pawn's capture position" );

        const Field & field = board_.getField(to);
        if ( !field || field.color() != ocolor || (field.type() < minimalType_ && !promotion) )
          continue;

        add_capture(m, fig.where(), to, field.index(), promotion ? Figure::TypeQueen : 0);
      }

      if ( board_.en_passant_ >= 0 && minimalType_ <= Figure::TypePawn )
      {
        const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
        THROW_IF( !epawn, "there is no en passant pawn" );

        int to = epawn.where() + pw_delta[board_.color_];
        THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

        int dir = board_.g_figureDir->dir(fig, to);
        if ( 0 == dir || 1 == dir )
        {
          THROW_IF( !pawns_eat, "have pawns capture, but not detected by mask" );

          add_capture(m, fig.where(), to, board_.en_passant_, 0);
        }
      }
    }
    else if ( fig.getType() == Figure::TypeKnight || fig.getType() == Figure::TypeKing )
    {
      // don't need to verify capture possibility by mask
      uint64 f_caps = board_.g_movesTable->caps(fig.getType(), fig.where()) & oppenent_mask;
      for ( ; f_caps; )
      {
        int to = least_bit_number(f_caps);

        THROW_IF( (unsigned)to > 63, "invalid field index while capture" );

        const Field & field = board_.getField(to);

        THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

        THROW_IF( field.type() < minimalType_, "try to capture figure with score lower than required" );

        add_capture(m, fig.where(), to, field.index(), 0);
      }
    }
    else // other figures
    {
      uint64 f_caps = board_.g_movesTable->caps(fig.getType(), fig.where()) & oppenent_mask;
      for ( ; f_caps; )
      {
        int8 to = least_bit_number(f_caps);

        const Field & field = board_.getField(to);
        THROW_IF( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

        THROW_IF( field.type() < minimalType_, "try to capture figure " );

        // can't go here
        const uint64 & btw_msk = board_.g_betweenMasks->between(fig.where(), to);
        if ( (btw_msk & mask_all_inv) != btw_msk )
          continue;

        add_capture(m, fig.where(), to, field.index(), 0);
      }
    }

	// only king's movements are available
	if ( board_.checkingNum_ > 1 )
		break;
  }

  return m;
}
