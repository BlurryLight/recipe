let a = 1;

let sum = fn(x){
    return fn(y){ x + y };
};

let sum_five = sum(5);
puts(sum_five(5));
puts(sum_five(6));
puts(sum_five(7));


let i = 0;
let b = 10;
while(i < 3)
{
    let b = 20;
    puts("inner b:",b + i++)
}
puts("old b:",b)


let c = 10;
puts("outer c before:", fn(){c;}());
puts("func c", fn(){let c = 20;c;}());
puts("old c end:",c)

