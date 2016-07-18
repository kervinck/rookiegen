
/*----------------------------------------------------------------------+
 |                                                                      |
 |      capture.c -- Move making functions for captures                 |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Move making functions for captures
 *
 *  History:
 *      2008-01-12 (marcelk) Tidy up original draft version.
 *      2008-01-13 (marcelk) Added Zobrist hashing.
 *      2008-01-29 (marcelk) Fixed light/dark bug in capture_take_piece_fn_table
 *      2008-02-16 (marcelk) Cleanup interface with evaluate
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
#include "capture.h"

/*
 *  Other includes
 */
#include "attack.h"

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Functions to remove pieces from attack board and piece list
 */
static capture_take_piece_fn take_white_queen;
static capture_take_piece_fn take_black_queen;
static inline capture_take_piece_fn take_queen_generic;

static capture_take_piece_fn take_white_rook;
static capture_take_piece_fn take_black_rook;
static capture_take_piece_fn take_white_rook_castle;
static capture_take_piece_fn take_black_rook_castle;
static inline capture_take_piece_fn take_rook_generic;

static capture_take_piece_fn take_white_bishop_light;
static capture_take_piece_fn take_white_bishop_dark;
static capture_take_piece_fn take_black_bishop_light;
static capture_take_piece_fn take_black_bishop_dark;
static inline capture_take_piece_fn take_bishop_generic;

static capture_take_piece_fn take_white_knight;
static capture_take_piece_fn take_black_knight;
static inline capture_take_piece_fn take_knight_generic;

static capture_take_piece_fn take_white_pawn;
static capture_take_piece_fn take_white_pawn_rank7;

static capture_take_piece_fn take_black_pawn;
static capture_take_piece_fn take_black_pawn_rank2;

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

/*
 *  Lookup table to find the appropriate take_xxx function
 */
capture_take_piece_fn * const capture_take_piece_fn_table[] = {
        [board_white_queen] = take_white_queen,
        [board_black_queen] = take_black_queen,

        [board_white_rook] = take_white_rook,
        [board_black_rook] = take_black_rook,
        [board_white_rook_castle] = take_white_rook_castle,
        [board_black_rook_castle] = take_black_rook_castle,

        [board_white_bishop_light] = take_white_bishop_light,
        [board_white_bishop_dark] = take_white_bishop_dark,
        [board_black_bishop_light] = take_black_bishop_light,
        [board_black_bishop_dark] = take_black_bishop_dark,

        [board_white_knight] = take_white_knight,
        [board_black_knight] = take_black_knight,

        [board_white_pawn] = take_white_pawn,
        [board_white_pawn_rank2] = take_white_pawn,
        [board_white_pawn_rank7] = take_white_pawn_rank7,

        [board_black_pawn] = take_black_pawn,
        [board_black_pawn_rank7] = take_black_pawn,
        [board_black_pawn_rank2] = take_black_pawn_rank2,
};

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      take_white_queen, take_black_queen, take_queen_generic          |
 +----------------------------------------------------------------------*/

/*
 *  Remove a queen from the attack board, piece list and keys
 */

