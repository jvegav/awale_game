#include <stdio.h>


void printmatrix_jouer1(int m[2][6]){
    for (int i = 0; i < 2; i++){
        for (int j = 0; j<6 ; j++){
            printf("[%d] ", m[i][j]);
            
        }
        printf("\n");
    }
}

void printmatrix_jouer2(int m[2][6]){
    for (int i = 1; i >= 0; i--){
        for (int j = 5; j>=0 ; j--){
            printf("[%d] ", m[i][j]);
            
        }
        printf("\n");
    }
}

void init_game(int matrix[2][6]) {
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 6; j++) {
            matrix[i][j] = 1;
        }
    }
}

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



int main() {
    int matrix [2][6];

    init_game(matrix);
    int turn=1;

    int score_j1 =0;
    int score_j2 =0;    

    matrix[1][5] = 6;

    while(1){

        printf("----------------------------------------------------------------------------------------\n");

        

       if(peut_jouer(matrix) == 0){

            printf("\n");
            printf("Le jeu est termine!\n");
            printf("\n");
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

        if (turn == 1) {
            printf("Tour du joueur 1\n");
            printmatrix_jouer1(matrix);
        } else {
            printf("Tour du joueur 2\n");
             printmatrix_jouer2(matrix);
            turn = 2;
            
        }

        

        int selection;
        printf("Choisi une cologne (1-6): ");
        scanf("%d", &selection);

        if (selection < 1 || selection > 6) {
            printf("Selection invalide. Choisi une cologne entre 1 et 6.\n");
            printf("----------------------------------------------------------------------------------------\n");
            continue;
        }

        int index  = selection -1 ;
        int index_j = 1;
        
        if (turn ==1){
            int seeds = matrix[1][index];
            matrix[index_j][index] = 0;
            if(index == 5) {
                index_j = 0;
            }
            else {
                index ++;
            }

            for(int s = seeds; s>0; s--){
                

                if(s ==1 && index_j ==0 && (0 < matrix[index_j][index] && matrix[index_j][index] < 3)){
                    printf("----------------------------------------------------------------------------------------\n");
                    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    printf("CAPTURE!\n");
                    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                    score_j1 += matrix[index_j][index] +1;
                    matrix[index_j][index] =0;
                    for(int i=0;  i< (5-index); i--){
                        index ++;
                        if (2 == matrix[index_j][index] || matrix[index_j][index] ==3){
                            score_j1 += matrix[index_j][index] ;
                            matrix[index_j][index] =0;
                        }
                        else{
                            break;
                        }
                        }
                    
                    
                }

                else if (index == 5 & index_j == 1){
                    matrix[index_j][index] ++;
                    index_j = 0;
                }
                else if(index == 0 & index_j == 0){
                    matrix[index_j][index] ++;
                    index_j = 1;
                }
                else if (index_j == 0){
                    matrix[index_j][index] ++;
                    index --;
                }
                else if (index_j == 1){
                    matrix[index_j][index] ++;
                    index ++;
                } 
               
            }
            turn =2;
        }
        else if(turn ==2){
            int index_2 = 6 - selection;
            int seeds = matrix[0][index_2];
            index_j =0;
            matrix[index_j][index_2] =0;

            if(index_2 ==0) {
                index_j = 1;
            }
            else {
                index_2 --;
            }

            for(int s = seeds; s>0; s--){
                if(s ==1 && index_j ==1 && (0 < matrix[index_j][index_2] && matrix[index_j][index_2] < 3)){
                    score_j2 += matrix[index_j][index_2]+1;
                    matrix[index_j][index_2] =0;
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
                else if (index_2 == 0 & index_j ==0){
                    matrix[index_j][index_2] ++;
                    index_j = 1;
                }
                else if(index_2 == 5 & index_j ==1){
                    matrix[index_j][index_2] ++;
                    index_j = 0;
                }
                else if (index_j == 0){
                    matrix[index_j][index_2] ++;
                    index_2 --;
                }
                else if (index_j == 1){
                    matrix[index_j][index_2] ++;
                    index_2 ++;
                } 
            }
            turn =1;
        }

        printf("----------------------------------------------------------------------------------------\n");

        printf("Score joueur 1: %d\n", score_j1);
        printf("Score joueur 2: %d\n", score_j2);


    }

    return 0 ;
}

