
/*----------------------------------------------------------------------+
 |                                                                      |
 |      castle.h -- Move making functions for castling                  |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Move making functions for castling
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
 *  #include "castle.h"
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*
 *  Castle white
 */
make_move_fn castle_white_king_side;
make_move_fn castle_white_queen_side;

/*
 *  Castle black
 */
make_move_fn castle_black_king_side;
make_move_fn castle_black_queen_side;

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

