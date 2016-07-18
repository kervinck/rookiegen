
/*----------------------------------------------------------------------+
 |                                                                      |
 |      enpassant.h -- Move making functions for en-passant capture     |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Move making functions for en-passant capture
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
 *  #include "enpassant.h"
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*
 *  En passant capture with white
 */
make_move_fn enpassant_with_white_pawn;

/*
 *  En passant capture with black
 */
make_move_fn enpassant_with_black_pawn;

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

