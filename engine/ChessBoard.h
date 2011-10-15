#pragma once

#include "Field.h"
#include "Helpers.h"
#include "MovesPrecounter.h"
#include "FigureDirs.h"

class Board
{

  WeightType calculateDeltaWeight(Step & step, const Step & prevStep) const
  {
    WeightType w = 0;
    const Figure & fig = getFigure(color_, step.index_);
    const Figure & rfig = getFigure(Figure::otherColor(color_), step.rindex_);
    w = Figure::figureWeight_[rfig.getType()] - Figure::figureWeight_[fig.getType()];
    if ( step.rindex_ == prevStep.index_ )
      w += Figure::figureWeight_[Figure::TypeQueen];
    return w;
  }

  void calculateStepWeight(Step & step, const Step & prevStep, int (&history)[64][64]) const
  {
    THROW_IF( !step, "calculating weight for invalid step" );

    step.weight_ = -std::numeric_limits<int>::max();
    const Figure & fig = getFigure(color_, step.index_);
    if ( UnderCheck == state_ )
    {
      if ( 2 == chkNum_ && Figure::TypeKing == fig.getType() )
        step.weight_ = Figure::figureWeight_[Figure::TypeQueen];
      else if ( step.rindex_ == checking_[0] )
        step.weight_ = calculateDeltaWeight(step, prevStep);
      else if ( Figure::TypeKing == fig.getType() )
        step.weight_ = Figure::figureWeight_[Figure::TypePawn];
    }
    else
    {
      if ( step.rindex_ >= 0 )
        step.weight_ = calculateDeltaWeight(step, prevStep);
      else
        step.weight_ = history[step.from_][step.to_];
    }
  }


public:


  enum State { Invalid, Ok, Castle, UnderCheck, Stalemat, DrawReps, DrawInsuf, Draw50Moves, ChessMat };
  enum AttackType { NoAttack, Attack, KingAttack, MaybeAttack };
  enum { PawnIndex, BishopIndex = 8, KnightIndex = 10, RookIndex = 12, QueenIndex = 14, KingIndex = 15, NumOfFigures = 16, NumOfFields = 64, NumOfSteps = 256 };

  static bool isDraw(State state)
  {
    return Stalemat == state || DrawReps == state || DrawInsuf == state || Draw50Moves == state;
  }

  class StepSorter
  {
    enum { NumGroups = 6 };
  public:

    StepSorter(Step (&steps)[Board::NumOfSteps], const Step & prev, const StepId & killerId) :
        steps_(steps), num_(0), prev_(prev), killerId_(killerId)
    {
      for (int i = 0; i < NumGroups; ++i)
        firsts_[i] = lasts_[i] = -1;
    }

    inline Step & back()
    {
      return steps_[num_];
    }

    inline void push(Step & step, Board & board)
    {
      THROW_IF(step.invalid(), "invalid step");

      board.calculateStepWeight(step, prev_, history_);

      int j = -1;
      if ( step.rindex_ >= 0 ) // eat
        j = 0;
      else if ( step == killerId_ )
        j = 1;
      else if ( step.maybe_check_ )
        j = 2;
      else if ( step.weight_ > 0 )
        j = 3;
      else if ( 0 == step.weight_ )
        j = 4;
      else
        j = 5;

      THROW_IF( j < 0, "invalid step" );

      int iprev = -1, icurr = firsts_[j];
      for (; icurr >= 0; iprev = icurr, icurr = steps_[icurr].next_)
      {
        if ( step.weight_ >= steps_[icurr].weight_ )
          break;
      }

      if ( iprev >= 0 )
        steps_[iprev].next_ = num_;
      else
        firsts_[j] = num_;

      step.next_ = icurr;

      if ( icurr < 0 )
        lasts_[j] = num_;

      num_++;
    }

