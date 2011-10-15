#pragma once

#include "BasicTypes.h"

#pragma pack (push, 1)

__declspec (align(1)) class Figure
{
#ifndef NDEBUG
public:
#endif

  // position evaluation. 0 - debut, 1 - endgame; color,type,pos
  static WeightType positionEvaluations_[2][8][64];

public:

  static WeightType figureWeight_[7]; // TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing
  static WeightType pawnGuarded_, pawnDoubled_, pawnIsolated_;
  static WeightType pawnPassed_[2][8];

  enum Weights { WeightDraw = 0, WeightMat = 32000 };
	enum Type    { TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing };
	enum Color   { ColorBlack, ColorWhite };

  static WeightType positionEvaluation(int stage, int color, int type, int pos);

  Figure();
	Figure(Type type, char x /* a - h */, int y /* 1 - 8 */, Color c, bool firstStep = false);

  void clear();

  bool operator == ( const Figure & other ) const;
  operator bool () const { return type_ != TypeNone; }

  Color getColor() const { return (Figure::Color)color_; }
  void  setType(Figure::Type type) { type_ = (Figure::Type)type; }
  Type  getType() const{ return (Figure::Type)type_; }
	void  setMoved() { firstStep_ = false; }
  bool  isFirstStep() const { return firstStep_; }
  void  setUnmoved() { firstStep_ = true; }
  int   getIndex() const { return index_; }
  void  setIndex(int idx) { index_ = idx; }
  int   where() const { return pos_; }
  int   go(int i) { return pos_ = (int8)i; }

  const char * name() const;

  static inline Figure::Color otherColor(Figure::Color color)
  {
    return (Figure::Color)((color + 1) & 1);
  }

protected:

  bool isDirValid(uint8 dir) const;

	Index   pos_;

  uint16 color_ : 1,
          type_ : 3,
          firstStep_ : 1,
          index_ : 4,
          reserved_ : 7;
};

