
/*----------------------------------------------------------------------+
 |                                                                      |
 |      layout.c -- Board layout functions                              |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Board layout functions
 */

/*----------------------------------------------------------------------+
 |      Copyright                                                       |
 +----------------------------------------------------------------------*/

/*
 *  This file is part of the Rookie(TM) Chess Program
 *  Copyright (C)1992-2010, Marcel van Kervinck
 *  All rights reserved.
 */

/*----------------------------------------------------------------------+
 |      Includes                                                        |
 +----------------------------------------------------------------------*/

/*
 *  C standard includes
 */
#include <assert.h>
#include <ctype.h> /* for isdigit */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h> /* for sscanf */
#include <stdlib.h> /* for malloc, free */
#include <string.h> /* for memset */

/*
 *  Base include
 */
#include "cplus.h"

/*
 *  Other module includes
 */

/*
 *  Own interface include
 */
#include "board.h"
#include "intern.h"

/*
 *  Other includes
 */
#include "attack.h"

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

#define ERR_INVALID_EPD "Invalid chess position EPD string"

#define WHITE_HAS_KING_SIDE_CASTLING_CONFIG(bd) (\
        ( (bd->squares[E1].piece == board_white_king) ||\
          (bd->squares[E1].piece == board_white_king_castle)\
        ) && (\
          (bd->squares[H1].piece == board_white_rook) ||\
          (bd->squares[H1].piece == board_white_rook_castle)\
        )\
)

#define WHITE_HAS_QUEEN_SIDE_CASTLING_CONFIG(bd) (\
        ( (bd->squares[E1].piece == board_white_king) ||\
          (bd->squares[E1].piece == board_white_king_castle)\
        ) && (\
          (bd->squares[A1].piece == board_white_rook) ||\
          (bd->squares[A1].piece == board_white_rook_castle)\
        )\
)

#define BLACK_HAS_KING_SIDE_CASTLING_CONFIG(bd) (\
        ( (bd->squares[E8].piece == board_black_king) ||\
          (bd->squares[E8].piece == board_black_king_castle)\
        ) && (\
          (bd->squares[H8].piece == board_black_rook) ||\
          (bd->squares[H8].piece == board_black_rook_castle)\
        )\
)

#define BLACK_HAS_QUEEN_SIDE_CASTLING_CONFIG(bd) (\
        ( (bd->squares[E8].piece == board_black_king) ||\
          (bd->squares[E8].piece == board_black_king_castle)\
        ) && (\
          (bd->squares[A8].piece == board_black_rook) ||\
          (bd->squares[A8].piece == board_black_rook_castle)\
        )\
)

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

const signed char board_vector_step[] = {
        [board_attack_north]     = BOARD_VECTOR_NORTH,
        [board_attack_northeast] = BOARD_VECTOR_NORTHEAST,
        [board_attack_east]      = BOARD_VECTOR_EAST,
        [board_attack_southeast] = BOARD_VECTOR_SOUTHEAST,
        [board_attack_south]     = BOARD_VECTOR_SOUTH,
        [board_attack_southwest] = BOARD_VECTOR_SOUTHWEST,
        [board_attack_west]      = BOARD_VECTOR_WEST,
        [board_attack_northwest] = BOARD_VECTOR_NORTHWEST,
};

const signed char board_vector_step_compact[8] = {
        [ DEBRUIJN_INDEX(board_attack_north)     ] = BOARD_VECTOR_NORTH,
        [ DEBRUIJN_INDEX(board_attack_northeast) ] = BOARD_VECTOR_NORTHEAST,
        [ DEBRUIJN_INDEX(board_attack_east)      ] = BOARD_VECTOR_EAST,
        [ DEBRUIJN_INDEX(board_attack_southeast) ] = BOARD_VECTOR_SOUTHEAST,
        [ DEBRUIJN_INDEX(board_attack_south)     ] = BOARD_VECTOR_SOUTH,
        [ DEBRUIJN_INDEX(board_attack_southwest) ] = BOARD_VECTOR_SOUTHWEST,
        [ DEBRUIJN_INDEX(board_attack_west)      ] = BOARD_VECTOR_WEST,
        [ DEBRUIJN_INDEX(board_attack_northwest) ] = BOARD_VECTOR_NORTHWEST,
};

const signed char board_vector_jump[] = {
        [jump_north_northwest]  = BOARD_VECTOR_NORTH + BOARD_VECTOR_NORTHWEST,
        [jump_north_northeast]  = BOARD_VECTOR_NORTH + BOARD_VECTOR_NORTHEAST,
        [jump_east_northeast]   = BOARD_VECTOR_EAST  + BOARD_VECTOR_NORTHEAST,
        [jump_east_southeast]   = BOARD_VECTOR_EAST  + BOARD_VECTOR_SOUTHEAST,
        [jump_south_southwest]  = BOARD_VECTOR_SOUTH + BOARD_VECTOR_SOUTHWEST,
        [jump_south_southeast]  = BOARD_VECTOR_SOUTH + BOARD_VECTOR_SOUTHEAST,
        [jump_west_northwest]   = BOARD_VECTOR_WEST  + BOARD_VECTOR_NORTHWEST,
        [jump_west_southwest]   = BOARD_VECTOR_WEST  + BOARD_VECTOR_SOUTHWEST,
};

