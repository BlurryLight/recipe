function fact(n)
    if n < 0 then
        print("invalid input n")
        return -1
    end
    if n == 0 then
        return 1
    else
        return n * fact(n - 1)
    end
end

print("enter a number")
a = io.read("*n") --"*n": reads a number; this is the only format that returns a number instead of a string. 
print(fact(a))

print(type(nil) == nil)
print(arg[0])
