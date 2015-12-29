local laoi = require "laoi"
local math = math

math.randomseed(os.time())

local function distance(x1, y1, x2, y2)
	return math.sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2))
end

local function printf(fmt, ...)
	return io.stdout:write(string.format(fmt, ...))
end

local mapinfo = {
	width = 512,
	height = 512,
	x = 0,
	y = 0,
	divide = 1,
}

local unitcount = 2000
local unitlist = {}

local map = laoi.new_map(mapinfo.width,
			 mapinfo.height,
			 mapinfo.x,
			 mapinfo.y,
			 mapinfo.divide)

-- add unit to map
for i=1, unitcount do 
	local x = math.random(0, mapinfo.width-1)
	local y = math.random(0, mapinfo.height-1)
	local unit = laoi.new_unit(i, x, y)
	map:unit_add(unit)
	-- print("unit", unit:get_id(), unit:get_pos())
	table.insert(unitlist, unit)
end

-- get map state
table.foreach(map:get_state(), print)

-- move unit
local unit = unitlist[3]
for i=1, 800 do
	local x = math.random(0, mapinfo.width-1)
	local y = math.random(0, mapinfo.height-1)
	map:unit_move(unit, x, y)
	map:unit_update(unit)
end

-- get range
local unit = unitlist[3]
for i=1, 500 do
	map:unit_search(unit, 20)
end

for id, unit in pairs(map:units()) do
	assert(id == unit:get_id())
end

local range = 20
for id, unit in pairs(map:search_circle(5, 5, range)) do
	local x, y = unit:get_pos()
	local dis = distance(x, y, 5, 5)
	printf("search_circle,range=%d,%d(%d, %d),pos(%d, %d),dis=%f\n", range, id, x, y, 5, 5, dis)
end

local centerunit = unitlist[5]
local cx, cy = centerunit:get_pos()
local cid = centerunit:get_id()
local range = 20
for id, unit in pairs(map:unit_search(centerunit, range)) do
	local x, y = unit:get_pos()
	local dis = distance(x, y, cx, cy)
	printf("unit_search,range=%d,%d(%d, %d),center=%d(%d, %d),dis=%f\n", range, id, x, y, cid, cx, cy, dis)
end

print("del units from map")
for i=1, unitcount do
	map:unit_del(unitlist[i])
end

-- table.foreach(map:get_state(), print)
map = nil
collectgarbage("collect")
collectgarbage("collect")

print("end")

