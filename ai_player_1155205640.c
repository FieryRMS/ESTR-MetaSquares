#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATASIZE 8
// clang-format off
const int TemplatePattern[8][8] = {
    {  1,   0,   1,   0,   1,   0,   1,   0},
    {  0,   1,   0,   1,   0,   0,   0,   0},
    {  1,   0,   1,   0,   1,   0,   1,   0},
    {  0,   1,   0,   1,   0,   0,   0,   1},
    {  1,   0,   1,   0,   1,   0,   1,   0},
    {  0,   0,   0,   0,   0,   0,   0,   1},
    {  1,   0,   1,   0,   1,   0,   1,   0},
    {  0,   0,   0,   1,   0,   1,   0,   0}
};
//clang-format on

enum {POSSIBLE_SQUARES, POSSIBLE_SCORE, PATTERN_SQUARES, PATTERN_SCORE, MY_SQUARES, MY_SCORE, OPPONENT_SQUARES, OPPONENT_SCORE, DEPTH, MAGIC_RATIO_CONSTANT};

double weights[DATASIZE+2] = {
    1, // possible squares
    1, // possible score
    1, // pattern squares
    1, // pattern score
    1, // my squares
    1, // my score
    50, // opponent squares
    50, // opponent score
    6, // depth
    7.81  // magic ratio constant
};

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
#define DEBUGS debug = 1
#define DEBUGE debug = 0

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
#define DEBUGS debug = 0
#define DEBUGE debug = 0
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

Point rotatePoint(Point p, int angle)
{
    switch (angle)
    {
        case 0:
            return p;
        case 1:
        case 90:
            return (Point){ 9 - p.y, p.x };
        case 2:
        case 180:
            return (Point){ 9 - p.x, 9 - p.y };
        case 3:
        case -90:
        case 270:
            return (Point){ p.y, 9 - p.x };
    }
    return p;
}

int CalculateTemplatePatternData(int TemplatePatternData[4][2][8][8],
                                 int TotalScore[4],
                                 const int GameBoard[],
                                 const int player,
                                 Point a,
                                 Point b,
                                 Point c,
                                 Point d,
                                 int mxRow,
                                 int bestIdx)
{
    int Opponent = (player == BLUE ? RED : BLUE);
    int highest = TotalScore[bestIdx];
    Point cpy[4] = { a, b, c, d };

    if (GameBoard[point2num(a)] == Opponent ||
        GameBoard[point2num(b)] == Opponent ||
        GameBoard[point2num(c)] == Opponent ||
        GameBoard[point2num(d)] == Opponent)
        return bestIdx;

    for (int i = 0; i < 4; i++)
    {
        if (TemplatePattern[a.x - 1][a.y - 1] &&
            TemplatePattern[b.x - 1][b.y - 1] &&
            TemplatePattern[c.x - 1][c.y - 1] &&
            TemplatePattern[d.x - 1][d.y - 1])
        {
            for (int j = 0; j < 4; j++)
                TemplatePatternData[i][0][cpy[j].x - 1][cpy[j].y - 1] += 1,
                    TemplatePatternData[i][1][cpy[j].x - 1][cpy[j].y - 1] +=
                    mxRow;

            TotalScore[i] += mxRow;

            if (TotalScore[i] > highest)
            {
                highest = TotalScore[i];
                bestIdx = i;
            }
        }
        a = rotatePoint(a, -90);
        b = rotatePoint(b, -90);
        c = rotatePoint(c, -90);
        d = rotatePoint(d, -90);
    }

    return bestIdx;
}

