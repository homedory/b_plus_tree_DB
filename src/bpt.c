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
    off_t next_offset;
    int i = 0;

    // Empty Tree, return header page offset
    if (hp->num_of_pages == 1) {
        return 0;
    }
    page * c = (page*)calloc(1, sizeof(page)); 
    memcpy(c, rt, sizeof(page)); // start from root page
    
    next_offset = hp->rpo;
    while (!c->is_leaf) {
        i = 0;
        while (i < c->num_of_keys) {
            if (key >= c->b_f[i].key) i++;
            else break;
        }
        next_offset = c->b_f[i].p_offset;
        free(c);
        c = load_page(next_offset);
    }

    free(c);
    return next_offset;
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
void insert_into_leaf(page * leaf, off_t leaf_offset, record new_record) {
    int i , insertion_point;

    insertion_point = 0;
    while(insertion_point < leaf->num_of_keys && 
            leaf->records[insertion_point].key < new_record.key)
        insertion_point++;
    
    for (i = leaf->num_of_keys; i > insertion_point; i--) 
        leaf->records[i] = leaf->records[i - 1];
    
    leaf->records[insertion_point] = new_record;
    leaf->num_of_keys++;
    // pwrite(fd, leaf, sizeof(page), leaf_offset);
    // free(leaf);
}

/* Inserts a new record
 * into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
void insert_into_leaf_after_splitting(page * leaf, off_t leaf_offset, record new_record) {
    page * new_leaf;
    record * temp_records;
    int64_t new_key;
    int insertion_index, split, i, j;
    off_t new_leaf_offset;

    new_leaf = (page*)calloc(1, sizeof(page));
    temp_records = (record*)calloc(leaf_order, sizeof(record));

    new_leaf_offset = new_page();

    insertion_index = 0;
    while(insertion_index < leaf_order - 1 && leaf->records[insertion_index].key < new_record.key)
        insertion_index++;

    for (i = 0, j = 0; i < leaf->num_of_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_records[j] = leaf->records[i];
    }

    temp_records[insertion_index] = new_record;

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
    pwrite(fd, new_leaf, sizeof(page), new_leaf_offset);
    free(new_leaf);
}

/* Inserts a new record
 * into a leaf so as to exceed
 * the tree's order, causing key rotation.
 */
void insert_into_leaf_using_key_rotation(page * leaf, off_t leaf_offset, record new_record) {
    page * next_leaf;
    off_t next_leaf_offset;
    record * temp_records;
    record rightmost_record;
    int64_t new_key;
    int insertion_index, i, j;
    
    temp_records = (record*)calloc(leaf_order, sizeof(record));

    insertion_index = 0;
    while(insertion_index < leaf_order - 1 && leaf->records[insertion_index].key < new_record.key)
        insertion_index++;

    for (i = 0, j = 0; i < leaf->num_of_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_records[j] = leaf->records[i];
    }
    temp_records[insertion_index] = new_record;

    // rearrange records in leaf node
    for (i = 0; i < leaf->num_of_keys; i++) {
        leaf->records[i] = temp_records[i];
    }
    rightmost_record = temp_records[leaf_order - 1];
    free(temp_records);

    next_leaf_offset = leaf->next_offset;
    next_leaf = load_page(next_leaf_offset);

    new_key = rightmost_record.key;
    update_key_for_key_rotation(next_leaf, next_leaf_offset, new_key);

    // insert rightmost record into next leaf node
    insert_into_leaf(next_leaf, next_leaf_offset, rightmost_record);
    pwrite(fd, next_leaf, sizeof(page), next_leaf_offset);
    free(next_leaf);
}

/* Check if there is a room
 * in a leaf node
 * for key rotation insertion
 */
bool has_room_for_record(off_t leaf_offset) {
    page * leaf;
    bool result;

    if (leaf_offset == 0)
        return false;

    leaf = load_page(leaf_offset);
    result = (leaf->num_of_keys < leaf_order - 1);
    free(leaf);
    return result;
}

