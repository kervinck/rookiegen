
/*----------------------------------------------------------------------+
 |                                                                      |
 |      main-mk-data.c -- Generator for lookup tables                   |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Generator for lookup tables.
 *
 *      On x86, it is faster to lookup from const tables than from
 *      tables that are initialized during startup. So all tables
 *      must be generated off-line or during compile time. We choose
 *      for the last method for most flexibility.
 *
 *  History:
 *      2008-01-12 (marcelk) Creation.
 *      2008-01-13 (marcelk) Added Zobrist hashing.
 *      2009-09-21 (marcelk) Added pawn attacks into data_sq2sq[].
 *      2013-01-23 (marcelk) Cuckoo table with zobrist hashes of reversible moves
 *      2013-03-10 (marcelk) Added cuckoo squares for faster legality test of cuckoo moves
 */

/*----------------------------------------------------------------------+
 |      Copyright                                                       |
 +----------------------------------------------------------------------*/

/*
 *  This file is part of the Rookie(TM) Chess Program
 *  Copyright (C) 1992-2013, Marcel van Kervinck
 *  All rights reserved.
 */

/*----------------------------------------------------------------------+
 |      Includes                                                        |
 +----------------------------------------------------------------------*/

/*
 *  C standard includes
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 *  Base include
 */
#include "cplus.h"

/*
 *  Other includes
 */
#include "board.h"
#include "intern.h"

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

static
int compare_reversible_move(const void *ap, const void *bp);

struct cuckoo_move {
        unsigned long long move_hash;
        char squares[2];
};

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

static
const signed char vector_step[] = {
        [board_attack_north]     = BOARD_VECTOR_NORTH,
        [board_attack_northeast] = BOARD_VECTOR_NORTHEAST,
        [board_attack_east]      = BOARD_VECTOR_EAST,
        [board_attack_southeast] = BOARD_VECTOR_SOUTHEAST,
        [board_attack_south]     = BOARD_VECTOR_SOUTH,
        [board_attack_southwest] = BOARD_VECTOR_SOUTHWEST,
        [board_attack_west]      = BOARD_VECTOR_WEST,
        [board_attack_northwest] = BOARD_VECTOR_NORTHWEST,
};

static
const signed char vector_jump[] = {
        [jump_north_northwest]  = BOARD_VECTOR_NORTH + BOARD_VECTOR_NORTHWEST,
        [jump_north_northeast]  = BOARD_VECTOR_NORTH + BOARD_VECTOR_NORTHEAST,
        [jump_east_northeast]   = BOARD_VECTOR_EAST  + BOARD_VECTOR_NORTHEAST,
        [jump_east_southeast]   = BOARD_VECTOR_EAST  + BOARD_VECTOR_SOUTHEAST,
        [jump_south_southwest]  = BOARD_VECTOR_SOUTH + BOARD_VECTOR_SOUTHWEST,
        [jump_south_southeast]  = BOARD_VECTOR_SOUTH + BOARD_VECTOR_SOUTHEAST,
        [jump_west_northwest]   = BOARD_VECTOR_WEST  + BOARD_VECTOR_NORTHWEST,
        [jump_west_southwest]   = BOARD_VECTOR_WEST  + BOARD_VECTOR_SOUTHWEST,
};

/*----------------------------------------------------------------------+
 |      Main                                                            |
 +----------------------------------------------------------------------*/

/*
 *  Generate contents of data.c on stdout
 */
