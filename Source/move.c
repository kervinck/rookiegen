
/*----------------------------------------------------------------------+
 |                                                                      |
 |      move.c -- Move making functions for regular moves               |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Move making functions for regular moves
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
#include "move.h"

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

static inline make_move_fn move_king_generic;
static inline make_move_fn move_queen_generic;
static inline make_move_fn move_rook_generic;
static inline make_move_fn move_bishop_generic;
static inline make_move_fn move_knight_generic;

/*----------------------------------------------------------------------+
 |      board_make_move                                                 |
 +----------------------------------------------------------------------*/

/*
 *  Make any generated move
 */
void board_make_move(struct board *bd, const union board_move *move)
{
        assert(board_current_ply(bd) < BOARD_MAX_DEPTH);

        struct board_stack_frame *frame = bd->current;

        /*
         *  Create new stack_frame, copy and swap active & passive data
         */
        frame[+1].passive = frame[0].active;
        frame[+1].active = frame[0].passive;

        /*
         *  Prepare hash and material key for new position
         */
        frame[+1].pawn_king_hash = frame[0].pawn_king_hash;
        frame[+1].material_key = frame[0].material_key;

        frame[+1].halfmove_clock = 0;

        frame++;
        bd->current = frame;

        /*
         *  Update 64-bit node counter for this level
         */
        frame->node_counter++;

        /*
         *  Get move details
         */
        int from = MOVE_FROM(move->bm.move);
        assert(BOARD_SQUARE_IS_VALID(from));

        int to = MOVE_TO(move->bm.move);
        assert(BOARD_SQUARE_IS_VALID(to));

        make_move_fn* make = (make_move_fn*) move->bm.make;

        /*
         *  Undo information (preliminary)
         */
        frame->undo[UNDO_FROM].square = from;
        frame->undo[UNDO_FROM].piece = bd->squares[from];
        frame->undo[UNDO_TO].square = to;
        frame->undo[UNDO_TO].piece = bd->squares[to];
        frame->undo_len = 2;

        /*
         *  Transfer control to the specialized move making function
         */
        make(bd, from, to);

        /*
         *  Confirm that the move is indeed legal
         */
        assert(frame->active.attacks[frame->passive.pieces[0]] == 0);

        /*
         *  Check halfmove logic
         */
        assert((frame->halfmove_clock == 0) ||
                (frame->halfmove_clock == frame[-1].halfmove_clock + 1));
}

/*----------------------------------------------------------------------+
 |      board_undo_move                                                 |
 +----------------------------------------------------------------------*/

/*
 *  Undo move
 */
void board_undo_move(struct board *bd)
{
        struct board_stack_frame *frame = bd->current;

        /*
         *  Restore board changes
         *  (this loop is unrolled to match typical values of frame->undo_len)
         */

        int undo_len = frame->undo_len;

        assert(undo_len >= 2);
        assert(undo_len <= BOARD_UNDO_LEN_MAX);

        bd->squares[ frame->undo[0].square ] = frame->undo[0].piece;
        bd->squares[ frame->undo[1].square ] = frame->undo[1].piece;
        if (undo_len > 2) {
                bd->squares[ frame->undo[2].square ] = frame->undo[2].piece;
                for (int i=3; i<undo_len; i++) {
                        bd->squares[ frame->undo[i].square ] =
                                frame->undo[i].piece;
                }
        }

        /*
         *  Pop one stack frame
         */
        frame = &frame[-1];
        bd->current = frame;
}

/*----------------------------------------------------------------------+
 |      board_make_null_move                                            |
 +----------------------------------------------------------------------*/

/*
 *  Make the null move
 */
void board_make_null_move(struct board *bd)
{
        assert(board_current_ply(bd) < BOARD_MAX_DEPTH);

        struct board_stack_frame *frame = bd->current;

        /*
         *  Create new stack_frame, copy and swap active & passive data
         */
        frame[+1].passive = frame[0].active;
        frame[+1].active = frame[0].passive;

        /*
         *  Prepare hash and material key for new position
         */
        frame[+1].board_hash_lazy = ~frame[0].board_hash_lazy;
        frame[+1].pawn_king_hash = frame[0].pawn_king_hash;
        frame[+1].material_key = frame[0].material_key;

        //frame[+1].halfmove_clock = 0; // null move is irreversible
        frame[+1].halfmove_clock = 1; // null move is reversible for 1 ply only
        /*
         *  But we should not reset halfmove_clock to 0, because that
         *  can then be seen as a conversion. Null move is not a conversion.    
         *  If we set to 1, we accept wrong repetitions however.
         *  We have given White an extra line that otherwise doesn't exist.
         *      <null>, Ra1-b1, <reply>, Rb1-c1, <retract>, Rc1-a1 'draw'?
         *  Is this bad? For it to make an effect, alpha must be < 0
         *  and White must be losing. If White loses a tempo to get this
         *  false draw, Black must have nothing better. So this trick is
         *  probably ok.
         */

        frame++;
        bd->current = frame;

        /*
         *  Update 64-bit node counter for this level
         */
        frame->node_counter++;

        /*
         *  No undo information needed
         */
        frame->undo_len = 0;

        /*
         *  Confirm that the move is indeed legal
         */
        assert(frame->active.attacks[frame->passive.pieces[0]] == 0);
}

