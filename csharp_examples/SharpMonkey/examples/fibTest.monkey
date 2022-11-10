let fib = fn(n)
{
    if(n < 2) {return n;}
    let i = 1;
    let p = 0;
    let q = 1;
    let tmp = 0;
    while(i++ < n)
    {
        tmp = q;
        q = p + q;
        p = tmp;
    }
    return q;
};

puts(fib(1)); // 1
puts(fib(2)); // 1
puts(fib(3)); // 2
puts(fib(4)); // 3
puts(fib(5)); // 5
