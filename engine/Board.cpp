#include "Board.h"
#include "fpos.h"
#include "FigureDirs.h"

// static data
char Board::fen_[FENsize];
const char * Board::stdFEN_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

Board::Board() :
  g_moves(0),
  g_movesTable(0),
  g_figureDir(0),
  g_pawnMasks(0),
  g_betweenMasks(0),
  g_deltaPosCounter(0),
  g_distanceCounter(0)
{
  clear();
}

void Board::clear()
{
  fmgr_.clear();

  // clear global FEN
  fen_[0] = 0;

  castle_index_[0][0] = castle_index_[0][1] = false;
  castle_index_[1][0] = castle_index_[1][1] = false;
  castle_[0] = castle_[1] = 0;
  checking_[0] = checking_[1] = 0;
  can_win_[0] = can_win_[1] = true;
  checkingNum_ = 0;
  en_passant_ = -1;
  state_ = Invalid;
  color_ = Figure::ColorBlack;
  fiftyMovesCount_ = 0;
  movesCounter_ = 1;
  halfmovesCounter_ = 0;
  stages_[0] = stages_[1] = 0;

  for (int i = 0; i < NumOfFields; ++i)
    fields_[i].clear();

  for (int i = 0; i < 2; ++i)
  {
    for (int j = 0; j < NumOfFigures; ++j)
      figures_[i][j].clear();
  }
}

bool Board::initEmpty(Figure::Color color)
{
  clear();
  color_ = color;
  return true;
}

/* rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 */
bool Board::fromFEN(const char * fen)
{
  clear();

  if ( !fen )
    fen = stdFEN_;

  // trim left
  const char * sepr =" \t\r\n";
  for ( ; *fen && strchr(sepr, *fen); ++fen);
  
  // copy
  strncpy(fen_, fen, FENsize);
  
  // trim right
  int n = strlen(fen_) - 1;
  for (; n >= 0 && strchr(sepr, fen_[n]); --n);
  fen_[n+1] = 0;

  const char * s = fen;
  int x = 0, y = 7;
  for ( ; s && y >= 0; ++s)
  {
    char c = *s;
    if ( '/' == c )
      continue;
    else if ( c >= '0' && c <= '9' )
    {
      int dx = c - '0';
      x += dx;
      if ( x > 7 )
      {
        --y;
        x = 0;
      }
    }
    else
    {
      Figure::Type  ftype = Figure::TypeNone;
      Figure::Color color = Figure::ColorBlack;

      if ( c >= 'A' && c <= 'Z' )
        color = Figure::ColorWhite;

      switch ( c )
      {
      case 'r':
      case 'R':
        ftype = Figure::TypeRook;
        break;

      case 'p':
      case 'P':
        ftype = Figure::TypePawn;
        break;

      case 'n':
      case 'N':
        ftype = Figure::TypeKnight;
        break;

      case 'b':
      case 'B':
        ftype = Figure::TypeBishop;
        break;

      case 'q':
      case 'Q':
        ftype = Figure::TypeQueen;
        break;

      case 'k':
      case 'K':
        ftype = Figure::TypeKing;
        break;
      }

      if ( !ftype )
        return false;

      bool firstStep = false;
      if ( Figure::TypePawn == ftype && (6 == y && Figure::ColorBlack == color || 1 == y && Figure::ColorWhite == color) )
        firstStep = true;

      Figure fig(ftype, color, x, y, firstStep);

      addFigure(fig);

      if ( ++x > 7 )
      {
        --y;
        x = 0;
      }
    }
  }

  int i = 0;
  for ( ; *s; )
  {
    char c = *s;
    if ( c <= ' ' )
    {
      ++s;
      continue;
    }

    if ( 0 == i ) // process move color
    {
      if ( 'w' == c )
        color_ = Figure::ColorWhite;
      else if ( 'b' == c )
        color_ = Figure::ColorBlack;
      else
        return false;
      ++i;
      ++s;
      continue;
    }

    if ( 1 == i ) // process castling possibility
    {
      if ( '-' == c )
      {
        ++i;
        ++s;
        continue;
      }

      for ( ; *s && *s > 32; ++s)
      {
        Field fk, fr;

        switch ( *s )
        {
        case 'k':
          fk = getField(60);
          if ( fk.type() != Figure::TypeKing || fk.color() != Figure::ColorBlack )
            return false;
          fr = getField(63);
          if ( fr.type() != Figure::TypeRook || fr.color() != Figure::ColorBlack )
            return false;
          castle_index_[0][0] = true;
          break;

        case 'K':
          fk = getField(4);
          if ( fk.type() != Figure::TypeKing || fk.color() != Figure::ColorWhite )
            return false;
          fr = getField(7);
          if ( fr.type() != Figure::TypeRook || fr.color() != Figure::ColorWhite )
            return false;
          castle_index_[1][0] = true;
          break;

        case 'q':
          fk = getField(60);
          if ( fk.type() != Figure::TypeKing || fk.color() != Figure::ColorBlack )
            return false;
          fr = getField(56);
          if ( fr.type() != Figure::TypeRook || fr.color() != Figure::ColorBlack )
            return false;
          castle_index_[0][1] = true;
          break;

        case 'Q':
          fk = getField(4);
          if ( fk.type() != Figure::TypeKing || fk.color() != Figure::ColorWhite )
            return false;
          fr = getField(0);
          if ( fr.type() != Figure::TypeRook || fr.color() != Figure::ColorWhite )
            return false;
          castle_index_[1][1] = true;
          break;

        default:
          return false;
        }

        Figure & king = getFigure(fk.color(), fk.index());
        Figure & rook = getFigure(fr.color(), fr.index());

        king.setFirstStep(true);
        rook.setFirstStep(true);
      }

      ++i;
      continue;
    }

    if ( 2 == i )
    {
      ++i;
      char cx = *s++;
      if ( !*s )
        return false;

      if ( '-' == cx ) // no en-passant pawn
        continue;

      char cy = *s++;
      if ( color_ )
        cy--;
      else
        cy++;

      Index pos(cx, cy);

      Field & fp = getField(pos);
      if ( fp.type() != Figure::TypePawn || fp.color() == color_ )
        return false;

      en_passant_ = fp.index();
      continue;
    }

    if ( 3 == i ) // fifty moves rule
    {
      char str[4];
      str[0] = 0;
      for (int j = 0; *s && j < 4; ++j, ++s)
      {
        if ( *s > 32 )
          str[j] = *s;
        else
        {
          str[j] = 0;
          break;
        }
      }

      fiftyMovesCount_ = atoi(str);
      ++i;
      continue;
    }

    if ( 4 == i ) // moves counter
    {
      movesCounter_ = atoi(s);
      break;
    }
  }

  if ( !invalidate() )
    return false;

  return true;
}

