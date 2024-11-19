#include "bpt.h"

H_P * hp;

page * rt = NULL; //root is declared as global

int fd = -1; //fd is declared as global

const leaf_order = 32;
const internal_order = 249;

const record_value_size = 120;


H_P * load_header(off_t off) {
    H_P * newhp = (H_P*)calloc(1, sizeof(H_P));
    if (sizeof(H_P) > pread(fd, newhp, sizeof(H_P), 0)) {

        return NULL;
    }
    return newhp;
}


page * load_page(off_t off) {
    page* load = (page*)calloc(1, sizeof(page));
    //if (off % sizeof(page) != 0) printf("load fail : page offset error\n");
    if (sizeof(page) > pread(fd, load, sizeof(page), off)) {

        return NULL;
    }
    return load;
}

int open_table(char * pathname) {
    fd = open(pathname, O_RDWR | O_CREAT | O_EXCL | O_SYNC  , 0775);
    hp = (H_P *)calloc(1, sizeof(H_P));
    if (fd > 0) {
        //printf("New File created\n");
        hp->fpo = 0;
        hp->num_of_pages = 1;
        hp->rpo = 0;
        pwrite(fd, hp, sizeof(H_P), 0);
        free(hp);
        hp = load_header(0);
        return 0;
    }
    fd = open(pathname, O_RDWR|O_SYNC);
    if (fd > 0) {
        //printf("Read Existed File\n");
        if (sizeof(H_P) > pread(fd, hp, sizeof(H_P), 0)) {
            return -1;
        }
        off_t r_o = hp->rpo;
        rt = load_page(r_o);
        return 0;
    }
    else return -1;
}

// fucnction to reset a page when making a new free page
void reset(off_t off) {
    page * reset;
    reset = (page*)calloc(1, sizeof(page));
    reset->parent_page_offset = 0;
    reset->is_leaf = 0;
    reset->num_of_keys = 0;
    reset->next_offset = 0;
    pwrite(fd, reset, sizeof(page), off);
    free(reset);
    return;
}

// function to free page before use
void freetouse(off_t fpo) {
    page * reset;
    reset = load_page(fpo);
    reset->parent_page_offset = 0;
    reset->is_leaf = 0;
    reset->num_of_keys = 0;
    reset->next_offset = 0;
    pwrite(fd, reset, sizeof(page), fpo);
    free(reset);
    return;
}

// function to free page and add it to header page
void usetofree(off_t wbf) {
    page * utf = load_page(wbf);
    utf->parent_page_offset = hp->fpo;
    utf->is_leaf = 0;
    utf->num_of_keys = 0;
    utf->next_offset = 0;
    pwrite(fd, utf, sizeof(page), wbf);
    free(utf);
    hp->fpo = wbf;
    pwrite(fd, hp, sizeof(hp), 0);
    free(hp);
    hp = load_header(0);
    return;
}


// function to get a free page
off_t new_page() {
    off_t newp;
    page * np;
    off_t prev;
    if (hp->fpo != 0) {
        newp = hp->fpo;
        np = load_page(newp);
        hp->fpo = np->parent_page_offset;
        pwrite(fd, hp, sizeof(hp), 0);
        free(hp);
        hp = load_header(0);
        free(np);
        freetouse(newp);
        return newp;
    }
    //change previous offset to 0 is needed
    newp = lseek(fd, 0, SEEK_END);
    //if (newp % sizeof(page) != 0) printf("new page made error : file size error\n");
    reset(newp);
    hp->num_of_pages++;
    pwrite(fd, hp, sizeof(H_P), 0);
    free(hp);
    hp = load_header(0);
    return newp;
}



int cut(int length) {
    if (length % 2 == 0)
        return length / 2;
    else
        return length / 2 + 1;
}



void start_new_file(record rec) {

    page * root;
    off_t ro;
    ro = new_page();
    rt = load_page(ro);
    hp->rpo = ro;
    pwrite(fd, hp, sizeof(H_P), 0);
    free(hp);
    hp = load_header(0);
    rt->num_of_keys = 1;
    rt->is_leaf = 1;
    rt->records[0] = rec;
    pwrite(fd, rt, sizeof(page), hp->rpo);
    free(rt);
    rt = load_page(hp->rpo);
    //printf("new file is made\n");
}


// Find Utitlity Function