void CalculatePossibleData(const int GameBoard[],
                           int DATA[DATASIZE][8][8],
                           const int player,
                           int *BlueScore,
                           int *RedScore)
{
    int Opponent = (player == BLUE ? RED : BLUE);
    int TemplatePatternData[4][2][8][8] = { 0 }, TotalScore[4] = { 0 };
    int bestIdx = 0;
    for (int i = 0; i < 64; i++)
        for (int j = i + 1; j < 64; j++)
        {
            Point a = { i / 8 + 1, i % 8 + 1 }, b = { j / 8 + 1, j % 8 + 1 };
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
            if (validate_input(point2num(a), GameBoard) != ERR_OUT_OF_BOUND &&
                validate_input(point2num(b), GameBoard) != ERR_OUT_OF_BOUND &&
                validate_input(point2num(c), GameBoard) != ERR_OUT_OF_BOUND &&
                validate_input(point2num(d), GameBoard) != ERR_OUT_OF_BOUND)
            {
                int GBa = GameBoard[point2num(a)],
                    GBb = GameBoard[point2num(b)],
                    GBc = GameBoard[point2num(c)],
                    GBd = GameBoard[point2num(d)];
                if (GBa != Opponent && GBb != Opponent && GBc != Opponent &&
                    GBd != Opponent)
                {
                    DATA[0][a.x - 1][a.y - 1] += 1;
                    DATA[0][b.x - 1][b.y - 1] += 1;
                    DATA[0][c.x - 1][c.y - 1] += 1;
                    DATA[0][d.x - 1][d.y - 1] += 1;
                    DATA[1][a.x - 1][a.y - 1] += mxRow;
                    DATA[1][b.x - 1][b.y - 1] += mxRow;
                    DATA[1][c.x - 1][c.y - 1] += mxRow;
                    DATA[1][d.x - 1][d.y - 1] += mxRow;

                    bestIdx = CalculateTemplatePatternData(
                        TemplatePatternData, TotalScore, GameBoard, player, a,
                        b, c, d, mxRow, bestIdx);
                }
                int playerCnt = 0, OppCnt = 0;
                playerCnt += (GBa == player) + (GBb == player) +
                             (GBc == player) + (GBd == player);
                OppCnt += (GBa == Opponent) + (GBb == Opponent) +
                          (GBc == Opponent) + (GBd == Opponent);
                if (playerCnt == 4)
                {
                    if (player == BLUE) *BlueScore += mxRow;
                    else *RedScore += mxRow;
                }
                else if (OppCnt == 4)
                {
                    if (player == BLUE) *RedScore += mxRow;
                    else *BlueScore += mxRow;
                }
                if (playerCnt > 1 && !OppCnt)
                {
                    mxRow *= playerCnt, playerCnt *= playerCnt;
                    DATA[4][a.x - 1][a.y - 1] += playerCnt;
                    DATA[4][b.x - 1][b.y - 1] += playerCnt;
                    DATA[4][c.x - 1][c.y - 1] += playerCnt;
                    DATA[4][d.x - 1][d.y - 1] += playerCnt;
                    DATA[5][a.x - 1][a.y - 1] += mxRow;
                    DATA[5][b.x - 1][b.y - 1] += mxRow;
                    DATA[5][c.x - 1][c.y - 1] += mxRow;
                    DATA[5][d.x - 1][d.y - 1] += mxRow;
                }
                if (!playerCnt && OppCnt > 1)
                {
                    mxRow *= OppCnt, OppCnt *= OppCnt;
                    DATA[6][a.x - 1][a.y - 1] += OppCnt;
                    DATA[6][b.x - 1][b.y - 1] += OppCnt;
                    DATA[6][c.x - 1][c.y - 1] += OppCnt;
                    DATA[6][d.x - 1][d.y - 1] += OppCnt;
                    DATA[7][a.x - 1][a.y - 1] += mxRow;
                    DATA[7][b.x - 1][b.y - 1] += mxRow;
                    DATA[7][c.x - 1][c.y - 1] += mxRow;
                    DATA[7][d.x - 1][d.y - 1] += mxRow;
                }
            }
        }

    if (0 && debug)
    {
        // print template pattern data
        printf("bestIdx: %d\n", bestIdx);
        printf("TemplatePatternData:\n");
        for (int i = 0; i < 4; i++)
        {
            printf("TemplatePatternData[%d]:\n", i);
            printf("TotalScore[%d]: %d\n", i, TotalScore[i]);
            for (int j = 0; j < 2; j++)
            {
                printf("TemplatePatternData[%d][%d]:\n", i, j);
                for (int k = 0; k < 8; k++)
                {
                    for (int l = 0; l < 8; l++)
                        printf("%*d ", 3, TemplatePatternData[i][j][k][l]);
                    printf("\n");
                }
            }
        }
    }

    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            DATA[2][i][j] = TemplatePatternData[bestIdx][0][i][j],
            DATA[3][i][j] = TemplatePatternData[bestIdx][1][i][j];
}

typedef struct {
    int index;
    long double value;
} IndexValue;

