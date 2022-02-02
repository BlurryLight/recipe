local lib = require "async-lib"
local mem = {}
local tasks = 0
setmetatable(mem,{__mode = 'k'})
function run(code)
    local co = coroutine.wrap(
        function()
            code()
            tasks = tasks - 1
            if(tasks == 0) then
                lib.stop()
            end
        end
    )
    tasks = tasks + 1
    co()
end

function putline(stream,line)
    local co = coroutine.running()
    local callback = mem[co]
    if not callback then
        callback = (function() coroutine.resume(co) end)
        mem[co] = callback
    end
    lib.writeline(stream,line,callback)
    coroutine.yield()
end

function getline(stream,line)
    local co = coroutine.running()
    local callback = mem[co]
    if not callback then
        callback = (function(l) coroutine.resume(co,l) end)
        mem[co] = callback
    end
    lib.readline(stream,callback)
    local line = coroutine.yield()
    return line
end

-- task1

run(function()
    local t = {}
    -- local inp = io.input()
    local inp = assert(io.open("coroutine.lua","r"))
    local out = io.output()
    while true do
        local line = getline(inp)
        if not line then break end
        t[#t+1] = line
    end
    for i = #t,1,-1 do
        putline(out,t[i].."\n")
    end
end)

-- task2
run(function()
    local t = {}
    local inp = io.input()
    local out = io.output()
    while true do
        local line = getline(inp)
        if not line then break end
        t[#t+1] = line
    end
    for i = #t,1,-1 do
        putline(out,t[i].."\n")
    end
end)

lib.runloop()
