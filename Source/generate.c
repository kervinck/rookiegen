
/*----------------------------------------------------------------------+
 |                                                                      |
 |      generate.c -- Legal move generator                              |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Generator for legal moves in any position
 *
 *  History:
 *      2008-01-13 (marcelk) Tidy up original draft version.
 *      2009-09-18 (marcelk) Introduction of BOARD_EXCHANGE_NEUTRAL
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

/*
 *  Standard includes
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 *  Base include
 */
#include "cplus.h"

/*
 *  Own interface include
 */
#include "board.h"
#include "intern.h"

/*
 *  Other includes
 */
#include "capture.h"
#include "castle.h"
#include "enpassant.h"
#include "move.h"
#include "exchange.h"
#include "promote.h"

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*
 *  IS_LEGAL macro to check if an intended move is legal
 *  (that it doesn't expose the king to an uncovered check).
 *  There is a fast part, which is part of the macro so it can
 *  be inlined, and a slower part, which is in _is_legal().
 *  Worst case, the fast part can't decide, and the slower
 *  part must possibly even trace over the board towards the king.
 */
#define IS_LEGAL(from, to) (\
        ((bd->current->passive.attacks[from] &\
           BOARD_ATTACK_QUEEN &\
           data_sq2sq[from][bd->current->active.pieces[0]] \
         ) == 0) ?\
                true :\
                _is_legal(bd, (from), (to))\
)

/*----------------------------------------------------------------------*/

/*
 *  Generate non-capture of any piece, but not for pawns.
 *  Attempt to do a fair SEE.
 */
#define GENERATE_MOVE_SLOW(from, to, move_maker) do{\
\
        assert(BOARD_SQUARE_IS_VALID(from));\
        assert(BOARD_SQUARE_IS_VALID(to));\
        assert(bd->squares[to].piece == board_empty);\
\
        /*\
         *  Collect defenders\
         */\
        int defenders = 0;\
        int _bits = bd->current->passive.attacks[to];\
        if (_bits != 0) {\
                defenders = exchange_collect_defenders(bd, to, _bits);\
        }\
\
        /*\
         *  Collect attackers\
         */\
        int attackers = 0;\
        _bits = bd->current->active.attacks[to];\
        if (_bits != 0) {\
                attackers = exchange_collect_attackers(bd, to, _bits);\
        }\
\
        attackers += exchange_put_upfront[ bd->squares[from].piece ];\
        assert(attackers >= 0);\
\
        /*\
         *  Calculate static exchange evaluation\
         */\
        int _prescore = EXCHANGE_NEUTRAL -\
                exchange_evaluate(defenders, attackers);\
\
        int _move = MOVE(from, to);\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) (move_maker);\
        (nr_moves)++;\
}while(0)

/*
 *  Generate non-capture of any piece, but not for pawns
 *  Don't attempt to do a fair SEE, instead only look if the
 *  target square is uncontested.
 */
#define GENERATE_MOVE_FAST(from, to, move_maker) do{\
\
        assert(BOARD_SQUARE_IS_VALID(from));\
        assert(BOARD_SQUARE_IS_VALID(to));\
        assert(bd->squares[to].piece == board_empty);\
\
        /*\
         *  Calculate static exchange evaluation\
         */\
        int _prescore;\
        if (bd->current->passive.attacks[to] != 0) {\
                _prescore = exchange_negative_piece_value[ bd->squares[from].piece ];\
        } else {\
                _prescore = EXCHANGE_NEUTRAL;\
        }\
\
        int _move = MOVE(from, to);\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) (move_maker);\
        (nr_moves)++;\
}while(0)

/*
 *  Generate king move
 */
#define GENERATE_KING_MOVE(from, to, move_maker) do{\
        assert(BOARD_SQUARE_IS_VALID(from));\
        assert(BOARD_SQUARE_IS_VALID(to));\
\
        int _move = MOVE(from, to);\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | EXCHANGE_NEUTRAL;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) (move_maker);\
        (nr_moves)++;\
}while(0)

/*
 *  Generate discovered check
 *  The EXCHANGE_GOOD_MOVE_OFFSET must be added in a loop afterwards
 */
#define GENERATE_DISCOVERED_CHECK(from, to, move_maker) do{\
        assert(BOARD_SQUARE_IS_VALID(from));\
        assert(BOARD_SQUARE_IS_VALID(to));\
        assert(bd->squares[to].piece == board_empty);\
\
        int _move = MOVE(from, to);\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | EXCHANGE_NEUTRAL;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) (move_maker);\
        (nr_moves)++;\
}while(0)

/*
 *  Generate king capture
 */
#define GENERATE_KING_CAPTURE(from, to, move_maker) do{\
        assert(BOARD_SQUARE_IS_VALID(from));\
        assert(BOARD_SQUARE_IS_VALID(to));\
\
        int _move = MOVE(from, to);\
        moves_p[nr_moves].bm.prescore =\
                bd->butterfly[_move].prescore |\
                (exchange_piece_value[ bd->squares[to].piece ] +\
                EXCHANGE_GOOD_MOVE_OFFSET);\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) (move_maker);\
        (nr_moves)++;\
}while(0)

/*
 *  Generate en-passant
 *
 *  Don't bother to calculate an accurate SEE value, it is too complicated
 *  and hardly worth the effort.
 */
#define GENERATE_EP(from, to, move_maker) do{\
        assert(BOARD_SQUARE_IS_VALID(from));\
        assert(BOARD_SQUARE_IS_VALID(to));\
\
        int _move = MOVE(from, to);\
        moves_p[nr_moves].bm.prescore =\
                bd->butterfly[_move].prescore |\
                (EXCHANGE_NEUTRAL +\
                EXCHANGE_UNIT +\
                EXCHANGE_GOOD_MOVE_OFFSET);\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) (move_maker);\
        (nr_moves)++;\
}while(0)

/*
 *  Generate a pawn move, but not a capture or promotion
 * 
 *  'dir' is the moving direction, this is used to trace back the pawn
 *  itself and any attackers supporting it from behind.
 */
#define GENERATE_PAWN_MOVE(from, to, move_maker, dir) do{\
        assert(BOARD_SQUARE_IS_VALID(from));\
        assert(BOARD_SQUARE_IS_VALID(to));\
        assert(dir != 0);\
\
        /*\
         *  Collect defenders\
         */\
        int defenders = 0;\
        int _bits = bd->current->passive.attacks[to];\
        if (_bits != 0) {\
                defenders = exchange_collect_defenders(bd, to, _bits);\
        }\
\
        /*\
         *  Collect attackers\
         */\
        _bits = bd->current->active.attacks[to];\
        assert((_bits & dir) == 0);\
        int attackers = exchange_collect_attackers(bd, to, _bits | dir) -\
                EXCHANGE_LIST_PAWN;\
\
        assert(attackers >= 0);\
\
        /*\
         *  Calculate static exchange evaluation\
         */\
        int _prescore = EXCHANGE_NEUTRAL -\
                exchange_evaluate(defenders, attackers);\
\
        int _move = MOVE(from, to);\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) (move_maker);\
        (nr_moves)++;\
}while(0)

/*
 *  Generate white promotions
 */
#define GENERATE_WHITE_PROMOTION(from, to) do{\
        assert(BOARD_SQUARE_IS_VALID(from));\
        assert(BOARD_SQUARE_IS_VALID(to));\
\
        /*\
         *  Collect _defenders\
         */\
        int _bits = bd->current->passive.attacks[to];\
\
        int _uncover_check = bd->current->active.attacks[from] &\
                BOARD_ATTACK_QUEEN;\
        if (_uncover_check != 0) {\
                int _xking = bd->current->passive.pieces[0];\
                _uncover_check &= data_sq2sq[from][_xking];\
                if (_uncover_check != 0) {\
\
                        int _step = board_vector_step_compact[DEBRUIJN_INDEX(_uncover_check)];\
                        assert(_step != 0);\
                        int _sq = from;\
                        do {\
                                _sq += _step;\
                                assert(BOARD_SQUARE_IS_VALID(_sq));\
                        } while (bd->squares[_sq].piece == board_empty);\
\
                        if (_sq == _xking) {\
                                _bits &= board_attack_king;\
                        }\
                }\
        }\
\
        int _defenders = 0;\
        if (_bits != 0) {\
                _defenders = exchange_collect_defenders(bd, to, _bits);\
        }\
        int _dir = data_sq2sq[from][to] & BOARD_ATTACK_QUEEN;\
        if ((bd->current->passive.attacks[from] & _dir) != 0) {\
                _defenders += exchange_collect_extra_defenders(bd, from, _dir);\
        }\
\
        /*\
         *  Collect _attackers\
         */\
        _bits = bd->current->active.attacks[to];\
        if (bd->squares[to].piece == board_empty) {\
                _bits |= board_attack_north;\
        }\
        int _attackers = (3 << 13) - EXCHANGE_LIST_PAWN +\
                exchange_collect_attackers(bd, to, _bits);\
        if (((_attackers & 0x0fff) % 3) == 0) {\
                _attackers &= ~EXCHANGE_LAST_RANK;\
        }\
        assert(_attackers >= 0);\
\
        /*\
         *  Calculate static exchange evaluation\
         */\
        int _prescore =\
                8 * EXCHANGE_UNIT +\
                exchange_piece_value[ bd->squares[to].piece ] -\
                exchange_evaluate(_defenders, _attackers);\
        /* PxQ=Q risks overflow of the result */\
        if (_prescore > EXCHANGE_NEUTRAL + 14 * EXCHANGE_UNIT) {\
                _prescore = EXCHANGE_NEUTRAL + 14 * EXCHANGE_UNIT;\
        };\
\
        /*\
         *  Add bias towards non-losing captures\
         */\
        if (_prescore >= EXCHANGE_NEUTRAL) {\
                _prescore += EXCHANGE_GOOD_MOVE_OFFSET;\
                assert((_prescore & 0xf000) == 0xf000);\
        }\
\
        int _move = MOVE(from, to) ^ XOR_PROM_QUEEN;\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) promote_white_queen;\
        (nr_moves)++;\
\
        _attackers -= 1 << 13;\
        assert(_attackers >= 0);\
        _prescore =\
                4 * EXCHANGE_UNIT +\
                exchange_piece_value[ bd->squares[to].piece ] -\
                exchange_evaluate(_defenders, _attackers);\
\
        if (_prescore >= EXCHANGE_NEUTRAL) {\
                _prescore += EXCHANGE_GOOD_MOVE_OFFSET;\
                assert((_prescore & 0xf000) == 0xf000);\
        }\
\
        _move = MOVE(from, to) ^ XOR_PROM_ROOK;\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) promote_white_rook;\
        (nr_moves)++;\
\
        _attackers -= 1 << 13;\
        assert(_attackers >= 0);\
        _prescore =\
                2 * EXCHANGE_UNIT +\
                exchange_piece_value[ bd->squares[to].piece ] -\
                exchange_evaluate(_defenders, _attackers);\
\
        if (_prescore >= EXCHANGE_NEUTRAL) {\
                _prescore += EXCHANGE_GOOD_MOVE_OFFSET;\
                assert((_prescore & 0xf000) == 0xf000);\
        }\
\
        _move = MOVE(from, to) ^ XOR_PROM_BISHOP;\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) promote_white_bishop;\
        (nr_moves)++;\
\
        _move = MOVE(from, to) ^ XOR_PROM_KNIGHT;\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) promote_white_knight;\
        (nr_moves)++;\
}while(0)

/*
 *  Generate black promotions
 */
#define GENERATE_BLACK_PROMOTION(from, to) do{\
        assert(BOARD_SQUARE_IS_VALID(from));\
        assert(BOARD_SQUARE_IS_VALID(to));\
\
        /*\
         *  Collect _defenders\
         */\
        int _bits = bd->current->passive.attacks[to];\
\
        int _uncover_check = bd->current->active.attacks[from] &\
                BOARD_ATTACK_QUEEN;\
        if (_uncover_check != 0) {\
                int _xking = bd->current->passive.pieces[0];\
                _uncover_check &= data_sq2sq[from][_xking];\
                if (_uncover_check != 0) {\
\
                        int _step = board_vector_step_compact[DEBRUIJN_INDEX(_uncover_check)];\
                        assert(_step != 0);\
                        int _sq = from;\
                        do {\
                                _sq += _step;\
                                assert(BOARD_SQUARE_IS_VALID(_sq));\
                        } while (bd->squares[_sq].piece == board_empty);\
\
                        if (_sq == _xking) {\
                                _bits &= board_attack_king;\
                        }\
                }\
        }\
