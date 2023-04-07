local mylib = require "mylib"
for _, v in ipairs(mylib.mydir(".")) do
    print(v)
end
print("hello")

-- ex29.1
print(mylib.summation()) -- 0
print(mylib.summation(2.3, 5.4)) -- 7.7
print(mylib.summation(2.3, 5.4, -34)) 
print(mylib.summation(2.3, 5.4, {})) -- will throw an error

