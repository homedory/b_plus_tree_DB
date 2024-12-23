#include "bpt.h"

int main(){
    int64_t input;
    char instruction;
    char buf[120];
    char *result;
    int table_num;

    set_table_pointer(1);
    open_table("table1.db");
    set_table_pointer(2);
    open_table("table2.db");
    // current table 1 (default)
    set_table_pointer(1);


    while(scanf("%c", &instruction) != EOF){
        switch(instruction){
            case 'i':
                scanf("%ld %s", &input, buf);
                db_insert(input, buf);
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
                db_delete(input);
                break;
            
            // Join
            case 'j':
                db_join();
                break;
            // Chang current table
            case 'c':
                scanf("%d", &table_num);
                set_table_pointer(table_num);
                break;

            case 'q':
                while (getchar() != (int)'\n');
                return EXIT_SUCCESS;
                break;   

        }
        while (getchar() != (int)'\n');
    }
    printf("\n");
    return 0;
}



