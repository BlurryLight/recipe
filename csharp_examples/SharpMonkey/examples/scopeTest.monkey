let a = 1;

let sum = fn(x){
    return fn(y){ x + y };
};

let sum_five = sum(5);
puts(sum_five(5));
puts(sum_five(6));
puts(sum_five(7));



let c = 10;
puts("outer c before:", fn(){c;}());
puts("func c", fn(){let c = 20;c;}());
puts("old c end:",c)

