
/*----------------------------------------------------------------------+
 |                                                                      |
 |      promote.c -- Move making functions for promotion                |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Move making functions for promotion
 *
 *  History:
 *      2008-01-13 (marcelk) Tidy up original draft version.
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
#include "promote.h"

/*
 *  Other includes
 */
#include "attack.h"
#include "capture.h"

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
 |      promote_white_queen                                             |
 +----------------------------------------------------------------------*/

/*
 *  Promote white pawn to queen
 */
void promote_white_queen(
        struct board *bd,
        int from,
        int to)
{
        int rays;
        int index;

        assert(BOARD_RANK(from) == BOARD_RANK_7);

        /*------------------------------------------------------+
         |      Fix to-square                                   |
         +------------------------------------------------------*/

        to ^= XOR_PROM_QUEEN;

        /*
         *  Repair undo information because board_make_move has used
         *  the wrong `to' square
         */
        bd->current->undo[UNDO_TO].square = to;
        bd->current->undo[UNDO_TO].piece = bd->squares[to];

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_pawn][from] ^
                data_zobrist[zobrist_white_queen][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_pawn][from];

        bd->current->material_key +=
                data_material_key[board_white_queen] -
                data_material_key[board_white_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        if (BOARD_FILE(to) == BOARD_FILE(from)) { // NO CAPTURE

                /*
                 *  Add missing piece attacks originating from this square
                 */

                rays = (bd->current->passive.attacks[to] ^ BOARD_ATTACK_QUEEN) &
                        data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->passive, bd, to, rays);
                }

                /*
                 *  Block any attacks passing through this square
                 */
                rays = bd->current->active.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->active, bd, to, rays);
                }

        } else {

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

                capture_take_piece_fn *take_piece;

                take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
                assert(take_piece != null);
                take_piece(bd, to);

                /*
                 *  Add piece attacks originating from this square
                 */
                attack_xor_rays(&bd->current->passive, bd, to,
                        BOARD_ATTACK_QUEEN & data_kingtab[to]);
        }

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update board
         */

        bd->squares[to].index = bd->squares[from].index;
        bd->squares[to].piece = board_white_queen;

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
         *  Clear last_rank_pawn bit
         */

        bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(from);

        /*
         *  Update board
         */

        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*
         *  Leave extra mark in undo[] that indicates the promotion piece.
         *
         *  This info is only used when we try to reconstruct moves from the
         *  stack. The main reason for doing this in such a complicated way is
         *  that we don't want to spend a single CPU cycle to store the move,
         *  because all the info to reconstruct the move is already present
         *  in the undo stack anyway and we don't need the move in the
         *  critical part of the search. All info is there already, except the
         *  promotion piece. So we only do some extra work for promotions.
         *
         *  We further assume that promotions don't use up all space in
         *  undo[], so we write past the last entry. This holds, because
         *  the worst case for undo[] is a king capture, not a promotion.
         */
        assert(bd->current->undo_len < BOARD_UNDO_LEN_MAX);
        bd->current->undo[ bd->current->undo_len ].piece.piece = bd->squares[to].piece;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |      promote_white_rook                                              |
 +----------------------------------------------------------------------*/

/*
 *  Promote white pawn to rook
 */
