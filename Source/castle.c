
/*----------------------------------------------------------------------+
 |                                                                      |
 |      castle.c -- Move making functions for castling                  |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Move making functions for castling
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
 *  Copyright (C) 1992-2012, Marcel van Kervinck
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
#include "castle.h"

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
 |      castle_white_king_side                                          |
 +----------------------------------------------------------------------*/

/*
 *  King-side castling ("O-O") with white
 */
void castle_white_king_side(struct board *bd, int from, int to)
{
        unused(from);
        unused(to);

        int king_from  = E1;
        int king_to    = G1;
        int rook_from  = H1;
        int rook_to    = F1;
        int other_rook = A1;

        /*
         *  Update Zobrist hashes
         */
        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_king][king_from] ^
                data_zobrist[zobrist_white_king][king_to] ^
                data_zobrist[zobrist_white_rook_castle][rook_from] ^
                data_zobrist[zobrist_white_rook][rook_to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_king][king_from] ^
                data_zobrist[zobrist_white_king][king_to] ^
                data_zobrist[zobrist_white_rook_castle][rook_from];

        /*
         *  Prepare board_unmake_move
         */
        PUSH_UNDO(bd->current, rook_from);
        PUSH_UNDO(bd->current, rook_to);

        /*
         *  Remove attacks
         */
        attack_xor_king(&bd->current->passive, king_from);
        attack_xor_rays(&bd->current->passive, bd, rook_from,
                BOARD_ATTACK_ROOK & data_kingtab[rook_from]);

        /*
         *  Move the king
         */
        bd->current->passive.pieces[ bd->squares[king_from].index ] = king_to;
        bd->squares[king_to].piece = board_white_king;
        bd->squares[king_to].index = 0;
        bd->squares[king_from].piece = board_empty;
        bd->squares[king_from].index = 0;

        /*
         *  Move the rook
         */
        bd->current->passive.pieces[ bd->squares[rook_from].index ] = rook_to;
        bd->squares[rook_to].piece = board_white_rook;
        bd->squares[rook_to].index = bd->squares[rook_from].index;
        bd->squares[rook_from].piece = board_empty;
        bd->squares[rook_from].index = 0;

        /*
         *  Place new attacks
         */
        int rays = bd->current->passive.attacks[king_from] &
                data_kingtab[king_from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, king_from, rays);
        }
        attack_xor_king(&bd->current->passive, king_to);
        attack_xor_rays(&bd->current->passive, bd, rook_to,
                BOARD_ATTACK_ROOK & data_kingtab[rook_to]);

        /*
         *  Downgrade other rook, if needed
         */
        if (bd->squares[other_rook].piece == board_white_rook_castle) {
                PUSH_UNDO(bd->current, other_rook);
                bd->squares[other_rook].piece = board_white_rook;

                bd->current->board_hash_lazy ^=
                        data_zobrist[zobrist_white_rook_castle][other_rook] ^
                        data_zobrist[zobrist_white_rook][other_rook];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_white_rook_castle][other_rook];
        }

        /*------------------------------------------------------+
         |      Update halfmove clock                           |
         +------------------------------------------------------*/

        bd->current->halfmove_clock = bd->current[-1].halfmove_clock + 1;
}

/*----------------------------------------------------------------------+
 |      castle_white_queen_side                                         |
 +----------------------------------------------------------------------*/

/*
 *  Queen-side castling ("O-O-O") with white
 */
