-- define window size
---@type number
width = 200;
---@type number
height = 300;

if string.find(string.lower(os.getenv("OS")), string.lower("Windows")) then
    width = 400;
    height = 600;
end

background = 'Blue'

function f(x,y)
    return (x ^ 2 * math.sin(y)) / (1 - x) + 2.5
end

---@param flag boolean
---@param s string 
function hello(flag,s)
    if(flag) then
        return "hello" ..  s;
    else
        return "world" ..  s;
    end
end