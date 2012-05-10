#pragma once

#include "BasicTypes.h"
#include "Field.h"

#pragma pack (push, 1)

__declspec (align(1))
struct PackedMove
{
  PackedMove() : from_(0), to_(0), new_type_(0)
  {}

  uint16 from_ : 6,
         to_ : 6,
         new_type_ : 4;

  operator bool () const
  {
    return from_ || to_;
  }

  bool operator == (const PackedMove & other) const
  {
    return *reinterpret_cast<const uint16*>(this) == *reinterpret_cast<const uint16*>(&other);
  }

  bool operator != (const PackedMove & other) const
  {
    return *reinterpret_cast<const uint16*>(this) != *reinterpret_cast<const uint16*>(&other);
  }
};

/*! basic move structure.
    contains minimal necessary information it could be stored in hash etc.
 */
__declspec (align(1))
struct Move
{
#ifndef NDEBUG
  // make all values invalid
  Move() : from_(-1), to_(-1), rindex_(100), new_type_(10), checkVerified_(1), alreadyDone_(1), flags_(-1),
    fkiller_(1), checkFlag_(1), threat_(1), srt_score_(0), strong_(1), discoveredCheck_(1), seen_(1)
  {}
#endif

  /// index of field go from
  int8 from_;

  /// index of field go to
  int8 to_;

  /// index of eaten (removed) figure
  int8 rindex_;

  /// new type while pawn promotion
  int8 new_type_;

  /// flags
  uint16 checkVerified_ : 1,
         alreadyDone_ : 1,
		     fkiller_ : 1,
         checkFlag_ : 1,
         threat_ : 1,
         strong_ : 1,
         discoveredCheck_ : 1,
         seen_ : 1,
         flags_;

  unsigned srt_score_;

  inline void clear()
  {
    from_ = -1;
    to_ = -1;
    new_type_ = 0;
    rindex_ = -1;
    srt_score_ = 0;//-std::numeric_limits<int>::max();

    checkVerified_ = 0;
    alreadyDone_ = 0;
	  fkiller_ = 0;
    checkFlag_ = 0;
    threat_ = 0;
    strong_ = 0;
    discoveredCheck_ = 0;
    flags_ = 0;
    seen_ = 0;
  }

  inline void clearFlags()
  {
    checkVerified_ = 0;
    alreadyDone_  = 0;
    fkiller_ = 0;
    checkFlag_ = 0;
    threat_ = 0;
    strong_ = 0;
    discoveredCheck_ = 0;
    flags_ = 0;
    seen_ = 0;
  }

  inline void set(int8 from, int8 to, int8 rindex, int8 new_type, int8 checkVerified)
  {
    from_ = from;
    to_ = to;
    rindex_ = rindex;
    new_type_ = new_type;
    checkVerified_ = checkVerified;
    alreadyDone_ = 0;
	  fkiller_ = 0;
    checkFlag_ = 0;
    threat_ = 0;
    strong_ = 0;
    discoveredCheck_ = 0;
    flags_ = 0;
    seen_ = 0;
    srt_score_ = 0;//-std::numeric_limits<int>::max();
  }

  inline operator bool () const
  {
    return to_ >= 0;
  }

  // compare only first 4 bytes
  inline bool operator == (const Move & other) const
  {
    return *reinterpret_cast<const uint32*>(this) == *reinterpret_cast<const uint32*>(&other);
  }

  // compare only first 4 bytes
  inline bool operator != (const Move & other) const
  {
    return *reinterpret_cast<const uint32*>(this) != *reinterpret_cast<const uint32*>(&other);
  }
};

/*! complete move structure with all necessary unmove information
  */
__declspec (align(1))
struct MoveCmd : public Move
{
  MoveCmd() {}

  MoveCmd(const Move & move) : Move(move)
  {}

  MoveCmd & operator = (const Move & move)
  {
    *((Move*)this) = move;
    return *this;
  }

  /// Zobrist key - used in fifty-move-rule detector
  uint64 zcode_;
  uint64 zcode_old_;

  /// figures masks
  uint64 mask_[2];

  /// if this move is irreversible, we don't need to enumerate any move in fifty-move-rule detector
  bool  irreversible_;

  /// state of board after move
  uint8 state_;

  /// fields, figure and rook moved to
  Field field_to_;
  Field field_rook_to_;

  /// index of moved figure
  int8 index_;

  /// en-passant pawn index
  int8 en_passant_;

  /// castle: 0 - no castle, 1 - short castle, 2 - long castle
  int8 castle_;

  /// old position of rook while castle
  int8 rook_from_;

  /// new position of rook while castle
  int8 rook_to_;

  /// true - if figure haven't moved yet
  bool first_move_;

  /// type of eaten figure to restore it while unmove
  int8 eaten_type_;

  /// used to restore rook position after castle
  int8 rook_index_;

  /// restore old board state while unmove
  int8 old_state_;

  /// do we need Undo
  bool need_undo_;

  /// do we need unmake
  bool need_unmake_;

  /// number of checking figures
  uint8 checkingNum_;

  /// checking figures indices
  uint8 checking_[2];

  /// previous checking figures number
  uint8 old_checkingNum_;

  /// previous checking figures indices
  uint8 old_checking_[2];

  /// save stage of current color
  uint8 stage_;

  /// weight for sorting moves for alpha-betta
  ScoreType weight_;

  /// fifty moves rule
  uint8 fifty_moves_;

  /// repetitions counter
  uint8 reps_counter_;

  //// 1st - color, 2nd - index of castle's possibility for zobrist key. 0 - short, 1 - long
  //bool castle_index_[2][2];

  bool can_win_[2];

  /// reduced || extended flags
  bool reduced_;
  bool extended_;

  /// performs clear operation before doing movement. it clears only undo info
  void clearUndo()
  {
    field_to_.clear();
    field_rook_to_.clear();

    en_passant_ = -1;
    castle_ = 0;
    eaten_type_ = 0;
    rook_index_ = -1;
    old_state_ = 0;
    state_ = 0;
    first_move_ = false;
    checkingNum_ = 0;
    need_undo_ = false;
    need_unmake_ = false;
    rook_to_ = -1;
    rook_from_ = -1;
    reduced_ = false;
    extended_ = false;
  }
};

#pragma pack (pop)