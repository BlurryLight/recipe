let globalSeed = 50;
let minusOne = fn(){
    let num = 1;
    return globalSeed - num;
};

let minusTwo = fn(){
    let num = 2;
    // implicit return
    globalSeed - num;
};

puts(minusOne() + minusTwo()) // 97