\
        int _defenders = 0;\
        if (_bits != 0) {\
                _defenders = exchange_collect_defenders(bd, to, _bits);\
        }\
        int _dir = data_sq2sq[from][to] & BOARD_ATTACK_QUEEN;\
        if ((bd->current->passive.attacks[from] & _dir) != 0) {\
                _defenders += exchange_collect_extra_defenders(bd, from, _dir);\
        }\
\
        /*\
         *  Collect _attackers\
         */\
        _bits = bd->current->active.attacks[to];\
        if (bd->squares[to].piece == board_empty) {\
                _bits |= board_attack_south;\
        }\
        int _attackers = (3 << 13) - EXCHANGE_LIST_PAWN +\
                exchange_collect_attackers(bd, to, _bits);\
        if (((_attackers & 0x0fff) % 3) == 0) {\
                _attackers &= ~EXCHANGE_LAST_RANK;\
        }\
        assert(_attackers >= 0);\
\
        /*\
         *  Calculate static exchange evaluation\
         */\
        int _prescore =\
                8 * EXCHANGE_UNIT +\
                exchange_piece_value[ bd->squares[to].piece ] -\
                exchange_evaluate(_defenders, _attackers);\
        /* PxQ=Q risks overflow of the result */\
        if (_prescore > EXCHANGE_NEUTRAL + 14 * EXCHANGE_UNIT) {\
                _prescore = EXCHANGE_NEUTRAL + 14 * EXCHANGE_UNIT;\
        };\
\
        /*\
         *  Add bias towards non-losing captures\
         */\
        if (_prescore >= EXCHANGE_NEUTRAL) {\
                _prescore += EXCHANGE_GOOD_MOVE_OFFSET;\
                assert((_prescore & 0xf000) == 0xf000);\
        }\
\
        int _move = MOVE(from, to) ^ XOR_PROM_QUEEN;\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) promote_black_queen;\
        (nr_moves)++;\
\
        _attackers -= 1 << 13;\
        assert(_attackers >= 0);\
        _prescore =\
                4 * EXCHANGE_UNIT +\
                exchange_piece_value[ bd->squares[to].piece ] -\
                exchange_evaluate(_defenders, _attackers);\
\
        if (_prescore >= EXCHANGE_NEUTRAL) {\
                _prescore += EXCHANGE_GOOD_MOVE_OFFSET;\
                assert((_prescore & 0xf000) == 0xf000);\
        }\
\
        _move = MOVE(from, to) ^ XOR_PROM_ROOK;\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) promote_black_rook;\
        (nr_moves)++;\
\
        _attackers -= 1 << 13;\
        assert(_attackers >= 0);\
        _prescore =\
                2 * EXCHANGE_UNIT +\
                exchange_piece_value[ bd->squares[to].piece ] -\
                exchange_evaluate(_defenders, _attackers);\
\
        if (_prescore >= EXCHANGE_NEUTRAL) {\
                _prescore += EXCHANGE_GOOD_MOVE_OFFSET;\
                assert((_prescore & 0xf000) == 0xf000);\
        }\
\
        _move = MOVE(from, to) ^ XOR_PROM_BISHOP;\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) promote_black_bishop;\
        (nr_moves)++;\
\
        _move = MOVE(from, to) ^ XOR_PROM_KNIGHT;\
        moves_p[nr_moves].bm.prescore = bd->butterfly[_move].prescore | _prescore;\
        moves_p[nr_moves].bm.move = (short) _move;\
        moves_p[nr_moves].bm.make = (voidFn*) promote_black_knight;\
        (nr_moves)++;\
}while(0)

/*----------------------------------------------------------------------*/

/*
 *  TODO: experiments with speed of regular moves
 */

#if 1

#undef GENERATE_MOVE
#define GENERATE_MOVE GENERATE_MOVE_FAST

#undef GENERATE_PAWN_MOVE
#define GENERATE_PAWN_MOVE(a,b,c,d) GENERATE_MOVE_FAST(a,b,c)

#endif

/*----------------------------------------------------------------------*/

/*
 *  Move makers for captures
 */
static
make_move_fn * const capture_fn_table[] = {
        [board_white_king] = capture_with_king,
        [board_black_king] = capture_with_king,
        [board_white_king_castle] = capture_with_white_king_castle,
        [board_black_king_castle] = capture_with_black_king_castle,
        [board_white_queen] = capture_with_queen,
        [board_black_queen] = capture_with_queen,
        [board_white_rook] = capture_with_rook,
        [board_black_rook] = capture_with_rook,
        [board_white_rook_castle] = capture_with_white_rook_castle,
        [board_black_rook_castle] = capture_with_black_rook_castle,
        [board_white_bishop_light] = capture_with_bishop,
        [board_black_bishop_light] = capture_with_bishop,
        [board_white_bishop_dark] = capture_with_bishop,
        [board_black_bishop_dark] = capture_with_bishop,
        // no pawns in this table
};

/*
 *  Move makers for regular moves
 */
static
make_move_fn * const move_fn_table[] = {
        [board_white_king] = move_white_king,
        [board_black_king] = move_black_king,
        [board_white_king_castle] = move_white_king_castle,
        [board_black_king_castle] = move_black_king_castle,
        [board_white_queen] = move_white_queen,
        [board_black_queen] = move_black_queen,
        [board_white_rook] = move_white_rook,
        [board_black_rook] = move_black_rook,
        [board_white_rook_castle] = move_white_rook_castle,
        [board_black_rook_castle] = move_black_rook_castle,
        [board_white_bishop_light] = move_white_bishop,
        [board_black_bishop_light] = move_black_bishop,
        [board_white_bishop_dark] = move_white_bishop,
        [board_black_bishop_dark] = move_black_bishop,
        [board_white_knight] = move_white_knight,
        [board_black_knight] = move_black_knight,
        // no pawns in this table
};

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

static inline
bool uncovers_check(
        struct board *bd,
        int from,
        int xking);

static inline
bool the_path_is_clear(struct board *bd, int a, int b);

static
int generate_moves_to_square(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES],
        int to);

static
int generate_captures_to_square(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES],
        int to);

static inline
int generate_pawn_push_to(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES],
        int to);

static
int generate_en_passant(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES]);

/*----------------------------------------------------------------------*/

static
bool _is_legal(struct board *bd, int from, int to);

/*----------------------------------------------------------------------+
 |      board_generate_moves                                            |
 +----------------------------------------------------------------------*/

/*
 *  Generate all legal moves
 */
int board_generate_all_moves(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES])
{
        int nr_moves;

        if (board_in_check(bd)) {
                /*
                 *  In check, get out
                 */
                nr_moves = board_generate_escapes(bd, moves_p);
        } else {
                /*
                 *  No check, generate captures
                 */
                nr_moves = board_generate_captures_and_promotions(
                        bd, moves_p);

                /*
                 *  And regular moves
                 */
                nr_moves += board_generate_regular_moves(
                        bd, &moves_p[nr_moves]);
        }
        return nr_moves;
}

/*----------------------------------------------------------------------+
 |      board_generate_regular_moves                                    |
 +----------------------------------------------------------------------*/

/*
 *  Generate regular moves
 *  (Non-captures, non-promotions, but including castling)
 */
