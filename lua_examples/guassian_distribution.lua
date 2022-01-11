function gaussian(mean,stdvar)

    -- box muller
    -- https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform
    u = math.random()
    v = math.random()
    z = math.sqrt( -2 * math.log(u)) * math.cos(2 * math.pi * v)
    return mean + z * stdvar
    
end

function mean(t)
    local sum = 0
    for k,v in pairs(t) do
        sum = sum + v
    end
    return sum / #t
end

function std(t)
    local squares,avg = 0,mean(t)
    for k,v in pairs(t) do
        squares = squares + ((avg - v) * (avg - v))
    end
    local variance = squares / #t
    return math.sqrt(variance)
end

function showHistogram(t)
    table.sort(t)
    local lo = math.ceil(t[1])
    local hi = math.floor(t[#t])

    local hist,barScale = {},200
    for i = lo,hi do
        hist[i] = 0
        for k,v in pairs(t) do
            if(math.ceil(v - 0.5) == i) then
                hist[i] = hist[i] + 1
            end
        end
        io.write(i.. "\t" .. string.rep('=',math.ceil(hist[i] / #t * barScale)))
        print("" .. hist[i])
    end
end

math.randomseed(os.time())
local t, average, stdvariance = {}, 15, 3
for i = 1, 1000 do
    table.insert(t, gaussian(average, stdvariance))
end 
print("Mean:", mean(t) .. ", expected " .. average)
print("StdDev:", std(t)  .. ", expected " .. stdvariance .. "\n")
showHistogram(t)