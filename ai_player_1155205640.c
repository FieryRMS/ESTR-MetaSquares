#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
const int DATA [5][8][8] = {
    {
        {  1,   0,   1,   0,   1,   0,   1,   0},
        {  0,   1,   0,   1,   0,   0,   0,   0},
        {  1,   0,   1,   0,   1,   0,   1,   0},
        {  0,   1,   0,   1,   0,   0,   0,   1},
        {  1,   0,   1,   0,   1,   0,   1,   0},
        {  0,   0,   0,   0,   0,   0,   0,   1},
        {  1,   0,   1,   0,   1,   0,   1,   0},
        {  0,   0,   0,   1,   0,   1,   0,   0}
    },
    {
        {  7,  13,  17,  19,  19,  17,  13,   7},
        { 13,  19,  23,  25,  25,  23,  19,  13},
        { 17,  23,  27,  29,  29,  27,  23,  17},
        { 19,  25,  29,  31,  31,  29,  25,  19},
        { 19,  25,  29,  31,  31,  29,  25,  19},
        { 17,  23,  27,  29,  29,  27,  23,  17},
        { 13,  19,  23,  25,  25,  23,  19,  13},
        {  7,  13,  17,  19,  19,  17,  13,   7}
    },
    {
        {203, 342, 428, 469, 469, 428, 342, 203},
        {342, 421, 467, 488, 488, 467, 421, 342},
        {428, 467, 473, 474, 474, 473, 467, 428},
        {469, 488, 474, 455, 455, 474, 488, 469},
        {469, 488, 474, 455, 455, 474, 488, 469},
        {428, 467, 473, 474, 474, 473, 467, 428},
        {342, 421, 467, 488, 488, 467, 421, 342},
        {203, 342, 428, 469, 469, 428, 342, 203}
    },
    {
        {  3,   0,   6,   0,   6,   0,   5,   0},
        {  0,   5,   0,   5,   0,   0,   0,   0},
        {  6,   0,  11,   0,   8,   0,   7,   0},
        {  0,   5,   0,   5,   0,   0,   0,   2},
        {  6,   0,   8,   0,   9,   0,   7,   0},
        {  0,   0,   0,   0,   0,   0,   0,   2},
        {  5,   0,   7,   0,   7,   0,   3,   0},
        {  0,   0,   0,   2,   0,   2,   0,   0}
    },
    {
        { 83,   0, 126,   0, 166,   0, 157,   0},
        {  0, 125,   0, 101,   0,   0,   0,   0},
        {126,   0, 147,   0, 120,   0, 191,   0},
        {  0, 101,   0,  77,   0,   0,   0,  74},
        {166,   0, 120,   0, 161,   0, 191,   0},
        {  0,   0,   0,   0,   0,   0,   0,  74},
        {157,   0, 191,   0, 191,   0,  83,   0},
        {  0,   0,   0,  74,   0,  74,   0,   0}
    }
};
//clang-format on

double weights[5+4+2] = {1,1,1,1,1, 1, 1,1,1,6,64};

typedef long long ll;
typedef struct {
    int x, y;
} Point;

typedef struct {
    Point p1, p2, p3, p4;
} Square;

enum { EMPTY, BLUE, RED, DRAW };
enum { ERR_NONE, ERR_OUT_OF_BOUND, ERR_OCCUPIED };

Point num2point(int Move)
{
    return (Point){ Move / 10, Move % 10 };
}
int point2num(Point Move)
{
    if (Move.x < 1 || Move.x > 8 || Move.y < 1 || Move.y > 8) return -1;
    return Move.x * 10 + Move.y;
}

int max(int a, int b)
{
    return a > b ? a : b;
}

int min(int a, int b)
{
    return a < b ? a : b;
}

#ifdef R_LOCAL
int debug = 1;