/*----------------------------------------------------------------------+
 |      board_undo_null_move                                            |
 +----------------------------------------------------------------------*/

/*
 *  Undo the null move
 */
void board_undo_null_move(struct board *bd)
{
        struct board_stack_frame *frame = bd->current;

        /*
         *  Consistency check
         */
        assert(frame->undo_len == 0);

        /*
         *  Pop one stack frame
         */
        frame = &frame[-1];
        bd->current = frame;
}

/*----------------------------------------------------------------------+
 |      move_white_king_castle, move_black_king_castle, move_king       |
 +----------------------------------------------------------------------*/

/*
 *  Move white king with castling rights
 */
void move_white_king_castle(struct board *bd, int from, int to)
{
        unsigned long long delta_hash = 0;

        /*
         *  Disable all castling rights
         */
        bd->squares[E1].piece = board_white_king;

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
         *  Rest is as a regular king move
         */
        move_white_king(bd, from, to);

        bd->current->board_hash_lazy ^= delta_hash; // must apply delta_hash AFTER because move function sets it
}

/*----------------------------------------------------------------------*/

/*
 *  Move black king with castling rights
 */
void move_black_king_castle(struct board *bd, int from, int to)
{
        unsigned long long delta_hash = 0;

        /*
         *  Disable all castling rights
         */
        bd->squares[E8].piece = board_black_king;

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
         *  Rest is as a regular king move
         */
        move_black_king(bd, from, to);

        bd->current->board_hash_lazy ^= delta_hash; // must apply delta_hash AFTER because move function sets it
}

/*----------------------------------------------------------------------*/

/*
 *  Move king
 */

void move_white_king(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_king][from] ^
                data_zobrist[zobrist_white_king][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_king][from] ^
                data_zobrist[zobrist_white_king][to];

        /*
         *  The rest is generic (tail call)
         */
        move_king_generic(bd, from, to);
}

/*----------------------------------------------------------------------*/

void move_black_king(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_king][from] ^
                data_zobrist[zobrist_black_king][to];

        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_king][from] ^
                data_zobrist[zobrist_black_king][to];

        /*
         *  The rest is generic (tail call)
         */
        move_king_generic(bd, from, to);
}

/*----------------------------------------------------------------------*/

static inline
void move_king_generic(struct board *bd, int from, int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        attack_xor_king(&bd->current->passive, from);

        /*------------------------------------------------------+
         |      Withdraw attacks from target square             |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, to, rays);
        }

        rays = bd->current->active.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, to, rays);
        }

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
         |      Update halfmove clock                           |
         +------------------------------------------------------*/

        bd->current->halfmove_clock = bd->current[-1].halfmove_clock + 1;
}

/*----------------------------------------------------------------------+
 |      move_white_queen, move_black_queen, move_queen_generic          |
 +----------------------------------------------------------------------*/

/*
 *  Move queen
 */

void move_white_queen(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_queen][from] ^
                data_zobrist[zobrist_white_queen][to];

        /*
         *  The rest is generic (tail call)
         */
        move_queen_generic(bd, from, to);
}

/*----------------------------------------------------------------------*/

void move_black_queen(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_queen][from] ^
                data_zobrist[zobrist_black_queen][to];

        /*
         *  The rest is generic (tail call)
         */
        move_queen_generic(bd, from, to);
}

/*----------------------------------------------------------------------*/

static inline
void move_queen_generic(struct board *bd, int from, int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Withdraw attacks from target square and         |
         |      update piece attacks on new square              |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[to] ^ BOARD_ATTACK_QUEEN;
        rays &= data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, to, rays);
        }

        rays = bd->current->active.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, to, rays);
        }

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
         |      Update halfmove clock                           |
         +------------------------------------------------------*/

        bd->current->halfmove_clock = bd->current[-1].halfmove_clock + 1;
}

/*----------------------------------------------------------------------+
 |      move_white_rook_castle, move_black_rook_castle,                 |
 |      move_white_rook, move_black_rook, move_rook_generic             |
 +----------------------------------------------------------------------*/

/*
 *  Move white rook with castling rights
 */