/* Update a new key 
 * in key rotation insertion
 */
void update_key_for_key_rotation(page * leaf, off_t leaf_offset, int64_t key) {
    page * upper;
    off_t upper_offset, lower_offset;
    int i;

    upper_offset = leaf->parent_page_offset;
    upper = load_page(upper_offset);
    lower_offset = leaf_offset;
    
    // loop until the upper becomes non-leftmost node
    while (upper->next_offset == lower_offset) {
        lower_offset = upper_offset;
        upper_offset = upper->parent_page_offset;
        free(upper);
        upper = load_page(upper_offset);
    }

    // find index of branching factor in upper node
    for (i = 0; i < upper->num_of_keys; i++) 
        if (upper->b_f[i].p_offset == lower_offset) break;
    
    // change key
    upper->b_f[i].key = key;
    pwrite(fd, upper, sizeof(page), upper_offset);
    free(upper);
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
    // pwrite(fd, right, sizeof(page), right_offset);
    // free(parent);

    // pwrite(fd, parent, sizeof(page), parent_offset);
    // free(right);
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
    off_t new_node_offset, child_offset, temp_leftmost;

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

    free(temp_b_fs);

    for (i = 0; i < new_node->num_of_keys; i++) {
        child_offset = new_node->b_f[i].p_offset;
        child = load_page(child_offset);
        child->parent_page_offset = new_node_offset;
        pwrite(fd, child, sizeof(page), child_offset);
        free(child);
    }
    
    // pwrite(fd, right, sizeof(page), right_offset);
    // free(right);
    insert_into_parent(old_node, old_node_offset, k_prime, new_node, new_node_offset);

    pwrite(fd, new_node, sizeof(page), new_node_offset);
    free(new_node);
}  
 
/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
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

    if (parent->num_of_keys < internal_order - 1) {
        insert_into_node(parent, parent_offset, left_index, key, right, right_offset);
        // pwrite(fd, left, sizeof(page), left_offset);
        // free(left);
    }

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */
    
    else {
        // pwrite(fd, left, sizeof(page), left_offset);
        // free(left);
        insert_into_node_after_splitting(parent, parent_offset, left_index, key, right, right_offset);
    }
    pwrite(fd, parent, sizeof(page), parent_offset);
    free(parent);
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
    // pwrite(fd, left, sizeof(page), left_offset);
    // free(left);

    right->parent_page_offset = root_offset;
    // pwrite(fd, right, sizeof(page), right_offset);
    // free(right);
}

// Deletion Utility Functions


