const LOOP_COUNT = 100_0000;
class Point {
    x;
    y;
    z;
    __tid__Point;
    constructor(_x, _y, _z) {
        this.x = _x;
        this.y = _y;
        this.z = _z;
    }
}
;
var v_es6;
let beginTime = new Date();
for (var i = 0; i < LOOP_COUNT; i++) {
    v_es6 = new Point(1, 2, 3);
}
let endTime = new Date();
console.log("1m CreateObj es6:  " + (endTime.getTime() - beginTime.getTime()) + "ms");


var Point_Es5 = /** @class */ (function () {
    function Point(_x, _y, _z) {
        this.x = _x;
        this.y = _y;
        this.z = _z;
        this.__tid__Point = undefined;
    }
    return Point;
}());
;

var v_es5;
beginTime = new Date();
for (var i = 0; i < LOOP_COUNT; i++) {
    v_es5 = new Point_Es5(1, 2, 3);
}
endTime = new Date();
console.log("1m CreateObj es5:  " + (endTime.getTime() - beginTime.getTime()) + "ms");

if(
    v_es6.x !== v_es5.x ||
    v_es6.y !== v_es5.y ||
    v_es6.z !== v_es5.z ) {
    throw new Error();
}
console.log("v_es5", v_es5)
console.log("v_es6", v_es6)