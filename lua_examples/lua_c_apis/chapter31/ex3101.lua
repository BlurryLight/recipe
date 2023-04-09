print("hello 3101")

local BitArray = require("BitArrayLib");

t = BitArray.new(1000);

-- lua range is [0,999]
for i = 0, 999 do
    BitArray.set(t, i, i % 2 == 0);
end
print("BitArray Size", BitArray.size(t)) 

print(BitArray.get(t, 0)) -- should be 1
print(BitArray.get(t, 1)) -- should be 0
print(BitArray.get(t, 999)) -- should be 0
print(BitArray.get(t, 998)) -- should be 1
print(BitArray.get(t, 1000)) -- should throw

