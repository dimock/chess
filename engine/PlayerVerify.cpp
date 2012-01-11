
#include "Player.h"
#include "MovesGenerator.h"

//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_ESCAPE_GENERATOR
void Player::verifyEscapeGen(int depth, int ply, ScoreType alpha, ScoreType betta)
{
  int counter = 0;
  if ( board_.getState() == Board::UnderCheck )
  {
    EscapeGenerator eg(board_, depth, ply, *this, alpha, betta, counter);
    MovesGenerator mg(board_, depth, ply, this, alpha, betta, counter, false, false);

    Move legal[Board::MovesMax];
    int num = 0;

    for ( ;; )
    {
      const Move & move = mg.move();
      if ( !move )
        break;

      bool valid = false;
      Board board0(board_);

      if ( board_.makeMove(move) )
      {
        legal[num++] = move;
        valid = true;
      }

      board_.verifyMasks();
      board_.unmakeMove();

      THROW_IF( board0 != board_, "board unmake wasn't correct applied" );

      board_.verifyMasks();

      if ( valid && !eg.find(move) )
      {
        EscapeGenerator eg1(board_, depth, ply, *this, alpha, betta, counter);
        THROW_IF( true, "some legal escape from check wasn't generated" );
      }
    }

    THROW_IF(eg.count() + counter != num, "number of escape moves isn't correct");

    for ( ;; )
    {
      const Move & move = eg.escape();
      if ( !move )
        break;

      Board board0(board_);

      if ( !board_.makeMove(move) )
      {
        THROW_IF( true, "illegal move generated by escape generator" );
      }

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

      if ( !found )
      {
        THROW_IF( true, "move from escape generator isn't found in the legal moves list" );
      }
    }
  }
}
#endif

//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_CHECKS_GENERATOR
void Player::verifyChecksGenerator(int depth, int ply, ScoreType alpha, ScoreType betta, Figure::Type minimalType)
{
  int counter = 0;
  MovesGenerator mg(board_, depth, ply, this, alpha, betta, counter, false, false);
  CapsGenerator cg(board_, minimalType, ply, *this, alpha, betta, counter);
  ChecksGenerator ckg(&cg, board_, ply, *this, alpha, betta, minimalType, counter);

  Move legal[Board::MovesMax], checks[Board::MovesMax];
  int n = 0, m = 0;

  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    Board board0(board_);

    if ( board_.makeMove(move) && board_.getState() == Board::UnderCheck )
    {
      if ( move.new_type_ == Figure::TypeQueen || !move.new_type_ )
        legal[n++] = move;
      else if ( move.new_type_ == Figure::TypeKnight && board_.getNumOfChecking() == 1 )
      {
        const Field & ffield = board_.getField(move.to_);
        THROW_IF( ffield.color() == board_.color_ || ffield.type() != Figure::TypeKnight, "invalid color or type of promoted knight" );
        const Figure & knight = board_.getFigure(Figure::otherColor(board_.color_), ffield.index());
        THROW_IF( knight.getType() != Figure::TypeKnight || knight.getColor() != Figure::otherColor(board_.color_), "invalid promotion to knight in check generator" );
        const Figure & oking = board_.getFigure(board_.color_, Board::KingIndex);
        int dir = board_.g_figureDir->dir(knight, oking.where());
        if ( dir >= 0 )
          legal[n++] = move;
      }
    }

    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't correct applied" );
    board_.verifyMasks();
  }

  for ( ;; )
  {
    const Move & move = cg.capture();
    if ( !move )
      break;

    Board board0(board_);

    if ( board_.makeMove(move) && board_.getState() == Board::UnderCheck )
      checks[m++] = move;

    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't correct applied" );
    board_.verifyMasks();
  }

  for ( ;; )
  {
    const Move & move = ckg.check();
    if ( !move )
      break;

    Board board0(board_);

    if ( board_.makeMove(move) && board_.getState() == Board::UnderCheck )
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
      ChecksGenerator ckg2(&cg, board_, ply, *this, alpha, betta, minimalType, counter);
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
      int ttt = 0;
      char fen[256];
      board_.toFEN(fen);
      ChecksGenerator ckg2(&cg, board_, ply, *this, alpha, betta, minimalType, counter);
    }

    THROW_IF( !found, "some invalid check was generated" );
  }
}
#endif
//////////////////////////////////////////////////////////////////////////
#ifdef VERIFY_CAPS_GENERATOR
void Player::verifyCapsGenerator(int ply, ScoreType alpha, ScoreType betta, int delta)
{

  Figure::Type minimalType = Figure::TypePawn;
  if ( delta > Figure::figureWeight_[Figure::TypeRook] )
    minimalType = Figure::TypeQueen;
  else if ( delta > Figure::figureWeight_[Figure::TypeBishop] )
    minimalType = Figure::TypeRook;
  else if ( delta > Figure::figureWeight_[Figure::TypeKnight] )
    minimalType = Figure::TypeBishop;
  else if ( delta > Figure::figureWeight_[Figure::TypePawn] )
    minimalType = Figure::TypeKnight;

  int counter = 0;
  int depth = 0;
  MovesGenerator mg(board_, depth, ply, this, alpha, betta, counter, false, false);
  CapsGenerator cg(board_, minimalType, ply, *this, alpha, betta, counter);

  Move legal[Board::MovesMax], caps[Board::MovesMax];
  int n = 0, m = 0;

  for ( ;; )
  {
    const Move & move = mg.move();
    if ( !move )
      break;

    Board board0(board_);

    bool valid = false;
    if ( board_.makeMove(move) )
      valid = true;

    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't applied correctly" );
    board_.verifyMasks();

    if ( valid && (!move.new_type_ || Figure::TypeQueen == move.new_type_) )
    {
      if ( move.rindex_ >= 0 )
      {
        const Figure & rfig = board_.getFigure(Figure::otherColor(board_.getColor()), move.rindex_);
        THROW_IF( !rfig, "no figure to capture" );
        if ( rfig.getType() >= minimalType || move.new_type_ == Figure::TypeQueen )
          legal[n++] = move;
      }
      else if ( move.new_type_ == Figure::TypeQueen && minimalType <= Figure::TypeQueen )
        legal[n++] = move;
    }
  }

  for ( ;; )
  {
    const Move & cap = cg.capture();
    if ( !cap )
      break;

    Board board0(board_);

    if ( board_.makeMove(cap) )
      caps[m++] = cap;

    board_.verifyMasks();
    board_.unmakeMove();
    THROW_IF( board0 != board_, "board unmake wasn't applied correctly" );
    board_.verifyMasks();

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
      board_.toFEN(fen);
      CapsGenerator cg2(board_, minimalType, ply, *this, alpha, betta, counter);
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
      board_.toFEN(fen);
      CapsGenerator cg2(board_, minimalType, ply, *this, alpha, betta, counter);
    }
    THROW_IF( !found, "some invalid capture was generated" );
  }
}
#endif