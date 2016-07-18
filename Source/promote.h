
/*----------------------------------------------------------------------+
 |                                                                      |
 |      promote.h -- Move making functions for promotion                |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Move making functions for promotion
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
 *  #include "promote.h"
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*
 *  White promotions
 */
make_move_fn promote_white_queen;
make_move_fn promote_white_rook;
make_move_fn promote_white_bishop;
make_move_fn promote_white_knight;

/*
 *  Black promotions
 */
make_move_fn promote_black_queen;
make_move_fn promote_black_rook;
make_move_fn promote_black_bishop;
make_move_fn promote_black_knight;

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

