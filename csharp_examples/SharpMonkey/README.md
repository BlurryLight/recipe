An interpreter of Monkey Language.
Learning interpreter from <https://interpreterbook.com/>.

# 额外补充的特性
Monkey是一个偏向函数式的教学语言，用递归代替了循环，支持高阶函数、闭包、不可变内建数据结构和函数是第一类值等特性。
根据个人习惯额外补充了一些特性。

- 逻辑与/或 && ||
- while循环(不包含break)
- 函数/while内层Block定义的变量不会覆盖外层的值
- 三目条件表达式 `condition ? {then} : {else}`
- 前后自增符 `++i`,`i++`
- 浮点数Double及其运算符支持以及向Bool的转换
- 可变数据结构。`let a = [1,2,3]; a[0] = 100; puts(a) // should be [100,2,3]`

#  snippets

- 函数是第一类值
```
let sum = fn(x){
    return fn(y){ x + y };
};

let sum_five = sum(5);
puts(sum_five(5));  // 10
puts(sum_five(6)); // 11
puts(sum_five(7)); // 12
```

- 函数是第一类值
```
let sum = fn(x){
    return fn(y){ x + y };
};

let sum_five = sum(5);
puts(sum_five(5));  // 10
puts(sum_five(6)); // 11
puts(sum_five(7)); // 12
```


- fibonacci

循环版
```
let fib = fn(n)
{
    if(n < 2) {return n;}
    let i = 1; let p = 0; let q = 1;
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
while(i < 10)
{
    puts(fib(i++)); // 0 1 1 2 3 5 8 13 21
}
```