static
void take_white_queen(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_white_queen][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_WHITE_QUEEN;

        /*
         *  The rest is generic. (Tail call)
         */
        take_queen_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

static
void take_black_queen(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_black_queen][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_BLACK_QUEEN;

        /*
         *  The rest is generic. (Tail call)
         */
        take_queen_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

/*
 *  Generic part. Only to be called by the specialized functions
 */
static
void take_queen_generic(struct board *bd, int sq)
{
        /*
         *  Remove attacks
         */
        attack_xor_rays(&bd->current->active, bd, sq,
                data_kingtab[sq] & BOARD_ATTACK_QUEEN);

        /*
         *  Remove from piece list
         */
        int last_piece = bd->current->active.nr_pieces - 1;
        assert((1 <= last_piece) && (last_piece < BOARD_SIDE_MAX_PIECES));

        int other = bd->current->active.pieces[last_piece];
        assert(BOARD_SQUARE_IS_VALID(other));

        // @TODO: benchmark conditional
        //if (other != sq) {

        PUSH_UNDO(bd->current, other);

        int index = bd->squares[sq].index;
        assert((0 <= index) && (index <= last_piece));

        bd->squares[other].index = index;
        bd->current->active.pieces[index] = other;

        bd->current->active.pieces[last_piece] = -1;
        bd->current->active.nr_pieces = last_piece;

        /*
         *  Note: Do not clear the square. The outside caller already does that.
         */
}

/*----------------------------------------------------------------------+
 |      take_white_rook, take_white_rook_castle,                        |
 |      take_black_rook, take_white_rook_castle, take_rook_generic      |
 +----------------------------------------------------------------------*/

/*
 *  Remove a rook from the attack board, piece list and keys
 */

static
void take_white_rook(struct board *bd, int sq)
{
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_white_rook][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_WHITE_ROOK;

        /*
         *  The rest is generic. (Tail call)
         */
        take_rook_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

/*
 *  Remove a white rook with castling rights from the attack board
 *  and piece list
 */
static
void take_white_rook_castle(struct board *bd, int sq)
{
        assert((sq == A1) || (sq == H1));
        int other = (sq == A1) ? H1 : A1;
        assert(bd->squares[E1].piece == board_white_king_castle);
        if (bd->squares[other].piece != board_white_rook_castle) {
                PUSH_UNDO(bd->current, E1);
                bd->squares[E1].piece = board_white_king;
        }

        /* Note: a rook_castle uses the Zobrist keys for 'pawn' */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_white_rook_castle][sq];
        bd->current->pawn_king_hash ^= data_zobrist[zobrist_white_rook_castle][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_WHITE_ROOK;

        /*
         *  The rest is generic. (Tail call)
         */
        take_rook_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

static
void take_black_rook(struct board *bd, int sq)
{
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_black_rook][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_BLACK_ROOK;

        /*
         *  The rest is generic. (Tail call)
         */
        take_rook_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

/*
 *  Remove a black rook with castling rights from the attack board
 *  and piece list
 */
static
void take_black_rook_castle(struct board *bd, int sq)
{
        assert((sq == A8) || (sq == H8));
        int other = (sq == A8) ? H8 : A8;
        assert(bd->squares[E8].piece == board_black_king_castle);
        if (bd->squares[other].piece != board_black_rook_castle) {
                PUSH_UNDO(bd->current, E8);
                bd->squares[E8].piece = board_black_king;
        }

        /* Note: a rook_castle uses the Zobrist keys for 'pawn' */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_black_rook_castle][sq];
        bd->current->pawn_king_hash ^= data_zobrist[zobrist_black_rook_castle][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_BLACK_ROOK;

        /*
         *  The rest is generic. (Tail call)
         */
        take_rook_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

/*
 *  Generic part. Only to be called by the specialized functions
 */
static
void take_rook_generic(struct board *bd, int sq)
{
        /*
         *  Remove attacks
         */
        attack_xor_rays(&bd->current->active, bd, sq,
                data_kingtab[sq] & BOARD_ATTACK_ROOK);

        /*
         *  Remove from piece list
         */
        int last_piece = bd->current->active.nr_pieces - 1;
        assert((1 <= last_piece) && (last_piece < BOARD_SIDE_MAX_PIECES));

        int other = bd->current->active.pieces[last_piece];
        assert(BOARD_SQUARE_IS_VALID(other));

        // @TODO: benchmark conditional
        //if (other != sq) {

        PUSH_UNDO(bd->current, other);

        int index = bd->squares[sq].index;
        assert((0 <= index) && (index <= last_piece));

        bd->squares[other].index = index;
        bd->current->active.pieces[index] = other;

        bd->current->active.pieces[last_piece] = -1;
        bd->current->active.nr_pieces = last_piece;

        /*
         *  Note: Do not clear the square. Caller does that.
         */
}

/*----------------------------------------------------------------------+
 |      take_white_bishop_light, take_white_bishop_dark,                |
 |      take_black_bishop_light, take_black_bishop_dark,                |
 |      take_bishop_generic                                             |
 +----------------------------------------------------------------------*/

/*
 *  Remove a bishop from the attack board, piece list and keys
 */

static
void take_white_bishop_light(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_white_bishop][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_WHITE_BISHOP_LIGHT;

        /*
         *  The rest is generic. (Tail call)
         */
        take_bishop_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

static
void take_white_bishop_dark(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_white_bishop][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_WHITE_BISHOP_DARK;

        /*
         *  The rest is generic. (Tail call)
         */
        take_bishop_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

static
void take_black_bishop_light(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_black_bishop][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_BLACK_BISHOP_LIGHT;

        /*
         *  The rest is generic. (Tail call)
         */
        take_bishop_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

static
void take_black_bishop_dark(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */

        bd->current->board_hash_lazy ^= data_zobrist[zobrist_black_bishop][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_BLACK_BISHOP_DARK;

        /*
         *  The rest is generic. (Tail call)
         */
        take_bishop_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

/*
 *  Generic part. Only to be called by the specialized functions
 */
static inline
void take_bishop_generic(struct board *bd, int sq)
{
        /*
         *  Remove attacks
         */
        attack_xor_rays(&bd->current->active, bd, sq,
                data_kingtab[sq] & BOARD_ATTACK_BISHOP);

        /*
         *  Remove from piece list
         */
        int last_piece = bd->current->active.nr_pieces - 1;
        assert((1 <= last_piece) && (last_piece < BOARD_SIDE_MAX_PIECES));

        int other = bd->current->active.pieces[last_piece];
        assert(BOARD_SQUARE_IS_VALID(other));

        // @TODO: benchmark conditional
        //if (other != sq) {

        PUSH_UNDO(bd->current, other);

        int index = bd->squares[sq].index;
        assert((0 <= index) && (index <= last_piece));

        bd->squares[other].index = index;
        bd->current->active.pieces[index] = other;

        bd->current->active.pieces[last_piece] = -1;
        bd->current->active.nr_pieces = last_piece;

        bd->current->active.bishop_diagonals ^= data_bishop_diagonals[sq];

        /*
         *  Note: Do not clear the square. Caller does that.
         */
}

/*----------------------------------------------------------------------+
 |      take_white_knight, take_black_knight, take_knight_generic       |
 +----------------------------------------------------------------------*/

/*
 *  Remove a knight from the attack board, piece list and keys
 */

static
void take_white_knight(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_white_knight][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_WHITE_KNIGHT;

        /*
         *  The rest is generic. (Tail call)
         */
        take_knight_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

static
void take_black_knight(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_black_knight][sq];
        bd->current->material_key -= BOARD_MATERIAL_KEY_BLACK_KNIGHT;

        /*
         *  The rest is generic. (Tail call)
         */
        take_knight_generic(bd, sq);
}

/*----------------------------------------------------------------------*/

/*
 *  Generic part. Only to be called by the specialized functions
 *
 *  Extra care is needed to keep all knights together in the piece list.
 *
 *                      last_knight
 *                          |
 *                          v
 *                +---+---+---+---+---+
 *      pieces[]  | K | N | N | x | x | nr_pieces = 5
 *                +---+---+---+---+---+
 *                  0   1   2   3   4
 *                      ^   ^       ^
 *                      |   |       |
 *                   unsafe safe   last_piece
 *                      index
 */
static inline
void take_knight_generic(struct board *bd, int sq)
{
        /*
         *  Remove attacks
         */
        attack_sub_knight(&bd->current->active, sq);

        /*
         *  Remove from piece list
         */
        int last_piece = bd->current->active.nr_pieces - 1;
        assert((1 <= last_piece) && (last_piece < BOARD_SIDE_MAX_PIECES));

        int other = bd->current->active.pieces[last_piece];
        assert(BOARD_SQUARE_IS_VALID(other));

        // @TODO: benchmark conditional
        //if (other != sq) {

        PUSH_UNDO(bd->current, other);

        int index = bd->squares[sq].index;
        assert((0 <= index) && (index <= last_piece));

        /*
         *  -----------------------------------------------------
         *  ALERT: We need special handling of knight captures to
         *  ensure that all knights stay up front in the piece
         *  list (directly after the king)
         *  -----------------------------------------------------
         */

        int last_knight = index;
        // @TODO: finding last knight easier with material counts?
        for (last_knight=index; last_knight<last_piece; last_knight++) {
                /*
                 *  There are more pieces after last_knight,
                 *  so check if it is a knight as well
                 */
                int next_sq = bd->current->active.pieces[last_knight+1];
                int next_piece = bd->squares[next_sq].piece;
                // @TODO: can use a bit set for this test
                if ((next_piece != board_white_knight) &&
                        (next_piece != board_black_knight))
                {
                        /*
                         *  If not we have found the last knight
                         */
                        break;
                }
        }

        /*
         *  If we're capturing something other than the last knight, AND
         *  there are other pieces than knights, then swap the captured
         *  piece with the last knight.
         */
        if ((index < last_knight) && (last_knight < last_piece)) {
                int square_a = bd->current->active.pieces[index];
                int square_b = bd->current->active.pieces[last_knight];

                PUSH_UNDO(bd->current, square_b);

                bd->squares[square_a].index = last_knight;
                bd->squares[square_b].index = index;

                bd->current->active.pieces[last_knight] = square_a;
                bd->current->active.pieces[index] = square_b;

                index = last_knight;
        }

        /*
         *  -----------------------------------------------------
         *  Now index points at the last knight, so it is
         *  safe to swap that with the last piece as normal
         *  -----------------------------------------------------
         */

        bd->squares[other].index = index;
        bd->current->active.pieces[index] = other;

        bd->current->active.pieces[last_piece] = -1;
        bd->current->active.nr_pieces = last_piece;

        /*
         *  Note: Do not clear the square. Caller does that.
         */
}

/*----------------------------------------------------------------------+
 |      take_white_pawn, take_white_pawn_rank7                          |
 +----------------------------------------------------------------------*/

/*
 *  Remove a white pawn from the attack board, piece list and keys
 */

void take_white_pawn_rank7(struct board *bd, int sq)
{
        /*
         *  Update promotion byte
         */
        bd->current->active.last_rank_pawns ^= 1 << BOARD_FILE(sq);

        /*
         *  The rest is the same as the other pawns. (Tail call)
         */
        take_white_pawn(bd, sq);
}

/*----------------------------------------------------------------------*/

static
void take_white_pawn(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_white_pawn][sq];
        bd->current->pawn_king_hash  ^= data_zobrist[zobrist_white_pawn][sq];
        bd->current->material_key    -= BOARD_MATERIAL_KEY_WHITE_PAWN;

        /*
         *  Remove attacks
         */
        attack_xor_white_pawn(&bd->current->active, sq);

        /*
         *  Remove from piece list
         */
        int last_piece = bd->current->active.nr_pieces - 1;
        assert((1 <= last_piece) && (last_piece < BOARD_SIDE_MAX_PIECES));

        int other = bd->current->active.pieces[last_piece];
        assert(BOARD_SQUARE_IS_VALID(other));

        // @TODO: benchmark conditional
        //if (other != sq) {

        PUSH_UNDO(bd->current, other);

        int index = bd->squares[sq].index;
        assert((0 <= index) && (index <= last_piece));

        bd->squares[other].index = index;
        bd->current->active.pieces[index] = other;

        bd->current->active.pieces[last_piece] = -1;
        bd->current->active.nr_pieces = last_piece;

        /*
         *  Note: Do not clear the square. Caller does that.
         */
}

/*----------------------------------------------------------------------+
 |      take_black_pawn, take_black_pawn_rank2                          |
 +----------------------------------------------------------------------*/

/*
 *  Remove a black pawn from the attack board, piece list and keys
 */

static
void take_black_pawn_rank2(struct board *bd, int sq)
{
        /*
         *  Update promotion byte
         */
        bd->current->active.last_rank_pawns ^= 1 << BOARD_FILE(sq);

        /*
         *  The rest is the same as the other pawns. (Tail call)
         */
        take_black_pawn(bd, sq);
}

/*----------------------------------------------------------------------*/

static
void take_black_pawn(struct board *bd, int sq)
{
        /*
         *  Update Zobrist hashes and material key
         */
        bd->current->board_hash_lazy ^= data_zobrist[zobrist_black_pawn][sq];
        bd->current->pawn_king_hash  ^= data_zobrist[zobrist_black_pawn][sq];
        bd->current->material_key    -= BOARD_MATERIAL_KEY_BLACK_PAWN;

        /*
         *  Remove attacks
         */
        attack_xor_black_pawn(&bd->current->active, sq);

        /*
         *  Remove from piece list
         */
        int last_piece = bd->current->active.nr_pieces - 1;
        assert((1 <= last_piece) && (last_piece < BOARD_SIDE_MAX_PIECES));

        int other = bd->current->active.pieces[last_piece];
        assert(BOARD_SQUARE_IS_VALID(other));

        // @TODO: benchmark conditional
        // if (other != sq) { ...

        PUSH_UNDO(bd->current, other);

        int index = bd->squares[sq].index;
        assert((0 <= index) && (index <= last_piece));

        bd->squares[other].index = index;
        bd->current->active.pieces[index] = other;

        bd->current->active.pieces[last_piece] = -1;
        bd->current->active.nr_pieces = last_piece;

        /*
         *  Note: Do not clear the square. Caller does that.
         */
}

/*----------------------------------------------------------------------+
 |      capture_with_white_king_castle, capture_with_black_king_castle, |
 |      capture_with_king, capture_with_queen,                          |
 |      capture_with_white_rook_castle, capture_with_black_rook_castle, |
 |      capture_with_rook, capture_with_bishop, capture_with_knight,    |
 |      capture_with_white_pawn, capture_with_black_pawn                |
 +----------------------------------------------------------------------*/

/*
 *  Capture with a white king with castling rights
 */
void capture_with_white_king_castle(
        struct board *bd,
        int from,
        int to)
{
        unsigned long long delta_hash = 0;

        /*
         *  Disable all castling rights
         */
        assert(from == E1);
        bd->squares[from].piece = board_white_king;

        if (bd->squares[A1].piece == board_white_rook_castle) {
                PUSH_UNDO(bd->current, A1);
                bd->squares[A1].piece = board_white_rook;
                delta_hash ^=
                        data_zobrist[zobrist_white_rook][A1] ^
                        data_zobrist[zobrist_white_rook_castle][A1];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_white_rook_castle][A1];
        }

        if (bd->squares[H1].piece == board_white_rook_castle) {
                PUSH_UNDO(bd->current, H1);
                bd->squares[H1].piece = board_white_rook;
                delta_hash ^=
                        data_zobrist[zobrist_white_rook][H1] ^
                        data_zobrist[zobrist_white_rook_castle][H1];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_white_rook_castle][H1];
        }

        /*
         *  Rest is as a normal king capture
         */
        capture_with_king(bd, from, to);

        bd->current->board_hash_lazy ^= delta_hash;
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a black king with castling rights
 */
void capture_with_black_king_castle(
        struct board *bd,
        int from,
        int to)
{
        unsigned long long delta_hash = 0;

        /*
         *  Disable all castling rights
         */
        assert(from == E8);
        bd->squares[from].piece = board_black_king;

        if (bd->squares[A8].piece == board_black_rook_castle) {
                PUSH_UNDO(bd->current, A8);
                bd->squares[A8].piece = board_black_rook;
                delta_hash ^=
                        data_zobrist[zobrist_black_rook][A8] ^
                        data_zobrist[zobrist_black_rook_castle][A8];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_black_rook_castle][A8];
        }

        if (bd->squares[H8].piece == board_black_rook_castle) {
                PUSH_UNDO(bd->current, H8);
                bd->squares[H8].piece = board_black_rook;
                delta_hash ^=
                        data_zobrist[zobrist_black_rook][H8] ^
                        data_zobrist[zobrist_black_rook_castle][H8];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_black_rook_castle][H8];
        }

        /*
         *  Rest is as a normal king capture
         */
        capture_with_king(bd, from, to);

        bd->current->board_hash_lazy ^= delta_hash;
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a king
 */
void capture_with_king(
        struct board *bd,
        int from,
        int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        if (bd->current->active.color == board_black) {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_white_king][from] ^
                        data_zobrist[zobrist_white_king][to];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_white_king][from] ^
                        data_zobrist[zobrist_white_king][to];
        } else {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_black_king][from] ^
                        data_zobrist[zobrist_black_king][to];
                bd->current->pawn_king_hash ^=
                        data_zobrist[zobrist_black_king][from] ^
                        data_zobrist[zobrist_black_king][to];
        }

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

        capture_take_piece_fn *take_piece;

        take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
        assert(take_piece != null);

        take_piece(bd, to);

        /*------------------------------------------------------+
         |      Withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        attack_xor_king(&bd->current->passive, from);

        /*------------------------------------------------------+
         |      Place piece on new square                       |
         +------------------------------------------------------*/

        bd->current->passive.pieces[ bd->squares[from].index ] = to;
        bd->squares[to] = bd->squares[from];

        /*------------------------------------------------------+
         |      Remove piece from original square               |
         +------------------------------------------------------*/

        bd->squares[from].index = 0;
        bd->squares[from].piece = board_empty;

        /*------------------------------------------------------+
         |      Extend attacks from original square             |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, from, rays);
        }

        rays = bd->current->active.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, from, rays);
        }

        /*------------------------------------------------------+
         |      Extend piece attacks on new square              |
         +------------------------------------------------------*/

        attack_xor_king(&bd->current->passive, to);

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a queen
 */
void capture_with_queen(
        struct board *bd,
        int from,
        int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        if (bd->current->active.color == board_black) {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_white_queen][from] ^
                        data_zobrist[zobrist_white_queen][to];
        } else {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_black_queen][from] ^
                        data_zobrist[zobrist_black_queen][to];
        }

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

        capture_take_piece_fn *take_piece;

        take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
        assert(take_piece != null);

        take_piece(bd, to);

        /*------------------------------------------------------+
         |      Extend piece attacks on new square              |
         +------------------------------------------------------*/

        rays = BOARD_ATTACK_QUEEN & data_kingtab[to];
        assert(rays != 0);

        attack_xor_rays(&bd->current->passive, bd, to, rays);

        /*------------------------------------------------------+
         |      Place piece on new square                       |
         +------------------------------------------------------*/

        bd->current->passive.pieces[bd->squares[from].index] = to;
        bd->squares[to] = bd->squares[from];

        /*------------------------------------------------------+
         |      Remove piece from original square               |
         +------------------------------------------------------*/

        bd->squares[from].index = 0;
        bd->squares[from].piece = board_empty;

        /*------------------------------------------------------+
         |      Extend attacks from original square and         |
         |      withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[from] ^ BOARD_ATTACK_QUEEN;
        rays &= data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, from, rays);
        }

        rays = bd->current->active.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, from, rays);
        }

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a white rook with castling rights
 */
void capture_with_white_rook_castle(
        struct board *bd,
        int from,
        int to)
{
        /*
         *  Transform castling-enabled rook into standard rook
         */
        bd->squares[from].piece = board_white_rook;

        /*
         *  If we can't castle to the other side anymore, then
         *  also transform the king
         */
        int other = (from == A1) ? H1 : A1;
        if (bd->squares[other].piece != board_white_rook_castle) {
                PUSH_UNDO(bd->current, E1);
                bd->squares[E1].piece = board_white_king;
        }

        /*
         *  Rest is as a normal rook capture
         */
        capture_with_rook(bd, from, to);

        bd->current->board_hash_lazy ^=
                data_zobrist[zobrist_white_rook][from] ^
                data_zobrist[zobrist_white_rook_castle][from];
        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_rook_castle][from];
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a black rook with castling rights
 */
void capture_with_black_rook_castle(
        struct board *bd,
        int from,
        int to)
{
        /*
         *  Transform castling-enabled rook into standard rook
         */
        bd->squares[from].piece = board_black_rook;

        /*
         *  If we can't castle to the other side anymore, then
         *  also transform the king
         */
        int other = (from == A8) ? H8 : A8;
        if (bd->squares[other].piece != board_black_rook_castle) {
                PUSH_UNDO(bd->current, E8);
                bd->squares[E8].piece = board_black_king;
        }

        /*
         *  Rest is as a normal rook capture
         */
        capture_with_rook(bd, from, to);

        bd->current->board_hash_lazy ^=
                data_zobrist[zobrist_black_rook][from] ^
                data_zobrist[zobrist_black_rook_castle][from];
        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_rook_castle][from];
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a rook
 */
void capture_with_rook(
        struct board *bd,
        int from,
        int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        if (bd->current->active.color == board_black) {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_white_rook][from] ^
                        data_zobrist[zobrist_white_rook][to];
        } else {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_black_rook][from] ^
                        data_zobrist[zobrist_black_rook][to];
        }

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

        capture_take_piece_fn *take_piece;

        take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
        assert(take_piece != null);

        take_piece(bd, to);

        /*------------------------------------------------------+
         |      Extend piece attacks on new square              |
         +------------------------------------------------------*/

        rays = BOARD_ATTACK_ROOK & data_kingtab[to];
        assert(rays != 0);

        attack_xor_rays(&bd->current->passive, bd, to, rays);

        /*------------------------------------------------------+
         |      Place piece on new square                       |
         +------------------------------------------------------*/

        bd->current->passive.pieces[bd->squares[from].index] = to;
        bd->squares[to] = bd->squares[from];

        /*------------------------------------------------------+
         |      Remove piece from original square               |
         +------------------------------------------------------*/

        bd->squares[from].index = 0;
        bd->squares[from].piece = board_empty;

        /*------------------------------------------------------+
         |      Extend attacks from original square and         |
         |      withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[from] ^ BOARD_ATTACK_ROOK;
        rays &= data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, from, rays);
        }

        rays = bd->current->active.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, from, rays);
        }

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a bishop
 */
void capture_with_bishop(
        struct board *bd,
        int from,
        int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        if (bd->current->active.color == board_black) {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_white_bishop][from] ^
                        data_zobrist[zobrist_white_bishop][to];
        } else {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_black_bishop][from] ^
                        data_zobrist[zobrist_black_bishop][to];
        }

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

        capture_take_piece_fn *take_piece;

        take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
        assert(take_piece != null);

        take_piece(bd, to);

        /*------------------------------------------------------+
         |      Extend piece attacks on new square              |
         +------------------------------------------------------*/

        rays = BOARD_ATTACK_BISHOP & data_kingtab[to];
        assert(rays != 0);

        attack_xor_rays(&bd->current->passive, bd, to, rays);

        /*------------------------------------------------------+
         |      Update bishop_diagonal bits                     |
         +------------------------------------------------------*/

        bd->current->passive.bishop_diagonals ^=
                data_bishop_diagonals[from] ^
                data_bishop_diagonals[to];

        /*------------------------------------------------------+
         |      Place piece on new square                       |
         +------------------------------------------------------*/

        bd->current->passive.pieces[ bd->squares[from].index ] = to;
        bd->squares[to] = bd->squares[from];


        /*------------------------------------------------------+
         |      Remove piece from original square               |
         +------------------------------------------------------*/

        bd->squares[from].index = 0;
        bd->squares[from].piece = board_empty;

        /*------------------------------------------------------+
         |      Extend attacks from original square and         |
         |      withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[from] ^ BOARD_ATTACK_BISHOP;
        rays &= data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, from, rays);
        }

        rays = bd->current->active.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, from, rays);
        }

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a knight
 */
void capture_with_knight(
        struct board *bd,
        int from,
        int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        if (bd->current->active.color == board_black) {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_white_knight][from] ^
                        data_zobrist[zobrist_white_knight][to];
        } else {
                bd->current->board_hash_lazy =
                        ~bd->current[-1].board_hash_lazy ^
                        data_zobrist[zobrist_black_knight][from] ^
                        data_zobrist[zobrist_black_knight][to];
        }

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

        capture_take_piece_fn *take_piece;

        take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
        assert(take_piece != null);

        take_piece(bd, to);

        /*------------------------------------------------------+
         |      Withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        attack_sub_knight(&bd->current->passive, from);

        /*------------------------------------------------------+
         |      Place piece on new square                       |
         +------------------------------------------------------*/

        bd->current->passive.pieces[ bd->squares[from].index ] = to;
        bd->squares[to] = bd->squares[from];

        /*------------------------------------------------------+
         |      Remove piece from original square               |
         +------------------------------------------------------*/

        bd->squares[from].index = 0;
        bd->squares[from].piece = board_empty;

        /*------------------------------------------------------+
         |      Extend attacks from original square             |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, from, rays);
        }

        rays = bd->current->active.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, from, rays);
        }

        /*------------------------------------------------------+
         |      Extend piece attacks on new square              |
         +------------------------------------------------------*/

        attack_add_knight(&bd->current->passive, to);

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a white pawn
 */
void capture_with_white_pawn(
        struct board *bd,
        int from,
        int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_pawn][from] ^
                data_zobrist[zobrist_white_pawn][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_pawn][from] ^
                data_zobrist[zobrist_white_pawn][to];

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

        capture_take_piece_fn *take_piece;

        take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
        assert(take_piece != null);

        take_piece(bd, to);

        /*------------------------------------------------------+
         |      Withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        attack_xor_white_pawn(&bd->current->passive, from);

        /*------------------------------------------------------+
         |      Place piece on new square                       |
         +------------------------------------------------------*/

        bd->current->passive.pieces[ bd->squares[from].index ] = to;

        if (BOARD_RANK(to) == BOARD_RANK_7) {
                bd->squares[to].piece = board_white_pawn_rank7;
                bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(to);
        } else {
                bd->squares[to].piece = board_white_pawn;
        }
        bd->squares[to].index = bd->squares[from].index;

        /*------------------------------------------------------+
         |      Remove piece from original square               |
         +------------------------------------------------------*/

        bd->squares[from].index = 0;
        bd->squares[from].piece = board_empty;

        /*------------------------------------------------------+
         |      Extend attacks from original square             |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, from, rays);
        }

        rays = bd->current->active.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, from, rays);
        }

        /*------------------------------------------------------+
         |      Extend piece attacks on new square              |
         +------------------------------------------------------*/

        attack_xor_white_pawn(&bd->current->passive, to);

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------*/

/*
 *  Capture with a black pawn
 */
void capture_with_black_pawn(
        struct board *bd,
        int from,
        int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_pawn][from] ^
                data_zobrist[zobrist_black_pawn][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_pawn][from] ^
                data_zobrist[zobrist_black_pawn][to];

        /*------------------------------------------------------+
         |      Remove captured piece from attack board         |
         +------------------------------------------------------*/

        capture_take_piece_fn *take_piece;

        take_piece = capture_take_piece_fn_table[bd->squares[to].piece];
        assert(take_piece != null);

        take_piece(bd, to);

        /*------------------------------------------------------+
         |      Withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        attack_xor_black_pawn(&bd->current->passive, from);

        /*------------------------------------------------------+
         |      Place piece on new square                       |
         +------------------------------------------------------*/

        bd->current->passive.pieces[ bd->squares[from].index ] = to;

        if (BOARD_RANK(to) == BOARD_RANK_2) {
                bd->squares[to].piece = board_black_pawn_rank2;
                bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(to);
        } else {
                bd->squares[to].piece = board_black_pawn;
        }
        bd->squares[to].index = bd->squares[from].index;

        /*------------------------------------------------------+
         |      Remove piece from original square               |
         +------------------------------------------------------*/

        bd->squares[from].index = 0;
        bd->squares[from].piece = board_empty;

        /*------------------------------------------------------+
         |      Extend attacks from original square             |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, from, rays);
        }

        rays = bd->current->active.attacks[from] & data_kingtab[from];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, from, rays);
        }

        /*------------------------------------------------------+
         |      Extend piece attacks on new square              |
         +------------------------------------------------------*/

        attack_xor_black_pawn(&bd->current->passive, to);

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