int board_generate_regular_moves(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES])
{
        int nr_moves = 0;

        signed char *pieces = &bd->current->active.pieces[0];

        int from, to;
        int dir, dirs;

        /*------------------------------------------------------+
         |      Generate king moves                             |
         +------------------------------------------------------*/

        from = *pieces;
        assert(BOARD_SQUARE_IS_VALID(from));
        assert(bd->current->passive.attacks[from] == 0);

        dirs = data_kingtab[from];
        dir = 0;
        do {
                dir -= dirs;
                dir &= dirs;
                dirs -= dir;

                to = from + board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                assert(BOARD_SQUARE_IS_VALID(to));
                assert(to != from);

                if ((bd->squares[to].piece == board_empty) &&
                        (bd->current->passive.attacks[to] == 0))
                {
                        GENERATE_KING_MOVE(from, to, move_fn_table[bd->squares[from].piece]);
                }
        } while (dirs != 0);

        /*------------------------------------------------------+
         |      Loop over the remaining pieces                  |
         +------------------------------------------------------*/

        int king = bd->current->active.pieces[0];

        while ((from = *++pieces) >= 0) {

                /*
                 *  Pre-check for any pins to the king
                 */

                int pin_dirs =
                        bd->current->passive.attacks[from] &
                        data_sq2sq[from][king] &
                        BOARD_ATTACK_QUEEN;

                if (pin_dirs != 0) {
                        assert((pin_dirs & (pin_dirs-1)) == 0);

                        if (the_path_is_clear(bd, from, king)) {
                                // only pin_dirs and reversed are safe
                                pin_dirs |= BOARD_ATTACK_REVERSE(pin_dirs);

                                // all others are unsafe
                                pin_dirs = ~pin_dirs;
                        } else {
                                // all directions are safe
                                pin_dirs = 0;
                        }
                }

                switch (bd->squares[from].piece) {

        /*------------------------------------------------------+
         |      Generate queen moves                            |
         +------------------------------------------------------*/

                case board_white_queen:

                        dirs = data_kingtab[from] & ~pin_dirs;
                        assert(dirs != 0);

                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                assert(len > 0);
                                to = from;
                                do {
                                        to += vector;
                                        if (bd->squares[to].piece != board_empty) {
                                                break;
                                        }
                                        GENERATE_MOVE(from, to, move_white_queen);
                                } while (--len);

                        } while (dirs != 0);

                        break;

                case board_black_queen:

                        dirs = data_kingtab[from] & ~pin_dirs;
                        assert(dirs != 0);

                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                assert(len > 0);
                                to = from;
                                do {
                                        to += vector;
                                        if (bd->squares[to].piece != board_empty) {
                                                break;
                                        }
                                        GENERATE_MOVE(from, to, move_black_queen);
                                } while (--len);
                        } while (dirs != 0);

                        break;

        /*------------------------------------------------------+
         |      Generate rook moves                             |
         +------------------------------------------------------*/

                case board_white_rook:

                        dirs = data_kingtab[from] & BOARD_ATTACK_ROOK & ~pin_dirs;
                        if (dirs == 0) {
                                break;
                        }
                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                assert(len > 0);

                                to = from;
                                do {
                                        to += vector;
                                        if (bd->squares[to].piece != board_empty) {
                                                break;
                                        }
                                        GENERATE_MOVE(from, to, move_white_rook);
                                } while (--len);
                        } while (dirs != 0);

                        break;

                case board_black_rook:

                        dirs = data_kingtab[from] & BOARD_ATTACK_ROOK & ~pin_dirs;
                        if (dirs == 0) {
                                break;
                        }
                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                assert(len > 0);

                                to = from;
                                do {
                                        to += vector;
                                        if (bd->squares[to].piece != board_empty) {
                                                break;
                                        }
                                        GENERATE_MOVE(from, to, move_black_rook);
                                } while (--len);
                        } while (dirs != 0);

                        break;

        /*------------------------------------------------------+
         |      Generate castling                               |
         +------------------------------------------------------*/

                case board_white_rook_castle:
                        dirs = data_kingtab[from] & BOARD_ATTACK_ROOK;
                        // Moving a castle-rook can't uncover check.. Ignore pin_dir
                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                assert(len > 0);

                                to = from;
                                do {
                                        to += vector;
                                        if (bd->squares[to].piece != board_empty) {
                                                break;
                                        }
                                        GENERATE_MOVE(from, to, move_white_rook_castle);
                                } while (--len);
                        } while (dirs != 0);


                        if (from == A1) {
                                if ((bd->squares[B1].piece == board_empty) &&
                                    (bd->squares[C1].piece == board_empty) &&
                                    (bd->squares[D1].piece == board_empty) &&
                                    (bd->current->passive.attacks[C1] == 0) &&
                                    (bd->current->passive.attacks[D1] == 0)
                                ) {
                                        GENERATE_KING_MOVE(E1, C1, castle_white_queen_side);
                                }
                        } else {
                                assert(from == H1);

                                if ((bd->squares[F1].piece == board_empty) &&
                                    (bd->squares[G1].piece == board_empty) &&
                                    (bd->current->passive.attacks[F1] == 0) &&
                                    (bd->current->passive.attacks[G1] == 0)
                                ) {
                                        GENERATE_KING_MOVE(E1, G1, castle_white_king_side);
                                }
                        }

                        break;

                case board_black_rook_castle:

                        dirs = data_kingtab[from] & BOARD_ATTACK_ROOK;
                        // Moving a castle-rook can't uncover check.. Ignore pin_dir
                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                assert(len > 0);

                                to = from;
                                do {
                                        to += vector;
                                        if (bd->squares[to].piece != board_empty) {
                                                break;
                                        }
                                        GENERATE_MOVE(from, to, move_black_rook_castle);
                                } while (--len);
                        } while (dirs != 0);

                        if (from == A8) {
                                if ((bd->squares[B8].piece == board_empty) &&
                                    (bd->squares[C8].piece == board_empty) &&
                                    (bd->squares[D8].piece == board_empty) &&
                                    (bd->current->passive.attacks[C8] == 0) &&
                                    (bd->current->passive.attacks[D8] == 0)
                                ) {
                                        GENERATE_KING_MOVE(E8, C8, castle_black_queen_side);
                                }
                        } else {
                                assert(from == H8);

                                if ((bd->squares[F8].piece == board_empty) &&
                                    (bd->squares[G8].piece == board_empty) &&
                                    (bd->current->passive.attacks[F8] == 0) &&
                                    (bd->current->passive.attacks[G8] == 0)
                                ) {
                                        GENERATE_KING_MOVE(E8, G8, castle_black_king_side);
                                }
                        }

                        break;

        /*------------------------------------------------------+
         |      Generate bishop moves                           |
         +------------------------------------------------------*/

                case board_white_bishop_light:
                case board_white_bishop_dark:

                        dirs = data_kingtab[from] & BOARD_ATTACK_BISHOP & ~pin_dirs;
                        if (dirs == 0) {
                                break;
                        }
                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                assert(len > 0);

                                to = from;
                                do {
                                        to += vector;
                                        if (bd->squares[to].piece != board_empty) {
                                                break;
                                        }
                                        GENERATE_MOVE(from, to, move_white_bishop);
                                } while (--len);
                        } while (dirs != 0);

                        break;

                case board_black_bishop_light:
                case board_black_bishop_dark:

                        dirs = data_kingtab[from] & BOARD_ATTACK_BISHOP & ~pin_dirs;
                        if (dirs == 0) {
                                break;
                        }
                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                assert(len > 0);

                                to = from;
                                do {
                                        to += vector;
                                        if (bd->squares[to].piece != board_empty) {
                                                break;
                                        }
                                        GENERATE_MOVE(from, to, move_black_bishop);
                                } while (--len);
                        } while (dirs != 0);

                        break;

        /*------------------------------------------------------+
         |      Generate knight moves                           |
         +------------------------------------------------------*/

                case board_white_knight:
                        dirs = data_knighttab[from];
                        if (pin_dirs != 0) {
                                break;
                        }
                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                to = from + board_vector_jump[dir];

                                if (bd->squares[to].piece == board_empty) {
                                        GENERATE_MOVE(from, to, move_white_knight);
                                }
                        } while (dirs != 0);

                        break;

                case board_black_knight:
                        dirs = data_knighttab[from];
                        if (pin_dirs != 0) {
                                break;
                        }
                        dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                to = from + board_vector_jump[dir];

                                if (bd->squares[to].piece == board_empty) {
                                        GENERATE_MOVE(from, to, move_black_knight);
                                }
                        } while (dirs != 0);

                        break;

        /*------------------------------------------------------+
         |      Pawn moves (except promotions)                  |
         +------------------------------------------------------*/

                case board_white_pawn:
                        to = from + BOARD_VECTOR_NORTH;
                        if ((bd->squares[to].piece == board_empty) &&
                                ((pin_dirs & board_attack_north) == 0))
                        {
                                GENERATE_PAWN_MOVE(from, to,
                                        move_white_pawn,
                                        board_attack_north);
                        }

                        break;

                case board_black_pawn:
                        to = from + BOARD_VECTOR_SOUTH;
                        if ((bd->squares[to].piece == board_empty) &&
                                ((pin_dirs & board_attack_south) == 0))
                        {
                                GENERATE_PAWN_MOVE(from, to,
                                        move_black_pawn,
                                        board_attack_south);
                        }
                        break;

                case board_white_pawn_rank2:
                        to = from + BOARD_VECTOR_NORTH;
                        if ((bd->squares[to].piece == board_empty) &&
                                ((pin_dirs & board_attack_north) == 0))
                        {
                                GENERATE_PAWN_MOVE(from, to,
                                        move_white_pawn_rank2_to_3,
                                        board_attack_north);

                                to += BOARD_VECTOR_NORTH;
                                if (bd->squares[to].piece == board_empty) {
                                        GENERATE_PAWN_MOVE(from, to,
                                                move_white_pawn_rank2_to_4,
                                                board_attack_north);
                                }
                        }
                        break;

                case board_black_pawn_rank7:
                        to = from + BOARD_VECTOR_SOUTH;
                        if ((bd->squares[to].piece == board_empty) &&
                                ((pin_dirs & board_attack_south) == 0))
                        {
                                GENERATE_PAWN_MOVE(from, to,
                                        move_black_pawn_rank7_to_6,
                                        board_attack_south);

                                to += BOARD_VECTOR_SOUTH;
                                if (bd->squares[to].piece == board_empty) {
                                        GENERATE_PAWN_MOVE(from, to,
                                                move_black_pawn_rank7_to_5,
                                                board_attack_south);
                                }
                        }
                        break;

                case board_white_pawn_rank7:
                case board_black_pawn_rank2:
                        /*
                         *  Don't generate promotions here
                         */
                        break;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/

                default:
                       assert(false);
                }
        }

        return nr_moves;
}

/*----------------------------------------------------------------------+
 |      generate_captures_and_promotions                                |
 +----------------------------------------------------------------------*/

/*
 *  Generate captures and promotions
 */
int board_generate_captures_and_promotions(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES])
{
        int nr_moves = 0;

        int to;
        signed char *pieces;

        /*------------------------------------------------------+
         |      Generate regular captures                       |
         +------------------------------------------------------*/

        pieces = &bd->current->passive.pieces[1];

        while ((to = *pieces++) >= 0) {

                int attack = bd->current->active.attacks[to];
                if (attack == 0) {
                        continue;
                }

                /*
                 *  With king
                 */
                if ((attack & board_attack_king) != 0) {

                        if (bd->current->passive.attacks[to] == 0) {
                                int from = bd->current->active.pieces[0];
                                int piece = bd->squares[from].piece;
                                GENERATE_KING_CAPTURE(from, to, capture_fn_table[piece]);
                        }
                }

                /*
                 *  With any piece (except king)
                 */
                nr_moves += generate_captures_to_square(bd, &moves_p[nr_moves], to);
        }

        /*------------------------------------------------------+
         |      Generate promotions                             |
         +------------------------------------------------------*/

        int last_rank_pawns = bd->current->active.last_rank_pawns;
        if (last_rank_pawns != 0) {

                static const char file[8] = {
                        [ DEBRUIJN_INDEX(1<<BOARD_FILE_A) ] = BOARD_FILE_A,
                        [ DEBRUIJN_INDEX(1<<BOARD_FILE_B) ] = BOARD_FILE_B,
                        [ DEBRUIJN_INDEX(1<<BOARD_FILE_C) ] = BOARD_FILE_C,
                        [ DEBRUIJN_INDEX(1<<BOARD_FILE_D) ] = BOARD_FILE_D,
                        [ DEBRUIJN_INDEX(1<<BOARD_FILE_E) ] = BOARD_FILE_E,
                        [ DEBRUIJN_INDEX(1<<BOARD_FILE_F) ] = BOARD_FILE_F,
                        [ DEBRUIJN_INDEX(1<<BOARD_FILE_G) ] = BOARD_FILE_G,
                        [ DEBRUIJN_INDEX(1<<BOARD_FILE_H) ] = BOARD_FILE_H,
                };

                if (bd->current->active.color == board_white) {

                        int bit = 0;
                        do {
                                bit -= last_rank_pawns;
                                bit &= last_rank_pawns;
                                last_rank_pawns -= bit;

                                int from = BOARD_SQUARE(
                                        file[ DEBRUIJN_INDEX(bit) ],
                                        BOARD_RANK_7 );

                                assert(bd->squares[from].piece == board_white_pawn_rank7);

                                /*
                                 *  Forward
                                 */
                                to = from + BOARD_VECTOR_NORTH;
                                if ((bd->squares[to].piece == board_empty) &&
                                     IS_LEGAL(from, to)
                                ) {
                                        GENERATE_WHITE_PROMOTION(from, to);
                                }

                        } while (last_rank_pawns != 0);

                } else {

                        int bit = 0;
                        do {
                                bit -= last_rank_pawns;
                                bit &= last_rank_pawns;
                                last_rank_pawns -= bit;

                                int from = BOARD_SQUARE(
                                        file[ DEBRUIJN_INDEX(bit) ],
                                        BOARD_RANK_2 );

                                assert(bd->squares[from].piece == board_black_pawn_rank2);

                                /*
                                 *  Forward
                                 */
                                to = from + BOARD_VECTOR_SOUTH;
                                if ((bd->squares[to].piece == board_empty) &&
                                     IS_LEGAL(from, to)
                                ) {
                                        GENERATE_BLACK_PROMOTION(from, to);
                                }

                        } while (last_rank_pawns != 0);
                }
        }

        /*------------------------------------------------------+
         |      Generate en-passant capture                     |
         +------------------------------------------------------*/

        if (bd->current->en_passant_lazy != 0) {
                nr_moves += generate_en_passant(bd, &moves_p[nr_moves]);
        }

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/

        return nr_moves;
}

/*----------------------------------------------------------------------+
 |      board_generate_escapes                                          |
 +----------------------------------------------------------------------*/

/*
 *  Generate legal moves out of check
 */