const unsigned long long data_material_key[BOARD_PIECE_TYPES] = {
        [board_white_pawn]              = BOARD_MATERIAL_KEY_WHITE_PAWN,
        [board_white_pawn_rank2]        = BOARD_MATERIAL_KEY_WHITE_PAWN,
        [board_white_pawn_rank7]        = BOARD_MATERIAL_KEY_WHITE_PAWN,
        [board_white_knight]            = BOARD_MATERIAL_KEY_WHITE_KNIGHT,
        [board_white_bishop_light]      = BOARD_MATERIAL_KEY_WHITE_BISHOP_LIGHT,
        [board_white_bishop_dark]       = BOARD_MATERIAL_KEY_WHITE_BISHOP_DARK,
        [board_white_rook]              = BOARD_MATERIAL_KEY_WHITE_ROOK,
        [board_white_rook_castle]       = BOARD_MATERIAL_KEY_WHITE_ROOK,
        [board_white_queen]             = BOARD_MATERIAL_KEY_WHITE_QUEEN,
        [board_black_pawn]              = BOARD_MATERIAL_KEY_BLACK_PAWN,
        [board_black_pawn_rank7]        = BOARD_MATERIAL_KEY_BLACK_PAWN,
        [board_black_pawn_rank2]        = BOARD_MATERIAL_KEY_BLACK_PAWN,
        [board_black_knight]            = BOARD_MATERIAL_KEY_BLACK_KNIGHT,
        [board_black_bishop_light]      = BOARD_MATERIAL_KEY_BLACK_BISHOP_LIGHT,
        [board_black_bishop_dark]       = BOARD_MATERIAL_KEY_BLACK_BISHOP_DARK,
        [board_black_rook]              = BOARD_MATERIAL_KEY_BLACK_ROOK,
        [board_black_rook_castle]       = BOARD_MATERIAL_KEY_BLACK_ROOK,
        [board_black_queen]             = BOARD_MATERIAL_KEY_BLACK_QUEEN,
};

const char board_starting_position_FEN[] =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

static
err_t board_check_side(
        const struct board *bd,
        struct board_side *side,
        int color);

/*----------------------------------------------------------------------*/

static
err_t calc_struct_board_side(
        const struct board *bd,
        struct board_side *side,
        int color);

static
err_t calc_attacks(
        const struct board *bd,
        struct board_side *side,
        int color);

static
err_t calc_zobrist_hashes(
        const struct board *bd,
        unsigned long long *board_hash_lazy_p,
        unsigned long long *pawn_king_hash_p);

static
err_t calc_material_key(
        const struct board *bd,
        unsigned long long *material_key_p);

/*----------------------------------------------------------------------*/

static
int cmp_piece(const void *ap, const void *bp);

static
err_t update_board_after_edit(struct board *bd, int side_to_move);

/*----------------------------------------------------------------------+
 |      board_module_init                                               |
 +----------------------------------------------------------------------*/

/*
 *  Module initialization during startup
 */
void board_module_init(void)
{
        assert(sizeof(struct board_square) == 2);
}

/*----------------------------------------------------------------------+
 |      board_check                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Check the board invariants
 */
