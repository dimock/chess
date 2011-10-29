#pragma once

#include "BasicTypes.h"

#pragma pack (push, 1)

/*! basic move structure.
    contains minimal necessary information it could be stored in hash etc.
 */
__declspec (align(1))
struct Move
{
  /// index of field go from
  int8 from_;

  /// index of field go to
  int8 to_;
};

/*! complete move structure with all necessary unmove information
  */
__declspec (align(1))
struct MoveCmd : public Move
{
  /// index of moved figure
  int8 index_;

  /// index of eaten (removed) figure
  int8 rindex_;

  /// en-passant pawn index
  int8 en_passant_;

  /// castle: 0 - no castle, 1 - short castle, 2 - long castle
  int8 castle_;

  /// 1 - if figure haven't moved yet
  int8 first_move_;

  /// type of eaten figure to restore it while unmove
  int8 eaten_type_;

  /// restore old board state while unmove
  int8 old_state_;

  /// number of checking figures
  uint8 checkingNum_;

  /// checking figures indices
  uint8 checking_[2];

  /// previous checking figures number
  uint8 old_checkingNum_;

  /// previous checking figures indices
  uint8 old_checking_[2];

  /// weight for sorting moves for alpha-betta
  WeightType weight_;


  /// performs clear operation before doing movement. it clears only undo info
  void clearUndo()
  {
    en_passant_ = -1;
    castle_ = 0;
    eaten_type_ = 0;
    old_state_ = 0;
    checkingNum_ = 0;
  }
};

#pragma pack (pop)