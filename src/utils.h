#ifndef _UTILS_H
#define _UTILS_H
#include <stdint.h>

typedef struct hashtbl_node {
    uint64_t key;
    void *value;
    struct hashtbl_node* next;
} HashTableNode;

typedef struct hashtbl {
    int hash_sz;
    int count;
    int max_resize_cnt;
    int min_resize_cnt;
    HashTableNode** nodes;
} HashTable;

typedef struct hashtbl_iter {
    int hash_sz;
    int count;
    HashTableNode *node;
} HashTableIter;

typedef void (*HashTableIterFunc)(void *ud, uint64_t key, void *value);

HashTable* hashtbl_create();

void hashtbl_destroy(HashTable *tbl);

int hashtbl_has(HashTable* tbl, uint64_t key);

void* hashtbl_get(HashTable* tbl, uint64_t key);

int hashtbl_insert(HashTable* tbl, uint64_t key, void* value);

int hashtbl_upsert(HashTable* tbl, uint64_t key, void* value);

int hashtbl_remove(HashTable* tbl, uint64_t key);

int hashtbl_resize(HashTable *tbl, int new_hash_size);

void hashtbl_iter_reset(HashTableIter* iter);

int hashtbl_iter(HashTable* tbl, HashTableIter* iter);

void hashtbl_foreach(HashTable* tbl, HashTableIterFunc func, void *ud);

#endif

