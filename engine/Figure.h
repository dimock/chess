#pragma once

#include "BasicTypes.h"
#include "Helpers.h"

#pragma pack (push, 1)

__declspec (align(1)) class Figure
{
#ifndef NDEBUG
public:
#endif

  // position evaluation. 0 - debut, 1 - endgame; color,type,pos
  static ScoreType positionEvaluations_[2][8][64];
  static uint8 mirrorIndex_[64];

public:

  static ScoreType figureWeight_[7]; // TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing
  static ScoreType pawnGuarded_, pawnDoubled_, pawnIsolated_;
  static ScoreType pawnPassed_[2][8];

  enum Weights { WeightDraw = 0, WeightMat = 32000 };
  enum Type  : uint8  { TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing };
  enum Color : uint8  { ColorBlack, ColorWhite };

  static inline ScoreType positionEvaluation(uint8 stage, uint8 color, uint8 type, int8 pos)
  {
    THROW_IF( stage > 1 || color > 1 || type > 7 || pos < 0 || pos > 63, "invalid figure params" );

    //if ( color )
    //{
    //  int x = pos & 7;
    //  int y = 7 - (pos >> 3);
    //  pos = x | (y << 3);
    //}

    uint8 cmask = ((int8)(color << 7)) >> 7;
    uint8 icmask = ~cmask;
    uint8 i = (mirrorIndex_[pos] & cmask) | (pos | icmask);

    ScoreType e = positionEvaluations_[stage][type][i];
    return e;
  }

  Figure();
	Figure(Type type, Color c, int x, int y, bool firstStep = false);

  bool operator == ( const Figure & other ) const;
  operator bool () const { return type_ != TypeNone; }

  Color getColor() const { return (Figure::Color)color_; }
  void  setType(Figure::Type type) { type_ = (Figure::Type)type; }
  Type  getType() const{ return (Figure::Type)type_; }
	void  setMoved() { first_step_ = false; }
  bool  isFirstStep() const { return first_step_; }
  void  setFirstStep(bool first_step) { first_step_ = first_step; }
  int   getIndex() const { return index_; }
  void  setIndex(int8 idx) { index_ = idx; }
  int8  where() const { return pos_; }
  int8  go(int8 i) { return pos_ = i; }
  void  clear() { type_ = TypeNone; }

  const char * name() const;

  static inline Figure::Color otherColor(Figure::Color color)
  {
    return (Figure::Color)((color + 1) & 1);
  }

protected:

	Index pos_;
  int8  index_;
  int8  type_;
  uint8 color_ : 1,
        first_step_ : 1;
};