int board_generate_escapes(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES])
{
        int nr_moves = 0;

        int king = bd->current->active.pieces[0];
        int attack = bd->current->passive.attacks[king];
        int from, to, step;
        int dir, dirs;

        /*
         *  For double-check, the only response is moving the king to a safe
         *  square. For single-check, the player can also try to capture the
         *  attacker or block the attack (in case of a sliding piece attack).
         */

        /*
         *  This is a bit-trick to distinguish single-check and double-check:
         *  The expression (a & (a - 1)) tests if there are >1 bits set in a.
         *  Single-check always has 1 attack bit set.
         *  Double-check must have multiple bits set.
         *  Note: Although an attack of 2 knights also sets just
         *  one bit in the attack vector (the knight attack counter becomes
         *  2 (decimal) == 10 (binary), such a double attack is not possible on
         *  the king: Double check always involves at least 1 sliding piece
         *  (Q, R, B) and 1 other piece (Q, R, B, N, P).
         */
        if ((attack & (attack - 1)) == 0) {

        /*------------------------------------------------------+
         |      Find attacker                                   |
         +------------------------------------------------------*/

                /*
                 *  Determine location of the attacker in `to'
                 *
                 *  In case of sliding attack, already generate
                 *  the blocking moves on the go.
                 *  In case of a pawn, already generate en passant
                 *  captures of it if applicible.
                 */

                if ((attack & BOARD_ATTACK_QUEEN) != 0) {

                        step = board_vector_step_compact[DEBRUIJN_INDEX(attack & BOARD_ATTACK_QUEEN)];
                        assert(step != 0);

                        to = king - step;
                        assert(BOARD_SQUARE_IS_VALID(to));
                        while (bd->squares[to].piece == board_empty) {

                                /*
                                 *  Generate moves to this empty square
                                 *  to block the attack on the king
                                 */

                                nr_moves += generate_moves_to_square(
                                        bd,
                                        &moves_p[nr_moves],
                                        to);

                                nr_moves += generate_pawn_push_to(
                                        bd,
                                        &moves_p[nr_moves],
                                        to);

                                to -= step;
                        }

                        /*
                         *  `to' now points to the attacker
                         */

                } else if ((attack & board_attack_pawn_west) != 0) {
                        if (bd->current->active.color == board_white) {
                                // location of black pawn
                                to = king - BOARD_VECTOR_SOUTHWEST;
                        } else {
                                // location of white pawn
                                to = king - BOARD_VECTOR_NORTHWEST;
                        }

                        /*
                         *  Special case: en-passant capture of checking pawn
                         */
                        if (bd->current->en_passant_lazy != 0) {
                                nr_moves += generate_en_passant(
                                        bd,
                                        &moves_p[nr_moves]);
                        }

                        /*
                         *  `to' now points to the attacker
                         */

                } else if ((attack & board_attack_pawn_east) != 0) {
                        if (bd->current->active.color == board_white) {
                                // location of black pawn
                                to = king - BOARD_VECTOR_SOUTHEAST;
                        } else {
                                // location of white pawn
                                to = king - BOARD_VECTOR_NORTHEAST;
                        }

                        /*
                         *  Special case: en-passant capture of checking pawn
                         */
                        if (bd->current->en_passant_lazy != 0) {
                                nr_moves += generate_en_passant(
                                        bd,
                                        &moves_p[nr_moves]);
                        }

                        /*
                         *  `to' now points to the attacker
                         */

                } else {
                        // Must be a knight
                        assert(attack == board_attack_knight);

                        signed char *knights = &bd->current->passive.pieces[1];
                        do {
                                to = *knights++;

                                assert((bd->squares[to].piece ==
                                       board_white_knight) ||
                                       (bd->squares[to].piece ==
                                        board_black_knight));

                        } while (data_sq2sq[king][to] != board_attack_knight);

                        /*
                         *  `to' now points to the attacker
                         */
                }

        /*------------------------------------------------------+
         |      Generate captures of the attacking piece        |
         +------------------------------------------------------*/

                assert(BOARD_SQUARE_IS_VALID(to));
                assert(BOARD_PIECE_COLOR(bd->squares[to].piece) == !bd->current->active.color);

                /*
                 *  Attacker identified. Generate captures.
                 */

                if (bd->current->active.attacks[to] != 0) {
                        nr_moves += generate_captures_to_square(
                                bd,
                                &moves_p[nr_moves],
                                to);
                }
        }

        /*------------------------------------------------------+
         |      Generate king moves or captures out of check    |
         +------------------------------------------------------*/

        from = king;

        dirs = data_kingtab[from] & ~bd->current->passive.attacks[from];
        assert(dirs != 0);
        dir = 0;
        do {
                dir -= dirs;
                dir &= dirs;
                dirs -= dir;

                to = from + board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                assert(to != from);
                if ((BOARD_PIECE_COLOR(bd->squares[to].piece) != bd->current->active.color) && // @TODO: color lookup (slow?)
                        (bd->current->passive.attacks[to] == 0))
                {
                        // by construction it should be a legal move
                        if (bd->squares[to].piece == board_empty) {
                                GENERATE_KING_MOVE(from, to, move_fn_table[bd->squares[from].piece]);
                        } else {
                                GENERATE_KING_CAPTURE(from, to, capture_fn_table[bd->squares[from].piece]);
                        }
                }
        } while (dirs != 0);

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/

        return nr_moves;
}

/*----------------------------------------------------------------------+
 |      board_generate_regular_checks                                   |
 +----------------------------------------------------------------------*/

/*
 *  Generate regular checks
 */
