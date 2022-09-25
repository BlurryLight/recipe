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

let i = 0;
while(i < 20)
{
    puts(fib(i++))
}
