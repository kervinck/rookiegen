
/*----------------------------------------------------------------------+
 |                                                                      |
 |      format.c -- Provide text representation for board data          |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Provide text representation for board data
 *
 *  History:
 *      2008-01-12 (marcelk) Tidy up original draft version.
 *      2008-02-17 (marcelk) Extended board_format options
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

/*
 *  C standard includes
 */
#include <assert.h>
#include <ctype.h>
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
#include "intern.h"

/*
 *  Other includes
 */
#include "move.h"

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*
 *  ANSI sequences for a nicely colored board on text consoles
 *
 *  The color scheme used sets background colors for the squares,
 *  foreground colors for the pieces and boldface for the white
 *  pieces. The result should also look reasonable on a monochrome
 *  terminal (empty squares show up as '-' there).
 */
#define ANSI_ESCAPE             "\033["
#define ANSI_LIGHT              "46"
#define ANSI_DARK               "44"
#define ANSI_EMPTY_LIGHT        "36"
#define ANSI_EMPTY_DARK         "34"
#define ANSI_WHITE              "37"
#define ANSI_BLACK              "30"
#define ANSI_NCSA_WORKAROUND    "37;40"
#define ANSI_RESET              "0"
#define ANSI_BOLD               "1"
#define ANSI_END                "m"

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      board_fen_string                                                |
 +----------------------------------------------------------------------*/

err_t board_fen_string(
        struct board *bd,
        char s[BOARD_MAX_FEN_STRING_SIZE])
{
        err_t err = OK;

        /*
         *  Piece placement data
         */

        for (int rank=7; rank>=0; rank--) {
                int empty = 0;
                for (int file=0; file<8; file++) {

                        int square = BOARD_SQUARE(file, rank);
                        int piece = bd->squares[ square ].piece;

                        if (piece == board_empty) {
                                empty++;
                                continue;
                        }

                        if (empty > 0) {
                                *s++ = '0' + empty;
                                empty = 0;
                        }

                        switch (piece) {
                        case board_white_king:
                        case board_white_king_castle:
                                *s++ = 'K';
                                break;
                        case board_white_queen:
                                *s++ = 'Q';
                                break;
                        case board_white_rook:
                        case board_white_rook_castle:
                                *s++ = 'R';
                                break;
                        case board_white_bishop_light:
                        case board_white_bishop_dark:
                                *s++ = 'B';
                                break;
                        case board_white_knight:
                                *s++ = 'N';
                                break;
                        case board_white_pawn:
                        case board_white_pawn_rank2:
                        case board_white_pawn_rank7:
                                *s++ = 'P';
                                break;
                        case board_black_king:
                        case board_black_king_castle:
                                *s++ = 'k';
                                break;
                        case board_black_queen:
                                *s++ = 'q';
                                break;
                        case board_black_rook:
                        case board_black_rook_castle:
                                *s++ = 'r';
                                break;
                        case board_black_bishop_light:
                        case board_black_bishop_dark:
                                *s++ = 'b';
                                break;
                        case board_black_knight:
                                *s++ = 'n';
                                break;
                        case board_black_pawn:
                        case board_black_pawn_rank2:
                        case board_black_pawn_rank7:
                                *s++ = 'p';
                                break;
                        default:
                                xRaise(ERR_INTERNAL);
                        }
                }

                if (empty > 0) {
                        *s++ = '0' + empty;
                }

                if (rank) {
                        *s++ = '/';
                }
        }

        /*
         *  Active color
         */

        *s++ = ' ';
        *s++ = (bd->current->active.color == board_white) ? 'w' : 'b';

        /*
         *  Castling availability
         */

        *s++ = ' ';
        int castling = 0;
        if (bd->squares[H1].piece == board_white_rook_castle) castling |= 1;
        if (bd->squares[A1].piece == board_white_rook_castle) castling |= 2;
        if (bd->squares[H8].piece == board_black_rook_castle) castling |= 4;
        if (bd->squares[A8].piece == board_black_rook_castle) castling |= 8;
        if (castling != 0) {
                if ((castling & 1) != 0) *s++ = 'K';
                if ((castling & 2) != 0) *s++ = 'Q';
                if ((castling & 4) != 0) *s++ = 'k';
                if ((castling & 8) != 0) *s++ = 'q';
        } else {
                *s++ = '-';
        }

        /*
         *  En passant target square
         */

        *s++ = ' ';

        int ep_square = bd->current->en_passant_lazy;
        if (bd->current->node_counter != bd->current->en_passant_node_counter) {
                ep_square = 0;
        }
        if (ep_square != 0) {
                bool can_capture = false;

                if (BOARD_RANK(ep_square) == BOARD_RANK_3) {
                        if ((BOARD_FILE(ep_square) != BOARD_FILE_A) &&
                            (bd->squares[ep_square + BOARD_VECTOR_NORTHWEST].piece == board_black_pawn)
                        ) {
                                can_capture = true;
                        }
                        if ((BOARD_FILE(ep_square) != BOARD_FILE_H) &&
                            (bd->squares[ep_square + BOARD_VECTOR_NORTHEAST].piece == board_black_pawn)
                        ) {
                                can_capture = true;
                        }
                }
                if (BOARD_RANK(ep_square) == BOARD_RANK_6) {
                        if ((BOARD_FILE(ep_square) != BOARD_FILE_A) &&
                            (bd->squares[ep_square + BOARD_VECTOR_SOUTHWEST].piece == board_white_pawn)
                        ) {
                                can_capture = true;
                        }
                        if ((BOARD_FILE(ep_square) != BOARD_FILE_H) &&
                            (bd->squares[ep_square + BOARD_VECTOR_SOUTHEAST].piece == board_white_pawn)
                        ) {
                                can_capture = true;
                        }
                }

                if (!can_capture) {
                        ep_square = 0;
                }
        }
        if (ep_square != 0) {
                *s++ = 'a' + BOARD_FILE(ep_square);
                *s++ = '1' + BOARD_RANK(ep_square);
        } else {
                *s++ = '-';
        }

        /*
         *  Terminate string
         */

        *s = '\0';

cleanup:
        return err;
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

