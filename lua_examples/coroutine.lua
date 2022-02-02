co = coroutine.create(function () 
    for i = 1,10 do
        print("co "..i)
        coroutine.yield()
    end
 end)
print(type(co))
print(coroutine.status(co))
print(coroutine.resume(co))
print(coroutine.status(co))
print(coroutine.resume(co))
print(coroutine.status(co))
co = nil

-- producer-cosumer
function producer ()
    return coroutine.create( function ()
        while true do
            local x = io.read()
            send(x)
        end
    end)
end

function filter(prod)
    return coroutine.create( function()
        for line = 1,math.huge do
            local x = receive(prod)
            x = string.format("%5d %s",line,x)
            send(x)
        end
    end
    )
end

function consumer (prod)
    -- while true do
        local x = receive(prod)
        io.write(x,"\n")
    -- end
end

function send(x)
    coroutine.yield(x)
end
function receive(prod)
    local status,value = coroutine.resume(prod)
    return value
end


consumer(filter(producer()))