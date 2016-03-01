local laoi = require "laoi"
local math = math

--[[
local map = laoi.new_map(size, divide, pos)
size : {width, height}
divide : divide -- optional (default : 1)
pos : {posx, posy} -- optional (default: {0, 0})

map:unit_update(unit, pos)
unit : unit
pos : {posx, posy} -- optional, if nil just update tick

local unit = laoi.new_unit(id, pos)
id : number
pos : {posx, posy}

unit:get_tick() -- return last unit_update time
--]]

math.randomseed(os.time())

local function distance(x1, y1, x2, y2)
	return math.sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2))
end

local function tablesize(t)
	local idx = 0
	table.foreach(t, function()
		idx = idx + 1
	end)
	return idx
end

local function printf(fmt, ...)
	return io.stdout:write(string.format(fmt, ...))
end

if table.foreach == nil then
    table.foreach = function(table, func) 
        for key, value in pairs(table) do
            func(key, value)
        end
    end
end

local splitstr = string.rep("=", 10)
printf("%s %s %s\n", splitstr, "laoi.NodeEnum", splitstr)
table.foreach(laoi.NodeEnum, print)
printf("%s %s %s\n", splitstr, "laoi.MapEnum", splitstr)
table.foreach(laoi.MapEnum, print)
printf("%s %s %s\n", splitstr, "laoi.IMetaCacheCap", splitstr)
table.foreach(laoi.IMetaCacheCap, print)

local mapinfo = {
	width = 512,
	height = 512,
	x = 0,
	y = 0,
	divide = 6,
}

local unitcount = 2000
-- local unitlist = {}

local map = laoi.new_map({mapinfo.width, mapinfo.height},
			 mapinfo.divide,
			 {mapinfo.x, mapinfo.y})

-- add unit to map
for i=1, unitcount do 
	local x = math.random(0, mapinfo.width-1)
	local y = math.random(0, mapinfo.height-1)
	local unit = laoi.new_unit(i, {x, y})
	map:unit_add(unit)
	-- print("unit", unit:get_id(), unit:get_pos())
	-- table.insert(unitlist, unit)
end

-- get map state
printf("%s %s %s\n", splitstr, "map state", splitstr)
table.foreach(map:get_state(), print)

-- move unit
local unit = map:get_units()[3]
for i=1, 800 do
	local x = math.random(0, mapinfo.width-1)
	local y = math.random(0, mapinfo.height-1)
	map:unit_update(unit, {x, y})
end
printf("%s %s %s\n", splitstr, "unit tick", splitstr)
print("tick", unit:get_tick())
print("getcurtick", laoi.getcurtick())
print("getcurnano", laoi.getcurnano())
print("getnextnano", laoi.getnextnano())

-- get range
printf("%s %s %s\n", splitstr, "unit_search", splitstr)
local unit = map:get_units()[3]
for i=1, 500 do
	map:unit_search(unit, 20)
end

for id, unit in pairs(map:get_units()) do
	assert(id == unit:get_id())
end

local range = 20
local cx, cy = 20, 50
print("============ search_circle ============ ")
for id, unit in pairs(map:search_circle(range, {cx, cy})) do
	local x, y = unit:get_pos()
	local dis = distance(x, y, cx, cy)
	printf("search_circle,range=%d,%d(%d, %d),pos(%d, %d),dis=%f\n", range, id, x, y, cx, cy, dis)
end

print("============ unit_search ==============")
local centerunit = map:get_units()[5]
local cx, cy = centerunit:get_pos()
local cid = centerunit:get_id()
local range = 20
for id, unit in pairs(map:unit_search(centerunit, range)) do
	local x, y = unit:get_pos()
	local dis = distance(x, y, cx, cy)
	printf("unit_search,range=%d,%d(%d, %d),center=%d(%d, %d),dis=%f\n", range, id, x, y, cid, cx, cy, dis)
end

printf("%s %s %s\n", splitstr, "del units from map", splitstr)
map:unit_del_by_id(2)
printf("unit count=%d, after remove id=2\n", tablesize(map:get_units()))
map:unit_del(map:get_units()[3])
collectgarbage("collect")
collectgarbage("collect")

table.foreach(map:get_state(), print)

for i=1, unitcount do
	map:unit_del_by_id(i)
end

printf("%s %s %s\n", splitstr, "laoi.imeta_cache_clear", splitstr)
laoi.imeta_cache_clear()

laoi.dbg_dump_mapstate(map)
laoi.dbg_dump_map(map)
map = nil
collectgarbage("collect")
collectgarbage("collect")

print("end")