int board_generate_regular_checks(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES])
{
        int nr_moves = 0;

        assert(!board_in_check(bd));

        int king = bd->current->active.pieces[0];
        int xking = bd->current->passive.pieces[0];
        int from, to;
        int dir, dirs;

        signed char *pieces = &bd->current->active.pieces[0];

        while ((from = *pieces++) >= 0) {

                int piece = bd->squares[from].piece;

                /*
                 *  Pre-check for any pins to the king
                 */

                int pin_dirs =
                        bd->current->passive.attacks[from] &
                        data_sq2sq[from][king] &
                        BOARD_ATTACK_QUEEN;

                if (pin_dirs != 0) {
                        assert((pin_dirs & (pin_dirs-1)) == 0);

                        if (the_path_is_clear(bd, from, king)) {
                                // only pin_dirs and reversed are safe
                                pin_dirs |= BOARD_ATTACK_REVERSE(pin_dirs);

                                // all others are unsafe
                                pin_dirs = ~pin_dirs;
                        } else {
                                // all directions are safe
                                pin_dirs = 0;
                        }
                }

                switch (piece) {

        /*------------------------------------------------------+
         |      Generate king checks (and pawn pushes)          |
         +------------------------------------------------------*/

                case board_white_king_castle:
                case board_white_king:

                        if (uncovers_check(bd, from, xking)) {

                                dirs = data_kingtab[from];
                                dirs &= ~data_sq2sq[from][xking];
                                dirs &= ~data_sq2sq[xking][from];
                                assert(dirs != 0);

                                dir = 0;
                                do {
                                        dir -= dirs;
                                        dir &= dirs;
                                        dirs -= dir;

                                        to = from + board_vector_step_compact[DEBRUIJN_INDEX(dir)];

                                        if ((bd->current->passive.attacks[to] == 0) && /* don't walk into check */
                                            (bd->squares[to].piece == board_empty)
                                        ) {
                                                GENERATE_KING_MOVE(from, to, move_fn_table[piece]);
                                        }
                                } while (dirs != 0);
                        }

                        /*
                         *  Abuse the fact that we know the side to move now to
                         *  look for direct pawn checks as well. This might be a
                         *  bit faster than trying this from the pawn point of view.
                         */

                        /*
                         *  Single pawn pushes
                         */

                        if (BOARD_RANK(xking) != BOARD_RANK_1) {
                                int attacks = bd->current->active.attacks[xking - BOARD_VECTOR_NORTH];
                                if ((attacks & board_attack_pawn_west) != 0) {
                                        to = xking - BOARD_VECTOR_NORTHWEST;
                                        from = to - BOARD_VECTOR_NORTH;
                                        if (bd->squares[to].piece == board_empty) {
                                                if (((bd->current->passive.attacks[from] &
                                                      data_sq2sq[from][king] &
                                                      BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) == 0) ||
                                                    !the_path_is_clear(bd, from, king)
                                                ) {
                                                        if (bd->squares[from].piece == board_white_pawn) {
                                                                GENERATE_PAWN_MOVE(from, to,
                                                                        move_white_pawn,
                                                                        board_attack_north);
                                                        } else {
                                                                GENERATE_PAWN_MOVE(from, to,
                                                                        move_white_pawn_rank2_to_3,
                                                                        board_attack_north);
                                                        }
                                                }
                                        }
                                }
                                if ((attacks & board_attack_pawn_east) != 0) {
                                        to = xking - BOARD_VECTOR_NORTHEAST;
                                        from = to - BOARD_VECTOR_NORTH;
                                        if (bd->squares[to].piece == board_empty) {
                                                if (((bd->current->passive.attacks[from] &
                                                      data_sq2sq[from][king] &
                                                      BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) == 0) ||
                                                    !the_path_is_clear(bd, from, king)
                                                ) {
                                                        if (bd->squares[from].piece == board_white_pawn) {
                                                                GENERATE_PAWN_MOVE(from, to,
                                                                        move_white_pawn,
                                                                        board_attack_north);
                                                        } else {
                                                                assert(bd->squares[from].piece == board_white_pawn_rank2);
                                                                GENERATE_PAWN_MOVE(from, to,
                                                                        move_white_pawn_rank2_to_3,
                                                                        board_attack_north);
                                                        }
                                                }
                                        }
                                }
                        }

                        /*
                         *  Double pawn pushes
                         */

                        if (BOARD_RANK(xking) == BOARD_RANK_5) {
                                int attacks = bd->current->active.attacks[xking - 2 * BOARD_VECTOR_NORTH];
                                if ((attacks & board_attack_pawn_west) != 0) {
                                        to = xking - BOARD_VECTOR_NORTHWEST;
                                        int mid = to - BOARD_VECTOR_NORTH;
                                        from = to - 2 * BOARD_VECTOR_NORTH;
                                        if ((bd->squares[to].piece == board_empty) &&
                                            (bd->squares[mid].piece == board_empty)
                                        ) {
                                                if (((bd->current->passive.attacks[from] &
                                                      data_sq2sq[from][king] &
                                                      BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) == 0) ||
                                                    !the_path_is_clear(bd, from, king)
                                                ) {
                                                        GENERATE_PAWN_MOVE(from, to,
                                                                move_white_pawn_rank2_to_4,
                                                                board_attack_north);
                                                }
                                        }
                                }
                                if ((attacks & board_attack_pawn_east) != 0) {
                                        to = xking - BOARD_VECTOR_NORTHEAST;
                                        int mid = to - BOARD_VECTOR_NORTH;
                                        from = to - 2 * BOARD_VECTOR_NORTH;
                                        if ((bd->squares[to].piece == board_empty) &&
                                            (bd->squares[mid].piece == board_empty)
                                        ) {
                                                if (((bd->current->passive.attacks[from] &
                                                      data_sq2sq[from][king] &
                                                      BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) == 0) ||
                                                    !the_path_is_clear(bd, from, king)
                                                ) {
                                                        GENERATE_PAWN_MOVE(from, to,
                                                                move_white_pawn_rank2_to_4,
                                                                board_attack_north);
                                                }
                                        }
                                }
                        }

                        break;

                case board_black_king_castle:
                case board_black_king:

                        if (uncovers_check(bd, from, xking)) {

                                dirs = data_kingtab[from];
                                dirs &= ~data_sq2sq[from][xking];
                                dirs &= ~data_sq2sq[xking][from];
                                assert(dirs != 0);

                                dir = 0;
                                do {
                                        dir -= dirs;
                                        dir &= dirs;
                                        dirs -= dir;

                                        to = from + board_vector_step_compact[DEBRUIJN_INDEX(dir)];

                                        if ((bd->current->passive.attacks[to] == 0) && /* don't walk into check */
                                            (bd->squares[to].piece == board_empty)
                                        ) {
                                                GENERATE_KING_MOVE(from, to, move_fn_table[piece]);
                                        }
                                } while (dirs != 0);
                        }

                        /*
                         *  Abuse the fact that we know the side to move now to
                         *  look for direct pawn checks as well. This might be a
                         *  bit faster than trying this from the pawn point of view.
                         */

                        /*
                         *  Single pawn pushes
                         */

                        if (BOARD_RANK(xking) != BOARD_RANK_8) {
                                int attacks = bd->current->active.attacks[xking - BOARD_VECTOR_SOUTH];
                                if ((attacks & board_attack_pawn_west) != 0) {
                                        to = xking - BOARD_VECTOR_SOUTHWEST;
                                        from = to - BOARD_VECTOR_SOUTH;
                                        if (bd->squares[to].piece == board_empty) {
                                                if (((bd->current->passive.attacks[from] &
                                                      data_sq2sq[from][king] &
                                                      BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) == 0) ||
                                                    !the_path_is_clear(bd, from, king)
                                                ) {
                                                        if (bd->squares[from].piece == board_black_pawn) {
                                                                GENERATE_PAWN_MOVE(from, to,
                                                                        move_black_pawn,
                                                                        board_attack_south);
                                                        } else {
                                                                GENERATE_PAWN_MOVE(from, to,
                                                                        move_black_pawn_rank7_to_6,
                                                                        board_attack_south);
                                                        }
                                                }
                                        }
                                }
                                if ((attacks & board_attack_pawn_east) != 0) {
                                        to = xking - BOARD_VECTOR_SOUTHEAST;
                                        from = to - BOARD_VECTOR_SOUTH;
                                        if (bd->squares[to].piece == board_empty) {

                                                if (((bd->current->passive.attacks[from] &
                                                      data_sq2sq[from][king] &
                                                      BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) == 0) ||
                                                    !the_path_is_clear(bd, from, king)
                                                ) {
                                                        if (bd->squares[from].piece == board_black_pawn) {
                                                                GENERATE_PAWN_MOVE(from, to,
                                                                        move_black_pawn,
                                                                        board_attack_south);
                                                        } else {
                                                                GENERATE_PAWN_MOVE(from, to,
                                                                        move_black_pawn_rank7_to_6,
                                                                        board_attack_south);
                                                        }
                                                }
                                        }
                                }
                        }

                        /*
                         *  Double pawn pushes
                         */

                        if (BOARD_RANK(xking) == BOARD_RANK_4) {
                                int attacks = bd->current->active.attacks[xking - 2 * BOARD_VECTOR_SOUTH];
                                if ((attacks & board_attack_pawn_west) != 0) {
                                        to = xking - BOARD_VECTOR_SOUTHWEST;
                                        int mid = to - BOARD_VECTOR_SOUTH;
                                        from = to - 2 * BOARD_VECTOR_SOUTH;
                                        if ((bd->squares[to].piece == board_empty) &&
                                            (bd->squares[mid].piece == board_empty)
                                        ) {
                                                if (((bd->current->passive.attacks[from] &
                                                      data_sq2sq[from][king] &
                                                      BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) == 0) ||
                                                    !the_path_is_clear(bd, from, king)
                                                ) {
                                                        GENERATE_PAWN_MOVE(from, to,
                                                                move_black_pawn_rank7_to_5,
                                                                board_attack_south);
                                                }
                                        }
                                }
                                if ((attacks & board_attack_pawn_east) != 0) {
                                        to = xking - BOARD_VECTOR_SOUTHEAST;
                                        int mid = to - BOARD_VECTOR_SOUTH;
                                        from = to - 2 * BOARD_VECTOR_SOUTH;
                                        if ((bd->squares[to].piece == board_empty) &&
                                            (bd->squares[mid].piece == board_empty)
                                        ) {
                                                if (((bd->current->passive.attacks[from] &
                                                      data_sq2sq[from][king] &
                                                      BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) == 0) ||
                                                    !the_path_is_clear(bd, from, king)
                                                ) {
                                                        GENERATE_PAWN_MOVE(from, to,
                                                                move_black_pawn_rank7_to_5,
                                                                board_attack_south);
                                                }
                                        }
                                }
                        }
                        break;

        /*------------------------------------------------------+
         |      Generate knight checks                          |
         +------------------------------------------------------*/

                case board_white_knight:
                case board_black_knight:

                        if (pin_dirs != 0) {
                                break;
                        }
                        if (uncovers_check(bd, from, xking)) {
                                dirs = data_knighttab[from];

                                dir = 0;
                                while (dirs != 0)  {
                                        dir -= dirs;
                                        dir &= dirs;
                                        dirs -= dir;

                                        to = from + board_vector_jump[dir];

                                        if (bd->squares[to].piece == board_empty) {
                                                GENERATE_DISCOVERED_CHECK(from, to, move_fn_table[piece]);
                                        }
                                }
                        } else {
                                dirs = data_knight_checks[xking][from];

                                dir = 0;
                                while (dirs != 0)  {
                                        dir -= dirs;
                                        dir &= dirs;
                                        dirs -= dir;

                                        to = from + board_vector_jump[dir];

                                        if (bd->squares[to].piece == board_empty) {
                                                GENERATE_MOVE(from, to, move_fn_table[piece]);
                                        }
                                }
                        }
                        break;

        /*------------------------------------------------------+
         |      Generate pawn checks (only discovered checks)   |
         +------------------------------------------------------*/

                case board_white_pawn:

                        to = from + BOARD_VECTOR_NORTH;

                        if (((bd->current->active.attacks[from] & data_sq2sq[from][xking] &
                              BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) != 0) &&
                            the_path_is_clear(bd, from, xking) &&
                            (bd->squares[to].piece == board_empty) &&
                            ((pin_dirs & board_attack_north) == 0)
                        ) {
                                GENERATE_PAWN_MOVE(from, to, move_white_pawn, board_attack_north);
                        }
                        break;

                case board_white_pawn_rank2:

                        to = from + BOARD_VECTOR_NORTH;

                        if (((bd->current->active.attacks[from] & data_sq2sq[from][xking] &
                              BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) != 0) &&
                            the_path_is_clear(bd, from, xking) &&
                            (bd->squares[to].piece == board_empty) &&
                            ((pin_dirs & board_attack_north) == 0)
                        ) {
                                GENERATE_PAWN_MOVE(from, to, move_white_pawn_rank2_to_3, board_attack_north);

                                to += BOARD_VECTOR_NORTH;
                                if (bd->squares[to].piece == board_empty) {
                                        GENERATE_PAWN_MOVE(from, to, move_white_pawn_rank2_to_4, board_attack_north);
                                }
                        }
                        break;

                case board_black_pawn:

                        to = from + BOARD_VECTOR_SOUTH;

                        if (((bd->current->active.attacks[from] & data_sq2sq[from][xking] &
                              BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) != 0) &&
                            the_path_is_clear(bd, from, xking) &&
                            (bd->squares[to].piece == board_empty) &&
                            ((pin_dirs & board_attack_south) == 0)
                        ) {
                                GENERATE_PAWN_MOVE(from, to, move_black_pawn, board_attack_south);
                        }
                        break;

                case board_black_pawn_rank7:

                        to = from + BOARD_VECTOR_SOUTH;

                        if (((bd->current->active.attacks[from] & data_sq2sq[from][xking] &
                              BOARD_ATTACK_QUEEN & ~BOARD_ATTACK_VERTICAL) != 0) &&
                            the_path_is_clear(bd, from, xking) &&
                            (bd->squares[to].piece == board_empty) &&
                            ((pin_dirs & board_attack_south) == 0)
                        ) {
                                GENERATE_PAWN_MOVE(from, to, move_black_pawn_rank7_to_6, board_attack_south);

                                to += BOARD_VECTOR_SOUTH;
                                if (bd->squares[to].piece == board_empty) {
                                        GENERATE_PAWN_MOVE(from, to, move_black_pawn_rank7_to_5, board_attack_south);
                                }
                        }
                        break;

                case board_white_pawn_rank7:
                case board_black_pawn_rank2:

                        /*
                         *  All of these are generated in board_generate_captures_and_promotions()
                         */
                        break;

        /*------------------------------------------------------+
         |      Generate bishop checks                          |
         +------------------------------------------------------*/

                case board_white_bishop_light:
                case board_black_bishop_light:
                case board_white_bishop_dark:
                case board_black_bishop_dark:

                        if (uncovers_check(bd, from, xking)) {

                                dirs = data_kingtab[from] & BOARD_ATTACK_BISHOP;
                                dirs &= ~data_sq2sq[from][xking];
                                dirs &= ~data_sq2sq[xking][from];
                                dirs &= ~pin_dirs;

                                dir = 0;
                                while (dirs != 0) {
                                        dir -= dirs;
                                        dir &= dirs;
                                        dirs -= dir;

                                        int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                        int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                        assert(len > 0);

                                        to = from;
                                        do {
                                                to += vector;
                                                if (bd->squares[to].piece != board_empty) {
                                                        break;
                                                }
                                                GENERATE_DISCOVERED_CHECK(from, to, move_fn_table[piece]);
                                        } while (--len);
                                }

                        } else {
                                int xking_dia1 = BOARD_FILE(xking) + BOARD_RANK(xking);
                                int xking_dia2 = BOARD_FILE(xking) - BOARD_RANK(xking);
                                int from_dia1 = BOARD_FILE(from) + BOARD_RANK(from);
                                int from_dia2 = BOARD_FILE(from) - BOARD_RANK(from);

                                if ((xking_dia1 != from_dia1) &&
                                    (xking_dia2 != from_dia2) && /* Not on the same line already */
                                    (((xking_dia1 - from_dia1) & 1) == 0) /* On the same color squares */
                                ) {
                                        int to_file = BOARD_FILE(from) + (xking_dia1 - from_dia1)/2;
                                        int to_rank = BOARD_RANK(from) + (xking_dia1 - from_dia1)/2;

                                        if (((to_file & 7) == to_file) && ((to_rank & 7) == to_rank) && /* still on board */
                                            ((pin_dirs & (board_attack_northeast|board_attack_southwest)) == 0) /* not pinned */
                                        ) {

                                                to = BOARD_SQUARE(to_file, to_rank);
                                                dir = data_sq2sq[from][to] & BOARD_ATTACK_QUEEN;
                                                assert((dir & BOARD_ATTACK_BISHOP) == dir);

                                                if (((bd->current->active.attacks[to] & dir) != 0) &&
                                                    (bd->squares[to].piece == board_empty) &&
                                                    the_path_is_clear(bd, xking, to) &&
                                                    the_path_is_clear(bd, from, to)
                                                ) {
                                                        GENERATE_MOVE(from, to, move_fn_table[piece]);
                                                }
                                        }

                                        to_file = BOARD_FILE(from) + (xking_dia2 - from_dia2)/2;
                                        to_rank = BOARD_RANK(from) - (xking_dia2 - from_dia2)/2;

                                        if (((to_file & 7) == to_file) && ((to_rank & 7) == to_rank) && /* still on board */
                                            ((pin_dirs & (board_attack_northwest|board_attack_southeast)) == 0) /* not pinned */
                                        ) {
                                                to = BOARD_SQUARE(to_file, to_rank);
                                                dir = data_sq2sq[from][to] & BOARD_ATTACK_QUEEN;
                                                assert((dir & BOARD_ATTACK_BISHOP) == dir);

                                                if (((bd->current->active.attacks[to] & dir) != 0) &&
                                                    (bd->squares[to].piece == board_empty) &&
                                                    the_path_is_clear(bd, xking, to) &&
                                                    the_path_is_clear(bd, from, to)
                                                ) {
                                                        GENERATE_MOVE(from, to, move_fn_table[piece]);
                                                }
                                        }
                                }
                        }

                        break;

        /*------------------------------------------------------+
         |      Generate rook checks                            |
         +------------------------------------------------------*/

                case board_white_rook_castle:
                case board_black_rook_castle:

                        switch (from) {
                        case A1:
                                if (((BOARD_RANK(xking) == BOARD_RANK_1) && the_path_is_clear(bd, E1, xking)) ||
                                    ((BOARD_FILE(xking) == BOARD_FILE_D) && the_path_is_clear(bd, D1, xking))
                                ) {
                                        if ((bd->squares[B1].piece == board_empty) &&
                                            (bd->squares[C1].piece == board_empty) &&
                                            (bd->squares[D1].piece == board_empty) &&
                                            (bd->current->passive.attacks[C1] == 0) &&
                                            (bd->current->passive.attacks[D1] == 0)
                                        ) {
                                                GENERATE_KING_MOVE(E1, C1, castle_white_queen_side);
                                        }
                                }
                                break;
                        case H1:
                                if (((BOARD_RANK(xking) == BOARD_RANK_1) && the_path_is_clear(bd, E1, xking)) ||
                                    ((BOARD_FILE(xking) == BOARD_FILE_F) && the_path_is_clear(bd, F1, xking))
                                ) {
                                        if ((bd->squares[F1].piece == board_empty) &&
                                            (bd->squares[G1].piece == board_empty) &&
                                            (bd->current->passive.attacks[F1] == 0) &&
                                            (bd->current->passive.attacks[G1] == 0)
                                        ) {
                                                GENERATE_KING_MOVE(E1, G1, castle_white_king_side);
                                        }
                                }
                                break;
                        case A8:
                                if (((BOARD_RANK(xking) == BOARD_RANK_8) && the_path_is_clear(bd, E8, xking)) ||
                                    ((BOARD_FILE(xking) == BOARD_FILE_D) && the_path_is_clear(bd, D8, xking))
                                ) {
                                        if ((bd->squares[B8].piece == board_empty) &&
                                            (bd->squares[C8].piece == board_empty) &&
                                            (bd->squares[D8].piece == board_empty) &&
                                            (bd->current->passive.attacks[C8] == 0) &&
                                            (bd->current->passive.attacks[D8] == 0)
                                        ) {
                                                GENERATE_KING_MOVE(E8, C8, castle_black_queen_side);
                                        }
                                }
                                break;
                        case H8:
                                if (((BOARD_RANK(xking) == BOARD_RANK_8) && the_path_is_clear(bd, E8, xking)) ||
                                    ((BOARD_FILE(xking) == BOARD_FILE_F) && the_path_is_clear(bd, F8, xking))
                                ) {
                                        if ((bd->squares[F8].piece == board_empty) &&
                                            (bd->squares[G8].piece == board_empty) &&
                                            (bd->current->passive.attacks[F8] == 0) &&
                                            (bd->current->passive.attacks[G8] == 0)
                                        ) {
                                                GENERATE_KING_MOVE(E8, G8, castle_black_king_side);
                                        }
                                }
                                break;
                        default:
                                assert(false);
                        }

                        /* !!! FALL THROUGH !!! */

                case board_white_rook:
                case board_black_rook:

                        if (uncovers_check(bd, from, xking)) {

                                dirs = data_kingtab[from] & BOARD_ATTACK_ROOK;
                                dirs &= ~data_sq2sq[from][xking];
                                dirs &= ~data_sq2sq[xking][from];
                                dirs &= ~pin_dirs;

                                dir = 0;
                                while (dirs != 0) {
                                        dir -= dirs;
                                        dir &= dirs;
                                        dirs -= dir;

                                        int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                        int len = data_raylen[from][DEBRUIJN_INDEX(dir)];
                                        assert(len > 0);

                                        to = from;
                                        do {
                                                to += vector;
                                                if (bd->squares[to].piece != board_empty) {
                                                        break;
                                                }
                                                GENERATE_DISCOVERED_CHECK(from, to, move_fn_table[piece]);
                                        } while (--len);
                                }
                        } else {
                                if ((data_sq2sq[xking][from] & BOARD_ATTACK_ROOK) == 0) { /* Not on the same line already */
                                        if ((pin_dirs & BOARD_ATTACK_HORIZONTAL) == 0) {

                                                /* Move along rank */
                                                to = BOARD_SQUARE(BOARD_FILE(xking), BOARD_RANK(from));
                                                dir = data_sq2sq[from][to] & BOARD_ATTACK_QUEEN;

                                                if (((bd->current->active.attacks[to] & dir) != 0) &&
                                                    (bd->squares[to].piece == board_empty) &&
                                                    the_path_is_clear(bd, xking, to) &&
                                                    the_path_is_clear(bd, from, to)
                                                ) {
                                                        GENERATE_MOVE(from, to, move_fn_table[piece]);
                                                }

                                        }

                                        if ((pin_dirs & BOARD_ATTACK_VERTICAL) == 0) {

                                                /* Move along file */
                                                to = BOARD_SQUARE(BOARD_FILE(from), BOARD_RANK(xking));
                                                dir = data_sq2sq[from][to] & BOARD_ATTACK_QUEEN;

                                                if (((bd->current->active.attacks[to] & dir) != 0) &&
                                                    (bd->squares[to].piece == board_empty) &&
                                                    the_path_is_clear(bd, xking, to) &&
                                                    the_path_is_clear(bd, from, to)
                                                ) {
                                                        GENERATE_MOVE(from, to, move_fn_table[piece]);
                                                }
                                        }
                                }
                        }

                        break;

        /*------------------------------------------------------+
         |      Generate queen checks                           |
         +------------------------------------------------------*/

                case board_white_queen:
                case board_black_queen:

                        dirs = data_kingtab[xking];
                        dir = 0;
#if 0
                        int dist[BOARD_MAX_MOVES];
                        int mindist = BOARD_SIZE; // inf
                        int queen_moves = nr_moves;
#endif
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                                assert(vector != 0);

                                int len = data_raylen[xking][DEBRUIJN_INDEX(dir)];
                                assert(len > 0);
                                to = xking;

#if 0
                                int found = false; // only give a good SEE to the first on each ray
#endif

                                do {
                                        /* Step away from the opponent king */
                                        to += vector;
                                        if (bd->squares[to].piece != board_empty) {
                                                break;
                                        }

                                        /* A sliding attack might be a queen */
                                        int attacks = ~pin_dirs & bd->current->active.attacks[to] & BOARD_ATTACK_QUEEN;
                                        if (attacks == 0) {
                                                continue;
                                        }

                                        /* Especially if the move direction matches */
                                        int move_dir = data_sq2sq[from][to] & attacks;
                                        if (move_dir == 0) {
                                                continue;
                                        }

                                        /* Walk towards the attacker to see where it is */
                                        int step = board_vector_step_compact[DEBRUIJN_INDEX(move_dir)];
                                        assert(step != 0);
                                        int sq = to;
                                        do {
                                                sq -= step;
                                        } while (bd->squares[sq].piece == board_empty);

                                        /* If it is the queen generate the move */
                                        if (sq == from) {
                                                /*
                                                 *  Generate queen check
                                                 */
                                                GENERATE_MOVE_SLOW(from, to, move_fn_table[piece]);
#if 0
                                                if (moves_p[nr_moves-1].bm.prescore >= EXCHANGE_NEUTRAL) {
                                                        if (found) {
                                                                /* Demote queen checks further along the line */
                                                                moves_p[nr_moves-1].bm.prescore = EXCHANGE_NEUTRAL - EXCHANGE_UNIT;
                                                        } else {
                                                                /* Keep nearest good check on each ray from the king */
                                                                found = true;
                                                        }
                                                }
#endif
#if 0
                                                /*
                                                 *  Calculate distance to king
                                                 */
                                                if (moves_p[nr_moves-1].bm.prescore >= EXCHANGE_NEUTRAL) {
                                                        dist[nr_moves-1] = abs(BOARD_RANK(to) - BOARD_RANK(xking)) + abs(BOARD_FILE(to) - BOARD_FILE(xking));
                                                        mindist = MIN(mindist, dist[nr_moves-1]);
                                                } else {
                                                        dist[nr_moves-1] = BOARD_SIZE;
                                                }
#endif
                                        }
                                } while (--len);

                        } while (dirs != 0);

#if 0
                        /*
                         *  Discredit the queen checks that don't chase the king
                         */
                        for (int i=queen_moves; i<nr_moves; i++) {
                                assert((0 < dist[i]) && (dist[i] <= BOARD_SIZE));
                                if ((mindist < dist[i]) && (dist[i] < BOARD_SIZE)) {
                                        moves_p[i].bm.prescore = EXCHANGE_NEUTRAL - EXCHANGE_UNIT;
                                }
                        }
#endif

                        break;

                default:
                       assert(false);
                }
        }

        /*
         *  Bump up SEE for safe checks
         */

        for (int i=0; i<nr_moves; i++) {
                if (moves_p[i].bm.prescore >= EXCHANGE_NEUTRAL) {
                        moves_p[i].bm.prescore += EXCHANGE_GOOD_MOVE_OFFSET;
                        assert((moves_p[i].bm.prescore & 0xf000) == 0xf000);
                }
        }

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/

        return nr_moves;
}