off_t find_leaf(int key) {
    off_t next_page_offset;
    int i = 0;

    // Empty Tree, return header page offset
    if (rt == NULL) {
        return 0;
    }
    page * c = (page*)calloc(1, sizeof(page)); 
    memcpy(c, rt, sizeof(page)); // start from root page
    
    next_page_offset = hp->rpo;
    while (!c->is_leaf) {
        i = 0;
        while (i < c->num_of_keys) {
            if (key >= c->b_f[i].key) i++;
            else break;
        }
        next_page_offset = c->b_f[i].p_offset;
        c = load_page(next_page_offset);
    }

    free(c);
    return next_page_offset;
}


// Insertion Utility Functions


/* Creates a new record to hold the value
 * to which a key refers.
 */
record * make_record(int64_t key, char * value) {
    record *new_record = (record*)calloc(1, sizeof(record));
    if (new_record == NULL) {
        perror("record creation.");
        exit(EXIT_FAILURE);
    }
    new_record->key = key;
    memcpy(new_record->value, value, record_value_size*sizeof(char));
    return new_record;
}

/* Helper function used in insert_into_parent
 * to find the index of the parent's index of branch_factors to 
 * the node to the left of the key to be inserted.
*/
int get_left_index(off_t parent_offest, off_t left_offset) {
    int left_index = 0;
    page * parent = load_page(parent_offest);    
    while (left_index < parent->num_of_keys &&
            parent->b_f[left_index]->p_offset != left_offset)
        left_index++;
    
    // case: left_offset is in leftmost
    if (parent->next_offset == left_offset) {
        free(parent);
        return internal_order - 1;
    }
    
    free(parent);
    return left_index;
}

/* Inserts a new record 
 * including key and value
*/
void insert_into_leaf(page * leaf, off_t leaf_offset, record * new_record) {
    int i , insertion_point;

    insertion_point = 0;
    while(insertion_point < leaf->num_of_keys && 
            leaf->records[insertion_point].key < new_record->key)
        insertion_point++;
    
    for (i = leaf->num_of_keys; i > insertion_point; i--) 
        memcpy(&(leaf->records[i]), &(leaf->records[i]), sizeof(record));
    
    memcpy(&(leaf->records[insertion_point]), new_record, sizeof(record));
    leaf->num_of_keys++;
    pwrite(fd, leaf, sizeof(page), leaf_offset);
    free(leaf);
    // new_record free????
}

void insert_into_leaf_after_splitting(page * leaf, off_t leaf_offset, record * new_record) {
    page * new_leaf,;
    record * temp_records;
    int64_t new_key;
    int insertion_index, split, i, j;
    off_t new_leaf_offset;

    new_leaf = (page*)calloc(1, sizeof(page));
    new_leaf_offset = new_page();
    temp_records = (record*)calloc(leaf_order, sizeof(record));

    insertion_index = 0;
    while(insertion_index < leaf_order - 1 && leaf->records[insertion_index].key < new_record->key) {
        insertion_index++;
    }

    for (i = 0, j = 0; i < leaf->num_of_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_records[j] = leaf->records[i];
    }

    memcpy(&(temp_records[i]), new_record, sizeof(record));

    leaf->num_of_keys = 0;
    new_leaf->num_of_keys = 0;

    split = cut(leaf_order - 1);

    for (i = 0; i < split; i++) {
        leaf->records[i] = temp_records[i];
        leaf->num_of_keys++;
    }

    for (i = split, j = 0; i < leaf_order; i++, j++) {
        new_leaf->records[j] = temp_records[i];
        new_leaf->num_of_keys++;
    }

    free(temp_records);

    new_leaf->next_offset = leaf->next_offset;
    new_leaf->parent_page_offset = leaf->parent_page_offset;
    new_leaf->is_leaf = 1;

    leaf->next_offset = new_leaf_offset;
    
    new_key = new_leaf->records[0].key;

    insert_into_parent(leaf, leaf_offset, new_key, new_leaf, new_leaf_offset);

    // disk write page and free memory will be conducted in other functions

    // pwrite(fd, new_leaf, sizeof(page), new_page_offset);
    // pwrite(fd, leaf, sizeof(page), leaf_page_offset);

    // free(leaf);
    // free(new_leaf);
}

