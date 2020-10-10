//gcc -o aoi aoi.c utils.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "utils.h"
#include "aoi.h"

static int NEARBY_CEIL_OFFSETS[9][2] = {
  { 0,  0}, // center
  {-1,  1}, // TL
  { 0,  1}, // T
  { 1,  1}, // TR

  {-1,  0}, // L
  { 1,  0}, // R

  {-1, -1}, // BL
  { 0, -1}, // B
  { 1, -1}, // BR
};

static double cost_time_in_lua = 0.0;
static double cost_time_in_cal = 0.0;
static double cost_time_in_get_event = 0.0;

static int get_grid_idx(World* w, int x, int y){
    return (x-1)*(w->col)+y-1;
};
 
static void get_xy_by_grididx(World* w, int grid_idx, int* x, int* y){
    *x = (grid_idx / w->col) + 1;
    *y = grid_idx - ((*x)-1)*w->col + 1;
    return;
}

static int out_of_range(World* w, int grid_idx, int idx){
    int x1,y1;
    get_xy_by_grididx(w,grid_idx,&x1,&y1);
    int x2,y2;
    get_xy_by_grididx(w,idx,&x2,&y2);
    if((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)>2){
        return 1;
    }
    return 0;
}

static HashTable* get_nearby_grids(World* w, Grid* grid){
    // Grid** grid_list = (Grid**)malloc(9*sizeof(Grid*));
    // int x,y;
    // get_xy_by_grididx(w,grid->idx,&x,&y);
    // int i, _x, _y;
    // for(i=0; i<9; i++) {
    //     _x = x + NEARBY_CEIL_OFFSETS[i][0];
    //     _y = y + NEARBY_CEIL_OFFSETS[i][1];
    //     if (!(_x>=1&&_x<=w->row&&_y>=1&&_y<=w->col)){
    //         grid_list[i] = NULL;
    //         continue;
    //     }
    //     int tmp_idx = get_grid_idx(w, _x, _y);
    //     Grid* tmp_grid = w->grids[tmp_idx];
    //     grid_list[i] = tmp_grid;
    // }
    // local ret = {}
    // local cur_grididx = grid_obj.grid_idx
    // local col = aoi_obj.col
    // local grids = aoi_obj.grids
    int grid_num = w->row*w->col;
    HashTable* ret = hashtbl_create();

    int x,y;
    get_xy_by_grididx(w,grid->idx,&x,&y);
    int i, _x, _y;
    for(i=0; i<9; i++) {
        _x = x + NEARBY_CEIL_OFFSETS[i][0];
        _y = y + NEARBY_CEIL_OFFSETS[i][1];
        if (_x>=1&&_x<=w->row&&_y>=1&&_y<=w->col){
            int tmp_idx = get_grid_idx(w,_x,_y);
            Grid* grid = w->grids[tmp_idx];
            hashtbl_upsert(ret, tmp_idx, grid);
            //printf("get_nearby_grids %d %d\n",grid->idx,tmp_idx);
        }
    }

    // for (int i = -1; i<=1; i++){
    //     for (int j= -1; j<=1; j++){
    //         int index = i*w->col + grid->idx + j;
    //         if(index >= 0 && index < grid_num){
    //             Grid* grid = w->grids[index];
    //             hashtbl_upsert(ret, index, grid);
    //         }
    //     }
    // }
    //         local k = i*col + cur_grididx + j
    //         local grid = grids[k]
    //         if grid and not ret[k] then
    //             ret[k] = grid
    //         end
    //     end
    // end
    // return ret

    return ret;
}

static void print_event(int grid_idx, char flag, HashTable* events){
    HashTableIter iter;
    hashtbl_iter_reset(&iter);
    while(hashtbl_iter(events, &iter)){
        Event* e=(Event*)(iter.node->value);
        printf("flag:%c grid_idx:%d event:%c,id:%d,x:%d,y:%d\n",flag,grid_idx,e->e,e->id,e->x,e->y);
    }
}