static inline
bool uncovers_check(struct board *bd, int from, int xking)
{
        int dir;

        /* test for a sliding piece hiding behind and facing the opponent king */
        dir = bd->current->active.attacks[from] & data_sq2sq[from][xking] & BOARD_ATTACK_QUEEN;
        if (dir == 0) {
                return false;
        }

        /* test if the path from king to xking is clear */
        int step = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
        int to = from + step;
        while (bd->squares[to].piece == board_empty) { // we will reach either xking or a blocker
                to += step;
        }

        return to == xking;
}

static inline
bool the_path_is_clear(struct board *bd, int a, int b)
{
        int step = board_vector_step_compact[DEBRUIJN_INDEX(data_sq2sq[a][b] & BOARD_ATTACK_QUEEN)];
        assert(step != 0);

        int sq = a + step;
        while (sq != b) {
                assert(BOARD_SQUARE_IS_VALID(sq));
                if (bd->squares[sq].piece != board_empty) {
                        return false;
                }
                sq += step;
        }
        return true;
}

/*----------------------------------------------------------------------+
 |      generate_moves_to_square                                        |
 +----------------------------------------------------------------------*/

/*
 *  Generate any move to the destination square, except
 *  pawn pushes and king moves. The destination square must be empty.
 */
static
int generate_moves_to_square(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES],
        int to)
{
        int nr_moves = 0;

        int from;
        int piece;

        int attack = bd->current->active.attacks[to];

        assert(bd->squares[to].piece == board_empty);

        /*
         *  With knight
         */
        if (attack >= board_attack_knight) {
                signed char *knights = &bd->current->active.pieces[1];

                do {
                        attack -= board_attack_knight;

                        do {
                                from = *knights++;

                                assert((bd->squares[from].piece == board_white_knight) ||
                                        (bd->squares[from].piece == board_black_knight));

                        } while (data_sq2sq[to][from] != board_attack_knight);

                        assert(BOARD_PIECE_COLOR(bd->squares[from].piece) ==
                                bd->current->active.color);
                        if (IS_LEGAL(from, to)) {
                                GENERATE_MOVE(from, to, move_fn_table[bd->squares[from].piece]);
                        }
                } while (attack >= board_attack_knight);
        }

        /*
         *  With sliding piece (Q, R, B)
         */
        int dirs = attack & BOARD_ATTACK_QUEEN;
        if (dirs != 0) {
                int dir = 0;
                do {
                        dir -= dirs;
                        dir &= dirs;
                        dirs -= dir;

                        int step = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                        assert(step != 0);

                        from = to;
                        do {
                                from -= step;
                                assert(BOARD_SQUARE_IS_VALID(from));
                                piece = bd->squares[from].piece;
                        } while (piece == board_empty);
                        assert(BOARD_PIECE_COLOR(piece) == bd->current->active.color);

                        if (IS_LEGAL(from, to)) {
                                GENERATE_MOVE(from, to, move_fn_table[piece]);
                        }
                } while (dirs != 0);
        }

        return nr_moves;
}

/*----------------------------------------------------------------------+
 |      generate_captures_to_square                                     |
 +----------------------------------------------------------------------*/

/*
 *  Generate any move to the destination square, except
 *  pawn pushes and king moves (but including pawn captures).
 */
