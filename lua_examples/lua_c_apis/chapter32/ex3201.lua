print("hello 3201")


local dir = require("DirLib");
local d = dir.open("..\\*.*")
print(d)
for v in d do
    print(v)
end

print("dir handle should be closed")
d = nil
-- collectgarbage()