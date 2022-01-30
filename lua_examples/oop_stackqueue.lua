local Stack = require('oop_stack')
local StackQueue = Stack:New()

function StackQueue:insertbottom(v)
    table.insert(self.data_,1,v)
    self.size_ = self.size_ + 1
end

return StackQueue