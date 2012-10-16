#include "MovesGenerator.h"
#include "MovesTable.h"
#include "Player.h"

//////////////////////////////////////////////////////////////////////////

History MovesGenerator::history_[64][64];
/*************************************************************
  MovesGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

unsigned History::history_max_;

MovesGenerator::MovesGenerator(Board & board, int depth, int ply, Player * player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), depth_(depth), ply_(ply), player_(player), history_max_(0)
{
#ifdef USE_KILLER
  if ( player_ && player_->contexts_[ply_].killer_ )
    killer_ = player_->contexts_[ply_].killer_;
  else
#endif
    killer_.clear();

  numOfMoves_ = generate(alpha, betta, counter);
  moves_[numOfMoves_].clear();
}

MovesGenerator::MovesGenerator(Board & board) :
  board_(board), current_(0), numOfMoves_(0), ply_(0), depth_(0), player_(0), history_max_(0)
{
  killer_.clear();
  ScoreType alpha = 0, betta = 0;
  int counter = 0;
  numOfMoves_ = generate(alpha, betta, counter);
  moves_[numOfMoves_].clear();
}

void MovesGenerator::clear_history()
{
  for (int i = 0; i < 64; ++i)
    for (int j = 0; j < 64; ++j)
      history_[i][j].clear();
  History::history_max_ = 0;
}

void MovesGenerator::normalize_history(int n)
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
bool MovesGenerator::find(const Move & m) const
{
  for (int i = 0; i < numOfMoves_; ++i)
  {
    const Move & move = moves_[i];
    if ( m == move )
      return true;
  }
  return false;
}
//////////////////////////////////////////////////////////////////////////
int MovesGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
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
          int rindex = -1;
          if ( field && field.color() == ocolor )
            rindex = field.index();
          else if ( board_.en_passant_ >= 0 )
          {
            const Figure & rfig = board_.getFigure(ocolor, board_.en_passant_);
            int8 to = rfig.where();
            static const int8 delta_pos[] = {8, -8};
            to += delta_pos[ocolor];
            if ( to == *table )
              rindex = board_.en_passant_;
          }

          if ( rindex < 0 )
            continue;

          const Figure & rfig = board_.getFigure(ocolor, rindex);

          bool promotion = *table > 55 || *table < 8;

          Move & move = moves_[m++];
          move.alreadyDone_ = 0;
          move.set(pw_pos, *table, rfig.getIndex(), 0, 0);
          calculateWeight(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeRook;
            calculateWeight(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeBishop;
            calculateWeight(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeKnight;
            calculateWeight(moves_[m++]);
          }
        }

        for (; *table >= 0 && !board_.getField(*table); ++table)
        {
          bool promotion = *table > 55 || *table < 8;

          Move & move = moves_[m++];

          move.alreadyDone_ = 0;
          move.set(pw_pos, *table, -1, 0, 0);
          calculateWeight(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeRook;
            calculateWeight(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeBishop;
            calculateWeight(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeKnight;
            calculateWeight(moves_[m++]);
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
          int rindex = -1;
          if ( field )
          {
            if ( field.color() == color )
              continue;
            rindex = field.index();
          }

          add_move(m, kn_pos, *table, rindex, 0);
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
          int rindex = -1;
          for ( ; count && rindex < 0; --count)
          {
            p += delta;

            const Field & field = board_.getField(p);
            if ( field )
            {
              if ( field.color() == color )
                break;

              rindex = field.index();
            }

            add_move(m, fg_pos, p, rindex, 0);
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
      int rindex = -1;
      if ( field )
      {
        if ( field.color() == color )
          continue;
        rindex = field.index();
      }

      add_move(m, ki_pos, *table, rindex, 0);
    }

    int index = board_.getField(ki_pos).index();
    const Figure & king = board_.getFigure(color, index);
    if ( king.isFirstStep() && board_.state_ != Board::UnderCheck )
    {
      add_move(m, ki_pos, ki_pos+2, -1, 0);
      add_move(m, ki_pos, ki_pos-2, -1, 0);
    }
  }

  return m;
}

void MovesGenerator::calculateWeight(Move & move)
{
  const Field & ffield = board_.getField(move.from_);
  THROW_IF( !ffield, "no figure on field we move from" );

  const History & hist = history_[move.from_][move.to_];
  move.srt_score_ = hist.score_ + 100000;

  if ( move.srt_score_ > history_max_ )
    history_max_ = move.srt_score_;

  if ( move.rindex_ >= 0 )
  {
    Figure::Type atype = board_.getField(move.from_).type();
    Figure::Type vtype = board_.getFigure(Figure::otherColor(board_.getColor()), move.rindex_).getType();
    move.srt_score_ = Figure::figureWeight_[vtype] - Figure::figureWeight_[atype] + 10000000;
    if ( board_.halfmovesCount() > 1 )
    {
      MoveCmd & prev = board_.getMoveRev(-1);
      if ( prev.to_ == move.to_ )
        move.srt_score_ += Figure::figureWeight_[vtype] >> 1;
    }
  }
  else if ( move.new_type_ > 0 )
  {
    move.srt_score_ = Figure::figureWeight_[move.new_type_] + 5000000;
  }
#ifdef USE_KILLER
  else if ( move == killer_ )
  {
    move.srt_score_ = 3000000;
    move.fkiller_ = 1;
  }
#endif
}

Move & MovesGenerator::move()
{
  for ( ;; )
  {
    Move * move = moves_ + numOfMoves_;
    Move * mv = moves_;
    for ( ; *mv; ++mv)
    {
      if ( mv->alreadyDone_ || mv->srt_score_ < move->srt_score_ )
        continue;

      move = mv;
    }
    if ( !*move )
      return *move;

    int see_gain = 0;
    if ( (move->rindex_ >= 0 || move->new_type_ > 0) && !move->seen_ && !see(*move, see_gain) )
    {
      move->seen_ = 1;
      if ( move->rindex_ >= 0 )
        move->srt_score_ = see_gain + 3000;//000;
      else
        move->srt_score_ = see_gain + 2000;//000;

      continue;
    }

    move->alreadyDone_ = 1;
    return *move;
  }
}


bool MovesGenerator::see(Move & move, int & see_gain)
{
  THROW_IF(move.rindex_ < 0 && move.new_type_ == 0, "try to see() move that isn't capture or promotion");

  if ( move.rindex_ >= 0 )
  {
    // victim >= attacker
    Figure::Type vtype = board_.getFigure(Figure::otherColor(board_.getColor()), move.rindex_).getType();
    Figure::Type atype = board_.getField(move.from_).type();
    if ( Figure::figureWeightSEE_[vtype] >= Figure::figureWeightSEE_[atype] )
      return true;
  }

  // we look from side, that goes to move. we should adjust sing of initial mat-balance
  int initial_balance = board_.fmgr().weight();
  if ( !board_.getColor() )
    initial_balance = -initial_balance;

  see_gain = board_.see_before(initial_balance, move);

//#ifndef NDEBUG
//  int see_gain1 = board_.see_before2(initial_balance, move);
//  THROW_IF(see_gain != see_gain1, "see_before2() failed" );
//#endif

  return see_gain >= 0;
}

//////////////////////////////////////////////////////////////////////////
EscapeGenerator::EscapeGenerator(const Move & pv, Board & board, int depth, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), depth_(depth), ply_(ply), player_(player), pv_(pv)
{
  numOfMoves_ = push_pv();
  numOfMoves_ = generate(alpha, betta, counter);
  escapes_[numOfMoves_].clear();

#ifdef SORT_ESCAPE_MOVES
  sort();
#endif
}

EscapeGenerator::EscapeGenerator(Board & board, int depth, int ply, Player & player, ScoreType & alpha, ScoreType betta, int & counter) :
  board_(board), current_(0), numOfMoves_(0), depth_(depth), ply_(ply), player_(player)
{
  pv_.clear();

  numOfMoves_ = generate(alpha, betta, counter);
  escapes_[numOfMoves_].clear();
}

int EscapeGenerator::push_pv()
{
  int m = 0;
  if ( pv_ && board_.validMove(pv_) && board_.isMoveValidUnderCheck(pv_) )
  {
    Move & move = escapes_[m];
    move = pv_;
    move.checkVerified_ = 1;
    move.alreadyDone_ = 0;
    ++m;
  }
  else
    pv_.clear();

  return m;
}

void EscapeGenerator::sort()
{
  for (int i = 0; i < numOfMoves_; ++i)
  {
    Move & move = escapes_[i];
    move.srt_score_ = 0;
    if ( move == pv_ )
      move.srt_score_ = 100000000;
    else if ( move.rindex_ >= 0 )
    {
      Figure::Type atype = board_.getField(move.from_).type();
      Figure::Type vtype = board_.getFigure(Figure::otherColor(board_.getColor()), move.rindex_).getType();
      if ( Figure::figureWeightSEE_[vtype] >= Figure::figureWeightSEE_[atype] || atype == Figure::TypeKing )
      {
        if ( atype == Figure::TypeKing )
          move.srt_score_ = Figure::figureWeightSEE_[vtype] + 10002000;
        else
          move.srt_score_ = Figure::figureWeightSEE_[vtype] - (Figure::figureWeightSEE_[atype] >> 2) + 10000000;

        if ( board_.halfmovesCount() > 1 )
        {
          MoveCmd & prev = board_.getMoveRev(-1);
          if ( prev.to_ == move.to_ )
            move.srt_score_ += Figure::figureWeight_[vtype] >> 1;
        }
      }
      else
      {
        int initial_balance = board_.fmgr().weight();
        if ( !board_.getColor() )
          initial_balance = -initial_balance;

        int score_see = board_.see_before(initial_balance, move);

//#ifndef NDEBUG
//        int score_see1 = board_.see_before2(initial_balance, move);
//        THROW_IF(score_see != score_see1, "see_before2() failed" );
//#endif

        if ( score_see >= 0 )
          move.srt_score_ = (unsigned)score_see + 10000000;
        else
          move.srt_score_ = (unsigned)(10000 + score_see);
      }
    }
    else if ( move.new_type_ > 0 )
    {
      int initial_balance = board_.fmgr().weight();
      if ( !board_.getColor() )
        initial_balance = -initial_balance;

      int score_see = board_.see_before(initial_balance, move);

//#ifndef NDEBUG
//      int score_see1 = board_.see_before2(initial_balance, move);
//      THROW_IF(score_see != score_see1, "see_before2() failed" );
//#endif

      if ( score_see >= 0 )
        move.srt_score_ = Figure::figureWeightSEE_[move.new_type_] + 5000000;
      else
        move.srt_score_ = (unsigned)(5000 + score_see);
    }
    else
    {
      const History & hist = MovesGenerator::history(move.from_, move.to_);
      move.srt_score_ = hist.score_ + 20000;
    }
  }

  for (int i = 0; i < numOfMoves_; ++i)
  {
    Move & move_i = escapes_[i];
    for (int j = i+1; j < numOfMoves_; ++j)
    {
      Move & move_j = escapes_[j];
      if ( move_j.srt_score_ > move_i.srt_score_ )
      {
        Move t = move_j;
        move_j = move_i;
        move_i = t;
      }
    }
  }
}

int EscapeGenerator::generate(ScoreType & alpha, ScoreType betta, int & counter)
{
  if ( board_.checkingNum_ == 1 )
    return generateUsual(alpha, betta, counter);
  else
    return generateKingonly(numOfMoves_, alpha, betta, counter);
}

int EscapeGenerator::generateUsual(ScoreType & alpha, ScoreType betta, int & counter)
{
  int m = numOfMoves_;
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);

  THROW_IF( board_.checking_[0] < 0, "there is no checking figure index" );

  const Figure & cfig = board_.getFigure(ocolor, board_.checking_[0]);

  THROW_IF( !cfig, "there is no checking figure" );
  THROW_IF( cfig.getType() == Figure::TypeKing, "king is attacking king" );

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
    uint64 checking_fig_mask = 1ULL << cfig.where();

    pawns_eat = (pawn_eat_msk & checking_fig_mask) != 0;

    if ( cfig.getType() == Figure::TypePawn && board_.en_passant_ == board_.checking_[0] )
    {
      const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
      THROW_IF( !epawn, "there is no en passant pawn" );

      int to = epawn.where() + pw_delta[board_.color_];
      THROW_IF( (unsigned)to > 63, "invalid en passant field index" );

      ep_capture = (pawn_eat_msk & (1ULL << to)) != 0;
    }

    pawns_eat = pawns_eat || ep_capture;
  }

  // 1st - get pawn's captures
  if ( pawns_eat )
  {
    // en-passant
    if ( ep_capture )
    {
      const Figure & epawn = board_.getFigure(ocolor, board_.en_passant_);
      THROW_IF( !epawn, "there is no en passant pawn" );

      int8 to = epawn.where() + pw_delta[board_.color_];
      const uint64 & opawn_caps_ep = board_.g_movesTable->pawnCaps_o(ocolor, to);
      uint64 eat_msk_ep = pawn_msk & opawn_caps_ep;

      for ( ; eat_msk_ep; )
      {
        int n = clear_lsb(eat_msk_ep);

        const Field & fpawn = board_.getField(n);
        
        THROW_IF( !fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != board_.color_, "no pawn on field we are going to do capture from" );

        const Figure & pawn = board_.getFigure(board_.color_, fpawn.index());

        THROW_IF( !pawn, "capturing pawn not found" );

        add_escape(m, n, to, epawn.getIndex(), 0);
      }
    }


    const uint64 & opawn_caps = board_.g_movesTable->pawnCaps_o(ocolor, cfig.where());
    uint64 eat_msk = pawn_msk & opawn_caps;

    bool promotion = cfig.where() > 55 || cfig.where() < 8; // 1st || last line

    for ( ; eat_msk; )
    {
      int n = clear_lsb(eat_msk);

      const Field & fpawn = board_.getField(n);

      THROW_IF( !fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != board_.color_, "no pawn on field we are going to do capture from" );

      const Figure & pawn = board_.getFigure(board_.color_, fpawn.index());

      THROW_IF( !pawn, "capturing pawn not found" );

      int m0 = m;
      if ( add_escape(m, n, cfig.where(), cfig.getIndex(), promotion ? Figure::TypeQueen : 0) && promotion )
      {
        Move & move = escapes_[m0];

        escapes_[m] = move;
        escapes_[m++].new_type_ = Figure::TypeRook;

        escapes_[m] = move;
        escapes_[m++].new_type_ = Figure::TypeBishop;

        escapes_[m] = move;
        escapes_[m++].new_type_ = Figure::TypeKnight;
      }
    }
  }

  // 2nd - knight's captures
  {
    const uint64 & knight_caps = board_.g_movesTable->caps(Figure::TypeKnight, cfig.where());
    const uint64 & knight_msk = board_.fmgr_.knight_mask(color);
    uint64 eat_msk = knight_msk & knight_caps;

    for ( ; eat_msk; )
    {
      int n = clear_lsb(eat_msk);

      const Field & fknight = board_.getField(n);

      THROW_IF( !fknight || fknight.type() != Figure::TypeKnight || fknight.color() != board_.color_, "no knight on field we are going to do capture from" );

      const Figure & knight = board_.getFigure(board_.color_, fknight.index());

      THROW_IF( !knight, "capturing knight not found" );

      add_escape(m, n, cfig.where(), cfig.getIndex(), 0);
    }
  }

  // 3rd - bishops, rooks and queens
  {
    const uint64 & queen_caps = board_.g_movesTable->caps(Figure::TypeQueen, cfig.where());
    uint64 brq_msk = board_.fmgr_.bishop_mask(color) | board_.fmgr_.rook_mask(color) | board_.fmgr_.queen_mask(color);
    uint64 eat_msk = brq_msk & queen_caps;

    for ( ; eat_msk; )
    {
      int n = clear_lsb(eat_msk);

      const Field & field = board_.getField(n);

      THROW_IF( !field || field.color() != board_.color_, "no figure on field we are going to do capture from" );

      const Figure & fig = board_.getFigure(board_.color_, field.index());

      THROW_IF( !fig || !(fig.getType() == Figure::TypeBishop || fig.getType() == Figure::TypeRook || fig.getType() == Figure::TypeQueen), "capturing figure not found" );

      // can fig go to cfig's field
      int dir = board_.g_figureDir->dir(field.type(), board_.color_, n, cfig.where());
      if ( dir < 0 )
        continue;

      const uint64 & btw_msk = board_.g_betweenMasks->between(n/*fig.where()*/, cfig.where());
      if ( (btw_msk & mask_all_inv) != btw_msk )
        continue;

      add_escape(m, n, cfig.where(), cfig.getIndex(), 0);
    }
  }

  // now try to protect king - put something between it and checking figure
  const Figure & king = board_.getFigure(color, Board::KingIndex);
  const uint64 & protect_king_msk = board_.g_betweenMasks->between(king.where(), cfig.where());

  if ( Figure::TypePawn != cfig.getType() && Figure::TypeKnight != cfig.getType()&& protect_king_msk )
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

        int m0 = m;
        if ( add_escape(m, pw_pos, *table, -1, promotion ? Figure::TypeQueen : 0) && promotion )
        {
          Move & move = escapes_[m0];

          escapes_[m] = move;
          escapes_[m++].new_type_ = Figure::TypeRook;

          escapes_[m] = move;
          escapes_[m++].new_type_ = Figure::TypeBishop;

          escapes_[m] = move;
          escapes_[m++].new_type_ = Figure::TypeKnight;
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

        add_escape(m, kn_pos, n, -1, 0);
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

          add_escape(m, fg_pos, n, -1, 0);
        }
      }
    }
  }

  // at the last generate all king's movements
  m = generateKingonly(m, alpha, betta, counter);

  return m;
}

