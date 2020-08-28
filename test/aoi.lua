local aoi_mt = {}
aoi_mt.__index = aoi_mt


local grid_mt = {}
grid_mt.__index = grid_mt

local NEARBY_CEIL_OFFSETS = {
  { 0,  0}, 
  {-1,  1}, 
  { 0,  1}, 
  { 1,  1}, 

  {-1,  0}, 
  { 1,  0}, 

  {-1, -1}, 
  { 0, -1}, 
  { 1, -1}, 
}

local function get_grid_idx(aoi_obj, x, y)
    return aoi_obj.col * (x-1) + y
end

local function get_xy_by_grididx(aoi_obj, grid_idx)
    local x = math.ceil(grid_idx / aoi_obj.col)
    local y = grid_idx - (x-1)*aoi_obj.col
    return x,y
end

local function out_of_range(aoi_obj, grid_idx, idx)
    local x1,y1 = get_xy_by_grididx(aoi_obj, grid_idx)
    local x2,y2 = get_xy_by_grididx(aoi_obj, idx)
    if((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2)>2) then
        return true
    end
    return false
end

local function grid_new(grid_idx)
    local obj = {
        grid_idx = grid_idx,
        watchers = {},
        objs = {},
        caches = {},
        
        add_events = nil,
        del_events = nil,
        update_events = nil,
    }
    return setmetatable(obj, grid_mt)
end

function grid_mt:grid_add(id, x, y, is_maker, is_watcher)
    if is_maker then
        local caches = self.caches
        caches[id] = {
            id = id,
            x = x,
            y = y,
            e = 'A',
        }
    end

    if is_watcher then
        local watchers = self.watchers
        watchers[id] = {
            id = id,
            x = x,
            y = y,
            is_maker = is_maker,
            is_watcher = is_watcher,
        }
    end
end



function grid_mt:grid_delete(id)
    local objs = self.objs
    if objs[id] then
        local obj = objs[id]
        self.caches[id] = {
            id = id,
            x = obj.x,
            y = obj.y,
            e = 'D',
        }
    elseif self.caches[id] then
        self.caches[id] = nil
    end

    local watchers = self.watchers
    if watchers and watchers[id] then
        watchers[id] = nil
    end
end


function grid_mt:grid_update(id)
    local objs = self.objs
    local caches = self.caches
    if objs[id] and not caches[id] then
        local obj = objs[id]
        caches[id] = {
            id = obj.id,
            x = obj.x,
            y = obj.y,
            e = 'U',
        }
    end
end



local function aoi_new(row, col)
    local obj = {
        row = row,
        col = col,
        grids = {},
        where_is = {},
        pre_where_is = {},
    }

    obj = setmetatable(obj, aoi_mt)

    local grid_num = obj.row*obj.col
    for i = 1, grid_num do
        obj.grids[i] = grid_new(i)
    end
    return obj
end


function aoi_mt:aoi_add(id, x, y, is_maker, is_watcher)
    if self.where_is[id] then
        error(string.format("duplicate aoi_add obj_id:%s", id))
    end
    if not is_maker and not is_watcher then
        error(string.format("no watcher and no maker id:%s", id))
    end

    local grid_idx = get_grid_idx(self, x, y)
    local grid_obj = self.grids[grid_idx]
    grid_obj:grid_add(id, x, y, is_maker, is_watcher)

    self.where_is[id] = grid_idx
end


function aoi_mt:aoi_remove(id)
    local grid_idx = self.where_is[id]
    if not grid_idx then
        error(string.format("no exist aoi_remove obj_id:%s", id))
    end

    --- delete obj from grid
    local grid_obj = self.grids[grid_idx]
    grid_obj:grid_delete(id)
    self.where_is[id] = nil
end


function aoi_mt:aoi_set(id, x, y)
    local cur_grid_idx = self.where_is[id]
    if not cur_grid_idx then
        error(string.format("no exist aoi_set obj_id:%s", id))
    end

    --- update obj from grid
    local new_grid_idx = get_grid_idx(self, x, y)
    local new_grid_obj = self.grids[new_grid_idx]
    if new_grid_obj.grid_idx == cur_grid_idx then
        new_grid_obj:grid_update(id)
    else
        local old_grid = self.grids[cur_grid_idx]
        local is_maker, is_watcher = false, false
        if old_grid.objs[id] or old_grid.caches[id] then
            is_maker = true
        end
        if old_grid.watchers[id] then
            is_watcher = true
        end
        old_grid:grid_delete(id)
        new_grid_obj:grid_add(id, x, y, is_maker, is_watcher)
        self.where_is[id] = new_grid_idx
    end
end


local function get_nearby_grids(aoi_obj, grid_obj)
    local ret = {}
    local x, y = get_xy_by_grididx(aoi_obj, grid_obj.grid_idx)
    --print("get_nearby_grids", x, y)
    for i = 1, 9 do
        local _x = x + NEARBY_CEIL_OFFSETS[i][1]
        local _y = y + NEARBY_CEIL_OFFSETS[i][2]
        --print("_x _y", _x, _y)
        if (not (_x>=1 and _x<=aoi_obj.row and _y >=1 and _y<=aoi_obj.col)) then
            --print("gg")
            ret[i] = nil
        else
            local idx = get_grid_idx(aoi_obj, _x, _y)
            --print("get_nearby_grids add", _x, _y, idx)
            ret[i] = aoi_obj.grids[idx]
        end
    end
    return ret
end

