#ifndef AWALE_LOGIC_H
#define AWALE_LOGIC_H

#define ROWS 2
#define COLS 6

void init_game(int matrix[ROWS][COLS]);
void matrix_to_string_joueur1(int m[ROWS][COLS], char *output);
void matrix_to_string_joueur2(int m[ROWS][COLS], char *output);
int process_move(int matrix[ROWS][COLS], int turn, int move, int *score_j1, int *score_j2);
int can_play(int matrix[ROWS][COLS]);

#endif