int EscapeGenerator::generateKingonly(int m, ScoreType & alpha, ScoreType betta, int & counter)
{
  Figure::Color & color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);

  Move prev;
  prev.clear();
  if ( board_.halfmovesCount() > 0 )
    prev = board_.getMoveRev(0);
  
  const Figure & king = board_.getFigure(color, Board::KingIndex);
  const int8 * table = board_.g_movesTable->king(king.where());

  for (; *table >= 0; ++table)
  {
    const Field & field = board_.getField(*table);
    int rindex = -1;
    if ( field )
    {
      if ( field.color() == color )
        continue;

      rindex = field.index();
    }

    Move move;
    move.set(king.where(), *table, rindex, 0, 0);
    if ( move == pv_ || !board_.isMoveValidUnderCheck(move) )
      continue;

    move.checkVerified_ = 1;
    escapes_[m] = move;
    m++;
  }

  return m;
}

//////////////////////////////////////////////////////////////////////////
bool EscapeGenerator::find(const Move & m) const
{
  for (int i = 0; i < numOfMoves_; ++i)
  {
    const Move & move = escapes_[i];
    if ( m == move )
      return true;
  }
  return false;
}


//////////////////////////////////////////////////////////////////////////
void MovesGenerator::save_history(const char * fname)
{
  FILE * f = fopen(fname, "wb");
  if ( !f )
    return;
  
  fwrite(&History::history_max_, sizeof(History::history_max_), 1, f);
  fwrite((char*)history_, sizeof(History), 64*64, f);

  fclose(f);
}

void MovesGenerator::load_history(const char * fname)
{
  FILE * f = fopen(fname, "rb");
  if ( !f )
    return;

  fread(&History::history_max_, sizeof(History::history_max_), 1, f);
  fread((char*)history_, sizeof(History), 64*64, f);

  fclose(f);
}
