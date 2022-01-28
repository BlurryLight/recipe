local deque = require "deque_module"

dq = deque.dequeNew()
deque.pushFirst(dq,1)
print(deque.popFirst(dq))