err_t board_check(struct board *bd)
{
        err_t err = OK;

        assert((bd->current->active.color == board_white) ||
                (bd->current->active.color == board_black));

        for (int sq=0; sq<BOARD_SIZE; sq++) {
                int piece = bd->squares[sq].piece;
                switch (piece) {
                case board_empty:
                        assert(bd->squares[sq].index == 0);
                        break;
                case board_white_king_castle:
                        assert(sq == E1);
                        assert((bd->squares[A1].piece == board_white_rook_castle) ||
                                (bd->squares[H1].piece == board_white_rook_castle));
                        break;
                case board_black_king_castle:
                        assert(sq == E8);
                        assert((bd->squares[A8].piece == board_black_rook_castle) ||
                                (bd->squares[H8].piece == board_black_rook_castle));
                        break;
                case board_white_rook_castle:
                        assert((sq == A1) || (sq == H1));
                        assert(bd->squares[E1].piece == board_white_king_castle);
                        break;
                case board_black_rook_castle:
                        assert((sq == A8) || (sq == H8));
                        assert(bd->squares[E8].piece == board_black_king_castle);
                        break;
                case board_white_pawn_rank2:
                case board_black_pawn_rank2:
                        assert(BOARD_RANK(sq) == BOARD_RANK_2);
                        break;
                case board_white_pawn_rank7:
                case board_black_pawn_rank7:
                        assert(BOARD_RANK(sq) == BOARD_RANK_7);
                        break;
                case board_white_pawn:
                case board_black_pawn:
                        assert((BOARD_RANK(sq) == BOARD_RANK_3) ||
                                (BOARD_RANK(sq) == BOARD_RANK_4) ||
                                (BOARD_RANK(sq) == BOARD_RANK_5) ||
                                (BOARD_RANK(sq) == BOARD_RANK_6));
                        break;
                case board_white_bishop_light:
                case board_black_bishop_light:
                        assert(((BOARD_RANK(sq) ^ BOARD_FILE(sq)) & 1) == 1);
                        break;
                case board_white_bishop_dark:
                case board_black_bishop_dark:
                        assert(((BOARD_RANK(sq) ^ BOARD_FILE(sq)) & 1) == 0);
                        break;
                case board_white_king:
                case board_black_king:
                case board_white_knight:
                case board_black_knight:
                case board_white_rook:
                case board_black_rook:
                case board_white_queen:
                case board_black_queen:
                        break;
                default:
                        xRaise(ERR_INTERNAL);
                }

                int index = bd->squares[sq].index;
                assert(index >= 0);

                if (bd->current->active.color == BOARD_PIECE_COLOR(piece)) {
                        assert(index < bd->current->active.nr_pieces);
                } else {
                        assert(index < bd->current->passive.nr_pieces);
                }
        }

        /* @TODO: 2008-01-06 (marcelk) check en_passant flag */

        err = board_check_side(bd, &bd->current->active, bd->current->active.color);
        check(err);

        err = board_check_side(bd, &bd->current->passive, !bd->current->active.color);
        check(err);

        /*
         *  Check zobrist hashes and material key
         */
        unsigned long long board_hash_lazy;
        unsigned long long pawn_king_hash;
        unsigned long long material_key;

        err = calc_zobrist_hashes(bd, &board_hash_lazy, &pawn_king_hash);
        check(err);

        err = calc_material_key(bd, &material_key);
        check(err);

        assert(board_hash_lazy == bd->current->board_hash_lazy);
        assert(pawn_king_hash == bd->current->pawn_king_hash);
        assert(material_key == bd->current->material_key);

        /*
         *  Check the sanity of halfmove_clock
         */

        assert((bd->current[ 0].halfmove_clock == 0) ||
                   (bd->current[ 0].halfmove_clock ==
                    bd->current[-1].halfmove_clock + 1));

cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      board_check_side                                                |
 +----------------------------------------------------------------------*/

/*
 *  Check constraints for side data
 */
static
err_t board_check_side(
        const struct board *bd,
        struct board_side *side,
        int color)
{
        err_t err = OK;

        assert(side->nr_pieces > 0);
        assert(side->nr_pieces <= BOARD_SIDE_MAX_PIECES);

        int next_knight = 1; // Index where we expect the next knight, if any

        for (int i=0; i<side->nr_pieces; i++) {
                int sq = side->pieces[i];
                assert((0 <= sq) && (sq < BOARD_SIZE));

                /*
                 *  Check that the indexes and piece color match
                 */
                assert(bd->squares[sq].index == i);
                int piece = bd->squares[sq].piece;
                assert(BOARD_PIECE_COLOR(piece) == color);

                /*
                 *  Check that the king is first
                 */
                if (i == 0) {
                        assert((piece == board_white_king) ||
                                (piece == board_white_king_castle) ||
                                (piece == board_black_king) ||
                                (piece == board_black_king_castle));
                }

                /*
                 *  Check that any knights follow the king
                 */
                if ((piece == board_white_knight) ||
                        (piece == board_black_knight))
                {
                        assert(i == next_knight);
                        next_knight++;
                }
        }

        /*
         *  Check that the piece list is terminated
         */
        assert(side->pieces[side->nr_pieces] == -1);

        /*
         *  @TODO 2007-12-29 (marcelk) Check constraints on piece counts.
         */

        /*
         *  Check the board_side attack tables.
         *  Just recompute them from scratch, and then compare.
         *  Also check bishop diagonals
         */
        struct board_side ref;

        // active
        err = calc_struct_board_side(bd, &ref, bd->current->active.color);
        check(err);
        for (int sq=0; sq<BOARD_SIZE; sq++) {
                assert(ref.attacks[sq] == bd->current->active.attacks[sq]);
        }
        assert(bd->current->active.last_rank_pawns == ref.last_rank_pawns);
        assert(bd->current->active.bishop_diagonals == ref.bishop_diagonals);

        // passive
        err = calc_struct_board_side(bd, &ref, !bd->current->active.color);
        check(err);
        for (int sq=0; sq<BOARD_SIZE; sq++) {
                assert(ref.attacks[sq] == bd->current->passive.attacks[sq]);
        }
        assert(bd->current->passive.last_rank_pawns == ref.last_rank_pawns);
        assert(bd->current->passive.bishop_diagonals == ref.bishop_diagonals);

cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      board_clear                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Clear board
 */
err_t board_clear(struct board *bd)
{
        err_t err = OK;

        memset(bd->squares, 0, sizeof bd->squares);

        memset(bd->stack, 0, sizeof(bd->stack));
        bd->current = &bd->stack[2];

        bd->current->active.nr_pieces = 0;
        bd->current->active.pieces[0] = -1;
        memset(bd->current->active.attacks, 0, sizeof bd->current->active.attacks);
        bd->current->active.color = -1;

        bd->current->passive.nr_pieces = 0;
        bd->current->passive.pieces[0] = -1;
        memset(bd->current->passive.attacks, 0, sizeof bd->current->passive.attacks);
        bd->current->passive.color = -1;

        bd->current->en_passant_lazy = 0;

        bd->current->halfmove_clock = 0;
        bd->game_halfmove_clock_offset = 0;
        bd->game_fullmove_number = 1;

        bd->butterfly[0].prescore = 0x0123;
        assert(bd->butterfly[0].bytes[ BOARD_BUTTERFLY_HI ] == 0x01);
        assert(bd->butterfly[0].bytes[ BOARD_BUTTERFLY_LO ] == 0x23);

        bd->butterfly[0].prescore = 0;

        return err;
}

/*----------------------------------------------------------------------+
 |      board_create                                                    |
 +----------------------------------------------------------------------*/

/*
 *  Create board object
 */
err_t board_create(struct board **bd)
{
        err_t err = OK;

        struct board *new_bd = calloc(1, sizeof(*new_bd));
        // @TODO 2007-07-14 (marcelk) Regulate use of allocs

        if (new_bd == null) xRaise(ERR_NO_MEMORY);

        board_clear(new_bd);
        check(err);

        *bd = new_bd;
cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      board_destroy                                                   |
 +----------------------------------------------------------------------*/

/*
 *  Destroy board object
 */
err_t board_destroy(struct board *bd)
{
        err_t err = OK;
        free(bd);
        return err;
}

/*----------------------------------------------------------------------+
 |      board_setup                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Setup position from an already split EPD string
 */
err_t board_setup(struct board *bd,
        const char *board,
        const char *side_to_move,
        const char *castling,
        const char *enpassant,
        const char *halfmove_clock,
        const char *fullmove_number)
{
        err_t err = OK;

        int len = strlen(board) + strlen(side_to_move) +
              strlen(castling) + strlen(enpassant) +
              strlen(halfmove_clock) + strlen(fullmove_number);

        char epd[len+6];

        char *s = epd;
        while (*board) *s++ = *board++;
        *s++ = ' ';
        while (*side_to_move) *s++ = *side_to_move++;
        *s++ = ' ';
        while (*castling) *s++ = *castling++;
        *s++ = ' ';
        while (*enpassant) *s++ = *enpassant++;
        *s++ = ' ';
        while (*halfmove_clock) *s++ = *halfmove_clock++;
        *s++ = ' ';
        while (*fullmove_number) *s++ = *fullmove_number++;
        *s = '\0';

        /*
         *  TODO: board_setup_raw should use board_setup, instead
         *  of the other way around.
         */
        err = board_setup_raw(bd, epd);
        check(err);
cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      board_setup_raw                                                 |
 +----------------------------------------------------------------------*/

/*
 *  Setup position from a raw EPD string
 */
err_t board_setup_raw(struct board *bd, const char *epd)
{
        err_t err = OK;

        /*------------------------------------------------------+
         |      Parse EPD string                                |
         +------------------------------------------------------*/

        int file = BOARD_FILE_A;
        int rank = BOARD_RANK_8;
        int ix = 0;

        memset(bd->stack, 0, sizeof(bd->stack));
        bd->current = &bd->stack[2]; /* keep two frames unused */

        int side_to_move = -1;

        while (epd[ix] == ' ') ix++;

        /*
         *  Rows with pieces
         */

        for (; epd[ix] != ' '; ix++) {
                int count = 1;
                int piece = board_empty;
                switch (epd[ix]) {
                case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8':
                        count = epd[ix] - '0';
                        break;
                case 'K':
                        piece = board_white_king;
                        break;
                case 'k':
                        piece = board_black_king;
                        break;
                case 'Q':
                        piece = board_white_queen;
                        break;
                case 'q':
                        piece = board_black_queen;
                        break;
                case 'R':
                        piece = board_white_rook;
                        break;
                case 'r':
                        piece = board_black_rook;
                        break;
                case 'B':
                        // Test for dark-colored or light-colored square
                        if (((file ^ rank) & 1) == 0) {
                                piece = board_white_bishop_dark;
                        } else {
                                piece = board_white_bishop_light;
                        }
                        break;
                case 'b':
                        // Test for dark-colored or light-colored square
                        if (((file ^ rank) & 1) == 0) {
                                piece = board_black_bishop_dark;
                        } else {
                                piece = board_black_bishop_light;
                        }
                        break;
                case 'N':
                        piece = board_white_knight;
                        break;
                case 'n':
                        piece = board_black_knight;
                        break;
                case 'P':
                        if (rank == BOARD_RANK_2) {
                                piece = board_white_pawn_rank2;
                        } else if (rank == BOARD_RANK_7) {
                                piece = board_white_pawn_rank7;
                        } else {
                                piece = board_white_pawn;
                        }
                        break;
                case 'p':
                        if (rank == BOARD_RANK_7) {
                                piece = board_black_pawn_rank7;
                        } else if (rank == BOARD_RANK_2) {
                                piece = board_black_pawn_rank2;
                        } else {
                                piece = board_black_pawn;
                        }
                        break;
                case '/':
                        // Accept sloppy notation
                        while (file <= BOARD_FILE_H) {
                                bd->squares[BOARD_SQUARE(file, rank)].index = 0;
                                bd->squares[BOARD_SQUARE(file, rank)].piece = board_empty;
                                file += (BOARD_FILE_B - BOARD_FILE_A);
                        }
                        if (rank == 0) xRaise(ERR_INVALID_EPD);
                        rank += (BOARD_RANK_6 - BOARD_RANK_7);
                        file = BOARD_FILE_A;
                        continue;
                default:
                        xRaise(ERR_INVALID_EPD);
                }
                while (count > 0) {
                        if (file > BOARD_FILE_H) xRaise(ERR_INVALID_EPD);
                        bd->squares[BOARD_SQUARE(file, rank)].index = 0;
                        bd->squares[BOARD_SQUARE(file, rank)].piece = piece;
                        file += (BOARD_FILE_B - BOARD_FILE_A);
                        count--;
                }
        }

        // Accept sloppy notation
        while (file <= BOARD_FILE_H) {
                bd->squares[BOARD_SQUARE(file, rank)].index = 0;
                bd->squares[BOARD_SQUARE(file, rank)].piece = board_empty;
                file += (BOARD_FILE_B - BOARD_FILE_A);
        }

        if ((file != BOARD_FILE_H+1) || (rank != BOARD_RANK_1)) {
                xRaise(ERR_INVALID_EPD);
        }

        if (epd[ix] != ' ') xRaise(ERR_INVALID_EPD);
        ix++;

        while (epd[ix] == ' ') ix++;

        /*
         *  Side to move
         */

        switch (epd[ix]) {
        case 'w':
                side_to_move = board_white;
                break;
        case 'b':
                side_to_move = board_black;
                break;
        default:
                xRaise(ERR_INVALID_EPD);
        }
        ix++;

        if (epd[ix] != ' ') xRaise(ERR_INVALID_EPD);
        ix++;

        while (epd[ix] == ' ') ix++;

        /*
         *  Castling rights
         */

        if (epd[ix] == '-') {
                ix++;
        } else {
                for (; epd[ix] != ' '; ix++) {
                        switch(epd[ix]) {
                        case 'K':
                                if (!WHITE_HAS_KING_SIDE_CASTLING_CONFIG(bd)) {
                                        xRaise(ERR_INVALID_EPD);
                                }
                                bd->squares[E1].piece = board_white_king_castle;
                                bd->squares[H1].piece = board_white_rook_castle;
                                break;
                        case 'Q':
                                if (!WHITE_HAS_QUEEN_SIDE_CASTLING_CONFIG(bd)) {
                                        xRaise(ERR_INVALID_EPD);
                                }
                                bd->squares[E1].piece = board_white_king_castle;
                                bd->squares[A1].piece = board_white_rook_castle;
                                break;
                        case 'k':
                                if (!BLACK_HAS_KING_SIDE_CASTLING_CONFIG(bd)) {
                                        xRaise(ERR_INVALID_EPD);
                                }
                                bd->squares[E8].piece = board_black_king_castle;
                                bd->squares[H8].piece = board_black_rook_castle;
                                break;
                        case 'q':
                                if (!BLACK_HAS_QUEEN_SIDE_CASTLING_CONFIG(bd)) {
                                        xRaise(ERR_INVALID_EPD);
                                }
                                bd->squares[E8].piece = board_black_king_castle;
                                bd->squares[A8].piece = board_black_rook_castle;
                                break;
                        default:
                                xRaise(ERR_INVALID_EPD);
                        }
                }
        }

        if (epd[ix] != ' ') xRaise(ERR_INVALID_EPD);
        ix++;

        while (epd[ix] == ' ') ix++;

        /*
         *  En-passant status
         */

        switch (epd[ix]) {
        case '-':
                bd->current->en_passant_lazy = 0;
                ix++;
                break;
        case 'a': case 'b': case 'c': case 'd':
        case 'e': case 'f': case 'g': case 'h':
                file = epd[ix] - 'a';
                ix++;
                if (side_to_move == board_white) {
                        if (epd[ix] != '6') xRaise(ERR_INVALID_EPD);
                        ix++;
                        rank = BOARD_RANK_6;
                        bd->current->en_passant_lazy = BOARD_SQUARE(file, rank);
                        bd->current->en_passant_node_counter = bd->current->node_counter;
                } else {
                        if (epd[ix] != '3') xRaise(ERR_INVALID_EPD);
                        ix++;
                        rank = BOARD_RANK_3;
                        bd->current->en_passant_lazy = BOARD_SQUARE(file, rank);
                        bd->current->en_passant_node_counter = bd->current->node_counter;
                }
                break;
        default:
                xRaise(ERR_INVALID_EPD);
        }

        if ((epd[ix] == ' ') && isdigit(epd[ix+1])) {
                /*
                 *  Halfmove clock
                 */
                int d;
                int n;
                int r = sscanf(&epd[ix], "%d%n", &d, &n);
                if ((r != 1) || (d < 0)) {
                        xRaise(ERR_INVALID_EPD);
                }
                bd->game_halfmove_clock_offset = d;
                ix += n;
        } else {
                bd->game_halfmove_clock_offset = 0;
        }
        bd->current->halfmove_clock = 0;

        if ((epd[ix] == ' ') && isdigit(epd[ix+1])) {
                /*
                 *  Move number
                 */
                int d;
                int n;
                int r = sscanf(&epd[ix], "%d%n", &d, &n);
                if ((r != 1) || (d < 1)) {
                        xRaise(ERR_INVALID_EPD);
                }
                bd->game_fullmove_number = d;
                ix += n;
        } else {
                bd->game_fullmove_number = 1;
        }

        while (epd[ix] == ' ') ix++;

        if (epd[ix] != '\0') xRaise(ERR_INVALID_EPD);

        err = update_board_after_edit(bd, side_to_move);
        check(err);

cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      board_setup_square                                              |
 +----------------------------------------------------------------------*/

/*
 *  Set a piece on a square, or clear it.
 *  Automatically infer castling rights from the resulting position
 */

err_t board_setup_square(struct board *bd, int square, int piece_char, int side)
{
        err_t err = OK;

        assert(BOARD_SQUARE_IS_VALID(square));
        assert((piece_char == '-') || (side == board_white) || (side == board_black));

        int piece;

        switch (piece_char) {
        case '-':
                piece = board_empty;
                break;
        case 'K':
                piece = (side == board_white) ?
                        board_white_king :
                        board_black_king;
                break;
        case 'Q':
                piece = (side == board_white) ?
                        board_white_queen :
                        board_black_queen;
                break;
        case 'R':
                piece = (side == board_white) ?
                        board_white_rook :
                        board_black_rook;
                break;
        case 'B':
                if (BOARD_SQUARE_IS_LIGHT(square)) {
                        piece = (side == board_white) ?
                                board_white_bishop_light :
                                board_black_bishop_light;
                } else {
                        piece = (side == board_white) ?
                                board_white_bishop_dark :
                                board_black_bishop_dark;
                }
                break;
        case 'N':
                piece = (side == board_white) ?
                        board_white_knight :
                        board_black_knight;
                break;
        case 'P':
                switch (BOARD_RANK(square)) {

                case BOARD_RANK_2:
                        piece = (side == board_white) ?
                                board_white_pawn_rank2 :
                                board_black_pawn_rank2;
                        break;

                case BOARD_RANK_7:
                        piece = (side == board_white) ?
                                board_white_pawn_rank7 :
                                board_black_pawn_rank7;
                        break;

                default:
                        piece = (side == board_white) ?
                                board_white_pawn :
                                board_black_pawn;
                        break;
                }
                break;
        default:
                xRaise(ERR_INTERNAL);
        }

        bd->squares[ square ].index = 0;
        bd->squares[ square ].piece = piece;

        /*
         *  Infer castling rights from piece locations
         */

        if (bd->squares[E1].piece == board_white_king_castle) {
                bd->squares[E1].piece = board_white_king;
        }

        if (bd->squares[H1].piece == board_white_rook_castle) {
                bd->squares[H1].piece = board_white_rook;
        }

        if (bd->squares[A1].piece == board_white_rook_castle) {
                bd->squares[A1].piece = board_white_rook;
        }

        if (bd->squares[E8].piece == board_black_king_castle) {
                bd->squares[E8].piece = board_black_king;
        }

        if (bd->squares[H8].piece == board_black_rook_castle) {
                bd->squares[H8].piece = board_black_rook;
        }

        if (bd->squares[A8].piece == board_black_rook_castle) {
                bd->squares[A8].piece = board_black_rook;
        }

        if (WHITE_HAS_KING_SIDE_CASTLING_CONFIG(bd)) {
                bd->squares[E1].piece = board_white_king_castle;
                bd->squares[H1].piece = board_white_rook_castle;
        }

        if (WHITE_HAS_QUEEN_SIDE_CASTLING_CONFIG(bd)) {
                bd->squares[E1].piece = board_white_king_castle;
                bd->squares[A1].piece = board_white_rook_castle;
        }

        if (BLACK_HAS_KING_SIDE_CASTLING_CONFIG(bd)) {
                bd->squares[E8].piece = board_black_king_castle;
                bd->squares[H8].piece = board_black_rook_castle;
        }

        if (BLACK_HAS_QUEEN_SIDE_CASTLING_CONFIG(bd)) {
                bd->squares[E8].piece = board_black_king_castle;
                bd->squares[A8].piece = board_black_rook_castle;
        }

        err = update_board_after_edit(bd, bd->current->active.color);
        check(err);

cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      update_board_after_edit                                         |
 +----------------------------------------------------------------------*/

/*
 *  Update data structures after edit board
 */
static
err_t update_board_after_edit(struct board *bd, int side_to_move)
{
        err_t err = OK;

        /*------------------------------------------------------+
         |      Prepare for repetition detection                |
         +------------------------------------------------------*/

         // 2010-04-06 (marcelk) take game history into account
         // 2010-04-06 (marcelk) use halfmove clock from FEN field?

        bd->current->halfmove_clock = 0;

        bd->current[-1].halfmove_clock = 0;
        bd->current[-2].halfmove_clock = 0;

        /*------------------------------------------------------+
         |      Setup the data structures                       |
         +------------------------------------------------------*/

        struct board_side *side = &bd->current->active;

        err = calc_struct_board_side(bd, side, side_to_move);
        check(err);

        // copy piece list to board_side
        for (int i=0; i<side->nr_pieces; i++) {
                int sq = side->pieces[i];
                bd->squares[sq].index = i;
        }
        side->pieces[side->nr_pieces] = -1;
        side->nr_pieces = side->nr_pieces;

        side = &bd->current->passive;
        err = calc_struct_board_side(bd, side, !side_to_move);
        check(err);

        // copy piece list to board_side
        for (int i=0; i<side->nr_pieces; i++) {
                int sq = side->pieces[i];
                bd->squares[sq].index = i;
        }
        side->pieces[side->nr_pieces] = -1;
        side->nr_pieces = side->nr_pieces;

        /*------------------------------------------------------+
         |      Check for attacks on passive king               |
         +------------------------------------------------------*/

        if (bd->current->active.attacks[ bd->current->passive.pieces[0] ] != 0) {
                xRaise("Wrong king in check");
        }

        /*------------------------------------------------------+
         |      Check against impossible en passant indicator   |
         +------------------------------------------------------*/

        if ((bd->current->en_passant_lazy != 0) &&
            (bd->current->node_counter == bd->current->en_passant_node_counter)
        ) {
                int ep_square = bd->current->en_passant_lazy; // 3rd or 6th rank
                int from_square;
                int dest_square;
                int my_pawn;
                int his_pawn;

                if (bd->current->active.color == board_white) {
                        if (BOARD_RANK(ep_square) != BOARD_RANK_6) {
                                xRaise("Invalid en passant square");
                        }
                        from_square = ep_square + BOARD_VECTOR_NORTH;
                        dest_square = ep_square + BOARD_VECTOR_SOUTH;
                        my_pawn = board_white_pawn;
                        his_pawn = board_black_pawn;
                } else {
                        if (BOARD_RANK(ep_square) != BOARD_RANK_3) {
                                xRaise("Invalid en passant square");
                        }
                        from_square = ep_square + BOARD_VECTOR_SOUTH;
                        dest_square = ep_square + BOARD_VECTOR_NORTH;
                        my_pawn = board_black_pawn;
                        his_pawn = board_white_pawn;
                }

                /*
                 *  There must be an opponent pawn acting as ep pawn
                 */
                if (bd->squares[dest_square].piece != his_pawn) {
                        xRaise("Invalid en passant square");
                }

                /*
                 *  The two squares behind the ep pawn should be empty
                 */
                if (bd->squares[from_square].piece != board_empty) {
                        xRaise("Invalid en passant square");
                }
                if (bd->squares[ep_square].piece != board_empty) {
                        xRaise("Invalid en passant square");
                }

                /*
                 *  There must be one of our pawns next to the ep pawn
                 */
                bool has_neighbor = false;
                if (BOARD_FILE(dest_square) != BOARD_FILE_A) {
                        int sq = dest_square + BOARD_VECTOR_WEST;
                        if (bd->squares[sq].piece == my_pawn) {
                                has_neighbor = true;
                        }
                }
                if (BOARD_FILE(dest_square) != BOARD_FILE_H) {
                        int sq = dest_square + BOARD_VECTOR_EAST;
                        if (bd->squares[sq].piece == my_pawn) {
                                has_neighbor = true;
                        }
                }
                if (!has_neighbor) {
                        xRaise("Invalid en passant square");
                }

                /*
                 *  The ep pawn can't uncover a diagonal attack on our king
                 */
                int my_king = bd->current->active.pieces[0];
                int dir =
                        bd->current->passive.attacks[dest_square] &
                        BOARD_ATTACK_BISHOP &
                        data_sq2sq[dest_square][my_king];
                if (dir != 0) {
                        int step = board_vector_step[dir];
                        int sq = dest_square;
                        do {
                                sq += step;
                        } while (bd->squares[sq].piece == board_empty);

                        if (sq == my_king) {
                                xRaise("Invalid en passant square");
                        }
                }
        }

        /*------------------------------------------------------+
         |      Calculate hashes                                |
         +------------------------------------------------------*/

        err = calc_zobrist_hashes(bd,
                &bd->current->board_hash_lazy,
                &bd->current->pawn_king_hash);
        check(err);

        /*------------------------------------------------------+
         |      Calculate material key                          |
         +------------------------------------------------------*/

        err = calc_material_key(bd, &bd->current->material_key);
        check(err);

        /*------------------------------------------------------+
         |      Test for consistency                            |
         +------------------------------------------------------*/

        //err = board_check(bd);
        //check(err);

cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      calc_struct_board_side                                          |
 +----------------------------------------------------------------------*/

/*
 *  Initialize board_side data from scratch for one side
 */
static
err_t calc_struct_board_side(
        const struct board *bd,
        struct board_side *side,
        int color)
{
        err_t err = OK;

        // piece list for this side
        struct board_square const *pieces[BOARD_SIDE_MAX_PIECES];
        int nr_pieces = 0;

        side->color = color;

        int count[BOARD_PIECE_TYPES] = {0,};

        side->last_rank_pawns = 0; // Pawns on 7th(/2nd) rank

        side->bishop_diagonals = 0;

        for (int sq=0; sq<BOARD_SIZE; sq++) {
                int piece = bd->squares[sq].piece;
                if (BOARD_PIECE_COLOR(piece) != color) {
                        continue;
                }

                // append to piece list
                if (nr_pieces > BOARD_SIDE_MAX_PIECES) {
                        xRaise("Too many pieces of the same color");
                }
                pieces[nr_pieces] = &bd->squares[sq];
                nr_pieces++;

                // count piece types
                count[piece]++;

                // No pieces on rank 1 or rank 8
                if ((piece == board_white_pawn) || (piece == board_black_pawn)) {
                        if ((BOARD_RANK(sq) == BOARD_RANK_8) || (BOARD_RANK(sq) == BOARD_RANK_1)) {
                                xRaise("Pawn on back rank");
                        }
                }

                // last_rank_pawns
                if (piece == board_white_pawn_rank7) {
                        side->last_rank_pawns |= 1 << BOARD_FILE(sq);
                }
                if (piece == board_black_pawn_rank2) {
                        side->last_rank_pawns |= 1 << BOARD_FILE(sq);
                }

                // bishop diagonals
                if ((piece == board_white_bishop_dark) || (piece == board_white_bishop_light) ||
                    (piece == board_black_bishop_dark) || (piece == board_black_bishop_light)
                ) {
                        side->bishop_diagonals ^= data_bishop_diagonals[sq];
                }
        }

        int nr_kings =
                count[board_white_king] +
                count[board_white_king_castle] +
                count[board_black_king] +
                count[board_black_king_castle];

        int nr_queens =
                count[board_white_queen] +
                count[board_black_queen];

        int nr_rooks =
                count[board_white_rook] +
                count[board_white_rook_castle] +
                count[board_black_rook] +
                count[board_black_rook_castle];

        int nr_bishops_light =
                count[board_white_bishop_light] +
                count[board_black_bishop_light];

        int nr_bishops_dark =
                count[board_white_bishop_dark] +
                count[board_black_bishop_dark];

        int nr_knights =
                count[board_white_knight] +
                count[board_black_knight];

        int nr_pawns =
                count[board_white_pawn] +
                count[board_white_pawn_rank2] +
                count[board_white_pawn_rank7] +
                count[board_black_pawn] +
                count[board_black_pawn_rank2] +
                count[board_black_pawn_rank7];

        if (nr_pawns > 8) xRaise("Too many pawns");

        if (nr_kings == 0) xRaise("King missing");
        if (nr_kings > 1) xRaise("Multiple kings");

        int nr_promoted = 0;

        if (nr_queens > 1) nr_promoted += nr_queens - 1;
        if (nr_rooks > 2) nr_promoted += nr_rooks - 2;
        if (nr_bishops_light > 1) nr_promoted += nr_bishops_light - 1;
        if (nr_bishops_dark > 1) nr_promoted += nr_bishops_dark - 1;
        if (nr_knights > 2) nr_promoted += nr_knights - 2;
        if (nr_pawns + nr_promoted > 8) {
                xRaise("Too many promoted pieces");
        }

        err = calc_attacks(bd, side, color);
        check(err);

        // sort piece list (king, knights, others)
        qsort(pieces, nr_pieces, sizeof pieces[0], cmp_piece);

        for (int i=0; i<nr_pieces; i++) {
                side->pieces[i] = pieces[i] - &bd->squares[0];
        }
        side->nr_pieces = nr_pieces;

cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      calc_attacks                                                    |
 +----------------------------------------------------------------------*/

/*
 *  Calculate attack tables from scratch for one side
 */
static
err_t calc_attacks(
        const struct board *bd,
        struct board_side *side,
        int color)
{
        err_t err = OK;

        // clear all attacks
        memset(side->attacks, 0, sizeof side->attacks);

        // loop over board, not over the piece list, because piece list may not
        // be initialized
        for (int sq=0; sq<BOARD_SIZE; sq++) {

                // only pieces of the specified color
                int piece = bd->squares[sq].piece;
                if (BOARD_PIECE_COLOR(piece) != color) {
                        continue;
                }

                // mark the squares attacked by this piece
                switch (piece)
                {
                case board_white_king:
                case board_black_king:
                case board_white_king_castle:
                case board_black_king_castle:
                        attack_xor_king(side, sq);
                        break;

                case board_white_queen:
                case board_black_queen:
                        attack_xor_rays(side, bd, sq, BOARD_ATTACK_QUEEN & data_kingtab[sq]);
                        break;

                case board_white_rook:
                case board_black_rook:
                case board_white_rook_castle:
                case board_black_rook_castle:
                        attack_xor_rays(side, bd, sq, BOARD_ATTACK_ROOK & data_kingtab[sq]);
                        break;

                case board_white_bishop_light:
                case board_black_bishop_light:
                case board_white_bishop_dark:
                case board_black_bishop_dark:
                        attack_xor_rays(side, bd, sq, BOARD_ATTACK_BISHOP & data_kingtab[sq]);
                        break;

                case board_white_knight:
                case board_black_knight:
                        attack_add_knight(side, sq);
                        break;

                case board_white_pawn:
                case board_white_pawn_rank2:
                case board_white_pawn_rank7:
                        attack_xor_white_pawn(side, sq);
                        break;

                case board_black_pawn:
                case board_black_pawn_rank7:
                case board_black_pawn_rank2:
                        attack_xor_black_pawn(side, sq);
                        break;

                default:
                        xRaise(ERR_INTERNAL);
                        break;
                }
        }
cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      calc_zobrist_hashes                                             |
 +----------------------------------------------------------------------*/

/*
 *  Calculate position hashes from scratch
 *
 *  The position hash is a hash including all pieces and flags.
 *  For more speed we normally work with a lazy hash, which excludes
 *  the en-passant status.
 *
 *  The pawn_king hash hashes only pawns and kings, and uncastled rooks
 *  (the rooks so that the king safety shelter evaluation can take
 *   future castling position into consideration)
 */
static
err_t calc_zobrist_hashes(
        const struct board *bd,
        unsigned long long *board_hash_lazy_p,
        unsigned long long *pawn_king_hash_p)
{
        err_t err = OK;

        unsigned long long board_hash = 0;
        unsigned long long pawn_king_hash = 0;

        for (int sq=0; sq<BOARD_SIZE; sq++) {
                switch (bd->squares[sq].piece) {

                case board_empty:
                        break;

                /*
                 *  King
                 */
                case board_white_king:
                case board_white_king_castle:
                        board_hash ^= data_zobrist[zobrist_white_king][sq];
                        pawn_king_hash ^= data_zobrist[zobrist_white_king][sq];
                        break;

                case board_black_king:
                case board_black_king_castle:
                        board_hash ^= data_zobrist[zobrist_black_king][sq];
                        pawn_king_hash ^= data_zobrist[zobrist_black_king][sq];
                        break;

                /*
                 *  Queen
                 */
                case board_white_queen:
                        board_hash ^= data_zobrist[zobrist_white_queen][sq];
                        break;

                case board_black_queen:
                        board_hash ^= data_zobrist[zobrist_black_queen][sq];
                        break;

                /*
                 *  Rook
                 */
                case board_white_rook:
                        board_hash ^= data_zobrist[zobrist_white_rook][sq];
                        break;

                case board_white_rook_castle:
                        board_hash ^= data_zobrist[zobrist_white_rook_castle][sq];
                        pawn_king_hash ^= data_zobrist[zobrist_white_rook_castle][sq];
                        break;

                case board_black_rook:
                        board_hash ^= data_zobrist[zobrist_black_rook][sq];
                        break;

                case board_black_rook_castle:
                        board_hash ^= data_zobrist[zobrist_black_rook_castle][sq];
                        pawn_king_hash ^= data_zobrist[zobrist_black_rook_castle][sq];
                        break;

                /*
                 *  Bishop
                 */
                case board_white_bishop_light:
                case board_white_bishop_dark:
                        board_hash ^= data_zobrist[zobrist_white_bishop][sq];
                        break;

                case board_black_bishop_light:
                case board_black_bishop_dark:
                        board_hash ^= data_zobrist[zobrist_black_bishop][sq];
                        break;

                /*
                 *  Knight
                 */
                case board_white_knight:
                        board_hash ^= data_zobrist[zobrist_white_knight][sq];
                        break;

                case board_black_knight:
                        board_hash ^= data_zobrist[zobrist_black_knight][sq];
                        break;

                /*
                 *  Pawn
                 */
                case board_white_pawn:
                case board_white_pawn_rank2:
                case board_white_pawn_rank7:
                        board_hash    ^= data_zobrist[zobrist_white_pawn][sq];
                        pawn_king_hash ^= data_zobrist[zobrist_white_pawn][sq];
                        break;

                case board_black_pawn:
                case board_black_pawn_rank7:
                case board_black_pawn_rank2:
                        board_hash    ^= data_zobrist[zobrist_black_pawn][sq];
                        pawn_king_hash ^= data_zobrist[zobrist_black_pawn][sq];
                        break;

                default:
                        xRaise(ERR_INTERNAL);
                }
        }

        /*
         *  Side to move (invert all bits for black)
         */
        if (bd->current->active.color == board_black) {
                board_hash = ~board_hash;
        }

        /*
         *  Ignore en-passant flags
         */

        /*
         *  Return results
         */
        *board_hash_lazy_p = board_hash;
        *pawn_king_hash_p = pawn_king_hash;
cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |      calc_material_key                                               |
 +----------------------------------------------------------------------*/

/*
 *  Calculate the material vector from scratch
 *
 *  The material key is a 64-bit value holding 12 4-bit counters
 *  representing each piece type and a 16-bit hash.
 *
 *  Kings don't have a counter, because there is always 1.
 *  Bishops have a separate counter for bishops on light-colored squares
 *  and on dark-colored squares. This makes it possible to score the bishop
 *  pair correctly and to score opposite bishop endings more easily.
 */
static
err_t calc_material_key(
        const struct board *bd,
        unsigned long long *material_key_p)
{
        err_t err = OK;

        unsigned long long material_key = 0;

        for (int i=0; i<bd->current->active.nr_pieces; i++) {
                int piece = bd->squares[ bd->current->active.pieces[i] ].piece;
                material_key += data_material_key[piece];
        }

        for (int i=0; i<bd->current->passive.nr_pieces; i++) {
                int piece = bd->squares[ bd->current->passive.pieces[i] ].piece;
                material_key += data_material_key[piece];
        }

        *material_key_p = material_key;

        return err;
}

/*----------------------------------------------------------------------+
 |      cmp_piece                                                       |
 +----------------------------------------------------------------------*/

/*
 *  Comparison function for piece list ordering
 *  (doesn't have to be deterministic)
 */
static
int cmp_piece(const void *ap, const void *bp)
{
        struct board_square * const *a = ap;
        struct board_square * const *b = bp;

        if ((*a)->piece < (*b)->piece) return -1;
        if ((*a)->piece > (*b)->piece) return 1;

        // Make the sorting deterministic
        if (*a < *b) return -1;
        if (*a > *b) return 1;

        assert(false); // qsort() shouldn't come as far as this
        return 0;
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

