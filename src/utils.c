#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#include "utils.h"

#define HASH_TABLE_MIN_HASH_SIZE 16
#define HASH_TABLE_MAX_HASH_SIZE 10240
#define HASH_TABLE_MIN_LOAD_FACTOR 0.1
#define HASH_TABLE_MAX_LOAD_FACTOR 0.8
#define HASH_TABLE_HASH_FUNC(tbl, key) ((key) % (tbl)->hash_sz)

HashTable* hashtbl_create() {
    HashTable* tbl = (HashTable*)malloc(sizeof(HashTable));

    tbl->hash_sz = HASH_TABLE_MIN_HASH_SIZE;
    tbl->count = 0;
    tbl->min_resize_cnt = (int)(tbl->hash_sz*HASH_TABLE_MIN_LOAD_FACTOR);
    tbl->max_resize_cnt = (int)(tbl->hash_sz*HASH_TABLE_MAX_LOAD_FACTOR);
    HashTableNode** nodes = (HashTableNode**)malloc(sizeof(HashTableNode*)*tbl->hash_sz);

    int n;
    for (n=0;n<tbl->hash_sz;n++){
        nodes[n] = NULL;
    }
    tbl->nodes = nodes;
    return tbl;
}

void hashtbl_destroy(HashTable* tbl){
    int n;
    HashTableNode *p, *next_p;
    for(n=0;n<tbl->hash_sz;n++){
        p=tbl->nodes[n];
        tbl->nodes[n]=NULL;
        while(p!=NULL){
            next_p=p->next;
            free(p);
            p=next_p;
        }
    }
    free(tbl->nodes);
    free(tbl);
    return;
}

static int _hashtbl_resize(HashTable* tbl, int new_hash_sz){
    HashTableNode** new_nodes = (HashTableNode**)malloc(sizeof(HashTableNode*)*new_hash_sz);
    int old_hash_sz = tbl->hash_sz;
    HashTableNode** old_nodes = tbl->nodes;
    tbl->nodes = new_nodes;
    tbl->hash_sz = new_hash_sz;
    tbl->min_resize_cnt = (int)(tbl->hash_sz*HASH_TABLE_MIN_LOAD_FACTOR);
    tbl->max_resize_cnt = (int)(tbl->hash_sz*HASH_TABLE_MAX_LOAD_FACTOR);

    int n, hash_key;
    HashTableNode* p, *next_p;
    for(n=0;n<new_hash_sz;n++){
        new_nodes[n] = NULL;
    }

    for(n=0;n<old_hash_sz;n++){
        p=old_nodes[n];
        old_nodes[n]=NULL;
        while(p!=NULL){
            next_p=p->next;

            hash_key=p->key%new_hash_sz;
            p->next=new_nodes[hash_key];
            new_nodes[hash_key]=p;

            p=next_p;
        }
    }
    free(old_nodes);
    return 0;
}

static int _hashtbl_insert(HashTable* tbl, int hash_key, uint64_t key, void* value){
    HashTableNode* node = malloc(sizeof(HashTableNode));
    node->key = key;
    node->value = value;
    node->next = tbl->nodes[hash_key];
    tbl->nodes[hash_key] = node;
    tbl->count++;

    if(tbl->hash_sz < HASH_TABLE_MAX_HASH_SIZE && tbl->count > tbl->max_resize_cnt){
        _hashtbl_resize(tbl, tbl->hash_sz*2);
    }
    return 0;
}

int hashtbl_insert(HashTable* tbl, uint64_t key, void* value){
    int hash_key = HASH_TABLE_HASH_FUNC(tbl, key);
    HashTableNode* p = tbl->nodes[hash_key];
    while(p != NULL){
        if(p->key == key){
            printf("table_insert fail, key<%lu>, already exist\n", key);
            assert(0);
            return 1;
        }
        p = p->next;
    }
    return _hashtbl_insert(tbl, hash_key, key, value);
}

int hashtbl_upsert(HashTable* tbl, uint64_t key, void* value){
    int hash_key = HASH_TABLE_HASH_FUNC(tbl, key);
    HashTableNode* p = tbl->nodes[hash_key];
    while(p != NULL){
        if(p->key == key){
            p->value = value;
            return 0;
        }
        p = p->next;
    }
    return _hashtbl_insert(tbl, hash_key, key, value);
}

int hashtbl_has(HashTable* tbl, uint64_t key){
    HashTableNode* p = tbl->nodes[HASH_TABLE_HASH_FUNC(tbl,key)];
    while(p!=NULL){
        if(p->key==key){
            return 1;
        }
        p=p->next;
    }
    return 0;
}

void* hashtbl_get(HashTable* tbl, uint64_t key){
    HashTableNode* p = tbl->nodes[HASH_TABLE_HASH_FUNC(tbl,key)];
    while(p!=NULL){
        if(p->key==key){
            return p->value;
        }
        p=p->next;
    }
    return 0;
}

int hashtbl_remove(HashTable* tbl, uint64_t key){
    int hash_key = HASH_TABLE_HASH_FUNC(tbl,key);
    HashTableNode* free_p;
    HashTableNode** p=&(tbl->nodes[hash_key]);
    int hash_sz=tbl->hash_sz;
    while((*p)!=NULL){
        if((*p)->key!=key){
            p=&((*p)->next);
            continue;
        }
        free_p = *p;
        *p=free_p->next;
        free(free_p);
        free_p=NULL;
        tbl->count--;
        if((hash_sz>HASH_TABLE_MIN_LOAD_FACTOR)&&(tbl->count<tbl->min_resize_cnt)){
            int min_hash_sz = (int)(tbl->count/HASH_TABLE_MAX_LOAD_FACTOR)+1;
            if(min_hash_sz<HASH_TABLE_MIN_HASH_SIZE){
                min_hash_sz=HASH_TABLE_MIN_HASH_SIZE;
            }
            int max_hash_sz = 2*min_hash_sz;
            while(hash_sz>=min_hash_sz){
                if(hash_sz<max_hash_sz){
                    break;
                }
                hash_sz=hash_sz/2;
            }
            _hashtbl_resize(tbl,hash_sz);
        }
        return 1;
    }
    return 0;
}

void hashtbl_iter_reset(HashTableIter* iter){
    iter->hash_sz=-1;
    iter->count=0;
    iter->node=NULL;
    return;
}

int hashtbl_iter(HashTable* tbl, HashTableIter* iter){
    if(tbl->count<=iter->count){
        return 0;
    }
    if(iter->node){
        iter->node = iter->node->next;
    }
    while(!iter->node){
        iter->hash_sz++;
        if(iter->hash_sz>=tbl->hash_sz){
            break;
        }
        iter->node=tbl->nodes[iter->hash_sz];
    }
    if(!iter->node){
        return 0;
    }
    iter->count++;
    return 1;
}

void hashtbl_foreach(HashTable* tbl, HashTableIterFunc func, void* ud){
    HashTableIter iter;
    hashtbl_iter_reset(&iter);
    while(hashtbl_iter(tbl,&iter)){
        func(ud, iter.node->key, iter.node->value);
    }
}