    inline int first()
    {
      int ifirst = firsts_[NumGroups-1];
      for (int i = NumGroups-2; i >= 0; --i)
      {
        if ( firsts_[i] < 0 )
          continue;

        THROW_IF( lasts_[i] < 0, "invalid last step in chain" );

        Step & last = steps_[lasts_[i]];

        THROW_IF( last.next_ >= 0, "last.next should be -1" );

        last.next_ = ifirst;
        ifirst = firsts_[i];
      }

      return ifirst;
    }

    static void clearHistory();
    static int (&history())[64][64] { return history_; }

  private:

    Step (&steps_)[Board::NumOfSteps];
    int  firsts_[NumGroups], lasts_[NumGroups];
    int  num_;
    const Step & prev_;
    const StepId & killerId_;

    // history sort
    static int history_[64][64];
  };

  bool stepById(const StepId & stepId, Step & step)
  {
    if ( stepId.index_ < 0 )
      return false;

    const Figure & fig = getFigure(color_, stepId.index_);
    if ( !fig || fig.where() != stepId.from_ || stepId.to_ < 0 )
      return false;

    if ( stepId.castle_ && Figure::TypeKing == fig.getType() && fig.isFirstStep() )
    {
      for (int i = 0; i < 2; ++i)
      {
        step.clear();
        doCastling(fig, 8+i, step);
        if ( step && step == stepId )
          return true;
      }

      step.clear();
      return false;
    }

    int * indices = MovesTable::inds(fig.where(), color_, fig.getType());
    int dir = FigureDir::dir(fig, stepId.to_);
    if ( dir < 0 )
      return false;

    if ( Figure::TypePawn == fig.getType() )
    {
      THROW_IF( dir > 3, "invalid move dir for pawn" );

      int to_idx = *(indices + (dir<<3));
      if ( to_idx < 0 )
        return false;

      const Field & f = getField(to_idx);

      int rindex = -1;
      bool can_go = false;

      if ( !f )
      {
        Figure fake;
        if ( getFake(fake) && fake.where() == to_idx && fake.getColor() != color_ )
        {
          THROW_IF( (dir != 0 && dir != 1) || fig.isFirstStep() || Figure::TypePawn != fake.getType(), "attempt to eat fake pawn by wrong way");

          rindex = fakeIndex_;
          can_go = true;
        }
        else
          can_go = (2 == dir|| 3 == dir);
      }
      else if ( (0 == dir || 1 == dir) && f.color() != color_ )
      {
        rindex = f.index();
        can_go = true;
      }

      if ( !can_go )
        return false;

      int newType = 0;
      int y = (to_idx>>3) & 7;
      if ( (Figure::ColorWhite == fig.getColor() && 7 == y) || (Figure::ColorBlack == fig.getColor() && 0 == y) ) // going to reach the last line
      {
        THROW_IF( 3 == dir, "pawn is going to reach the last line on the 1st step");
        if ( stepId.newType_ < Figure::TypeKnight || stepId.newType_ > Figure::TypeKing )
          return false;

        newType = stepId.newType_;
      }

      step.index_ = fig.getIndex();
      step.rindex_ = rindex;
      step.newType_ = newType;
      step.to_ = to_idx;
      step.from_ = fig.where();
      step.ftype_ = Figure::TypePawn;

      THROW_IF(step.invalid(), "invalid step");

      if ( step == stepId )
        return true;
    }
    else
    {
      for (int * inds = indices + (dir<<3); *inds >= 0; ++inds)
      {
        const Field & f = getField(*inds);

        if ( Figure::TypeKing == fig.getType() && Figure::TypeKing == f.type() )
          break;

        int rindex = f.index();
        if ( rindex >= 0 && f.color() == color_ )
          break;

        step.index_ = fig.getIndex();
        step.to_ = *inds;
        step.from_ = fig.where();
        step.rindex_ = rindex;
        step.ftype_ = fig.getType();

        THROW_IF(step.invalid(), "invalid step");

        if ( step == stepId )
          return true;

        if ( f )
          break;
      }
    }

    step.clear();
    return false;
  }


private:

