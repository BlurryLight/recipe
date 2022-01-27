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

-- lexical scope
-- normal way
names = {"Peter","Paul","Mary"}
grades = {Mary = 10,Peter=8,Paul = 7}
table.sort(names,function(n1,n2) return grades[n1] > grades[n2] end) -- decendent order
for index,data in ipairs(names) do
    print(data)
end

function sortByGrades(names_,grades_)
    -- note: the cmp lambda can capture grades which is outside the function scope
    table.sort(names_,function(n1,n2) return grades_[n1] > grades_[n2] end)
end

-- another example

function newCounter()
    local count = 0
    return function()
        count = count + 1
        return count
    end
end
    
c1 = newCounter() -- count = 0
print(c1()) -- count = 1
print(c1()) -- count = 2
-- the lambda capture the count, and extend its lifetime as long as the lambda
c1 = nil
-- now the count is invalid

math.randomseed(os.time())
function integral_monte_carlo(f,samples)--low bound
    samples = samples or 10000
    local sum = 0
    local function calc(lb,ub)
        for i = 1,samples do
            local sample = math.random() * (ub - lb) + lb
            sum = sum + f(sample) * (ub - lb)
        end
        sum = sum / samples
        return sum
    end
    return calc
end

function integral(f,samples)--low bound
    samples = samples or 10000
    local sum = 0
    local function calc(lb,ub)
        local step = (ub - lb) / samples
        for i = 1,samples do
            sum = sum + f(lb + (i - 1) * step) * step
        end
        return sum
    end
    return calc
end

f1 = integral_monte_carlo(math.cos) -- == math.sin
f2 = integral(math.cos) -- == math.sin
print(f1(0,math.pi)) -- should be zero
print(f2(0,math.pi)) -- should be zero
print(f1(0,math.pi / 2)) -- should be 1
print(f2(0,math.pi / 2)) -- should be 1


-- ex2

function F(x)
    return {
        set = function (y) x = y end,
        get = function () return x end
    }
end


o1 = F(10)
o2 = F(20)
print(o1.get(),o2.get(),o1.get())

o2.set(300)
o1.set(100)
print(o1.get(),o2.get())




