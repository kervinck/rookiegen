
/*----------------------------------------------------------------------+
 |                                                                      |
 |      board.h -- Chess board for engine                               |
 |                                                                      |
 +----------------------------------------------------------------------*/

/*
 *  Description:
 *      Maintain a chess board, generate moves, make and unmake moves.
 *      The board does book keeping for the evaluator and tree search.
 */

/*----------------------------------------------------------------------+
 |      Copyright                                                       |
 +----------------------------------------------------------------------*/

/*
 *  This file is part of the Rookie(TM) Chess Program
 *  Copyright (C) 1992-2015, Marcel van Kervinck
 *  All rights reserved.
 */

/*----------------------------------------------------------------------+
 |      Synopsis                                                        |
 +----------------------------------------------------------------------*/

/*
 *  #include "base.h"
 *  #include "board.h"
 */

/*----------------------------------------------------------------------+
 |      Definitions                                                     |
 +----------------------------------------------------------------------*/

/*----------------------------------------------------------------------+
 |      Board geometry                                                  |
 +----------------------------------------------------------------------*/

/*
 *  The chess board has 64 squares.
 *
 *  The board layout is line-by-line instead of row-by-row for two reasons:
 *  1. It is easier to scan lines for pawn structure evaluation.
 *  2. It is easier to determine if a pawn can capture/attack left or right
 *     (attack.c).
 *
 *  Note: "proper" identifier names would be 'BOARD_A1' or
 *  BOARD_SQUARE_A1' or something like that. We break such naming
 *  rules here on purpose to avoid that.
 */
enum {
        A1, A2, A3, A4, A5, A6, A7, A8,
        B1, B2, B3, B4, B5, B6, B7, B8,
        C1, C2, C3, C4, C5, C6, C7, C8,
        D1, D2, D3, D4, D5, D6, D7, D8,
        E1, E2, E3, E4, E5, E6, E7, E8,
        F1, F2, F3, F4, F5, F6, F7, F8,
        G1, G2, G3, G4, G5, G6, G7, G8,
        H1, H2, H3, H4, H5, H6, H7, H8,
#define BOARD_SIZE 64
};

#define BOARD_SQUARE_IS_VALID(square) (((square) & ~63) == 0)

enum {
        BOARD_RANK_1, BOARD_RANK_2, BOARD_RANK_3, BOARD_RANK_4,
        BOARD_RANK_5, BOARD_RANK_6, BOARD_RANK_7, BOARD_RANK_8,
};

enum {
        BOARD_FILE_A, BOARD_FILE_B, BOARD_FILE_C, BOARD_FILE_D,
        BOARD_FILE_E, BOARD_FILE_F, BOARD_FILE_G, BOARD_FILE_H,
};

#define BOARD_SQUARE(file, rank) (((file) << 3) | (rank))
#define BOARD_RANK(square) ((square) & 7)
#define BOARD_FILE(square) ((square) >> 3)

#define BOARD_SQUARE_IS_LIGHT(sq) (((BOARD_RANK(sq) ^ BOARD_FILE(sq)) & 1) == 1)

#define BOARD_VECTOR_NORTH     (E5 - E4)
#define BOARD_VECTOR_EAST      (F4 - E4)
#define BOARD_VECTOR_SOUTH     (E3 - E4)
#define BOARD_VECTOR_WEST      (D4 - E4)
#define BOARD_VECTOR_NORTHEAST (F5 - E4)
#define BOARD_VECTOR_NORTHWEST (D5 - E4)
#define BOARD_VECTOR_SOUTHEAST (F3 - E4)
#define BOARD_VECTOR_SOUTHWEST (D3 - E4)

/*----------------------------------------------------------------------+
 |      Chess pieces                                                    |
 +----------------------------------------------------------------------*/

/*
 *  Many piece types have multiple representations to speed-up
 *  move generation.
 */

enum board_piece {
        board_empty                     = 0,

        board_white_king                = 2,
        board_black_king                = 3,
        board_white_king_castle         = 4, // can castle
        board_black_king_castle         = 5, // can castle

        board_white_knight              = 6,
        board_black_knight              = 7,

        board_white_pawn_rank2          = 8, // can move two steps
        board_black_pawn_rank7          = 9, // can move two steps
        board_white_pawn                = 10,
        board_black_pawn                = 11,
        board_white_pawn_rank7          = 12, // can promote
        board_black_pawn_rank2          = 13, // can promote

