
/*----------------------------------------------------------------------+
 |                                                                      |
 |      attack.c -- Maintain attack tables                              |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Functions to add or remove a piece from an attack table.
 *
 *  History:
 *      2008-01-11 (marcelk) Tidy up original draft version.
 */

/*----------------------------------------------------------------------+
 |      Copyright                                                       |       
 +----------------------------------------------------------------------*/

/*
 *  This file is part of the Rookie(TM) Chess Program
 *  Copyright (C) 1992-2008, Marcel van Kervinck
 *  All rights reserved.
 */

/*----------------------------------------------------------------------+
 |      Includes                                                        |       
 +----------------------------------------------------------------------*/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

/*
 *  Base include
 */
#include "cplus.h"

/*
 *  Own interface include
 */
#include "board.h"
#include "intern.h"
#include "attack.h"

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      attack_xor_rays                                                 |
 +----------------------------------------------------------------------*/

/*
 *  Flip (XOR) the sliding attack rays specified in 'dirs' (directions)
 *  on the side->attacks[] board, originating from square 'sq'
 *
 *  Example: sq = e4,
 *  blocking pieces on b7 + g4,
 *  dirs = north + south + northwest + east,
 *  Then the attacks on the following squares are changed:
 *  8  - - - - x - - -
 *  7  - x - - x - - -
 *  6  - - x - x - - -
 *  5  - - - x x - - -
 *  4  - - - - - x X -
 *  3  - - - - x - - -
 *  2  - - - - x - - -
 *  1  - - - - x - - -
 *     a b c d e f g h
 */
void attack_xor_rays(
        struct board_side *side,
        const struct board *bd,
        int sq,
        int dirs)
{
        int dir = 0;
        int to;
        int len;

        /*
         *  28 out of 64 squares (44%) are on the edge of the board.
         *  Many of the rays on these square have length 0.
         *  We make the caller responsible for this culling these.
         */
        assert(dirs != 0);
        assert((dirs & data_kingtab[sq]) == dirs);

        do {
                dir -= dirs;
                dir &= dirs;
                dirs -= dir;

                //vector = board_vector_step_compact[DEBRUIJN_INDEX(dir)];
                int vector = board_vector_step[dir];
                assert(vector != 0);

                len = data_raylen[sq][DEBRUIJN_INDEX(dir)];
                assert(len > 0);
                to = sq;
#ifndef __GNUC__
                do {
                        to += vector;
                        assert(BOARD_SQUARE_IS_VALID(to));

                        side->attacks[to] ^= dir;
                        if (bd->squares[to].piece != board_empty) {
                                break;
                        }
                } while (--len);
#else
                static void * const jump_table[] = {
                        [7] = &&len_7,
                        [6] = &&len_6,
                        [5] = &&len_5,
                        [4] = &&len_4,
                        [3] = &&len_3,
                        [2] = &&len_2,
                        [1] = &&len_1,
                };

                goto *(jump_table[len]);
len_7:
                to += vector;
                side->attacks[to] ^= dir;
                if (bd->squares[to].piece != board_empty) {
                        continue;
                }
len_6:
                to += vector;
                side->attacks[to] ^= dir;
                if (bd->squares[to].piece != board_empty) {
                        continue;
                }
len_5:
                to += vector;
                side->attacks[to] ^= dir;
                if (bd->squares[to].piece != board_empty) {
                        continue;
                }
len_4:
                to += vector;
                side->attacks[to] ^= dir;
                if (bd->squares[to].piece != board_empty) {
                        continue;
                }
len_3:
                to += vector;
                side->attacks[to] ^= dir;
                if (bd->squares[to].piece != board_empty) {
                        continue;
                }
len_2:
                to += vector;
                side->attacks[to] ^= dir;
                if (bd->squares[to].piece != board_empty) {
                        continue;
                }
len_1:
                to += vector;
                side->attacks[to] ^= dir;
#endif
        } while (dirs != 0);
}

/*----------------------------------------------------------------------+
 |      attack_xor_king                                                 |
 +----------------------------------------------------------------------*/

/*
 *  Flip (xor) the king attack bits around square 'sq'
 *
 *  Example: sq = e4,
 *  Then the attacks on the following squares are changed:
 *  8  - - - - - - - -
 *  7  - - - - - - - -
 *  6  - - - - - - - -
 *  5  - - - x x x - -
 *  4  - - - x - x - -
 *  3  - - - x x x - -
 *  2  - - - - - - - -
 *  1  - - - - - - - -
 *     a b c d e f g h
 */
void attack_xor_king(struct board_side *side, int sq)
{
        int dirs, dir = 0;
        dirs = data_kingtab[sq];
        do {
                dir -= dirs;
                dir &= dirs;
                dirs -= dir;

                int to = sq + board_vector_step[dir];
                assert(to != sq);
                assert(BOARD_SQUARE_IS_VALID(to));

                side->attacks[to] ^= board_attack_king;
        } while (dirs != 0);
}

/*----------------------------------------------------------------------+
 |      attack_add_knight, attack_sub_knight                            |
 +----------------------------------------------------------------------*/

/*
 *  Add (or subtract) the knight attack counters around square 'sq'
 *
 *  Example: sq = e4,
 *  Then the attacks on the following squares are changed:
 *  8  - - - - - - - -
 *  7  - - - - - - - -
 *  6  - - - x - x - -
 *  5  - - x - - - x -
 *  4  - - - - - - - -
 *  3  - - x - - - x -
 *  2  - - - x - x - -
 *  1  - - - - - - - -
 *     a b c d e f g h
 */

void attack_add_knight(struct board_side *side, int sq)
{
        int dirs, dir = 0;

        dirs = data_knighttab[sq];
        do {
                dir -= dirs;
                dir &= dirs;
                dirs -= dir;

                int to = sq + board_vector_jump[dir];
                assert(to != sq);
                assert(BOARD_SQUARE_IS_VALID(to));

                side->attacks[to] += board_attack_knight;
        } while (dirs != 0);
}

/*----------------------------------------------------------------------*/

void attack_sub_knight(struct board_side *side, int sq)
{
        int dirs, dir = 0;

        dirs = data_knighttab[sq];
        do {
                dir -= dirs;
                dir &= dirs;
                dirs -= dir;

                int to = sq + board_vector_jump[dir];
                assert(to != sq);
                assert(BOARD_SQUARE_IS_VALID(to));

                side->attacks[to] -= board_attack_knight;
        } while (dirs != 0);
}

/*----------------------------------------------------------------------+
 |      attack_xor_white_pawn, attack_xor_black_pawn                    |
 +----------------------------------------------------------------------*/

/*
 *  Flip (xor) the pawn attack bits for a white pawn
 */
void attack_xor_white_pawn(struct board_side *side, int sq)
{
        if (sq >= B1) {
                side->attacks[sq + BOARD_VECTOR_NORTHWEST] ^=
                        board_attack_pawn_west;
        }
        if (sq <  H1) {
                side->attacks[sq + BOARD_VECTOR_NORTHEAST] ^=
                        board_attack_pawn_east;
        }
}

/*----------------------------------------------------------------------*/

/*
 *  Flip (xor) the pawn attack bits for a black pawn
 */
void attack_xor_black_pawn(struct board_side *side, int sq)
{
        if (sq >= B1) {
                side->attacks[sq + BOARD_VECTOR_SOUTHWEST] ^=
                        board_attack_pawn_west;
        }
        if (sq <  H1) {
                side->attacks[sq + BOARD_VECTOR_SOUTHEAST] ^=
                        board_attack_pawn_east;
        }
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