void castle_white_queen_side(struct board *bd, int from, int to)
{
        unused(from);
        unused(to);

        int king_from  = E1;
        int king_to    = C1;
        int rook_from  = A1;
        int rook_to    = D1;
        int other_rook = H1;

        /*
         *  Update Zobrist hashes
         */
        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_king][king_from] ^
                data_zobrist[zobrist_white_king][king_to] ^
                data_zobrist[zobrist_white_rook_castle][rook_from] ^
                data_zobrist[zobrist_white_rook][rook_to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_king][king_from] ^
                data_zobrist[zobrist_white_king][king_to] ^
                data_zobrist[zobrist_white_rook_castle][rook_from];

        /*
         *  Prepare board_unmake_move
         */
        PUSH_UNDO(bd->current, rook_from);
        PUSH_UNDO(bd->current, rook_to);

        /*
         *  Remove attacks
         */
        attack_xor_king(&bd->current->passive, king_from);
        attack_xor_rays(&bd->current->passive, bd, rook_from,
                BOARD_ATTACK_ROOK & data_kingtab[rook_from]);

        /*
         *  Move the king
         */
        bd->current->passive.pieces[ bd->squares[king_from].index ] = king_to;
        bd->squares[king_to].piece = board_white_king;
        bd->squares[king_to].index = 0;
        bd->squares[king_from].piece = board_empty;
        bd->squares[king_from].index = 0;

        /*
         *  Move the rook
         */
        bd->current->passive.pieces[ bd->squares[rook_from].index ] = rook_to;
        bd->squares[rook_to].piece = board_white_rook;
        bd->squares[rook_to].index = bd->squares[rook_from].index;
        bd->squares[rook_from].piece = board_empty;
        bd->squares[rook_from].index = 0;

        /*
         *  Place new attacks
         */
        int rays = bd->current->passive.attacks[king_from] &
                data_kingtab[king_from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, king_from, rays);
        }
        attack_xor_king(&bd->current->passive, king_to);
        attack_xor_rays(&bd->current->passive, bd, rook_to,
                BOARD_ATTACK_ROOK & data_kingtab[rook_to]);

        /*
         *  Downgrade other rook, if needed
         */
        if (bd->squares[other_rook].piece == board_white_rook_castle) {
                PUSH_UNDO(bd->current, other_rook);
                bd->squares[other_rook].piece = board_white_rook;

                bd->current->board_hash_lazy ^=
                        data_zobrist[zobrist_white_rook_castle][other_rook] ^
                        data_zobrist[zobrist_white_rook][other_rook];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_white_rook_castle][other_rook];
        }

        /*------------------------------------------------------+
         |      Update halfmove clock                           |
         +------------------------------------------------------*/

        bd->current->halfmove_clock = bd->current[-1].halfmove_clock + 1;
}

/*----------------------------------------------------------------------+
 |      castle_black_king_side                                          |
 +----------------------------------------------------------------------*/

/*
 *  King-side castling ("O-O") with black
 */
void castle_black_king_side(struct board *bd, int from, int to)
{
        unused(from);
        unused(to);

        int king_from  = E8;
        int king_to    = G8;
        int rook_from  = H8;
        int rook_to    = F8;
        int other_rook = A8;

        /*
         *  Update Zobrist hashes
         */
        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_king][king_from] ^
                data_zobrist[zobrist_black_king][king_to] ^
                data_zobrist[zobrist_black_rook_castle][rook_from] ^
                data_zobrist[zobrist_black_rook][rook_to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_king][king_from] ^
                data_zobrist[zobrist_black_king][king_to] ^
                data_zobrist[zobrist_black_rook_castle][rook_from];

        /*
         *  Prepare board_unmake_move
         */
        PUSH_UNDO(bd->current, rook_from);
        PUSH_UNDO(bd->current, rook_to);

        /*
         *  Remove attacks
         */
        attack_xor_king(&bd->current->passive, king_from);
        attack_xor_rays(&bd->current->passive, bd, rook_from,
                BOARD_ATTACK_ROOK & data_kingtab[rook_from]);

        /*
         *  Move the king
         */
        bd->current->passive.pieces[ bd->squares[king_from].index ] = king_to;
        bd->squares[king_to].piece = board_black_king;
        bd->squares[king_to].index = 0;
        bd->squares[king_from].piece = board_empty;
        bd->squares[king_from].index = 0;

        /*
         *  Move the rook
         */
        bd->current->passive.pieces[ bd->squares[rook_from].index ] = rook_to;
        bd->squares[rook_to].piece = board_black_rook;
        bd->squares[rook_to].index = bd->squares[rook_from].index;
        bd->squares[rook_from].piece = board_empty;
        bd->squares[rook_from].index = 0;

        /*
         *  Place new attacks
         */
        int rays = bd->current->passive.attacks[king_from] &
                data_kingtab[king_from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, king_from, rays);
        }
        attack_xor_king(&bd->current->passive, king_to);
        attack_xor_rays(&bd->current->passive, bd, rook_to,
                BOARD_ATTACK_ROOK & data_kingtab[rook_to]);

        /*
         *  Downgrade other rook, if needed
         */
        if (bd->squares[other_rook].piece == board_black_rook_castle) {
                PUSH_UNDO(bd->current, other_rook);
                bd->squares[other_rook].piece = board_black_rook;

                bd->current->board_hash_lazy ^=
                        data_zobrist[zobrist_black_rook_castle][other_rook] ^
                        data_zobrist[zobrist_black_rook][other_rook];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_black_rook_castle][other_rook];
        }

        /*------------------------------------------------------+
         |      Update halfmove clock                           |
         +------------------------------------------------------*/

        bd->current->halfmove_clock = bd->current[-1].halfmove_clock + 1;
}

