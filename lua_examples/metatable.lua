t = {}
print(getmetatable(t)) -- nil

t1 = {
    __tostring = function () return 'lol' end,
    __metatable = "not your business"
}
setmetatable(t,t1)
print(t) -- print 'lol'

-- protect metatable
print(getmetatable(t))
-- print(setmetatable(t,{}))  -- cannot change a protected metatable

-- example
prototype = {x = 0,y = 0,width = 100,height = 100}
local mt = {}
-- mt.__index = function(_,key) --first arg will be the w,but it is ignored
--     return prototype[key]
-- end
-- more compact style
mt.__index = prototype
function new (o)
    setmetatable(o,mt)
    return o
end


w = new{x = 0,y = 0}
print(w.width)


-- default table
-- naive method
function setDefault(t,d)
    local mt = {__index = function() return d end}
    setmetatable(t,mt)
end

t = {x = 10,y = 20}
print(t.x,t.z) -- 10 nil
setDefault(t,100)
print(t.x,t.z) -- 10 100

-- advanced way
local mt = {__index = function(t) return t.___ end}
-- default value is saved in t.___
function setDefault2(t,d)
    t.___ = d
    setmetatable(t,mt)
end

setDefault2(t,1000)
print(t.x,t.z) -- 10 1000

-- advanced way3
local key = {} -- use local variable as key
local mt = {__index = function(t) return t[key] end}
-- default value is saved in t.___
function setDefault3(t,d)
    t[key] = d
    setmetatable(t,mt)
end
setDefault3(t,2000)
print(t.x,t.z) -- 10 2000

-- advanced way4 dual presentation
local defaults = {}
setmetatable(defaults,{__mode = 'k'})
local mt = {__index = function(t) return defaults[t] end}
function setDefault4(t,d)
    defaults[t] = d
    setmetatable(t,mt)
end
setDefault4(t,5000)
print(t.x,t.z) -- 10 5000 
t = nil
collectgarbage()
-- 由于defaults是弱引用表，t被设置为nil以后,defaults内的引用也被回收了,所以下面的应该什么也不输出
for k,v in pairs(defaults) do
    print("in defaults",v) 
end



-- proxy table
function track(t)
    local proxy = {}
    local mt = {}

    --override index
    mt.__index = function(_,key)
        print("*access to element " .. tostring(key))
        return t[key]
    end

    mt.__newindex = function(_,key,value)
        print("*update to element " .. tostring(key))
        t[key] = value
    end

    mt.__pairs = function()
        return function(_,key)
            local nextkey,nextvalue = next(t,key)
            if(nextkey) ~= nil then
                print("*traverrsing element" .. tostring(nextkey))
            end
            return nextkey,nextvalue
        end
    end
    
    mt.__len = function() return #t end

    setmetatable(proxy,mt)
    return proxy
end

t = {}
t = track(t)
t[2] = "hello"
print(t[2])
t[1] = "world"
for k,v in pairs(t) do
    print(v)
end

-- readonly table
-- naive way, have to create a local metatable per table
function ReadOnly (t)
    local proxy = {}
    local mt = {}
    mt.__index = t
    mt.__newindex = function(_,_,_)
        error("attempt to update a read-only table",2)
    end
    setmetatable(proxy,mt)
    return proxy
end

t = ReadOnly{"a","b","c"}
print(t[1]) -- a
print(t.name) -- nil
-- t[1] = 'z'  -- 通过proxy的手法，确保每一次查询和update都必定会触发__index和__newindex方法


local mt = {}
mt.__index = function(t,key) return rawget(t.___,key) end
mt.__newindex = function(_,_,_)
    error("attempt to update a read-only table",2)
end

function ReadOnly2 (t)
    local proxy = {___ = t}
    setmetatable(proxy,mt)
    return proxy
end

t = ReadOnly2{"a","b","c"}
print(t[1]) -- a
print(t.name) -- nil
-- t[1] = 'z'  -- 通过proxy的手法，确保每一次查询和update都必定会触发__index和__newindex方法