        board_white_bishop_light        = 14, // on light-colored squares
        board_black_bishop_light        = 15, // on light-colored squares
        board_white_bishop_dark         = 16, // on dark-colored squares
        board_black_bishop_dark         = 17, // on dark-colored squares

        board_white_rook                = 18,
        board_black_rook                = 19,
        board_white_rook_castle         = 20, // can castle
        board_black_rook_castle         = 21, // can castle

        board_white_queen               = 22,
        board_black_queen               = 23,
#define BOARD_PIECE_TYPES 24
};

#define BOARD_PIECE_COLOR(pc) (((pc) == board_empty) ? -1 : ((pc) & 1))

enum {
        board_neutral = -1,
        board_white = 0,
        board_black = 1,
};

/*
 *  A piece on the board.
 *  A square with a piece contains also an index into the piece list.
 */
struct board_square {
        /*
         *  Piece type (value from enum board_piece)
         */
        signed char piece;

        /*
         *  Index into the pieces[] list. Needed for captures.
         */
        signed char index;
};

/*----------------------------------------------------------------------+
 |      Attack board information                                        |
 +----------------------------------------------------------------------*/

enum board_attack {
        board_attack_north              = 0x0001,
        board_attack_northeast          = 0x0002,
        board_attack_east               = 0x0004,
        board_attack_southeast          = 0x0008,
        board_attack_south              = 0x0010,
        board_attack_southwest          = 0x0020,
        board_attack_west               = 0x0040,
        board_attack_northwest          = 0x0080,
        board_attack_king               = 0x0100,
        board_attack_pawn_west          = 0x0200,
        board_attack_pawn_east          = 0x0400,
        board_attack_knight             = 0x0800, // 4-bit counter for knights
        board_attack_unused             = ~0x7fff,
};

#define BOARD_ATTACK_PAWN (\
        board_attack_pawn_west |\
        board_attack_pawn_east)

#define BOARD_ATTACK_VERTICAL (\
        board_attack_north |\
        board_attack_south )

#define BOARD_ATTACK_HORIZONTAL (\
        board_attack_west |\
        board_attack_east )

#define BOARD_ATTACK_ROOK (\
        board_attack_north |\
        board_attack_east |\
        board_attack_south |\
        board_attack_west )

#define BOARD_ATTACK_BISHOP (\
        board_attack_northeast |\
        board_attack_southeast |\
        board_attack_southwest |\
        board_attack_northwest )

#define BOARD_ATTACK_QUEEN (\
        BOARD_ATTACK_ROOK |\
        BOARD_ATTACK_BISHOP)

/*
 *  Reverse a ray attack (for example: northwest -> southeast)
 */
#define BOARD_ATTACK_REVERSE(dir) (\
        (((dir) << 4) | ((dir) >> 4)) & BOARD_ATTACK_QUEEN\
)

/*----------------------------------------------------------------------+
 |      Chess moves                                                     |
 +----------------------------------------------------------------------*/

/*
 *  Maximum string size
 */
#define BOARD_MOVE_STRING_SIZE_MAX (sizeof "a7xb8=N+")

/*
 *  A convenient upper limit on the number of possible moves.
 *  It is believed that no legal chess position can exceed this number.
 *  Actually, the known world record is somewhat lower.
 */
#define BOARD_MAX_MOVES 256

/*
 *  Number of killer moves that search' can use
 */
#define BOARD_MAX_KILLER_MOVES 6

/*----------------------------------------------------------------------+
 |      Information per side                                            |
 +----------------------------------------------------------------------*/

/*
 *  Maximum number of pieces per side (included king and pawns)
 */
#define BOARD_SIDE_MAX_PIECES 16

/*
 *  Information per side (white or black)
 */
struct board_side {
        /*
         *  Detailed attack information per square
         */
        short attacks[BOARD_SIZE];

        /*
         *  Flags indicating which diagonals have a bishop on them.
         *  Indexed as: '14 - file - rank' and '22 + file - rank' so that
         *  the bits are sorted suitably for the square evaluation in full.c
         */
        int bishop_diagonals;

        /*
         *  A list (terminated by -1) of squares of the pieces for this player.
         *  King is always on first position.
         *  Knights follow immediately after the king.
         *  After that are all other pieces, in any order.
         */
        signed char pieces[BOARD_SIDE_MAX_PIECES+1];
        signed char nr_pieces;

        /*
         *  Identifies this side's color.
         *  This constant field eliminates the need for flipping a
         *  side-to-move flag during every move and unmove.
         */
        signed char color;

        /*
         *  Flags for pending promotions, one per file
         */
        unsigned char last_rank_pawns;
};

