N = 8
Count = 0
Flag = false
function isValid(array,n,c)
    for i = 1, n - 1 do
        if(array[i] == c) or 
            (array[i] - i == c - n) or 
            (array[i] + i == c + n) then
        return false
        end
    end
    return true
end

function printSolution (array)
    for i = 1, N do --row
        for j = 1, N do -- col
            io.write(array[i] == j and "X" or "-"," ")
        end
        io.write('\n')
    end
    io.write('\n')
end

function addQueue(array,n)
    if n > N then
        printSolution(array)
        Flag = true
        Count = Count + 1
    elseif(Flag == false) then
        for c = 1,N do
            if(isValid(array,n,c)) then
                array[n] = c
                addQueue(array,n + 1)
            end
        end
    end
end
addQueue({},1)
print(Count)
