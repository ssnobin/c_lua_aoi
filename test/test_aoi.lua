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

local function print_aoi_events(aoi_events)
    print("==========print_aoi_events========")
    for watcher, grid_info in pairs(aoi_events) do
        print("watcher", watcher)
        for grid_idx, event_list in pairs(grid_info) do
            --print("grid_idx", grid_idx)
            for i = 1, 3 do
                local sub_event_list = event_list[i]
                if sub_event_list then
                    for _, e in ipairs(sub_event_list) do
                        if(e[1]==1) then
                            print(string.format("event:A,id:%d,x:%d,y:%d",e[2],e[3],e[4]))
                        elseif(e[1]==2) then
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
--my_aoi_test()
-- test()
-- m1_test()
-- my_test()
--my_test2()

local function test_my_aoi(times,n)
    local my_aoi = require "laoi"
    --local b = os.clock()
    --local n = 10000
    local world = my_aoi.create_world(9, 9)
    for i=1,n do
        local x = math.random(1,9)
        local y = math.random(1,9)
        my_aoi.add_obj(world,i,x,y,1,1)
    end
    --local e1 = os.clock()
    --print("test_my_aoi after_add:", e1 - b)
    my_aoi.update_aoi(world)
    local b = os.clock()
    --local e2 = os.clock()
    --print("analyze", my_aoi.get_time_cost())
    --print("test_my_aoi cost:", e2 - b, my_aoi.get_all_time_cost(), my_aoi.get_time_cost())
    --print("test_my_aoi cost:", e2 - b)
    for j=1,times do
        for i = 1,n do
            local x = math.random(1,9)
            local y = math.random(1,9)
            local id = i--math.random(1,n)
            --print("id is",id,x,y)
            my_aoi.set_obj(world, id, x, y)
            --print("set end")
            if i%1000 == 0 or i ==n then
                my_aoi.update_aoi(world) --??为啥这个次数多了就比lua版的满了。。。只有一次就比lua的快
                print("analyze", my_aoi.get_time_cost())
            end
        end
    end
    --my_aoi.update_aoi(world)
    local e3 = os.clock()
    print("test_my_aoi cost:", e3 - b)
    print("analyze", my_aoi.get_time_cost())
end

--test_my_aoi(1,10000)

function test()
    local my_aoi = require "laoi"
    local world = my_aoi.create_world(9, 9)
    --print(type(world))
    --print(world)
    local ret = my_aoi.test_create(world)
    --print(type(ret))
    --print(ret)

    my_aoi.test_set(world,3,2)
    local value = my_aoi.test_get(world,3)
    print(value)

    my_aoi.test_set(world,4,9)
    local value = my_aoi.test_get(world,4)
    print(value)

    --my_aoi.test_set(world,3,2)
    local value = my_aoi.test_get(world,3)
    print(value)

    print("set new uservalue")
    my_aoi.test_create(world)
    local value = my_aoi.test_get(world,3)
    print(value)

    local other_world = my_aoi.create_world(9, 9)
    local ret = my_aoi.test_create(other_world)
    local value = my_aoi.test_get(other_world,3)
    print(value)
end

test()