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
  static const ScoreType positionGain_ = 60;

  enum Weights { WeightDraw = 0, WeightMat = 32000 };
  enum Type  { TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing };
  enum Color { ColorBlack, ColorWhite };

  static inline ScoreType positionEvaluation(uint8 stage, uint8 color, uint8 type, int8 pos)
  {
    THROW_IF( stage > 1 || color > 1 || type > 7 || pos < 0 || pos > 63, "invalid figure params" );

    uint8 cmask = ((int8)(color << 7)) >> 7;
    uint8 icmask = ~cmask;
    uint8 i = (mirrorIndex_[pos] & cmask) | (pos & icmask);

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

Figure::Type toFtype(char c);
char fromFtype(Figure::Type t);


__declspec (align(1)) class FiguresCounter
{
  static  int8 s_whiteColors_[64];

public:

  static uint8 s_transposeIndex_[64];

  FiguresCounter()
  {
    clear();
  }

  void clear()
  {
    count_ = 0;
    weight_ = 0;
    eval_[0] = 0;
    eval_[1] = 0;
    for (int i = 0; i < 8; ++i)
    {
      tcount_[i][0] = tcount_[i][1] = 0;
      tmask_[i] = 0ULL;
    }
  }

  void incr(const Figure & fig)
  {
    static int8 fcmask[8] = { 0/*None*/, 0/*Pawn*/, 0/*Knight*/, (int8)255/*bishop*/, 0/*rook*/, 0/*queen*/, 0/*king*/ };
    static int8 mincr[8]  = { 0/*None*/, 1/*Pawn*/, 1/*Knight*/, 1/*bishop*/, 1/*rook*/, 1/*queen*/, 0/*king*/ };
    static ScoreType mscore[8] = { 0/*None*/, (ScoreType)65535/*Pawn*/, (ScoreType)65535/*Knight*/, (ScoreType)65535/*bishop*/, (ScoreType)65535/*rook*/, (ScoreType)65535/*queen*/, 0/*king*/ };
    
    int8 fc = s_whiteColors_[fig.where()] & fcmask[fig.getType()];
    
    tcount_[fig.getType()][fc]++;
    weight_ += Figure::figureWeight_[fig.getType()] & mscore[fig.getType()];
    count_  += mincr[fig.getType()];

    // 0xFF - pawn, 0 - other figures
    int8 ptmask = ((int8)fig.getType() - 2) >> 8;
    int8 bit = (s_transposeIndex_[fig.where()] & ptmask) | (fig.where() & ~ptmask);
    tmask_[fig.getType()] |= 1ULL << bit;

    THROW_IF( (fig.getType() == Figure::TypePawn && bit != s_transposeIndex_[fig.where()]) || (fig.getType() != Figure::TypePawn && bit != fig.where()), "invalid bit shift for mask" );

    eval_[0] += Figure::positionEvaluation(0, fig.getColor(), fig.getType(), fig.where());
    eval_[1] += Figure::positionEvaluation(1, fig.getColor(), fig.getType(), fig.where());

    THROW_IF(weight_ != pawns()*Figure::figureWeight_[Figure::TypePawn] + bishops()*Figure::figureWeight_[Figure::TypeBishop] + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook] + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
    THROW_IF(count_ != pawns() + bishops() + knights() + rooks() + queens(), "invalid number of figures encountered");
  }

  void decr(const Figure & fig)
  {
    static int8 fcmask[8] = { 0/*None*/, 0/*Pawn*/, 0/*Knight*/, (int8)255/*bishop*/, 0/*rook*/, 0/*queen*/, 0/*king*/ };
    static int8 mdecr[8]  = { 0/*None*/, 1/*Pawn*/, 1/*Knight*/, 1/*bishop*/, 1/*rook*/, 1/*queen*/, 0/*king*/ };
    static ScoreType mscore[8] = { 0/*None*/, (ScoreType)65535/*Pawn*/, (ScoreType)65535/*Knight*/, (ScoreType)65535/*bishop*/, (ScoreType)65535/*rook*/, (ScoreType)65535/*queen*/, 0/*king*/ };

    int8 fc = s_whiteColors_[fig.where()] & fcmask[fig.getType()];

    tcount_[fig.getType()][fc]--;
    weight_ -= Figure::figureWeight_[fig.getType()] & mscore[fig.getType()];
    count_  -= mdecr[fig.getType()];

    // 0xFF - pawn, 0 - other figures
    int8 ptmask = ((int8)fig.getType() - 2) >> 8;
    int8 bit = (s_transposeIndex_[fig.where()] & ptmask) | (fig.where() & ~ptmask);
    tmask_[fig.getType()] ^= 1ULL << bit;

    THROW_IF( (fig.getType() == Figure::TypePawn && bit != s_transposeIndex_[fig.where()]) || (fig.getType() != Figure::TypePawn && bit != fig.where()), "invalid bit shift for mask" );
    THROW_IF( tmask_[fig.getType()] & (1ULL << bit), "invalid queen mask" );

    eval_[0] -= Figure::positionEvaluation(0, fig.getColor(), fig.getType(), fig.where());
    eval_[1] -= Figure::positionEvaluation(1, fig.getColor(), fig.getType(), fig.where());;

    THROW_IF(weight_ != pawns()*Figure::figureWeight_[Figure::TypePawn] + bishops()*Figure::figureWeight_[Figure::TypeBishop] + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook] + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
    THROW_IF(count_ != pawns() + bishops() + knights() + rooks() + queens(), "invalid number of figures encountered");
  }

  // fig in old position, to - it's new position
  void move(Figure & fig, int to)
  {
    eval_[0] -= Figure::positionEvaluation(0, fig.getColor(), fig.getType(), fig.where());
    eval_[0] += Figure::positionEvaluation(0, fig.getColor(), fig.getType(), to);

    eval_[1] -= Figure::positionEvaluation(1, fig.getColor(), fig.getType(), fig.where());
    eval_[1] += Figure::positionEvaluation(1, fig.getColor(), fig.getType(), to);

    // 0xFF - pawn, 0 - other figures
    int8 ptmask = ((int8)fig.getType() - 2) >> 8;
    int bit_from = (s_transposeIndex_[fig.where()] & ptmask) | (fig.where() & ~ptmask);;
    int bit_to   = (s_transposeIndex_[to] & ptmask) | (to & ~ptmask);
    tmask_[fig.getType()] ^= 1ULL << bit_from;
    THROW_IF( tmask_[fig.getType()] & (1ULL << bit_from), "invalid figure mask" );
    tmask_[fig.getType()] |= 1ULL << bit_to;

    fig.go(to);
  }

  int count() const { return count_; }
  int pawns() const { return tcount_[Figure::TypePawn][0]; }
  int knights() const { return tcount_[Figure::TypeKnight][0]; }
  int bishops_w() const { return tcount_[Figure::TypeBishop][1]; }
  int bishops_b() const { return tcount_[Figure::TypeBishop][0]; }
  int bishops() const { return bishops_w() + bishops_b(); }
  int rooks() const { return tcount_[Figure::TypeRook][0]; }
  int queens() const { return tcount_[Figure::TypeQueen][0]; }
  ScoreType weight() const { return weight_; }
  ScoreType eval(int i) const { return eval_[i]; } // position
  const uint64 & pawn_mask() const { return tmask_[Figure::TypePawn]; }
  const uint64 & knight_mask() const { return tmask_[Figure::TypeKnight]; }
  const uint64 & bishop_mask() const { return tmask_[Figure::TypeBishop]; }
  const uint64 & rook_mask() const { return tmask_[Figure::TypeRook]; }
  const uint64 & queen_mask() const { return tmask_[Figure::TypeQueen]; }
  const uint64 & king_mask() const { return tmask_[Figure::TypeKing]; }

private:

  // 8 - types, 2 - field color (for bishops only!!!)
  uint8 tcount_[8][2];
  uint8 count_;
  uint64 tmask_[8];
  ScoreType weight_, eval_[2];
};

class FiguresManager
{
  static uint64 s_zobristCodes_[64*2*8];
  static uint64 s_zobristColor_;
  static uint64 s_zobristCastle_[2][2];

public:

  FiguresManager() : hashCode_(0ULL)
  {
    mask_[0] = mask_[1] = 0ULL;
  }

  void clear()
  {
    hashCode_ = 0ULL;
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
  }

  void decr(const Figure & fig)
  {
    fcounter_[fig.getColor()].decr(fig);
    const uint64 & uc = code(fig, fig.where());
    hashCode_ ^= uc;
    mask_[fig.getColor()] ^= 1ULL << fig.where();
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
  const uint64 & pawn_mask(Figure::Color color) const { return fcounter_[color].pawn_mask(); }
  const uint64 & knight_mask(Figure::Color color) const { return fcounter_[color].knight_mask(); }
  const uint64 & bishop_mask(Figure::Color color) const { return fcounter_[color].bishop_mask(); }
  const uint64 & rook_mask(Figure::Color color) const { return fcounter_[color].rook_mask(); }
  const uint64 & queen_mask(Figure::Color color) const { return fcounter_[color].queen_mask(); }
  const uint64 & king_mask(Figure::Color color) const { return fcounter_[color].king_mask(); }
  const uint64 & mask(Figure::Color color) const { return mask_[color]; }

private:

  const uint64 & code(const Figure & fig, int i) const
  {
    return s_zobristCodes_[ (i<<4) | (fig.getColor()<<3) | fig.getType() ];
  }

  /// all figures mask
  uint64 mask_[2];

  FiguresCounter fcounter_[2];
  uint64 hashCode_;
};


#pragma pack (pop)
