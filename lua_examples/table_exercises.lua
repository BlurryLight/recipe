-- ex5.1
sunday = "monday"; monday = "sunday"
t = {sunday = "monday", [sunday] = monday}
print(t.sunday, t[sunday], t[t.sunday]) -- t[“sunday”] monday  t["monday"] = "sunday"   sunday

-- ex 5.2
a = {}
a.a = a
-- infact a.a.a.a == a.a
-- so a.a.a.a = 3  == (a.a = 3)
a.a.a.a = 3
print(a.a)
-- print(a.a.a.a) it will throw an errno because a.a now is a value instead of a table


t = {
    ["\a"] = "bell",
    ["\b"] = "back space",
    ["\f"] = "form feed",
    ["\n"] = "newline",
    --- ...and so on
}

print(t["\n"])
-- for k,v in pairs(t) do
--     print (k,v)
-- end

-- // ex5.4

function polynomial(t,x)
    local value = 0
    -- for k,v in ipairs(t) do
    --    value = value +  
    -- end
    for idx = 1,#t do
        value = value + t[idx] * (x ^ (idx - 1))
    end
    return value
end

-- ex5.5
function polynomial2(t,x)
    local value, pow = 0,1
    for _,v in ipairs(t) do
        value = value + v * pow
        pow = pow * x
    end
    return value
end

print(polynomial({1,2,3,1},2)) -- 1 + 2 * 2 ^1 + 3 * 2 ^2 + 1 * 2 ^3 = 25
print(polynomial2({1,2,3,1},2)) -- 1 + 2 * 2 ^1 + 3 * 2 ^2 + 1 * 2 ^3 = 25

-- ex5.6

function test_valid_seq(t) --seq: all elements are not nil, all keys are positive number from [1...n]
    local len = 0
    for k,v in pairs(t) do
        if(math.type(k) == "integer" and k > 0) then
            len = math.max(len,k)
        end
    end
    for i = 1,len do
        if t[i] == nil then 
            return false
        end
    end
    return true
end

print(test_valid_seq({"a","b","c"})) -- true
print(test_valid_seq({"a","b","nil",123,"c"})) -- true
print(test_valid_seq({"a","b",nil,"c"})) -- false



function insert_to(from,pos,to)
    local len_of_from = #from
    table.move(to,pos,#to,pos + len_of_from)
    table.move(from,1,#from,pos,to)
end

to = {1,2,3}
from = {'a','b'}
insert_to(from,1,to) --should be 'a','b',1,2,3

for i,v in ipairs(to) do
    print(i,v)
end

to = {1,2,3}
insert_to(from,2,to) --should be 1,'a','b',2,3
for i,v in ipairs(to) do
    print(i,v)
end

function my_concat(t,seq,start,endidx)
    local new_s = ""
    start = start or 1
    endidx = endidx or #t
    seq = seq or ""
    for k =start,endidx do
        new_s = new_s .. seq .. t[k]
    end
    return new_s
end

t = {}

local upperCase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
local lowerCase = "abcdefghijklmnopqrstuvwxyz"
local numbers = "0123456789"
local symbols = [[!@#$%&()*+-,./\:;<=>?^[]{}"]]

local characterSet = upperCase .. lowerCase .. numbers .. symbols

for i = 1,100000 do
	local rand = math.random(#characterSet)
	t[i] = string.sub(characterSet, rand, rand)
end

clock = os.clock()
s1 = table.concat(t)
print("table.concat",os.clock() - clock)

clock = os.clock()
s2 = my_concat(t)
print("my concat",os.clock() - clock)
print(s2 == s1)