  friend class Figure;

  static const int s_numDirs_[8];

public:

  Board(Figure::Color color) : fakeIndex_(-1), color_(color), state_(Ok), chkNum_(0)
  {
    castle_[0] = castle_[1] = 0;
    stages_[0] = stages_[1] = 0;
    can_win_[0] = can_win_[1] = true;
  }

  inline int8 getFiguresCount(Figure::Color color) const { return fmgr_.count(color); }
  inline int8 getFiguresCount() const { return fmgr_.count(); }

  inline const Field & getField(const FPos & p) const { return fields_[p.index()]; }
  inline Field & getField(const FPos & p) { return fields_[p.index()]; }

  inline const Field & getField(int index) const
  {
    THROW_IF((unsigned)index > 63, "field index is invalid" );
    return fields_[index];
  }

  inline Field & getField(int index)
  {
    THROW_IF((unsigned)index > 63, "field index is invalid" );
    return fields_[index];
  }

  inline int getFakeIdx() const { return fakeIndex_; }

  bool addFigure(const Figure & fig);

  inline bool getFigure(const FPos & p, Figure & fig) const
  {
    const Field & f = getField(p);
    if ( !f )
      return false;

    fig = figures_[(f.color()<<4) + f.index()];
    return true;
  }

  inline const Figure & getFigure(Figure::Color color, int index) const
  {
    THROW_IF( (size_t)index >= NumOfFigures, "try to get invalid figure");
    return figures_[(color<<4) + index];
  }

  inline Figure & getFigure(Figure::Color color, int index)
  {
    THROW_IF( (size_t)index >= NumOfFigures, "try to get invalid figure");
    return figures_[(color<<4) + index];
  }

  inline bool getFake(Figure & fake) const
  {
    if ( (size_t)fakeIndex_ >= NumOfFigures )
      return false;

    Figure::Color ocolor = Figure::otherColor(color_);
    fake = getFigure(ocolor, fakeIndex_);
    int idx = fake.where();
    if ( Figure::ColorWhite == fake.getColor() )
      idx -= 8;
    else
      idx += 8;
    fake.go(idx);
    return true;
  }

  inline Figure::Color getColor() const { return color_; }

  // movements
  int   getSteps(Step (&steps)[NumOfSteps], bool capturesOnly, const Step & prev, const StepId & killerId);

  inline void makeNullMove(const Step & prev)
  {
    color_ = Figure::otherColor(color_);
    fmgr_.hashColor();

    if ( fakeIndex_ >= 0 )
    {
      Figure fake;
      if ( getFake(fake) )
      {
        fmgr_.hashFake(prev.from_, color_);
      }
      fakeIndex_ = -1;
    }
  }

  inline bool allowNullMove() const
  {
    Figure::Color ocolor = Figure::otherColor(color_);

	  bool ok = fmgr_.weight(color_) > Figure::figureWeight_[Figure::TypeQueen] ||
			        fmgr_.weight(ocolor) > Figure::figureWeight_[Figure::TypeQueen];

    return ok;
  }