void promote_white_rook(
        struct board *bd,
        int from,
        int to)
{
        int rays;
        int index;

        assert(BOARD_RANK(from) == BOARD_RANK_7);

        /*------------------------------------------------------+
         |      Fix to-square                                   |
         +------------------------------------------------------*/

        to ^= XOR_PROM_ROOK;

        /*
         *  Repair undo information because board_make_move has used
         *  the wrong `to' square
         */
        bd->current->undo[UNDO_TO].square = to;
        bd->current->undo[UNDO_TO].piece = bd->squares[to];

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_pawn][from] ^
                data_zobrist[zobrist_white_rook][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_pawn][from];

        bd->current->material_key +=
                data_material_key[board_white_rook] -
                data_material_key[board_white_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        if (BOARD_FILE(to) == BOARD_FILE(from)) { // NO CAPTURE

                /*
                 *  Add missing piece attacks originating from this square
                 */

                rays = (bd->current->passive.attacks[to] ^ BOARD_ATTACK_ROOK) &
                        data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->passive, bd, to, rays);
                }

                /*
                 *  Block any attacks passing through this square
                 */
                rays = bd->current->active.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->active, bd, to, rays);
                }

        } else {

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

                capture_take_piece_fn *take_piece;

                take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
                assert(take_piece != null);
                take_piece(bd, to);

                /*
                 *  Add piece attacks originating from this square
                 */
                attack_xor_rays(&bd->current->passive, bd, to,
                        BOARD_ATTACK_ROOK & data_kingtab[to]);
        }

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update board
         */

        bd->squares[to].index = bd->squares[from].index;
        bd->squares[to].piece = board_white_rook;

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
         *  Clear last_rank_pawn bit
         */

        bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(from);

        /*
         *  Update board
         */

        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*
         *  Leave extra mark in undo[] that indicates the promotion piece.
         *
         *  This info is only used when we try to reconstruct moves from the 
         *  stack. The main reason for doing this in such a complicated way is
         *  that we don't want to spend a single CPU cycle to store the move,
         *  because all the info to reconstruct the move is already present
         *  in the undo stack anyway and we don't need the move in the
         *  critical part of the search. All info is there already, except the
         *  promotion piece. So we only do some extra work for promotions.
         *
         *  We further assume that promotions don't use up all space in
         *  undo[], so we write past the last entry. This holds, because
         *  the worst case for undo[] is a king capture, not a promotion.
         */
        assert(bd->current->undo_len < BOARD_UNDO_LEN_MAX);
        bd->current->undo[ bd->current->undo_len ].piece.piece = bd->squares[to].piece;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |      promote_white_bishop                                            |
 +----------------------------------------------------------------------*/

/*
 *  Promote white pawn to bishop
 */
void promote_white_bishop(
        struct board *bd,
        int from,
        int to)
{
        int rays;
        int index;

        assert(BOARD_RANK(from) == BOARD_RANK_7);

        /*------------------------------------------------------+
         |      Fix to-square                                   |
         +------------------------------------------------------*/

        to ^= XOR_PROM_BISHOP;

        /*
         *  Repair undo information because board_make_move has used
         *  the wrong `to' square
         */
        bd->current->undo[UNDO_TO].square = to;
        bd->current->undo[UNDO_TO].piece = bd->squares[to];

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_pawn][from] ^
                data_zobrist[zobrist_white_bishop][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_pawn][from];

        bd->current->material_key +=
                data_material_key[ BOARD_SQUARE_IS_LIGHT(to) ?
                        board_white_bishop_light :
                        board_white_bishop_dark ] -
                data_material_key[board_white_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        if (BOARD_FILE(to) == BOARD_FILE(from)) { // NO CAPTURE

                /*
                 *  Add missing piece attacks originating from this square
                 */

                rays = (bd->current->passive.attacks[to] ^ BOARD_ATTACK_BISHOP) &
                        data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->passive, bd, to, rays);
                }

                /*
                 *  Block any attacks passing through this square
                 */
                rays = bd->current->active.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->active, bd, to, rays);
                }

        } else {

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

                capture_take_piece_fn *take_piece;

                take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
                assert(take_piece != null);
                take_piece(bd, to);

                /*
                 *  Add piece attacks originating from this square
                 */
                attack_xor_rays(&bd->current->passive, bd, to,
                        BOARD_ATTACK_BISHOP & data_kingtab[to]);
        }

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update bishop diagonal mask
         */
        bd->current->passive.bishop_diagonals ^= data_bishop_diagonals[to];

        /*
         *  Update board
         */

        bd->squares[to].index = bd->squares[from].index;
        bd->squares[to].piece =
                BOARD_SQUARE_IS_LIGHT(to) ?
                board_white_bishop_light :
                board_white_bishop_dark;

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
         *  Clear last_rank_pawn bit
         */

        bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(from);

        /*
         *  Update board
         */

        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*
         *  Leave extra mark in undo[] that indicates the promotion piece.
         *
         *  This info is only used when we try to reconstruct moves from the
         *  stack. The main reason for doing this in such a complicated way is
         *  that we don't want to spend a single CPU cycle to store the move,
         *  because all the info to reconstruct the move is already present
         *  in the undo stack anyway and we don't need the move in the
         *  critical part of the search. All info is there already, except the
         *  promotion piece. So we only do some extra work for promotions.
         *
         *  We further assume that promotions don't use up all space in
         *  undo[], so we write past the last entry. This holds, because
         *  the worst case for undo[] is a king capture, not a promotion.
         */
        assert(bd->current->undo_len < BOARD_UNDO_LEN_MAX);
        bd->current->undo[ bd->current->undo_len ].piece.piece = bd->squares[to].piece;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |      promote_white_knight                                            |
 +----------------------------------------------------------------------*/

