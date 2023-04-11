print("hello 3101")

local BitArray = require("BitArrayLib");

t = BitArray.new(1000);

for i = 1, 1000 do
    t:set(i, i % 2 == 0);
end
print("BitArray Size", t:size()) 

print(t:get(1)) -- should be 0
print(t:get(999)) -- should be 0
print("should be false", t.get(t, 999)) -- should be 0
t[999] = 1
print("should be true", t:get(999)) -- should be 1

print(t:get(998)) -- should be 1
print(t:get(1000)) -- should be 1
print("t: ", t)

-- print(BitArray.get(t, 0)) -- should throw
-- BitArray.set(io.stdin, 1, 0) -- will throw. io.stdin is an userdata,but not an BitArray
