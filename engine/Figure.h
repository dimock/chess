#pragma once

/*************************************************************
  Figure.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "BasicTypes.h"
#include "Helpers.h"

#pragma pack (push, 1)

namespace Figure
{
  enum Weights { WeightDraw = 0, WeightMat = 32000 };
  enum Type  { TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing };
  enum Color { ColorBlack, ColorWhite };
  
  extern const ScoreType figureWeight_[7], figureWeightSEE_[7];
  extern const ScoreType positionGain_;
  extern const BitMask pawnCutoffMasks_[2];

  // position evaluation. 0 - opening, 1 - endgame; color,type,pos
  extern const ScoreType positionEvaluations_[2][8][64];
  extern const ScoreType bishopKnightMat_[64];
  extern const ScoreType figureWeight_[7], figureWeightSEE_[7]; // TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing
  extern const ScoreType pawnDoubled_, pawnIsolated_, pawnBackward_, openRook_, semiopenRook_, winloseBonus_;
  extern const ScoreType queenToMyKingDistPenalty_[8];
  extern const ScoreType fianchettoBonus_;
  extern const ScoreType fakecastlePenalty_, castleImpossiblePenalty_;
  extern const ScoreType kingbishopPressure_;
  extern const ScoreType pawnPassed_[2][8], pawnGuarded_[2][8];
  extern const BitMask pawnCutoffMasks_[2];
  extern const uint8 mirrorIndex_[64];

  inline Figure::Color otherColor(Figure::Color color)
  {
    return (Figure::Color)((color + 1) & 1);
  }

  const char * name(Type type);

  inline ScoreType positionEvaluation(int stage, Figure::Color color, Figure::Type type, int pos)
  {
    THROW_IF( stage > 1 || color > 1 || type > 7 || pos < 0 || pos > 63, "invalid figure params" );

    uint8 cmask = ((int8)(color << 7)) >> 7;
    uint8 icmask = ~cmask;
    uint8 i = (mirrorIndex_[pos] & cmask) | (pos & icmask);

    ScoreType e = positionEvaluations_[stage][type][i];
    return e;
  }
}

Figure::Type toFtype(char c);
char fromFtype(Figure::Type t);


inline bool typeLEQ(Figure::Type type1, Figure::Type type2)
{
  if ( type1 == type2 ||
       type1 == Figure::TypeKnight && type2 == Figure::TypeBishop ||
       type1 == Figure::TypeBishop && type2 == Figure::TypeKnight )
  {
    return true;
  }

  if ( type1 < type2 )
    return true;

  return false;
}


__declspec (align(1)) class FiguresCounter
{
public:

  static  int8 s_whiteColors_[64];
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
    pmask_t_ = 0;
    for (int i = 0; i < 8; ++i)
    {
      tcount_[i][0] = tcount_[i][1] = 0;
      tmask_[i] = 0ULL;
    }
  }

  inline void incr(const Figure::Color c, const Figure::Type t, int p)
  {
    static int8 fcmask[8] = { 0/*None*/, 0/*Pawn*/, 0/*Knight*/, (int8)255/*bishop*/, 0/*rook*/, 0/*queen*/, 0/*king*/ };
    static int8 xincr[8]  = { 0/*None*/, 1/*Pawn*/, 1/*Knight*/, 1/*bishop*/, 1/*rook*/, 1/*queen*/, 0/*king*/ };
    static ScoreType xscore[8] = { 0/*None*/, (ScoreType)65535/*Pawn*/, (ScoreType)65535/*Knight*/, (ScoreType)65535/*bishop*/, (ScoreType)65535/*rook*/, (ScoreType)65535/*queen*/, 0/*king*/ };
    
    int8 field_color = s_whiteColors_[p] & fcmask[t];
    
    tcount_[t][field_color]++;
    weight_ += Figure::figureWeight_[t] & xscore[t];
    count_  += xincr[t];

    tmask_[t] |= set_mask_bit(p);

    if ( t == Figure::TypePawn )
      pmask_t_ |= set_mask_bit(s_transposeIndex_[p]);

    eval_[0] += Figure::positionEvaluation(0, c, t, p);
    eval_[1] += Figure::positionEvaluation(1, c, t, p);

    THROW_IF(weight_ != pawns()*Figure::figureWeight_[Figure::TypePawn] + bishops()*Figure::figureWeight_[Figure::TypeBishop] + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook] + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
    THROW_IF(count_ != pawns() + bishops() + knights() + rooks() + queens(), "invalid number of figures encountered");
  }

  inline void decr(const Figure::Color c, const Figure::Type t, int p)
  {
    static int8 fcmask[8] = { 0/*None*/, 0/*Pawn*/, 0/*Knight*/, (int8)255/*bishop*/, 0/*rook*/, 0/*queen*/, 0/*king*/ };
    static int8 xdecr[8]  = { 0/*None*/, 1/*Pawn*/, 1/*Knight*/, 1/*bishop*/, 1/*rook*/, 1/*queen*/, 0/*king*/ };
    static ScoreType xscore[8] = { 0/*None*/, (ScoreType)65535/*Pawn*/, (ScoreType)65535/*Knight*/, (ScoreType)65535/*bishop*/, (ScoreType)65535/*rook*/, (ScoreType)65535/*queen*/, 0/*king*/ };

    int8 fc = s_whiteColors_[p] & fcmask[t];

    tcount_[t][fc]--;
    weight_ -= Figure::figureWeight_[t] & xscore[t];
    count_  -= xdecr[t];

    tmask_[t] ^= set_mask_bit(p);

    if ( t == Figure::TypePawn )
      pmask_t_ ^= set_mask_bit(s_transposeIndex_[p]);

    THROW_IF( tmask_[t] & set_mask_bit(p), "invalid mask" );

    eval_[0] -= Figure::positionEvaluation(0, c, t, p);
    eval_[1] -= Figure::positionEvaluation(1, c, t, p);

    THROW_IF(weight_ != pawns()*Figure::figureWeight_[Figure::TypePawn] + bishops()*Figure::figureWeight_[Figure::TypeBishop] + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook] + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
    THROW_IF(count_ != pawns() + bishops() + knights() + rooks() + queens(), "invalid number of figures encountered");
  }

  inline void move(const Figure::Color c, const Figure::Type t, int from, int to)
  {
    eval_[0] -= Figure::positionEvaluation(0, c, t, from);
    eval_[0] += Figure::positionEvaluation(0, c, t, to);

    eval_[1] -= Figure::positionEvaluation(1, c, t, from);
    eval_[1] += Figure::positionEvaluation(1, c, t, to);

    tmask_[t] ^= set_mask_bit(from);
    tmask_[t] |= set_mask_bit(to);

    if ( t == Figure::TypePawn )
    {
      pmask_t_ ^= set_mask_bit(s_transposeIndex_[from]);
      pmask_t_ |= set_mask_bit(s_transposeIndex_[to]);
    }
  }

  inline int count() const { return count_; }
  inline int pawns() const { return tcount_[Figure::TypePawn][0]; }
  inline int knights() const { return tcount_[Figure::TypeKnight][0]; }
  inline int bishops_c(Figure::Color field_c) const { return tcount_[Figure::TypeBishop][field_c]; }
  inline int bishops_w() const { return tcount_[Figure::TypeBishop][1]; }
  inline int bishops_b() const { return tcount_[Figure::TypeBishop][0]; }
  inline int bishops() const { return bishops_w() + bishops_b(); }
  inline int rooks() const { return tcount_[Figure::TypeRook][0]; }
  inline int queens() const { return tcount_[Figure::TypeQueen][0]; }
  inline ScoreType weight() const { return weight_; }
  inline ScoreType eval(int stage) const { return eval_[stage]; }
  inline const BitMask & pawn_mask_t() const { return pmask_t_; }
  inline const BitMask & pawn_mask_o() const { return tmask_[Figure::TypePawn]; }
  inline const BitMask & knight_mask() const { return tmask_[Figure::TypeKnight]; }
  inline const BitMask & bishop_mask() const { return tmask_[Figure::TypeBishop]; }
  inline const BitMask & rook_mask() const { return tmask_[Figure::TypeRook]; }
  inline const BitMask & queen_mask() const { return tmask_[Figure::TypeQueen]; }
  inline const BitMask & king_mask() const { return tmask_[Figure::TypeKing]; }
  inline const BitMask & type_mask(const Figure::Type type) const { return tmask_[type]; }

