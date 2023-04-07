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