  inline Board::State makeStep(Step & step)
  {
    step.need_unmake_ = false;

    if ( !doStep(step) )
    {
      state_ = Invalid;
      return state_;
    }

    if ( !wasStepValid(step) )
    {
      state_ = Invalid;
      return state_;
    }

    isChecking(step);

    verifyChessDraw();

    if ( drawState() )
      return state_;

    if ( step.index_ >= 0 ) // go to endgame if material is lost
    {
      Figure::Color ocolor = Figure::otherColor((Figure::Color)color_);
      if ( !( (fmgr_.queens(ocolor) > 0 && fmgr_.rooks(ocolor)+fmgr_.knights(ocolor)+fmgr_.bishops(ocolor) > 0) ||
              (fmgr_.rooks(ocolor) > 1 && fmgr_.bishops(ocolor) + fmgr_.knights(ocolor) > 1) ||
              (fmgr_.rooks(ocolor) > 0 && ((fmgr_.bishops_w(ocolor) > 0 && fmgr_.bishops_b(ocolor) > 0) || (fmgr_.bishops(ocolor) + fmgr_.knights(ocolor) > 2))) ) )
      {
        stages_[color_] = 1;
      }
    }

    step.need_unmake_ = true;

    step.old_checkingNum_ = chkNum_;
    if ( chkNum_ > 0 )
    {
      step.old_checking_[0] = checking_[0];
      step.old_checking_[1] = checking_[1];
    }

    chkNum_ = step.checkingNum_;
    if ( chkNum_ > 0 )
    {
      checking_[0] = step.checking_[0];
      checking_[1] = step.checking_[1];
      state_ = UnderCheck;
    }

    THROW_IF( isAttacked(color_, getFigure(Figure::otherColor(color_), KingIndex).where()) && UnderCheck != state_, "check isn't detected" );

    // now change color
    color_ = Figure::otherColor(color_);
    fmgr_.hashColor();

    if ( !hasSteps() )
    {
      if ( UnderCheck == state_ )
        state_ = ChessMat;
      else
        state_ = Stalemat;
    }

    return state_;
  }

  //void unmakeStep(Step & step)
  //{
  //  if ( step.need_unmake_ )
  //  {
  //    fmgr_.hashColor();
  //    color_ = Figure::otherColor(color_);
  //    chkNum_ = step.old_checkingNum_;

  //    if ( chkNum_ > 0 )
  //    {
  //      checking_[0] = step.old_checking_[0];
  //      checking_[1] = step.old_checking_[1];
  //    }
  //  }

  //  undoStep(step);
  //}


  inline State getState() const { return state_; }
  inline bool  drawState() const { return isDraw(state_); }

  // set from outside if there are 3 or more repetitions or 50 moves rule
  inline void setChessDraw(State state)
  {
    if ( !drawState() && (DrawReps == state || Draw50Moves == state) )
      state_ = state;
  }

  void writeTo(std::ostream & out) const;
  void readFrom(std::istream & in);

  inline bool doStep(Step & step)
  {
    THROW_IF( state_ == Invalid, "try to move from invalid state" );

    step.old_state_ = state_;
    step.need_undo_ = false;
    step.fake_index_ = fakeIndex_;

    fakeIndex_ = -1;

    Figure::Color ocolor = Figure::otherColor(color_);

    THROW_IF( step.index_ < 0, "try to move invalid figure" );

    Figure & fig = getFigure(color_, step.index_);
    if ( Figure::TypeKing == fig.getType() && step.castle_ )
    {
      if ( UnderCheck == state_ )
        return false;

      THROW_IF( isAttacked(ocolor, fig.where()), "can't do castling under check" );
      THROW_IF( step.rindex_ >= 0, "can't eat while castling" );
      THROW_IF( step.rookidx_ < 0 || step.rook_to_ < 0, "rook index is undefined or it's new position is wrong" );

      Figure & rook = getFigure( color_, step.rookidx_ );
      THROW_IF( !rook.isFirstStep(), "rook was already moved" );

      Field & rfield = getField(rook.where());
      THROW_IF( !rfield, "there is no rook for castling" );
      if ( isAttacked(ocolor, step.rook_to_) )
        return false;

      rfield.clear();
      fmgr_.move(rook, step.rook_to_);
      rook.setMoved();
      getField(step.rook_to_).set(rook);

      fmgr_.hashCastling(color_);

      castle_[color_] = step.to_ > step.from_ ? 1 : 2;

      state_ = Castle;
    }
    else
      state_ = Ok;

    // eat
    if ( step.rindex_ >= 0 )
    {
      Figure & rfig = getFigure(ocolor, step.rindex_);
      step.eaten_type_ = rfig.getType();

      getField(rfig.where()).clear();
      fmgr_.decr(rfig);
      rfig.setType(Figure::TypeNone);
    }

    if ( Figure::TypePawn == fig.getType() && fig.isFirstStep() )
    {
      int dy = step.to_ - fig.where();
      if ( 16 == dy || -16 == dy )
      {
        fakeIndex_ = fig.getIndex();
        fmgr_.hashFake(fig.where(), color_);
      }
    }

    step.first_step_ = fig.isFirstStep();

    getField(fig.where()).clear();
    fig.setMoved();

    if ( step.newType_ > 0 )
    {
      step.oldType_ = fig.getType();
      fmgr_.decr(fig);

      fig.go(step.to_);
      fig.setType((Figure::Type)step.newType_);
      fmgr_.incr(fig);
    }
    else
    {
      fmgr_.move(fig, step.to_);
    }

    getField(fig.where()).set(fig);
    step.need_undo_ = true;

    THROW_IF( calculatePositionEval(0) != fmgr_.eval(0) || calculatePositionEval(1) != fmgr_.eval(1), "position evaluated incorrectly" );

    return true;
  }

