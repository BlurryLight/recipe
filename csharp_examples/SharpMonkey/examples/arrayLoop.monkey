let double = fn (x) { return x * 2;};
let array = [1,2,3];
let str = "hello world";

let index = 0;
let b = [];
while(index < len(array))
{
    b = push(b,double(array[index]));
    index++;
}
puts(b);
puts(str);