/*
 *  Promote white pawn to knight
 */
void promote_white_knight(
        struct board *bd,
        int from,
        int to)
{
        int rays;
        int index;

        assert(BOARD_RANK(from) == BOARD_RANK_7);

        /*------------------------------------------------------+
         |      Fix to-square                                   |
         +------------------------------------------------------*/

        to ^= XOR_PROM_KNIGHT;

        /*
         *  Repair undo information because board_make_move has used
         *  the wrong `to' square
         */
        bd->current->undo[UNDO_TO].square = to;
        bd->current->undo[UNDO_TO].piece = bd->squares[to];

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_pawn][from] ^
                data_zobrist[zobrist_white_knight][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_pawn][from];

        bd->current->material_key +=
                data_material_key[board_white_knight] -
                data_material_key[board_white_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        if (BOARD_FILE(to) == BOARD_FILE(from)) { // NO CAPTURE

                /*
                 *  Add missing piece attacks originating from this square
                 */

                rays = bd->current->passive.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->passive, bd, to, rays);
                }

                /*
                 *  Block any attacks passing through this square
                 */
                rays = bd->current->active.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->active, bd, to, rays);
                }
        } else {

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

                capture_take_piece_fn *take_piece;

                take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
                assert(take_piece != null);
                take_piece(bd, to);
        }

        /*
         *  Add piece attacks originating from this square
         */
        attack_add_knight(&bd->current->passive, to);

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update board
         */

        bd->squares[to].index = bd->squares[from].index;
        bd->squares[to].piece = board_white_knight;

        /*------------------------------------------------------+
         |      EXTRA: Swap to bring all knights up front       |
         +------------------------------------------------------*/

        for (int knight=1; knight<index; knight++) {
                int sq = bd->current->passive.pieces[knight];
                if (bd->squares[sq].piece == board_white_knight) {
                        continue;
                }

                /*
                 *  A non-knight is in the piece_list before current index.
                 *  We must swap 'knight' with 'index' to restore the
                 *  invariant.
                 */
                PUSH_UNDO(bd->current, sq);
                bd->squares[to].index = knight;
                bd->squares[sq].index = index;

                bd->current->passive.pieces[knight] = to;
                bd->current->passive.pieces[index] = sq;

                index = knight;
                break;
        }

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
         *  Clear last_rank_pawn bit
         */

        bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(from);

        /*
         *  Update board
         */

        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*
         *  Leave extra mark in undo[] that indicates the promotion piece.
         *
         *  This info is only used when we try to reconstruct moves from the
         *  stack. The main reason for doing this in such a complicated way is
         *  that we don't want to spend a single CPU cycle to store the move,
         *  because all the info to reconstruct the move is already present
         *  in the undo stack anyway and we don't need the move in the
         *  critical part of the search. All info is there already, except the
         *  promotion piece. So we only do some extra work for promotions.
         *
         *  We further assume that promotions don't use up all space in
         *  undo[], so we write past the last entry. This holds, because
         *  the worst case for undo[] is a king capture, not a promotion.
         */
        assert(bd->current->undo_len < BOARD_UNDO_LEN_MAX);
        bd->current->undo[ bd->current->undo_len ].piece.piece = bd->squares[to].piece;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |      promote_black_queen                                             |
 +----------------------------------------------------------------------*/

