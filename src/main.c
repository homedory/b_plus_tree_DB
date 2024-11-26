#include "bpt.h"

#include <time.h>

int main(){
    int64_t input;
    char instruction;
    char buf[120];
    char *result;

    struct timespec start_time, end_time;
    double execution_time;

    // Start measuring time
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    open_table("test.db");
    while(scanf("%c", &instruction) != EOF){
        switch(instruction){
            case 'i':
                scanf("%ld %s", &input, buf);
                if(db_insert(input, buf) != 0)
                    printf("Insertion Failed!!!!!!!!\n\n");
                else 
                    // printf("Insertion Success: key %ld, value %s\n\n", input, buf);
                break;
            case 'f':
                scanf("%ld", &input);
                result = db_find(input);
                if (result) {
                    printf("Key: %ld, Value: %s\n", input, result);
                    free(result);   
                }
                else
                    printf("Not Exists\n");

                fflush(stdout);
                break;
            case 'd':
                scanf("%ld", &input);
                if (db_delete(input) != 0) 
                    printf("Deletion Failed\n");
                break;
            case 'q':
                while (getchar() != (int)'\n');

                // Stop measuring time
                clock_gettime(CLOCK_MONOTONIC, &end_time);
                
                // Calculate total execution time
                execution_time = (end_time.tv_sec - start_time.tv_sec) +
                                 (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
                printf("Total Execution Time: %.2f seconds\n", execution_time);

                return EXIT_SUCCESS;
                break;
        }
        while (getchar() != (int)'\n');
    }

    // Stop measuring time
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    // Calculate total execution time
    execution_time = (end_time.tv_sec - start_time.tv_sec) +
                     (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    printf("Total Execution Time: %.2f seconds\n", execution_time);


    printf("\n");
    return 0;
}



