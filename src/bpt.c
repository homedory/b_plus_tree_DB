#include "bpt.h"

H_P * hp;

page * rt = NULL; //root is declared as global

int fd = -1; //fd is declared as global

const int leaf_order = 32;
const int internal_order = 249;

const int record_value_size = 120;


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

/* fucnction to reset a page when making a new free page */
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

/* function to free page before use */
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

/* function to free page and add it to header page */
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


/* function to get a free page */
off_t new_page() {
    off_t newp;
    page * np;
    off_t prev;
    if (hp->fpo != 0) {
        newp = hp->fpo;
        np = load_page(newp);
        hp->fpo = np->parent_page_offset;
        pwrite(fd, hp, sizeof(H_P), 0);
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


/* First insertion:
 * start a new tree.
 */
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


void print_bpt() {
    if (hp->rpo == 0) {
        printf("Tree is empty.\n");
        return;
    }
    printf("B+ Tree keys:\n");
    printf("hp->rpo = %ld\n", hp->rpo);
    print_keys(hp->rpo, 0);
}
// B+ Tree의 키를 출력하는 함수
void print_keys(off_t page_offset, int depth) {
    // 노드를 로드
    page *current_page = load_page(page_offset);
    if(current_page==NULL){
        printf("load page fail\n");
        return;
    }
    // 들여쓰기(Depth) 출력
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    // 현재 노드의 키 출력
    printf("[");
    if(!current_page->is_leaf) printf("offt: %ld ",current_page->next_offset);
    for (int i = 0; i < current_page->num_of_keys; i++) {
        if (current_page->is_leaf) {
            printf("%" PRId64, current_page->records[i].key); // 리프 노드
        } else {
            printf("%" PRId64, current_page->b_f[i].key); // 내부 노드
            printf(" offt: %ld ",current_page->b_f[i].p_offset);
        }
        if (i < current_page->num_of_keys - 1) {
            printf(", ");
        }
    }
    printf("]");
    printf("  next offset: %ld, numofkeys: %d\n", current_page->next_offset, current_page->num_of_keys);
    // 리프 노드인 경우 탐색 종료
    if (current_page->is_leaf) {
        free(current_page);
        return;
    }
    off_t child_offset;
     // 내부 노드 탐색: 첫 번째 자식부터 출력
    // 1. 첫 번째 자식 (next_offset) 출력
    if (current_page->next_offset != 0) {
        print_keys(current_page->next_offset, depth + 1);
    }
    // 2. 나머지 자식 (b_f 배열에 저장된 포인터들) 출력
    for (int i = 0; i < current_page->num_of_keys; i++) {
        print_keys(current_page->b_f[i].p_offset, depth + 1);
    }
    free(current_page);
}

// Find Utitlity Function


/* Traces the path from the root to a leaf, searching by key. 
 * Returns the leaf_offset containing the given key.
 */
off_t find_leaf(int64_t key) {
    off_t next_offset;
    int i = 0;

    // tree does not exist yet, return header page offset
    if (hp->num_of_pages == 1) {
        return 0;
    }
    page * c = load_page(hp->rpo); // start from root page
    
    next_offset = hp->rpo;
    while (!c->is_leaf) {
        i = 0;
        while (i < c->num_of_keys && key >= c->b_f[i].key) 
            i++;
        
        // If i == 0, next_offset is the leftmost in the node.
        if (i == 0)
            next_offset = c->next_offset;
        else
            next_offset = c->b_f[i - 1].p_offset;
        free(c);
        c = load_page(next_offset);
    }

    free(c);
    return next_offset;
}


// Insertion Utility Functions


/* Helper function used in insert_into_parent
 * to find the index in the parent's branch_factors array
 * corresponding to the node immediately to the left of the key to be inserted.
*/
int get_child_index_in_parent(off_t parent_offset, off_t child_offset) {
    int child_index = 0;
    page * parent = load_page(parent_offset);    
    while (child_index < parent->num_of_keys &&
            parent->b_f[child_index].p_offset != child_offset)
        child_index++;
    
    // case: left_offset is in leftmost
    if (parent->next_offset == child_offset) {
        free(parent);
        return internal_order - 1;
    }
    
    free(parent);
    return child_index;
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
    
    pwrite(fd, leaf, sizeof(page), leaf_offset);
    free(leaf);
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

    // Find the position to insert the new record.
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

    // Split the records between the original and new leaves.
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
    
    // pwrite(fd, new_leaf, sizeof(page), new_leaf_offset);
    // free(new_leaf);
    // The caller function, db_insert, will handle the disk write for the leaf node
}

/* Inserts a new record
 * into a leaf so as to exceed
 * the tree's order, causing key rotation.
 */
void insert_into_leaf_using_key_rotation(page * leaf, off_t leaf_offset, record new_record) {
    // for_test
    // printf("key rotation executed!!!!!!!!!!!\n");

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
    // pwrite(fd, next_leaf, sizeof(page), next_leaf_offset);
    // free(next_leaf);
    pwrite(fd, leaf, sizeof(page), leaf_offset);
    free(leaf);
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

    // for_test
    if (leaf->parent_page_offset == 0) {
        printf("Failed Leaf parent page offset 0\n");
    }
    else if (leaf->parent_page_offset == leaf_offset) {
        printf("Failed Leaf parent page offset is same with leaf_offset\n");
    }

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
    
    // if (upper->next_offset == lower_offset) {
    //     // change key
    //     upper->b_f[0].key = key;
    //     pwrite(fd, upper, sizeof(page), upper_offset);
    //     free(upper);        
    // }
    if (i == upper->num_of_keys) {
        // for_test
        printf("Failed to find matching branching factor in parent\n");
        // for_test
        printf("------------------------upper status------------------------\n");
        printf("num of keys %d\n", upper->num_of_keys);
        printf("%ld ", upper->next_offset);
        for (int x = 0; x < upper->num_of_keys; x++) {
            printf("||- %ld -|| %ld ", upper->b_f[x].key, upper->b_f[x].p_offset);
        }
        printf("\n--------------------------------------------------------------\n");
    
        perror("Failed to find matching branching factor in parent");
        free(upper);
        return;
    }

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

    int left_bound = left_index == internal_order - 1 ? 0 : left_index;

    for (i = parent->num_of_keys; i > left_bound; i--) {
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

    for (i = 0; i < old_node->num_of_keys; i++, j++) {
        if (j == left_index + 1) j++;
        temp_b_fs[j] = old_node->b_f[i];
    }

    // left node is leftmost child
    if (left_index == internal_order - 1) {
        temp_b_fs[0].key = key;
        temp_b_fs[0].p_offset = right_offset;
    }
    else {
        temp_b_fs[left_index + 1].key = key;
        temp_b_fs[left_index + 1].p_offset = right_offset;
    }
    /* Create the new node and copy
     * half the branch_factors to the
     * old and half to the new.
     */  
    split = cut(internal_order);
    new_node_offset = new_page();
    new_node = load_page(new_node_offset);
    old_node->num_of_keys = 0;
    old_node->next_offset = temp_leftmost;
    for (i = 0; i < split - 1; i++) {
        old_node->b_f[i] = temp_b_fs[i];
        old_node->num_of_keys++;
    }
    k_prime = temp_b_fs[split - 1].key;
    new_node->next_offset = temp_b_fs[split - 1].p_offset;
    for (++i, j = 0; i < internal_order; i++, j++) {
        new_node->b_f[j] = temp_b_fs[i];
        new_node->num_of_keys++;
    }

    free(temp_b_fs);

    child_offset = new_node->next_offset;
    child = load_page(child_offset);
    child->parent_page_offset = new_node_offset;
    pwrite(fd, child, sizeof(page), child_offset);
    free(child);
    for (i = 0; i < new_node->num_of_keys; i++) {
        child_offset = new_node->b_f[i].p_offset;
        child = load_page(child_offset);
        child->parent_page_offset = new_node_offset;
        pwrite(fd, child, sizeof(page), child_offset);
        free(child);
    }
    
    // right node is a child of old_node
    if (left_index + 1 < split - 1 || left_index == internal_order - 1)
        right->parent_page_offset = old_node_offset;
    // right node is a child of new_node
    else
        right->parent_page_offset = new_node_offset;

    pwrite(fd, right, sizeof(page), right_offset);
    free(right);
    
    insert_into_parent(old_node, old_node_offset, k_prime, new_node, new_node_offset);

    // pwrite(fd, new_node, sizeof(page), new_node_offset);
    // free(new_node);
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
        pwrite(fd, left, sizeof(page), left_offset);
        free(left);
        pwrite(fd, right, sizeof(page), right_offset);
        free(right);
        return;
    }

    parent = load_page(parent_offset);
    
    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    /* Find the parent's branch_factor index to the left 
     * node.
     */

    left_index = get_child_index_in_parent(parent_offset, left_offset);

    /* Simple case: the new key fits into the node. 
     */

    left->parent_page_offset = parent_offset;

    if (parent->num_of_keys < internal_order - 1) {   
        pwrite(fd, left, sizeof(page), left_offset);
        free(left); 
        insert_into_node(parent, parent_offset, left_index, key, right, right_offset);
        pwrite(fd, parent, sizeof(page), parent_offset);
        free(parent);
    }

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */
    
    else {
        pwrite(fd, left, sizeof(page), left_offset);
        free(left);
        insert_into_node_after_splitting(parent, parent_offset, left_index, key, right, right_offset);
        return;
        // parent node will be written on disk when it stops 
    }
    
    pwrite(fd, right, sizeof(page), right_offset);
    free(right);
}


/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
void insert_into_new_root(page * left, off_t left_offset, int64_t key, page * right, off_t right_offset) {
    off_t root_offset = new_page();
    page * root = load_page(root_offset);

    // for_test
    // printf("Insert_into_new_root starts, new_root %ld---------------------------------------\n", root_offset);
    // save change in header page
    hp->rpo = root_offset;
    pwrite(fd, hp, sizeof(H_P), 0);

    root->parent_page_offset = 0;
    root->is_leaf = 0;
    root->num_of_keys = 1;
    root->next_offset = left_offset;
    root->b_f[0].key = key;
    root->b_f[0].p_offset = right_offset;

    // Free the old root page if it exists.
    if (rt) {
        free(rt);
    }
    rt = root;  
    pwrite(fd, root, sizeof(page), root_offset);

    left->parent_page_offset = root_offset;

    right->parent_page_offset = root_offset;

    // for_test
    // root = load_page(root_offset);
    // printf("%ld ", root->next_offset);
    // for (int x = 0; x < root->num_of_keys; x++) {
    //     printf("||- %ld -|| %ld ", root->b_f[x].key, root->b_f[x].p_offset);
    // }
    // printf("\n");
    // free(root);
}

// Deletion Utility Functions


/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index(page * node, off_t node_offset) {
    page * parent;
    off_t parent_offset;
    int i;

    parent_offset = node->parent_page_offset;
    parent = load_page(parent_offset);

    /* Return the index of the key to the left
     * of the p_offset in the parent pointing
     * to node.  
     * If node is the lefttmost child, this means
     * return -1.
     */

    for (i = 0; i < parent->num_of_keys; i++)
        if (parent->b_f[i].p_offset == node_offset)
            break;

    // case: neighbor node (left sibling) is leftmost node
    if (i == 0)
        i = internal_order;
    // case: node is leftmost node which means there is no left sibling, return value is -1
    else if (node_offset == parent->next_offset) {
        // for_test
        // printf("------------------------------neightbor index -1---------------------------\n");
        i = 0;
    }
    
    free(parent);
    return i - 1;
}

// Assumes that the entry is to the right of the key in the node.
void remove_entry_from_node(page * node, int64_t key) {

    int i, num_entries;

    // The key and entry share the same index because the entry is immediately to the right of the key.

    if (node->is_leaf) {
        i = 0;
        while (i < node->num_of_keys && node->records[i].key != key)
            i++;
        
        if (i == node->num_of_keys) {
            perror("remove_entry_from_node: Key not found in leaf node.\n");
            return;
        }

        for (++i; i < node->num_of_keys; i++) 
            node->records[i - 1] = node->records[i];
    }
    else {
        i = 0;
        while (i < node->num_of_keys && node->b_f[i].key != key)
            i++;
        
        if (i == node->num_of_keys) {
            perror("remove_entry_from_node: Key not found in internal node.\n");
            return;
        }

        for (++i; i < node->num_of_keys; i++) 
            node->b_f[i - 1] = node->b_f[i];
    }

    node->num_of_keys--;
}

void adjust_root(page * root, off_t root_offset) {

    page * new_root;
    off_t new_root_offset;

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root->num_of_keys > 0) {
        pwrite(fd, root, sizeof(page), root_offset);
        free(root);
        return;
    }

    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.

    if (!root->is_leaf) {
        new_root_offset = root->next_offset;
        new_root = load_page(new_root_offset);
        new_root->parent_page_offset = 0;
        pwrite(fd, new_root, sizeof(page), new_root_offset);
        free(new_root);
        free(root);
        usetofree(root_offset);
        hp->rpo = new_root_offset;
        pwrite(fd, hp, sizeof(H_P), 0);
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.
    // remove root page
    
    else {
        free(root);
        usetofree(root_offset);
    }
}

/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
void coalesce_nodes(page * node, off_t node_offset, page * neighbor, off_t neighbor_offset,
        int neighbor_index, int64_t k_prime) {

    int i, j, neighbor_insertion_index, node_end;
    page * temp_node, * parent, * child;
    off_t temp_offset, parent_offset, child_offset;

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -1) {
        temp_node = node;
        temp_offset = node_offset;
        node = neighbor;
        node_offset = neighbor_offset;
        neighbor = temp_node;
        neighbor_offset = temp_offset;
    }

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor->num_of_keys;
    node_end = node->num_of_keys;

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    if (!node->is_leaf) {

        /* Append k_prime.
         */

        neighbor->b_f[neighbor_insertion_index].key = k_prime;
        neighbor->b_f[neighbor_insertion_index].p_offset = node->next_offset;
        neighbor->num_of_keys++;

        // update child's parent
        child_offset = node->next_offset;
        child = load_page(child_offset);
        child->parent_page_offset = neighbor_offset;
        pwrite(fd, child, sizeof(page), child_offset);
        free(child);


        for (i = neighbor_insertion_index + 1, j = 0; j < node_end; i++, j++) {
            neighbor->b_f[i] = node->b_f[j];
            neighbor->num_of_keys++;
            node->num_of_keys--;

            child_offset = node->b_f[j].p_offset;
            child = load_page(child_offset);
            child->parent_page_offset = neighbor_offset;
            pwrite(fd, child, sizeof(page), child_offset);
            free(child);
        }
    }

    /* In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been node's right neighbor.
     */

    else {
        for (i = neighbor_insertion_index, j = 0; j < node_end; i++, j++) {
            neighbor->records[i] = node->records[j];
            neighbor->num_of_keys++;
            node->num_of_keys--;
        }
        neighbor->next_offset = node->next_offset;
    }

    parent_offset = node->parent_page_offset;
    parent = load_page(parent_offset);

    // node should be deleted beacuse it was coalesced
    free(node);
    usetofree(node_offset);

    pwrite(fd, neighbor, sizeof(page), neighbor_offset);
    free(neighbor);

    // delete k_prime and deleted node's entry from parent
    delete_entry(parent, parent_offset, k_prime);

    // The callee function, delete_entry, will handle the disk write for the parent node
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
void redistribute_nodes(page * node, off_t node_offset, page * neighbor, off_t neighbor_offset,
        int neighbor_index, int k_prime_index, int k_prime) {  

    int i;
    page * child, * parent, * temp_node;
    off_t child_offset, parent_offset;

    parent_offset = node->parent_page_offset;
    parent = load_page(parent_offset);   

    /* Case: node has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to node's left end.
     */

    if (neighbor_index != -1) {
        if (!node->is_leaf) {
            for (i = node->num_of_keys; i > 0; i--) {
                node->b_f[i] = node->b_f[i - 1];
            }
            node->b_f[0].p_offset = node->next_offset;
            node->b_f[0].key = k_prime;
            node->next_offset = neighbor->b_f[neighbor->num_of_keys - 1].p_offset;
            
            child_offset = neighbor->b_f[neighbor->num_of_keys - 1].p_offset;
            child = load_page(child_offset);
            child->parent_page_offset = node_offset;
            pwrite(fd, child, sizeof(page), child_offset);
            free(child);

            parent->b_f[k_prime_index].key = neighbor->b_f[neighbor->num_of_keys - 1].key;
        }
        else {
            for (i = node->num_of_keys; i > 0; i--) {
                node->records[i] = node->records[i - 1];
            }
            node->records[0] = neighbor->records[neighbor->num_of_keys - 1];

            parent->b_f[k_prime_index].key = node->records[0].key;
        }
    }

    /* Case: node is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to node's rightmost position.
     */

    else {
        if (!node->is_leaf) {
            node->b_f[node->num_of_keys].key = k_prime;
            node->b_f[node->num_of_keys].p_offset = neighbor->next_offset;
            
            child_offset = neighbor->next_offset;
            child = load_page(child_offset);
            child->parent_page_offset = node_offset;
            pwrite(fd, child, sizeof(page), child_offset);
            free(child);
            
            parent->b_f[k_prime_index].key = neighbor->b_f[0].key;

            neighbor->next_offset = neighbor->b_f[0].p_offset;
            for (i = 0; i < neighbor->num_of_keys - 1; i++) {
                neighbor->b_f[i] = neighbor->b_f[i + 1];
            }
        }
        else {
            node->records[node->num_of_keys] = neighbor->records[0];
            for (i = 0; i < neighbor->num_of_keys - 1; i++) {
                neighbor->records[i] = neighbor->records[i + 1];
            }

            parent->b_f[k_prime_index].key = neighbor->records[0].key;  
            
            // for_test
            // printf("restribute nodes in leaf with neighbor index -1\n k_prime_index %d new key: %ld\n",
            // k_prime_index, neighbor->records[0].key);
        }
    }

    pwrite(fd, parent, sizeof(page), parent_offset);
    free(parent);

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    node->num_of_keys++;
    neighbor->num_of_keys--;

    pwrite(fd, node, sizeof(page), node_offset);
    free(node);

    pwrite(fd, neighbor, sizeof(page), neighbor_offset);
    free(neighbor);
}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
void delete_entry(page * node, off_t node_offset, int64_t key) {

    int min_keys;
    page * parent, * neighbor;
    off_t parent_offset, neighbor_offset;
    int64_t k_prime;
    int neighbor_index, k_prime_index; 
    int capacity;

    // Remove key and pointer from node.

    remove_entry_from_node(node, key);

    /* Case:  deletion from the root. 
     */

    if (node_offset == hp->rpo) {
        adjust_root(node, node_offset);
        return;
    }


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = node->is_leaf ? cut(leaf_order - 1) : cut(internal_order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (node->num_of_keys >= min_keys) {
        pwrite(fd, node, sizeof(page), node_offset);
        free(node);
        return;
    }

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

    // get left sibling node index in parent, if -1 it means there is no left sibling node
    neighbor_index = get_neighbor_index(node, node_offset);

    if (neighbor_index == -1) {
        k_prime_index = 0;
        neighbor_offset = parent->b_f[0].p_offset;
    }
    else if (neighbor_index == internal_order - 1) {
        k_prime_index = 0;
        neighbor_offset = parent->next_offset;
    }
    else {
        k_prime_index = neighbor_index + 1;
        neighbor_offset = parent->b_f[neighbor_index].p_offset;
    }
    k_prime = parent->b_f[k_prime_index].key;
    neighbor = load_page(neighbor_offset);

    capacity = node->is_leaf ? leaf_order : internal_order - 1;


    free(parent);
    /* Coalescence. */

    if (neighbor->num_of_keys + node->num_of_keys < capacity) 
        coalesce_nodes(node, node_offset, neighbor, neighbor_offset, neighbor_index, k_prime);

    /* Redistribution. */

    else 
        redistribute_nodes(node, node_offset, neighbor, neighbor_offset, neighbor_index, k_prime_index, k_prime);
    
    /* The callee function (delete_entry, coalesce_nodes, redistrubute_nodes) 
     * handled the disk write or free page for the node
     */
}


/* Helper function to check if a value already exists for the given key.
 * Returns the leaf offset if the key exists; otherwise, returns 0, 
 * which corresponds to the header page offset.
 */
off_t find_leaf_if_value_exists(int64_t key) {
    int i;
    page * leaf;
    off_t leaf_offset;
    
    leaf_offset = find_leaf(key);

    /* Case: the tree does not exist yet.
     * Start a new tree.
     */
    if (leaf_offset == 0) 
        return 0;

    leaf = load_page(leaf_offset);

    if (leaf == NULL) return 0;
    for (i = 0; i < leaf->num_of_keys; i++)
        if (leaf->records[i].key == key) break;
    if (i == leaf->num_of_keys) {
        free(leaf);
        return 0;
    }
    free(leaf);
    return leaf_offset;
}


// Mater functions

char * db_find(int64_t key) {
    int i;
    page * leaf;
    off_t leaf_offset;
    char * value;
    
    leaf_offset = find_leaf(key);

    /* Case: the tree does not exist yet.
     * Start a new tree.
     */
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

    value = calloc(record_value_size, sizeof(char));
    memcpy(value, leaf->records[i].value, record_value_size * sizeof(char));
    free(leaf);
    return value;
}

int db_insert(int64_t key, char * value) {
    record new_rec;
    page * leaf;
    off_t leaf_offset;
    char * record_value;

    /* The current implementation ignores
     * duplicates.
     */
    record_value = db_find(key);
    if (record_value != NULL) {
        free(record_value);
        return -1;
    }

    new_rec.key = key;
    memcpy(new_rec.value, value, record_value_size * sizeof(char));

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

    if (leaf->num_of_keys < leaf_order - 1) {
        insert_into_leaf(leaf, leaf_offset, new_rec);
    }

    /* Case: right sibling leaf has room for record.
     */
    else if (has_room_for_record(leaf->next_offset)) {
        insert_into_leaf_using_key_rotation(leaf, leaf_offset, new_rec);
    }

    /* Case: leaf must be split.
     */
    else {
        insert_into_leaf_after_splitting(leaf, leaf_offset, new_rec);
    }

    // pwrite(fd, leaf, sizeof(page), leaf_offset);
    // free(leaf);
    return 0;
}


int db_delete(int64_t key) {
    
    page * leaf;
    off_t leaf_offset;

    leaf_offset = find_leaf_if_value_exists(key);
    if (leaf_offset == 0)
        return -1;

    leaf = load_page(leaf_offset);
    delete_entry(leaf, leaf_offset, key);
    
    return 0;
}//fin

