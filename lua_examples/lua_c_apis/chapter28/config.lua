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