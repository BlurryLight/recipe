local Stack = {}
Stack.data_ = {}
Stack.size_ = 0

function Stack:push(v)
    table.insert(self.data_,v)
    self.size_ = self.size_ + 1
end

function Stack:pop()
    assert(self.size_ >= 1,"stack is empty")
    local v = table.remove(self.data_)
    self.size_ = self.size_ - 1
    return v
end

function Stack:size()
    return self.size_
end

function Stack:top()
    return self.data_[#self.data_]
end

function Stack:isEmpty()
    return self:size() == 0
end

function Stack:New(o)
    o = o or {}
    self.__index = self
    setmetatable(o,self)
    return o
end

return Stack