var console = {};
console.log = print;
// Function that contains the pattern to be inspected (using an `with` statement)
function fn() {
    with ({ i: 0, v: 0 }) {
        return v;
    }
}

// https://github.com/v8/v8/blob/5fe0aa3bc79c0a9d3ad546b79211f07105f09585/src/runtime/runtime.h#L465 所有的intrisinc可以在这里看到

// js里的用法可以在这里看到
// https://github.com/NathanaelA/v8-Natives/blob/master/lib/v8-native-calls.js

// https://github.com/v8/v8/blob/5fe0aa3bc79c0a9d3ad546b79211f07105f09585/src/runtime/runtime.h#L868
// 对应解码
function printStatus(fn) {
    const status = %GetOptimizationStatus(fn);
    console.log(status.toString(2).padStart(12, '0'));

    if (status & (1 << 0)) {
        console.log("is function");
    }

    if (status & (1 << 1)) {
        console.log("is never optimized");
    }
    
    if (status & (1 << 2)) {
        console.log("is always optimized");
    }
    
    if (status & (1 << 3)) {
        console.log("is maybe deoptimized");
    }
    
    if (status & (1 << 4)) {
        console.log("is optimized");
    }
    
    if (status & (1 << 5)) {
        console.log("is optimized by TurboFan");
    }
    
    if (status & (1 << 6)) {
        console.log("is interpreted");
    }
    
    if (status & (1 << 7)) {
        console.log("is marked for optimization");
    }
    
    if (status & (1 << 8)) {
        console.log("is marked for concurrent optimization");
    }
    
    if (status & (1 << 9)) {
        console.log("is optimizing concurrently");
    }
    
    if (status & (1 << 10)) {
        console.log("is executing");
    }
    
    if (status & (1 << 11)) {
        console.log("topmost frame is turbo fanned");
    }
}

console.log('0. prepare function for optimization');
printStatus(fn);

console.log("1. fill type-info");
fn();

console.log('2. calls function to go from uninitialized -> pre-monomorphic -> monomorphic')
fn();
printStatus(fn);

%OptimizeFunctionOnNextCall(fn);

console.log('3. optimize function on next call')
printStatus(fn);

console.log('4. next call')
fn();

console.log('5. get optimization status')
printStatus(fn);

console.log('6. de optimiza function');
%DeoptimizeFunction(fn);
printStatus(fn);



console.log('fib test');

function fibonacci(n) {
   return n < 1 ? 0
        : n <= 2 ? 1
        : fibonacci(n - 1) + fibonacci(n - 2)
}

%OptimizeFunctionOnNextCall(fibonacci);
fibonacci(0); // trigger jit

{
  let k = 1e+6;
  let started = new Date();
  while (k--) {
    let result = fibonacci(9);
    if(result !== 34)
    {
      %Abort(-1);
    }
  }
  let finished = new Date();
  console.log('1m fb jit took', finished - started + 'ms');
}

%DeoptimizeFunction(fibonacci);
%NeverOptimizeFunction(fibonacci);

{
  let k = 1e+6;
  let started = new Date();
  while (k--) {
    let result = fibonacci(9);
    if(result !== 34)
    {
      %Abort(-1);
    }
  }
  let finished = new Date();
  console.log('1m fib jitless took', finished - started + 'ms');
}
