local m  = require "testLib"
assert(m ~= nil)
print(m.add(1,2))

local vec0 = m.Vec3f(1,2,3)
vec0:Display()
print(string.format("Vec0 %f %f %f\n",vec0.x,vec0.y,vec0.z))
-- from std::string to lua string
---@type string
local s = vec0:Text()
print(s:upper())

local circle = m.Circle(2.0)
assert(m.Shape_nShapes == 1)
print(circle:area())
circle = nil
collectgarbage()
assert(m.Shape_nShapes == 0)