static
int generate_captures_to_square(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES],
        int to)
{
        int nr_moves = 0;

        int attackers = 0;
        int piece;
        int bits = bd->current->active.attacks[to];

        /*
         *  Two-pass generation of captures
         *  Pass 1: put captures first in a local list
         *          (but promotions go directly into the global list)
         *  Pass 2: evaluate captures and copy to global list
         */
        struct {
                int from;
                make_move_fn *make;
        } captures[BOARD_SIDE_MAX_PIECES];
        int nr_captures = 0;

        /*
         *  Pawn capture west
         */
        if ((bits & board_attack_pawn_west) != 0) {

                attackers += EXCHANGE_LIST_PAWN;

                if (bd->current->active.color == board_white) {
                        int from = to - BOARD_VECTOR_NORTHWEST;
                        piece = bd->squares[from].piece;
                        assert(BOARD_PIECE_COLOR(piece) == board_white);
                        if (IS_LEGAL(from, to)) {
                                if (BOARD_RANK(to) != BOARD_RANK_8) {
                                        assert(nr_captures < BOARD_SIDE_MAX_PIECES);
                                        captures[nr_captures].from = from;
                                        captures[nr_captures].make = capture_with_white_pawn;
                                        nr_captures++;

                                        /* Find extra defenders behind the pawn */
                                        bd->extra_defenders[from] = 0;
                                        int extra_bit =
                                                bd->current->passive.attacks[from] &
                                                board_attack_northwest;
                                        if (extra_bit != 0) {
                                                bd->extra_defenders[from] =
                                                        exchange_collect_extra_defenders(
                                                                bd, from, extra_bit);
                                        }
                                } else {
                                        GENERATE_WHITE_PROMOTION(from, to);
                                }
                        }

                        /* Find attackers behind the pawn */
                        while ((bd->current->active.attacks[from] & board_attack_northwest) != 0) {
                                do {
                                        from -= BOARD_VECTOR_NORTHWEST;
                                        assert(BOARD_SQUARE_IS_VALID(from));
                                        piece = bd->squares[from].piece;
                                } while (piece == board_empty);
                                assert(BOARD_PIECE_COLOR(piece) == bd->current->active.color);
                                attackers += exchange_piece_to_list[piece];
                        }
                } else {
                        int from = to - BOARD_VECTOR_SOUTHWEST;
                        piece = bd->squares[from].piece;
                        assert(BOARD_PIECE_COLOR(piece) == board_black);
                        if (IS_LEGAL(from, to)) {
                                if (BOARD_RANK(to) != BOARD_RANK_1) {
                                        assert(nr_captures < BOARD_SIDE_MAX_PIECES);
                                        captures[nr_captures].from = from;
                                        captures[nr_captures].make = capture_with_black_pawn;
                                        nr_captures++;

                                        /* Find extra defenders behind the pawn */
                                        bd->extra_defenders[from] = 0;
                                        int extra_bit =
                                                bd->current->passive.attacks[from] &
                                                board_attack_southwest;
                                        if (extra_bit != 0) {
                                                bd->extra_defenders[from] =
                                                        exchange_collect_extra_defenders(
                                                                bd, from, extra_bit);
                                        }
                                } else {
                                        GENERATE_BLACK_PROMOTION(from, to);
                                }
                        }

                        /* Find attackers behind the pawn */
                        while ((bd->current->active.attacks[from] & board_attack_southwest) != 0) {
                                do {
                                        from -= BOARD_VECTOR_SOUTHWEST;
                                        assert(BOARD_SQUARE_IS_VALID(from));
                                        piece = bd->squares[from].piece;
                                } while (piece == board_empty);
                                assert(BOARD_PIECE_COLOR(piece) == bd->current->active.color);
                                attackers += exchange_piece_to_list[piece];
                        }
                }
        }

        /*
         *  Pawn capture east
         */
        if ((bits & board_attack_pawn_east) != 0) {

                attackers += EXCHANGE_LIST_PAWN;

                if (bd->current->active.color == board_white) {
                        int from = to - BOARD_VECTOR_NORTHEAST;
                        piece = bd->squares[from].piece;
                        assert(BOARD_PIECE_COLOR(piece) == board_white);
                        if (IS_LEGAL(from, to)) {
                                if (BOARD_RANK(to) != BOARD_RANK_8) {
                                        assert(nr_captures < BOARD_SIDE_MAX_PIECES);
                                        captures[nr_captures].from = from;
                                        captures[nr_captures].make = capture_with_white_pawn;
                                        nr_captures++;

                                        /* Find extra defenders behind the pawn */
                                        bd->extra_defenders[from] = 0;
                                        int extra_bit =
                                                bd->current->passive.attacks[from] &
                                                board_attack_northeast;
                                        if (extra_bit != 0) {
                                                bd->extra_defenders[from] =
                                                        exchange_collect_extra_defenders(
                                                                bd, from, extra_bit);
                                        }
                                } else {
                                        GENERATE_WHITE_PROMOTION(from, to);
                                }
                        }

                        /* Find attackers behind the pawn */
                        while ((bd->current->active.attacks[from] & board_attack_northeast) != 0) {
                                do {
                                        from -= BOARD_VECTOR_NORTHEAST;
                                        assert(BOARD_SQUARE_IS_VALID(from));
                                        piece = bd->squares[from].piece;
                                } while (piece == board_empty);
                                assert(BOARD_PIECE_COLOR(piece) == bd->current->active.color);
                                attackers += exchange_piece_to_list[piece];
                        }
                } else {
                        int from = to - BOARD_VECTOR_SOUTHEAST;
                        piece = bd->squares[from].piece;
                        assert(BOARD_PIECE_COLOR(piece) == board_black);
                        if (IS_LEGAL(from, to)) {
                                if (BOARD_RANK(to) != BOARD_RANK_1) {
                                        assert(nr_captures < BOARD_SIDE_MAX_PIECES);
                                        captures[nr_captures].from = from;
                                        captures[nr_captures].make = capture_with_black_pawn;
                                        nr_captures++;

                                        /* Find extra defenders behind the pawn */
                                        bd->extra_defenders[from] = 0;
                                        int extra_bit =
                                                bd->current->passive.attacks[from] &
                                                board_attack_southeast;
                                        if (extra_bit != 0) {
                                                bd->extra_defenders[from] =
                                                        exchange_collect_extra_defenders(
                                                                bd, from, extra_bit);
                                        }
                                } else {
                                        GENERATE_BLACK_PROMOTION(from, to);
                                }
                        }

                        /* Find attackers behind the pawn */
                        while ((bd->current->active.attacks[from] & board_attack_southeast) != 0) {
                                do {
                                        from -= BOARD_VECTOR_SOUTHEAST;
                                        assert(BOARD_SQUARE_IS_VALID(from));
                                        piece = bd->squares[from].piece;
                                } while (piece == board_empty);
                                assert(BOARD_PIECE_COLOR(piece) == bd->current->active.color);
                                attackers += exchange_piece_to_list[piece];
                        }
                }
        }

        /*
         *  With king
         */
        if ((bits & board_attack_king) != 0) {
                attackers += EXCHANGE_LIST_ROYAL;
                /* Don't generate king moves here */
        }

        /*
         *  With knight
         */
        if (bits >= board_attack_knight) {
                signed char *knights = &bd->current->active.pieces[1];

                do {
                        attackers += EXCHANGE_LIST_MINOR;

                        bits -= board_attack_knight;

                        int from;
                        do {
                                from = *knights++;

                                assert((bd->squares[from].piece == board_white_knight) ||
                                        (bd->squares[from].piece == board_black_knight));

                        } while (data_sq2sq[to][from] != board_attack_knight);

                        assert(BOARD_PIECE_COLOR(bd->squares[from].piece) ==
                                bd->current->active.color);
                        if (IS_LEGAL(from, to)) {
                                assert(nr_captures < BOARD_SIDE_MAX_PIECES);
                                captures[nr_captures].from = from;
                                captures[nr_captures].make = capture_with_knight;
                                nr_captures++;
                                bd->extra_defenders[from] = 0;
                        }
                } while (bits >= board_attack_knight);
        }

        /*
         *  With sliding piece (Q, R, B, also pawn directions included)
         */
        int dirs = bits & BOARD_ATTACK_QUEEN;
        if (dirs != 0) {
                int dir = 0;
                do {
                        dir -= dirs;
                        dir &= dirs;
                        dirs -= dir;

                        //int step = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                        int step = board_vector_step[dir];
                        assert(step != 0);

                        int from = to;
                        do {
                                from -= step;
                                assert(BOARD_SQUARE_IS_VALID(from));
                                piece = bd->squares[from].piece;
                        } while (piece == board_empty);

                        assert(BOARD_PIECE_COLOR(piece) == bd->current->active.color);

                        attackers += exchange_piece_to_list[piece];

                        if (IS_LEGAL(from, to)) {
                                assert(nr_captures < BOARD_SIDE_MAX_PIECES);
                                captures[nr_captures].from = from;
                                captures[nr_captures].make = capture_fn_table[piece];
                                nr_captures++;

                                /* Find extra defenders behind the piece */
                                bd->extra_defenders[from] = 0;
                                int extra_bit =
                                        bd->current->passive.attacks[from] &
                                        dir;
                                if (extra_bit != 0) {
                                        bd->extra_defenders[from] =
                                                exchange_collect_extra_defenders(
                                                        bd, from, extra_bit);
                                }
                        }

                        /*
                         *  Find more attackers to complete the SEE attack list,
                         *  also if the move is illegal.
                         */
                        while ((bd->current->active.attacks[from] & dir) != 0) {
                                do {
                                        from -= step;
                                        assert(BOARD_SQUARE_IS_VALID(from));
                                        piece = bd->squares[from].piece;
                                } while (piece == board_empty);

                                assert(BOARD_SQUARE_IS_VALID(from));
                                assert(BOARD_PIECE_COLOR(piece) == bd->current->active.color);

                                attackers += exchange_piece_to_list[piece];
                        }

                } while (dirs != 0);
        }

        assert(attackers != 0);

        /*------------------------------------------------------+
         |      Calculate prescores                             |
         +------------------------------------------------------*/

        /*
         *  Get defenders
         */

        int defenders = 0;

        int defender_bits = bd->current->passive.attacks[to];
        if (defender_bits != 0) {
                defenders = exchange_collect_defenders(
                        bd,
                        to,
                        defender_bits);
        }

        /*
         *  Calculate prescores
         */

        int captured = exchange_piece_value[ bd->squares[to].piece ];

        while (nr_captures != 0) {
                nr_captures--;

                int from = captures[nr_captures].from;

                piece = bd->squares[from].piece;

                int uncover_check =
                        bd->current->active.attacks[from] &
                        BOARD_ATTACK_QUEEN;
                if (uncover_check != 0) {
                        int xking = bd->current->passive.pieces[0];
                        uncover_check &= data_sq2sq[from][xking];
                        if (uncover_check != 0) {

                                int step = board_vector_step_compact[DEBRUIJN_INDEX(uncover_check)];
                                assert(step != 0);
                                int sq = from;
                                do {
                                        sq += step;
                                        assert(BOARD_SQUARE_IS_VALID(sq));
                                } while (bd->squares[sq].piece == board_empty);

                                if (sq != xking) {
                                        uncover_check = 0;
                                }
                        }
                }

                int prescore = captured;
                if (!uncover_check) {

                        prescore -= exchange_evaluate(
                                defenders + bd->extra_defenders[from],
                                attackers + exchange_put_upfront[piece]);

                        if (prescore >= EXCHANGE_NEUTRAL) {
                                /* Non-losing captures get an extra offset */
                                prescore += EXCHANGE_GOOD_MOVE_OFFSET;
                                assert((prescore & 0xf000) == 0xf000);
                        }
                        
                } else {
                        if ((bd->current->passive.attacks[to] &
                             board_attack_king) != 0
                        ) {
                                prescore -= exchange_evaluate(
                                        EXCHANGE_LIST_ROYAL << 16,
                                        attackers + exchange_put_upfront[piece]);
                        }

                        if (prescore >= EXCHANGE_NEUTRAL) {
                                /* Non-losing captures get an extra offset */
                                prescore += EXCHANGE_GOOD_MOVE_OFFSET;
                                assert((prescore & 0xf000) == 0xf000);
                        }
                }

                int move = MOVE(from, to);
                moves_p[nr_moves].bm.move = move;
                moves_p[nr_moves].bm.prescore = bd->butterfly[move].prescore | prescore;
                moves_p[nr_moves].bm.make = (voidFn*) captures[nr_captures].make;
                (nr_moves)++;
        }

        return nr_moves;
}

/*----------------------------------------------------------------------+
 |      generate_pawn_push_to                                           |
 +----------------------------------------------------------------------*/

/*
 *  Generate pawn moves to a specified square (not capturing)
 */
static inline
int generate_pawn_push_to(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES],
        int to)
{
        int nr_moves = 0;

        /*
         *  This function is only called by generate_escapes
         *  to find pawn pushes that block an attack
         */

        int from;

        if (bd->current->active.color == board_white) {
                /*
                 *  Search white pawn to push
                 */
                switch (BOARD_RANK(to)) {
                case BOARD_RANK_1:
                case BOARD_RANK_2:
                        break;
                case BOARD_RANK_3:
                        from = to - BOARD_VECTOR_NORTH;
                        if ((bd->squares[from].piece == board_white_pawn_rank2) &&
                                IS_LEGAL(from, to))
                        {
                                GENERATE_PAWN_MOVE(from, to,
                                        move_white_pawn_rank2_to_3,
                                        board_attack_north);
                        }
                        break;
                case BOARD_RANK_4:
                        from = to - BOARD_VECTOR_NORTH;
                        if (bd->squares[from].piece == board_white_pawn) {
                                if (IS_LEGAL(from, to)) {
                                        GENERATE_PAWN_MOVE(from, to,
                                                move_white_pawn,
                                                board_attack_north);
                                }
                        } else if (bd->squares[from].piece == board_empty) {
                                from = to - 2 * BOARD_VECTOR_NORTH;
                                if ((bd->squares[from].piece == board_white_pawn_rank2) &&
                                        IS_LEGAL(from, to))
                                {
                                        GENERATE_PAWN_MOVE(from, to,
                                                move_white_pawn_rank2_to_4,
                                                board_attack_north);
                                }
                        } else {
                                pass;
                        }
                        break;
                case BOARD_RANK_5:
                case BOARD_RANK_6:
                case BOARD_RANK_7:
                        from = to - BOARD_VECTOR_NORTH;
                        if ((bd->squares[from].piece == board_white_pawn) &&
                                IS_LEGAL(from, to))
                        {
                                GENERATE_PAWN_MOVE(from, to,
                                        move_white_pawn,
                                        board_attack_north);
                        }
                        break;
                case BOARD_RANK_8:
                        from = to - BOARD_VECTOR_NORTH;
                        if ((bd->squares[from].piece == board_white_pawn_rank7) &&
                                IS_LEGAL(from, to))
                        {
                                GENERATE_WHITE_PROMOTION(from, to);
                        }
                        break;
                }
        } else {
                /*
                 *  Search black pawn to push
                 */
                switch (BOARD_RANK(to)) {
                case BOARD_RANK_8:
                case BOARD_RANK_7:
                        break;
                case BOARD_RANK_6:
                        from = to - BOARD_VECTOR_SOUTH;
                        if ((bd->squares[from].piece == board_black_pawn_rank7) &&
                                IS_LEGAL(from, to))
                        {
                                GENERATE_PAWN_MOVE(from, to,
                                        move_black_pawn_rank7_to_6,
                                        board_attack_south);
                        }
                        break;
                case BOARD_RANK_5:
                        from = to - BOARD_VECTOR_SOUTH;
                        if (bd->squares[from].piece == board_black_pawn) {
                                if (IS_LEGAL(from, to)) {
                                        GENERATE_PAWN_MOVE(from, to,
                                                move_black_pawn,
                                                board_attack_south);
                                }
                        } else if (bd->squares[from].piece == board_empty) {
                                from = to - 2 * BOARD_VECTOR_SOUTH;
                                if ((bd->squares[from].piece == board_black_pawn_rank7) &&
                                        IS_LEGAL(from, to))
                                {
                                        GENERATE_PAWN_MOVE(from, to,
                                                move_black_pawn_rank7_to_5,
                                                board_attack_south);
                                }
                        } else {
                                pass;
                        }
                        break;
                case BOARD_RANK_4:
                case BOARD_RANK_3:
                case BOARD_RANK_2:
                        from = to - BOARD_VECTOR_SOUTH;
                        if ((bd->squares[from].piece == board_black_pawn) &&
                                IS_LEGAL(from, to))
                        {
                                GENERATE_PAWN_MOVE(from, to,
                                        move_black_pawn,
                                        board_attack_south);
                        }
                        break;
                case BOARD_RANK_1:
                        from = to - BOARD_VECTOR_SOUTH;
                        if ((bd->squares[from].piece == board_black_pawn_rank2) &&
                                IS_LEGAL(from, to))
                        {
                                GENERATE_BLACK_PROMOTION(from, to);
                        }
                        break;
                }
        }

        return nr_moves;
}