void printBoard(const int GameBoard[])
{
    if (!debug) return;
    printf("   1 2 3 4 5 6 7 8\n");
    for (int i = 1; i <= 8; i++)
    {
        printf("%d  ", i);
        for (int j = 1; j <= 8; j++)
        {
            switch (GameBoard[point2num((Point){ i, j })])
            {
                case EMPTY:
                    printf(". ");
                    break;
                case BLUE:
                    printf("# ");
                    break;
                case RED:
                    printf("0 ");
                    break;
            };
        }
        printf("\n");
    }
}

#define DEBUG(debug, fmt, ...)                                                \
    do {                                                                      \
        if (debug)                                                            \
            fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, \
                    __VA_ARGS__);                                             \
    } while (0)

#else
int debug = 0;
void PrintBoard(const int GameBoard[]) {}
#define DEBUG(...)
#endif

/**
 * Function #4: Check if a location is valid.
 */
int validate_input(const int Move, const int GameBoard[])
{
    Point MovePoint = num2point(Move);
    if (MovePoint.x < 1 || MovePoint.x > 8 || MovePoint.y < 1 ||
        MovePoint.y > 8)
        return ERR_OUT_OF_BOUND;

    if (GameBoard[Move] != EMPTY) return ERR_OCCUPIED;

    return ERR_NONE;
}

/**
 * given two points, get two squares
 */
Square get_square(const Point p1, const Point p2)
{
    int dx = (p1.x - p2.x), dy = (p1.y - p2.y);
    return (
        Square){ { p1.x + dy, p1.y - dx }, { p2.x + dy, p2.y - dx }, p2, p1 };
}

/**
 * prints the square and registers it in done[]
 */
void register_square(const Square sq,
                     const ll score,
                     const int Player,
                     int ShouldPrint)
{
    int num[4] = { point2num(sq.p1), point2num(sq.p2), point2num(sq.p3),
                   point2num(sq.p4) };

    if (ShouldPrint)
        printf(
            "%s gains %lld more points by formulating the squre {%d, %d, "
            "%d, %d}\n",
            Player == BLUE ? "BLUE" : "RED", score, num[0], num[1], num[2],
            num[3]);
}
/**
 * validates and returns possible score
 */
int validate_square(const Square sq, const int Player, const int GameBoard[])
{
    int p1num = point2num(sq.p1), p2num = point2num(sq.p2),
        p3num = point2num(sq.p3), p4num = point2num(sq.p4);
    // clang-format off
    if (validate_input(p1num, GameBoard) == ERR_OCCUPIED &&
        validate_input(p2num, GameBoard) == ERR_OCCUPIED &&
        validate_input(p3num, GameBoard) == ERR_OCCUPIED &&
        validate_input(p4num, GameBoard) == ERR_OCCUPIED &&
        GameBoard[p1num] == Player &&
        GameBoard[p2num] == Player &&
        GameBoard[p3num] == Player && 
        GameBoard[p4num] == Player
    )  // clang-format on
    {
        int mxRow = max(abs(sq.p3.y - sq.p1.y), abs(sq.p4.y - sq.p2.y)) + 1;
        return mxRow * mxRow;
    }
    return 0;
}

/**
 * Function #5: Check if new squares are formed
 */
int new_squares_score(const int Move,
                      const int Player,
                      const int GameBoard[],
                      int ShouldPrint,
                      Point PointList[],
                      int PointCnt)
{
    Point MovePoint = num2point(Move);
    ll total_score = 0;
    for (int j = 0; j < PointCnt - 1; j++)
    {
        Square sq1 = get_square(MovePoint, PointList[j]);
        ll score1 = validate_square(sq1, Player, GameBoard);

        if (score1) register_square(sq1, score1, Player, ShouldPrint);

        total_score += score1;
    }

    return total_score;
}

/**
 * Function #6: Check if the game is over.
 */
int is_game_over(const int GameBoard[], const ll BlueScore, const ll RedScore)
{
    if (BlueScore - RedScore >= 15 && BlueScore > 150) return BLUE;
    if (RedScore - BlueScore >= 15 && RedScore > 150) return RED;
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            if (GameBoard[point2num((Point){ i, j })] == EMPTY) return 0;

    if (BlueScore == RedScore) return DRAW;
    else if (BlueScore > RedScore) return BLUE;
    else return RED;
}

