#include "binary_plus_tree.h"
#include <stddef.h>
#include <stdlib.h>

void* bpt_search(BPTreeNode* root, int key) {
    if (root == NULL) return NULL;

    BPTreeNode* node = root;

    while (!node->is_leaf) {
        int i = 0;
        while (i < node->num_keys && key >= node->keys[i]) i++;
        node = (BPTreeNode*)node->pointers[i];
    }

    for (int i = 0; i < node->num_keys; i++) {
        if (node->keys[i] == key) {
            return node->pointers[i];
        }
    }
    return NULL;
}


BPTreeNode* create_node(int is_leaf) {
    BPTreeNode* node = malloc(sizeof(BPTreeNode));
    node->is_leaf = is_leaf;
    node->num_keys = 0;
    node->next = NULL;
    for (int i = 0; i < MAX_KEYS + 1; i++) {
        node->pointers[i] = NULL;
    }
    return node;
}


// TODO
void bpt_insert_internal(BPTreeNode* parent, int key, BPTreeNode* right_child) {


}


// TODO
void bpt_insert(BPTree* tree, int key, void* row_ptr) {
    if (tree->root == NULL) {
        tree->root = create_node(1);
        tree->root->keys[0] = key;
        tree->root->pointers[0] = row_ptr;
        tree->root->num_keys = 1;
        return;
    }

    BPTreeNode* node = tree->root;
    BPTreeNode* parent = NULL;

    // finds the leaf
    while (!node->is_leaf) {
        parent = node;
        int i = 0;
        while (i < node->num_keys && key >= node->keys[i]) i++;
        node = (BPTreeNode*)node->pointers[i];
    }

    //insert to the leaf TODO



    //split the leaf if there is an overflow TODO
}