/*
 *  Promote black pawn to queen
 */
void promote_black_queen(
        struct board *bd,
        int from,
        int to)
{
        int rays;
        int index;

        assert(BOARD_RANK(from) == BOARD_RANK_2);

        /*------------------------------------------------------+
         |      Fix to-square                                   |
         +------------------------------------------------------*/

        to ^= XOR_PROM_QUEEN;

        /*
         *  Repair undo information because board_make_move has used
         *  the wrong `to' square
         */
        bd->current->undo[UNDO_TO].square = to;
        bd->current->undo[UNDO_TO].piece = bd->squares[to];

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_pawn][from] ^
                data_zobrist[zobrist_black_queen][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_pawn][from];

        bd->current->material_key +=
                data_material_key[board_black_queen] -
                data_material_key[board_black_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        if (BOARD_FILE(to) == BOARD_FILE(from)) { // NO CAPTURE

                /*
                 *  Add missing piece attacks originating from this square
                 */

                rays = (bd->current->passive.attacks[to] ^ BOARD_ATTACK_QUEEN) &
                        data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->passive, bd, to, rays);
                }

                /*
                 *  Block any attacks passing through this square
                 */
                rays = bd->current->active.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->active, bd, to, rays);
                }

        } else {

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

                capture_take_piece_fn *take_piece;

                take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
                assert(take_piece != null);
                take_piece(bd, to);

                /*
                 *  Add piece attacks originating from this square
                 */
                attack_xor_rays(&bd->current->passive, bd, to,
                        BOARD_ATTACK_QUEEN & data_kingtab[to]);
        }

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update board
         */

        bd->squares[to].index = bd->squares[from].index;
        bd->squares[to].piece = board_black_queen;

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
         *  Clear last_rank_pawn bit
         */

        bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(from);

        /*
         *  Update board
         */

        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*
         *  Leave extra mark in undo[] that indicates the promotion piece.
         *
         *  This info is only used when we try to reconstruct moves from the
         *  stack. The main reason for doing this in such a complicated way is
         *  that we don't want to spend a single CPU cycle to store the move,
         *  because all the info to reconstruct the move is already present
         *  in the undo stack anyway and we don't need the move in the
         *  critical part of the search. All info is there already, except the
         *  promotion piece. So we only do some extra work for promotions.
         *
         *  We further assume that promotions don't use up all space in
         *  undo[], so we write past the last entry. This holds, because
         *  the worst case for undo[] is a king capture, not a promotion.
         */
        assert(bd->current->undo_len < BOARD_UNDO_LEN_MAX);
        bd->current->undo[ bd->current->undo_len ].piece.piece = bd->squares[to].piece;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |      promote_black_rook                                              |
 +----------------------------------------------------------------------*/

/*
 *  Promote black pawn to rook
 */
