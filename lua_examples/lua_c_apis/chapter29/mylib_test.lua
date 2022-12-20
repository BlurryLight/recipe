local mylib = require "mylib"
for _,v in ipairs(mylib.mydir(".")) do
    print(v)
end
print("hello")