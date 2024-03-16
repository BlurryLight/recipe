var console = {};
console.log = print;
console.log('fib test');

function fibonacci(n) {
   return n < 1 ? 0
        : n <= 2 ? 1
        : fibonacci(n - 1) + fibonacci(n - 2)
}

function fibonacci_loop(n) {
    if(n <= 1)
    {
        return n;
    }
    
    let p = 0;
    let q = 1;
    for(let i = 2; i <= n; i++)
    {
        let t = p + q;
        p = q;
        q = t;
    }
    return q;
}

// --jitless failed 可能是库里用的某个上古v8版本有问题，我用9.4的就没问题
// --no-opt works
{
  let k = 1e+6;
  let started = new Date();
  while (k--) {
    let result = fibonacci_loop(9);
    if(result !== 34)
    {
        print("error",result);
        break;
    }
  }
  let finished = new Date();
  console.log('1m fb took', finished - started + 'ms');
}
console.log('1m fb end')