void CalculateExistingData(const Point PointList[64],
                           const int PointCnt,
                           int BoardData[2][8][8],
                           const int GameBoard[],
                           const int player)
{
    for (int i = 0; i < PointCnt - 1; i++)
    {
        for (int j = i + 1; j < PointCnt; j++)
        {
            Point a = PointList[i], b = PointList[j];
            if (a.x > b.x || (a.x == b.x && a.y > b.y))
            {
                Point temp = a;
                a = b;
                b = temp;
            }
            if (a.y >= b.y) continue;
            int dx = abs(a.x - b.x), dy = abs(a.y - b.y);
            Point c = { a.x + dy, a.y - dx }, d = { b.x + dy, b.y - dx };
            int mxRow = max(abs(c.y - b.y), abs(d.y - a.y)) + 1;
            mxRow *= mxRow;
            int validateC = validate_input(point2num(c), GameBoard),
                validateD = validate_input(point2num(d), GameBoard);
            if (validateC == ERR_NONE && validateD == ERR_NONE)
            {
                BoardData[(player - 1) * 2][c.x - 1][c.y - 1] = 1;
                BoardData[(player - 1) * 2][d.x - 1][d.y - 1] = 1;
                BoardData[(player - 1) * 2 + 1][c.x - 1][c.y - 1] = mxRow;
                BoardData[(player - 1) * 2 + 1][d.x - 1][d.y - 1] = mxRow;
            }
            else if (validateC != ERR_OUT_OF_BOUND &&
                     validateD != ERR_OUT_OF_BOUND)
            {
                BoardData[(player - 1) * 2][c.x - 1][c.y - 1] = 2;
                BoardData[(player - 1) * 2][d.x - 1][d.y - 1] = 2;
                BoardData[(player - 1) * 2 + 1][c.x - 1][c.y - 1] = mxRow;
                BoardData[(player - 1) * 2 + 1][d.x - 1][d.y - 1] = mxRow;
            }
        }
    }
}

typedef struct {
    int index;
    long double value;
} IndexValue;

int compareHeuristic(const void *a, const void *b)
{
    IndexValue *ia = (IndexValue *)a, *ib = (IndexValue *)b;
    return (int)(ia->value - ib->value);
}
void calculate_heuristics(const int player,
                          const int GameBoard[],
                          const Point PointList[2][64],
                          const int PointCnt[2],
                          int SearchPointList[64])
{
    int BoardData[4][8][8] = { 0 };
    CalculateExistingData(PointList[0], PointCnt[0], BoardData, GameBoard,
                          BLUE);
    CalculateExistingData(PointList[1], PointCnt[1], BoardData, GameBoard, RED);
    IndexValue HeuristicPointList[64];
    int HeuristicPointListCnt = 0;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
        {
            if (validate_input(point2num((Point){ i + 1, j + 1 }), GameBoard) ==
                ERR_NONE)
                HeuristicPointList[HeuristicPointListCnt].index =
                    point2num((Point){ i + 1, j + 1 });
            for (int k = 0; k < 5; k++)
                HeuristicPointList[HeuristicPointListCnt].value +=
                    weights[k] * DATA[k][i][j];

            for (int k = 0; k < 4; k++)
                HeuristicPointList[HeuristicPointListCnt].value +=
                    weights[5 + k] * BoardData[k][i][j];

            HeuristicPointListCnt++;
        }

    qsort(HeuristicPointList, HeuristicPointListCnt, sizeof(IndexValue),
          compareHeuristic);

    for (int i = 0; i < HeuristicPointListCnt; i++)
        SearchPointList[i] = HeuristicPointList[i].index;
}

