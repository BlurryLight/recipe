
--[=[
    ex 24.1

-- producer-cosumer producer drive
function producer (filter)
    while true do
        local x = io.read()
        send(filter,x)
    end
end

function filter(cons)
    return coroutine.create( function(x)
        for line = 1,math.huge do
            x = string.format("%5d %s",line,x)
            send(cons,x)
            x = receive()
        end
    end
    )
end

function consumer ()
    return coroutine.create (function(x)
        while true do
            io.write(x,"\n")
            x = receive()
        end
    end
    )
end

function send(cons,x)
    coroutine.resume(cons,x)
end

function receive()
    -- yield会返回resume时的第二个后面的参数
    -- 当协程第一次启动的时候，其funcion(x)接收来自resume的参数
    -- 协程内部是个循环，后续的x的更新需要靠resume-yeild来传递
    return coroutine.yield()
end


producer(filter(consumer()))

]=]--


-- transfer like longjmp

local function transfer(co)
    return coroutine.yield(co)
end

local function dispatch(startco)
    local stat,co = true,startco
    repeat
        stat,co = coroutine.resume(co)
    until not stat or co == nil
end

co1 = coroutine.create (
    function()
        print("co1 stage 1")
        transfer(co2) -- yeild co2, resume will get the co2 as return value
        print("co1 stage 2")
        transfer(co2)
        print("co1 stage 3")
        transfer(co2)
    end
)

co2 = coroutine.create (
    function()
        print("co2 stage 1")
        transfer(co1) -- yeild co1
        print("co2 stage 2")
        transfer(co1) 
        print("co2 stage 3")
        transfer(co2) 
        print("co2 stage 4")
        transfer(co3) 
    end
)
co3 = coroutine.create (
    function()
        print("co3 end")
        print("hello end")
    end
)

dispatch(co1)