#define BOARD_BISHOP_DIAGONALS(sq) (\
        1 << (14 - BOARD_FILE(sq) - BOARD_RANK(sq)) |\
        1 << (22 + BOARD_FILE(sq) - BOARD_RANK(sq)))

/*----------------------------------------------------------------------+
 |      Chess board and move stack                                      |
 +----------------------------------------------------------------------*/

/*
 *  Board unmove information per square
 */
struct board_undo {
        signed char square;
        struct board_square piece;
};

/*
 *  A regular move changes 2 squares that must be restored in board_move_undo.
 *  A capture usually also changes the location of the last_piece, because
 *  that gets moved into the gap that the captured piece leaves behind.
 *  En-passant capture changes (3 or) 4 squares.
 *  Castling also changes the castle status of the non-castling rook, so 5.
 *
 *  The absolute worst-case is a capture of a non-last knight with a king that
 *  can still castle both ways. That changes the board for 6 locations:
 *   from (king), A-rook, H-rook, to (knight), other (last_piece), last_knight)
 */
#define BOARD_UNDO_LEN_MAX 6

/*
 *  Constants for the material key
 */
#define BOARD_MATERIAL_KEY_WHITE_PAWN           0x514e000000000001ull
#define BOARD_MATERIAL_KEY_WHITE_KNIGHT         0x6ab5000000000010ull
#define BOARD_MATERIAL_KEY_WHITE_BISHOP_LIGHT   0x2081000000000100ull
#define BOARD_MATERIAL_KEY_WHITE_BISHOP_DARK    0xb589000000001000ull
#define BOARD_MATERIAL_KEY_WHITE_ROOK           0xae45000000010000ull
#define BOARD_MATERIAL_KEY_WHITE_QUEEN          0x9ac3000000100000ull
#define BOARD_MATERIAL_KEY_BLACK_PAWN           0x696d000001000000ull
#define BOARD_MATERIAL_KEY_BLACK_KNIGHT         0xd903000010000000ull
#define BOARD_MATERIAL_KEY_BLACK_BISHOP_LIGHT   0x3d15000100000000ull
#define BOARD_MATERIAL_KEY_BLACK_BISHOP_DARK    0x67f5001000000000ull
#define BOARD_MATERIAL_KEY_BLACK_ROOK           0x7de9010000000000ull
#define BOARD_MATERIAL_KEY_BLACK_QUEEN          0xa96f100000000000ull

#define BOARD_MATERIAL_KEY_COUNTS_MASK ((1ULL<<48)-1)
#define BOARD_MATERIAL_KEY_COUNTS(key) ((key) & BOARD_MATERIAL_KEY_COUNTS_MASK)

#define BOARD_MATERIAL_KEY_SLIDERS(key) ((key) &\
        (BOARD_MATERIAL_KEY_COUNTS(\
                BOARD_MATERIAL_KEY_WHITE_BISHOP_LIGHT |\
                BOARD_MATERIAL_KEY_WHITE_BISHOP_DARK |\
                BOARD_MATERIAL_KEY_WHITE_ROOK |\
                BOARD_MATERIAL_KEY_WHITE_QUEEN |\
                BOARD_MATERIAL_KEY_BLACK_BISHOP_LIGHT |\
                BOARD_MATERIAL_KEY_BLACK_BISHOP_DARK |\
                BOARD_MATERIAL_KEY_BLACK_ROOK |\
                BOARD_MATERIAL_KEY_BLACK_QUEEN)\
        * 0xf))

/*
 *  Board stack frame structure
 */
struct board_stack_frame {
        struct board_side active; /* side to move, friend */
        struct board_side passive; /* other side, enemy */

        int undo_len;
        struct board_undo undo[BOARD_UNDO_LEN_MAX];

        /*
         *  Static frame data (it will not be cleared)
         */
        long long node_counter;

        /*
         *  The halfmove clock is defined in the PGN standard:
         *  "This number is the count of halfmoves (or ply) since the last
         *   pawn advance or capturing move."
         *  Although irreversible, we don't update for castling. Reason
         *  is that we want to do proper 50-move detection as well.
         */
        unsigned char halfmove_clock;

        /*
         *  En-passant square (if any, 0 otherwise)
         *
         *  This is a `lazy' field to gain a little speed:
         *  It is SET after each double pawn-push, but not CLEARED after
         *  other moves (so it can give false positives).
         *  The idea is that setting en-passant is a rare action, and
         *  clearing would be needed for every move. We don't want to that.
         */
        signed char en_passant_lazy;

