
/*----------------------------------------------------------------------+
 |                                                                      |
 |      enpassant.c -- Move making functions for en-passant capture     |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Move making functions for en-passant capture
 *
 *  History:
 *      2008-01-12 (marcelk) Tidy up original draft version.
 *      2008-01-13 (marcelk) Added Zobrist hashing.
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
#include "enpassant.h"

/*
 *  Other includes
 */
#include "attack.h"

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      enpassant_with_white_pawn                                       |
 +----------------------------------------------------------------------*/

/*
 *  Perform en-passant capture with a white pawn
 */
void enpassant_with_white_pawn(
        struct board *bd,
        int from,
        int to)
{
        int victim = to + BOARD_VECTOR_SOUTH;
        int rays;
        int index;

        assert(BOARD_RANK(victim) == BOARD_RANK_5);
        assert(bd->squares[victim].piece == board_black_pawn);
        assert(bd->squares[to].piece == board_empty);

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_pawn][from] ^
                data_zobrist[zobrist_white_pawn][to] ^
                data_zobrist[zobrist_black_pawn][victim];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_pawn][from] ^
                data_zobrist[zobrist_white_pawn][to] ^
                data_zobrist[zobrist_black_pawn][victim];

        bd->current->material_key -= data_material_key[board_black_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        /*
         *  Block any attacks passing through this square
         */

        rays = bd->current->passive.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, to, rays);
        }

        rays = bd->current->active.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, to, rays);
        }

        /*
         *  Add piece attacks originating from this square
         */

        attack_xor_white_pawn(&bd->current->passive, to);

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update board
         */

        bd->squares[to] = bd->squares[from];

        /*------------------------------------------------------+
         |      Clear victim-square                             |
         +------------------------------------------------------*/

        /*
         *  Extend any attacks passing through this square
         */

        rays = bd->current->passive.attacks[victim] & data_kingtab[victim];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, victim, rays);
        }

        rays = bd->current->active.attacks[victim] & data_kingtab[victim];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, victim, rays);
        }

        /*
         *  Remove piece attacks originating from this square
         */

        attack_xor_black_pawn(&bd->current->active, victim);

        /*
         *  Update piece list
         */

        index = bd->squares[victim].index;
        int nr_pieces = bd->current->active.nr_pieces - 1;
        int other = bd->current->active.pieces[nr_pieces];

        PUSH_UNDO(bd->current, other);
        bd->squares[other].index = index;
        bd->current->active.pieces[index] = other;
        bd->current->active.pieces[nr_pieces] = -1;
        bd->current->active.nr_pieces = nr_pieces;

        /*
         *  Update board
         */

        PUSH_UNDO(bd->current, victim);
        bd->squares[victim].piece = board_empty;
        bd->squares[victim].index = 0;

        /*------------------------------------------------------+
         |      Clear from-square                               |
         +------------------------------------------------------*/

        /*
         *  Extend any attacks
         */

        rays = bd->current->passive.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, from, rays);
        }

        rays = bd->current->active.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, from, rays);
        }

        /*
         *  Remove own attacks
         */

        attack_xor_white_pawn(&bd->current->passive, from);

        /*
         *  Update board
         */


        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |      enpassant_with_white_pawn                                       |
 +----------------------------------------------------------------------*/

/*
 *  Perform en-passant capture with a white pawn
 */
void enpassant_with_black_pawn(
        struct board *bd,
        int from,
        int to)
{
        int victim = to + BOARD_VECTOR_NORTH;
        int rays;
        int index;

        assert(BOARD_RANK(victim) == BOARD_RANK_4);
        assert(bd->squares[victim].piece == board_white_pawn);
        assert(bd->squares[to].piece == board_empty);

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_pawn][from] ^
                data_zobrist[zobrist_black_pawn][to] ^
                data_zobrist[zobrist_white_pawn][victim];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_pawn][from] ^
                data_zobrist[zobrist_black_pawn][to] ^
                data_zobrist[zobrist_white_pawn][victim];

        bd->current->material_key -= data_material_key[board_white_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        /*
         *  Block any attacks passing through this square
         */

        rays = bd->current->passive.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, to, rays);
        }

        rays = bd->current->active.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, to, rays);
        }

        /*
         *  Add piece attacks originating from this square
         */

        attack_xor_black_pawn(&bd->current->passive, to);

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update board
         */

        bd->squares[to] = bd->squares[from];

        /*------------------------------------------------------+
         |      Clear victim-square                             |
         +------------------------------------------------------*/

        /*
         *  Extend any attacks passing through this square
         */

        rays = bd->current->passive.attacks[victim] & data_kingtab[victim];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, victim, rays);
        }

        rays = bd->current->active.attacks[victim] & data_kingtab[victim];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, victim, rays);
        }

        /*
         *  Remove piece attacks originating from this square
         */

        attack_xor_white_pawn(&bd->current->active, victim);

        /*
         *  Update piece list
         */

        index = bd->squares[victim].index;
        int nr_pieces = bd->current->active.nr_pieces - 1;
        int other = bd->current->active.pieces[nr_pieces];

        PUSH_UNDO(bd->current, other);
        bd->squares[other].index = index;
        bd->current->active.pieces[index] = other;
        bd->current->active.pieces[nr_pieces] = -1;
        bd->current->active.nr_pieces = nr_pieces;

        /*
         *  Update board
         */

        PUSH_UNDO(bd->current, victim);
        bd->squares[victim].piece = board_empty;
        bd->squares[victim].index = 0;

        /*------------------------------------------------------+
         |      Clear from-square                               |
         +------------------------------------------------------*/

        /*
         *  Extend any attacks
         */

        rays = bd->current->passive.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, from, rays);
        }

        rays = bd->current->active.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, from, rays);
        }

        /*
         *  Remove own attacks
         */

        attack_xor_black_pawn(&bd->current->passive, from);

        /*
         *  Update board
         */

        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