static void get_grid_add_event(Grid* grid, lua_State* L){//你进入某个格子，要知道该格子的事情
    int idx_grid_info = lua_gettop(L);//2
    lua_rawgeti(L, idx_grid_info, grid->idx);
    if(lua_isnil(L,-1)){
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_grid_info, grid->idx);
    }
    int idx_grid_detail = lua_gettop(L);//3
    lua_rawgeti(L, idx_grid_detail, 1);//key_idx==1
    if(lua_isnil(L, -1)){
        lua_pop(L,1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_grid_detail, 1);
    }
    int idx_grid_add_del_update = lua_gettop(L);//4
    size_t tbl_len = lua_rawlen(L, idx_grid_add_del_update);
    if(tbl_len==0){
        int len = 1;

        HashTableIter iter;
        hashtbl_iter_reset(&iter);
        while(hashtbl_iter(grid->caches, &iter)){
            Cache* cache=(Cache*)(iter.node->value);
            if(cache->e=='A' || cache->e=='U'){
                lua_newtable(L);
                lua_pushinteger(L,1);//'A'
                lua_rawseti(L, -2, 1);
                lua_pushinteger(L, cache->id);
                lua_rawseti(L, -2, 2);
                lua_pushinteger(L, cache->x);
                lua_rawseti(L, -2, 3);
                lua_pushinteger(L, cache->y);
                lua_rawseti(L, -2, 4);

                lua_rawseti(L, idx_grid_add_del_update, len);
                len = len + 1;
            }
        }

        hashtbl_iter_reset(&iter);
        while(hashtbl_iter(grid->makers, &iter)){
            Obj* obj=(Obj*)(iter.node->value);
            if(!hashtbl_get(grid->caches, obj->id)){//如果在cache中且没进入event_adds，说明e是D
                lua_newtable(L);
                lua_pushinteger(L,1);//'A'
                lua_rawseti(L, -2, 1);
                lua_pushinteger(L, obj->id);
                lua_rawseti(L, -2, 2);
                lua_pushinteger(L, obj->x);
                lua_rawseti(L, -2, 3);
                lua_pushinteger(L, obj->y);
                lua_rawseti(L, -2, 4);

                lua_rawseti(L, idx_grid_add_del_update, len);
                len = len + 1;
            }
        }
    }
}

static void get_grid_del_event(Grid* grid, lua_State* L){//你离开某个格子，要知道该格子的事情
    int idx_grid_info = lua_gettop(L);//2
    lua_rawgeti(L, idx_grid_info, grid->idx);
    if(lua_isnil(L,-1)){
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_grid_info, grid->idx);
    }
    int idx_grid_detail = lua_gettop(L);//3
    lua_rawgeti(L, idx_grid_detail, 2);//key_idx==2
    if(lua_isnil(L, -1)){
        lua_pop(L,1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_grid_detail, 2);
    }
    int idx_grid_add_del_update = lua_gettop(L);//4
    size_t tbl_len = lua_rawlen(L, idx_grid_add_del_update);
    if(tbl_len==0){
        int len = 1;

        HashTableIter iter;
        hashtbl_iter_reset(&iter);
        //cache中还没加进makers的就不管了
        while(hashtbl_iter(grid->makers, &iter)){
            Obj* obj=(Obj*)(iter.node->value);
            lua_newtable(L);

            lua_pushinteger(L,2);//'D'
            lua_rawseti(L, -2, 1);
            lua_pushinteger(L, obj->id);
            lua_rawseti(L, -2, 2);
            lua_pushinteger(L, obj->x);
            lua_rawseti(L, -2, 3);
            lua_pushinteger(L, obj->y);
            lua_rawseti(L, -2, 4);

            lua_rawseti(L, idx_grid_add_del_update, len);
            len = len + 1;
        }
    }
}

