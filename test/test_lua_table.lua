package.cpath = "../build/?.so;" .. package.cpath
local function add_sth_to_table(t, grids, grid_idx, id, key_idx)
    local grid_info = grids[grid_idx]
    if not grid_info then
        grid_info = {}
        grids[grid_idx] = grid_info
    end
    local events = grid_info[key_idx]
    if not events then
        events = {}
        grid_info[key_idx] = events
    end
    if #events==0 then
        --print("wtf")
        for i = 1,10000 do
            -- local event={}
            -- table.insert(event,1)
            -- table.insert(event,100)
            -- table.insert(event,1)
            -- table.insert(event,1)
            -- table.insert(events,event)
            --{xx,xx}比event[#event+1]=1快，event[#event+1]=1比table.insert快
            -- local event={}
            -- event[#event+1]=1
            -- event[#event+1]=100
            -- event[#event+1]=1
            -- event[#event+1]=1
            -- table.insert(events,event)
            table.insert(events,{1,100,1,1})--这两种差别很大
            --events[#events+1] = {1,100,1,1}
        end
    end
    local watcher_grids = t[id]
    if not watcher_grids then
        watcher_grids = {}
        t[id] = watcher_grids
    end
    local watcher_grid_info = watcher_grids[grid_idx]
    if not watcher_grid_info then
        watcher_grid_info = {}
        watcher_grids[grid_idx] = watcher_grid_info
    end
    local watcher_grid_events = watcher_grid_info[key_idx]
    if not watcher_grid_events then
        watcher_grid_info[key_idx] = events
    end
end

local function gen_lua_table(grids)
    if not grids then
        grids = {}
    end
    local t = {}
    for id = 1, 10000 do
        for grid_idx = 0, 9 do
            for key_idx = 1, 3 do
                add_sth_to_table(t, grids, grid_idx, id, key_idx)
            end
        end
    end
    return t
end

local function test_aoi_lua_table()
    local grids = {}
    for grid_idx = 0, 9 do
        grids[grid_idx] = {}
        for key_idx = 1, 3 do
            grids[grid_idx][key_idx] = {}
            for j=1,10000 do
                table.insert(grids[grid_idx][key_idx], {1,100,1,1})
            end
        end
    end
    --前面grids数据的生成是不计入耗时的，因为两个aoi的比较，lua版的grids本身就是生成好的。而c版（的不止要生成c的还要生成lua的）就是需要重新生成grids数据
    local b = os.clock()
    for i=1,10 do
        gen_lua_table(grids)
        --local ret = gen_lua_table()
        --print_aoi_events(ret)
    end
    local e = os.clock()
    print("test_aoi_lua_table total_cost:", e - b)
end

local function test_lua_table()
    local b = os.clock()
    for i=1,10 do
        gen_lua_table()
        --local ret = gen_lua_table()
        --print_aoi_events(ret)
    end
    local e = os.clock()
    print("test_lua_table total_cost:", e - b)
end

local function test_c_handle_luatable()
    local my_aoi = require "laoi"
    local b = os.clock()
    for i=1,10 do
        my_aoi.test_lua_table()
        -- local ret = my_aoi.test_lua_table()
        --print_aoi_events(ret)
    end
    local e = os.clock()
    print("test_c_handle_luatable total_cost:", e - b)
    
end
test_aoi_lua_table()
test_lua_table()
test_c_handle_luatable()