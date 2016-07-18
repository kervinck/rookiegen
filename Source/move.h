
/*----------------------------------------------------------------------+
 |                                                                      |
 |      move.h -- Move making functions for regular moves               |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Move making functions for regular moves
 *
 *  History:
 *      2008-01-13 (marcelk) Tidy up original draft version.
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
 *  #include "move.h"
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

#define MOVE(from,to) (((from)<<6) | (to))
#define MOVE_FROM(move) ((move)>>6)
#define MOVE_TO(move) ((move)&63)

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*
 *  Move king
 */
make_move_fn move_white_king;
make_move_fn move_black_king;
make_move_fn move_white_king_castle;
make_move_fn move_black_king_castle;

/*
 *  Move queen
 */
make_move_fn move_white_queen;
make_move_fn move_black_queen;

/*
 *  Move rook
 */
make_move_fn move_white_rook;
make_move_fn move_black_rook;
make_move_fn move_white_rook_castle;
make_move_fn move_black_rook_castle;

/*
 *  Move bishop
 */
make_move_fn move_white_bishop;
make_move_fn move_black_bishop;

/*
 *  Move knight
 */
make_move_fn move_white_knight;
make_move_fn move_black_knight;

/*
 *  Move white pawn
 */
make_move_fn move_white_pawn_rank2_to_3;
make_move_fn move_white_pawn_rank2_to_4;
make_move_fn move_white_pawn;

/*
 *  Move black pawn
 */
make_move_fn move_black_pawn_rank7_to_6;
make_move_fn move_black_pawn_rank7_to_5;
make_move_fn move_black_pawn;

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

