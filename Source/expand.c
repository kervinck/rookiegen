
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cplus.h"

#include "board.h"

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

static long long factor = 0;

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

static
void expand(struct board *bd, int depth)
{
        union board_move moves[BOARD_MAX_MOVES];
        int nrMoves = board_generate_all_moves(bd, moves);

        depth--;
        if (!depth) {
                for (int i=0; i<nrMoves; i++) {
                        char fen[BOARD_MAX_FEN_STRING_SIZE];
                        board_make_move(bd, &moves[i]);
                        (void) board_fen_string(bd, fen);
                        board_undo_move(bd);
                        printf("%s,%lld\n", fen, factor);
                }
        } else {
                for (int i=0; i<nrMoves; i++) {
                        board_make_move(bd, &moves[i]);
                        expand(bd, depth);
                        board_undo_move(bd);
                }
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

        while (readLine(stdin, &lineBuffer) != 0) {
                if (depth == 0) {
                        fputs(lineBuffer.v, stdout);
                        continue;
                }

                char *s = strchr(lineBuffer.v, ',');
                if (s == null) {
                        s = strchr(lineBuffer.v, '\n');
                        factor = 0;
                } else
                        factor = atoll(s+1);
                if (s != null)
                        *s = '\0';
                err = board_setup_raw(bd, lineBuffer.v);
                check(err);
                expand(bd, depth);
        }

cleanup:
        board_destroy(bd);
        freeList(lineBuffer);
        return errExitMain(err);
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

