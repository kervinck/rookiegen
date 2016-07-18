
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cplus.h"

#include "board.h"

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

static
void perft(
        struct board *bd,
        int depth_minus_1,
        union board_move *moves,
        int nr_moves);

/*----------------------------------------------------------------------+
 |      board_perft                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Perft main function
 */
err_t board_perft(struct board *bd, int depth, long long *count_p)
{
        err_t err = OK;
        long long count = 0;

        if (depth == 0) {
                count = 1;
                xReturn;
        }

        union board_move moves[BOARD_MAX_MOVES];
        int nr_moves = board_generate_all_moves(bd, moves);

        if (depth == 1) {
                count = nr_moves;
        } else {
                perft(bd, depth-2, moves, nr_moves);
                count = bd->current[depth].node_counter;
        }
done:
        *count_p = count;
        return err;
}

/*----------------------------------------------------------------------+
 |      perft                                                           |
 +----------------------------------------------------------------------*/

/*
 *  Recursive inner function for perft
 *
 *  Because the move generator creates only legal moves, there is no
 *  need to make any moves in the deepest level. We can just count the
 *  moves in the move list as legal nodes. This speeds up perft and
 *  also makes it more useful for testing correctness.
 */
static
void perft(
        struct board *bd,
        int depth_minus_1,
        union board_move *moves,
        int nr_moves)
{
        for (int i=0; i<nr_moves; i++) {
                board_make_move(bd, &moves[i]);
                union board_move new_moves[BOARD_MAX_MOVES];
                int nr_new_moves = board_generate_all_moves(bd, new_moves);

                if (depth_minus_1 == 0) {
                        bd->current[+1].node_counter += nr_new_moves;
                } else {
                        perft(bd, depth_minus_1 - 1, new_moves, nr_new_moves);
                }

                board_undo_move(bd);
        }
}

/*----------------------------------------------------------------------+
 |      main                                                            |
 +----------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
        err_t err = OK;
        struct board *bd = null;
        charList lineBuffer = emptyList;

        if (argc != 2)
                xRaise("Invalid arguments");

        int depth = atoi(argv[1]);

        err = board_create(&bd);
        check(err);

        long long total = 0;

        while (readLine(stdin, &lineBuffer) != 0) {
                //fputs(lineBuffer.v, stdout);
                long long factor = 0;
                char *s = strchr(lineBuffer.v, ',');
                if (s == null)
                        s = strchr(lineBuffer.v, '\n');
                else
                        factor = atoll(s+1);
                if (s != null)
                        *s = '\0';
                err = board_setup_raw(bd, lineBuffer.v);
                check(err);
                long long result;
                board_perft(bd, depth, &result);
                total += factor * result;
        }

        printf("%lld\n", total);

cleanup:
        board_destroy(bd);
        freeList(lineBuffer);
        return errExitMain(err);
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

