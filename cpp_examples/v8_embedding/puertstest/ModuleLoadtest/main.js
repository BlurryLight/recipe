// Static import
import {repeat, shout} from './lib.mjs';
let r = repeat('hello');
let s = shout('Modules in action');
print(r);
print(s);

// Dynamic import
import('./lib.mjs')
 .then((module) => {
  let a = module.repeat('hello');
  let b = module.shout ('Modules in action');
  print(a);
  print(b);
 });