private:

  // 8 - types, 2 - field color (for bishops only!!!)
  uint8 tcount_[8][2];
  uint8 count_;
  BitMask tmask_[7];
  BitMask pmask_t_; // transposed pawn's mask
  ScoreType weight_, eval_[2];
};

class FiguresManager
{
  static BitMask s_zobristCodes_[64*2*8];
  static BitMask s_zobristColor_;
  static BitMask s_zobristCastle_[2][2];

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

  inline void incr(const Figure::Color c, const Figure::Type t, int p)
  {
    fcounter_[c].incr(c, t, p);
    const BitMask & uc = code(c, t, p);
    hashCode_ ^= uc;
    mask_[c] |= set_mask_bit(p);
  }

  inline void decr(const Figure::Color c, const Figure::Type t, int p)
  {
    fcounter_[c].decr(c, t, p);
    const BitMask & uc = code(c, t, p);
    hashCode_ ^= uc;
    mask_[c] ^= set_mask_bit(p);
  }

  inline void move(const Figure::Color c, const Figure::Type t, int from, int to)
  {
    const BitMask & uc0 = code(c, t, from);
    const BitMask & uc1 = code(c, t, to);
    hashCode_ ^= uc0;
    hashCode_ ^= uc1;

    fcounter_[c].move(c, t, from, to);

    mask_[c] ^= set_mask_bit(from);
    mask_[c] |= set_mask_bit(to);

    THROW_IF(mask_[c] & set_mask_bit(from), "invalid figures mask");
  }