  inline void undoStep(Step & step)
  {
    // restore state, because we have changed it
    state_ = (State)step.old_state_;

    if ( !step.need_undo_ )
      return;

    // restore figure
    Figure & fig = getFigure(color_, step.index_);

    // restore old type
    if ( step.newType_ > 0 )
    {
      fmgr_.decr(fig);

      fig.go(step.from_);
      fig.setType((Figure::Type)step.oldType_);
      fmgr_.incr(fig);
    }
    else
    {
      fmgr_.move(fig, step.from_);
    }

    // restore position and field
    getField(step.to_).clear();
    if ( step.first_step_ )
      fig.setUnmoved();
    getField(fig.where()).set(fig);

    // restore fake hash code (en-passant)
    if ( fakeIndex_ >= 0 )
      fmgr_.hashFake(fig.where(), color_);

    // restore fake index
    fakeIndex_ = step.fake_index_;


    // restore eaten figure
    if ( step.rindex_ >= 0 )
    {
      Figure::Color ocolor = Figure::otherColor(color_);
      Figure & rfig = getFigure(ocolor, step.rindex_);

      THROW_IF( step.eaten_type_ <= 0, "type of eaten figure is invalid" );
      rfig.setType((Figure::Type)step.eaten_type_);
      getField(rfig.where()).set(rfig);
      fmgr_.incr(rfig);
    }

    // restore king and rook after castling
    if ( step.castle_ )
    {
      fmgr_.hashCastling(color_);

      getField(step.rook_to_).clear();

      Figure & rook = getFigure(color_, step.rookidx_);

      fmgr_.move(rook, step.rook_from_);

      rook.setUnmoved();
      Field & rfield = getField(rook.where());
      rfield.set(rook);

      castle_[color_] = 0;
    }
  }

  inline bool wasStepValid(const Step & step) const
  {
    if ( UnderCheck == step.old_state_ )
      return wasValidUnderCheck(step);
    else
      return wasValidWithoutCheck(step);
  }