__declspec (align(1)) class FiguresCounter
{
public:

  FiguresCounter() : pawns_(0), wbishops_(0), bbishops_(0), knights_(0), rooks_(0), queens_(0), count_(0), weight_(0)
  {
    for (int i = 0; i < 2; ++i)
      eval_[i] = 0;
  }

  void incr(const Figure & fig)
  {
    switch ( fig.getType() )
    {
    case Figure::TypePawn:
      pawns_++;
      break;

    case Figure::TypeBishop:
      if ( FPosIndexer::get(fig.where()).is_white() )
        wbishops_++;
      else
        bbishops_++;
      break;

    case Figure::TypeKnight:
      knights_++;
      break;

    case Figure::TypeRook:
      rooks_++;
      break;

    case Figure::TypeQueen:
      queens_++;
      break;

	  case Figure::TypeKing:
		  break;

    default:
      return;
    }

	  if ( fig.getType() != Figure::TypeKing )
	  {
		  count_++;
		  weight_ += Figure::figureWeight_[fig.getType()];
	  }

    eval_[0] += Figure::positionEvaluation(0, fig.getColor(), fig.getType(), fig.where());
    eval_[1] += Figure::positionEvaluation(1, fig.getColor(), fig.getType(), fig.where());

    THROW_IF(weight_ != pawns_*Figure::figureWeight_[Figure::TypePawn] + (wbishops_+bbishops_)*Figure::figureWeight_[Figure::TypeBishop] + knights_*Figure::figureWeight_[Figure::TypeKnight] + rooks_*Figure::figureWeight_[Figure::TypeRook] + queens_*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
    THROW_IF(count_ != pawns_ + wbishops_ + bbishops_ + knights_ + rooks_ + queens_, "invalid number of figures encountered");
  }

  void decr(const Figure & fig)
  {
    switch ( fig.getType() )
    {
    case Figure::TypePawn:
      pawns_--;
      THROW_IF(pawns_ > 8, "number of pawns is invalid");
      break;

    case Figure::TypeBishop:
      if ( FPosIndexer::get(fig.where()).is_white() )
      {
        wbishops_--;
        THROW_IF(wbishops_ > 9, "number of white-field bishops is invalid");
      }
      else
      {
        bbishops_--;
        THROW_IF(bbishops_ > 9, "number of black-field bishops is invalid");
      }
      break;

    case Figure::TypeKnight:
      knights_--;
      THROW_IF(knights_ > 10, "number of knights is invalid");
      break;

    case Figure::TypeRook:
      rooks_--;
      THROW_IF(rooks_ > 10, "number of rooks is invalid");
      break;

    case Figure::TypeQueen:
      queens_--;
      THROW_IF(queens_ > 9, "number of queens is invalid");
      break;

    case Figure::TypeKing:
      break;

    default:
      return;
    }

    if ( fig.getType() != Figure::TypeKing )
    {
      count_--;
      weight_ -= Figure::figureWeight_[fig.getType()];
    }

    eval_[0] -= Figure::positionEvaluation(0, fig.getColor(), fig.getType(), fig.where());
    eval_[1] -= Figure::positionEvaluation(1, fig.getColor(), fig.getType(), fig.where());;

    THROW_IF(weight_ != pawns_*Figure::figureWeight_[Figure::TypePawn] + (wbishops_+bbishops_)*Figure::figureWeight_[Figure::TypeBishop] + knights_*Figure::figureWeight_[Figure::TypeKnight] + rooks_*Figure::figureWeight_[Figure::TypeRook] + queens_*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
    THROW_IF(count_ != pawns_ + wbishops_ + bbishops_ + knights_ + rooks_ + queens_, "invalid number of figures encountered");
  }

  // fig in old position, to - it's new position
  void move(Figure & fig, int to)
  {
    eval_[0] -= Figure::positionEvaluation(0, fig.getColor(), fig.getType(), fig.where());
    eval_[0] += Figure::positionEvaluation(0, fig.getColor(), fig.getType(), to);

    eval_[1] -= Figure::positionEvaluation(1, fig.getColor(), fig.getType(), fig.where());
    eval_[1] += Figure::positionEvaluation(1, fig.getColor(), fig.getType(), to);

    fig.go(to);
  }

  int count() const { return count_; }
  int pawns() const { return pawns_; }
  int bishops_w() const { return wbishops_; }
  int bishops_b() const { return bbishops_; }
  int bishops() const { return bbishops_ + wbishops_; }
  int knights() const { return knights_; }
  int rooks() const { return rooks_; }
  int queens() const { return queens_; }
  WeightType weight() const { return weight_; }
  WeightType eval(int i) const { return eval_[i]; } // position

private:

  uint32 pawns_ : 4,
          wbishops_ : 4,
          bbishops_ : 4,
          knights_ : 4,
          rooks_ : 4,
          queens_ : 4,
          count_ : 4;

  WeightType weight_, eval_[2];
};

class FiguresManager
{
  static uint64 s_zobristCodes_[64*2*8];
  static uint64 s_zobristColor_;

public:

  FiguresManager() : hashCode_(0)
  {
    pawn_mask_[0] = pawn_mask_[1] = 0;
  }

  void incr(const Figure & fig)
  {
    fcounter_[fig.getColor()].incr(fig);
    const uint64 & uc = code(fig, fig.where());
    hashCode_ ^= uc;

    if ( fig.getType() != Figure::TypePawn )
      return;

    int x = fig.where() & 7;
    int y = fig.where() >> 3;
    int bit = y | (x << 3);

    pawn_mask_[fig.getColor()] |= 1ULL << bit;
  }

  void decr(const Figure & fig)
  {
    fcounter_[fig.getColor()].decr(fig);
    const uint64 & uc = code(fig, fig.where());
    hashCode_ ^= uc;

    if ( fig.getType() != Figure::TypePawn )
      return;

    int x = fig.where() & 7;
    int y = fig.where() >> 3;
    int bit = y | (x << 3);

    pawn_mask_[fig.getColor()] ^= 1ULL << bit;
    THROW_IF( pawn_mask_[fig.getColor()] & (1ULL << bit), "invalid pawn mask" );
  }

  void move(Figure & fig, int to)
  {
    int from = fig.where();

    const uint64 & uc0 = code(fig, from);
    const uint64 & uc1 = code(fig, to);
    hashCode_ ^= uc0;
    hashCode_ ^= uc1;
    fcounter_[fig.getColor()].move(fig, to);

    if ( fig.getType() != Figure::TypePawn )
      return;

    int xfrom = from & 7;
    int yfrom = from >> 3;
    int xto = to & 7;
    int yto = to >> 3;

    int bit_from = yfrom | (xfrom << 3);
    int bit_to = yto | (xto << 3);

    pawn_mask_[fig.getColor()] ^= 1ULL << bit_from;
    THROW_IF( pawn_mask_[fig.getColor()] & (1ULL << bit_from), "invalid pawn mask" );
    pawn_mask_[fig.getColor()] |= 1ULL << bit_to;
  }

  void hashFake(int fakePos, int color)
  {
    const uint64 & fakeCode = s_zobristCodes_[ (fakePos<<4) | (color<<3)/* | fig.getType() - use 0 instead*/ ];
    hashCode_ ^= fakeCode;
  }

  void hashCastling(int color)
  {
    const uint64 & castleCode = s_zobristCodes_[ (color<<3) | 7 ];
    hashCode_ ^= castleCode;
  }

  void hashColor()
  {
    hashCode_ ^= s_zobristColor_;
  }

  int count(Figure::Color color) const { return fcounter_[color].count(); }
  int count() const { return fcounter_[0].count() + fcounter_[1].count(); }
  int pawns(Figure::Color color) const { return fcounter_[color].pawns(); }
  int pawns() const { return pawns(Figure::ColorWhite) + pawns(Figure::ColorBlack); }
  int bishops_w(Figure::Color color) const { return fcounter_[color].bishops_w(); }
  int bishops_b(Figure::Color color) const { return fcounter_[color].bishops_b(); }
  int bishops(Figure::Color color) const { return fcounter_[color].bishops(); }
  int knights(Figure::Color color) const { return fcounter_[color].knights(); }
  int rooks(Figure::Color color) const { return fcounter_[color].rooks(); }
  int queens(Figure::Color color) const { return fcounter_[color].queens(); }
  WeightType weight(Figure::Color color) const { return fcounter_[color].weight(); }
  WeightType weight() const { return weight(Figure::ColorWhite) - weight(Figure::ColorBlack); }
  WeightType eval(Figure::Color color, int stage) const { return fcounter_[color].eval(stage); }
  WeightType eval(int stage) const { return fcounter_[Figure::ColorWhite].eval(stage) - fcounter_[Figure::ColorBlack].eval(stage); }
  const uint64 & hashCode() const { return hashCode_; }
  const uint64 & pawn_mask(Figure::Color color) const { return pawn_mask_[color]; }

private:

  const uint64 & code(const Figure & fig, int i) const
  {
    return s_zobristCodes_[ (i<<4) | (fig.getColor()<<3) | fig.getType() ];
  }

  uint64 pawn_mask_[2];
  FiguresCounter fcounter_[2];
  uint64 hashCode_;
};


#pragma pack (pop)
