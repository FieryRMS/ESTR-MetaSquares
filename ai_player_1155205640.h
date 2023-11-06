#pragma once

typedef long long ll;

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point p1, p2, p3, p4;
} Square;

int ai_player(int player, const int *board);

Point num2point(int Move);

int point2num(Point Move);

int validate_input(const int Move, const int GameBoard[]);

int new_squares_score(const int Move,
                      const int Player,
                      const int GameBoard[],
                      int ShouldPrint,
                      Point PointList[],
                      int PointCnt);

int is_game_over(const int GameBoard[],
                 const int RedScore,
                 const int BlueScore);