void move_white_rook_castle(struct board *bd, int from, int to)
{
        /*
         *  Transform castling-enabled rook into standard rook
         */
        bd->squares[from].piece = board_white_rook;

        /*
         *  If we can't castle to the other side anymore, then
         *  also transform the king
         */
        assert((from == A1) || (from == H1));
        int other = (from == A1) ? H1 : A1;
        if (bd->squares[other].piece != board_white_rook_castle) {
                PUSH_UNDO(bd->current, E1);
                bd->squares[E1].piece = board_white_king;
        }

        /*
         *  Rest is as a regular rook move
         */
        move_white_rook(bd, from, to);

        bd->current->board_hash_lazy ^=
                data_zobrist[zobrist_white_rook][from] ^
                data_zobrist[zobrist_white_rook_castle][from];
        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_white_rook_castle][from];
}

/*----------------------------------------------------------------------*/

/*
 *  Move black rook with castling rights
 */
void move_black_rook_castle(struct board *bd, int from, int to)
{
        /*
         *  Transform castling-enabled rook into standard rook
         */
        bd->squares[from].piece = board_black_rook;

        /*
         *  If we can't castle to the other side anymore, then
         *  also transform the king
         */
        assert((from == A8) || (from == H8));
        int other = (from == A8) ? H8 : A8;
        if (bd->squares[other].piece != board_black_rook_castle) {
                PUSH_UNDO(bd->current, E8);
                bd->squares[E8].piece = board_black_king;
        }

        /*
         *  Rest is as a regular rook move
         */
        move_black_rook(bd, from, to);

        bd->current->board_hash_lazy ^=
                data_zobrist[zobrist_black_rook][from] ^
                data_zobrist[zobrist_black_rook_castle][from];
        bd->current->pawn_king_hash ^=
                data_zobrist[zobrist_black_rook_castle][from];
}

/*----------------------------------------------------------------------*/

/*
 *  Move rook
 */

void move_white_rook(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_rook][from] ^
                data_zobrist[zobrist_white_rook][to];

        /*
         *  The rest is generic (tail call)
         */
        move_rook_generic(bd, from, to);
}

/*----------------------------------------------------------------------*/

void move_black_rook(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_rook][from] ^
                data_zobrist[zobrist_black_rook][to];

        /*
         *  The rest is generic (tail call)
         */
        move_rook_generic(bd, from, to);
}

/*----------------------------------------------------------------------*/

static inline
void move_rook_generic(struct board *bd, int from, int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Withdraw attacks from target square and         |
         |      update piece attacks on new square              |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[to] ^ BOARD_ATTACK_ROOK;
        rays &= data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, to, rays);
        }

        rays = bd->current->active.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, to, rays);
        }

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
         |      Update halfmove clock                           |
         +------------------------------------------------------*/

        bd->current->halfmove_clock = bd->current[-1].halfmove_clock + 1;
}

/*----------------------------------------------------------------------+
 |      move_white_bishop, move_black_bishop, move_bishop_generic       |
 +----------------------------------------------------------------------*/

/*
 *  Move bishop
 */

void move_white_bishop(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_bishop][from] ^
                data_zobrist[zobrist_white_bishop][to];

        /*
         *  The rest is generic (tail call)
         */
        move_bishop_generic(bd, from, to);
}

/*----------------------------------------------------------------------*/

void move_black_bishop(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_bishop][from] ^
                data_zobrist[zobrist_black_bishop][to];

        /*
         *  The rest is generic (tail call)
         */
        move_bishop_generic(bd, from, to);
}

/*----------------------------------------------------------------------*/

static inline
void move_bishop_generic(struct board *bd, int from, int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Withdraw attacks from target square and         |
         |      update piece attacks on new square              |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[to] ^ BOARD_ATTACK_BISHOP;
        rays &= data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, to, rays);
        }

        rays = bd->current->active.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, to, rays);
        }

        /*------------------------------------------------------+
         |      Update bishop diagonal bits                     |
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
         |      Update halfmove clock                           |
         +------------------------------------------------------*/

        bd->current->halfmove_clock = bd->current[-1].halfmove_clock + 1;
}

/*----------------------------------------------------------------------+
 |      move_white_knight, move_black_knight, move_knight_generic       |
 +----------------------------------------------------------------------*/

/*
 *  Move knight
 */

void move_white_knight(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_white_knight][from] ^
                data_zobrist[zobrist_white_knight][to];

        /*
         *  The rest is generic (tail call)
         */
        move_knight_generic(bd, from, to);
}

/*----------------------------------------------------------------------*/

void move_black_knight(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Update Zobrist hashes                           |
         +------------------------------------------------------*/

        bd->current->board_hash_lazy =
                ~bd->current[-1].board_hash_lazy ^
                data_zobrist[zobrist_black_knight][from] ^
                data_zobrist[zobrist_black_knight][to];

        /*
         *  The rest is generic (tail call)
         */
        move_knight_generic(bd, from, to);
}
 
/*----------------------------------------------------------------------*/

