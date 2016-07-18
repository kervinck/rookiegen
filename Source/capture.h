
/*----------------------------------------------------------------------+
 |                                                                      |
 |      capture.h -- Move making functions for captures                 |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *       Move making functions for captures
 *
 *  History:
 *      2008-01-12 (marcelk) Tidy up original draft version.
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
 |      Synopsis                                                        |
 +----------------------------------------------------------------------*/

/*
 *  #include "base.h"
 *  #include "board.h"
 *  #include "intern.h"
 *  #include "capture.h"
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Functions to remove a piece from the attack board and piece list
 */
typedef
void capture_take_piece_fn(struct board *board, int sq);

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

/*
 *  Access to take_xxx functions for use by capturing promotions (promote.c)
 */
extern
capture_take_piece_fn * const capture_take_piece_fn_table[];

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*
 *  Capture with king
 */
make_move_fn capture_with_king;
make_move_fn capture_with_white_king_castle;
make_move_fn capture_with_black_king_castle;

/*
 *  Capture with queen
 */
make_move_fn capture_with_queen;

/*
 *  Capture with rook
 */
make_move_fn capture_with_rook;
make_move_fn capture_with_white_rook_castle;
make_move_fn capture_with_black_rook_castle;

/*
 *  Capture with bishop
 */
make_move_fn capture_with_bishop;

/*
 *  Capture with knight
 */
make_move_fn capture_with_knight;

/*
 *  Capture with pawn
 */
make_move_fn capture_with_white_pawn;
make_move_fn capture_with_black_pawn;

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

