#include <stdio.h>
#include <string.h>
#include "awale_logic.h"

void init_game(int matrix[ROWS][COLS]) {
    for (int i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
            matrix[i][j] = 4;
}

void matrix_to_string_joueur1(int m[ROWS][COLS], char *output, size_t size) {
    int off = 0;
    off += snprintf(output + off, size - off, "\n");
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            off += snprintf(output + off, size - off, "[%d] ", m[i][j]);
            if (off >= (int)size) break;  // seguridad
        }
        off += snprintf(output + off, size - off, "\n");
        if (off >= (int)size) break;
    }
    output[size - 1] = '\0'; // asegurar null-termination
}


void matrix_to_string_joueur2(int m[ROWS][COLS], char *output, size_t size) {
    int off = 0;
    off += snprintf(output + off, size - off, "\n");
    for (int i = ROWS - 1; i >= 0; i--) {
        for (int j = COLS - 1; j >= 0; j--) {
            off += snprintf(output + off, size - off, "[%d] ", m[i][j]);
            if (off >= (int)size) break;  // seguridad
        }
        off += snprintf(output + off, size - off, "\n");
        if (off >= (int)size) break;
    }
    output[size - 1] = '\0';
}

int can_play(int matrix[ROWS][COLS]) {
    int sum1 = 0, sum2 = 0;
    for (int j = 0; j < COLS; j++) {
        sum1 += matrix[0][j];
        sum2 += matrix[1][j];
    }
    return (sum1 > 0 && sum2 > 0);
}

// Simulación mínima de jugada para red (sólo para demostrar)
int process_move(int matrix[ROWS][COLS], int turn, int move, int *score_j1, int *score_j2) {
    
    
    if (move < 1 || move > 6) return 0;
    int idx  = move -1 ;
    int index_j = 1;
        
     if (turn ==1){
            int index_j = 1;
            // know how many seeds are in the selected pit
            int seeds = matrix[1][idx];

            // Empty the selected pit
            matrix[index_j][idx] = 0;

            // If index is at the end of the row, change row
            if(idx == 5) {
                index_j = 0;
            }
            // else continue in the same row
            else {
                idx ++;
            }

            // Distribute the seeds
            for(int s = seeds; s>0; s--){

                // Capture condition
                if(s ==1 && index_j ==0 && (0 < matrix[index_j][idx] && matrix[index_j][idx] < 3)){

                    // Update score and empty the pit
                    (*score_j1) += matrix[index_j][idx] +1;
                    matrix[index_j][idx] =0;

                    // Check for additional captures in the same row
                    // 5- index because we want to check the pits to the left of the current index
                    // index =  1 -> we check the cells that we have passed
                    for(int i=0;  i< (5-idx); i--){
                        // move to the previous pit
                        idx ++;

                        // if there are 2 or 3 seeds, capture them
                        if (2 == matrix[index_j][idx] || matrix[index_j][idx] ==3){
                            (*score_j1)+= matrix[index_j][idx] ;
                            matrix[index_j][idx] =0;
                        }
                        else{
                            break;
                        }
                        }
                    
                    
                }
                // IF we are at the end of the row, change row
                else if (idx == 5 & index_j == 1){
                    matrix[index_j][idx] ++;
                    index_j = 0;
                }
                // If we are at the beginning of the row, change row
                else if(idx == 0 & index_j == 0){
                    matrix[index_j][idx] ++;
                    index_j = 1;
                }
                // Continue distributing seeds in the same row
                else if (index_j == 0){
                    matrix[index_j][idx] ++;
                    idx --;
                }
                // Continue distributing seeds in the same row
                else if (index_j == 1){
                    matrix[index_j][idx] ++;
                    idx ++;
                } 
               
            }
    } else {

        int index_2 = 6 - move;

        // Get the number of seeds in the selected pit
        int seeds = matrix[0][index_2];

            // Empty the selected pit and set row index for player 2
        index_j =0;
        matrix[index_j][index_2] =0;

        // If at the beginning of the row, change row
        if(index_2 ==0) {
                index_j = 1;
        }
        else {
                index_2 --;
        }

        // Distribute the seeds
        for(int s = seeds; s>0; s--){
                // Capture condition
                if(s ==1 && index_j ==1 && (0 < matrix[index_j][index_2] && matrix[index_j][index_2] < 3)){

                    // Update score and empty the pit
                    (*score_j2) += matrix[index_j][index_2]+1;
                    matrix[index_j][index_2] =0;

                    // Check for additional captures in the same row
                    // 5- index because we want to check the pits to the left of the current
                    for(int i=0;  i< (5-index_2); i--){
                        index_2 --;
                        if (2 == matrix[index_j][index_2] || matrix[index_j][index_2] == 3){
                            (*score_j2) += matrix[index_j][index_2];
                            matrix[index_j][index_2] =0;
                        }
                        else{
                            break;
                        }
                    }
                    
                }

                // If at the end of the row, change row
                else if (index_2 == 5 & index_j ==0){
                    matrix[index_j][index_2] ++;
                    index_j = 1;
                }

                // If at the beginning of the row, change row
                else if (index_2 == 0 & index_j ==0){
                    matrix[index_j][index_2] ++;
                    index_j = 1;
                }

                // If at the beginning of the row of the oponnent, change row
                else if(index_2 == 5 & index_j ==1){
                    matrix[index_j][index_2] ++;
                    index_j = 0;
                }
                // Continue distributing seeds in the same row
                else if (index_j == 0){
                    matrix[index_j][index_2] ++;
                    index_2 --;
                }
                // Continue distributing seeds in the same row
                else if (index_j == 1){
                    matrix[index_j][index_2] ++;
                    index_2 ++;
                } 
            }
            
    }
    return 1;
}

    