static void get_grid_update_event(Grid* grid, lua_State* L){//你在某个格子附近晃，要知道该格子的事情
    int idx_grid_info = lua_gettop(L);//2
    lua_rawgeti(L, idx_grid_info, grid->idx);
    if(lua_isnil(L,-1)){
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_grid_info, grid->idx);
    }
    int idx_grid_detail = lua_gettop(L);//3
    lua_rawgeti(L, idx_grid_detail, 3);//key_idx==3
    if(lua_isnil(L, -1)){
        lua_pop(L,1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_grid_detail, 3);
    }
    int idx_grid_add_del_update = lua_gettop(L);//4
    size_t tbl_len = lua_rawlen(L, idx_grid_add_del_update);
    if(tbl_len==0){
        int len = 1;

        HashTableIter iter;
        hashtbl_iter_reset(&iter);
        while(hashtbl_iter(grid->caches, &iter)){
            Cache* cache=(Cache*)(iter.node->value);
            lua_newtable(L);
            if (cache->e=='A') {
                lua_pushinteger(L,1);//'A'
            }
            else if (cache->e =='D'){
                lua_pushinteger(L,2);
            }
            else{
                lua_pushinteger(L,3);
            }
            lua_rawseti(L, -2, 1);
            lua_pushinteger(L, cache->id);
            lua_rawseti(L, -2, 2);
            lua_pushinteger(L, cache->x);
            lua_rawseti(L, -2, 3);
            lua_pushinteger(L, cache->y);
            lua_rawseti(L, -2, 4);

            lua_rawseti(L, idx_grid_add_del_update, len);
            len = len + 1;
        }
    }
}

//结果处理
static void add_grid_events_to_watchers(Grid* grid, char e,int id, lua_State* L){
    //printf("add_grid_events_to_watchers %d %c %d\n", grid->idx, e, id);
    //clock_t t1 = clock();
    //HashTable* events;
    int key_idx;
    if (e=='U'){//3
        get_grid_update_event(grid, L);
        key_idx = 3;
    }
    else if(e=='A'){//1
        get_grid_add_event(grid, L);
        key_idx = 1;
    }
    else if(e=='D'){//2
        get_grid_del_event(grid, L);
        key_idx = 2;
    }
    //clock_t t2 = clock();
    //double t3 = (double)(t2-t1)/CLOCKS_PER_SEC;
    //cost_time_in_get_event += t3;

    //clock_t time_begin = clock();
    //不copy，直接用引用
    int idx_ret = 1;
    //将events upsert进L
    //printf("upsert events succ %d\n",idx_grid_add_del_update);

    lua_rawgeti(L, idx_ret, id);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_ret, id);
    }
    int idx_watcher_ret = lua_gettop(L);//5
    size_t tbl_len = lua_rawlen(L, idx_watcher_ret);
    lua_pushvalue(L,4);
    lua_rawseti(L,idx_watcher_ret,tbl_len+1);
    lua_pop(L,3);

    // lua_rawgeti(L, idx_watcher_ret, grid->idx);
    // if(lua_isnil(L,-1)){
    //     lua_pop(L,1);
    //     lua_newtable(L);
    //     lua_pushvalue(L,-1);
    //     lua_rawseti(L, idx_watcher_ret, grid->idx);
    // }
    // int idx_watcher_grid_ret = lua_gettop(L);//6
    // lua_pushvalue(L, 4);//引用,idx_grid_add_del_update==4
    // lua_rawseti(L, idx_watcher_grid_ret, key_idx);
    // lua_pop(L,4);
    //clock_t time_end = clock();
    //double time_cost = (double)(time_end-time_begin)/CLOCKS_PER_SEC;
    //cost_time_in_lua += time_cost;
    //printf("===cost_time_in_lua:%f====\n", cost_time_in_lua);
}

