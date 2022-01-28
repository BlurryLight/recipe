-- 闭包的捕获变量的特性可以在连续多次调用中保存状态

function values(t)
    local i = 0
    return function ()
        i = i + 1
        return t[i]
    end
end

tb = {10,20,30}
for element in values(tb) do
    print(element) -- should be 10\n,20\n,30\n
end
--another iterator

function allwords()
    local line = io.read() -- current line
    local pos = 1 -- current line pos
    return function()
        while line do
            local w,e = string.match(line,"(%w+)()",pos);
            if w then
                pos = e
                return w
            else
                line = io.read() -- next line
                pos = 1
            end
        end
        return nil -- no line
    end
end

-- for word in allwords() do
--     print(word)
-- end


-- above method will create a lambda every call, it's stateful
-- stateless generator

a = {"one","two","three"}



local function MyIPairsIter(table, i)
    i = i + 1
    local value = table[i]
    if value then
        return i, value
    end
end

local function MyIPairs(table)
    return  MyIPairsIter, table, 0
end

local PairsIter = function (table, key)
    return next(table, key)--lua 内置函数
end

local function MyPairs(table)
    return PairsIter, table, nil
end

for index,data in MyIPairs(a) do
    print(index,data)
end


-- another example 
-- ordered iterator for table

lines = {
    ["luaH_set"] = 10,
    ["luaH_get"] = 24,
    ["luaH_present"] = 48
}
-- naive way to sort the lines by key
tmp = {}
for key in pairs(lines) do
    tmp[#tmp+1] = key
end
table.sort(tmp)
for _,value in ipairs(tmp) do
    print(value,"\t",lines[value])
end

function pairsByKeysStateful(t,f) 
    local a = {}
    for n in pairs(t) do 
        a[#a + 1] = n
    end
    table.sort(a,f)
    local i = 0
    return function()
        i = i + 1
        return a[i],t[a[i]]
    end
end

local function pairsByKeysIter(invariant,i)
    i = i + 1
    local keys = invariant.word
    local values = invariant.values
    local key = keys[i]
    if key then
        return i,key,values[key]
    end
end

function pairsByKeysStateLess(t,f) 
    local a = {}
    for n in pairs(t) do 
        a[#a + 1] = n
    end
    table.sort(a,f)
    local invariant = {
        word = a,
        values = t
    }
    return pairsByKeysIter,invariant,0
    -- local i = 0
    -- return function()
    --     i = i + 1
    --     return a[i],t[a[i]]
    -- end
end

for name,line in pairsByKeysStateful(lines) do
    print(name,line)
end

for name,line in pairsByKeysStateful(lines,function (a,b) return a > b end) do
    print(name,line)
end

for _,name,line in pairsByKeysStateLess(lines,function (a,b) return a > b end) do
    print(name,line)
end