  // the same as original versions, except of skipped mask & zcode
  inline void u_incr(const Figure::Color c, const Figure::Type t, int p)
  {
	  fcounter_[c].incr(c, t, p);
  }

  inline void u_decr(const Figure::Color c, const Figure::Type t, int p)
  {
	  fcounter_[c].decr(c, t, p);
  }

  inline void u_move(const Figure::Color c, const Figure::Type t, int from, int to)
  {
	  fcounter_[c].move(c, t, from, to);
  }

  inline void hashEnPassant(uint8 pos, uint8 color)
  {
    const BitMask & enpassantCode = s_zobristCodes_[ (pos<<4) | (color<<3) ];
    hashCode_ ^= enpassantCode;
  }

  inline void hashCastling(uint8 color, uint8 index /* 0 - short, 1 - long */)
  {
    const BitMask & castleCode = s_zobristCastle_[color][index];
    hashCode_ ^= castleCode;
  }

  inline void hashColor()
  {
    hashCode_ ^= s_zobristColor_;
  }

  inline void restoreMasks(const BitMask (& mask)[2])
  {
	  mask_[0] = mask[0];
	  mask_[1] = mask[1];
  }

  void restoreHash(const BitMask & hcode) { hashCode_ = hcode; }

  inline int count(Figure::Color color) const { return fcounter_[color].count(); }
  inline int count() const { return fcounter_[0].count() + fcounter_[1].count(); }
  inline int pawns(Figure::Color color) const { return fcounter_[color].pawns(); }
  inline int pawns() const { return pawns(Figure::ColorWhite) + pawns(Figure::ColorBlack); }
  inline int bishops_w(Figure::Color color) const { return fcounter_[color].bishops_w(); }
  inline int bishops_b(Figure::Color color) const { return fcounter_[color].bishops_b(); }
  inline int bishops(Figure::Color color) const { return fcounter_[color].bishops(); }
  inline int bishops_c(Figure::Color color, Figure::Color field_c) const { return fcounter_[color].bishops_c(field_c); }
  inline int knights(Figure::Color color) const { return fcounter_[color].knights(); }
  inline int rooks(Figure::Color color) const { return fcounter_[color].rooks(); }
  inline int queens(Figure::Color color) const { return fcounter_[color].queens(); }
  inline ScoreType weight(Figure::Color color) const { return fcounter_[color].weight(); }
  inline ScoreType weight() const { return weight(Figure::ColorWhite) - weight(Figure::ColorBlack); }
  inline ScoreType eval(Figure::Color color, int stage) const { return fcounter_[color].eval(stage); }
  inline ScoreType eval(int stage) const { return fcounter_[Figure::ColorWhite].eval(stage) - fcounter_[Figure::ColorBlack].eval(stage); }
  inline const BitMask & hashCode() const { return hashCode_; }
  inline const BitMask & pawn_mask_o(Figure::Color color) const { return fcounter_[color].pawn_mask_o(); }
  inline const BitMask & pawn_mask_t(Figure::Color color) const { return fcounter_[color].pawn_mask_t(); }
  inline const BitMask & knight_mask(Figure::Color color) const { return fcounter_[color].knight_mask(); }
  inline const BitMask & bishop_mask(Figure::Color color) const { return fcounter_[color].bishop_mask(); }
  inline const BitMask & rook_mask(Figure::Color color) const { return fcounter_[color].rook_mask(); }
  inline const BitMask & queen_mask(Figure::Color color) const { return fcounter_[color].queen_mask(); }
  inline const BitMask & king_mask(Figure::Color color) const { return fcounter_[color].king_mask(); }
  inline const BitMask & mask(Figure::Color color) const { return mask_[color]; }
  inline const BitMask & type_mask(const Figure::Type type, const Figure::Color color) const { return fcounter_[color].type_mask(type); }

private:

  inline const BitMask & code(const Figure::Color c, const Figure::Type t, int p) const
  {
    return s_zobristCodes_[(p<<4) | (c<<3) | t];
  }

  /// all figures mask
  BitMask mask_[2];

  FiguresCounter fcounter_[2];
  BitMask hashCode_;
};


#pragma pack (pop)
