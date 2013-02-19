/*************************************************************
  Board.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "Board.h"
#include "fpos.h"
#include "FigureDirs.h"
#include "MovesGenerator.h"
#include "Evaluator.h"

// static data
char Board::fen_[FENsize];
const char * Board::stdFEN_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int64 Board::ticks_;
int   Board::tcounter_;

Board::Board() :
  g_undoStack(0),
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

  can_win_[0] = can_win_[1] = true;
  en_passant_ = -1;
  state_ = Invalid;
  color_ = Figure::ColorBlack;
  fiftyMovesCount_ = 0;
  movesCounter_ = 1;
  halfmovesCounter_ = 0;
  repsCounter_ = 0;
  castling_ = 0;
  checkingNum_ = 0;

  for (int i = 0; i < NumOfFields; ++i)
    fields_[i].clear();
}

bool Board::initEmpty(Figure::Color color)
{
  clear();
  color_ = color;
  return true;
}

bool Board::canBeReduced() const
{
  const UndoInfo & undo = undoInfoRev(0);

  History & hist = MovesGenerator::history(undo.from_, undo.to_);

  return  ((hist.good()<<2) <= hist.bad()) &&
    !(undo.capture_ || undo.new_type_ > 0 || undo.threat_ || undo.castle_ || underCheck());
}

bool Board::isDangerPawn(const Move & move) const
{
  const Field & ffrom = getField(move.from_);
  if ( ffrom.type() != Figure::TypePawn )
    return false;

  if ( move.capture_ || move.new_type_ > 0 )
    return true;

  Figure::Color  color = color_;
  Figure::Color ocolor = Figure::otherColor(color);

  // attacking
  const uint64 & p_caps = g_movesTable->pawnCaps_o(ffrom.color(), move.to_);
  const uint64 & o_mask = fmgr_.mask(ocolor);
  if ( p_caps & o_mask )
    return true;

  //// becomes passed
  const uint64 & pmsk = fmgr_.pawn_mask_t(color);
  const uint64 & opmsk = fmgr_.pawn_mask_t(ocolor);
  const uint64 & passmsk = g_pawnMasks->mask_passed(color, move.to_);
  const uint64 & blckmsk = g_pawnMasks->mask_blocked(color, move.to_);

  if ( !(opmsk & passmsk) && !(pmsk & blckmsk) )
    return true;

  if ( see(move) >= 0 )
    return true;

  return false;
}

bool Board::isDangerQueen(const Move & move) const
{
  const Field & ffrom = getField(move.from_);
  if ( ffrom.type() != Figure::TypeQueen )
    return false;

  if ( move.capture_ )
    return true;

  Figure::Color  color = color_;
  Figure::Color ocolor = Figure::otherColor(color);

  int oki_pos = kingPos(ocolor);
  int dist = g_distanceCounter->getDistance(oki_pos, move.to_);
  if ( dist > 2 )
    return false;

  BitMask mask_all = fmgr().mask(Figure::ColorBlack) | fmgr().mask(Figure::ColorWhite);
  BitMask oki_caps = g_movesTable->caps(Figure::TypeKing, oki_pos);
  BitMask q_caps = g_movesTable->caps(Figure::TypeQueen, move.to_);
  BitMask attacked_mask = (oki_caps & q_caps) & ~mask_all;
  BitMask ki_moves = oki_caps & ~(mask_all | attacked_mask);
  int movesN = pop_count(ki_moves);
  if ( !attacked_mask || movesN > 2 )
    return false;

  if ( see(move) < 0 )
    return false;

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
  int n = (int)strlen(fen_) - 1;
  for (; n >= 0 && strchr(sepr, fen_[n]); --n);
  fen_[n+1] = 0;

  const char * s = fen;
  int x = 0, y = 7;
  for ( ; s && y >= 0; ++s)
  {
    char c = *s;
    if ( '/' == c )
      continue;
    else if ( isdigit(c) )
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

      if ( isupper(c) )
        color = Figure::ColorWhite;

      ftype = Figure::toFtype(toupper(c));

      if ( Figure::TypeNone == ftype )
        return false;

      Index p(x, y);
      addFigure(color, ftype, p);

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
          set_castling(Figure::ColorBlack, 0);
          fmgr_.hashCastling(Figure::ColorBlack, 0);
          break;

        case 'K':
          set_castling(Figure::ColorWhite, 0);
          fmgr_.hashCastling(Figure::ColorWhite, 0);
          break;

        case 'q':
          set_castling(Figure::ColorBlack, 1);
          fmgr_.hashCastling(Figure::ColorBlack, 1);
          break;

        case 'Q':
          set_castling(Figure::ColorWhite, 1);
          fmgr_.hashCastling(Figure::ColorWhite, 1);
          break;

        default:
          return false;
        }
      }

      ++i;
      continue;
    }

    if ( 2 == i )
    {
      ++i;
      char cx = *s++;
      if ( '-' == cx ) // no en-passant pawn
        continue;

      if ( !*s )
        return false;

      char cy = *s++;

      Index enpassant(cx, cy);

      if ( color_ )
        cy--;
      else
        cy++;

      Index pawn_pos(cx, cy);

      Field & fp = getField(pawn_pos);
      if ( fp.type() != Figure::TypePawn || fp.color() == color_ )
        return false;

      en_passant_ = enpassant;
      fmgr_.hashEnPassant(en_passant_, fp.color());
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
bool Board::toFEN(char * fen) const
{
  if ( !fen )
    return false;

  char * s = fen;

  // 1 - write figures
  for (int y = 7; y >= 0; --y)
  {
    int n = 0;
    for (int x = 0; x < 8; ++x)
    {
      Index idx(x, y);
      const Field & field = getField(idx);
      if ( !field )
      {
        ++n;
        continue;
      }

      if ( n > 0 )
      {
        *s++ = '0' + n;
        n = 0;
      }

      char c = fromFtype(field.type());
      if ( field.color() == Figure::ColorBlack )
        c = tolower(c);

      *s++ = c;
    }
    
    if ( n > 0 )
      *s++ = '0' + n;

    if ( y > 0 )
      *s++ = '/';
  }

  // 2 - color to move
  {
    *s++ = ' ';
    if ( Figure::ColorBlack == color_ )
      *s++ = 'b';
    else
      *s++ = 'w';
  }

  // 3 - castling possibility
  {
    *s++ = ' ';
	  if ( !castling() )
    {
      *s++ = '-';
    }
    else
    {
      if ( castling_K() )
      {
        if ( !verifyCastling(Figure::ColorWhite, 0) )
          return false;

        *s++ = 'K';
      }
      if ( castling_Q() )
      {
        if ( !verifyCastling(Figure::ColorWhite, 1) )
          return false;

        *s++ = 'Q';
      }
      if ( castling_k() )
      {
        if ( !verifyCastling(Figure::ColorBlack, 0) )
          return false;
        
        *s++ = 'k';
      }
      if ( castling_q() )
      {
        if ( !verifyCastling(Figure::ColorBlack, 1) )
          return false;

        *s++ = 'q';
      }
    }
  }

  {
    // 4 - en passant
    *s++ = ' ';
    if ( en_passant_ >= 0 )
    {
      Index ep_pos(en_passant_);

      int x = ep_pos.x();
      int y = ep_pos.y();

      if ( color_ )
        y--;
      else
        y++;

      Index pawn_pos(x, y);
      const Field & ep_field = getField(pawn_pos);
      if ( ep_field.color() == color_ || ep_field.type() != Figure::TypePawn )
        return false;

      char cx = 'a' + ep_pos.x();
      char cy = '1' + ep_pos.y();
      *s++ = cx;
      *s++ = cy;
    }
    else
      *s++ = '-';

    // 5 - fifty move rule
    {
      *s++ = ' ';
      char str[8];
      _itoa(fiftyMovesCount_, str, 10);
      char * sf = str;
      for ( ; *sf; ++sf, ++s)
        *s = *sf;
    }

    // 6 - moves counter
    {
      *s++ = ' ';
      char str[8];
      _itoa(movesCounter_, str, 10);
      char * sf = str;
      for ( ; *sf; ++sf, ++s)
        *s = *sf;
    }
  }

  // terminal
  *s++ = 0;

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

bool Board::verifyCastling(const Figure::Color c, int t) const
{
  if ( c && getField(4).type() != Figure::TypeKing )
    return false;

  if ( !c && getField(60).type() != Figure::TypeKing )
    return false;

  if ( c && t == 0 && getField(7).type() != Figure::TypeRook )
    return false;

  if ( c && t == 1 && getField(0).type() != Figure::TypeRook )
    return false;

  if ( !c && t == 0 && getField(63).type() != Figure::TypeRook )
    return false;

  if ( !c && t == 1 && getField(56).type() != Figure::TypeRook )
    return false;

  return true;
}

//////////////////////////////////////////////////////////////////////////
bool Board::invalidate()
{
  Figure::Color ocolor = Figure::otherColor(color_);

  int ki_pos = kingPos(color_);
  int oki_pos = kingPos(ocolor);

  state_ = Ok;

  if ( isAttacked(color_, oki_pos) )
  {
    state_ = Invalid;
    return false;
  }

  if ( isAttacked(ocolor, ki_pos) )
    state_ |= UnderCheck;

  verifyChessDraw();

  if ( drawState() )
    return true;

  int cnum = findCheckingFigures(ocolor, ki_pos);
  if ( cnum > 2 )
  {
    state_ = Invalid;
    return false;
  }
  else if ( cnum > 0 )
  {
    state_ |= UnderCheck;
  }

  verifyState();

  return true;
}

// verify is there draw or mat
void Board::verifyState()
{
  if ( matState() || drawState() )
    return;

#ifndef NDEBUG
  Board board0(*this);
#endif

  MovesGenerator mg(*this);
  bool found = false;
  for ( ; !found; )
  {
    const Move & m = mg.move();
    if ( !m )
      break;

    if ( validateMove(m) )
      found = true;
  }

  if ( !found )
  {
    setNoMoves();

    // update move's state because it is last one
    if ( halfmovesCounter_ > 0 )
    {
      UndoInfo & undo = undoInfo(halfmovesCounter_-1);
      undo.state_ = state_;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
bool Board::addFigure(const Figure::Color color, const Figure::Type type, int pos)
{
  if ( !type || (unsigned)pos > 63 )
    return false;

  Field & field = getField(pos);
  if ( field )
    return false;

  fields_[pos].set(color, type);
  fmgr_.incr(color, type, pos);

  return false;
}

//////////////////////////////////////////////////////////////////////////
bool Board::load(Board & board, std::istream & is)
{
  const int N = 1024;
  char str[N];
  const char * sepr  = " \t\n\r";
  const char * strmv = "123456789abcdefgh+-OxPNBRQKnul=#";
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

    if ( !board.validateMove(move) )
      return false;

    board.makeMove(move);
  }

  board.verifyState();

  return true;
}

bool Board::save(const Board & board, std::ostream & os, bool write_prefix)
{
  const char * sres = "*";
  if ( board.matState() )
  {
    if ( board.getColor() )
      sres = "0-1";
    else
      sres = "1-0";
  }
  else if ( board.drawState() )
    sres = "1/2-1/2";

  Board sboard = board;
  UndoInfo tempUndo[Board::GameLength];

  int num = sboard.halfmovesCount();
  for (int i = 0; i < num; ++i)
    tempUndo[i] = board.undoInfo(i);
  sboard.set_undoStack(tempUndo);

  while ( sboard.halfmovesCount() > 0 )
    sboard.unmakeMove();

  if ( write_prefix )
  {
    char fen[FENsize];
    if ( sboard.toFEN(fen) && strcmp(fen, stdFEN_) != 0 )
    {
      os << "[SetUp \"1\"] " << std::endl;
      os << "[FEN \"" << fen << "\"]" << std::endl;
    }

    os << "[Result \"";
    os << sres;
    os << "\"]" << std::endl;
  }

  for (int i = 0; i < num; ++i)
  {
    Figure::Color color = sboard.getColor();
    int moveNum = sboard.movesCount();

    // stash move
    Move move = board.undoInfo(i);

    char str[16];
    if ( !printSAN(sboard, move, str) )
      return false;

    // now apply move
    if ( !sboard.validateMove(move) )
      return false;

    sboard.makeMove(move);

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
    BitMask pawn_mask_o = 0ULL;
    BitMask pawn_mask_t = 0ULL;
    BitMask knight_mask = 0ULL;
    BitMask bishop_mask = 0ULL;
    BitMask rook_mask = 0ULL;
    BitMask queen_mask = 0ULL;
    BitMask king_mask = 0ULL;
    BitMask all_mask = 0ULL;

    Figure::Color color = (Figure::Color)c;
    for (int p = 0; p < NumOfFields; ++p)
    {
      const Field & field = getField(p);
      if ( !field || field.color() != color )
        continue;

      all_mask |= set_mask_bit(p);

      switch ( field.type() )
      {
      case Figure::TypePawn:
        {
          pawn_mask_t |= set_mask_bit(Index(p).transp());
          pawn_mask_o |= set_mask_bit(p);
        }
        break;

      case Figure::TypeKnight:
        knight_mask |= set_mask_bit(p);
        break;

      case Figure::TypeBishop:
        bishop_mask |= set_mask_bit(p);
        break;

      case Figure::TypeRook:
        rook_mask |= set_mask_bit(p);
        break;

      case Figure::TypeQueen:
        queen_mask |= set_mask_bit(p);
        break;

      case Figure::TypeKing:
        king_mask |= set_mask_bit(p);
        break;
      }
    }

    THROW_IF( pawn_mask_o != fmgr_.pawn_mask_o(color), "pawn mask invalid" );
    THROW_IF( pawn_mask_t != fmgr_.pawn_mask_t(color), "pawn mask invalid" );
    THROW_IF( knight_mask != fmgr_.knight_mask(color), "knight mask invalid" );
    THROW_IF( bishop_mask != fmgr_.bishop_mask(color), "bishop mask invalid" );
    THROW_IF( rook_mask != fmgr_.rook_mask(color), "rook mask invalid" );
    THROW_IF( queen_mask != fmgr_.queen_mask(color), "queen mask invalid" );
    THROW_IF( king_mask != fmgr_.king_mask(color), "king mask invalid" );
    THROW_IF( all_mask != fmgr_.mask(color), "invalid all figures mask" );

  }
}

//////////////////////////////////////////////////////////////////////////
// only for debugging purposes, saves complete board memory dump
void Board::save(const char * fname) const
{
  FILE * f = fopen(fname, "wb");
  if ( !f )
    return;
  fwrite((char*)this, sizeof(*this), 1, f);
  fwrite((char*)g_undoStack, sizeof(UndoInfo), GameLength, f);
  fclose(f);
}

void Board::load(const char * fname)
{
  FILE * f = fopen(fname, "rb");
  if ( !f )
    return;
  UndoInfo * undoStack = g_undoStack;
  const MovesTable * movesTable = g_movesTable;
  const FigureDir * figuresDir = g_figureDir;
  const PawnMasks * pawnMask = g_pawnMasks;
  const BetweenMask * betweenMask = g_betweenMasks;
  const DeltaPosCounter * deltaPos = g_deltaPosCounter;
  const DistanceCounter * distCounter = g_distanceCounter;

  fread((char*)this, sizeof(*this), 1, f);
  g_undoStack = undoStack;
  fread((char*)g_undoStack, sizeof(UndoInfo), GameLength, f);
  fclose(f);

  g_movesTable = movesTable;
  g_figureDir = figuresDir;
  g_pawnMasks = pawnMask;
  g_betweenMasks = betweenMask;
  g_deltaPosCounter = deltaPos;
  g_distanceCounter = distCounter;
}
