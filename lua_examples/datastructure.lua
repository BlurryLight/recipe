-- deque
function dequeNew()
    return {first = 0,last = -1,size=0}
end

function pushFirst(deque,value)
    local first = deque.first - 1
    deque.first = first
    deque[first] = value
    deque.size = deque.size + 1
end

function pushLast(deque,value)
    local last = deque.last + 1
    deque.last = last
    deque[last] = value
    deque.size = deque.size + 1
end

function popFirst(deque)
    local first = deque.first
    assert(first <= deque.last,"deque is empty")
    local value = deque[first]
    deque[first] = nil
    deque.first = first + 1
    deque.size = deque.size - 1
    return value
end

function popLast(deque)
    local last = deque.last
    assert(deque.first <= last,"deque is empty")
    local value = deque[last]
    deque[last] = nil
    deque.last= last - 1
    deque.size = deque.size - 1
    return value
end

dq = dequeNew()
pushLast(dq,1)
pushLast(dq,2)
pushLast(dq,3) -- should be 1 2 3
print(popFirst(dq)) -- shoule be 1,     [2,3]
pushFirst(dq,10) -- 10 2 3
print(popLast(dq)) -- shoule be 3,     [10,2]
print(dq.size) -- 2