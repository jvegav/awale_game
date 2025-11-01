#include <stdio.h>


// Print matrix for player 1 view
void printmatrix_jouer1(int m[2][6]){
    for (int i = 0; i < 2; i++){
        for (int j = 0; j<6 ; j++){
            printf("[%d] ", m[i][j]);
            
        }
        printf("\n");
    }
}

// Print matrix for player 2 view
void printmatrix_jouer2(int m[2][6]){
    for (int i = 1; i >= 0; i--){
        for (int j = 5; j>=0 ; j--){
            printf("[%d] ", m[i][j]);
            
        }
        printf("\n");
    }
}

//  Create the matrix and initialize all pits with 4 seed
void init_game(int matrix[2][6]) {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 6; j++) {
            matrix[i][j] = 4;
        }
    }
}

// Function to check if a player can still play so the game can continue
int peut_jouer(int matrix[2][6]) {
    int totale;
    for (int j = 0; j < 2; j++) {
        for (int i = 0; i < 6; i++) {
            totale += matrix[j][i];
        }
        if (totale == 0) {
            return 0;
        }
    }
    return 1; 
}


// Main function to run the game loop
int main() {
    // Initialize the game matrix
    int matrix [2][6];

    // Create the matrix
    init_game(matrix);

    // Variables to track turns and scores
    int turn=1;
    int score_j1 =0;
    int score_j2 =0;    


    // Game loop
    while(1){

        printf("----------------------------------------------------------------------------------------\n");


        // Check if any player can still play
       if(peut_jouer(matrix) == 0){

            printf("\n");
            printf("Le jeu est termine!\n");
            printf("\n");

            // Calculate final scores
            for (int j = 0; j < 6; j++) {
                score_j1 += matrix[1][j];
                score_j2 += matrix[0][j];
            }
            if (score_j1 > score_j2){
                printf("Le joueur 1 gagne avec un score de %d contre %d\n", score_j1, score_j2);
            }
            else if (score_j2 > score_j1){
                printf("Le joueur 2 gagne avec un score de %d contre %d\n", score_j2, score_j1);
            }
            else{
                printf("Egalite! Les deux joueurs ont un score de %d\n", score_j1);
            }
            break;
        }


        // Display current player's turn and the game board
        if (turn == 1) {
            printf("Tour du joueur 1\n");
            printmatrix_jouer1(matrix);
        } else {
            printf("Tour du joueur 2\n");
             printmatrix_jouer2(matrix);
            turn = 2;
        }

        
        // Prompt the current player for their move
        int selection;
        printf("Choisi une cologne (1-6): ");
        scanf("%d", &selection);

        // Validate the player's selection
        if (selection < 1 || selection > 6) {
            printf("Selection invalide. Choisi une cologne entre 1 et 6.\n");
            printf("----------------------------------------------------------------------------------------\n");
            continue;
        }

        // The player enters a number between 1 and 6 so we change it to correspond to the index of the matrix
        int index  = selection -1 ;
        int index_j = 1;
        
        // Process the move based on the current player's turn
        if (turn ==1){

            // know how many seeds are in the selected pit
            int seeds = matrix[1][index];

            // Empty the selected pit
            matrix[index_j][index] = 0;

            // If index is at the end of the row, change row
            if(index == 5) {
                index_j = 0;
            }
            // else continue in the same row
            else {
                index ++;
            }

            // Distribute the seeds
            for(int s = seeds; s>0; s--){

                // Capture condition
                if(s ==1 && index_j ==0 && (0 < matrix[index_j][index] && matrix[index_j][index] < 3)){

                    printf("----------------------------------------------------------------------------------------\n");
                    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    printf("CAPTURE!\n");
                    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

                    // Update score and empty the pit
                    score_j1 += matrix[index_j][index] +1;
                    matrix[index_j][index] =0;

                    // Check for additional captures in the same row
                    // 5- index because we want to check the pits to the left of the current index
                    // index =  1 -> we check the cells that we have passed
                    for(int i=0;  i< (5-index); i--){
                        // move to the previous pit
                        index ++;

                        // if there are 2 or 3 seeds, capture them
                        if (2 == matrix[index_j][index] || matrix[index_j][index] ==3){
                            score_j1 += matrix[index_j][index] ;
                            matrix[index_j][index] =0;
                        }
                        else{
                            break;
                        }
                        }
                    
                    
                }
                // IF we are at the end of the row, change row
                else if (index == 5 & index_j == 1){
                    matrix[index_j][index] ++;
                    index_j = 0;
                }
                // If we are at the beginning of the row, change row
                else if(index == 0 & index_j == 0){
                    matrix[index_j][index] ++;
                    index_j = 1;
                }
                // Continue distributing seeds in the same row
                else if (index_j == 0){
                    matrix[index_j][index] ++;
                    index --;
                }
                // Continue distributing seeds in the same row
                else if (index_j == 1){
                    matrix[index_j][index] ++;
                    index ++;
                } 
               
            }

            // Switch turn to the other player
            turn =2;
        }

        // Process move for player 2
        else if(turn ==2){

            // Calculate the correct index for player 2's perspective
            int index_2 = 6 - selection;

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

                    printf("----------------------------------------------------------------------------------------\n");
                    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    printf("CAPTURE!\n");
                    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

                    // Update score and empty the pit
                    score_j2 += matrix[index_j][index_2]+1;
                    matrix[index_j][index_2] =0;

                    // Check for additional captures in the same row
                    // 5- index because we want to check the pits to the left of the current
                    for(int i=0;  i< (5-index_2); i--){
                        index_2 --;
                        if (2 == matrix[index_j][index_2] || matrix[index_j][index_2] == 3){
                            score_j2 += matrix[index_j][index_2];
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
            turn =1;
        }

        // Display current scores

        printf("----------------------------------------------------------------------------------------\n");

        printf("Score joueur 1: %d\n", score_j1);
        printf("Score joueur 2: %d\n", score_j2);


    }

    return 0 ;
}

