#pragma once

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point p1, p2, p3, p4;
} Square;

int ai_player(int player, const int *board);

Point num2point(int Move);
int point2num(Point Move);
int eq_point(Point a, Point b);
int max(int a, int b);
int validate_square(const Square sq,
                    const int Player,
                    const int GameBoard[],
                    const char done[]);
void get_squares(const Point p1,
                 const Point p2,
                 Square *const sq1,
                 Square *const sq2);
void register_square(const Square sq,
                     char done[],
                     const int score,
                     const int Player);
int validate_input(const int Move, const int GameBoard[]);

void get_squares(const Point p1,
                 const Point p2,
                 Square *const sq1,
                 Square *const sq2);

void register_square(const Square sq,
                     char done[],
                     const int score,
                     const int Player);

int validate_square(const Square sq,
                    const int Player,
                    const int GameBoard[],
                    const char done[]);

int new_squares_score(const int Move, const int Player, const int GameBoard[], int ShouldPrint);