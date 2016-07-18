
/*----------------------------------------------------------------------+
 |                                                                      |
 |      intern.h -- Internal shared definitions for board               |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Internal shared definitions for board
 *
 *  History:
 *      2008-01-13 (marcelk) Tidy up original draft version
 *      2008-01-13 (marcelk) Added Zobrist hashing
 *      2008-01-14 (marcelk) Added material key constants
 *      2008-02-16 (marcelk) Cleanup interface with evaluate
 *      2009-10-11 (marcelk) Detection of mate-in-1 moves.
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
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Prototype for a move maker helper function (move_p->make)
 */
typedef
void make_move_fn(struct board *bd, int from, int to);

/*
 *  Null move definition.
 *  Should not be 0 because that better means 'no move'.
 *  (The null move -is- a move.)
 */

#define NULL_MOVE 0x0fff

/*----------------------------------------------------------------------*/

/*
 *  King and knight tables
 */
extern const signed char board_vector_step[1+128];
extern const signed char board_vector_step_compact[8];
extern const signed char board_vector_jump[1+128];

/*
 *  Directions for knight jumps
 */
enum {
        jump_north_northwest    = 1,
        jump_north_northeast    = 2,
        jump_east_northeast     = 4,
        jump_east_southeast     = 8,
        jump_south_southwest    = 16,
        jump_south_southeast    = 32,
        jump_west_northwest     = 64,
        jump_west_southwest     = 128,
};

/*
 *  Possible king moves per square
 */
extern const unsigned char data_kingtab[BOARD_SIZE];

/*
 *  Possible knight jumps per square
 */
extern const unsigned char data_knighttab[BOARD_SIZE];

/*
 *  In which direction to move with knigth to give check
 */
extern const unsigned char data_knight_checks[BOARD_SIZE][BOARD_SIZE];

/*
 *  Lookup table to obtain the length of a ray from any square in any direction.
 *  Indexing is done by DEBRUIJN_INDEX(dir), with (dir == 1<<n), for speed.
 */
extern const signed char data_raylen[BOARD_SIZE][8];

/*
 *  Macro to map values of the form 1<<n into the range 0..N
 *  Possible values are 23 and 29
 */
#define DEBRUIJN_INDEX(n) ((( (n) * 23 ) >> 5 ) & 7)
//#define DEBRUIJN_INDEX(n) (((((n) << 5) - ((n) + (n) + (n) )) >> 5 ) & 7)

/*----------------------------------------------------------------------*/

/*
 *  Zobrist numbers for creating a position hash
 */
enum zobrist_piece_type {
        zobrist_white_king,
        zobrist_white_queen,
        zobrist_white_rook,
        zobrist_white_bishop,
        zobrist_white_knight,
        zobrist_white_pawn,
        zobrist_black_king,
        zobrist_black_queen,
        zobrist_black_rook,
        zobrist_black_bishop,
        zobrist_black_knight,
        zobrist_black_pawn,
#define ZOBRIST_PIECE_TYPES 12
};

const unsigned long long data_zobrist[ZOBRIST_PIECE_TYPES][BOARD_SIZE];

/*
 *  Hash castling rooks as a 'king', so that the castling
 *  status effects the hash
 */
#define zobrist_white_rook_castle zobrist_white_pawn
#define zobrist_black_rook_castle zobrist_black_pawn

/*----------------------------------------------------------------------*/

/*
 *  Material keys
 */
const unsigned long long data_material_key[BOARD_PIECE_TYPES];

/*----------------------------------------------------------------------*/

/*
 *  Macro to append an element to the undo list
 */
#define PUSH_UNDO(frame, xsquare) do{\
        int undo_len = ((frame)->undo_len)++;\
        assert(undo_len < BOARD_UNDO_LEN_MAX);\
        (frame)->undo[undo_len].square = (xsquare);\
        (frame)->undo[undo_len].piece = bd->squares[xsquare];\
}while(0)

/*
 *  The first undo entry is always the from-field, then the to-field
 */
#define UNDO_FROM 0
#define UNDO_TO 1

/*----------------------------------------------------------------------*/

/*
 *  Four special values to XOR'ed the to-squares of promotion moves
 *
 *  The purpose is to make such moves uniquely distinguishable
 *  from other moves and from each other, while staying within
 *  12 bits. (For example killer tables, transposition tables,
 *  counter move table, etc)
 *
 *  The diagram below visualizes the effect of such XOR'ing for a pawn
 *  promotion from C7 to B8, C8 or D8. The XOR values are chosen
 *  such that they can't map to an otherwise legally reachable square.
 *
 *      8  - ? ? ? - Q Q Q      XOR_PROM_QUEEN
 *      7  x x P x x x x x
 *      6  - x x x - R R R      XOR_PROM_QUEEN
 *      5  x - x - x B B B      XOR_PROM_QUEEN
 *      4  - - x - - x - -
 *      3  - - x - - - x -
 *      2  - - x - - - - x
 *      1  - - x - - N N N      XOR_PROM_KNIGHT
 */
#define XOR_PROM_QUEEN  BOARD_SQUARE(4,0)
#define XOR_PROM_ROOK   BOARD_SQUARE(4,2)
#define XOR_PROM_BISHOP BOARD_SQUARE(4,3)
#define XOR_PROM_KNIGHT BOARD_SQUARE(4,7)

/*
 *  The sq2sq[][] table uses these flags to identify XOR-encoded promotion moves
 */
enum {
        data_promotion_queen    = 0x1000,
        data_promotion_rook     = 0x2000,
        data_promotion_bishop   = 0x4000,
        data_promotion_knight   = 0x8000,
};

#define DATA_PROMOTION_FLAGS (\
        data_promotion_queen |\
        data_promotion_rook |\
        data_promotion_bishop |\
        data_promotion_knight )

/*----------------------------------------------------------------------*/

const int data_bishop_diagonals[BOARD_SIZE];

/*----------------------------------------------------------------------*/

/*
 *  Tables and definitions for upcoming repetition detection
 */

#define DATA_CUCKOO_MOVE_HASH1(h) ((int) (((h) >> 32) & 0x0fff))
#define DATA_CUCKOO_MOVE_HASH2(h) ((int) (((h) >> 48) & 0x0fff))
#define DATA_CUCKOO_MOVE_KEY(h) (((unsigned long) h) & 0xffffffffUL)

const unsigned long data_cuckoo_move_keys[2][0x1000];
const char data_cuckoo_squares[2][0x1000][2]; // from-to

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