/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the right if one exists.  If not (the node
 * is the rightmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index( page * node, off_t node_offset ) {
    page * parent;
    off_t parent_offset;
    int i;

    parent_offset = node->parent_page_offset;
    parent = load_page(parent_offset);

    /* Return the index of the key to the right
     * of the pointer in the parent pointing
     * to node.  
     * If node is the rightmost child, this means
     * return -1.
     */

    for (i = 0; i < parent->num_of_keys; i++)
        if (parent->b_f[i].p_offset == node_offset)
            break;

    // case: node is leftmost node, return value 0
    if (node->next_offset == node_offset)
        i = -1;
    // case: node is rightmost node, return value -1
    else if (i == parent->num_of_keys)
        i = -2;
    
    free(parent);
    return i + 1;
}

void remove_entry_from_node(page * node, off_t node_offset, int64_t key) {

    int i, num_entries;

    if (!node->is_leaf && ) {

    }

    if (node->is_leaf) {
        // Remove the record and shift other records accordingly.
        i = 0;
        while (node->records[i].key != key) 
            i++;
        for (++i; i < node->num_of_keys; i++)
            node->records[i - 1] = node->records[i];
        
        // One key fewer.
        node->num_of_keys--;
    }
    else {

    }

    // Remove the entry and shift other entries accordingly.
    i = 0;
    while (node->keys[i] != key)
        i++;
    for (++i; i < n->num_keys; i++)
        n->keys[i - 1] = n->keys[i];

    // Remove the pointer and shift other pointers accordingly.
    // First determine number of pointers.
    num_entries = n->is_leaf ? n->num_keys : n->num_keys + 1;
    i = 0;
    while (n->pointers[i] != pointer)
        i++;
    for (++i; i < num_entries; i++)
        n->pointers[i - 1] = n->pointers[i];


    // One key fewer.
    n->num_keys--;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    if (n->is_leaf)
        for (i = n->num_keys; i < order - 1; i++)
            n->pointers[i] = NULL;
    else
        for (i = n->num_keys + 1; i < order; i++)
            n->pointers[i] = NULL;

    return n;
}




/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
void delete_entry(page * node, off_t node_offset, int64_t key, void * pointer ) {

    int min_keys;
    page * parent, neighbor;
    off_t parent_offset, neighbor_offset;
    int neighbor_index;
    int k_prime_index; 
    int64_t k_prime;
    int capacity;

    // Remove key and pointer from node.

    n = remove_entry_from_node(n, key, (node *)pointer);

    /* Case:  deletion from the root. 
     */

    if (n == root) 
        return adjust_root(root);


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (n->num_keys >= min_keys)
        return root;

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */

    parent_offset = node->parent_page_offset;
    parent = load_page(parent_offset);

    // 중요 !!!!!!! 고려해서 수정했음
    // get right sibling node index in parent, if -1 it means there is no right sibling node
    neighbor_index = get_neighbor_index(node, node_offset);
    k_prime_index = neighbor_index == -1 ? parent->num_of_keys - 1 : neighbor_index;
    k_prime = parent->b_f[k_prime_index].key;

    neighbor_offset = neighbor_index == -1 ? parent->b_f[parent->num_of_keys - 1].p_offset : 
        parent->b_f[neighbor_index].p_offset; //여기서 그럼 neighbor index가 leftmost 이면 에러날듯 -> offset구하는 함수 만들어줘야될듯

    capacity = n->is_leaf ? order : order - 1;


    free(parent);
    /* Coalescence. */

    if (neighbor->num_keys + n->num_keys < capacity)
        return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

    /* Redistribution. */

    else
        return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}


char * db_find(int64_t key) {
    int i;
    page * leaf;
    off_t leaf_offset;
    char * value;
    
    leaf_offset = find_leaf(key);

    // Empty Tree
    if (leaf_offset == 0) 
        return NULL;

    leaf = load_page(leaf_offset);

    if (leaf == NULL) return NULL;
    for (i = 0; i < leaf->num_of_keys; i++)
        if (leaf->records[i].key == key) break;
    if (i == leaf->num_of_keys) {
        free(leaf);
        return NULL;
    }

    value = calloc(120, sizeof(char));
    memcpy(value, leaf->records[i].value, 120 * sizeof(char));
    free(leaf);
    return value;
}

int db_insert(int64_t key, char * value) {
    record new_rec;
    page * leaf;
    off_t leaf_offset;

    /* The current implementation ignores
     * duplicates.
     */
    if (db_find(key) != NULL)
        return -1;

    new_rec.key = key;
    memcpy(new_rec.value, value, 120 * sizeof(char));

    /* Case: the tree does not exist yet.
     * Start a new tree.
     */

    if (hp->num_of_pages == 1)  {
        start_new_file(new_rec);
        return 0;
    }

    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    leaf_offset = find_leaf(key);
    leaf = load_page(leaf_offset);

    /* Case: leaf has room for record.
     */

    if (leaf->num_keys < leaf_order - 1) {
        insert_into_leaf(leaf, leaf_offset, new_rec);
    }

    /* Case: right sibling leaf has room for record.
     */
    else if (has_room_for_record(leaf->next_offset)) {
        insert_into_leaf_using_key_rotation(leaf, leaf_offset, new_rec);
    }

    /* Case:  leaf must be split.
     */
    else {
        insert_into_leaf_after_splitting(leaf, leaf_offset, new_rec);
    }

    free(leaf);
    return 0;
}


int db_delete(int64_t key) {

}//fin








