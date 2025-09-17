#ifndef BINARY_PLUS_TREE_H
#define BINARY_PLUS_TREE_H

#define MAX_KEYS 3

typedef struct BPTreeNode {
    int is_leaf;
    int num_keys;
    int keys[MAX_KEYS];
    void* pointers[MAX_KEYS + 1];
    struct BPTreeNode* next;
}BPTreeNode;

typedef struct {
    BPTreeNode* root;
} BPTree;

void* bpt_search(BPTreeNode* root, int key);
BPTreeNode* create_node(int is_leaf);
void bpt_insert_internal(BPTreeNode* tree, int key, BPTreeNode* right_child);
void bpt_insert(BPTree* tree, int key, void* row_ptr);



#endif