  inline bool isChecking(Step & step) const
  {
    int idx0 = -1;
    step.checkingNum_ = 0;
    Figure::Color ocolor = Figure::otherColor(color_);
    const Figure & fig = getFigure(color_, step.index_);
    if ( step.castle_ )
      idx0 = getAttackedFrom(ocolor, step.rook_to_);
    else
    {
      if ( Figure::TypePawn == fig.getType() || Figure::TypeKnight == fig.getType() )
      {
        const Figure & king = getFigure(ocolor, KingIndex);
        int dir = FigureDir::dir(fig, king.where());
        if ( (Figure::TypePawn == fig.getType() && 0 == dir || 1 == dir) || (Figure::TypeKnight == fig.getType() && dir >= 0) )
          idx0 = step.index_;
      }
      else
        idx0 = getAttackedFrom(ocolor, step.to_);

      THROW_IF( idx0 >= 0 && idx0 != step.index_, "attacked by wrong figure" );
    }

    if ( idx0 >= 0 )
      step.checking_[step.checkingNum_++] = idx0;

    if ( !step.castle_ )
    {
      int idx1 = getAttackedFrom( ocolor, step.from_ );
      if ( idx1 >= 0 && idx1 != idx0 )
        step.checking_[step.checkingNum_++] = idx1;

      if ( step.rindex_ >= 0 && step.rindex_ == step.fake_index_ && Figure::TypePawn == fig.getType() )
      {
        const Figure & fake = getFigure(ocolor, step.rindex_);
        int fakePos = fake.where();
        static int fake_dp[2] = { 8, -8 };
        fakePos += fake_dp[ocolor];

        if ( fakePos == step.to_ )
        {
          int idx2 = getAttackedFrom(ocolor, fake.where());
          if ( idx2 >= 0 && idx2 != idx0 && idx2 != idx1 )
            step.checking_[step.checkingNum_++] = idx2;
        }
      }
    }

    THROW_IF( step.checkingNum_ > 2, "more than 2 figures attacking king" );
    THROW_IF( !step.checkingNum_ && wasStepValid(step) && isAttacked(color_, getFigure(ocolor, KingIndex).where()), "ckeck wasn't detected" );

    return step.checkingNum_ > 0;
  }

  inline int getStage(Figure::Color color) const
  {
    if ( can_win_[0] != can_win_[1] )
      return 2;

    return stages_[color];
  }

  inline bool pawnToLastLine(const Step & step) const
  {
    Figure::Color ocolor = Figure::otherColor(color_);
    const Figure & fig = getFigure(ocolor, step.index_);
    if ( Figure::TypePawn != fig.getType() )
      return false;
    int y = step.to_ >> 3;
    return (0 == y && Figure::ColorBlack == ocolor) || (7 == y && Figure::ColorWhite == ocolor);
  }

  inline bool pawnBeforeLastLine(const Step & step) const
  {
    Figure::Color ocolor = Figure::otherColor(color_);
    const Figure & fig = getFigure(ocolor, step.index_);
    if ( Figure::TypePawn != fig.getType() )
      return false;
    int y = step.to_ >> 3;
    return (1 == y && Figure::ColorBlack == ocolor) || (6 == y && Figure::ColorWhite == ocolor);
  }

  const uint64 & hashCode() const
  {
    return fmgr_.hashCode();
  }

  bool validatePosition();

  WeightType evaluate() const;

private:

  WeightType calculateEval() const;
  WeightType evaluatePawns(Figure::Color color, int stage) const;
  WeightType evaluateWinnerLoser() const;

  // for DEBUG only
#ifndef NDEBUG
  WeightType calculatePositionEval(int stage);
#endif

  int findCheckingFigures(Figure::Color color, int pos);

  inline int getAttackedFrom(Figure::Color color, int apt) const
  {
    const Figure & king = getFigure(color, KingIndex);
    FPos dp = getDeltaPos(apt, king.where());
    if ( FPos(0, 0) == dp )
      return -1;

    FPos p = FPosIndexer::get(king.where()) + dp;
    for ( ; p; p += dp)
    {
      const Field & field = getField(p);
      if ( !field )
        continue;

      if ( field.color() == color )
        return -1;

      const Figure & afig = getFigure(field.color(), field.index());
      if ( Figure::TypeBishop != afig.getType() && Figure::TypeRook != afig.getType() && Figure::TypeQueen != afig.getType() )
        return -1;

      int dir = FigureDir::dir(afig, king.where());
      return dir >= 0 ? afig.getIndex() : -1;
    }
    return -1;
  }

