#include "csv.h"

// Write row major array to csv
int writeCSV(char filename[50], void* array, int type, int rows, int columns) {

    // Create and open file
    FILE *fp;
    fp = fopen(filename, "a");

    // Write array to file
    for (int i = 0; i < rows; i++) {
        for(int j = 0; j < columns; j++){
            switch(type){
                case 1:
                    fprintf(fp, "%d,", ((int*)array)[i*columns+j]);
                    break;
                case 2:
                    fprintf(fp, "%f,", ((double*)array)[i*columns+j]);
                    break;
                default:
                    return -1;
            }
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
    fclose(fp);
    printf("%s file created\n", filename);
    return 0;
}