//watcher到了grid
static void resolve_change_watcher(World* w, Grid* grid, HashTable* right_grid_map, int pre_idx, int id, lua_State* L){
    //printf("resolve_change_watcher id:%d grid_idx:%d pre_idx:%d\n",id, grid->idx, pre_idx);
    HashTableIter iter;
    HashTable* left_grid_map;
    if (pre_idx!=-1){
        // int x,y;
        // get_xy_by_grididx(w,pre_idx,&x,&y);
        // int i, _x, _y;
        // for(i=0; i<9; i++) {
        //     _x = x + NEARBY_CEIL_OFFSETS[i][0];
        //     _y = y + NEARBY_CEIL_OFFSETS[i][1];
        //     if (!(_x>=1&&_x<=w->row&&_y>=1&&_y<=w->col)){
        //         continue;
        //     }
        //     int tmp_idx = get_grid_idx(w, _x, _y);
        //     Grid* tmp_grid = w->grids[tmp_idx];

        //     if(out_of_range(w, grid->idx, tmp_idx)){
        //         add_grid_events_to_watchers(tmp_grid, 'D', id, L);
        //     }
        //     else{
        //         add_grid_events_to_watchers(tmp_grid, 'U', id, L);
        //     }
        // }
        Grid* old_grid = w->grids[pre_idx];
        left_grid_map = get_nearby_grids(w, old_grid);
        
        hashtbl_iter_reset(&iter);
        while(hashtbl_iter(left_grid_map, &iter)){
            Grid* tmp_grid=(Grid*)(iter.node->value);
            if(hashtbl_get(right_grid_map, tmp_grid->idx)){
                add_grid_events_to_watchers(tmp_grid, 'U', id, L);
            }
            else{
                add_grid_events_to_watchers(tmp_grid, 'D', id, L);
            }
        }
    }

    hashtbl_iter_reset(&iter);
    while(hashtbl_iter(right_grid_map, &iter)){
        Grid* tmp_grid=(Grid*)(iter.node->value);
        if (pre_idx==-1 || !hashtbl_get(left_grid_map, tmp_grid->idx)){
            add_grid_events_to_watchers(tmp_grid, 'A', id, L);
        }
    }
    if (pre_idx!=-1){
        hashtbl_destroy(left_grid_map);
    }
    // for (int i = 0; i<9;i++){
    //     Grid* tmp_grid = grid_list[i];
    //     if(!tmp_grid){
    //         continue;
    //     }
    //     if(pre_idx!=-1 && !out_of_range(w, pre_idx, tmp_grid->idx)){
    //         continue;//之前已经get 'U'过了
    //     }
    //     add_grid_events_to_watchers(tmp_grid, 'A', id, L);
    // }
}
static int grid_add_obj(World* w, Grid* grid, int id, int x, int y, int is_maker, int is_watcher){
    if(is_maker){
        if(hashtbl_get(grid->caches, id)){
            //之前可能有D 在里面
            Cache* old_cache = (Cache*)hashtbl_get(grid->caches, id);
            free(old_cache);
            hashtbl_remove(grid->caches, id);
            old_cache = NULL;
            //printf("old cache id:%d e:%c x:%d y:%d \n", old_cache->id, old_cache->e, old_cache->x, old_cache->y);
        }
        Cache * cache = (Cache*)malloc(sizeof(Cache));
        cache->id=id;
        cache->x=x;
        cache->y=y;
        cache->e='A';
        hashtbl_insert(grid->caches, id, cache);
    }
    if(is_watcher){
        Obj* obj = (Obj*)malloc(sizeof(Obj));
        obj->id = id;
        obj->x = x;
        obj->y = y;
        obj->is_maker = is_maker;
        obj->is_watcher = is_watcher;
        hashtbl_insert(grid->watchers, obj->id, obj);
        hashtbl_upsert(w->watcher_grids, grid->idx, NULL);
    }
}

static int grid_del_obj(World* w, Grid* grid, int id){
    if(hashtbl_get(grid->makers, id)){//已经落地了
        //旧的cache清除
        if(hashtbl_get(grid->caches, id)){
            Cache* old_cache = (Cache*)hashtbl_get(grid->caches, id);
            free(old_cache);
            hashtbl_remove(grid->caches, id);
            old_cache = NULL;
        }
        //添加新的
        Obj* obj = (Obj*)hashtbl_get(grid->makers, id);
        Cache * cache = (Cache*)malloc(sizeof(Cache));
        cache->id=obj->id;
        cache->x=obj->x;
        cache->y=obj->y;
        cache->e='D';
        hashtbl_insert(grid->caches, obj->id, cache);
    }
    else if(hashtbl_get(grid->caches, id)){//只是存在cache中
        //当你从来没有出现过
        //printf("grid_del_obj remove_caches %d\n", id);
        Cache* cache = (Cache*)hashtbl_get(grid->caches, id);
        free(cache);
        hashtbl_remove(grid->caches, id);
        cache = NULL;
    }

    if (hashtbl_get(grid->watchers, id)){
        //printf("grid_del_obj remove_watchers %d\n", id);
        Obj* obj = (Obj*)hashtbl_get(grid->watchers, id);
        hashtbl_remove(grid->watchers, id);
        if(!obj->is_maker){
        //如果只是watcher的话，释放对象
            free(obj);
            obj=NULL;
        }
        if(grid->watchers->count == 0){
            hashtbl_remove(w->watcher_grids, grid->idx);
        }
    }
}

