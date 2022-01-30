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