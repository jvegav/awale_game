#include <stddef.h>
#ifndef AWALE_LOGIC_H
#define AWALE_LOGIC_H

#define ROWS 2
#define COLS 6
#define BUF_SIZE 8001

void init_game(int matrix[ROWS][COLS]);
void matrix_to_string_joueur1(int m[ROWS][COLS], char *output,size_t size);
void matrix_to_string_joueur2(int m[ROWS][COLS], char *output,size_t size);
int process_move(int matrix[ROWS][COLS], int turn, int move, int *score_j1, int *score_j2);
int can_play(int matrix[ROWS][COLS]);

#endif
