#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
int debug = 0;

#ifdef R_LOCAL

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
int is_game_over(const int GameBoard[], const ll RedScore, const ll BlueScore)
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

// ideas
// 1. minimax
// 2. alpha-beta pruning
// 3. prefer lattice structure
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
           int PointCnt[2])
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
                ll tempScore =
                    getMove(player, GameBoard, depth + 1, score + deltaScore,
                            (currPlayer == BLUE) ? RED : BLUE, BestMove, alpha,
                            beta, maxDepth, PointList, PointCnt);

                if (isMaximizing == 1)
                {
                    if (tempScore > bestScore)
                    {
                        bestScore = tempScore;
                        if (depth == 0) *BestMove = Move;
                    }
                    if (bestScore > alpha) alpha = bestScore;
                }
                else
                {
                    if (tempScore < bestScore)
                    {
                        bestScore = tempScore;
                        if (depth == 0) *BestMove = Move;
                    }
                    if (bestScore < beta) beta = bestScore;
                }

                GameBoard[Move] = EMPTY;
                PointCnt[currPlayer - 1]--;
                if (alpha >= beta)
                {
                    if (isMaximizing == 1) return alpha;
                    else return beta;
                }
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

    int BestMove = -1, maxDepth = 6;
    if (empty == 0) return 0;

    getMove(player, GameBoard, 0, 0, player, &BestMove, LLONG_MIN, LLONG_MAX,
            maxDepth, PointList, PointCnt);

    return BestMove;
}