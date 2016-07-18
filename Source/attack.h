
/*----------------------------------------------------------------------+
 |                                                                      |
 |      attack.h -- Maintain attack tables                              |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Functions to add or remove a piece from an attack table.
 *
 *  History:
 *      2008-01-11 (marcelk) Tidy up original draft version.
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
 *  #include "attack.h"
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Note: the attack table definitions are shared with other
 *  modules through board.h.
 */

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*
 *  Queen, rook or bishop
 */
void attack_xor_rays(
        struct board_side *side,
        const struct board *bd,
        int sq,
        int dirs);

/*
 *  King
 */
void attack_xor_king(struct board_side *side, int sq);

/*
 *  Knight
 */
void attack_add_knight(struct board_side *side, int sq);
void attack_sub_knight(struct board_side *side, int sq);

/*
 *  Pawn
 */
void attack_xor_white_pawn(struct board_side *side, int sq);
void attack_xor_black_pawn(struct board_side *side, int sq);

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