__declspec (align(1)) class FiguresCounter
{
public:

  FiguresCounter() : pawns_(0), wbishops_(0), bbishops_(0), knights_(0), rooks_(0), queens_(0), count_(0), weight_(0)
  {
    eval_[0] = 0;
    eval_[1] = 0;
  }

  void clear()
  {
    pawns_ = 0;
    wbishops_ = 0;
    bbishops_ = 0;
    knights_ = 0;
    rooks_ = 0;
    queens_ = 0;
    count_ = 0;
    weight_ = 0;
    eval_[0] = 0;
    eval_[1] = 0;
  }

  void incr(const Figure & fig)
  {
    switch ( fig.getType() )
    {
    case Figure::TypePawn:
      pawns_++;
      break;

    case Figure::TypeBishop:
      if ( FieldColors::isWhite(fig.where()) )
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
      if ( FieldColors::isWhite(fig.where()) )
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
  ScoreType weight() const { return weight_; }
  ScoreType eval(int i) const { return eval_[i]; } // position

private:

  uint8 pawns_;
  uint8 wbishops_;
  uint8 bbishops_;
  uint8 knights_;
  uint8 rooks_;
  uint8 queens_;
  uint8 count_;

  ScoreType weight_, eval_[2];
};

class FiguresManager
{
  static uint64 s_zobristCodes_[64*2*8];
  static uint64 s_zobristColor_;
  static uint64 s_zobristCastle_[2][2];
  static uint8  s_transposeIndex_[64];

public:

  FiguresManager() : hashCode_(0ULL)
  {
    pawn_mask_[0] = pawn_mask_[1] = 0ULL;
    mask_[0] = mask_[1] = 0ULL;
  }

  void clear()
  {
    hashCode_ = 0ULL;
    pawn_mask_[0] = pawn_mask_[1] = 0ULL;
    mask_[0] = mask_[1] = 0ULL;

    fcounter_[0].clear();
    fcounter_[1].clear();
  }

  void incr(const Figure & fig)
  {
    fcounter_[fig.getColor()].incr(fig);
    const uint64 & uc = code(fig, fig.where());
    hashCode_ ^= uc;

    mask_[fig.getColor()] |= 1ULL << fig.where();

    if ( fig.getType() == Figure::TypePawn )
    {
      int bit = s_transposeIndex_[fig.where()];
      pawn_mask_[fig.getColor()] |= 1ULL << bit;
    }
  }

  void decr(const Figure & fig)
  {
    fcounter_[fig.getColor()].decr(fig);
    const uint64 & uc = code(fig, fig.where());
    hashCode_ ^= uc;

    mask_[fig.getColor()] ^= 1ULL << fig.where();

    if ( fig.getType() == Figure::TypePawn )
    {
      int bit = s_transposeIndex_[fig.where()];
      pawn_mask_[fig.getColor()] ^= 1ULL << bit;
      THROW_IF( pawn_mask_[fig.getColor()] & (1ULL << bit), "invalid pawn mask" );
    }
  }

  void move(Figure & fig, int to)
  {
    int from = fig.where();

    const uint64 & uc0 = code(fig, from);
    const uint64 & uc1 = code(fig, to);
    hashCode_ ^= uc0;
    hashCode_ ^= uc1;
    fcounter_[fig.getColor()].move(fig, to);

    mask_[fig.getColor()] ^= 1ULL << from;
    mask_[fig.getColor()] |= 1ULL << to;

    THROW_IF(mask_[fig.getColor()] & (1ULL << from), "invalid figures mask");

    if ( fig.getType() != Figure::TypePawn )
      return;

    int bit_from = s_transposeIndex_[from];
    int bit_to = s_transposeIndex_[to];

    pawn_mask_[fig.getColor()] ^= 1ULL << bit_from;
    THROW_IF( pawn_mask_[fig.getColor()] & (1ULL << bit_from), "invalid pawn mask" );
    pawn_mask_[fig.getColor()] |= 1ULL << bit_to;
  }

  void hashEnPassant(uint8 pos, uint8 color)
  {
    const uint64 & enpassantCode = s_zobristCodes_[ (pos<<4) | (color<<3)/* | fig.getType() - use 0 instead*/ ];
    hashCode_ ^= enpassantCode;
  }

  void hashCastling(uint8 color, uint8 index /* 0 - short, 1 - long */)
  {
    const uint64 & castleCode = s_zobristCastle_[color][index];
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
  ScoreType weight(Figure::Color color) const { return fcounter_[color].weight(); }
  ScoreType weight() const { return weight(Figure::ColorWhite) - weight(Figure::ColorBlack); }
  ScoreType eval(Figure::Color color, int stage) const { return fcounter_[color].eval(stage); }
  ScoreType eval(int stage) const { return fcounter_[Figure::ColorWhite].eval(stage) - fcounter_[Figure::ColorBlack].eval(stage); }
  const uint64 & hashCode() const { return hashCode_; }
  const uint64 & pawn_mask(Figure::Color color) const { return pawn_mask_[color]; }
  const uint64 & mask(Figure::Color color) const { return mask_[color]; }

private:

  const uint64 & code(const Figure & fig, int i) const
  {
    return s_zobristCodes_[ (i<<4) | (fig.getColor()<<3) | fig.getType() ];
  }

  /// all figures mask
  uint64 mask_[2];

  /// only pawns. transposed for optimization
  uint64 pawn_mask_[2];

  FiguresCounter fcounter_[2];
  uint64 hashCode_;
};


#pragma pack (pop)
