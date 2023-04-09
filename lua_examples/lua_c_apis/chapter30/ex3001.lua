function add_by_value(value)
    return function(i) 
        local res = i + value;
        return res;
    end
end

---@type fun(arr: number[]):nil
local l_map = my_map

---@param prompt string
---@param t number[]
function print_debug(prompt, t)
    print(prompt)
    print(table.concat(t, " "))
end

---@type number[]
t = { 1, 2, 3, 4, 5 }

print_debug("before map", t)

local add_three = add_by_value(3)

l_map(t, add_three)

print_debug("after map", t)

-- test split
print("test split")

---@type string
s = "Hi:Lua:Split:From:C:Client";

---@type fun(str: string): string[]
local l_split = my_split;
local split_s = l_split(s, ':');
print(table.concat(split_s, '_'))

print("test upper")
---@type fun(str: string): string
local l_string_upper = my_string_upper;
print(l_string_upper("i'm going to be an upper string"));

print("test concat")
---@type fun(arr: string[],seq: string): string
local l_string_concat = my_string_concat
print(l_string_concat({ "hello", "I", "am", "going", "to", "be", "concatted" }, "!"));