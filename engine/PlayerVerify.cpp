/*************************************************************
  PlayerVerify.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "Player.h"
#include "MovesGenerator.h"
#include "windows.h"

//////////////////////////////////////////////////////////////////////////
void Player::saveHash(const char * fname) const
{
  if ( !fname )
    return;
  char gfname[MAX_PATH], cfname[MAX_PATH], bfname[MAX_PATH], hfname[MAX_PATH];
  sprintf(gfname, "%s_g.hash", fname);
  sprintf(cfname, "%s_c.hash", fname);
  sprintf(bfname, "%s_b.hash", fname);
  sprintf(hfname, "%s_h.hash", fname);

#ifdef USE_HASH_TABLE_GENERAL
  ghash_.save(gfname);
#endif

#ifdef USE_HASH_TABLE_CAPTURE
  chash_.save(cfname);
#endif

  board_.save(bfname);
  MovesGeneratorBase::save_history(hfname);
}

void Player::loadHash(const char * fname)
{
  char gfname[MAX_PATH], cfname[MAX_PATH], bfname[MAX_PATH], hfname[MAX_PATH];
  sprintf(gfname, "%s_g.hash", fname);
  sprintf(cfname, "%s_c.hash", fname);
  sprintf(bfname, "%s_b.hash", fname);
  sprintf(hfname, "%s_h.hash", fname);

#ifdef USE_HASH_TABLE_GENERAL
  ghash_.load(gfname);
#endif

#ifdef USE_HASH_TABLE_CAPTURE
  chash_.load(cfname);
#endif

  board_.load(bfname);
  MovesGenerator::load_history(hfname);
}
//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_ESCAPE_GENERATOR
void Player::verifyEscapeGen()
{
  if ( !board_.underCheck() )
    return;

  Move hmove;
  Move killer;
  hmove.clear();
  killer.clear();
  EscapeGenerator eg(hmove, board_);
  MovesGenerator mg(board_, killer);
  THROW_IF( mg.has_duplicates(), "move generator generates some move twice" );
  THROW_IF( eg.has_duplicates(), "escape generator generates some move twice" );

  Move legal[Board::MovesMax];
  int num = 0;

  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    Board board0(board_);

    if ( !board_.validateMove(move) )
      continue;

    if ( move.new_type_ && move.new_type_ != Figure::TypeQueen )
    {
      if ( move.new_type_ != Figure::TypeKnight )
        continue;

      int ki_pos = board_.kingPos(Figure::otherColor(board_.getColor()));
      int dir = board_.g_figureDir->dir(Figure::TypeKnight, board_.color_, move.to_, ki_pos);
      if ( dir < 0 )
        continue;

      if ( board_.see(move) < 0 )
        continue;
    }

    legal[num++] = move;

    board_.makeMove(move);
    board_.verifyMasks();
    board_.unmakeMove();

    THROW_IF( board0 != board_, "board unmake wasn't correct applied" );

    board_.verifyMasks();

    if ( !eg.find(move) )
    {
      char fen[256];
      board_.toFEN(fen);
      EscapeGenerator eg1(hmove, board_);
      {
        if ( move.new_type_ && move.new_type_ != Figure::TypeQueen )
        {
          if ( move.new_type_ != Figure::TypeKnight )
            continue;

          int ki_pos = board_.kingPos(Figure::otherColor(board_.getColor()));
          int dir = board_.g_figureDir->dir(Figure::TypeKnight, board_.color_, move.to_, ki_pos);
          if ( dir < 0 )
            continue;

          if ( board_.see(move) < 0 )
            continue;
        }
      }

      THROW_IF( true, "some legal escape from check wasn't generated" );
    }
  }

  THROW_IF(eg.count() != num, "number of escape moves isn't correct");

  for ( ;; )
  {
    const Move & move = eg.escape();
    if ( !move )
      break;

    Board board0(board_);

    if ( !board_.validateMove(move) )
    {
      THROW_IF( true, "illegal move generated by escape generator" );
    }

    board_.makeMove(move);
    board_.verifyMasks();
    board_.unmakeMove();

    THROW_IF( board0 != board_, "board unmake wasn't correct applied" );

    board_.verifyMasks();

    bool found = false;
    for (int i = 0; !found && i < num; ++i)
    {
      if ( legal[i] == move )
        found = true;
    }

    THROW_IF( !found, "move from escape generator isn't found in the legal moves list" );
  }
}
#endif

//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_CHECKS_GENERATOR
void Player::verifyChecksGenerator(Figure::Type minimalType)
{
  Move hmove, killer;
  hmove.clear();
  killer.clear();
  MovesGenerator mg(board_, killer);
  CapsGenerator cg(hmove, board_, minimalType);
  ChecksGenerator ckg(hmove, board_, minimalType);

  Move legal[Board::MovesMax], checks[Board::MovesMax];
  int n = 0, m = 0;

  int num = 0;
  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    num++;
    if ( !board_.validateMove(move) )
      continue;

    Board board0(board_);

    board_.makeMove(move);
    if ( board_.underCheck() )
    {
      if ( move.new_type_ == Figure::TypeQueen || !move.new_type_ )
        legal[n++] = move;
      else if ( move.new_type_ == Figure::TypeKnight )
      {
        const Field & ffield = board_.getField(move.to_);
        THROW_IF( ffield.color() == board_.color_ || ffield.type() != Figure::TypeKnight, "invalid color or type of promoted knight" );
        bool checkingKnight = false;
        for (int j = 0; j < board_.checkingNum_; ++j)
        {
          if ( board_.checking_[j] == move.to_ )
          {
            checkingKnight = true;
            break;
          }
        }
        if ( checkingKnight )
        {
          int oking_pos = board_.kingPos(board_.color_);
          int dir = board_.g_figureDir->dir(ffield.type(), ffield.color(), move.to_, oking_pos);
          THROW_IF ( dir < 0, "checking knight actually doesn't" );
          legal[n++] = move;
        }
      }
    }

    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't correct applied" );
    board_.verifyMasks();
  }

  THROW_IF( mg.count() != num, "not all moves were enumerated" );

  for (int i = 0; i < cg.count(); ++i)
  {
    const Move & move = cg[i];
    if ( !move )
      break;

    if ( !board_.validateMove(move) )
      continue;

    Board board0(board_);

    board_.makeMove(move);
    
    if ( board_.underCheck() )
      checks[m++] = move;

    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't correct applied" );
    board_.verifyMasks();
  }

  for (int i = 0; i < ckg.count(); ++i)
  {
    const Move & move = ckg[i];
    if ( !move )
      break;

    if ( !board_.validateMove(move) )
      continue;

    Board board0(board_);

    board_.makeMove(move);

    THROW_IF( !board_.underCheck(), "non checking move" );

    for (int i = 0; i < m; ++i)
    {
      THROW_IF(checks[i] == move, "check is generated twice by caps & checks generators");
    }

    checks[m++] = move;

    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't correct applied" );
    board_.verifyMasks();

    THROW_IF( cg.find(move), "duplicated move found in checks geneator" );

  }

  for (int i = 0; i < n; ++i)
  {
    const Move & move = legal[i];

    bool found = false;
    for (int j = 0; j < m; ++j)
    {
      const Move & cmove = checks[j];
      if ( cmove == move )
      {
        found = true;
        break;
      }
    }

    if ( !found )
    {
      char fen[256];
      board_.toFEN(fen);
      CapsGenerator cg1(hmove, board_, minimalType);
      ChecksGenerator ckg1(hmove, board_, minimalType);
    }

    THROW_IF( !found, "some check wasn't generated" );
  }

  for (int i = 0; i < m; ++i)
  {
    const Move & cmove = checks[i];

    bool found = false;
    for (int j = 0; j < n; ++j)
    {
      const Move & move = legal[j];
      if ( move == cmove )
      {
        found = true;
        break;
      }
    }

    THROW_IF( !found, "some invalid check was generated" );
  }
}
#endif
//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_CAPS_GENERATOR
void Player::verifyCapsGenerator()
{

  Figure::Type minimalType = Figure::TypePawn;

  Move hmove, killer;
  hmove.clear();
  killer.clear();
  MovesGenerator mg(board_, killer);
  CapsGenerator cg(hmove, board_, minimalType);

  Move legal[Board::MovesMax], caps[Board::MovesMax];
  int n = 0, m = 0;

  int num = 0;
  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    num++;
    if ( !board_.validateMove(move) )
      continue;

    Board board0(board_);

    board_.makeMove(move);
    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't applied correctly" );
    board_.verifyMasks();

    if ( move.capture_ )
    {
      if ( move.to_ == board_.en_passant_ && board_.getField(move.from_).type() == Figure::TypePawn )
      {
        int ep_pos = board_.enpassantPos();
        THROW_IF( board_.getField(ep_pos).type() != Figure::TypePawn || board_.getField(ep_pos).color() == board_.color_, "invalid en-passant position" );
        if ( minimalType <= Figure::TypePawn )
          legal[n++] = move;
      }
      else
      {
        const Field & tfield = board_.getField(move.to_);
        THROW_IF( !tfield, "captured field is empty" );
        if ( (tfield.type() >= minimalType && !move.new_type_) || (move.new_type_ == Figure::TypeQueen) )
          legal[n++] = move;
      }
    }
    else if ( move.new_type_ == Figure::TypeQueen && minimalType <= Figure::TypeQueen )
      legal[n++] = move;
  }

  THROW_IF( num != mg.count(), "not all moves were enumerated" );

  for (int i = 0; i < cg.count(); ++i)
  {
    const Move & cap = cg[i];
    if ( !cap )
      break;

    if ( !board_.validateMove(cap) )
      continue;

    Board board0(board_);

    board_.makeMove(cap);
    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't applied correctly" );
    board_.verifyMasks();

    caps[m++] = cap;
  }

  for (int i = 0; i < n; ++i)
  {
    const Move & move = legal[i];

    bool found = false;
    for (int j = 0; j < m; ++j)
    {
      const Move & cap = caps[j];
      if ( cap == move )
      {
        found = true;
        break;
      }
    }

    THROW_IF( !found, "some capture wasn't generated" );
  }

  for (int i = 0; i < m; ++i)
  {
    const Move & cap = caps[i];

    bool found = false;
    for (int j = 0; j < n; ++j)
    {
      const Move & move = legal[j];
      if ( move == cap )
      {
        found = true;
        break;
      }
    }
    THROW_IF( !found, "some invalid capture was generated" );
  }
}
#endif


#ifdef VERIFY_FAST_GENERATOR
void Player::verifyFastGenerator(const Move & hmove, const Move & killer)
{
  MovesGenerator mg(board_, killer);
  FastGenerator fg(board_, hmove, killer);

  Move legal[Board::MovesMax];
  Move fast[Board::MovesMax];
  int n = 0, m = 0;

  for ( ;; )
  {
    Move & move = mg.move();
    if ( !move )
      break;

    if ( !board_.validateMove(move) )
      continue;

    Board board0(board_);
    board_.makeMove(move);

    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't applied correctly" );
    board_.verifyMasks();

    if ( move.new_type_ && move.new_type_ != Figure::TypeQueen )
    {
      if ( move.new_type_ != Figure::TypeKnight )
        continue;

      int ki_pos = board_.kingPos(Figure::otherColor(board_.getColor()));
      int dir = board_.g_figureDir->dir(Figure::TypeKnight, board_.color_, move.to_, ki_pos);
      if ( dir < 0 )
        continue;

      if ( board_.see(move) < 0 )
        continue;
    }

    legal[n++] = move;
  }

  for ( ;; )
  {
    Move & move = fg.move();
    if ( !move )
      break;

    fast[m++] = move;
  }


  for (int i = 0; i < n; ++i)
  {
    Move & move = legal[i];
    bool found = false;
    for (int j = 0; !found && j < m; ++j)
    {
      if ( move == fast[j] )
        found = true;
    }

    if ( !found )
    {
      char str[256];
      board_.toFEN(str);
      FastGenerator fg1(board_, hmove, killer);
      while ( fg1.move() );
    }

    THROW_IF( !found, "move wasn't generated by fast generator" );
  }

  for (int j = 0; j < m; ++j)
  {
    Move & move = fast[j];
    bool found = false;
    for (int i = 0; !found && i < n; ++i)
    {
      if ( move == fast[j] )
        found = true;
    }

    if ( !found )
    {
      char str[256];
      board_.toFEN(str);
      FastGenerator fg1(board_, hmove, killer);
      while ( fg1.move() );
    }

    THROW_IF( !found, "odd move was generated by fast generator" );
  }
}
#endif
