

local lu = require('luaunit')
local Stack = require('oop_stack')
local StackQueue = require('oop_stackqueue')

TestStack = {} --class

function TestStack:TestStackAdd(v)
    local st = Stack:New()
    st:push(1)
    lu.assertEquals(st:top(),1)
    lu.assertEquals(st:size(),1)
    st:push(2)
    lu.assertEquals(st:top(),2)
    lu.assertEquals(st:size(),2)
end

function TestStack:TestStackPop(v)
    local st = Stack:New()
    st:push(1)
    st:push(2)
    st:push(3)
    for i = 3,1,-1 do 
        lu.assertFalse(st:isEmpty())
        lu.assertEquals(st:top(),i)
        lu.assertEquals(st:size(),i)
        lu.assertEquals(st:pop(),i)
    end
    lu.assertTrue(st:isEmpty())
end

TestQueue = {} --class
function TestQueue:TestInsertBottom(v)
    local qe = StackQueue:New()
    -- order
    qe:insertbottom(3)
    qe:insertbottom(2)
    qe:insertbottom(1)
    for i = 3,1,-1 do 
        lu.assertFalse(qe:isEmpty())
        lu.assertEquals(qe:top(),i)
        lu.assertEquals(qe:size(),i)
        lu.assertEquals(qe:pop(),i)
    end
    lu.assertTrue(qe:isEmpty())
end

local runner = lu.LuaUnit.new()
runner:setOutputType("text")
os.exit( runner:runSuite() )