/*----------------------------------------------------------------------+
 |      castle_black_queen_side                                         |
 +----------------------------------------------------------------------*/

/*
 *  Queen-side castling ("O-O-O") with black
 */
void castle_black_queen_side(struct board *bd, int from, int to)
{
        unused(from);
        unused(to);

        int king_from  = E8;
        int king_to    = C8;
        int rook_from  = A8;
        int rook_to    = D8;
        int other_rook = H8;

        /*
         *  Update Zobrist hashes
         */
        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_king][king_from] ^
                data_zobrist[zobrist_black_king][king_to] ^
                data_zobrist[zobrist_black_rook_castle][rook_from] ^
                data_zobrist[zobrist_black_rook][rook_to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_king][king_from] ^
                data_zobrist[zobrist_black_king][king_to] ^
                data_zobrist[zobrist_black_rook_castle][rook_from];

        /*
         *  Prepare board_unmake_move
         */
        PUSH_UNDO(bd->current, rook_from);
        PUSH_UNDO(bd->current, rook_to);

        /*
         *  Remove attacks
         */
        attack_xor_king(&bd->current->passive, king_from);
        attack_xor_rays(&bd->current->passive, bd, rook_from,
                BOARD_ATTACK_ROOK & data_kingtab[rook_from]);

        /*
         *  Move the king
         */
        bd->current->passive.pieces[ bd->squares[king_from].index ] = king_to;
        bd->squares[king_to].piece = board_black_king;
        bd->squares[king_to].index = 0;
        bd->squares[king_from].piece = board_empty;
        bd->squares[king_from].index = 0;

        /*
         *  Move the rook
         */
        bd->current->passive.pieces[ bd->squares[rook_from].index ] = rook_to;
        bd->squares[rook_to].piece = board_black_rook;
        bd->squares[rook_to].index = bd->squares[rook_from].index;
        bd->squares[rook_from].piece = board_empty;
        bd->squares[rook_from].index = 0;

        /*
         *  Place new attacks
         */
        int rays = bd->current->passive.attacks[king_from] &
                data_kingtab[king_from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, king_from, rays);
        }
        attack_xor_king(&bd->current->passive, king_to);
        attack_xor_rays(&bd->current->passive, bd, rook_to,
                BOARD_ATTACK_ROOK & data_kingtab[rook_to]);

        /*
         *  Downgrade other rook, if needed
         */
        if (bd->squares[other_rook].piece == board_black_rook_castle) {
                PUSH_UNDO(bd->current, other_rook);
                bd->squares[other_rook].piece = board_black_rook;

                bd->current->board_hash_lazy ^=
                        data_zobrist[zobrist_black_rook_castle][other_rook] ^
                        data_zobrist[zobrist_black_rook][other_rook];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_black_rook_castle][other_rook];
        }

        /*------------------------------------------------------+
         |      Update halfmove clock                           |
         +------------------------------------------------------*/

        bd->current->halfmove_clock = bd->current[-1].halfmove_clock + 1;
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