  inline bool wasValidUnderCheck(const Step & step) const
  {
    const Figure & fig = getFigure(color_, step.index_);
    Figure::Color ocolor = Figure::otherColor(color_);
    if ( Figure::TypeKing == fig.getType() )
      return !step.castle_ && !isAttacked(ocolor, fig.where());

    if ( 1 == chkNum_ )
    {
      THROW_IF( checking_[0] < 0 || checking_[0] >= NumOfFigures, "invalid checking figure index" );

      const Figure & king = getFigure(color_, KingIndex);

      if ( step.rindex_ == checking_[0] )
        return getAttackedFrom(color_, step.from_) < 0;

      const Figure & afig = getFigure(ocolor, checking_[0]);
      THROW_IF( Figure::TypeKing == afig.getType(), "king is attacking king" );
      THROW_IF( !afig, "king is attacked by non existing figure" );

      if ( Figure::TypeKnight == afig.getType() || Figure::TypePawn == afig.getType() )
        return false;

      FPos dp1 = getDeltaPos(step.to_, afig.where());
      FPos dp2 = getDeltaPos(king.where(), step.to_);

      // can protect king. now check if we can move this figure
      if ( FPos(0, 0) != dp1 && dp1 == dp2 )
        return getAttackedFrom(color_, step.from_) < 0;

      return false;
    }

    THROW_IF(2 != chkNum_, "invalid number of checking figures");
    return false;
  }

  inline bool wasValidWithoutCheck(const Step & step) const
  {
    const Figure & fig = getFigure(color_, step.index_);
    Figure::Color ocolor = Figure::otherColor(color_);
    if ( Figure::TypeKing == fig.getType() )
      return !isAttacked(ocolor, fig.where());

    return getAttackedFrom(color_, step.from_) < 0;
  }

  void setFigure(const Figure & );

  // is field 'pos' attacked by given color?
  bool isAttacked(const Figure::Color c, int pos) const
  {
    for (int i = KingIndex; i >= 0; --i)
    {
      const Figure & fig = getFigure(c, i);
      if ( !fig )
        continue;

      int dir = FigureDir::dir(fig, pos);
      if ( (dir < 0) || (Figure::TypePawn == fig.getType() && (2 == dir || 3 == dir)) || (Figure::TypeKing == fig.getType() && dir > 7) )
        continue;

      if ( Figure::TypePawn == fig.getType() || Figure::TypeKnight == fig.getType() || Figure::TypeKing == fig.getType() )
        return true;

      FPos dp = getDeltaPos(fig.where(), pos);

      THROW_IF( FPos(0, 0) == dp, "invalid attacked position" );

      FPos p = FPosIndexer::get(pos) + dp;
      const FPos & figp = FPosIndexer::get(fig.where());
      bool have_fig = false;
      for ( ; p != figp; p += dp)
      {
        const Field & field = getField(p);
        if ( !field )
          continue;

        if ( field.color() == c && Figure::TypeQueen == field.type() )
        {
          return true;
        }

        if ( field.color() == c && (Figure::TypeBishop == field.type() || Figure::TypeRook == field.type()) )
        {
          const Figure & afig = getFigure(c, field.index());
          int dir = FigureDir::dir(afig, pos);
          if ( dir >= 0 )
            return true;

          have_fig = true;
          break;
        }

        if ( (Figure::TypeKnight == field.type()) ||
          (field.color() != c && Figure::TypeKing != field.type()) ||
          (field.color() == c && Figure::TypeKing == field.type()) )
        {
          have_fig = true;
          break;
        }

        if ( Figure::TypePawn == field.type() )
        {
          const Figure & pawn = getFigure(field.color(), field.index());
          int d = FigureDir::dir(pawn, pos);
          if ( 0 == d || 1 == d )
            return true;

          have_fig = true;
          break;
        }
      }

      if ( !have_fig )
        return true;
    }

    return false;
  }