//////////////////////////////////////////////////////////////////////////
/// verification of move/unmove methods; use it for debug only
bool Board::operator != (const Board & other) const
{
  const char * buf0 = reinterpret_cast<const char*>(this);
  const char * buf1 = reinterpret_cast<const char*>(&other);

  for (int i = 0; i < sizeof(Board); ++i)
  {
    if ( buf0[i] != buf1[i] )
      return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////
bool Board::invalidate()
{
  Figure::Color ocolor = Figure::otherColor(color_);

  Figure & king1 = getFigure(color_, KingIndex);
  Figure & king2 = getFigure(ocolor, KingIndex);

  state_ = Ok;

  if ( isAttacked(color_, king2.where()) )
  {
    state_ = Invalid;
    return false;
  }

  if ( isAttacked(ocolor, king1.where()) )
    state_ = UnderCheck;

  verifyChessDraw();

  if ( drawState() )
    return true;

  int cnum = findCheckingFigures(ocolor, king1.where());
  if ( cnum > 2 )
  {
    state_ = Invalid;
    return false;
  }
  else if ( cnum > 0 )
  {
    state_ = UnderCheck;
  }

  // setup stages for each color
  for (int c = 0; c < 2; ++c)
  {
    Figure::Color color =  (Figure::Color)c;
    Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
    stages_[color] = 0;
    if ( !( (fmgr_.queens(ocolor) > 0 && fmgr_.rooks(ocolor)+fmgr_.knights(ocolor)+fmgr_.bishops(ocolor) > 0) ||
            (fmgr_.rooks(ocolor) > 1 && fmgr_.bishops(ocolor) + fmgr_.knights(ocolor) > 1) ||
            (fmgr_.rooks(ocolor) > 0 && ((fmgr_.bishops_w(ocolor) > 0 && fmgr_.bishops_b(ocolor) > 0) || (fmgr_.bishops(ocolor) + fmgr_.knights(ocolor) > 2))) ) )
    {
      stages_[color] = 1;
    }
  }

  verifyState();

  return true;
}

// verify if there is draw or mat
void Board::verifyState()
{
#ifndef NDEBUG
  Board board0(*this);
#endif

  Move moves[Board::MovesMax];
  int num = generateMoves(moves);
  bool found = false;
  for (int i = 0; !found && i < num; ++i)
  {
    const Move & m = moves[i];
    if ( makeMove(m) )
      found = true;

    unmakeMove();

    THROW_IF(board0 != *this, "board is not restored by undo move method");
  }

  if ( !found )
  {
    setNoMoves();

    // update move's state because it is last one
    if ( halfmovesCounter_ > 0 )
    {
      MoveCmd & move = getMove(halfmovesCounter_-1);
      move.state_ = state_;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

void Board::setFigure(const Figure & fig)
{
  getFigure(fig.getColor(), fig.getIndex()) = fig;
  fields_[fig.where()].set(fig);
  fmgr_.incr(fig);
}

bool Board::addFigure(const Figure & fig)
{
  if ( !fig || fig.where() < 0 )
    return false;

  Field & field = getField(fig.where());
  if ( field )
    return false;

  if ( Figure::TypePawn == fig.getType() )
  {
    int j = fig.where() & 7;
    Figure & f = getFigure(fig.getColor(), j);
    if ( !f )
    {
      f = fig;
      f.setIndex(j);
      field.set(f);
      fmgr_.incr(f);
      return true;
    }
  }

  int jfrom = -1, jto = -1;
  switch ( fig.getType() )
  {
  case Figure::TypePawn:
    jfrom = PawnIndex;
    jto = KnightIndex;
    break;

  case Figure::TypeKnight:
    jfrom = KnightIndex;
    jto = BishopIndex;
    break;

  case Figure::TypeBishop:
    jfrom = BishopIndex;
    jto = RookIndex;
    break;

  case Figure::TypeRook:
    jfrom = RookIndex;
    jto = QueenIndex;
    break;

  case Figure::TypeQueen:
    jfrom = QueenIndex;
    jto = KingIndex;
    break;

  case Figure::TypeKing:
    jfrom = KingIndex;
    jto = NumOfFigures;
    break;
  }

  for (int i = 0; i < 2; ++i)
  {
    for (int j = jfrom; j < jto; ++j)
    {
      Figure & f = getFigure(fig.getColor(), j);
      if ( !f )
      {
        f = fig;
        f.setIndex(j);
        field.set(f);
        fmgr_.incr(f);
        return true;
      }
    }

    // if we haven't found empty slot yet, lets try to put anywhere :)
    jfrom = PawnIndex;
    jto = KingIndex;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////
bool Board::load(Board & board, std::istream & is)
{
  const int N = 1024;
  char str[N];
  const char * sepr  = " \t\n\r";
  const char * strmv = "123456789abcdefgh+-OxPNBRQK=#";
  bool fen_expected = false, fen_init = false;

  const int ML = 16;
  char smoves[GameLength][ML];
  int  hmovesN = 0;

  bool annot = false;
  bool stop = false;
  while ( !stop && is.getline(str, N-1, '\n') )
  {
    bool skip = false;
    char * s = str;
    for ( ; *s && !skip; ++s)
    {
      // skip separators
      if ( strchr(sepr, *s) )
        continue;

      // skip annotations {}
      if ( '{' == *s )
        annot = true;

      if ( annot && '}' != *s )
        continue;

      annot = false;

      // skip comment string
      if ( '%' == *s )
      {
        skip = true;
        break;
      }

      if ( '.' == *s ) // skip .
        continue;
      else if ( isdigit(*s) && (*(s+1) && (isdigit(*(s+1)) || strchr(sepr, *(s+1)) || '.' == *(s+1))) ) // skip move number
      {
        for ( ;*s && isdigit(*s); ++s);
        if ( !*s )
          skip = true;
      }
      else if ( isalpha(*s) || (isdigit(*s) && (*(s+1) && isalpha(*(s+1)))) ) // read move
      {
        int i = 0;
        for ( ;*s && i < ML && strchr(strmv, *s); ++s)
          smoves[hmovesN][i++] = *s;
        if ( i < ML )
          smoves[hmovesN][i] = 0;
        hmovesN++;
        if ( !*s )
          skip = true;
      }
      else if ( '*' == *s || isdigit(*s) && (*(s+1) && '-' == *(s+1)) ) // end game
      {
        skip = true;
        stop = true;
      }
      else if ( '[' == *s ) // read some extra params, like FEN
      {
        char param[N]= {'\0'};
        char value[N]= {'\0'};
        for ( ; *s && ']' != *s; ++s)
        {
          // skip separators
          if ( strchr(sepr, *s) )
            continue;

          // read param name
          if ( isalpha(*s) && !param[0] )
          {
            for (int i = 0; *s && i < N && !strchr(sepr, *s); ++s, ++i)
              param[i] = *s;
            continue;
          }

          // read value
          if ( '"' == *s && !value[0] )
          {
            s++;
            for (int i = 0; *s && i < N && '"' != *s; ++s, ++i)
              value[i] = *s;
            continue;
          }
        }

        if ( strcmp(param, "SetUp") == 0 && '1' == value[0] )
          fen_expected = true;
        else if ( strcmp(param, "FEN") == 0 && fen_expected )
        {
          if ( !board.fromFEN(value) )
            return false;
          fen_init = true;
        }
        skip = true;
      }
    }
  }

  if ( !fen_init )
    board.fromFEN(0);

  for (int i = 0; i < hmovesN; ++i)
  {
    const char * smove = smoves[i];
    Move move;
    if ( !parseSAN(board, smove, move) )
      return false;

    //char str[64];
    //moveToStr(move, str, false);

    //Move mv;
    //strToMove(str, board, mv);

    if ( !board.makeMove(move) )
    {
      board.unmakeMove();
      return false;
    }
  }

  board.verifyState();

  return true;
}

bool Board::save(const Board & board, std::ostream & os)
{
  if ( strcmp(fen_, stdFEN_) != 0 )
  {
    os << "[SetUp \"1\"] " << std::endl;
    os << "[FEN \"" << fen_ << "\"]" << std::endl;
  }

  const char * sres = "*";
  if ( board.getState() == Board::ChessMat )
  {
    if ( board.getColor() )
      sres = "0-1";
    else
      sres = "1-0";
  }
  else if ( board.drawState() )
    sres = "1/2-1/2";

  os << "[Result \"";
  os << sres;
  os << "\"]" << std::endl;

  Board sboard = board;
  MoveCmd tempMoves[Board::GameLength];

  int num = sboard.halfmovesCount();
  for (int i = 0; i < num; ++i)
    tempMoves[i] = board.getMove(i);
  sboard.set_moves(tempMoves);

  while ( sboard.halfmovesCount() > 0 )
    sboard.unmakeMove();

  for (int i = 0; i < num; ++i)
  {
    Figure::Color color = sboard.getColor();
    int moveNum = sboard.movesCount();

    char str[16];
    if ( !printSAN(sboard, i, str) )
      return false;

    if ( color || !i )
      os << moveNum << ". ";
    
    if ( !i && !color )
      os << " ... ";

    os << str;

    if ( !color )
      os << std::endl;
    else
      os << " ";
  }

  os << sres << std::endl;

  return true;
}

//////////////////////////////////////////////////////////////////////////
void Board::verifyMasks() const
{
  for (int c = 0; c < 2; ++c)
  {
    uint64 pawn_mask = 0ULL;
    uint64 knight_mask = 0ULL;
    uint64 bishop_mask = 0ULL;
    uint64 rook_mask = 0ULL;
    uint64 queen_mask = 0ULL;
    uint64 king_mask = 0ULL;
    uint64 all_mask = 0ULL;

    Figure::Color color = (Figure::Color)c;
    for (int i = 0; i < NumOfFigures; ++i)
    {
      const Figure & f = getFigure(color, i);
      if ( !f )
        continue;
      all_mask |= 1ULL << f.where();

      switch ( f.getType() )
      {
      case Figure::TypePawn:
        {
          int x = f.where() & 7;
          int y = f.where() >>3;
          pawn_mask |= 1ULL << ((x << 3) | y);
        }
        break;

      case Figure::TypeKnight:
        knight_mask |= 1ULL << f.where();
        break;

      case Figure::TypeBishop:
        bishop_mask |= 1ULL << f.where();
        break;

      case Figure::TypeRook:
        rook_mask |= 1ULL << f.where();
        break;

      case Figure::TypeQueen:
        queen_mask |= 1ULL << f.where();
        break;

      case Figure::TypeKing:
        king_mask |= 1ULL << f.where();
        break;
      }
    }

    THROW_IF( pawn_mask != fmgr_.pawn_mask(color), "pawn mask invalid" );
    THROW_IF( knight_mask != fmgr_.knight_mask(color), "knight mask invalid" );
    THROW_IF( bishop_mask != fmgr_.bishop_mask(color), "bishop mask invalid" );
    THROW_IF( rook_mask != fmgr_.rook_mask(color), "rook mask invalid" );
    THROW_IF( queen_mask != fmgr_.queen_mask(color), "queen mask invalid" );
    THROW_IF( king_mask != fmgr_.king_mask(color), "king mask invalid" );
    //THROW_IF( all_mask != fmgr_.mask(color), "invalid all figures mask" );

  }
}