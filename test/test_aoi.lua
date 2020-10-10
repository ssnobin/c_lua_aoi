local key = ""
local function PrintTable(table , level)
    level = level or 1
    local indent = ""
    for i = 1, level do
        indent = indent.."  "
    end

    if key ~= "" then
        print(indent..key.." ".."=".." ".."{")
    else
        print(indent .. "{")
    end

    key = ""
    for k,v in pairs(table) do
        if type(v) == "table" then
            key = k
            PrintTable(v, level + 1)
        else
            local content = string.format("%s%s = %s", indent .. "  ",tostring(k), tostring(v))
            print(content)  
        end
    end
    print(indent .. "}")
end

local function old_print_aoi_events(aoi_events)
    print("==========print_aoi_events========")
    for watcher, grid_info in pairs(aoi_events) do
        print("watcher", watcher)
        for grid_idx, event_list in pairs(grid_info) do
            --print("grid_idx", grid_idx)
            for i = 1, 3 do
                local sub_event_list = event_list[i]
                if sub_event_list then
                    for _, e in ipairs(sub_event_list) do
                        if(e[1]==1 or e[1] == 'A') then
                            print(string.format("event:A,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
                        elseif(e[1]==2 or e[1] == 'D') then
                            print(string.format("event:D,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
                        else
                            print(string.format("event:U,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
                        end
                    end
                end
            end
            
        end
    end
end

local function print_aoi_events(aoi_events)
    print("==========print_aoi_events========")
    -- for watcher, grid_info in pairs(aoi_events) do
    --     print("watcher", watcher)
    --     for grid_idx, event_list in pairs(grid_info) do
    --         --print("grid_idx", grid_idx)
    --         for i = 1, 3 do
    --             local sub_event_list = event_list[i]
    --             if sub_event_list then
    --                 for _, e in ipairs(sub_event_list) do
    --                     if(e[1]==1 or e[1] == 'A') then
    --                         print(string.format("event:A,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
    --                     elseif(e[1]==2 or e[1] == 'D') then
    --                         print(string.format("event:D,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
    --                     else
    --                         print(string.format("event:U,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
    --                     end
    --                 end
    --             end
    --         end
            
    --     end
    -- end
    for watcher, watch_ret in pairs(aoi_events) do
        print("watcher", watcher)
        for _, data in ipairs(watch_ret) do
            for _, e in ipairs(data) do
                if(e[1]==1 or e[1] == 'A') then
                    print(string.format("event:A,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
                elseif(e[1]==2 or e[1] == 'D') then
                    print(string.format("event:D,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
                else
                    print(string.format("event:U,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
                end
            end
        end
    end
end

package.cpath = "../build/?.so;" .. package.cpath
local function my_aoi_test()
    local my_aoi = require "laoi"

    --print(my_aoi)
    local world = my_aoi.create_world(4,3)
    my_aoi.add_obj(world, 7, 2,2,1,1)--my_aoi.add_obj(world, 7, 2,2,1,1)
    my_aoi.add_obj(world, 1, 2,2,1,1)
    my_aoi.add_obj(world, 6, 3,3,1,1)
    print_aoi_events(my_aoi.update_aoi(world))
    print_aoi_events(my_aoi.update_aoi(world))

    my_aoi.set_obj(world,7,2,3)
    print_aoi_events(my_aoi.update_aoi(world))

    my_aoi.add_obj(world,2,1,1,1,1)
    my_aoi.set_obj(world,2,1,2)
    print_aoi_events(my_aoi.update_aoi(world))

    my_aoi.add_obj(world,3,2,2,1,1)
    my_aoi.del_obj(world,3)
    print_aoi_events(my_aoi.update_aoi(world))

    my_aoi.set_obj(world,2,3,3)
    my_aoi.set_obj(world,7,1,1)
    print_aoi_events(my_aoi.update_aoi(world))

    my_aoi.set_obj(world,2,3,2)
    my_aoi.set_obj(world,2,3,1)
    print_aoi_events(my_aoi.update_aoi(world))
end

local function my_aoi_lua_test()
    local my_aoi = require "aoi"
    --print(my_aoi)
    local aoi_obj = my_aoi.aoi_new(4,3)
    aoi_obj:aoi_add(7,2,2,true,true)
    aoi_obj:aoi_add(1,2,2,true,true)
    aoi_obj:aoi_add(6,3,3,true,true)
    
    print_aoi_events(aoi_obj:aoi_update())
    print_aoi_events(aoi_obj:aoi_update())

    aoi_obj:aoi_set(7,2,3)
    print_aoi_events(aoi_obj:aoi_update())

    aoi_obj:aoi_add(2,1,1,true,true)
    aoi_obj:aoi_set(2,1,2)
    print_aoi_events(aoi_obj:aoi_update())

    aoi_obj:aoi_add(3,2,2,true,true)
    aoi_obj:aoi_remove(3)
    print_aoi_events(aoi_obj:aoi_update())

    aoi_obj:aoi_set(2,3,3)
    aoi_obj:aoi_set(7,1,1)
    print_aoi_events(aoi_obj:aoi_update())

    aoi_obj:aoi_set(2,3,2)
    aoi_obj:aoi_set(2,3,1)
    print_aoi_events(aoi_obj:aoi_update())
end
--my_aoi_lua_test()
--my_aoi_test()
-- test()
-- m1_test()
-- my_test()
--my_test2()
local function bench_test_my_lua_aoi(times, n)
    local my_aoi = require "aoi"
    local b = os.clock()
    local aoi_obj = my_aoi.aoi_new(9,9)
    for i=1,n do
        local x = math.random(1,9)
        local y = math.random(1,9)
        aoi_obj:aoi_add(i,x,y,true,true)
    end
    local e2 = os.clock()
    --print("bench_test_my_lua_aoi add_cost:", e2 - b)
    aoi_obj:aoi_update()
    local b = os.clock()
    local cnt = 0
    for j =1,times do
        for i = 1,10000 do
            local x = math.random(1,9)
            local y = math.random(1,9)
            local id = math.random(1,n)
            aoi_obj:aoi_set(id,x,y)
            -- if i%10000 == 0 then
            --     aoi_obj:aoi_update()
            --     cnt = cnt + 1
            -- end
        end
        aoi_obj:aoi_update()
        cnt = cnt + 1
    end
    local e3 = os.clock()
    print("bench_test_my_lua_aoi cost:", cnt, e3 - b)
end

local ZiXunAoi = require "zixun.aoi"
local MAKER_MARK = ZiXunAoi.MAKER_MARK
local WATCHER_MARK = ZiXunAoi.WATCHER_MARK
local function bench_test_zixun(times, n)
    --local b = os.clock()
    local aoi_obj = ZiXunAoi.aoi_new(9, 9, 1, 1)
    for i=1,n do
        local x = math.random(0,8)
        local y = math.random(0,8)
        aoi_obj:aoi_add(i, MAKER_MARK|WATCHER_MARK, x, y)
    end
    --local e1 = os.clock()
    --print("test_zixun after_add:", e1 - b)
    local ret = aoi_obj:aoi_update()
    --{watcherid:[{makerid:xx}]} 其中{makerid:xx}只有一份，都是引用
    --PrintTable(ret)
    -- local e2 = os.clock()
    -- print("test_zixun cost:", e2 - b)
    local b = os.clock()
    local cnt = 0
    for j =1,times do
        for i = 1,10000 do
            local x = math.random(0,8)
            local y = math.random(0,8)
            local id = math.random(1,n)
            aoi_obj:aoi_set(id,x,y)
            -- if i%10 == 0then
            --     aoi_obj:aoi_update()
            --     cnt = cnt + 1
            -- end
        end
        aoi_obj:aoi_update()
        cnt = cnt + 1
    end
    --aoi_obj:aoi_update()
    local e3 = os.clock()
    print("bench_test_zixun cost:", cnt, e3 - b)
end

local function bench_test_my_c_aoi(times,n)
    local my_aoi = require "laoi"
    local b = os.clock()
    local world = my_aoi.create_world(9, 9)
    for i=1,n do
        local x = math.random(1,9)
        local y = math.random(1,9)
        my_aoi.add_obj(world,i,x,y,1,1)
    end
    local e2 = os.clock()
    --print("bench_test_my_aoi add_cost:", e2 - b)
    my_aoi.update_aoi(world)
    local b = os.clock()
    local cnt = 0
    for j=1,times do
        for i = 1,10000 do
            local x = math.random(1,9)
            local y = math.random(1,9)
            local id = math.random(1,n)
            my_aoi.set_obj(world, id, x, y)
            -- if i%10 == 0 then
            --     my_aoi.update_aoi(world)
            --     cnt = cnt + 1
            -- end
            --??为啥这个次数多了就比lua版的满了。。。只有一次就比lua的快
        end
        my_aoi.update_aoi(world)
        cnt = cnt + 1
    end
    local e3 = os.clock()
    print("bench_test_my_c_aoi cost:", cnt, e3 - b)
    --print("analyze", my_aoi.get_time_cost())
end

--同样的代码c更快
bench_test_zixun(100,10000)
bench_test_my_lua_aoi(100,10000)
bench_test_my_c_aoi(100,10000) --todo 优化点1， c的取周围9个格子 优化点2 返回的aoi_events少一层table结构 
--todo zixun的get_nearby_grids是有bug的。。
--少一层的aoi_events结果，速度就上去了=v=

function test()
    local my_aoi = require "laoi"
    -- local world = my_aoi.create_world(9, 9)
    -- local ret = my_aoi.test_create(world)

    -- my_aoi.test_set(world,3,2)
    -- local value = my_aoi.test_get(world,3)
    -- print(value)

    -- my_aoi.test_set(world,4,9)
    -- local value = my_aoi.test_get(world,4)
    -- print(value)

    -- --my_aoi.test_set(world,3,2)
    -- local value = my_aoi.test_get(world,3)
    -- print(value)

    -- print("set new uservalue")
    -- my_aoi.test_create(world)
    -- local value = my_aoi.test_get(world,3)
    -- print(value)

    -- local other_world = my_aoi.create_world(9, 9)
    -- local ret = my_aoi.test_create(other_world)
    -- local value = my_aoi.test_get(other_world,3)
    -- print(value)
    local world = my_aoi.create_world(9, 9)
    my_aoi.test_create(world)
    --my_aoi.test_set(world,1,2)
    local ret = my_aoi.test_get(world,1)
    print(ret)
end

--test()