void promote_black_rook(
        struct board *bd,
        int from,
        int to)
{
        int rays;
        int index;

        assert(BOARD_RANK(from) == BOARD_RANK_2);

        /*------------------------------------------------------+
         |      Fix to-square                                   |
         +------------------------------------------------------*/

        to ^= XOR_PROM_ROOK;

        /*
         *  Repair undo information because board_make_move has used
         *  the wrong `to' square
         */
        bd->current->undo[UNDO_TO].square = to;
        bd->current->undo[UNDO_TO].piece = bd->squares[to];

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_pawn][from] ^
                data_zobrist[zobrist_black_rook][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_pawn][from];

        bd->current->material_key +=
                data_material_key[board_black_rook] -
                data_material_key[board_black_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        if (BOARD_FILE(to) == BOARD_FILE(from)) { // NO CAPTURE

                /*
                 *  Add missing piece attacks originating from this square
                 */

                rays = (bd->current->passive.attacks[to] ^ BOARD_ATTACK_ROOK) &
                        data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->passive, bd, to, rays);
                }

                /*
                 *  Block any attacks passing through this square
                 */
                rays = bd->current->active.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->active, bd, to, rays);
                }

        } else {

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

                capture_take_piece_fn *take_piece;

                take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
                assert(take_piece != null);
                take_piece(bd, to);

                /*
                 *  Add piece attacks originating from this square
                 */
                attack_xor_rays(&bd->current->passive, bd, to,
                        BOARD_ATTACK_ROOK & data_kingtab[to]);
        }

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update board
         */

        bd->squares[to].index = bd->squares[from].index;
        bd->squares[to].piece = board_black_rook;

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
         *  Clear last_rank_pawn bit
         */

        bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(from);

        /*
         *  Update board
         */

        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*
         *  Leave extra mark in undo[] that indicates the promotion piece.
         *
         *  This info is only used when we try to reconstruct moves from the
         *  stack. The main reason for doing this in such a complicated way is
         *  that we don't want to spend a single CPU cycle to store the move,
         *  because all the info to reconstruct the move is already present
         *  in the undo stack anyway and we don't need the move in the
         *  critical part of the search. All info is there already, except the
         *  promotion piece. So we only do some extra work for promotions.
         *
         *  We further assume that promotions don't use up all space in
         *  undo[], so we write past the last entry. This holds, because
         *  the worst case for undo[] is a king capture, not a promotion.
         */
        assert(bd->current->undo_len < BOARD_UNDO_LEN_MAX);
        bd->current->undo[ bd->current->undo_len ].piece.piece = bd->squares[to].piece;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |      promote_black_bishop                                            |
 +----------------------------------------------------------------------*/

/*
 *  Promote black pawn to bishop
 */
void promote_black_bishop(
        struct board *bd,
        int from,
        int to)
{
        int rays;
        int index;

        assert(BOARD_RANK(from) == BOARD_RANK_2);

        /*------------------------------------------------------+
         |      Fix to-square                                   |
         +------------------------------------------------------*/

        to ^= XOR_PROM_BISHOP;

        /*
         *  Repair undo information because board_make_move has used
         *  the wrong `to' square
         */
        bd->current->undo[UNDO_TO].square = to;
        bd->current->undo[UNDO_TO].piece = bd->squares[to];

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_pawn][from] ^
                data_zobrist[zobrist_black_bishop][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_pawn][from];

        bd->current->material_key +=
                data_material_key[ BOARD_SQUARE_IS_LIGHT(to) ?
                        board_black_bishop_light :
                        board_black_bishop_dark ] -
                data_material_key[board_black_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        if (BOARD_FILE(to) == BOARD_FILE(from)) { // NO CAPTURE

                /*
                 *  Add missing piece attacks originating from this square
                 */

                rays = (bd->current->passive.attacks[to] ^ BOARD_ATTACK_BISHOP) &
                        data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->passive, bd, to, rays);
                }

                /*
                 *  Block any attacks passing through this square
                 */
                rays = bd->current->active.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->active, bd, to, rays);
                }

        } else {

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

                capture_take_piece_fn *take_piece;

                take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
                assert(take_piece != null);
                take_piece(bd, to);

                /*
                 *  Add piece attacks originating from this square
                 */
                attack_xor_rays(&bd->current->passive, bd, to,
                        BOARD_ATTACK_BISHOP & data_kingtab[to]);
        }

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update bishop diagonal mask
         */
        bd->current->passive.bishop_diagonals ^= data_bishop_diagonals[to];

        /*
         *  Update board
         */

        bd->squares[to].index = bd->squares[from].index;
        bd->squares[to].piece =
                BOARD_SQUARE_IS_LIGHT(to) ?
                board_black_bishop_light :
                board_black_bishop_dark;

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
         *  Clear last_rank_pawn bit
         */

        bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(from);

        /*
         *  Update board
         */

        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*
         *  Leave extra mark in undo[] that indicates the promotion piece.
         *
         *  This info is only used when we try to reconstruct moves from the
         *  stack. The main reason for doing this in such a complicated way is
         *  that we don't want to spend a single CPU cycle to store the move,
         *  because all the info to reconstruct the move is already present
         *  in the undo stack anyway and we don't need the move in the
         *  critical part of the search. All info is there already, except the
         *  promotion piece. So we only do some extra work for promotions.
         *
         *  We further assume that promotions don't use up all space in
         *  undo[], so we write past the last entry. This holds, because
         *  the worst case for undo[] is a king capture, not a promotion.
         */
        assert(bd->current->undo_len < BOARD_UNDO_LEN_MAX);
        bd->current->undo[ bd->current->undo_len ].piece.piece = bd->squares[to].piece;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |      promote_black_knight                                            |
 +----------------------------------------------------------------------*/

