# laoi

## Functions for Module

### laoi.new_map(size, divide, pos)
##### Description
create a map userdata "cls{aoi_map}"
##### Params
```size``` : {width, height}, size of map

```divide``` : number, see [aoi.c](https://github.com/rocaltair/aoi/blob/master/aoi.c)

```pos``` : {x, y}, option({0, 0} default)

##### Returns 
userdata: map"cls{aoi_map}"

### laoi.new_unit(unit_id, pos)
##### Description
create a unit userdata "cls{aoi_unit}"
##### Params
```unit_id``` : integer number

```pos``` : {x, y}
##### Returns
userdata : unit "cls{aoi_unit}"

### laoi.imeta_cache_clear(type)
##### Description
in fact, every obj release would be cached other than calling free(ptr),
you need call this function to release immediately.
##### Params
```type``` : string, option("all" default)

type values:

```
"iobj"
"iref"
"ireflist"
"irefjoint"
"inode"
"iunit"
"imap"
"irefcache"
"ifilter"
"isearchresult"
"irefautoreleasepool"
```

### laoi.getcurnano()
##### Returns
nano : number
### laoi.getcurtick()
##### Returns
tick : number
### laoi.getnextnano()
##### Returns
nano

## Debug Utility Functions
### laoi.dbg_dump_map()
##### Description
dump all dbg info
### laoi.dbg_dump_mapstate()
##### Description
dump map state info


## Method for Map
### map:get_size()
##### Description
return map size
##### Returns
width : number

height : number
### map:get_state()
##### Description
return map state
##### Returns
state_map : table

such as :

```
{
	nodecount = 234,
	leafcount = 123,
	unitcount = 789,
}
```

### map:get_units()
##### Description
return units_map
##### Returns
units_map : table

```
{
	[uid] = unit_userdata,
}
```

### map:search_circle(circle_range, center_pos)
##### Description
search all units around ```center_pos``` within ```circle_range```
##### Params
```circle_range``` : number

center_pos : table, {x, y}
##### Returns
units_map : table

```
{
	[uid] = unit_userdata,
}
```

### map:unit_search(unit, circle_range)
##### Description
search all units around ```unit``` within ```circle_range```
##### Params
```unit``` : unit "cls{aoi_unit}"
##### Returns
units_map : table

circle_range : number

```
{
	[uid] = unit_userdata,
}
```

### map:unit_add
##### Description(unit)
add a unit into map
##### Params
```unit``` : unit "cls{aoi_unit}"
### map:unit_del(unit)
##### Description
delete a unit from map
##### Params
```unit``` : unit "cls{aoi_unit}"
### map:unit_del_by_id(unit_id)
##### Description
delete a unit from map by unit_id
##### Params
```unit_id``` : number
### map:unit_update(unit, pos)
##### Description
update position for unit in map
##### Params
```unit``` : unit "cls{aoi_unit}"

pos : table, {x, y}

## Method for Unit
### unit:get_pos()
##### Description
get position of unit
##### Returns
x : number

y : number

### unit:get_id()
##### Description
get id of unit
##### Returns
id : number

### unit:get_tick()
##### Description
get tick of unit
##### Returns
tick : number

### unit:get_dispow2(other_unit)
##### Description
pow(distance, 2) of unit and other_unit
##### Params
```other_unit``` : unit "cls{aoi_unit}"
##### Returns
distance_power2 : number, (distance * distance)