static void grid_update_obj(Grid* grid, int id){
    //watcher不需要update
    if(hashtbl_get(grid->makers, id) && !hashtbl_get(grid->caches, id)){
        //之前已经在的且没有caches的才需要添加cache
        Obj* obj = (Obj*)hashtbl_get(grid->makers, id);
        Cache * cache = (Cache*)malloc(sizeof(Cache));
        cache->id=obj->id;
        cache->x=obj->x;
        cache->y=obj->y;
        cache->e='U';
        hashtbl_insert(grid->caches, obj->id, cache);
    }
}

static void handle_cache(Grid* grid){
    HashTableIter iter;
    hashtbl_iter_reset(&iter);
    while(hashtbl_iter(grid->caches, &iter)){
        Cache* cache=(Cache*)(iter.node->value);
        iter.node->value = NULL;
        if(cache->e=='A' && !hashtbl_get(grid->makers, cache->id)){//之前在makers，D了再A，就会重复，无须处理
            //先D了去了其他格子，再回来，就不需要add_makers了
            Obj* obj;
            if(hashtbl_get(grid->watchers, cache->id)){
                obj = (Obj*)hashtbl_get(grid->watchers, cache->id);
            }
            else{
                obj = (Obj*)malloc(sizeof(Obj));
                obj->id = cache->id;
                obj->x = cache->x;
                obj->y = cache->y;
                obj->is_maker = 1;
                obj->is_watcher = 0;
            }
            hashtbl_insert(grid->makers, obj->id, obj);
        }
        else if(cache->e=='D'){
            Obj* obj = (Obj*)hashtbl_get(grid->makers, cache->id);
            hashtbl_remove(grid->makers, cache->id);
            free(obj);
            obj = NULL;
        }
        free(cache);
        cache=NULL;
    }
    hashtbl_destroy(grid->caches);
    grid->caches = hashtbl_create();
}

static void handle_aoi(World* w, Grid* grid, lua_State* L)
{

    HashTable* grid_map = get_nearby_grids(w, grid);
    HashTableIter iter2;

    HashTableIter iter;
    hashtbl_iter_reset(&iter);
    //Grid** grid_list = get_nearby_grids(w, grid);
    while(hashtbl_iter(grid->watchers, &iter)){
        Obj* watcher=(Obj*)(iter.node->value);
        int pre_idx = -1;
        int id = watcher->id;
        //printf("handle_aoi %d %d\n",grid->idx, id);
        if(hashtbl_get(w->pre_where_is, id)){
            int *p_idx = (int*)hashtbl_get(w->pre_where_is, id);
            pre_idx=*p_idx;
        }
        if (pre_idx == grid->idx){

            
            hashtbl_iter_reset(&iter2);
            while(hashtbl_iter(grid_map, &iter2)){
                Grid* tmp_grid=(Grid*)(iter2.node->value);
                // if(id==6) {
                //     printf("tmp_grid %d\n",tmp_grid->idx);
                // }
                add_grid_events_to_watchers(tmp_grid, 'U', id, L);
            }
            // for (int i = 0; i<9;i++){
            //     Grid* tmp_grid = grid_list[i];
            //     if(!tmp_grid){
            //         continue;
            //     }
            //     add_grid_events_to_watchers(tmp_grid, 'U', id, L);
            // }
        }
        else{
            resolve_change_watcher(w, grid, grid_map, pre_idx, id, L);
            if(hashtbl_get(w->pre_where_is, id)){
                int *p_idx = (int*)hashtbl_get(w->pre_where_is, id);
                *p_idx=grid->idx;
            }
            else{
                int* p_idx=(int*)malloc(sizeof(int));
                *p_idx=grid->idx;
                hashtbl_insert(w->pre_where_is, id, p_idx);
            }
        }
    }
    hashtbl_destroy(grid_map);
    //free(grid_list);
}