  inline void verifyChessDraw()
  {
    if ( drawState() )
      return;

    can_win_[0] = can_win_[1] = true;
    for (int n = 0; n < 2; ++n)
    {
      Figure::Color c = (Figure::Color)n;
      Figure::Color oc = (Figure::Color)((n + 1) & 1);
      if ( fmgr_.pawns(c) > 0 || fmgr_.rooks(c) > 0 || fmgr_.queens(c) > 0 )
        continue;

      if ( (fmgr_.count(c) == 0) ||
		       (fmgr_.knights(c) <= 2 &&  fmgr_.bishops(c) == 0) ||
		       (fmgr_.knights(c) == 0 && (fmgr_.bishops_b(c) == 0 || fmgr_.bishops_w(c) == 0)) )
      {
        can_win_[n] = false;
        continue;
      }
    }

    if ( !can_win_[0] && !can_win_[1] )
      state_ = DrawInsuf;
  }

  bool hasSteps();

  inline void doCastling(const Figure & king, int8 dir, Step & step) const
  {
    THROW_IF( 4 != (king.where() & 7) || (Figure::ColorBlack == king.getColor() && 7 != (king.where()>>3)) || (Figure::ColorWhite == king.getColor() && 0 != (king.where()>>3) ),
      "invalid king position for castling" );

    int idx = -1, ridx = -1;
    int ifrom, ito, rto;
    if ( Figure::ColorBlack == color_ )
    {
      if ( 8 == dir )
      {
        idx = 58;
        ridx = 56;
        ifrom = 57;
        ito = 60;
        rto = 59;
      }
      else
      {
        idx = 62;
        ridx = 63;
        ifrom = 61;
        ito = 63;
        rto = 61;
      }
    }
    else
    {
      if ( 8 == dir )
      {
        idx = 2;
        ridx = 0;
        ifrom = 1;
        ito = 4;
        rto = 3;
      }
      else
      {
        idx = 6;
        ridx = 7;
        ifrom = 5;
        ito = 7;
        rto = 5;
      }
    }

    const Field & rf = getField(ridx);
    if ( !rf || Figure::TypeRook != rf.type() || rf.color() != king.getColor() )
      return;

    const Figure & rook = getFigure(color_, rf.index());

    THROW_IF( !rook, "rook is invalid" );

    if ( !rook.isFirstStep() )
      return;

    int i = ifrom;
    for (; i < ito && !getField(i); ++i);

    if ( i == ito )
    {
      step.from_ = king.where();
      step.to_ = idx;
      step.rook_from_ = ridx;
      step.rook_to_ = rto;
      step.castle_ = 1;
      step.index_ = king.getIndex();
      step.rookidx_ = rook.getIndex();
      step.rindex_ = -1;
      step.newType_ = 0;
      step.ftype_ = Figure::TypeKing;

      THROW_IF(step.invalid(), "invalid step");
    }
  }

  inline void setPawnStep(StepSorter & ssorter, int rindex, int newType, int toIdx, const Figure & fig, bool verifyCheck = false)
  {
    Step & step = ssorter.back();
    step.clear();

    step.index_ = fig.getIndex();
    step.rindex_ = rindex;
    step.newType_ = newType;
    step.to_ = toIdx;
    step.from_ = fig.where();
    step.ftype_ = Figure::TypePawn;

    bool ok = !verifyCheck;
    if ( verifyCheck && doStep(step) )
    {
      ok = /*wasStepValid(step) && */isChecking(step);
      undoStep(step);
    }

    if ( !ok )
      return;

    ssorter.push(step, *this);
  }

private:

  uint8 castle_[2];
  int8 checking_[2];
  int8 chkNum_;

  // en-passant. must be removed while coping. it has color different from "color_"
  int8  fakeIndex_;

  State  state_;
  Figure::Color color_;

  // 0 - black, 1 - white
  Field  fields_[NumOfFields];
  Figure figures_[2*NumOfFigures];
  FiguresManager fmgr_;

  // set in verifyChessDraw() method
  bool can_win_[2];
  uint8 stages_[2];
};
