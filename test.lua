local laoi = require "laoi"

local mapinfo = {
	width = 512,
	height = 512,
	x = 0,
	y = 0,
	divide = 2,
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

print("del units from map")
for i=1, unitcount do
	map:unit_del(unitlist[i])
end

table.foreach(map:get_state(), print)

