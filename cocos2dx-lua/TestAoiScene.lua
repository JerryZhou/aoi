local laoi = require("laoi")

local TestAoiScene = class("TestAoiScene" , function()
    return cc.Scene:create()
end)
TestAoiScene.__index = TestAoiScene
TestAoiScene.map = nil
TestAoiScene.nodes = nil

--地图配置信息
local mapinfo = {
    width = display.width,
    height = display.height,
    x = 0,
    y = 0,
    divide = 6,
}

--静态区域中心点及范围
local range = 200
local cx, cy = display.cx, display.cy

--每次ADD增加的新结点数目
local newNodeCount = 10

local function newDrawNode(posX, posY, radius, solid)
    local draw = cc.DrawNode:create()
    draw:setPosition(posX, posY)
    if solid then
        draw:drawSolidCircle(cc.p(0,0), radius, 0, 30, 1.0, 1.0, cc.c4f(1,0,1,0.5))
    else
        draw:drawCircle(cc.p(0,0), radius, 0, 30, false, 1.0, 1.0, cc.c4f(1.0, 0.0, 0.0, 0.5))
    end
    return draw
end

function TestAoiScene:addNode(posX, posY, radius, viewRadius, tag)
    local node = cc.Node:create()
    local t = newDrawNode(0, 0, radius, true)
    node:addChild(t)
    t = newDrawNode(0, 0, viewRadius, false)
    node:addChild(t)
    node:setPosition(posX, posY)
    node.radius = viewRadius
    node:setTag(tag)
    self:addChild(node)

    local unit = laoi.new_unit_with_radius(tag, {posX, posY,radius})
    -- local unit = laoi.new_unit(tag, {posX, posY})
    node.info = unit

    self.map:unit_add(unit)
    table.insert(self.nodes, node)
end

function TestAoiScene:init()

    self.map = laoi.new_map({mapinfo.width, mapinfo.height}, mapinfo.divide, {mapinfo.x, mapinfo.y})
    self.nodes = {}
    local menu = cc.Menu:create()
    self:addChild(menu, 10)

    --随机增加若干个单位
    local label = cc.MenuItemLabel:create(cc.Label:createWithSystemFont("Test Add", "Helvetica", 30))
    label:setAnchorPoint(cc.p(0.5, 0.5))
    label:registerScriptTapHandler(function()
        for i=1,newNodeCount do
            self:addNode(math.random(0, mapinfo.width-1), math.random(0, mapinfo.height-1), math.random(10,50), math.random(10,100),  #self.nodes+1)
        end
        
    end)
    menu:addChild(label)

    --随机移动单位
    label = cc.MenuItemLabel:create(cc.Label:createWithSystemFont("Test Move", "Helvetica", 30))
    label:setAnchorPoint(cc.p(0.5, 0.5))
    label:registerScriptTapHandler(function()
        for k,node in pairs(self.nodes) do
            node:runAction(cc.MoveBy:create(math.random(15, 20), cc.p(math.random(-display.cx, display.cx), math.random(-display.cy, display.cy))))
        end
    end)
    menu:addChild(label)

    --进入区域内的单位会被隐藏
    label = cc.MenuItemLabel:create(cc.Label:createWithSystemFont("Test Static Area", "Helvetica", 30))
    label:setAnchorPoint(cc.p(0.5, 0.5))
    label:registerScriptTapHandler(function()
        local draw = newDrawNode(cx, cy, range, false)
        self:addChild(draw)
        self.areaFlag = true
    end)
    menu:addChild(label)


    self.drawLayer = cc.DrawNode:create()
    self:addChild(self.drawLayer, 999)

    menu:alignItemsVertically()

end

function TestAoiScene.createWithData(data)
    local scene = TestAoiScene.new()
    return scene
end

function TestAoiScene:onEnter()
    -- Engine:getUIManager():open("TestLayer")
end

function TestAoiScene:ctor()
    self:init()
end

function TestAoiScene:update(delta)
    self.drawLayer:clear()
    for k,node in pairs(self.nodes) do
        self.map:unit_update(node.info, {node:getPositionX(), node:getPositionY()})
        node:setVisible(true)
        
        for id, unit in pairs(self.map:unit_search(node.info, node.radius)) do
            local target = self:getChildByTag(unit:get_id())
            self.drawLayer:drawLine(cc.p(target:getPosition()), cc.p(node:getPosition()), cc.c4f(0,1,0,0.4))
        end
    end
    if self.areaFlag then
        for id, unit in pairs(self.map:search_circle(range, {cx, cy})) do
            self:getChildByTag(unit:get_id()):setVisible(false)
        end
    end
end

return TestAoiScene