World* aoi_create_world(int row, int col){
    //printf("aoi_create_world %d %d\n",row, col);
    World* w = (World*)malloc(sizeof(World));
    w->row = row;
    w->col = col;

    int grid_num = w->row*w->col;
    w->grids = (Grid**)malloc(grid_num*sizeof(Grid*));
    for(int i=0;i<grid_num;i++){
        Grid* grid = (Grid*)malloc(sizeof(Grid));
        grid->idx = i;
        grid->watchers=hashtbl_create();
        grid->makers=hashtbl_create();
        grid->caches=hashtbl_create();

        w->grids[i] = grid;
    }
    w->where_is=hashtbl_create();
    w->pre_where_is=hashtbl_create();
    w->watcher_grids=hashtbl_create();
    return w;
}

void aoi_get_cost_time(void* l){
    lua_State* L = (lua_State*)l;
    lua_pushnumber(L, cost_time_in_get_event);
    lua_pushnumber(L, cost_time_in_lua);
    lua_pushnumber(L, cost_time_in_cal);
    cost_time_in_get_event = 0.0;
    cost_time_in_lua = 0.0;
    cost_time_in_cal = 0.0;
    return;
}

void aoi_update_aoi(World*w, void* l){
    //clock_t time_begin = clock();
    lua_State* L = (lua_State*)l;
    //printf("=========start update_aoi=========\n");
    int grid_num = w->row*w->col;
    //抛出事件

    //for(int i=0;i<grid_num;i++){
    //    Grid* grid = w->grids[i];
    //    handle_aoi(w, grid, L);
    //}
    HashTableIter iter;
    hashtbl_iter_reset(&iter);
    while(hashtbl_iter(w->watcher_grids, &iter)){
        int grid_idx = (iter.node->key);
        //printf("handle_aoi grid_idx %d\n", grid_idx);
        Grid* grid = w->grids[grid_idx];
        handle_aoi(w, grid, L);
    }
    //clock_t time_end = clock();
    //处理cache
    //printf("handle_cache\n");
    for(int i=0;i<grid_num;i++){
        Grid* grid = w->grids[i];
        handle_cache(grid);
    }

    //清空每个格子的events 通过lua_state自己清
    //printf("clean all grids events\n");
    
    //printf("=========end update_aoi=========\n");
    //clock_t time_end = clock();
    //double time_cost = (double)(time_end-time_begin)/CLOCKS_PER_SEC;
    //cost_time_in_cal += time_cost;
    //printf("===time_cost:%f====\n",cost_time_in_cal);
}

int aoi_add_obj(World* w, int id, int x, int y, int is_maker, int is_watcher){
    int idx = get_grid_idx(w, x, y);//函数结束会释放idx的，不能hashtbl_insert(&idx)
    Grid* grid = w->grids[idx];

    if(hashtbl_get(w->where_is, id)
        || hashtbl_get(grid->caches, id) 
        || hashtbl_get(grid->watchers, id)
        || hashtbl_get(grid->makers, id)
        ){
        printf("add_obj duplicated %d\n", id);
        return 1;
    }
    if(!is_maker&&!is_watcher){
        printf("add_obj no watcher_and_maker %d\n", id);
        return 2;
    }
    int *p_idx = (int*)malloc(sizeof(int));
    *p_idx=idx;//不同于p_idx=&idx
    hashtbl_insert(w->where_is, id, p_idx);
    grid_add_obj(w, grid, id, x, y, is_maker, is_watcher);
    return 0;
}

int aoi_del_obj(World* w, int id){
    int *p_idx = (int*)hashtbl_get(w->where_is, id);
    if(!p_idx){
        printf("del_obj not_exist %d\n", id);
        return 1;
    }
    Grid* grid = w->grids[*p_idx];
    grid_del_obj(w, grid, id);

    hashtbl_remove(w->where_is, id);
    free(p_idx);
    return 0;
}

