
/*----------------------------------------------------------------------+
 |                                                                      |
 |      exchange.c -- Static exchange evaluation (SEE)                  |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Static exchange evaluation (SEE)
 *
 *      This module does the static exchange evalution (SEE).
 *      Lots of work has been done in generate.c already: during
 *      move generation data on attackers and defenders has been
 *      collected.
 *
 *  History:
 *      2008-01-13 (marcelk) Import from Rookie 2.0 sources, add comments
 *      2009-09-19 (marcelk) Adapt for Rookie 3.0 structures
 *      2009-12-xx (marcelk) Clean up and optimized
 */

/*----------------------------------------------------------------------+
 |      Copyright                                                       |
 +----------------------------------------------------------------------*/

/*
 *  This file is part of the Rookie(TM) Chess Program
 *  Copyright (C) 1992-2009, Marcel van Kervinck
 *  All rights reserved.
 */

/*----------------------------------------------------------------------+
 |      Includes                                                        |
 +----------------------------------------------------------------------*/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/*
 *  Base include
 */
#include "cplus.h"

/*
 *  Own interface include
 */
#include "board.h"
#include "exchange.h"

/*
 *  Other includes
 */
#include "intern.h"

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*
 *  PIECE LIST WORD LAYOUT
 *
 *      15 bits encode the piece list for one side. The piece fields are
 *      not aligned on exact bit-boundaries to make it fit in small space.
 *
 *        2 bits      1 bit            12 bits
 *      |         |           |                               |
 *      +---------+-----------+-------+-------+-------+-------+
 *      | UPFRONT | LAST_RANK | ROYAL * ROOK  * MINOR * PAWN  |
 *      +---------+-----------+-------+-------+-------+-------+
 *          0..3       0..1     0..10   0..10   0..11   0..2
 *
 *  PIECE COUNT FIELDS
 *
 *      ROYAL, QUEEN, ROOK, PAWN count the number of such pieces in the list.
 *      MINOR counts knights and bishops. ROYAL counts queens and kings.
 *
 *      The maximum number of queens is 9: 8 promoted pawns + 1 original queen.
 *      We treat the king as an additional queen.
 *      The maximum number of rooks is 10: 8 promoted pawns + 2 original rooks.
 *      The maximum number of minors is 11: 8 promoted pawns + 2 original
 *      knights + 1 original bishop. The 2 original bishops can't both attack
 *      the same square because they are on a different square color.
 *
 *      The space needed for PAWN .. ROYAL is 3*12*11*11 = 4356, which
 *      requires 12.1 bits. However, actual values can't exceed 4041, so 12
 *      bits is enough (10 royal + 2 rook + 3 minor + 0 pawn = 4041).
 *
 *  LAST_RANK FIELD
 *
 *      The `LAST_RANK' flag denotes that a capture by a pawn leads to
 *      promotion. The exchange evaluation assumes that such promotion is to
 *      a queen.
 *
 *  HASHING
 *
 *      A nice property of this tightly packed representation is that we get
 *      reasonable good hashing by using the XOR operator on two piece lists.
 *      We exploit this in the implementation.
 */

#define RANGE_PAWN      (1+2)
#define RANGE_MINOR     (1+11)
#define RANGE_ROOK      (1+10)
#define RANKE_ROYAL     (1+10)

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

/*
 *  Maps a piece to an index
 */
const short exchange_piece_to_list[] = {
        [board_white_pawn]         = EXCHANGE_LIST_PAWN,
        [board_white_pawn_rank2]   = EXCHANGE_LIST_PAWN,
        [board_white_pawn_rank7]   = EXCHANGE_LIST_PAWN,

        [board_black_pawn]         = EXCHANGE_LIST_PAWN,
        [board_black_pawn_rank7]   = EXCHANGE_LIST_PAWN,
        [board_black_pawn_rank2]   = EXCHANGE_LIST_PAWN,

        [board_white_knight]       = EXCHANGE_LIST_MINOR,
        [board_black_knight]       = EXCHANGE_LIST_MINOR,

        [board_white_bishop_light] = EXCHANGE_LIST_MINOR,
        [board_white_bishop_dark]  = EXCHANGE_LIST_MINOR,
        [board_black_bishop_light] = EXCHANGE_LIST_MINOR,
        [board_black_bishop_dark]  = EXCHANGE_LIST_MINOR,

        [board_white_rook]         = EXCHANGE_LIST_ROOK,
        [board_white_rook_castle]  = EXCHANGE_LIST_ROOK,
        [board_black_rook]         = EXCHANGE_LIST_ROOK,
        [board_black_rook_castle]  = EXCHANGE_LIST_ROOK,

        [board_white_queen]        = EXCHANGE_LIST_ROYAL,
        [board_black_queen]        = EXCHANGE_LIST_ROYAL,

        [board_white_king]         = EXCHANGE_LIST_ROYAL,
        [board_white_king_castle]  = EXCHANGE_LIST_ROYAL,
        [board_black_king]         = EXCHANGE_LIST_ROYAL,
        [board_black_king_castle]  = EXCHANGE_LIST_ROYAL,
};

/*
 *  Gives the piece value of something on the board (absolute value).
 *  This is used by generate.c to include the value of the captured piece.
 */
const short exchange_piece_value[BOARD_PIECE_TYPES] = {
        [board_empty]              = EXCHANGE_NEUTRAL,

        [board_white_pawn]         = EXCHANGE_NEUTRAL + 1 * EXCHANGE_UNIT,
        [board_white_pawn_rank2]   = EXCHANGE_NEUTRAL + 1 * EXCHANGE_UNIT,
        [board_white_pawn_rank7]   = EXCHANGE_NEUTRAL + 1 * EXCHANGE_UNIT,

        [board_black_pawn]         = EXCHANGE_NEUTRAL + 1 * EXCHANGE_UNIT,
        [board_black_pawn_rank7]   = EXCHANGE_NEUTRAL + 1 * EXCHANGE_UNIT,
        [board_black_pawn_rank2]   = EXCHANGE_NEUTRAL + 1 * EXCHANGE_UNIT,

        [board_white_knight]       = EXCHANGE_NEUTRAL + 3 * EXCHANGE_UNIT,
        [board_black_knight]       = EXCHANGE_NEUTRAL + 3 * EXCHANGE_UNIT,

        [board_white_bishop_light] = EXCHANGE_NEUTRAL + 3 * EXCHANGE_UNIT,
        [board_white_bishop_dark]  = EXCHANGE_NEUTRAL + 3 * EXCHANGE_UNIT,
        [board_black_bishop_light] = EXCHANGE_NEUTRAL + 3 * EXCHANGE_UNIT,
        [board_black_bishop_dark]  = EXCHANGE_NEUTRAL + 3 * EXCHANGE_UNIT,

        [board_white_rook]         = EXCHANGE_NEUTRAL + 5 * EXCHANGE_UNIT,
        [board_white_rook_castle]  = EXCHANGE_NEUTRAL + 5 * EXCHANGE_UNIT,
        [board_black_rook]         = EXCHANGE_NEUTRAL + 5 * EXCHANGE_UNIT,
        [board_black_rook_castle]  = EXCHANGE_NEUTRAL + 5 * EXCHANGE_UNIT,

        [board_white_queen]        = EXCHANGE_NEUTRAL + 9 * EXCHANGE_UNIT,
        [board_black_queen]        = EXCHANGE_NEUTRAL + 9 * EXCHANGE_UNIT,

        [board_white_king]         = EXCHANGE_NEUTRAL + 9 * EXCHANGE_UNIT,
        [board_white_king_castle]  = EXCHANGE_NEUTRAL + 9 * EXCHANGE_UNIT,
        [board_black_king]         = EXCHANGE_NEUTRAL + 9 * EXCHANGE_UNIT,
        [board_black_king_castle]  = EXCHANGE_NEUTRAL + 9 * EXCHANGE_UNIT,
};

/*
 *  Gives the piece value of something on the board (absolute value).
 *  This is used by generate.c as a heurisic to prevent heavy pieces
 *  moving to defended squares.
 */
const short exchange_negative_piece_value[BOARD_PIECE_TYPES] = {
        [board_empty]              = EXCHANGE_NEUTRAL,

        [board_white_pawn]         = EXCHANGE_NEUTRAL - 1 * EXCHANGE_UNIT,
        [board_white_pawn_rank2]   = EXCHANGE_NEUTRAL - 1 * EXCHANGE_UNIT,
        [board_white_pawn_rank7]   = EXCHANGE_NEUTRAL - 1 * EXCHANGE_UNIT,

        [board_black_pawn]         = EXCHANGE_NEUTRAL - 1 * EXCHANGE_UNIT,
        [board_black_pawn_rank7]   = EXCHANGE_NEUTRAL - 1 * EXCHANGE_UNIT,
        [board_black_pawn_rank2]   = EXCHANGE_NEUTRAL - 1 * EXCHANGE_UNIT,

        [board_white_knight]       = EXCHANGE_NEUTRAL - 3 * EXCHANGE_UNIT,
        [board_black_knight]       = EXCHANGE_NEUTRAL - 3 * EXCHANGE_UNIT,

        [board_white_bishop_light] = EXCHANGE_NEUTRAL - 3 * EXCHANGE_UNIT,
        [board_white_bishop_dark]  = EXCHANGE_NEUTRAL - 3 * EXCHANGE_UNIT,
        [board_black_bishop_light] = EXCHANGE_NEUTRAL - 3 * EXCHANGE_UNIT,
        [board_black_bishop_dark]  = EXCHANGE_NEUTRAL - 3 * EXCHANGE_UNIT,

        [board_white_rook]         = EXCHANGE_NEUTRAL - 5 * EXCHANGE_UNIT,
        [board_white_rook_castle]  = EXCHANGE_NEUTRAL - 5 * EXCHANGE_UNIT,
        [board_black_rook]         = EXCHANGE_NEUTRAL - 5 * EXCHANGE_UNIT,
        [board_black_rook_castle]  = EXCHANGE_NEUTRAL - 5 * EXCHANGE_UNIT,

        [board_white_queen]        = EXCHANGE_NEUTRAL - 9 * EXCHANGE_UNIT,
        [board_black_queen]        = EXCHANGE_NEUTRAL - 9 * EXCHANGE_UNIT,

        [board_white_king]         = EXCHANGE_NEUTRAL - 9 * EXCHANGE_UNIT,
        [board_white_king_castle]  = EXCHANGE_NEUTRAL - 9 * EXCHANGE_UNIT,
        [board_black_king]         = EXCHANGE_NEUTRAL - 9 * EXCHANGE_UNIT,
        [board_black_king_castle]  = EXCHANGE_NEUTRAL - 9 * EXCHANGE_UNIT,
};

const short exchange_put_upfront[BOARD_PIECE_TYPES] = {
        [board_white_pawn]         = (0 << 13) - EXCHANGE_LIST_PAWN,
        [board_white_pawn_rank2]   = (0 << 13) - EXCHANGE_LIST_PAWN,
        [board_white_pawn_rank7]   = (0 << 13) - EXCHANGE_LIST_PAWN,

        [board_black_pawn]         = (0 << 13) - EXCHANGE_LIST_PAWN,
        [board_black_pawn_rank7]   = (0 << 13) - EXCHANGE_LIST_PAWN,
        [board_black_pawn_rank2]   = (0 << 13) - EXCHANGE_LIST_PAWN,

        [board_white_knight]       = (1 << 13) - EXCHANGE_LIST_MINOR,
        [board_black_knight]       = (1 << 13) - EXCHANGE_LIST_MINOR,

        [board_white_bishop_light] = (1 << 13) - EXCHANGE_LIST_MINOR,
        [board_white_bishop_dark]  = (1 << 13) - EXCHANGE_LIST_MINOR,
        [board_black_bishop_light] = (1 << 13) - EXCHANGE_LIST_MINOR,
        [board_black_bishop_dark]  = (1 << 13) - EXCHANGE_LIST_MINOR,

        [board_white_rook]         = (2 << 13) - EXCHANGE_LIST_ROOK,
        [board_white_rook_castle]  = (2 << 13) - EXCHANGE_LIST_ROOK,
        [board_black_rook]         = (2 << 13) - EXCHANGE_LIST_ROOK,
        [board_black_rook_castle]  = (2 << 13) - EXCHANGE_LIST_ROOK,

        [board_white_queen]        = (3 << 13) - EXCHANGE_LIST_ROYAL,
        [board_black_queen]        = (3 << 13) - EXCHANGE_LIST_ROYAL,

        [board_white_king]         = (3 << 13) - EXCHANGE_LIST_ROYAL,
        [board_white_king_castle]  = (3 << 13) - EXCHANGE_LIST_ROYAL,
        [board_black_king]         = (3 << 13) - EXCHANGE_LIST_ROYAL,
        [board_black_king_castle]  = (3 << 13) - EXCHANGE_LIST_ROYAL,
};

/*
 *  The lookup table is static and thus shared with any other threads.
 *  It is filled dynamically. We assume the CPU performs atomic reads
 *  and writes, so there is no need to lock the table on access to make
 *  it thread-safe.
 *
 *  We construct the table index by combining 'defenders' and 'attackers'
 *  with a shift and exclusive OR.
 * 
 *  We store the 12 bits of 'defenders' in the table slot as key.
 *  Note that storing the 'last_rank' bit is not needed because it can be
 *  derived from the index and the 12 bits key.
 *
 *  This leaves 4 bits for the value. The wanted range is from 0 (no gain)
 *  to 17 (capture a queen while promoting). 18 values doesn't fit in 4 bits
 *  though. We clip for that reason. The clipping is at +14, not +15, so that
 *  we obtain the following ranges for moves:
 *      Good captures   0xf0?? ~ 0xfe?? (0xff?? is used for the ttable move)
 *      Regular moves   0x01?? ~ 0x0f?? (0x00?? is reserved)
 *
 *  TODO: check if 0 is a valid initializer
 *  A: only if the 'defenders != 0' stays in the outer inline function.
 */

static unsigned short exchange_table[ 1 << 15 ];

long long board_exchange_table_miss_counter = 0LL;

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      exchange_reset_caches                                           |
 +----------------------------------------------------------------------*/

err_t
exchange_reset_caches(void)
{
        err_t err = OK;

        memset(&exchange_table, 0, sizeof(exchange_table));

        return err;
}

/*----------------------------------------------------------------------+
 |      exchange_reset_stats                                            |
 +----------------------------------------------------------------------*/

err_t
exchange_reset_stats(void)
{
        err_t err = OK;

        board_exchange_table_miss_counter = 0;

        return err;
}

/*----------------------------------------------------------------------+
 |      next_upfront                                                    |
 +----------------------------------------------------------------------*/

/*
 *  Picks the next capturer from the set and places it 'upfront'
 */

static inline
int next_upfront(int defenders)
{
        assert(defenders != 0);

        int d = defenders;

        if ((d % RANGE_PAWN) != 0) {
                return defenders + (0 << 13) - EXCHANGE_LIST_PAWN;
        }
        d /= RANGE_PAWN;

        if ((d % RANGE_MINOR) != 0) {
                return defenders + (1 << 13) - EXCHANGE_LIST_MINOR;
        }
        d /= RANGE_MINOR;

        if ((d % RANGE_ROOK) != 0) {
                return defenders + (2 << 13) - EXCHANGE_LIST_ROOK;
        } else {
                assert((d / RANGE_ROOK) != 0);
                return defenders + (3 << 13) - EXCHANGE_LIST_ROYAL;
        }
}

/*----------------------------------------------------------------------+
 |      exchange_evaluate                                               |
 +----------------------------------------------------------------------*/

/*
 *  Static Exchange Evaluation (SEE).
 *
 *  We assume that the attacker has just moved, thus the defending side
 *  is to move next. This function calculates the maximum retaliation that
 *  the defender can obtain by following an optimal exchange sequence.
 *
 *  Input are the two piece sets, each represented by a compact word.
 *  Returned is the static exchange evaluation, scaled by EXCHANGE_UNIT
 *  for convenience.
 *
 *  Defenders
 *                14-13      12               11-0
 *              +-------+---------+-------------------------------+
 *              |   0   |last_rank|    set of defending pieces    |
 *              +-------+---------+-------------------------------+
 *
 *  'defenders' represents the set of defending pieces. 0 means none.
 *  The 'last_rank' flag indicates that the exchange occurs on the last rank.
 *  We assume that the defender will always capture with its least-valued
 *  piece. Interestingly, this is also the optimal strategy on the last rank.
 *
 *  Attackers
 *                14-13      12               11-0
 *              +-------+---------+-------------------------------+
 *              |upfront|last_rank|    set of attacking pieces    |
 *              +-------+---------+-------------------------------+
 *
 *  'attackers' represents the set of attacking pieces. This set can't be
 *  empty because of the assumption that this side has just moved.
 *  This set has a 2-bit field 'first' which indicates what piece moved
 *  just before, because that is the first to get captured by the defender.
 *
 *  The piece encoded in 'first' is not present in the 12 bit set. The
 *  reason is that this allows for 3 pawns (one just moved, 2 defending)
 *  while the set only has space for 2 (otherwise more than 12 bits are
 *  needed for the set and everything doesn't fit anymore).
 *  Therefore in this set 0 means: one unsupported pawn.
 *
 *  The returned result can't be negative because the side to move has the
 *  right not to continue the exchange sequence.
 *
 *  The function caches earlier results for improved performance.
 */

int exchange_evaluate_fn(int defenders, int attackers)
{
        /*
         *  Input check
         */
        assert(defenders != 0);

        assert((defenders & 0x1fff) == defenders);
        assert((attackers & 0x7fff) == attackers);

        assert((defenders & attackers & EXCHANGE_LAST_RANK) == 0);

        int store = defenders & 0x0fff;

        int hash = (store << 3) ^ defenders ^ attackers;

        assert(hash >= 0);
        assert(hash < 0x8000);

        /*
         *  Hashing should be such that we can reconstruct the two inputs
         *  from the hash and the stored lower 12 bits of the defenders.
         *  There is one ambiguity allowed: which of the two inputs has the
         *  LAST_RANK bit set, if any.
         *  Verify the hashing inverse with assertions.
         */
        assert(((hash ^ (store << 3))   & EXCHANGE_LAST_RANK) ==
                ((attackers ^ defenders) & EXCHANGE_LAST_RANK));
        assert(((hash ^ (store << 3) ^ store) & ~EXCHANGE_LAST_RANK) ==
                (attackers                     & ~EXCHANGE_LAST_RANK));

        /*
         *  Consult the lookup table first
         */
        unsigned short lookup = exchange_table[hash]; /* Thread-safe */

        if (((lookup ^ defenders) & 0x0fff) == 0) {
                assert(EXCHANGE_UNIT == 0x0100);
                return (lookup & 0xf000) >> 4;
        }

        /*
         *  Table missed, calculate exchange sequence result
         */
        board_exchange_table_miss_counter++;

        /*
         *  Value of captured piece
         */

        static const int victim_value[4] = {
                1 * EXCHANGE_UNIT,        // pawn
                3 * EXCHANGE_UNIT,        // minor (bishop or knight)
                5 * EXCHANGE_UNIT,        // rook
                9 * EXCHANGE_UNIT,        // royal (king or queen)
        };
        int result = victim_value[ attackers >> 13 ];

        /*
         *  Fix up a false LAST_RANK flag in some promotions captures
         */

        if (defenders == EXCHANGE_LAST_RANK) {
                return 0; // TODO: fix the flow of control (and cache)
        }

        /*
         *  Select next capturer
         */

        if ((defenders & EXCHANGE_LAST_RANK) != 0) {

                /*
                 *  Capture with pawn and promotion
                 */
                switch ((defenders & 0x0fff) % RANGE_PAWN) {
                case 0:
                        /* No pawn present... Clear the LAST_RANK flag and proceed as normal */
                        defenders = next_upfront(defenders & ~EXCHANGE_LAST_RANK);
                        break;
                case 1:
                        result += 8 * EXCHANGE_UNIT;
                        defenders += (3 << 13) - EXCHANGE_LIST_PAWN - EXCHANGE_LAST_RANK;
                        assert((defenders & EXCHANGE_LAST_RANK) == 0);
                        break;
                default:
                        result += 8 * EXCHANGE_UNIT;
                        defenders += (3 << 13) - EXCHANGE_LIST_PAWN;
                        assert((defenders & EXCHANGE_LAST_RANK) != 0);
                        break;
                }
        } else {
                /*
                 *  Regular capture
                 */
                defenders = next_upfront(defenders);
                assert(defenders >= 0);
        }

        /*
         *  Recursive evaluation
         */
        attackers &= 0x1fff; /* remove the upfront piece */
        if (attackers != 0) {
                result -= exchange_evaluate_fn(attackers, defenders);

                /*
                 *  If result is negative, don't capture but stand pat
                 */
                if (result < 0) {
                        result = 0;
                }
        }

        assert(result >= 0);

        /*
         *  Clip at +14 so that the SEE result always fits in 5 bits
         *      0xfe00 -> +14 == maximum gain
         *      0xf000 ->  +0 == neutral with capture
         *      0x0f00 ->  -0 == neutral, no capture
         *      0x0100 -> -14 == maximum loss
         */
        if (result > 14 * EXCHANGE_UNIT) {
                result = 14 * EXCHANGE_UNIT;
        }

        /*
         *  Update the lookup table and return
         */

        exchange_table[hash] = (result << 4) | store; /* Thread-safe */

        return result;
}

/*----------------------------------------------------------------------+
 |      exchange_collect_attackers                                      |
 +----------------------------------------------------------------------*/

int exchange_collect_attackers(const struct board *bd, int sq, short bits)
{
        assert(bits != 0);

        const struct board_side *active = &bd->current->active;

        short attackers = 0;

        if ((bits & board_attack_pawn_west) != 0) {
                if (active->color == board_white) {
                        bits ^= board_attack_pawn_west | board_attack_northwest;
                        assert((bits & board_attack_northwest) != 0);
                        if (BOARD_RANK(sq) == BOARD_RANK_8) {
                                attackers |= EXCHANGE_LAST_RANK;
                        }
                } else {
                        bits ^= board_attack_pawn_west | board_attack_southwest;
                        assert((bits & board_attack_southwest) != 0);
                        if (BOARD_RANK(sq) == BOARD_RANK_1) {
                                attackers |= EXCHANGE_LAST_RANK;
                        }
                }
        }

        if ((bits & board_attack_pawn_east) != 0) {
                if (active->color == board_white) {
                        bits ^= board_attack_pawn_east | board_attack_northeast;
                        assert((bits & board_attack_northeast) != 0);
                        if (BOARD_RANK(sq) == BOARD_RANK_8) {
                                attackers |= EXCHANGE_LAST_RANK;
                        }
                } else {
                        bits ^= board_attack_pawn_east | board_attack_southeast;
                        assert((bits & board_attack_southeast) != 0);
                        if (BOARD_RANK(sq) == BOARD_RANK_1) {
                                attackers |= EXCHANGE_LAST_RANK;
                        }
                }
        }

        if ((bits & board_attack_king) != 0) {
                bits ^= board_attack_king;
                attackers += EXCHANGE_LIST_ROYAL;

                if (bits == 0) {
                        return attackers;
                }
        }

        if (bits >= board_attack_knight) {
                do {
                        attackers += EXCHANGE_LIST_MINOR;
                        bits -= board_attack_knight;
                } while (bits >= board_attack_knight);

                if (bits == 0) {
                        return attackers;
                }
        }

        assert((bits & ~BOARD_ATTACK_QUEEN)==0);
        assert(bits != 0);

        unsigned char ray;
        int step;
        int fr;

        ray = 0;
        do {
                ray -= bits;
                ray &= bits;
                bits -= ray;

                step = board_vector_step_compact[DEBRUIJN_INDEX(ray)];
                fr = sq;
                do {
                        do {
                                fr -= step;
                        } while (bd->squares[fr].piece == board_empty);
                        attackers += exchange_piece_to_list[bd->squares[fr].piece];

                } while ((active->attacks[fr] & ray) != 0);

        } while (bits != 0);

        return attackers;
}

/*----------------------------------------------------------------------+
 |      if_defender_not_pinned                                          |
 +----------------------------------------------------------------------*/

/*
 *  Helper function to detect pinned defenders and handle these cases.
 *
 *  Returns 'value' if the piece on defender_sq is not pinned.
 *  Otherwise, store 'value' in bd->extra_defenders[] and return 0.
 */

static inline
int if_defender_not_pinned(struct board *bd, int defender_sq, int value)
{
        int king = bd->current->passive.pieces[0];

        int pin_dir =
                data_sq2sq[defender_sq][king] &
                BOARD_ATTACK_QUEEN &
                bd->current->active.attacks[defender_sq];

        if (pin_dir == 0) {
                return value;
        }

        int step = board_vector_step_compact[DEBRUIJN_INDEX(pin_dir)];
        int sq = defender_sq;
        do {
                sq += step;
        } while (bd->squares[sq].piece == board_empty);

        if (sq != king) {
                return value;
        }

        /*
         *  defender is confirmed pinned. find
         *  the attacker location and mark it
         */

        sq = defender_sq;
        do {
                sq -= step;
        } while (bd->squares[sq].piece == board_empty);

        if ((bd->current->active.attacks[sq] & pin_dir) == 0) {
                bd->extra_defenders[sq] += value;
                return 0;
        } else {
                /*
                 *  from is pinned, but moving the attacker
                 *  maintains the pin because there is a second
                 *  attacker backing it up
                 */
                return value;
        }
}

/*----------------------------------------------------------------------+
 |      exchange_collect_defenders                                      |
 +----------------------------------------------------------------------*/

/*
 *  Collect the defenders to a given square for use in optimistic SEE
 *  and return it as a piece set.
 * 
 *  Optimistic SEE means that whenever in doubt, we err to the
 *  attacker's side. Specifically this means that we:
 *      - Exclude defenders that are pinned against the king.
 *      - Inflate weak defenders hiding behind stronger defenders (because the
 *        list is a set, not an ordered list)
 *  This way good captures won't get dropped by a too pessimistic SEE score.
 */

int exchange_collect_defenders(struct board *bd, int sq, short bits)
{
        assert(bits != 0);

        int defenders = 0;

        const struct board_side *active = &bd->current->active;
        const struct board_side *passive = &bd->current->passive;

        /*
         *  Pawns
         */

        if ((bits & board_attack_pawn_west) != 0) {
                if (passive->color == board_white) {
                        bits ^= board_attack_pawn_west | board_attack_northwest;
                        assert((bits & board_attack_northwest) != 0);
                        if (BOARD_RANK(sq) == BOARD_RANK_8) {
                                defenders |= EXCHANGE_LAST_RANK << 16;
                        }
                } else {
                        bits ^= board_attack_pawn_west | board_attack_southwest;
                        assert((bits & board_attack_southwest) != 0);
                        if (BOARD_RANK(sq) == BOARD_RANK_1) {
                                defenders |= EXCHANGE_LAST_RANK << 16;
                        }
                }
        }

        if ((bits & board_attack_pawn_east) != 0) {
                if (passive->color == board_white) {
                        bits ^= board_attack_pawn_east | board_attack_northeast;
                        assert((bits & board_attack_northeast) != 0);
                        if (BOARD_RANK(sq) == BOARD_RANK_8) {
                                defenders |= EXCHANGE_LAST_RANK << 16;
                        }
                } else {
                        bits ^= board_attack_pawn_east | board_attack_southeast;
                        assert((bits & board_attack_southeast) != 0);
                        if (BOARD_RANK(sq) == BOARD_RANK_1) {
                                defenders |= EXCHANGE_LAST_RANK << 16;
                        }
                }
        }

        /*
         *  King
         */

        if ((bits & board_attack_king) != 0) {
                bits ^= board_attack_king;
                defenders += EXCHANGE_LIST_ROYAL << 16;

                if (bits == 0) {
                        return defenders;
                }
        }

        /*
         *  Knights
         */

        if (bits >= board_attack_knight) {
                const signed char *knights = &passive->pieces[1];

                do {
                        bits -= board_attack_knight;
                        int from;

                        do {
                                from = *knights++; // knights after the king

                        } while (data_sq2sq[sq][from] != board_attack_knight);

                        /*
                         *  Check if this knight is not pinned
                         */

                        defenders += if_defender_not_pinned(bd, from,
                                EXCHANGE_LIST_MINOR << 16);

                } while (bits >= board_attack_knight);

                if (bits == 0) {
                        return defenders;
                }
        }

        assert((bits & ~BOARD_ATTACK_QUEEN) == 0);
        assert(bits != 0);

        /*
         *  Scan each of the rays to see what is there
         */

        unsigned char ray = 0;
        int step;
        int fr;

        do {
                ray -= bits;
                ray &= bits;
                bits -= ray;

                //step = board_vector_step_compact[DEBRUIJN_INDEX(ray)];
                step = board_vector_step[ray];
                int last = 0;
                fr = sq;

                do {
                        do {
                                fr -= step;
                        } while (bd->squares[fr].piece == board_empty);


                        /*
                         *  The piece might be pinned
                         */

                        int next = if_defender_not_pinned(bd, fr,
                                exchange_piece_to_list[ bd->squares[fr].piece ] << 16);
                        if (next == 0) {
                                goto xxx; // TODO: cleanup
                        }

                        /*
                         *  Pretend that pieces behind others are at
                         *  least as strong as the first one (optimistic
                         *  SEE scenario). So Q+R becomes Q+Q.
                         *
                         *  TODO: This can cause overflow in the 12-bits
                         */
                        if (next > last) {
                                last = next;
                        }

                        defenders += last;

                } while ((passive->attacks[fr] & ray) != 0);

                /*
                 *  If there are -attackers- behind the last defender, add them to the
                 *  attacker list. (Behind that we won't look for other defenders anymore).
                 *
                 *      [sq]<----[defender]<----[attacker2]
                 */
                while ((active->attacks[fr] & ray) != 0) {
                        do {
                                fr -= step;
                        } while (bd->squares[fr].piece == board_empty);

                        defenders += exchange_piece_to_list[ bd->squares[fr].piece ];
                }

xxx: pass;
 
        } while (bits != 0);

        return defenders;
}

/*----------------------------------------------------------------------+
 |      exchange_extra_collect_defenders                                |
 +----------------------------------------------------------------------*/

/*
 *  Collect the additional defenders behind the given square in the ray
 *  direction.
 */
 
int exchange_collect_extra_defenders(struct board *bd, int from, short ray)
{
        assert(ray != 0);
        assert((ray & BOARD_ATTACK_QUEEN) == ray);
        assert((ray & (ray - 1)) == 0);

        int step = board_vector_step_compact[DEBRUIJN_INDEX(ray)];

        int last = 0;
        int sq = from;
        int defenders = 0;

        do {
                do {
                        sq -= step;
                } while (bd->squares[sq].piece == board_empty);

                /*
                 *  Verify that this potential extra defender is not
                 *  pinned by another attacker
                 */
                int pin_dir =
                        bd->current->active.attacks[sq] &
                        BOARD_ATTACK_QUEEN;
                if (pin_dir != 0) {
                        int xking = bd->current->passive.pieces[0];
                        pin_dir &= data_sq2sq[sq][xking];

                        if (pin_dir != 0) {
                                /*
                                 *  The extra defender is potentially pinned.
                                 */

                                int pin_step = board_vector_step_compact[DEBRUIJN_INDEX(pin_dir)];

                                if (pin_step + step != 0) { /* eqv: pin_step != -step */

                                        /* Step towards that king */

                                        int sq2 = sq;
                                        do {
                                                sq2 += pin_step;
                                        } while (bd->squares[sq2].piece == board_empty);

                                        if (sq2 == xking) {
                                                /*
                                                 *  Yes, it is really pinned
                                                 */
                                                break;
                                        }
                                }
                        }
                }

                /*
                 *  Not pinned: defender is valid
                 */

                int next = exchange_piece_to_list[ bd->squares[sq].piece ] << 16;

                /*
                 *  Pretend that pieces behind others are at
                 *  least as strong as the first one (optimistic
                 *  SEE scenario). So Q+R becomes Q+Q.
                 *
                 *  TODO: This can cause overflow in the 12-bits
                 */

                if (next > last) {
                        last = next;
                }

                defenders += last;

        } while ((bd->current->passive.attacks[sq] & ray) != 0);

        /*
         *  Any extra attackers? Include them
         */

        while ((bd->current->active.attacks[sq] & ray) != 0) {
                do {
                        sq -= step;
                } while (bd->squares[sq].piece == board_empty);

                defenders += exchange_piece_to_list[ bd->squares[sq].piece ];
        }

        return defenders;
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

