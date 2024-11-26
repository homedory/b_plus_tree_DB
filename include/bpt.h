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
void start_new_file(record rec);

// Find Utitlity Function
off_t find_leaf(int64_t key);

// Insertion Utility Functions
int get_child_index_in_parent(off_t parent_offest, off_t child_offset);
void insert_into_leaf(page * leaf, off_t leaf_offset, record new_record);
void insert_into_leaf_after_splitting(page * leaf, off_t leaf_offset, record new_record);

void insert_into_leaf_using_key_rotation(page * leaf, off_t leaf_offset, record new_record);
bool has_room_for_record(off_t leaf_offset);
void update_key_for_key_rotation(page * leaf, off_t leaf_offset, int64_t key);

void insert_into_node(page * parent, off_t parent_offset,
        int left_index, int64_t key, page * right, off_t right_offset);
void insert_into_node_after_splitting(page * old_node, off_t old_node_offset, int left_index,
        int64_t key, page * right, off_t right_offset);

void insert_into_parent(page * left, off_t left_offset, int64_t key, page * right, off_t right_offset);
void insert_into_new_root(page * left, off_t left_offset, int64_t key, page * right, off_t right_offset);

// Deletion Utility Functions
int get_neighbor_index(page * node, off_t node_offset);
void remove_entry_from_node(page * node, int64_t key);

void adjust_root(page * root, off_t root_offset);
void coalesce_nodes(page * node, off_t node_offset, page * neighbor, off_t neighbor_offset,
        int neighbor_index, int64_t k_prime);
void redistribute_nodes(page * node, off_t node_offset, page * neighbor, off_t neighbor_offset,
        int neighbor_index, int k_prime_index, int k_prime);

void delete_entry(page * node, off_t node_offset, int64_t key);

off_t find_leaf_if_value_exists(int64_t key);

// Master Functions
char * db_find(int64_t key);
int db_insert(int64_t key, char * value);
int db_delete(int64_t key);


#endif /* __BPT_H__*/


