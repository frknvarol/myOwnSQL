#ifndef BINARY_PLUS_TREE_H
#define BINARY_PLUS_TREE_H

#define MAX_KEYS 3
#include <stdint.h>

typedef struct BPTreeNode {
    int is_leaf;
    int num_keys;
    uint32_t keys[MAX_KEYS];
    void* pointers[MAX_KEYS + 1];
    struct BPTreeNode* next;
    struct BPTreeNode* previous;
}BPTreeNode;

typedef struct {
    BPTreeNode* root;
    int indexed_col;
} BPTree;

void* bpt_search_equals(const BPTreeNode* root, long int key);
BPTreeNode* bpt_search_greater_equal(BPTreeNode* root, long int key);
BPTreeNode* create_node(int is_leaf);
void bpt_insert_internal(BPTree* tree, BPTreeNode* parent, uint32_t key, BPTreeNode* right_child);
void bpt_insert(BPTree* tree, uint32_t key, void* row_ptr);
BPTreeNode* find_parent(BPTreeNode* root, BPTreeNode* child);
void free_node(BPTreeNode* node);
void free_tree(BPTree* tree);



#endif