
/*----------------------------------------------------------------------+
 |                                                                      |
 |      exchange.h -- Static exchange evaluation                        |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Static exchange evaluation
 *
 *      This module does the static exchange evalution (SEE).
 *      Lots of work has been done in generate.c already: during
 *      move generation data on attackers and defenders has been
 *      collected.
 *
 *  History:
 *      2008-01-13 (marcelk) Import from Rookie 2.0 sources, add comments
 *      2009-12-xx (marcelk) Clean up and optimized
 */

/*----------------------------------------------------------------------+
 |      Copyright                                                       |
 +----------------------------------------------------------------------*/

/*
 *  This file is part of the Rookie(TM) Chess Program
 *  Copyright (C) 1992-2009, Marcel van Kervinck
 *  All rights reserved.
 */

/*----------------------------------------------------------------------+
 |      Synopsis                                                        |
 +----------------------------------------------------------------------*/

/*
 *  #include "base.h"
 *  #include "board.h"
 *  #include "exchange.h"
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Unit for static prescores (1 pawn)
 */
#define EXCHANGE_UNIT 0x0100

/*
 *  Good moves have a neutral prescore or higher.
 *  Bad moves have a lower than neutral prescore.
 */
#define EXCHANGE_NEUTRAL 0x0f00

/*
 *  Good moves get an extra offset that changes
 *  their 4 highest bits to all-ones
 */
#define EXCHANGE_GOOD_MOVE_OFFSET (0xf000 - EXCHANGE_NEUTRAL)

/*
 *  List elements
 */
#define EXCHANGE_LIST_PAWN              (1)             /* 1 */
#define EXCHANGE_LIST_MINOR             (1*3)           /* 3 */
#define EXCHANGE_LIST_ROOK              (1*3*12)        /* 36 */
#define EXCHANGE_LIST_ROYAL             (1*3*12*11)     /* 396 */

/*
 *  Promotions indicator
 */
#define EXCHANGE_LAST_RANK              (1<<12)

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

extern const short exchange_piece_to_list[BOARD_PIECE_TYPES];

extern const short exchange_piece_value[BOARD_PIECE_TYPES];

extern const short exchange_negative_piece_value[BOARD_PIECE_TYPES];

extern const short exchange_put_upfront[BOARD_PIECE_TYPES];

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

int exchange_collect_defenders(
        struct board *board,
        int square,
        short defense_bits);

int exchange_collect_extra_defenders(
        struct board *board,
        int square,
        short defense_bit);

int exchange_collect_attackers(
        const struct board *board,
        int square,
        short attack_bits);

extern
int exchange_evaluate_fn(int defenders, int attackers);

static inline
int exchange_evaluate(int defenders, int attackers)
{
        attackers = (attackers + defenders) & 0xffff;
        defenders >>= 16;

        /* TODO: remove this branch, solve with lookup instead */
        if (defenders == 0) {
                return 0;
        } else {
                return exchange_evaluate_fn(defenders, attackers);
        }
}

err_t exchange_reset_caches(void);
err_t exchange_reset_stats(void);

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