int main(void)
{
        printf("/* This code is generated. Don't edit. */\n");
        printf("\n");

        printf("#include \"assert.h\"\n");
        printf("#include \"stdbool.h\"\n");
        printf("#include \"stdint.h\"\n");
        printf("#include \"Source/cplus.h\"\n");
        printf("#include \"Source/board.h\"\n");
        printf("#include \"Source/intern.h\"\n");
        printf("\n");

        /*
         *  The relation between any two squares
         */
        unsigned short sq2sq[BOARD_SIZE][BOARD_SIZE] = {{0,}};

        /*------------------------------------------------------+
         |      data_kingtab                                    |
         +------------------------------------------------------*/

        unsigned char kingtab[BOARD_SIZE] = {0,};

        printf("const unsigned char data_kingtab[BOARD_SIZE] = {");

        for (int sq=0; sq<BOARD_SIZE; sq++) {
                int dirs = 0;

                if (BOARD_RANK(sq) != BOARD_RANK_8) {
                        dirs |= board_attack_north;
                        if (BOARD_FILE(sq) != BOARD_FILE_H) {
                                dirs |= board_attack_northeast;
                        }
                        if (BOARD_FILE(sq) != BOARD_FILE_A) {
                                dirs |= board_attack_northwest;
                        }
                }
                if (BOARD_FILE(sq) != BOARD_FILE_H) {
                        dirs |= board_attack_east;
                }
                if (BOARD_FILE(sq) != BOARD_FILE_A) {
                        dirs |= board_attack_west;
                }
                if (BOARD_RANK(sq) != BOARD_RANK_1) {
                        dirs |= board_attack_south;
                        if (BOARD_FILE(sq) != BOARD_FILE_H) {
                                dirs |= board_attack_southeast;
                        }
                        if (BOARD_FILE(sq) != BOARD_FILE_A) {
                                dirs |= board_attack_southwest;
                        }
                }

                kingtab[sq] = dirs;

                printf("%s %3d,", (sq&7)?"":"\n", dirs);
        }

        printf("\n};\n\n");

        /*------------------------------------------------------+
         |      data_knighttab                                  |
         +------------------------------------------------------*/

        unsigned char knighttab[BOARD_SIZE] = {0,};

        printf("const unsigned char data_knighttab[BOARD_SIZE] = {");

        for (int sq=0; sq<BOARD_SIZE; sq++) {
                int dirs = 0;

                if ((BOARD_RANK(sq) != BOARD_RANK_7) &&
                    (BOARD_RANK(sq) != BOARD_RANK_8))
                {
                        if (BOARD_FILE(sq) != BOARD_FILE_A) {
                                dirs |= jump_north_northwest;
                        }
                        if (BOARD_FILE(sq) != BOARD_FILE_H) {
                                dirs |= jump_north_northeast;
                        }
                }

                if ((BOARD_FILE(sq) != BOARD_FILE_G) &&
                    (BOARD_FILE(sq) != BOARD_FILE_H))
                {
                        if (BOARD_RANK(sq) != BOARD_RANK_8) {
                                dirs |= jump_east_northeast;
                        }
                        if (BOARD_RANK(sq) != BOARD_RANK_1) {
                                dirs |= jump_east_southeast;
                        }
                }

                if ((BOARD_RANK(sq) != BOARD_RANK_1) &&
                    (BOARD_RANK(sq) != BOARD_RANK_2))
                {
                        if (BOARD_FILE(sq) != BOARD_FILE_A) {
                                dirs |= jump_south_southwest;
                        }
                        if (BOARD_FILE(sq) != BOARD_FILE_H) {
                                dirs |= jump_south_southeast;
                        }
                }

                if ((BOARD_FILE(sq) != BOARD_FILE_A) &&
                    (BOARD_FILE(sq) != BOARD_FILE_B))
                {
                        if (BOARD_RANK(sq) != BOARD_RANK_8) {
                                dirs |= jump_west_northwest;
                        }
                        if (BOARD_RANK(sq) != BOARD_RANK_1) {
                                dirs |= jump_west_southwest;
                        }
                }

                knighttab[sq] = dirs;

                printf("%s %3d,", (sq&7)?"":"\n", dirs);
        }

        printf("\n};\n\n");

        /*------------------------------------------------------+
         |      data_ray_len                                    |
         +------------------------------------------------------*/

        /* init DeBruijn-based index raylen table */

        printf("const signed char data_raylen[BOARD_SIZE][8] = {\n");

        for (int sq=0; sq<BOARD_SIZE; sq++) {

                signed char row[8] = { 0, };

                printf(" {");

                for (int dir=1; dir<=128; dir<<=1) {
                        int vector = vector_step[dir];
                        int len = 0;
                        int step = sq;
                        while ((kingtab[step] & dir) != 0) {
                                step += vector;
                                len++;

                                sq2sq[sq][step] |= dir;
                        }

                        row[DEBRUIJN_INDEX(dir)] = len;
                }

                for (int i=0; i<arrayLen(row); i++) {
                        printf(" %d,", row[i]);
                }

                printf(" },\n");
        }

        printf("};\n\n");

        /*------------------------------------------------------+
         |      data_sq2sq                                      |
         +------------------------------------------------------*/

        for (int a=0; a<BOARD_SIZE; a++) {
                int dirs, dir;
                int b;

                /*
                 *  Within a king step
                 */
                dirs = kingtab[a];
                dir = 0;
                do {
                        dir -= dirs;
                        dir &= dirs;
                        dirs -= dir;
                        b = a + vector_step[dir];
                        sq2sq[a][b] |= board_attack_king;
                } while (dirs != 0);

                /*
                 *  Within a knight jump
                 */
                dirs = knighttab[a];
                dir = 0;
                do {
                        dir -= dirs;
                        dir &= dirs;
                        dirs -= dir;
                        b = a + vector_jump[dir];
                        sq2sq[a][b] |= board_attack_knight;
                } while (dirs != 0);

                /*
                 *  Within a pawn capture
                 */
                if (BOARD_FILE(a) != BOARD_FILE_A) {
                        if (BOARD_RANK(a) != BOARD_RANK_8) {
                                b = a + BOARD_VECTOR_NORTHWEST;
                                sq2sq[a][b] |= board_attack_pawn_west;
                        }
                        if (BOARD_RANK(a) != BOARD_RANK_1) {
                                b = a + BOARD_VECTOR_SOUTHWEST;
                                sq2sq[a][b] |= board_attack_pawn_west;
                        }
                }
                if (BOARD_FILE(a) != BOARD_FILE_H) {
                        if (BOARD_RANK(a) != BOARD_RANK_8) {
                                b = a + BOARD_VECTOR_NORTHEAST;
                                sq2sq[a][b] |= board_attack_pawn_east;
                        }
                        if (BOARD_RANK(a) != BOARD_RANK_1) {
                                b = a + BOARD_VECTOR_SOUTHEAST;
                                sq2sq[a][b] |= board_attack_pawn_east;
                        }
                }

                /*
                 *  XOR-encoded promotion moves [from][to^XOR_PROM_XXX]
                 */
                if (BOARD_RANK(a) == BOARD_RANK_7) {
                        b = a + BOARD_VECTOR_NORTH;
                        sq2sq[a][b^XOR_PROM_QUEEN] |= data_promotion_queen;
                        sq2sq[a][b^XOR_PROM_ROOK] |= data_promotion_rook;
                        sq2sq[a][b^XOR_PROM_BISHOP] |= data_promotion_bishop;
                        sq2sq[a][b^XOR_PROM_KNIGHT] |= data_promotion_knight;

                        if (BOARD_FILE(a) != BOARD_FILE_A) {
                                b = a + BOARD_VECTOR_NORTHWEST;
                                sq2sq[a][b^XOR_PROM_QUEEN] |= data_promotion_queen;
                                sq2sq[a][b^XOR_PROM_ROOK] |= data_promotion_rook;
                                sq2sq[a][b^XOR_PROM_BISHOP] |= data_promotion_bishop;
                                sq2sq[a][b^XOR_PROM_KNIGHT] |= data_promotion_knight;
                        }
                        if (BOARD_FILE(a) != BOARD_FILE_H) {
                                b = a + BOARD_VECTOR_NORTHEAST;
                                sq2sq[a][b^XOR_PROM_QUEEN] |= data_promotion_queen;
                                sq2sq[a][b^XOR_PROM_ROOK] |= data_promotion_rook;
                                sq2sq[a][b^XOR_PROM_BISHOP] |= data_promotion_bishop;
                                sq2sq[a][b^XOR_PROM_KNIGHT] |= data_promotion_knight;
                        }
                }

                if (BOARD_RANK(a) == BOARD_RANK_2) {
                        b = a + BOARD_VECTOR_SOUTH;
                        sq2sq[a][b^XOR_PROM_QUEEN] |= data_promotion_queen;
                        sq2sq[a][b^XOR_PROM_ROOK] |= data_promotion_rook;
                        sq2sq[a][b^XOR_PROM_BISHOP] |= data_promotion_bishop;
                        sq2sq[a][b^XOR_PROM_KNIGHT] |= data_promotion_knight;

                        if (BOARD_FILE(a) != BOARD_FILE_A) {
                                b = a + BOARD_VECTOR_SOUTHWEST;
                                sq2sq[a][b^XOR_PROM_QUEEN] |= data_promotion_queen;
                                sq2sq[a][b^XOR_PROM_ROOK] |= data_promotion_rook;
                                sq2sq[a][b^XOR_PROM_BISHOP] |= data_promotion_bishop;
                                sq2sq[a][b^XOR_PROM_KNIGHT] |= data_promotion_knight;
                        }
                        if (BOARD_FILE(a) != BOARD_FILE_H) {
                                b = a + BOARD_VECTOR_SOUTHEAST;
                                sq2sq[a][b^XOR_PROM_QUEEN] |= data_promotion_queen;
                                sq2sq[a][b^XOR_PROM_ROOK] |= data_promotion_rook;
                                sq2sq[a][b^XOR_PROM_BISHOP] |= data_promotion_bishop;
                                sq2sq[a][b^XOR_PROM_KNIGHT] |= data_promotion_knight;
                        }
                }
        }

        printf("const unsigned short data_sq2sq[BOARD_SIZE][BOARD_SIZE] = {\n");

        for (int a=0; a<BOARD_SIZE; a++) {
                printf(" {");

                for (int b=0; b<BOARD_SIZE; b++) {
                        unsigned short value = sq2sq[a][b];
                        printf("%s 0x%04x,", (b&7)?"":"\n ", value);

                        /* Check consistency of promotion XOR encoding */
                        if ((value & DATA_PROMOTION_FLAGS) != 0) {
                                int prom_bits = value & DATA_PROMOTION_FLAGS;
                                if ((prom_bits & (prom_bits-1)) != 0) {

                                        fprintf(stderr, "Warning: Promotion move conflicts with other promotion (a=%d, b=%d, value=0x%04x)\n",
                                                a, b, value);
                                }

                                if ((value & ~DATA_PROMOTION_FLAGS) != 0) {
                                        fprintf(stderr, "Warning: Promotion move conflicts with regular move (a=%d, b=%d, value=0x%04x)\n",
                                                a, b, value);
                                }
                        }
                }
                printf("\n },\n");
        }

        printf("};\n\n");

        /*------------------------------------------------------+
         |      Directions to check with knight                 |
         +------------------------------------------------------*/

        unsigned char knight_checks[BOARD_SIZE][BOARD_SIZE] = {{0,}};

        for (int xking=0; xking<BOARD_SIZE; xking++) {
                for (int knight=0; knight<BOARD_SIZE; knight++) {
                        int dirs = knighttab[knight];
                        int dir = 0;
                        do {
                                dir -= dirs;
                                dir &= dirs;
                                dirs -= dir;

                                int sq = knight + vector_jump[dir];

                                if ((sq2sq[xking][sq] & board_attack_knight) != 0) {
                                        knight_checks[xking][knight] |= dir;
                                }
                        } while (dirs != 0);
                }
        }

        printf("const unsigned char data_knight_checks[BOARD_SIZE][BOARD_SIZE] = {\n");

        for (int a=0; a<BOARD_SIZE; a++) {
                printf(" {");

                for (int b=0; b<BOARD_SIZE; b++) {
                        unsigned char value = knight_checks[a][b];
                        printf("%s %3d,", (b&7)?"":"\n ", (int) value);
                }
                printf("\n },\n");
        }

        printf("};\n\n");

        /*------------------------------------------------------+
         |      Hash table                                      |
         +------------------------------------------------------*/

        unsigned long long zobrist[ZOBRIST_PIECE_TYPES][BOARD_SIZE];

        long random = 1;

        for (int piece=0; piece<ZOBRIST_PIECE_TYPES; piece++) {
                for (int sq=0; sq<BOARD_SIZE; sq++) {
                        unsigned long long value = 0;

                        for (int i=0; i<8; i++) {
                                random = 16807L * (random % 127773L) -
                                        2836L * (random / 127773L);
                                if (random <= 0) {
                                        random += 0x7fffffffL;
                                }

                                value = (value << 8) ^ (random & 0xff);
                        }

                        zobrist[piece][sq] = value;
                }
        }

        printf("const unsigned long long data_zobrist[ZOBRIST_PIECE_TYPES][BOARD_SIZE] = {\n");

        for (int piece=0; piece<ZOBRIST_PIECE_TYPES; piece++) {
                printf(" {");
                for (int sq=0; sq<BOARD_SIZE; sq++) {
                        printf("%s 0x%016llx,", (sq&3)?"":"\n", zobrist[piece][sq]);
                }
                printf("\n },\n");
        }

        printf("};\n\n");

        /*------------------------------------------------------+
         |      Bishop diagonals                                |
         +------------------------------------------------------*/

        printf("const int data_bishop_diagonals[BOARD_SIZE] = {");

        for (int sq=0; sq<BOARD_SIZE; sq++) {
                printf("%s 0x%08x,", (sq&7)?"":"\n", BOARD_BISHOP_DIAGONALS(sq));
        }
        printf("\n};\n\n");

        /*------------------------------------------------------+
         |      Hash codes for reversible moves                 |
         +------------------------------------------------------*/

        struct cuckoo_move reversible_moves[2][1834];
        int nr_reversible_moves = 0;

        for (int a=0; a<BOARD_SIZE-1; a++) {
                for (int b=a; b<BOARD_SIZE; b++) {

                        /*
                         *  Kings
                         */
                        if ((sq2sq[a][b] & board_attack_king) != 0) {

                                assert(nr_reversible_moves < arrayLen(reversible_moves[0]));

                                reversible_moves[board_white][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_white_king][a] ^
                                        zobrist[zobrist_white_king][b]);

                                reversible_moves[board_white][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_white][nr_reversible_moves].squares[1] = b;

                                reversible_moves[board_black][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_black_king][a] ^
                                        zobrist[zobrist_black_king][b]);

                                reversible_moves[board_black][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_black][nr_reversible_moves].squares[1] = b;

                                nr_reversible_moves++;
                        }

                        /*
                         *  Queens
                         */
                        if ((sq2sq[a][b] & BOARD_ATTACK_QUEEN) != 0) {

                                assert(nr_reversible_moves < arrayLen(reversible_moves[0]));

                                reversible_moves[board_white][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_white_queen][a] ^
                                        zobrist[zobrist_white_queen][b]);

                                reversible_moves[board_white][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_white][nr_reversible_moves].squares[1] = b;

                                reversible_moves[board_black][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_black_queen][a] ^
                                        zobrist[zobrist_black_queen][b]);

                                reversible_moves[board_black][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_black][nr_reversible_moves].squares[1] = b;

                                nr_reversible_moves++;
                        }

                        /*
                         *  Rooks
                         */
                        if ((sq2sq[a][b] & BOARD_ATTACK_ROOK) != 0) {

                                assert(nr_reversible_moves < arrayLen(reversible_moves[0]));

                                reversible_moves[board_white][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_white_rook][a] ^
                                        zobrist[zobrist_white_rook][b]);

                                reversible_moves[board_white][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_white][nr_reversible_moves].squares[1] = b;

                                reversible_moves[board_black][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_black_rook][a] ^
                                        zobrist[zobrist_black_rook][b]);

                                reversible_moves[board_black][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_black][nr_reversible_moves].squares[1] = b;

                                nr_reversible_moves++;
                        }

                        /*
                         *  Bishops
                         */
                        if ((sq2sq[a][b] & BOARD_ATTACK_BISHOP) != 0) {

                                assert(nr_reversible_moves < arrayLen(reversible_moves[0]));

                                reversible_moves[board_white][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_white_bishop][a] ^
                                        zobrist[zobrist_white_bishop][b]);

                                reversible_moves[board_white][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_white][nr_reversible_moves].squares[1] = b;

                                reversible_moves[board_black][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_black_bishop][a] ^
                                        zobrist[zobrist_black_bishop][b]);

                                reversible_moves[board_black][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_black][nr_reversible_moves].squares[1] = b;

                                nr_reversible_moves++;
                        }

                        /*
                         *  Knights
                         */
                        if ((sq2sq[a][b] & board_attack_knight) != 0) {

                                assert(nr_reversible_moves < arrayLen(reversible_moves[0]));

                                reversible_moves[board_white][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_white_knight][a] ^
                                        zobrist[zobrist_white_knight][b]);

                                reversible_moves[board_white][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_white][nr_reversible_moves].squares[1] = b;

                                reversible_moves[board_black][nr_reversible_moves].move_hash = ~(
                                        zobrist[zobrist_black_knight][a] ^
                                        zobrist[zobrist_black_knight][b]);

                                reversible_moves[board_black][nr_reversible_moves].squares[0] = a;
                                reversible_moves[board_black][nr_reversible_moves].squares[1] = b;

                                nr_reversible_moves++;
                        }
                }
        }

        assert(nr_reversible_moves == arrayLen(reversible_moves[0]));

        qsort(reversible_moves[board_white],
                nr_reversible_moves,
                sizeof(reversible_moves[0][0]),
                compare_reversible_move);

        qsort(reversible_moves[board_black],
                nr_reversible_moves,
                sizeof(reversible_moves[0][0]),
                compare_reversible_move);

        struct cuckoo_move cuckoo_moves[2][0x1000];

        memset(cuckoo_moves, 0, sizeof cuckoo_moves);

        for (int j=0; j<2; j++) {
                for (int i=0; i<nr_reversible_moves; i++) {

                        struct cuckoo_move move = reversible_moves[j][i];
                        assert(move.move_hash != 0ULL);

                        int hash = DATA_CUCKOO_MOVE_HASH1(move.move_hash);

                        for (;;) {
                                struct cuckoo_move other = cuckoo_moves[j][hash];
                                cuckoo_moves[j][hash] = move;

                                if (other.move_hash == 0ULL) {
                                        break;
                                }

                                move = other;
                                if (hash == DATA_CUCKOO_MOVE_HASH1(move.move_hash)) {
                                        hash = DATA_CUCKOO_MOVE_HASH2(move.move_hash);
                                } else {
                                        hash = DATA_CUCKOO_MOVE_HASH1(move.move_hash);
                                }
                        }
                }
        }

        printf("const unsigned long data_cuckoo_move_keys[2][0x1000] = {");

        for (int j=0; j<2; j++) {
                int k = 0;
                printf("\n {");
                for (int i=0; i<0x1000; i++) {
                        if (cuckoo_moves[j][i].move_hash != 0ULL) {
                                unsigned long key = DATA_CUCKOO_MOVE_KEY(cuckoo_moves[j][i].move_hash);
                                assert(key != 0UL);

                                printf("%s [%d] = 0x%08lx,", (k & 3) ? "" : "\n ", i, key);
                                k++;
                        }
                }
                printf("\n },");
        }

        printf("\n};\n\n");

        printf("const char data_cuckoo_squares[2][0x1000][2] = {");

        for (int j=0; j<2; j++) {
                int k = 0;
                printf("\n {");
                for (int i=0; i<0x1000; i++) {
                        if (cuckoo_moves[j][i].move_hash != 0ULL) {
                                int a = cuckoo_moves[j][i].squares[0];
                                int b = cuckoo_moves[j][i].squares[1];
                                printf("%s [%d] = {%c%c,%c%c},", (k & 3) ? "" : "\n ", i,
                                        'A' + BOARD_FILE(a), '1' + BOARD_RANK(a),
                                        'A' + BOARD_FILE(b), '1' + BOARD_RANK(b));
                                k++;
                        }
                }
                printf("\n },");
        }

        printf("\n};\n\n");

        /*------------------------------------------------------+
         |                                                      |
         +------------------------------------------------------*/

        return 0;
}

/*
 *  Function to compare reversible moves for qsort()
 */

static
int compare_reversible_move(const void *ap, const void *bp)
{
        const struct cuckoo_move *a = ap;
        const struct cuckoo_move *b = bp;

        if (a->move_hash < b->move_hash) return -1;
        if (a->move_hash > b->move_hash) return +1;
        return 0;
}

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

