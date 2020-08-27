#ifndef _AOI_H
#define _AOI_H

#include "utils.h"

typedef struct aoi_event {
    char e;//"A","U","D"
    int id;
    int x;
    int y;
}Event;

typedef struct aoi_cache {
    char e;
    int id;
    int x;
    int y;
}Cache;

typedef struct aoi_obj {
    int id;
    int x;
    int y;
    int is_maker;
    int is_watcher;
}Obj;

typedef struct aoi_grid {
    int idx;
    HashTable* watchers;
    HashTable* makers;
    HashTable* caches;
}Grid;

typedef struct aoi_world {
    int row;
    int col;
    Grid** grids;
    HashTable* where_is;//id在哪个格子 实时的
    HashTable* pre_where_is;
}World;

World* aoi_create_world(int row, int col);

int aoi_add_obj(World* w, int id, int x, int y, int is_maker, int is_watcher);
int aoi_del_obj(World* w, int id);
int aoi_set_obj(World* w, int id, int x, int y);

void aoi_update_aoi(World* w, void* lua_state);

void aoi_get_cost_time(double* c1, double* c2, double* c3);
#endif