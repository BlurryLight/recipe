-- high-order function
function derivative(f,delta)
    local delta = delta or 1e-5
    return function (x)
        return  (f(x + delta) - f (x - delta)) / (2.0 * delta)
    end
end

cosine = derivative(math.sin)
print(cosine(math.pi / 2),math.cos(math.pi/2))
print(cosine(math.pi),math.cos(math.pi))

-- lambda

network = 
{
    {name = "lua",IP = "202.197.45.16"},
    {name = "Python",IP = "202.197.45.13"}
}

table.sort(network,function (a,b) return a.name > b.name end)
for index,data in ipairs(network) do
    print(index)
    for key,value in pairs(data) do
        print("    ",key,value)
    end
end

-- table of functions
Lib = {
    tiktok = function() print("bytedance the best dance") end
}
Lib.foo = function () print('foo') end
Lib["bar"] = function () print('bar') end
function Lib.tencent () 
    local function timi()
        print("shut up and take my money")
    end
    timi()
end

Lib.foo()
Lib.bar()
Lib.tiktok()
Lib.tencent()

