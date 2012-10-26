#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

//////////////////////////////////////////////////////////////////////////

History MovesGeneratorBase::history_[64][64];
/*************************************************************
  MovesGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

unsigned History::history_max_;


void MovesGeneratorBase::clear_history()
{
  for (int i = 0; i < 64; ++i)
    for (int j = 0; j < 64; ++j)
      history_[i][j].clear();
  History::history_max_ = 0;
}

void MovesGeneratorBase::normalize_history(int n)
{
  History::history_max_ = 0;
  for (int i = 0; i < 64; ++i)
  {
    for (int j = 0; j < 64; ++j)
    {
      History & hist = history_[i][j];
      hist.normalize(n);
      if ( hist.score_ > History::history_max_ )
        History::history_max_ = hist.score_;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
bool MovesGeneratorBase::find(const Move & m) const
{
  for (int i = 0; i < numOfMoves_; ++i)
  {
    const Move & move = moves_[i];
    if ( m == move )
      return true;
  }
  return false;
}

bool MovesGeneratorBase::has_duplicates() const
{
  for (int i = 0; i < numOfMoves_; ++i)
  {
    for (int j = i+1; j < numOfMoves_; ++j)
    {
      if ( moves_[i] == moves_[j] )
        return true;
    }
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
MovesGenerator::MovesGenerator(Board & board, const Move & killer) :
  MovesGeneratorBase(board), killer_(killer)
{
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();
}

MovesGenerator::MovesGenerator(Board & board) :
  MovesGeneratorBase(board)
{
  killer_.clear();
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();
}
//////////////////////////////////////////////////////////////////////////
int MovesGenerator::generate()
{
  int m = 0;
  const Figure::Color & color = board_.color_;
  const Figure::Color ocolor = Figure::otherColor(color);

  if ( board_.checkingNum_ < 2 )
  {
    // pawns movements
    if ( board_.fmgr().pawn_mask_o(color) )
    {
      BitMask pw_mask = board_.fmgr().pawn_mask_o(color);
      for ( ; pw_mask; )
      {
        int pw_pos = clear_lsb(pw_mask);

        const int8 * table = board_.g_movesTable->pawn(color, pw_pos);

        for (int i = 0; i < 2; ++i, ++table)
        {
          if ( *table < 0 )
            continue;

          const Field & field = board_.getField(*table);
          bool capture = false;
          if ( (field && field.color() == ocolor) ||
               (board_.en_passant_ >= 0 && *table == board_.en_passant_) )
          {
            capture = true;
          }

          if ( !capture )
            continue;

          bool promotion = *table > 55 || *table < 8;

          Move & move = moves_[m++];
          move.alreadyDone_ = 0;
          move.set(pw_pos, *table, Figure::TypeNone, capture);
          calculateSortValue(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeRook;
            calculateSortValue(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeBishop;
            calculateSortValue(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeKnight;
            calculateSortValue(moves_[m++]);
          }
        }

        for (; *table >= 0 && !board_.getField(*table); ++table)
        {
          bool promotion = *table > 55 || *table < 8;

          Move & move = moves_[m++];

          move.alreadyDone_ = 0;
          move.set(pw_pos, *table, Figure::TypeNone, false);
          calculateSortValue(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeRook;
            calculateSortValue(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeBishop;
            calculateSortValue(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeKnight;
            calculateSortValue(moves_[m++]);
          }
        }
      }
    }

    // knights movements
    if ( board_.fmgr().knight_mask(color) )
    {
      BitMask kn_mask = board_.fmgr().knight_mask(color);
      for ( ; kn_mask; )
      {
        int kn_pos = clear_lsb(kn_mask);

        const int8 * table = board_.g_movesTable->knight(kn_pos);

        for (; *table >= 0; ++table)
        {
          const Field & field = board_.getField(*table);
          bool capture = false;
          if ( field )
          {
            if ( field.color() == color )
              continue;

            capture = true;
          }

          add(m, kn_pos, *table, Figure::TypeNone, capture);
        }
      }
    }

    // bishops, rooks and queens movements
    for (int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
    {
      BitMask fg_mask = board_.fmgr().type_mask((Figure::Type)type, color);

      for ( ; fg_mask; )
      {
        int fg_pos = clear_lsb(fg_mask);

        const uint16 * table = board_.g_movesTable->move(type-Figure::TypeBishop, fg_pos);

        for (; *table; ++table)
        {
          const int8 * packed = reinterpret_cast<const int8*>(table);
          int8 count = packed[0];
          int8 delta = packed[1];

          int8 p = fg_pos;
          bool capture = false;
          for ( ; count && !capture; --count)
          {
            p += delta;

            const Field & field = board_.getField(p);
            if ( field )
            {
              if ( field.color() == color )
                break;

              capture = true;
            }

            add(m, fg_pos, p, Figure::TypeNone, capture);
          }
        }
      }
    }
  }

  // kings movements
  {
    BitMask ki_mask = board_.fmgr().king_mask(color);
    
    THROW_IF( ki_mask == 0, "invalid position - no king" );

    int ki_pos = clear_lsb(ki_mask);

    const int8 * table = board_.g_movesTable->king(ki_pos);

    for (; *table >= 0; ++table)
    {
      const Field & field = board_.getField(*table);
      bool capture = false;
      if ( field )
      {
        if ( field.color() == color )
          continue;

        capture = true;
      }

      add(m, ki_pos, *table, Figure::TypeNone, capture);
    }

    if ( !board_.underCheck() )
    {
      // short castle
      if ( board_.castling(board_.color_, 0) )
        add(m, ki_pos, ki_pos+2, Figure::TypeNone, false);

      // long castle
      if ( board_.castling(board_.color_, 1) )
        add(m, ki_pos, ki_pos-2, Figure::TypeNone, false);
    }
  }

  return m;
}


//////////////////////////////////////////////////////////////////////////
EscapeGenerator::EscapeGenerator(const Move & hmove, Board & board) :
  MovesGeneratorBase(board), hmove_(hmove)
{
  numOfMoves_ = push_pv();
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();
}

int EscapeGenerator::push_pv()
{
  int m = 0;
  if ( hmove_ && board_.validMove(hmove_) && board_.isMoveValidUnderCheck(hmove_) )
  {
    Move & move = moves_[m];
    move = hmove_;
    move.checkVerified_ = 1;
    move.alreadyDone_ = 0;
    ++m;
  }
  else
    hmove_.clear();

  return m;
}

int EscapeGenerator::generate()
{
  if ( board_.checkingNum_ == 1 )
    return generateUsual();
  else
    return generateKingonly(numOfMoves_);
}

int EscapeGenerator::generateUsual()
{
  int m = numOfMoves_;
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);

  THROW_IF( (unsigned)board_.checking_[0] > 63, "there is no checking figure" );

  // checking figure position and type
  const int8 & ch_pos = board_.checking_[0];
  Figure::Type ch_type = board_.getField(ch_pos).type();

  THROW_IF( !ch_type, "there is no checking figure" );
  THROW_IF( ch_type == Figure::TypeKing, "king is attacking king" );

  const uint64 & black = board_.fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr_.mask(Figure::ColorWhite);
  const uint64 & pawn_msk = board_.fmgr_.pawn_mask_o(board_.color_);
  uint64 mask_all_inv = ~(white | black);

  static int pw_delta[] = { -8, 8 };

  // check if there is at least 1 pawns capture
  bool pawns_eat = false;
  bool ep_capture = false;

  {
    uint64 pawn_eat_msk = 0;
    if ( board_.color_ )
      pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
    else
      pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);

    // mask of attacking figure
    uint64 checking_fig_mask = 1ULL << ch_pos;

    pawns_eat = (pawn_eat_msk & checking_fig_mask) != 0;

    if ( ch_type == Figure::TypePawn && board_.en_passant_ >= 0 )
    {
      int ep_pos = board_.enpassantPos();
      THROW_IF( !board_.getField(ep_pos), "en-passant pown doesnt exist" );

      ep_capture = (ch_pos == ep_pos) && (pawn_eat_msk & (1ULL << board_.en_passant_));
    }

    pawns_eat = pawns_eat || ep_capture;
  }

  // 1st - get pawn's captures
  if ( pawns_eat )
  {
    // en-passant
    if ( ep_capture )
    {
      int8 to = board_.enpassantPos();
      const uint64 & opawn_caps_ep = board_.g_movesTable->pawnCaps_o(ocolor, to);
      uint64 eat_msk_ep = pawn_msk & opawn_caps_ep;

      for ( ; eat_msk_ep; )
      {
        int n = clear_lsb(eat_msk_ep);
        const Field & fpawn = board_.getField(n);
        THROW_IF( !fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != board_.color_, "no pawn on field we are going to do capture from" );
        add(m, n, to, Figure::TypeNone, true);
      }
    }


    const uint64 & opawn_caps = board_.g_movesTable->pawnCaps_o(ocolor, ch_pos);
    uint64 eat_msk = pawn_msk & opawn_caps;

    bool promotion = ch_pos > 55 || ch_pos < 8; // 1st || last line

    for ( ; eat_msk; )
    {
      int n = clear_lsb(eat_msk);

      const Field & fpawn = board_.getField(n);

      THROW_IF( !fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != board_.color_, "no pawn on field we are going to do capture from" );

      if ( add(m, n, ch_pos, promotion ? Figure::TypeQueen : Figure::TypeNone, true) && promotion )
      {
        // firstly decrease m because it was increased in add()
        Move & move = moves_[--m];

        // increase m before take move
        moves_[++m] = move;
        moves_[m++].new_type_ = Figure::TypeRook;

        moves_[m] = move;
        moves_[m++].new_type_ = Figure::TypeBishop;

        moves_[m] = move;
        moves_[m++].new_type_ = Figure::TypeKnight;
      }
    }
  }

  // 2nd - knight's captures
  {
    const uint64 & knight_caps = board_.g_movesTable->caps(Figure::TypeKnight, ch_pos);
    const uint64 & knight_msk = board_.fmgr_.knight_mask(color);
    uint64 eat_msk = knight_msk & knight_caps;

    for ( ; eat_msk; )
    {
      int n = clear_lsb(eat_msk);

      const Field & fknight = board_.getField(n);

      THROW_IF( !fknight || fknight.type() != Figure::TypeKnight || fknight.color() != board_.color_, "no knight on field we are going to do capture from" );

      add(m, n, ch_pos, Figure::TypeNone, true);
    }
  }

  // 3rd - bishops, rooks and queens
  {
    const uint64 & queen_caps = board_.g_movesTable->caps(Figure::TypeQueen, ch_pos);
    uint64 brq_msk = board_.fmgr_.bishop_mask(color) | board_.fmgr_.rook_mask(color) | board_.fmgr_.queen_mask(color);
    uint64 eat_msk = brq_msk & queen_caps;

    for ( ; eat_msk; )
    {
      int n = clear_lsb(eat_msk);

      const Field & field = board_.getField(n);

      THROW_IF( !field || field.color() != board_.color_, "no figure on field we are going to do capture from" );

      // can fig go to checking figure field
      int dir = board_.g_figureDir->dir(field.type(), board_.color_, n, ch_pos);
      if ( dir < 0 )
        continue;

      const uint64 & btw_msk = board_.g_betweenMasks->between(n, ch_pos);
      if ( (btw_msk & mask_all_inv) != btw_msk )
        continue;

      add(m, n, ch_pos, Figure::TypeNone, true);
    }
  }

  // now try to protect king - put something between it and checking figure
  const uint64 & protect_king_msk = board_.g_betweenMasks->between(board_.kingPos(color), ch_pos);

  if ( protect_king_msk && Figure::TypePawn != ch_type && Figure::TypeKnight != ch_type )
  {
    // 1. Pawns
    BitMask pw_mask = board_.fmgr().pawn_mask_o(color);
    for ( ; pw_mask; )
    {
      int pw_pos = clear_msb(pw_mask);

      // +2 - skip captures
      const int8 * table = board_.g_movesTable->pawn(color, pw_pos) + 2;

      for (; *table >= 0 && !board_.getField(*table); ++table)
      {
        if ( !(protect_king_msk & (1ULL << *table)) )
          continue;

        bool promotion = *table > 55 || *table < 8;

        if ( add(m, pw_pos, *table, promotion ? Figure::TypeQueen : Figure::TypeNone, false) && promotion )
        {
          Move & move = moves_[--m];

          moves_[++m] = move;
          moves_[m++].new_type_ = Figure::TypeRook;

          moves_[m] = move;
          moves_[m++].new_type_ = Figure::TypeBishop;

          moves_[m] = move;
          moves_[m++].new_type_ = Figure::TypeKnight;
        }
      }
    }


    // 2. Knights
    BitMask kn_mask = board_.fmgr().knight_mask(color);
    for ( ; kn_mask; )
    {
      int kn_pos = clear_msb(kn_mask);

      const uint64 & knight_msk = board_.g_movesTable->caps(Figure::TypeKnight, kn_pos);
      uint64 msk_protect = protect_king_msk & knight_msk;
      for ( ; msk_protect; )
      {
        int n = clear_lsb(msk_protect);

        const Field & field = board_.getField(n);

        THROW_IF( field, "there is something between king and checking figure" );

        add(m, kn_pos, n, Figure::TypeNone, false);
      }
    }

    // 3. Bishops + Rooks + Queens
    for (int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
    {
      BitMask fg_mask = board_.fmgr().type_mask((Figure::Type)type, color);
      for ( ; fg_mask; )
      {
        int fg_pos = clear_msb(fg_mask);

        const uint64 & figure_msk = board_.g_movesTable->caps(type, fg_pos);
        uint64 msk_protect = protect_king_msk & figure_msk;

        for ( ; msk_protect; )
        {
          int n = clear_lsb(msk_protect);

          const Field & field = board_.getField(n);

          THROW_IF( field, "there is something between king and checking figure" );

          THROW_IF( board_.g_figureDir->dir((Figure::Type)type, color, fg_pos, n) < 0, "figure can't go to required field" );

          const uint64 & btw_msk = board_.g_betweenMasks->between(fg_pos, n);
          if ( (btw_msk & mask_all_inv) != btw_msk )
            continue;

          add(m, fg_pos, n, Figure::TypeNone, false);
        }
      }
    }
  }

  // at the last generate all king's movements
  m = generateKingonly(m);

  return m;
}

int EscapeGenerator::generateKingonly(int m)
{
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);

  int king_pos = board_.kingPos(color);
  const int8 * table = board_.g_movesTable->king(king_pos);

  for (; *table >= 0; ++table)
  {
    const Field & field = board_.getField(*table);
    bool capture = false;
    if ( field )
    {
      if ( field.color() == color )
        continue;

      capture = true;
    }

    Move & move = moves_[m];
    move.set(king_pos, *table, Figure::TypeNone, capture);
    if ( move == hmove_ || !board_.isMoveValidUnderCheck(move) )
      continue;

    move.checkVerified_ = 1;
    m++;
  }

  return m;
}


//////////////////////////////////////////////////////////////////////////
void MovesGeneratorBase::save_history(const char * fname)
{
  FILE * f = fopen(fname, "wb");
  if ( !f )
    return;
  
  fwrite(&History::history_max_, sizeof(History::history_max_), 1, f);
  fwrite((char*)history_, sizeof(History), 64*64, f);

  fclose(f);
}

void MovesGeneratorBase::load_history(const char * fname)
{
  FILE * f = fopen(fname, "rb");
  if ( !f )
    return;

  fread(&History::history_max_, sizeof(History::history_max_), 1, f);
  fread((char*)history_, sizeof(History), 64*64, f);

  fclose(f);
}
