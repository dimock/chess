/*************************************************************
  PlayerVerify.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/


#include "Player.h"
#include "MovesGenerator.h"
#include "windows.h"
#include <fstream>
#include <sstream>

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

#ifdef USE_HASH
  hash_.save(gfname);
#endif

  scontexts_[0].board_.save(bfname);
  MovesGeneratorBase::save_history(hfname);
}

void Player::loadHash(const char * fname)
{
  char gfname[MAX_PATH], cfname[MAX_PATH], bfname[MAX_PATH], hfname[MAX_PATH];
  sprintf(gfname, "%s_g.hash", fname);
  sprintf(cfname, "%s_c.hash", fname);
  sprintf(bfname, "%s_b.hash", fname);
  sprintf(hfname, "%s_h.hash", fname);

#ifdef USE_HASH
  hash_.load(gfname);
#endif

  scontexts_[0].board_.load(bfname);
  MovesGenerator::load_history(hfname);
}
//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_ESCAPE_GENERATOR
void Player::verifyEscapeGen(int ictx, const Move & hmove)
{
  if ( !scontexts_[ictx].board_.underCheck() )
    return;

  EscapeGenerator eg(hmove, scontexts_[ictx].board_);
  MovesGenerator mg(scontexts_[ictx].board_);
  THROW_IF( mg.has_duplicates(), "move generator generates some move twice" );
  THROW_IF( eg.has_duplicates(), "escape generator generates some move twice" );

  Move legal[Board::MovesMax];
  int num = 0;

  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    Board board0(scontexts_[ictx].board_);

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    if ( move.new_type_ && move.new_type_ != Figure::TypeQueen )
    {
      if ( move.new_type_ != Figure::TypeKnight )
        continue;

      int ki_pos = scontexts_[ictx].board_.kingPos(Figure::otherColor(scontexts_[ictx].board_.getColor()));
      int dir = scontexts_[ictx].board_.g_figureDir->dir(Figure::TypeKnight, scontexts_[ictx].board_.color_, move.to_, ki_pos);
      if ( dir < 0 )
        continue;

      if ( scontexts_[ictx].board_.see(move) < 0 )
        continue;
    }

    legal[num++] = move;

    scontexts_[ictx].board_.makeMove(move);
    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();

    THROW_IF( board0 != scontexts_[ictx].board_, "board unmake wasn't correct applied" );

    scontexts_[ictx].board_.verifyMasks();

    if ( !eg.find(move) )
    {
      char fen[256];
      scontexts_[ictx].board_.toFEN(fen);
      EscapeGenerator eg1(hmove, scontexts_[ictx].board_);
      {
        if ( move.new_type_ && move.new_type_ != Figure::TypeQueen )
        {
          if ( move.new_type_ != Figure::TypeKnight )
            continue;

          int ki_pos = scontexts_[ictx].board_.kingPos(Figure::otherColor(scontexts_[ictx].board_.getColor()));
          int dir = scontexts_[ictx].board_.g_figureDir->dir(Figure::TypeKnight, scontexts_[ictx].board_.color_, move.to_, ki_pos);
          if ( dir < 0 )
            continue;

          if ( scontexts_[ictx].board_.see(move) < 0 )
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

    Board board0(scontexts_[ictx].board_);

    if ( !scontexts_[ictx].board_.validateMove(move) )
    {
      THROW_IF( true, "illegal move generated by escape generator" );
    }

    scontexts_[ictx].board_.makeMove(move);
    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();

    THROW_IF( board0 != scontexts_[ictx].board_, "board unmake wasn't correct applied" );

    scontexts_[ictx].board_.verifyMasks();

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
void Player::verifyChecksGenerator(int ictx)
{
  Move killer;
  killer.clear();
  MovesGenerator mg(scontexts_[ictx].board_, killer);
  ChecksGenerator ckg(scontexts_[ictx].board_);
  ckg.generate();

  Move legal[Board::MovesMax], checks[Board::MovesMax];
  int n = 0, m = 0;

  int num = 0;
  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    num++;
    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    if ( move.capture_ || move.new_type_ )
      continue;

    Board board0(scontexts_[ictx].board_);

    scontexts_[ictx].board_.makeMove(move);

    if ( scontexts_[ictx].board_.underCheck() )
      legal[n++] = move;

    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    THROW_IF( board0 != scontexts_[ictx].board_, "board unmake wasn't correct applied" );
    scontexts_[ictx].board_.verifyMasks();
  }

  THROW_IF( mg.count() != num, "not all moves were enumerated" );

  for (int i = 0; i < ckg.count(); ++i)
  {
    const Move & move = ckg[i];
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    Board board0(scontexts_[ictx].board_);

    scontexts_[ictx].board_.makeMove(move);

    THROW_IF( !scontexts_[ictx].board_.underCheck(), "non checking move" );

    checks[m++] = move;

    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    THROW_IF( board0 != scontexts_[ictx].board_, "board unmake wasn't correct applied" );
    scontexts_[ictx].board_.verifyMasks();
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
      scontexts_[ictx].board_.toFEN(fen);
      ChecksGenerator ckg1(scontexts_[ictx].board_);
      ckg1.generate();
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

    if ( !found )
    {
      char fen[256];
      scontexts_[ictx].board_.toFEN(fen);
      ChecksGenerator ckg1(scontexts_[ictx].board_);
      ckg1.generate();
    }

    THROW_IF( !found, "some invalid check was generated" );
  }

  for (int i = 0; i < m; ++i)
  {
    const Move & move = checks[i];
    for (int j = i+1; j < m; ++j)
    {
      if ( move == checks[j] )
      {
        char fen[256];
        scontexts_[ictx].board_.toFEN(fen);
        ChecksGenerator ckg1(scontexts_[ictx].board_);
        ckg1.generate();
        THROW_IF( true, "duplicate move generated by checks generator" );
      }
    }
  }
}
#endif
//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_CAPS_GENERATOR
void Player::verifyCapsGenerator(int ictx)
{
  Move hmove, killer;
  hmove.clear();
  killer.clear();
  MovesGenerator mg(scontexts_[ictx].board_, killer);
  CapsGenerator cg(hmove, scontexts_[ictx].board_);

  Figure::Color ocolor = Figure::otherColor(scontexts_[ictx].board_.getColor());

  Move legal[Board::MovesMax], caps[Board::MovesMax];
  int n = 0, m = 0;

  int num = 0;
  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    num++;
    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    Board board0(scontexts_[ictx].board_);

    scontexts_[ictx].board_.makeMove(move);
    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    THROW_IF( board0 != scontexts_[ictx].board_, "board unmake wasn't applied correctly" );
    scontexts_[ictx].board_.verifyMasks();

    if ( move.new_type_ == Figure::TypeRook || move.new_type_ == Figure::TypeBishop )
      continue;

    if ( move.capture_ && (move.new_type_ == Figure::TypeQueen || !move.new_type_) )
    {
      legal[n++] = move;
      continue;
    }

    if ( move.new_type_ == Figure::TypeQueen )
    {
      legal[n++] = move;
      continue;
    }
    
    if ( move.new_type_ == Figure::TypeKnight )
    {
      if ( scontexts_[ictx].board_.g_movesTable->caps(Figure::TypeKnight, move.to_) & scontexts_[ictx].board_.fmgr().king_mask(ocolor) )
        legal[n++] = move;
    }
  }

  THROW_IF( num != mg.count(), "not all moves were enumerated" );

  for (int i = 0; i < cg.count(); ++i)
  {
    const Move & cap = cg[i];
    if ( !cap )
      break;

    if ( !scontexts_[ictx].board_.validateMove(cap) )
      continue;

    Board board0(scontexts_[ictx].board_);

    scontexts_[ictx].board_.makeMove(cap);
    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    THROW_IF( board0 != scontexts_[ictx].board_, "board unmake wasn't applied correctly" );
    scontexts_[ictx].board_.verifyMasks();

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

    if ( !found )
    {
      char fen[256];
      scontexts_[ictx].board_.toFEN(fen);
      CapsGenerator cg1(hmove, scontexts_[ictx].board_);
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
    if ( !found )
    {
      char fen[256];
      scontexts_[ictx].board_.toFEN(fen);
      CapsGenerator cg1(hmove, scontexts_[ictx].board_);
    }

    THROW_IF( !found, "some invalid capture was generated" );
  }
}
#endif


#ifdef VERIFY_FAST_GENERATOR
void Player::verifyFastGenerator(int ictx, const Move & hmove, const Move & killer)
{
  MovesGenerator mg(scontexts_[ictx].board_, killer);
  FastGenerator fg(scontexts_[ictx].board_, hmove, killer);

  Move legal[Board::MovesMax];
  Move fast[Board::MovesMax];
  int n = 0, m = 0;

  for ( ;; )
  {
    Move & move = mg.move();
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    Board board0(scontexts_[ictx].board_);
    scontexts_[ictx].board_.makeMove(move);

    scontexts_[ictx].board_.verifyMasks();
    scontexts_[ictx].board_.unmakeMove();
    THROW_IF( board0 != scontexts_[ictx].board_, "board unmake wasn't applied correctly" );
    scontexts_[ictx].board_.verifyMasks();

    if ( move.new_type_ && move.new_type_ != Figure::TypeQueen )
    {
      if ( move.new_type_ != Figure::TypeKnight )
        continue;

      int ki_pos = scontexts_[ictx].board_.kingPos(Figure::otherColor(scontexts_[ictx].board_.getColor()));
      int dir = scontexts_[ictx].board_.g_figureDir->dir(Figure::TypeKnight, scontexts_[ictx].board_.color_, move.to_, ki_pos);
      if ( dir < 0 )
        continue;
    }

    legal[n++] = move;
  }

  for ( ;; )
  {
    Move & move = fg.move();
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

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
      scontexts_[ictx].board_.toFEN(str);
      FastGenerator fg1(scontexts_[ictx].board_, hmove, killer);
      while ( fg1.move() );
    }

    THROW_IF( !found, "move wasn't generated by fast generator" );
  }

  for (int i = 0; i < m; ++i)
  {
    Move & move = fast[i];
    bool found = false;
    for (int j = 0; !found && j < n; ++j)
    {
      if ( move == legal[j] )
        found = true;
    }

    if ( !found )
    {
      char str[256];
      scontexts_[ictx].board_.toFEN(str);
      FastGenerator fg1(scontexts_[ictx].board_, hmove, killer);
      while ( fg1.move() );
    }

    THROW_IF( !found, "odd move was generated by fast generator" );
  }

  for (int i = 0; i < m; ++i)
  {
    for (int j = i+1; j < m; ++j)
    {
      if ( fast[i] == fast[j] )
      {
        char str[256];
        scontexts_[ictx].board_.toFEN(str);
        FastGenerator fg1(scontexts_[ictx].board_, hmove, killer);
        while ( fg1.move() );
      }
      THROW_IF( fast[i] == fast[j], "duplicate found in fast generator" );
    }
  }
}
#endif


#ifdef VERIFY_TACTICAL_GENERATOR
void Player::verifyTacticalGenerator(int ictx)
{
  Figure::Type minimalType = Figure::TypePawn;
  MovesGenerator mg(scontexts_[ictx].board_);
  TacticalGenerator tg(scontexts_[ictx].board_, minimalType, 0);

  Move legal[Board::MovesMax], tactical[Board::MovesMax];
  int n = 0, m = 0;

  for ( ;; )
  {
    Move & move = mg.move();
    if ( !move )
      break;

    if ( !scontexts_[ictx].board_.validateMove(move) )
      continue;

    if ( scontexts_[ictx].board_.underCheck() )
    {
      legal[n++] = move;
      continue;
    }

    scontexts_[ictx].board_.makeMove(move);
    bool check = scontexts_[ictx].board_.underCheck();
    scontexts_[ictx].board_.unmakeMove();

    if ( !check && !move.capture_ && !move.new_type_ )
      continue;

    if ( move.new_type_ == Figure::TypeBishop || move.new_type_ == Figure::TypeRook )
      continue;

    int ss = scontexts_[ictx].board_.see(move);
    if ( ss < 0 )
      continue;

    if ( !check && move.new_type_ == Figure::TypeKnight )
      continue;

    if ( move.new_type_ == Figure::TypeKnight )
    {
      int ki_pos = scontexts_[ictx].board_.kingPos(Figure::otherColor(scontexts_[ictx].board_.getColor()));
      int dir = scontexts_[ictx].board_.g_figureDir->dir(Figure::TypeKnight, scontexts_[ictx].board_.color_, move.to_, ki_pos);
      if ( dir < 0 )
        continue;
    }

    legal[n++] = move;
  }

  for ( ;; )
  {
    Move & move = tg.next();
    if ( !move )
      break;

    if ( scontexts_[ictx].board_.validateMove(move) )
      tactical[m++] = move;
  }

  for (int i = 0; i < n; ++i)
  {
    Move & move = legal[i];
    bool found = false;
    for (int j = 0; !found && j < m; ++j)
    {
      if ( move == tactical[j] )
        found = true;
    }

    if ( !found )
    {
      char str[256];
      scontexts_[ictx].board_.toFEN(str);
      TacticalGenerator tg1(scontexts_[ictx].board_, minimalType, 0);
      while ( tg1.next() );
    }

    THROW_IF( !found, "move wasn't generated by tactical generator" );
  }

  for (int i = 0; i < m; ++i)
  {
    Move & move = tactical[i];
    bool found = false;
    for (int j = 0; !found && j < n; ++j)
    {
      if ( move == legal[j] )
        found = true;
    }

    if ( !found )
    {
      char str[256];
      scontexts_[ictx].board_.toFEN(str);
      TacticalGenerator tg1(scontexts_[ictx].board_, minimalType, 0);
      while ( tg1.next() );
    }

    THROW_IF( !found, "odd move was generated by tactical generator" );
  }

  for (int i = 0; i < m; ++i)
  {
    for (int j = i+1; j < m; ++j)
    {
      if ( tactical[i] == tactical[j] )
      {
        char str[256];
        scontexts_[ictx].board_.toFEN(str);
        TacticalGenerator tg1(scontexts_[ictx].board_, minimalType, 0);
        while ( tg1.next() );
      }
      THROW_IF( tactical[i] == tactical[j], "duplicate found in tactical generator" );
    }
  }
}
#endif

void Player::verifyGenerators(int ictx, const Move & hmove)
{

#ifdef VERIFY_ESCAPE_GENERATOR
  verifyEscapeGen(ictx, hmove);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  verifyChecksGenerator(ictx);
#endif

#ifdef VERIFY_CAPS_GENERATOR
  verifyCapsGenerator(ictx);
#endif

#ifdef VERIFY_FAST_GENERATOR
  Move killer(0);
  verifyFastGenerator(ictx, hmove, killer);
#endif

#ifdef VERIFY_TACTICAL_GENERATOR
  verifyTacticalGenerator(ictx);
#endif

}

void Player::enumerate()
{
  enumerate(sdata_.depth_);
}

void Player::enumerate(int depth)
{
}

//////////////////////////////////////////////////////////////////////////
/// for DEBUG
void Player::findSequence(int ictx, const Move & move, int ply, int depth, int counter, ScoreType alpha, ScoreType betta) const
{
  struct MOVE { int from_, to_; };
  bool identical = false;
  static MOVE sequence[] = {
    {52, 45},
    {9, 8},
     };

    if ( ply < sizeof(sequence)/sizeof(MOVE) && move.from_ == sequence[ply].from_ && move.to_ == sequence[ply].to_ )
    {
      for (int i = ply; i >= 0; --i)
      {
        identical = true;
        int j = i-ply;
        if ( j >= scontexts_[ictx].board_.halfmovesCount() )
          break;
        const UndoInfo & undo = scontexts_[ictx].board_.undoInfoRev(j);
        if ( undo.from_ != sequence[i].from_ || undo.to_ != sequence[i].to_ )
        {
          identical = false;
          break;
        }
      }
    }

    if ( identical )
    {
      if ( sdata_.depth_ == 5 && ply == 1 )
      {
        int ttt = 0;
      }
      //std::stringstream sstm;
      //Board::save(scontexts_[ictx].board_, sstm, false);
      //std::ofstream ofs("D:\\Projects\\git_tests\\temp\\report.txt", std::ios_base::app);
      //ofs << "PLY: " << ply << std::endl;
      //std::string s = sstm.str();
      //ofs << s;
      //ofs << "depth_ = " << sdata_.depth_ << "; depth = " << depth << "; ply = " << ply << "; alpha = " << alpha << "; betta = " << betta << "; counter = " << counter << std::endl;
      //ofs << "===================================================================" << std::endl << std::endl;
    }
}
