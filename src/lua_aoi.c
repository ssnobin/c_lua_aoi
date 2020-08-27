#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "utils.h"
#include "aoi.h"

static int create_world(lua_State* L){
    int isnum;
    int row = (int)lua_tointegerx(L, 1, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"row not number");
    }
    int col = (int)lua_tointegerx(L, 2, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"row not number");
    }
    World* w = aoi_create_world(row, col);
    if(w==NULL){
        return luaL_error(L, "create world fail");
    }
    lua_pushlightuserdata(L, w);
    return 1;
}

static int add_obj(lua_State* L){
    World* w = (World*)lua_touserdata(L, 1);
    if(w==NULL){
        return luaL_argerror(L,1,"no world");
    }
    int isnum;
    int id = (int)lua_tointegerx(L, 2, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"id not number");
    }
    int x = (int)lua_tointegerx(L, 3, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"id not number");
    }
    int y = (int)lua_tointegerx(L, 4, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"id not number");
    }
    int is_maker = (int)lua_tointegerx(L, 5, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"id not number");
    }
    int is_watcher = (int)lua_tointegerx(L, 6, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"id not number");
    }
    if(aoi_add_obj(w, id, x, y, is_maker, is_watcher)){
        return luaL_error(L, "add obj fail");
    }
    return 1;
}

static int del_obj(lua_State* L){
    World* w = (World*)lua_touserdata(L, 1);
    if(w==NULL){
        return luaL_argerror(L,1,"no world");
    }
    int isnum;
    int id = (int)lua_tointegerx(L, 2, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"id not number");
    }
    if(aoi_del_obj(w,id)){
        return luaL_error(L, "del obj fail");
    }
    return 1;
}

static int set_obj(lua_State* L){
    World* w = (World*)lua_touserdata(L, 1);
    if(w==NULL){
        return luaL_argerror(L,1,"no world");
    }
    int isnum;
    int id = (int)lua_tointegerx(L, 2, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"id not number");
    }
    int x = (int)lua_tointegerx(L, 3, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"id not number");
    }
    int y = (int)lua_tointegerx(L, 4, &isnum);
    if(!isnum){
        return luaL_argerror(L,1,"id not number");
    }
    if (aoi_set_obj(w,id,x,y)){
        return luaL_error(L, "set obj fail");
    }
    return 1;
}

static int update_aoi(lua_State* L){
    World* w = (World*)lua_touserdata(L, 1);
    if(w==NULL){
        return luaL_argerror(L,1,"no world");
    }
    lua_pop(L,1);
    lua_newtable(L);
    lua_newtable(L);
    aoi_update_aoi(w, L);
    lua_pop(L, 1);
    return 1;
}

static int get_time_cost(lua_State* L){
    aoi_get_cost_time(L);
    return 3;
}

static void add_results_to_L(lua_State* L, int grid_idx, int id, int key_idx)
{
    int idx_ret = 1;
    //将events upsert进L
    int idx_grid_info = lua_gettop(L);//2
    lua_rawgeti(L, idx_grid_info, grid_idx);
    if(lua_isnil(L,-1)){
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_grid_info, grid_idx);
    }
    int idx_grid_detail = lua_gettop(L);//3
    lua_rawgeti(L, idx_grid_detail, key_idx);
    if(lua_isnil(L, -1)){
        lua_pop(L,1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_grid_detail, key_idx);
    }
    int idx_grid_add_del_update = lua_gettop(L);//4
    size_t tbl_len = lua_rawlen(L, idx_grid_add_del_update);
    if(tbl_len==0){
        //printf("just do once 10000 events\n");
        for(int j = 1;j<=10000;j++){
            lua_newtable(L);
            lua_pushinteger(L,1);
            lua_rawseti(L, -2, 1);
            lua_pushinteger(L, 100);
            lua_rawseti(L, -2, 2);
            lua_pushinteger(L, 1);
            lua_rawseti(L, -2, 3);
            lua_pushinteger(L, 1);
            lua_rawseti(L, -2, 4);
            lua_rawseti(L, idx_grid_add_del_update, j);
        }
    }
    lua_rawgeti(L, idx_ret, id);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_ret, id);
    }
    int idx_watcher_ret = lua_gettop(L);//5
    lua_rawgeti(L, idx_watcher_ret, grid_idx);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);
        lua_newtable(L);
        lua_pushvalue(L,-1);
        lua_rawseti(L, idx_watcher_ret, grid_idx);
    }
    int idx_watcher_grid_ret = lua_gettop(L);//6
    lua_rawgeti(L, idx_watcher_grid_ret, key_idx);
    if(lua_isnil(L,-1)){
        lua_pop(L,1);
        lua_pushvalue(L, idx_grid_add_del_update);//引用
        lua_rawseti(L, idx_watcher_grid_ret, key_idx);
    }
    lua_pop(L,4);
}

static int test_lua_table(lua_State* L) {
    lua_newtable(L);
    lua_newtable(L);
    for(int id=1;id<=10000;id++){
        for(int grid_idx=0;grid_idx<=9;grid_idx++){
            for(int key_idx=1;key_idx<=3;key_idx++){
                add_results_to_L(L, grid_idx, id, key_idx);
            }
        }
    }
    lua_pop(L,1);
    return 1;
}

int luaopen_laoi(lua_State* L){
    luaL_Reg l[] = {
        {"create_world", create_world},
        {"add_obj", add_obj},
        {"del_obj", del_obj},
        {"set_obj", set_obj},
        {"update_aoi", update_aoi},
        {"get_time_cost", get_time_cost},
        {"test_lua_table", test_lua_table},
        {NULL, NULL},
    };

    luaL_newlib(L,l);
    return 1;
}