/*
 *  Promote black pawn to knight
 */
void promote_black_knight(
        struct board *bd,
        int from,
        int to)
{
        int rays;
        int index;

        assert(BOARD_RANK(from) == BOARD_RANK_2);

        /*------------------------------------------------------+
         |      Fix to-square                                   |
         +------------------------------------------------------*/

        to ^= XOR_PROM_KNIGHT;

        /*
         *  Repair undo information because board_make_move has used
         *  the wrong `to' square
         */
        bd->current->undo[UNDO_TO].square = to;
        bd->current->undo[UNDO_TO].piece = bd->squares[to];

        /*------------------------------------------------------+
         |      Update Zobrist hashes and material key          |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_pawn][from] ^
                data_zobrist[zobrist_black_knight][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_pawn][from];

        bd->current->material_key +=
                data_material_key[board_black_knight] -
                data_material_key[board_black_pawn];

        /*------------------------------------------------------+
         |      Occupy to-square                                |
         +------------------------------------------------------*/

        if (BOARD_FILE(to) == BOARD_FILE(from)) { // NO CAPTURE

                /*
                 *  Add missing piece attacks originating from this square
                 */

                rays = bd->current->passive.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->passive, bd, to, rays);
                }

                /*
                 *  Block any attacks passing through this square
                 */
                rays = bd->current->active.attacks[to] & data_kingtab[to];
                if (rays != 0) {
                        attack_xor_rays(&bd->current->active, bd, to, rays);
                }
        } else {

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

                capture_take_piece_fn *take_piece;

                take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
                assert(take_piece != null);
                take_piece(bd, to);
        }

        /*
         *  Add piece attacks originating from this square
         */
        attack_add_knight(&bd->current->passive, to);

        /*
         *  Update piece list
         */

        index = bd->squares[from].index;
        bd->current->passive.pieces[index] = to;

        /*
         *  Update board
         */

        bd->squares[to].index = bd->squares[from].index;
        bd->squares[to].piece = board_black_knight;

        /*------------------------------------------------------+
         |      EXTRA: Swap to bring all knights up front       |
         +------------------------------------------------------*/

        for (int knight=1; knight<index; knight++) {
                int sq = bd->current->passive.pieces[knight];
                if (bd->squares[sq].piece == board_black_knight) {
                        continue;
                }

                /*
                 *  A non-knight is in the piece_list before current index.
                 *  We must swap 'knight' with 'index' to restore the
                 *  invariant.
                 */
                PUSH_UNDO(bd->current, sq);
                bd->squares[to].index = knight;
                bd->squares[sq].index = index;

                bd->current->passive.pieces[knight] = to;
                bd->current->passive.pieces[index] = sq;

                index = knight;
                break;
        }

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
         *  Clear last_rank_pawn bit
         */

        bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(from);

        /*
         *  Update board
         */

        bd->squares[from].piece = board_empty;
        bd->squares[from].index = 0;

        /*
         *  Leave extra mark in undo[] that indicates the promotion piece.
         *
         *  This info is only used when we try to reconstruct moves from the
         *  stack. The main reason for doing this in such a complicated way is
         *  that we don't want to spend a single CPU cycle to store the move,
         *  because all the info to reconstruct the move is already present
         *  in the undo stack anyway and we don't need the move in the
         *  critical part of the search. All info is there already, except the
         *  promotion piece. So we only do some extra work for promotions.
         *
         *  We further assume that promotions don't use up all space in
         *  undo[], so we write past the last entry. This holds, because
         *  the worst case for undo[] is a king capture, not a promotion.
         */
        assert(bd->current->undo_len < BOARD_UNDO_LEN_MAX);
        bd->current->undo[ bd->current->undo_len ].piece.piece = bd->squares[to].piece;

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

