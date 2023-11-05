#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXDEPTH 7

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point p1, p2, p3, p4;
} Square;

enum { EMPTY, BLUE, RED };
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

int eq_point(Point a, Point b)
{
    return a.x == b.x && a.y == b.y;
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

void printBoard(const int GameBoard[])
{
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

#else
void PrintBoard(const int GameBoard[]) {}
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
void get_squares(const Point p1,
                 const Point p2,
                 Square *const sq1,
                 Square *const sq2)
{
    int dx = p1.x - p2.x, dy = p1.y - p2.y;
    *sq1 =
        (Square){ { p1.x + dy, p1.y - dx }, { p2.x + dy, p2.y - dx }, p2, p1 };
    *sq2 =
        (Square){ { p1.x - dy, p1.y + dx }, { p2.x - dy, p2.y + dx }, p2, p1 };
}

/**
 * prints the square and registers it in done[]
 */
void register_square(const Square sq,
                     char done[],
                     const int score,
                     const int Player,
                     int ShouldPrint)
{
    int num[4] = { point2num(sq.p1), point2num(sq.p2), point2num(sq.p3),
                   point2num(sq.p4) };
    for (int i = 0; i < 4; i++) done[num[i]] = 1;

    if (ShouldPrint)
        printf(
            "%s gains %d more points by formulating the squre {%d, %d, "
            "%d, %d}\n",
            Player == BLUE ? "BLUE" : "RED", score, num[0], num[1], num[2],
            num[3]);
}
/**
 * validates and returns possible score
 */
int validate_square(const Square sq,
                    const int Player,
                    const int GameBoard[],
                    const char done[])
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
        GameBoard[p4num] == Player && 
        (
            !done[p1num] ||
            !done[p2num] ||
            !done[p3num] ||
            !done[p4num]
        )
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
                      int ShouldPrint)
{
    Point MovePoint = num2point(Move);
    char done[89] = { 0 };
    int total_score = 0;
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
        {
            Point CurrPoint = (Point){ i, j };
            if (GameBoard[point2num(CurrPoint)] == Player &&
                !eq_point(CurrPoint, MovePoint))
            {
                Square sq1, sq2;
                // {73, 51, 55, 33}
                // {73, 51, 53, 71}
                get_squares(MovePoint, CurrPoint, &sq1, &sq2);
                int score1 = validate_square(sq1, Player, GameBoard, done),
                    score2 = validate_square(sq2, Player, GameBoard, done);

                if (score1)
                    register_square(sq1, done, score1, Player, ShouldPrint);
                if (score2)
                    register_square(sq2, done, score2, Player, ShouldPrint);

                total_score += score1 + score2;
            }
        }

    return total_score;
}

// ideas
// 1. minimax
// 2. alpha-beta pruning
// 3. prefer lattice structure
int getMove(const int *const player,
            int GameBoard[],
            int depth,
            int score,
            int currPlayer,
            int *BestMove,
            int alpha,
            int beta)
{
    int isMaximizing = (*player == currPlayer ? 1 : -1),
        bestScore = isMaximizing == 1 ? 0x80000000 : 0x7fffffff;

    if (depth == MAXDEPTH) return score;

    for (int i = 1; i <= 8; i++)
    {
        for (int j = 1; j <= 8; j++)
        {
            int Move = point2num((Point){ i, j });
            if (validate_input(Move, GameBoard) == ERR_NONE)
            {
                GameBoard[Move] = currPlayer;
                int deltaScore =
                    new_squares_score(Move, currPlayer, GameBoard, 0);
                deltaScore = (deltaScore)*isMaximizing;
                int tempScore = getMove(
                    player, GameBoard, depth + 1, score + deltaScore,
                    (currPlayer == BLUE) ? RED : BLUE, BestMove, alpha, beta);

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
    int GameBoard[89];
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            GameBoard[point2num((Point){ i, j })] =
                board[point2num((Point){ i, j })];

    int BestMove = -1;
    getMove(&player, GameBoard, 0, 0, player, &BestMove, 0x80000000,
            0x7fffffff);

    return BestMove;
}