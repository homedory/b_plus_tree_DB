#include "bpt.h"

int main(){
    int64_t input;
    char instruction;
    char buf[120];
    char *result;
    open_table("test.db");
    int input_line = 1;
    while(scanf("%c", &instruction) != EOF){
        printf("\n%d \n", input_line);
        switch(instruction){
            case 'i':
                scanf("%ld %s", &input, buf);
                if(db_insert(input, buf) != 0)
                    printf("Insertion Failed!!!!!!!!\n\n");
                else 
                    printf("Insertion Success: key %ld, value %s\n\n", input, buf);
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
                    printf("----------------------------Deletion Failed-----------------------------\n");
                break;
            case 'q':
                while (getchar() != (int)'\n');
                return EXIT_SUCCESS;
                break;
        }
        print_bpt();
        input_line++;
        while (getchar() != (int)'\n');
    }
    printf("\n");
    return 0;
}