int aoi_set_obj(World* w, int id, int x, int y){
    int *p_idx = (int*)hashtbl_get(w->where_is, id);
    if(!p_idx){
        printf("set_obj not_exist %d\n", id);
        return 1;
    }
    int idx = get_grid_idx(w, x, y);
    if (idx==*p_idx) {
        Grid* grid = w->grids[idx];
        grid_update_obj(grid, id);
    }
    else{
        Grid* old_grid = w->grids[*p_idx];
        Grid* new_grid = w->grids[idx];
        int is_maker = 0;
        int is_watcher = 0;
        if(hashtbl_get(old_grid->makers, id) || hashtbl_get(old_grid->caches, id)){
            is_maker = 1;
        }
        if(hashtbl_get(old_grid->watchers, id)){
            is_watcher = 1;
        }
        grid_del_obj(w, old_grid, id);
        grid_add_obj(w, new_grid, id, x, y, is_maker, is_watcher);

        *p_idx = idx;
    }
    return 0;
}

// int main() {
//     // World* w = aoi_create_world(3,3);
//     // aoi_add_obj(w,7,2,2,1,1);
//     // aoi_add_obj(w,1,2,2,1,1);
//     // aoi_add_obj(w,6,3,3,1,1);
//     // //aoi_update_aoi(w);
//     // //aoi_update_aoi(w);
//     // aoi_set_obj(w,7,2,3);
//     // //aoi_update_aoi(w);
//     // aoi_add_obj(w,2,1,1,1,1);
//     // aoi_set_obj(w,2,1,2);
//     // //aoi_update_aoi(w);
//     // //先加再删，等于从来没出现过
//     // aoi_add_obj(w,3,2,2,1,1);
//     // aoi_del_obj(w,3);
//     // //aoi_update_aoi(w);

//     // aoi_set_obj(w,2,3,3);
//     // aoi_set_obj(w,7,1,1);
//     //aoi_update_aoi(w);
//     // update_aoi(w);
//     // printf("======\n");
//     // del_obj(w,7);
//     // update_aoi(w);
//     // printf("=======\n");
//     // int grid_idx = get_grid_idx(w,1,2);
//     // int x,y;
//     // get_xy_by_grididx(w,grid_idx,&x,&y);
//     // printf("x:%d y:%d\n", x, y);
//     // add_obj(w,2,5,5,1,1);
//     // show_world(w);
//     // set_obj(w,1,6,6);
//     // show_world(w);
//     // del_obj(w,1);
//     // show_world(w);

//     // World* w = aoi_create_world(3,3);
//     // for(int i=1;i<=10000;i++)
//     // {
//     //     aoi_add_obj(w,i,1,1,1,1);
//     // }
//     // HashTable* aoi_results = hashtbl_create();
//     // aoi_update_aoi(w, aoi_results);
//     // //打印aoi_results
//     // HashTableIter iter;
//     // hashtbl_iter_reset(&iter);
//     // int i = 0;
//     // while(hashtbl_iter(aoi_results, &iter)){
//     //     Result* re=(Result*)(iter.node->value);

//     //     printf("watcher:%ld\n", iter.node->key);
//     //     int watcher_id = (int)iter.node->key;
//     //     HashTableIter tmp_iter;
//     //     hashtbl_iter_reset(&tmp_iter);
//     //     while(hashtbl_iter(re->add_results, &tmp_iter)){
//     //         Event* e=(Event*)(tmp_iter.node->value);
//     //         //printf("event:%c,id:%d,x:%d,y:%d\n",e->e,e->id,e->x,e->y);
//     //         i=i+1;
//     //         printf("i is %d\n",i);
//     //     }
//     //     // hashtbl_iter_reset(&tmp_iter);
//     //     // while(hashtbl_iter(re->del_results, &tmp_iter)){
//     //     //     Event* e=(Event*)(tmp_iter.node->value);
//     //     //     //printf("event:%c,id:%d,x:%d,y:%d\n",e->e,e->id,e->x,e->y);
//     //     // }
//     //     // hashtbl_iter_reset(&tmp_iter);
//     //     // while(hashtbl_iter(re->update_results, &tmp_iter)){
//     //     //     Event* e=(Event*)(tmp_iter.node->value);
//     //     //     //printf("event:%c,id:%d,x:%d,y:%d\n",e->e,e->id,e->x,e->y);
//     //     // }

//     //     hashtbl_destroy(re->add_results);
//     //     hashtbl_destroy(re->del_results);
//     //     hashtbl_destroy(re->update_results);
//     // }
//     // printf("clean aoi_results\n");
//     // //todo清空aoi_results
//     // hashtbl_destroy(aoi_results);
//     return 0;
// }
