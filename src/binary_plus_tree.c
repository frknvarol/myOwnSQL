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
void bpt_insert_internal(BPTree* tree, BPTreeNode* parent, int key, BPTreeNode* right_child) {

    int i = parent->num_keys - 1;
    while (i >= 0 && key < parent->keys[i]) {
        parent->keys[i+1] = parent->keys[i];
        parent->pointers[i+2] = parent->pointers[i+1];
        i--;
    }
    parent->keys[i+1] = key;
    parent->pointers[i+2] = right_child;
    parent->num_keys++;

    if (parent->num_keys > MAX_KEYS) {
        int split = (MAX_KEYS + 1) / 2;
        BPTreeNode* new_internal = create_node(0);

        int mid_key = parent->keys[split];

        new_internal->num_keys = parent->num_keys - split - 1;
        for (int j = 0; j < new_internal->num_keys; j++) {
            new_internal->keys[j] = parent->keys[split + 1 + j];
            new_internal->pointers[j] = parent->pointers[split + 1 + j];
        }
        new_internal->pointers[new_internal->num_keys] = parent->pointers[parent->num_keys];

        parent->num_keys = split;

        if (parent == tree->root) {
            BPTreeNode* new_root = create_node(0);
            new_root->keys[0] = mid_key;
            new_root->pointers[0] = parent;
            new_root->pointers[1] = new_internal;
            new_root->num_keys = 1;
        }
    }

}


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

    while (!node->is_leaf) {
        parent = node;
        int i = 0;
        while (i < node->num_keys && key >= node->keys[i]) i++;
        node = (BPTreeNode*)node->pointers[i];
    }

    int i = node->num_keys - 1;
    while (i >= 0 && key < node->keys[i]) {
        node->keys[i + 1] = node->keys[i];
        node->pointers[i + 1] = node->pointers[i];
        i--;
    }
    node->keys[i] = key;
    node->pointers[i] = row_ptr;
    node->num_keys++;


    if (node->num_keys > MAX_KEYS) {
        int split = (MAX_KEYS + 1) / 2;
        BPTreeNode* new_leaf = create_node(1);

        new_leaf->num_keys = node->num_keys - split;
        for (int j = 0; j < new_leaf->num_keys; j++) {
            new_leaf->keys[j] = node->keys[split + j];
            new_leaf->pointers[j] = node->pointers[split + j];
        }

        node->num_keys = split;

        new_leaf->next = node->next;
        node->next = new_leaf;

        int promoted_key = new_leaf->keys[0];

        if (node == tree->root) {
            BPTreeNode* new_root = create_node(0);
            new_root->keys[0] = promoted_key;
            new_root->pointers[0] = node;
            new_root->pointers[1] = new_leaf;
            new_root->num_keys = 1;
            tree->root = new_root;
        } else {
            //insert the promoted key into parent
            bpt_insert_internal(tree, parent, promoted_key, new_leaf);
        }


    }

}

void free_node(BPTreeNode* node) {
    if (node == NULL) return;

    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            free_node(node->pointers[i]);
        }
    }
    free(node);
}

void free_tree(BPTree* tree) {
    free_node(tree->root);
    tree->root = NULL;
}