local function grid_get_update_event(self)
    if self.update_events then
        return self.update_events
    end
    self.update_events = {}
    for _, cache in pairs(self.caches) do
        table.insert(self.update_events, {
            cache.e,
            cache.id,
            cache.x,
            cache.y
        })
    end
    return self.update_events
end


local function grid_get_del_event(self)
    if self.del_events then
        return self.del_events
    end
    self.del_events = {}
    for _, obj in pairs(self.objs) do
        table.insert(self.del_events, {
            'D',
            obj.id,
            obj.x,
            obj.y,
        })
    end
    return self.del_events
end


local function grid_get_add_event(self)
    if self.add_events then
        return self.add_events
    end
    self.add_events = {}
    for _, cache in pairs(self.caches) do
        if cache.e == 'A' or cache.e == 'U' then
            table.insert(self.add_events, {
                'A',
                cache.id,
                cache.x,
                cache.y
            })
        end
    end
    for _, obj in pairs(self.objs) do
        if not self.caches[obj.id] then
            table.insert(self.add_events, {
                'A',
                obj.id,
                obj.x,
                obj.y,
            })
        end
    end
    
    return self.add_events
end


local function add_grid_events_to_watchers(grid_obj, e, watcher_id, result_tbl)
    --print("add_grid_events_to_watchers", grid_obj.grid_idx, e, watcher_id)
    local watch_ret = result_tbl[watcher_id]
    if not watch_ret then
        watch_ret = {}
        result_tbl[watcher_id] = watch_ret
    end

    local grid_ret = watch_ret[grid_obj.grid_idx]
    if not grid_ret then
        grid_ret = {}
        watch_ret[grid_obj.grid_idx] = grid_ret
    end

    local data = nil
    if e == "U" then
        data = grid_get_update_event(grid_obj)
        grid_ret[3] = data
    elseif e == "D" then
        data = grid_get_del_event(grid_obj)
        grid_ret[2] = data
    elseif e == "A" then
        data = grid_get_add_event(grid_obj)
        grid_ret[1] = data
    else
        error(string.format("invalid event:%s", e))
    end
end

local function resolve_change_watcher(self, grid_obj, grid_list, pre_idx, id, result_tbl)
    --print("resolve_change_watcher", grid_obj.grid_idx, pre_idx, id)
    if pre_idx then
        local x,y = get_xy_by_grididx(self, pre_idx)
        for i = 1, 9 do
            local _x = x + NEARBY_CEIL_OFFSETS[i][1]
            local _y = y + NEARBY_CEIL_OFFSETS[i][2]
            if (not (_x>=1 and _x<=self.row and _y >=1 and _y<=self.col)) then
                goto CONTINUE
            end
            local tmp_idx = get_grid_idx(self, _x, _y)
            local tmp_grid = self.grids[tmp_idx]
            if out_of_range(self, grid_obj.grid_idx, tmp_idx) then
                add_grid_events_to_watchers(tmp_grid, 'D', id, result_tbl)
            else
                add_grid_events_to_watchers(tmp_grid, 'U', id, result_tbl)
            end
            ::CONTINUE::
        end
    end
    for i = 1, 9 do
        local tmp_grid = grid_list[i]
        if not tmp_grid then
            goto CONTINUE
        end
        if (pre_idx and not out_of_range(self, pre_idx, tmp_grid.grid_idx)) then
            goto CONTINUE
        end
        add_grid_events_to_watchers(tmp_grid, 'A', id, result_tbl)
        ::CONTINUE::
    end
end

local function handle_aoi(self, grid_obj, result_tbl)
    --print("handle_aoi", grid_obj.grid_idx)
    local grid_list = get_nearby_grids(self, grid_obj)
    -- for i = 1, 9 do
    --     local tmp_grid = grid_list[i]
    --     if tmp_grid then
    --         print(tmp_grid.grid_idx)
    --     end
    -- end
    local watchers = grid_obj.watchers
    for watcher_id,_ in pairs(watchers) do
        local pre_idx = self.pre_where_is[watcher_id]
        if pre_idx == grid_obj.grid_idx then
            for i = 1, 9 do
                local tmp_grid = grid_list[i]
                if tmp_grid then
                    add_grid_events_to_watchers(tmp_grid, 'U', watcher_id, result_tbl)
                end
            end
        else
            resolve_change_watcher(self, grid_obj, grid_list, pre_idx, watcher_id, result_tbl)
            self.pre_where_is[watcher_id] = grid_obj.grid_idx
        end
    end
end


local function handle_cache(grid_obj)
    for _, cache in pairs(grid_obj.caches) do
        if cache.e == 'A' and not grid_obj.objs[cache.id] then
            local obj
            if grid_obj.watchers[cache.id] then
                obj = grid_obj.watchers[cache.id]
            else
                obj = {
                    id = cache.id,
                    x = cache.x,
                    y = cache.y,
                    is_maker = true,
                    is_watcher = false
                }
            end
            grid_obj.objs[obj.id] = obj
        elseif cache.e == 'D' then
            grid_obj.objs[cache.id] = nil
        end
    end
    grid_obj.caches = {}

    grid_obj.add_events = nil
    grid_obj.del_events = nil
    grid_obj.update_events = nil
end

function aoi_mt:aoi_update()
    local result_tbl = {}

    local grid_num = self.row * self.col
    for i = 1, grid_num do
        handle_aoi(self, self.grids[i], result_tbl)
    end
    for i = 1, grid_num do
        handle_cache(self.grids[i])
    end

    return result_tbl
end



return {
    aoi_new = aoi_new,
    WATCHER_MARK = WATCHER_MARK,
    MAKER_MARK = MAKER_MARK,
}