// ideas
// 1. minimax
// 2. alpha-beta pruning
// 3. prefer lattice structure
// 4. use heuristics
// 5. program a genetic algorithm to find the best weights
// 6. use depth and search limit as weights
ll getMove(const int player,
           int GameBoard[],
           int depth,
           ll score,
           int currPlayer,
           int *BestMove,
           ll alpha,
           ll beta,
           int maxDepth,
           Point PointList[2][64],
           int PointCnt[2],
           int BestSequence[64])
{
    int isMaximizing = (player == currPlayer ? 1 : -1);
    ll bestScore = LLONG_MAX * (-1 * isMaximizing);

    int GameState = is_game_over(GameBoard, score, score);
    if (GameState == DRAW) return score;
    else if (GameState) return (10000 - depth) * isMaximizing;

    if (depth == maxDepth) return score;

    for (int i = 1; i <= 8; i++)
    {
        for (int j = 1; j <= 8; j++)
        {
            int Move = point2num((Point){ i, j });
            if (validate_input(Move, GameBoard) == ERR_NONE)
            {
                GameBoard[Move] = currPlayer;
                PointList[currPlayer - 1][PointCnt[currPlayer - 1]++] =
                    (Point){ i, j };
                ll deltaScore = new_squares_score(Move, currPlayer, GameBoard,
                                                  0, PointList[currPlayer - 1],
                                                  PointCnt[currPlayer - 1]);
                deltaScore = deltaScore * isMaximizing * 10;
                if (deltaScore) deltaScore -= depth * isMaximizing;

                int BestSequenceRet[64];

                ll tempScore =
                    getMove(player, GameBoard, depth + 1, score + deltaScore,
                            (currPlayer == BLUE) ? RED : BLUE, BestMove, alpha,
                            beta, maxDepth, PointList, PointCnt, BestSequenceRet);

                if (isMaximizing == 1)
                {
                    if (tempScore > bestScore)
                    {
                        bestScore = tempScore;
                        BestSequence[depth] = Move;
                        for (int k = depth + 1; k < maxDepth; k++)
                            BestSequence[k] = BestSequenceRet[k];
                        if (depth == 0) *BestMove = Move;
                    }
                    if (bestScore > alpha) alpha = bestScore;
                }
                else
                {
                    if (tempScore < bestScore)
                    {
                        bestScore = tempScore;
                        BestSequence[depth] = Move;
                        for (int k = depth + 1; k < maxDepth; k++)
                            BestSequence[k] = BestSequenceRet[k];
                        if (depth == 0) *BestMove = Move;
                    }
                    if (bestScore < beta) beta = bestScore;
                }

                GameBoard[Move] = EMPTY;
                PointCnt[currPlayer - 1]--;
                if (alpha >= beta) return bestScore;
            }
        }
    }

    return bestScore;
}

int ai_player(int player, const int *board)
{
    int GameBoard[89], empty = 0, PointCnt[2] = { 0 };
    Point PointList[2][64];
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
        {
            GameBoard[point2num((Point){ i, j })] =
                board[point2num((Point){ i, j })];
            switch (board[point2num((Point){ i, j })])
            {
                case EMPTY:
                    empty++;
                    break;
                case BLUE:
                    PointList[0][PointCnt[0]++] = (Point){ i, j };
                    break;
                case RED:
                    PointList[1][PointCnt[1]++] = (Point){ i, j };
                    break;
            }
        }

    int BestMove = -1, maxDepth = 6, BestSequence[64] = { 0 };
    if (empty == 0) return 0;

    getMove(player, GameBoard, 0, 0, player, &BestMove, LLONG_MIN, LLONG_MAX,
            maxDepth, PointList, PointCnt, BestSequence);

    if (debug)
    {
        printf("Move sequence: ");
        for (int i = 0; i < maxDepth; i++) printf("%d ", BestSequence[i]);
        printf("\n");
    }

    return BestMove;
}

int weighted_ai_player(int player, const int *board, double test_weights[])
{
    memcpy(weights, test_weights, sizeof(weights));
    return ai_player(player, board);
}