static inline
void move_knight_generic(struct board *bd, int from, int to)
{
        int rays;

        /*------------------------------------------------------+
         |      Withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        attack_sub_knight(&bd->current->passive, from);

        /*------------------------------------------------------+
         |      Withdraw attacks from target square             |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, to, rays);
        }

        rays = bd->current->active.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, to, rays);
        }

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
         |      Update halfmove clock                           |
         +------------------------------------------------------*/

        bd->current->halfmove_clock = bd->current[-1].halfmove_clock + 1;
}

/*----------------------------------------------------------------------+
 |      move_white_pawn_rank2_to_3, move_white_pawn_rank2_to_4,         |
 |      move_white_pawn                                                 |
 +----------------------------------------------------------------------*/

/*
 *  Move white pawn from rank 2 to rank 3
 */
void move_white_pawn_rank2_to_3(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Specific for board_white_pawn_rank2             |
         +------------------------------------------------------*/

        bd->squares[from].piece = board_white_pawn;

        /*
         *  Rest is as a regular pawn move
         */
        move_white_pawn(bd, from, to);
}

/*----------------------------------------------------------------------*/

/*
 *  Move white pawn from rank 2 to rank 4
 */
void move_white_pawn_rank2_to_4(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Specific for board_white_pawn_rank2             |
         +------------------------------------------------------*/

        bd->squares[from].piece = board_white_pawn;

        bd->current->en_passant_lazy = from + BOARD_VECTOR_NORTH;
        bd->current->en_passant_node_counter = bd->current->node_counter;

        /*
         *  Rest is as a regular pawn move
         */
        move_white_pawn(bd, from, to);
}

/*----------------------------------------------------------------------*/

/*
 *  Move white pawn
 */
void move_white_pawn(struct board *bd, int from, int to)
{
        int rays;

        assert(bd->squares[from].piece == board_white_pawn);

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
         |      Withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        attack_xor_white_pawn(&bd->current->passive, from);

        /*------------------------------------------------------+
         |      Withdraw attacks from target square             |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, to, rays);
        }

        rays = bd->current->active.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, to, rays);
        }

        /*------------------------------------------------------+
         |      Place piece on new square                       |
         +------------------------------------------------------*/

        bd->current->passive.pieces[ bd->squares[from].index ] = to;
        bd->squares[to] = bd->squares[from];

        if (BOARD_RANK(to) == BOARD_RANK_7) {
                bd->squares[to].piece = board_white_pawn_rank7;
                bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(to);
        }

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
}

/*----------------------------------------------------------------------+
 |      move_black_pawn_rank2_to_3, move_black_pawn_rank2_to_4,         |
 |      move_black_pawn                                                 |
 +----------------------------------------------------------------------*/

/*
 *  Move black pawn from rank 7 to rank 6
 */
void move_black_pawn_rank7_to_6(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Specific for board_black_pawn_rank7             |
         +------------------------------------------------------*/

        bd->squares[from].piece = board_black_pawn;

        /*
         *  Rest is as a regular pawn move
         */
        move_black_pawn(bd, from, to);
}

/*----------------------------------------------------------------------*/

/*
 *  Move black pawn from rank 7 to rank 5
 */
void move_black_pawn_rank7_to_5(struct board *bd, int from, int to)
{
        /*------------------------------------------------------+
         |      Specific for board_black_pawn_rank7             |
         +------------------------------------------------------*/

        bd->squares[from].piece = board_black_pawn;

        bd->current->en_passant_lazy = from + BOARD_VECTOR_SOUTH;
        bd->current->en_passant_node_counter = bd->current->node_counter;

        /*
         *  Rest is as a regular pawn move
         */
        move_black_pawn(bd, from, to);
}

/*----------------------------------------------------------------------*/

/*
 *  Move black pawn
 */
void move_black_pawn(struct board *bd, int from, int to)
{
        int rays;

        assert(bd->squares[from].piece == board_black_pawn);

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
         |      Withdraw piece attacks from original square     |
         +------------------------------------------------------*/

        attack_xor_black_pawn(&bd->current->passive, from);

        /*------------------------------------------------------+
         |      Withdraw attacks from target square             |
         +------------------------------------------------------*/

        rays = bd->current->passive.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->passive, bd, to, rays);
        }

        rays = bd->current->active.attacks[to] & data_kingtab[to];
        if (rays != 0) {
                attack_xor_rays(&bd->current->active, bd, to, rays);
        }

        /*------------------------------------------------------+
         |      Place piece on new square                       |
         +------------------------------------------------------*/

        bd->current->passive.pieces[ bd->squares[from].index ] = to;
        bd->squares[to] = bd->squares[from];

        if (BOARD_RANK(to) == BOARD_RANK_2) {
                bd->squares[to].piece = board_black_pawn_rank2;
                bd->current->passive.last_rank_pawns ^= 1 << BOARD_FILE(to);
        }

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
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