/*----------------------------------------------------------------------+
 |      generate_en_passant                                             |
 +----------------------------------------------------------------------*/

/*
 *  Generate legal en-passant moves.
 *  This is a helper function for generate_escapes and generate_regular.
 */
static
int generate_en_passant(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES])
{
        int nr_moves = 0;

        int to, from;

        /*
         *  First catch false hits due to lazy setting of the en_passant flag
         */
        if (bd->current->node_counter != bd->current->en_passant_node_counter) {
                /*
                 *  The en_passant flag has expired and should have been reset.
                 *  Clear it now and leave.
                 */
                bd->current->en_passant_lazy = 0;
                return 0;
        }

        /*
         *  Testing legality of a candidate en-passant move needs special logic.
         *  The en-passant move clears 2 squares.
         *  The capture can only leave the king in check in these cases:
         *   1. Horizontal attack uncovered (king on 5-th row)
         *   2. Vertical attack uncovered through capturing pawn (e5)
         *   3. Diagonal attack uncovered through the captured pawn (e5)
         *  The captured pawn cannot uncover a check on the king.
         *   1. A vertical attack on d5 (because the d-file will remain blocked)
         *   2. A diagonal attack on d5 (otherwise the king was illegally in
         *      check before)
         *
         *   8  - - - - x - - x
         *   7  - - - - x - x -
         *   6  - - - . x x - -
         *   5  x x x * O x x x
         *   4  - - - x x - - -
         *   3  - - x - x - - -
         *   2  - x - - x - - -
         *   1  x - - - x - - -
         *      a b c d e f g h
         *
         *              +---+---+
         *           7  | . |   |
         *              +---+---+
         *           6  | . |   |
         *              +---\---+
         *           5  | * | O |
         *              +---+---+
         *                d   e
         *
         *  @TODO: the board_from_EPD() validator must check that
         *  an ep-pawn is not illegally pinned!
         */

        to = bd->current->en_passant_lazy;
        assert(BOARD_SQUARE_IS_VALID(to));

        /*
         *  East
         */
        if ((bd->current->active.attacks[to] & board_attack_pawn_east) != 0) {

                if (bd->current->active.color == board_white) {
                        /*
                         *  White to move
                         */

                        from = to - BOARD_VECTOR_NORTHEAST;

                        assert(BOARD_SQUARE_IS_VALID(from));
                        assert(bd->squares[from].piece == board_white_pawn);

                        /*
                         *  Find uncovered attacks on king after ep capture
                         */
                        int king = bd->current->active.pieces[0];
                        int pin_dirs = bd->current->passive.attacks[from] &
                                BOARD_ATTACK_QUEEN &
                                // exclude the directions along the capture:
                                ~board_attack_northeast &
                                ~BOARD_ATTACK_REVERSE(board_attack_northeast);
                                // include horizontal attack on captured pawn:
                        pin_dirs |= bd->current->passive.attacks[to - BOARD_VECTOR_NORTH] &
                                BOARD_ATTACK_REVERSE(board_attack_east);
                        pin_dirs &= data_sq2sq[from][king];

                        if (pin_dirs != 0) {
                                assert((pin_dirs & (pin_dirs - 1)) == 0); // 1 bit set

                                // prepare walking to king to see if it is exposed
                                int step = board_vector_step_compact[DEBRUIJN_INDEX(pin_dirs)];
                                int sq = from;
                                if (pin_dirs == board_attack_east) {
                                        // but hop over the captured pawn
                                        sq += step;
                                        assert(sq == to - BOARD_VECTOR_NORTH);
                                }

                                // do the walk, stop at the first piece
                                do {
                                        sq += step;
                                        assert(sq != from);
                                        assert(BOARD_SQUARE_IS_VALID(sq));
                                } while (bd->squares[sq].piece == board_empty);

                                if (sq != king) {
                                        pin_dirs = 0;
                                }
                        }

                        if (pin_dirs == 0) {
                                GENERATE_EP(from, to, enpassant_with_white_pawn);
                        }
                } else {
                        /*
                         *  Black to move
                         */

                        from = to - BOARD_VECTOR_SOUTHEAST;

                        assert(BOARD_SQUARE_IS_VALID(from));
                        assert(bd->squares[from].piece == board_black_pawn);

                        /*
                         *  Find uncovered attacks on king after ep capture
                         */
                        int king = bd->current->active.pieces[0];
                        int pin_dirs = bd->current->passive.attacks[from] &
                                BOARD_ATTACK_QUEEN &
                                // exclude the directions along the capture:
                                ~board_attack_southeast &
                                ~BOARD_ATTACK_REVERSE(board_attack_southeast);
                                // include horizontal attack on captured pawn:
                        pin_dirs |= bd->current->passive.attacks[to - BOARD_VECTOR_SOUTH] &
                                BOARD_ATTACK_REVERSE(board_attack_east);
                        pin_dirs &= data_sq2sq[from][king];

                        if (pin_dirs != 0) {
                                assert((pin_dirs & (pin_dirs - 1)) == 0); // 1 bit set

                                // prepare walking to king to see if it is exposed
                                int step = board_vector_step_compact[DEBRUIJN_INDEX(pin_dirs)];
                                int sq = from;
                                if (pin_dirs == board_attack_east) {
                                        // but hop over the captured pawn
                                        sq += step;
                                        assert(sq == to - BOARD_VECTOR_SOUTH);
                                }

                                // do the walk, stop at the first piece
                                do {
                                        sq += step;
                                        assert(sq != from);
                                        assert(BOARD_SQUARE_IS_VALID(sq));
                                } while (bd->squares[sq].piece == board_empty);

                                if (sq != king) {
                                        pin_dirs = 0;
                                }
                        }

                        if (pin_dirs == 0) {
                                GENERATE_EP(from, to, enpassant_with_black_pawn);
                        }
                }
        }

        /*
         *  West
         */
        if ((bd->current->active.attacks[to] & board_attack_pawn_west) != 0) {

                if (bd->current->active.color == board_white) {
                        /*
                         *  White to move
                         */

                        from = to - BOARD_VECTOR_NORTHWEST;

                        assert(BOARD_SQUARE_IS_VALID(from));
                        assert(bd->squares[from].piece == board_white_pawn);

                        /*
                         *  Find uncovered attacks on king after ep capture
                         */
                        int king = bd->current->active.pieces[0];
                        int pin_dirs = bd->current->passive.attacks[from] &
                                BOARD_ATTACK_QUEEN &
                                // exclude the directions along the capture:
                                ~board_attack_northwest &
                                ~BOARD_ATTACK_REVERSE(board_attack_northwest);
                                // include horizontal attack on captured pawn:
                        pin_dirs |= bd->current->passive.attacks[to - BOARD_VECTOR_NORTH] &
                                BOARD_ATTACK_REVERSE(board_attack_west);
                        pin_dirs &= data_sq2sq[from][king];

                        if (pin_dirs != 0) {
                                assert((pin_dirs & (pin_dirs - 1)) == 0); // 1 bit set

                                // prepare walking to king to see if it is exposed
                                int step = board_vector_step_compact[DEBRUIJN_INDEX(pin_dirs)];
                                int sq = from;
                                if (pin_dirs == board_attack_west) {
                                        // but hop over the captured pawn
                                        sq += step;
                                        assert(sq == to - BOARD_VECTOR_NORTH);
                                }

                                // do the walk, stop at the first piece
                                do {
                                        sq += step;
                                        assert(sq != from);
                                        assert(BOARD_SQUARE_IS_VALID(sq));
                                } while (bd->squares[sq].piece == board_empty);

                                if (sq != king) {
                                        pin_dirs = 0;
                                }
                        }

                        if (pin_dirs == 0) {
                                GENERATE_EP(from, to, enpassant_with_white_pawn);
                        }
                } else {
                        /*
                         *  Black to move
                         */

                        from = to - BOARD_VECTOR_SOUTHWEST;

                        assert(BOARD_SQUARE_IS_VALID(from));
                        assert(bd->squares[from].piece == board_black_pawn);

                        /*
                         *  Find uncovered attacks on king after ep capture
                         */
                        int king = bd->current->active.pieces[0];
                        int pin_dirs = bd->current->passive.attacks[from] &
                                BOARD_ATTACK_QUEEN &
                                // exclude the directions along the capture:
                                ~board_attack_southwest &
                                ~BOARD_ATTACK_REVERSE(board_attack_southwest);
                                // include horizontal attack on captured pawn:
                        pin_dirs |= bd->current->passive.attacks[to - BOARD_VECTOR_SOUTH] &
                                BOARD_ATTACK_REVERSE(board_attack_west);
                        pin_dirs &= data_sq2sq[from][king];

                        if (pin_dirs != 0) {
                                assert((pin_dirs & (pin_dirs - 1)) == 0); // 1 bit set

                                // prepare walking to king to see if it is exposed
                                int step = board_vector_step_compact[DEBRUIJN_INDEX(pin_dirs)];
                                int sq = from;
                                if (pin_dirs == board_attack_west) {
                                        // but hop over the captured pawn
                                        sq += step;
                                        assert(sq == to - BOARD_VECTOR_SOUTH);
                                }

                                // do the walk, stop at the first piece
                                do {
                                        sq += step;
                                        assert(sq != from);
                                        assert(BOARD_SQUARE_IS_VALID(sq));
                                } while (bd->squares[sq].piece == board_empty);

                                if (sq != king) {
                                        pin_dirs = 0;
                                }
                        }

                        if (pin_dirs == 0) {
                                GENERATE_EP(from, to, enpassant_with_black_pawn);
                        }
                }
        }

        return nr_moves;
}

/*----------------------------------------------------------------------+
 |      _is_legal                                                       |
 +----------------------------------------------------------------------*/

/*
 *  Helper function for IS_LEGAL. Always use IS_LEGAL,
 *  do not call this inner function directly.
 */
static
bool _is_legal(struct board *bd, int from, int to)
{
        int king = bd->current->active.pieces[0];

        int pin_dirs = data_sq2sq[from][king] & BOARD_ATTACK_QUEEN;
        int move_dir = data_sq2sq[from][to] & BOARD_ATTACK_QUEEN;

        assert(pin_dirs != 0);
        assert((pin_dirs & (pin_dirs - 1)) == 0); // one bit set

        /*
         *  Should be either a knight jump or one sliding bit set
         */
        assert((move_dir == 0) || ((move_dir & (move_dir - 1)) == 0));

        /*
         *  If the pinned piece moves in the same or opposite direction as
         *  the pin, there is no risk to expose the king.
         */
        if ((pin_dirs & (move_dir | BOARD_ATTACK_REVERSE(move_dir))) != 0) {
                return true;
        }

        /*
         *  The piece is not pinned if there is another piece obstructing
         *  the path towards the king. Otherwise it is pinned and the
         *  proposed move is illegal.
         */
        int step = board_vector_step_compact[DEBRUIJN_INDEX(pin_dirs)];
        int sq = from;
        do {
                sq += step;
                assert(sq != from);
                assert(BOARD_SQUARE_IS_VALID(sq));
        } while (bd->squares[sq].piece == board_empty);

        return (sq != king);
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