        /*
         *  The generator must do an extra heck whenever the
         *  it finds the en-passant flag as SET, and CLEAR it in case
         *  it is found to be a false hit (to prevent repeated false hits)
         *  There are various ways to do the checking:
         *  - Checking if undo[] contains proof of a double-pawn move
         *      This is somewhat ugly code; Need special setup at root
         *  - Storing a counter
         *      Elegant, but sensitive to overflows (theoretically)
         *  - Storing the hash code
         *      Elegant, but sensitive to collisions (theoretically)
         *  We copy the node counter.
         */
        long long en_passant_node_counter;

        /*
         *  Zobrist hash for position and pawn/king status.
         *
         *  En passant status is excluded, so the move make function
         *  doesn't have to adjust it every time. Hence 'lazy'.
         */
        unsigned long long board_hash_lazy;
        unsigned long long pawn_king_hash;

        /*
         *  Vector of piece type counts and hash, for
         *  calculation of the material balance
         */
        unsigned long long material_key;

        /*
         *  Killer moves
         */
        unsigned short killer_moves[BOARD_MAX_KILLER_MOVES];
};

/*
 *  A fixed maximum search depth that is large enough to never be reached
 *
 *  The actual reached depth can be extracted from the stack afterwards
 *  because we keep a node counter per level.
 */
#define BOARD_MAX_DEPTH 250

/*
 *  Board structure with move make and undo
 */
struct board {
        /*
         *  The chess board squares with pieces
         */
        struct board_square squares[ BOARD_SIZE ];

        /*
         *  Pointer to the current stack frame
         */
        struct board_stack_frame *current;

        /*
         *  Dynamic move evaluation table, incorporated in move generation.
         */
        union {
                unsigned short prescore;
                uint8_t bytes[2];
        } butterfly[BOARD_SIZE * BOARD_SIZE];

        /*
         *  Stack for move making:
         *      BOARD_MAX_DEPTH moves
         *      1 extra frame for making BOARD_MAX_DEPTH moves
         *      2 extra frames to allow indexing of frame -1 and -2
         *      (used in killer moves & repetition detection)
         */
        struct board_stack_frame stack[ 2 + BOARD_MAX_DEPTH + 1 ];

        /*
         *  Small temporary data space to keep track of extra defenders in
         *  static exchange evaluation. Used by exchange.c and generate.c
         */
        int extra_defenders[ BOARD_SIZE ];

        /*
         *  Game's move number, incremented when black moves
         */
        int game_fullmove_number;
        int game_halfmove_clock_offset;
};

/*
 *  Byte order of the two halves in a prescore (little-endian CPU).
 *  Will be verified in board_clear()
 */
#define BOARD_BUTTERFLY_LO 0
#define BOARD_BUTTERFLY_HI 1

/*
 *  The move generator creates board_move structs with pre-calculated SEE
 *  scores and an opaque pointer to the specific move maker code.
 *  The search has to add its dynamic scoring and sort the move list.
 *  The search must invoke board_make_move to perform the move, not
 *  invoke make itself.
 */
union board_move {
        unsigned int sort_value;
        struct {
                short move;
                unsigned short prescore;
                void (*make)(void);
        } bm;
};

typedef union board_move board_move_t;

struct board_move_info {
        uint8_t from_square;        // 0..63
        char from_piece;        // "PNBRQK"
        uint8_t to_square;          // 0..63
        char to_piece;          // "PNBRQ\0"
        char promotion_piece;   // "QRBN\0"
};

/*----------------------------------------------------------------------+
 |      Data                                                            |
 +----------------------------------------------------------------------*/

/*
 *  Relationship between two squares
 */
extern unsigned const short data_sq2sq[BOARD_SIZE][BOARD_SIZE];

/*
 *  Starting position
 */
extern const char board_starting_position_FEN[];

/*
 *  Exported for monitoring the SEE effeciency
 */
extern long long board_exchange_table_miss_counter;

/*----------------------------------------------------------------------+
 |      Functions                                                       |
 +----------------------------------------------------------------------*/

/*
 *  Module initialization during startup
 */
void board_module_init(void);

/*
 *  Object creation
 */
err_t board_create(struct board **bd_p);
err_t board_destroy(struct board *bd);
err_t board_clear(struct board *bd);

/*
 *  Consistency checks
 */
err_t board_check(struct board *bd);

/*
 *  Setup a new position
 */
err_t board_setup_raw(struct board *bd, const char *epd);

