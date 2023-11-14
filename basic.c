/**
 * ESTR 1002 Problem Solving by Programming
 *
 * Course Project
 *
 * I declare that the assignment here submitted is original
 * except for source material explicitly acknowledged,
 * and that the same or closely related material has not been
 * previously submitted for another course.
 * I also acknowledge that I am aware of University policy and
 * regulations on honesty in academic work, and of the disciplinary
 * guidelines and procedures applicable to breaches of such
 * policy and regulations, as contained in the website.
 *
 * University Guideline on Academic Honesty:
 * http://www.cuhk.edu.hk/policy/academichonesty/
 *
 * Student Name : Raiyan Mohammed Shah
 * Student ID : 1155205640
 * Date : 04/11/2023
 */

#include <stdio.h>
#include <stdlib.h>
#include "ai_player_1155205640.h"

enum { EMPTY, BLUE, RED, DRAW };
enum { ERR_NONE, ERR_OUT_OF_BOUND, ERR_OCCUPIED };

void make_move(const int Move,
               const int Player,
               int GameBoard[],
               Point PointList[2][64],
               int PointCnt[2])
{
    if (validate_input(Move, GameBoard) != ERR_NONE)
    {
        printf("Unexpected invalid move: %d", Move);
        exit(validate_input(Move, GameBoard));
    }

    if (Player == BLUE)
    {
        printf("BLUE moves to %d\n", Move);
        GameBoard[Move] = BLUE;
        PointList[0][PointCnt[0]++] = num2point(Move);
    }
    else
    {
        printf("RED moves to %d\n", Move);
        GameBoard[Move] = RED;
        PointList[1][PointCnt[1]++] = num2point(Move);
    }
}
/**
 *  Function #1: Initialize the game board
 */
void init(int GameBoard[],
          int *RedScore,
          int *BlueScore,
          int *Player,
          int PointCnt[2])
{
    for (int i = 1; i < 9; i++)
        for (int j = 1; j < 9; j++)
            GameBoard[point2num((Point){ i, j })] = EMPTY;
    PointCnt[0] = PointCnt[1] = 0;
    *RedScore = 0;
    *BlueScore = 0;
    *Player = BLUE;
}

/**
 *  Function #2: Display user interface
 */
void printUI(const int GameBoard[],
             const int RedScore,
             const int BlueScore,
             const int Player)
{
    printf("This is %s's turn\n", Player == BLUE ? "BLUE" : "RED");
    printf("[ Blue's Score=%d ; Red's Score=%d ]\n", BlueScore, RedScore);
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

/**
 * Function #3: Read a location from the user.
 */
int input_move(const int Player, const int GameBoard[])
{
    int Move;
    printf("%d to move: ", Player);

    do {
        scanf("%d", &Move);
        switch (validate_input(Move, GameBoard))
        {
            case ERR_OUT_OF_BOUND:
                printf(
                    "Input out of the game board, please input again (in [11, "
                    "88]): ");
                break;
            case ERR_OCCUPIED:
                printf(
                    "Input invalid, the provided location is not empty, "
                    "please input again: ");
                break;
            case ERR_NONE:
                return Move;
        }
    } while (1);
}

int main()
{
    int GameBoard[89], RedScore, BlueScore, Player, GameMode,
        PointCnt[2] = { 0 };
    Point PointList[2][64];

    init(GameBoard, &RedScore, &BlueScore, &Player, PointCnt);
    printf("Choose the game mode: ");
    scanf("%d", &GameMode);
    int gameState = 0;
    printUI(GameBoard, RedScore, BlueScore, Player);
    do {
        int Move;
        if ((GameMode == 2 && Player == RED) ||
            (GameMode == 3 && Player == BLUE))
            Move = ai_player(Player, GameBoard);
        else Move = input_move(Player, GameBoard);
        make_move(Move, Player, GameBoard, PointList, PointCnt);
        int score =
            new_squares_score(Move, Player, GameBoard, 1, PointList[Player - 1],
                              PointCnt[Player - 1]);
        if (Player == BLUE) BlueScore += score;
        else RedScore += score;
        Player = Player == BLUE ? RED : BLUE;
        printUI(GameBoard, RedScore, BlueScore, Player);
    } while (!(gameState = is_game_over(GameBoard, BlueScore, RedScore)));

    if (gameState == BLUE) printf("Game over\nblue wins!\n");
    else if (gameState == RED) printf("Game over\nred wins!\n");
    else printf("Game over\nDraw!\n");
}
