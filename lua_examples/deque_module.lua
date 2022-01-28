local M =  {}
-- deque
function M.dequeNew()
    return {first = 0,last = -1,size=0}
end

function M.pushFirst(deque,value)
    local first = deque.first - 1
    deque.first = first
    deque[first] = value
    deque.size = deque.size + 1
end

function M.pushLast(deque,value)
    local last = deque.last + 1
    deque.last = last
    deque[last] = value
    deque.size = deque.size + 1
end

function M.popFirst(deque)
    local first = deque.first
    assert(first <= deque.last,"deque is empty")
    local value = deque[first]
    deque[first] = nil
    deque.first = first + 1
    deque.size = deque.size - 1
    return value
end

function M.popLast(deque)
    local last = deque.last
    assert(deque.first <= last,"deque is empty")
    local value = deque[last]
    deque[last] = nil
    deque.last= last - 1
    deque.size = deque.size - 1
    return value
end

return M