int compareHeuristic(const void *a, const void *b)
{
    IndexValue *ia = (IndexValue *)a, *ib = (IndexValue *)b;
    return (int)(ib->value - ia->value);
}
void calculate_heuristics(const int player,
                          const int GameBoard[],
                          int SearchPointList[64],
                          int *BlueScore,
                          int *RedScore)
{
    int DATA[DATASIZE][8][8] = {};
    CalculatePossibleData(GameBoard, DATA, player, BlueScore, RedScore);

    if (1 && debug)
    {
        printf("DATA:\n");
        for (int i = 0; i < DATASIZE; i++)
        {
            printf("DATA[%d]:\n", i);
            for (int j = 0; j < 8; j++)
            {
                for (int k = 0; k < 8; k++)
                {
                    if (GameBoard[point2num((Point){ j + 1, k + 1 })] == BLUE)
                        printf("%*s ", 3, "#");
                    else if (GameBoard[point2num((Point){ j + 1, k + 1 })] ==
                             RED)
                        printf("%*s ", 3, "@");
                    else printf("%*d ", 3, DATA[i][j][k]);
                }
                printf("\n");
            }
        }
    }

    IndexValue HeuristicPointList[64];
    int HeuristicPointListCnt = 0;
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            if (validate_input(point2num((Point){ i + 1, j + 1 }), GameBoard) ==
                ERR_NONE)
            {
                HeuristicPointList[HeuristicPointListCnt].index =
                    point2num((Point){ i + 1, j + 1 });
                HeuristicPointList[HeuristicPointListCnt].value = 0;
                for (int k = 0; k < DATASIZE; k++)
                    HeuristicPointList[HeuristicPointListCnt].value +=
                        weights[k] * DATA[k][i][j];
                HeuristicPointListCnt++;
            }

    qsort(HeuristicPointList, HeuristicPointListCnt, sizeof(IndexValue),
          compareHeuristic);

    if (1 && debug)
    {
        printf("HeuristicPointList:\n");
        for (int i = 0; i < HeuristicPointListCnt; i++)
            printf("%d: %Lf\n", HeuristicPointList[i].index,
                   HeuristicPointList[i].value);
    }

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
// 7. killer heuristics
ll getMove(const int player,
           int GameBoard[],
           int depth,
           ll score,
           ll BlueScore,
           ll RedScore,
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

    int GameState = is_game_over(GameBoard, BlueScore, RedScore);
    if (GameState == DRAW) return 0;
    else if (GameState) return (100000 - depth) * isMaximizing * -1;

    if (depth == maxDepth) return score;

    int SearchPointList[64] = {}, tempBlueScore = 0, tempRedScore = 0;
    debug = 0;
    calculate_heuristics(player, GameBoard, SearchPointList, &tempBlueScore,
                         &tempRedScore);
    DEBUGE;
    if (depth == 0) BlueScore = tempBlueScore, RedScore = tempRedScore;
    for (int i = 0; i < 20; i++)
    {
        if (SearchPointList[i] == 0) break;
        int Move = SearchPointList[i];
        if (validate_input(Move, GameBoard) == ERR_NONE)
        {
            GameBoard[Move] = currPlayer;
            PointList[currPlayer - 1][PointCnt[currPlayer - 1]++] =
                num2point(Move);
            ll deltaScore = new_squares_score(Move, currPlayer, GameBoard, 0,
                                              PointList[currPlayer - 1],
                                              PointCnt[currPlayer - 1]);

            int newBlueScore = BlueScore, newRedScore = RedScore;
            if (currPlayer == BLUE) newBlueScore += deltaScore;
            else newRedScore += deltaScore;
            deltaScore = deltaScore * isMaximizing * 10;
            if (deltaScore) deltaScore -= depth * isMaximizing;

            int BestSequenceRet[64];

            ll tempScore = getMove(
                player, GameBoard, depth + 1, score + deltaScore, newBlueScore,
                newRedScore, (currPlayer == BLUE) ? RED : BLUE, BestMove, alpha,
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

    return bestScore;
}

int ai_player(int player, const int *board)
{
    int GameBoard[89] = {}, empty = 0, PointCnt[2] = { 0 };
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
    if (empty == 0) return 0;

    int BestMove = -1, maxDepth = weights[DEPTH], BestSequence[64] = { 0 };

    int branching_factor = powl(10, weights[MAGIC_RATIO_CONSTANT / maxDepth]);
    if (branching_factor > empty)
    {
        branching_factor = empty;
        maxDepth = weights[MAGIC_RATIO_CONSTANT] / log10l(empty);
    }


    getMove(player, GameBoard, 0, 0, 0, 0, player, &BestMove,
            LLONG_MIN, LLONG_MAX, maxDepth, PointList, PointCnt, BestSequence);

    DEBUGE;
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