err_t board_setup(struct board *bd,
        const char *board,
        const char *side_to_move,
        const char *castling,
        const char *enpassant,
        const char *halfmove_clock,
        const char *fullmove_number);

err_t board_setup_square(
        struct board *bd,
        int square,
        int piece_char,
        int side);

/*
 *  Move generator
 */
int board_generate_all_moves(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES]);

int board_generate_captures_and_promotions(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES]);

int board_generate_regular_moves(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES]);

int board_generate_escapes(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES]);

int board_generate_regular_checks(
        struct board *bd,
        union board_move moves_p[BOARD_MAX_MOVES]);

static inline
bool board_in_check(const struct board *bd)
{
        return bd->current->passive.attacks[ bd->current->active.pieces[0] ] != 0;
}

bool board_is_stalemate(struct board *bd);

/*
 *  Make move, unmake move
 */
void board_make_move(struct board *bd, const union board_move *move);
void board_undo_move(struct board *bd);

/*
 *  Null move
 */
void board_make_null_move(struct board *bd);
void board_undo_null_move(struct board *bd);

/*
 *  Check for technical draw
 */
err_t board_is_draw(struct board *bd, const char **draw_reason_p);

/*
 *  String representations
 */

err_t board_format(
        struct board *bd,
        bool flip_view,
        bool use_ansi_colors,
        const char * const info_lines[9] /* optional */);

#define BOARD_MAX_FEN_STRING_SIZE 256

err_t board_fen_string(
        struct board *bd,
        char s[BOARD_MAX_FEN_STRING_SIZE]);

err_t board_dump(struct board *bd); // @TODO: 2007-07-14 (marcelk) StringBuffer

err_t board_format_variation(struct board *bd, char buffer[BOARD_MAX_DEPTH * BOARD_MOVE_STRING_SIZE_MAX]);

err_t board_move_format_simple(
        char s[BOARD_MOVE_STRING_SIZE_MAX],
        int move);

err_t board_move_parse_simple(
        const char *move_string,
        int *move_p);

err_t board_get_move_info(
        struct board *bd,
        int move,
        struct board_move_info *move_info_p);

int board_to_square(int move);
int board_from_square(int move);

/*
 *  Caches for move generation (but not move sortingduring search, such as killers)
 */
err_t board_reset_caches(void);

/*
 *  Statistics (counters, timers, ...)
 */
err_t board_reset_stats(struct board *bd);

/*
 *  Standard performance tester
 */
err_t board_perft(struct board *bd, int depth, long long *count_p);

/*
 *  The order (and signedness) in union board_move and signedness is
 *  'unsigned short move' + 'unsigned short prescore', such that GCC on
 *  an Intel CPU can treat the pair as one long word when sorting
 *  moves. (See board-move-value.s for verification)
 */
static inline
unsigned int board_move_sort_value(union board_move bm)
{
        assert(bm.sort_value == (((unsigned)bm.bm.prescore << 16) | (unsigned)bm.bm.move));
        return bm.sort_value;
}

/*
 *  Current depth. Beware, this is a slow function, due to the division needed.
 */
static inline
int board_current_ply(const struct board *bd)
{
        /* We start at 2. Frame 0 and 1 are dummy parent frames for killers. */
        return (int) (bd->current - &bd->stack[2]);
}

/*
 *  Check for repetition.
 *  Bugs:
 *  1. This function is slow and should be replaced with something
 *     more integrated
 *  2. This function relies on hash codes, it doesn't check the
 *     actual position.
 *  3. It uses the board_hash_lazy, which doesn't contain en-passant
 *     information.
 */
bool board_check_repetition_fn(struct board *bd);
bool board_check_upcoming_repetition_fn(struct board *bd);

static inline
bool board_check_repetition(struct board *bd)
{
        assert((bd->current[0].halfmove_clock <= 1) ||
                (bd->current[0].halfmove_clock == bd->current[-1].halfmove_clock + 1));

        if ((int)bd->current->halfmove_clock <= 3) {
                return false;
        }

        return board_check_repetition_fn(bd);
}

static inline
bool board_check_upcoming_repetition(struct board *bd)
{
        if ((int)bd->current->halfmove_clock <= 2) {
                return false;
        }

        return board_check_upcoming_repetition_fn(bd);
}
 
/*
 *  Stats
 */
err_t board_get_total_node_count(struct board *bd, long long *total_nodes_p);

/*----------------------------------------------------------------------+
 |                                                                      |
 +----------------------------------------------------------------------*/