/* Inserts a new key and page number to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
void insert_into_node(page * parent, off_t parent_offset,
        int left_index, int64_t key, page * right, off_t right_offset) {
    int i;

    for (i = parent->num_of_keys; i > left_index; i--) {
        memcpy(&(parent->b_f[i]), &(parent->b_f[i - 1]), sizeof(I_R));
    }
    // case: left page is in leftmost
    if (left_index == internal_order - 1) {
        parent->b_f[0].key = key;
        parent->b_f[0].p_offset = right_offset;
    }
    // general case
    else {
        parent->b_f[left_index + 1].key = key;
        parent->b_f[left_index + 1].p_offset = right_offset;
    }
    parent->num_of_keys++;

    right->parent_page_offset = parent_offset;
    pwrite(fd, right, sizeof(page), right_offset);
    free(parent);

    pwrite(fd, parent, sizeof(page), parent_offset);
    free(right);
}

/* Inserts a new key and page number to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
void insert_into_node_after_splitting(page * old_node, off_t old_node_offset, int left_index,
        int64_t key, page * right, off_t right_offset) {
    
    int i, j, split;
    int64_t k_prime;
    page * new_node, * child;
    I_R * temp_b_fs;
    off_t new_node_offset, temp_leftmost;


    /* First create a temporary array of branch_factor
     * to hold everything in order, including
     * the new key and page number, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * branch_factors to the old node and
     * the other half to the new.
     */

    temp_b_fs = (I_R *)calloc(internal_order, sizeof(I_R));
    temp_leftmost = old_node->next_offset;

    // case: left_index is leftmost, set j = 1 to push all b_fs to right
    j = (left_index == internal_order - 1) ? 1 : 0;

    for (i = 0; i < old_node->num_keys; i++, j++) {
        if (j == left_index + 1) j++;
        temp_b_fs[j] = old_node->b_f[i];
    }

    temp_b_fs[left_index + 1].key = key;
    temp_b_fs[left_index + 1].p_offset = right_offset;

    /* Create the new node and copy
     * half the branch_factors to the
     * old and half to the new.
     */  
    split = cut(leaf_order);
    new_node_offset = new_page();
    new_node = load_page(new_node_offset);
    old_node->num_of_keys = 0;
    old_node->next_offset = temp_leftmost;
    for (i = 0; i < split - 1; i++) {
        old_node->b_f[i] = temp_b_fs[i];
        old_node->num_keys++;
    }
    k_prime = temp_b_fs[split - 1].key;
    new_node->next_offset = temp_b_fs[split - 1].p_offset;
    for (++i, j = 0; i < leaf_order; i++, j++) {
        new_node->b_f[j] = temp_b_fs[i];
        new_node->num_keys++;
    }

    // 여기서부터 다시
    
}   

void insert_into_parent(page * left, off_t left_offset, int64_t key, page * right, off_t right_offset) {
    int left_index;
    off_t parent_offset;
    page * parent;

    parent_offset = left->parent_page_offset;
    /* Case: new root. */

    if (parent_offset == 0) {
        insert_into_new_root(left, left_offset, key, right, right_offset);
        return;
    }

    parent = load_page(parent_offset);
    
    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    /* Find the parent's branch_factor index to the left 
     * node.
     */

    left_index = get_left_index(parent_offset, left_offset);

    /* Simple case: the new key fits into the node. 
     */


    // 여기부터 다시ㅣㅣㅣㅣㅣㅣㅣ
    if (parent->num_of_keys < internal_order - 1) {
        insert_into_node(parent, parent_offset, left_index, key, right, right_offset);
        pwrite(fd, left, sizeof(page), left_offset);
        free(left);
        return;
    }

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */

    insert_into_node_after_splitting(parent, left_index, key, right);

}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
void insert_into_new_root(page * left, off_t left_offset, int64_t key, page * right, off_t right_offset) {
    off_t root_offset = new_page();
    page * root = load_page(root_offset);

    // save change in header page
    hp->rpo = root_offset;
    pwrite(fd, hp, sizeof(H_P), 0);

    root->parent_page_offset = 0;
    root->is_leaf = 0;
    root->num_of_keys = 1;
    root->next_offset = left_offset;
    root->b_f[0].key = key;
    root->b_f[0].p_offset = right_offset;

    free(rt);
    rt = root;
    pwrite(fd, rt, sizeof(page), root_offset);

    left->parent_page_offset = root_offset;
    pwrite(fd, left, sizeof(page), left_offset);
    free(left);

    right->parent_page_offset = root_offset;
    pwrite(fd, right, sizeof(page), right_offset);
    free(right);
}

// Deletion Utility Functions


char * db_find(int64_t key) {
    int i = 0;
    page * c = find_leaf(key);
    if (c == NULL) return NULL;
    for (i = 0; i < c->num_of_keys; i++)
        if (c->records[i].key == key) break;
    if (i == c->num_of_keys)
        return NULL;
    else 
        return &(c->records[i].value[0]);
    
    // leaf page free 필요??
}

int db_insert(int64_t key, char * value) {


}


int db_delete(int64_t key) {

}//fin








