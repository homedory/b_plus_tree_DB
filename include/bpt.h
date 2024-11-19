#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#define LEAF_MAX 31
#define INTERNAL_MAX 248

typedef struct record{
    int64_t key;
    char value[120];
}record;

typedef struct inter_record {
    int64_t key;
    off_t p_offset;
}I_R;

typedef struct Page{
    off_t parent_page_offset;
    int is_leaf;
    int num_of_keys;
    char reserved[104];
    off_t next_offset;
    union{
        I_R b_f[248];
        record records[31];
    };
}page;

typedef struct Header_Page{
    off_t fpo;
    off_t rpo;
    int64_t num_of_pages;
    char reserved[4072];
}H_P;


extern int fd;

extern page * rt;

extern H_P * hp;
// FUNCTION PROTOTYPES.
int open_table(char * pathname);
H_P * load_header(off_t off);
page * load_page(off_t off);

void reset(off_t off);
off_t new_page();
void freetouse(off_t fpo);
int cut(int length);
int parser();
void start_new_file(record rec);

// Find Utitlity Function
off_t find_leaf(int key);

// Insertion Utility Functions
record * make_record(int64_t key, char * value);
int get_left_index(off_t parent_page_offest, off_t left_page_offset);
void insert_into_leaf(page * leaf, off_t leaf_page_offset, record * new_record);
void insert_into_leaf_after_splitting(page * leaf, off_t leaf_page_offset, record * new_record);

void insert_into_node();
void insert_into_node_after_splitting();

void insert_into_parent(page * left, off_t left_page_offset, int64_t key, page * right, off_t right_page_offset);
void insert_into_new_root(page * left, off_t left_offset, int64_t key, page * right, off_t right_offset)
// Deletion Utility Functions


char * db_find(int64_t key);
int db_insert(int64_t key, char * value);
int db_delete(int64_t key);

#endif /* __BPT_H__*/


