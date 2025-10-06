#include "binary_plus_tree.h"

#include <stdio.h>
#include <stdlib.h>

void* bpt_search_equals(const BPTreeNode* root, const long int key) {
    if (root == NULL) return NULL;

    const BPTreeNode* node = root;

    while (!node->is_leaf) {
        int index = 0;
        while (index < node->num_keys && key >= node->keys[index]) index++;
        node = (BPTreeNode*)node->pointers[index];
    }

    for (int i = 0; i < node->num_keys; i++) {
        if (key == node->keys[i]) return node->pointers[i];
    }
    return NULL;
}

BPTreeNode* bpt_search_greater_equal(BPTreeNode* root, const long int key) {
    if (root == NULL) return NULL;

    BPTreeNode* node = root;

    while (!node->is_leaf) {
        int index = 0;
        while (index < node->num_keys && key >= node->keys[index]) index++;
        node = (BPTreeNode*)node->pointers[index];
    }

    for (int i = 0; i < node->num_keys; i++) {
        if (node->keys[i] >= key) return node;
    }
    return NULL;
}

BPTreeNode* create_node(const int is_leaf) {
    BPTreeNode* node = malloc(sizeof(BPTreeNode));
    node->is_leaf = is_leaf;
    node->num_keys = 0;
    node->next = NULL;
    node->previous = NULL;
    for (int i = 0; i < MAX_KEYS + 1; i++) {
        node->pointers[i] = NULL;
    }
    return node;
}


void bpt_insert_internal(BPTree* tree, BPTreeNode* node, const uint32_t key, BPTreeNode* right_child) {
    int i = node->num_keys - 1;
    while (i >= 0 && key < node->keys[i]) {
        node->keys[i + 1] = node->keys[i];
        node->pointers[i + 2] = node->pointers[i + 1];
        i--;
    }

    node->keys[i + 1] = key;
    node->pointers[i + 2] = right_child;
    node->num_keys++;

    if (node->num_keys > MAX_KEYS) {
        const int split = node->num_keys / 2;

        BPTreeNode* new_internal = create_node(0);

        const uint32_t mid_key = node->keys[split];

        new_internal->num_keys = node->num_keys - split - 1;
        for (int j = 0; j < new_internal->num_keys; j++) {
            new_internal->keys[j] = node->keys[split + 1 + j];
            new_internal->pointers[j] = node->pointers[split + 1 + j];
        }

        new_internal->pointers[new_internal->num_keys] = node->pointers[node->num_keys];

        node->num_keys = split;

        if (node == tree->root) {
            BPTreeNode* new_root = create_node(0);
            new_root->keys[0] = mid_key;
            new_root->pointers[0] = node;
            new_root->pointers[1] = new_internal;
            new_root->num_keys = 1;
            tree->root = new_root;
        } else {
            bpt_insert_internal(tree, find_parent(tree->root, node), mid_key, new_internal);
        }
    }
}

void bpt_insert(BPTree* tree, const uint32_t key, void* row_ptr) {
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
        int index = 0;
        while (index < node->num_keys && key >= node->keys[index]) index++;
        node = (BPTreeNode*)node->pointers[index];
    }

    int i = node->num_keys - 1;
    while (i >= 0 && key < node->keys[i]) {
        node->keys[i + 1] = node->keys[i];
        node->pointers[i + 1] = node->pointers[i];
        i--;
    }

    node->keys[i + 1] = key;
    node->pointers[i + 1] = row_ptr;
    node->num_keys++;

    if (node->num_keys > MAX_KEYS) {
        const int split = node->num_keys / 2;
        BPTreeNode* new_leaf = create_node(1);

        new_leaf->num_keys = node->num_keys - split;

        for (int j = 0; j < new_leaf->num_keys; j++) {
            new_leaf->keys[j] = node->keys[split + j];
            new_leaf->pointers[j] = node->pointers[split + j];
        }

        node->num_keys = split;

        new_leaf->next = node->next;
        if (new_leaf->next) new_leaf->next->previous = new_leaf;
        new_leaf->previous = node;
        node->next = new_leaf;

        const uint32_t promoted_key = new_leaf->keys[0];

        if (node == tree->root) {
            BPTreeNode* new_root = create_node(0);
            new_root->keys[0] = promoted_key;
            new_root->pointers[0] = node;
            new_root->pointers[1] = new_leaf;
            new_root->num_keys = 1;
            tree->root = new_root;
        } else {
            bpt_insert_internal(tree, parent, promoted_key, new_leaf);
        }
    }

}



BPTreeNode* find_parent(BPTreeNode* root, BPTreeNode* child) {
    if (root == NULL || root->is_leaf) return NULL;

    for (int i = 0; i <= root->num_keys; i++) {
        if ((BPTreeNode*)root->pointers[i] == child) return root;
    }

    for (int i = 0; i <= root->num_keys; i++) {
        BPTreeNode* ptr = root->pointers[i];
        if (!ptr) continue;
        BPTreeNode* result = find_parent(ptr, child);
        if (result) return result;
    }
    return NULL;
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

    free(tree);
}
