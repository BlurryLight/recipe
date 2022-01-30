-- oop style described in Programming in Lua 4th
-- basic operation around self.
-- colon operator(:) is just a syntax sugar which provides caller as the first arg as *self*
Account = {
    balance = 0,
    deposit = function (self,v)
        self.balance = self.balance + v
    end
}
function Account:withdraw(v)
    self.balance = self.balance - v
end

function Account:__tostring()
    return "Account Balance is "..self.balance
end

function Account:tostring()
    return self:__tostring()
end


-- Account:deposit(100)
-- print(Account.balance) -- 100
-- Account.withdraw(Account,40)
-- print(Account.balance) -- 60

-- naive way
-- local mt = {__index = Account,__tostring = Account.tostring}
-- function Account.new(o)
--     o = o or {}
--     setmetatable(o,mt)
--     return o
-- end

function Account:new(o)
    o = o or {}
    self.__index = self
    setmetatable(o,self) 
    -- 巧妙利用了：的第一个参数是self，也就是Account元表的机制，这样把传入参数o表的元表设做Account
    return o
end

a = Account:new{balance = 100}
print(a)
a:withdraw(119)
print(a)

b = Account:new()
print(b)
-- self.balance = self.balance + v
-- 后面的self.balance查到的是Account的balance的值
-- 通过赋值以后，b原表拥有了一个新的balance的值
-- 以后调用b.balance就不会再去查元表的值了
b:deposit(100)
print(b)
print(Account.balance)


-- inheritance

SpecialAccount = Account:new()
s = SpecialAccount:new{limit = 1000.00}
-- s的元表是SpecialAccount,SpecialAccount的元表是Account
-- 对s未定义的方法的访问会通过两次查找转交给Account，构成了继承树
-- override function
function SpecialAccount:withdraw(v)
    if v - self.balance >= self:getLimit() then
        error "insufficient funds!"
    end
    self.balance = self.balance - v
end

function SpecialAccount:getLimit()
    return self.limit or 0
end


s:withdraw(400)
print(s:tostring())



local function search(k,plist)
    for i = 1,#plist do 
        local v = plist[i][k]
        if v then return v end
    end
end

function createClass(...)
    local c = {}
    local parents = {...}

    setmetatable(c,{__index = function (t,k) 
        local v =  search(k,parents)
        -- it will caches the function from the parents to avoid repeatedly search in the parent tables
        -- however if the parent tables is modified in run-time
        -- the child table will NOT be notified
        t[k] = v
        return v
     end })
    c.__index = c
    function c:new(o)
        o = o or {}
        setmetatable(o,c)
        return o
    end

    return c
end

Named = {name = "default"}
function Named:getname()
    return self.name
end

function Named:setname(n)
    self.name = n
end


NamedAccount = createClass(Account,Named)

account = NamedAccount:new{name = "Paul"}
print(account:tostring())
print(account.name)


-- privacy
-- 利用闭包捕获变量来实现私有性，闭包能延长变量的生命周期，但是被捕获的变量无法被直接访问

function NewAccount(initialBalance)
    local acc = Account:new({balance=initialBalance})
    local withdraw = function (v)
        acc:withdraw(v)
    end
    local deposit = function (v)
        acc:deposit(v)
    end
    local getBalance = function()
        return acc.balance
    end
    local tostring = function()
        return acc:tostring()
    end
    return {
        withdraw = withdraw,
        tostring = tostring,
        deposit = deposit,
        getBalance = getBalance
    }
end


-- there is no way to access the inner acc
privateac = NewAccount(1000)
print(privateac.tostring())
privateac.deposit(100)
print(privateac.tostring())
privateac.withdraw(200)
print(privateac.tostring())