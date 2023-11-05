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

enum { EMPTY, BLUE, RED };
enum { ERR_NONE, ERR_OUT_OF_BOUND, ERR_OCCUPIED };

void make_move(const int Move, const int Player, int GameBoard[])
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
    }
    else
    {
        printf("RED moves to %d\n", Move);
        GameBoard[Move] = RED;
    }
}
/**
 *  Function #1: Initialize the game board
 */
void init(int GameBoard[], int *RedScore, int *BlueScore, int *Player)
{
    for (int i = 1; i < 9; i++)
        for (int j = 1; j < 9; j++)
            GameBoard[point2num((Point){ i, j })] = EMPTY;
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
/**
 * Function #6: Check if the game is over.
 */
int is_game_over(const int GameBoard[], const int RedScore, const int BlueScore)
{
    if (BlueScore - RedScore >= 15 && BlueScore > 150)
    {
        printf("Game over\nblue wins!\n");
        return 1;
    }
    if (RedScore - BlueScore >= 15 && RedScore > 150)
    {
        printf("Game over\nred wins!\n");
        return 1;
    }
    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            if (GameBoard[point2num((Point){ i, j })] == EMPTY) return 0;

    if (BlueScore == RedScore) printf("Game over\nDraw!\n");
    else if (BlueScore > RedScore) printf("Game over\nblue wins!\n");
    else printf("Game over\nred wins!\n");
    return 1;
}

int main()
{
    int GameBoard[89], RedScore, BlueScore, Player, GameMode;
    init(GameBoard, &RedScore, &BlueScore, &Player);
    printf("Choose the game mode: ");
    scanf("%d", &GameMode);
    printUI(GameBoard, RedScore, BlueScore, Player);
    do {
        int Move;
        if (GameMode == 2 && Player == RED) Move = ai_player(Player, GameBoard);
        else Move = input_move(Player, GameBoard);
        make_move(Move, Player, GameBoard);
        int score = new_squares_score(Move, Player, GameBoard, 1);
        if (Player == BLUE) BlueScore += score;
        else RedScore += score;
        Player = Player == BLUE ? RED : BLUE;
        printUI(GameBoard, RedScore, BlueScore, Player);
    } while (!is_game_over(GameBoard, RedScore, BlueScore));
}
