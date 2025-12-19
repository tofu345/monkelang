# [Writing an Compiler ~~in Go~~](https://compilerbook.com/) in C.

A port of the book in C. Takes a lot of inspiration from
[wren](https://github.com/wren-lang/wren).

## Syntax

```javascript
// Copied from: https://interpreterbook.com/#the-monkey-programming-language

// Bind values to names with let-statements
let version = 1;
let name = "Monkey programming language";
let myArray = [1, 2, 3, 4, 5];
let coolBooleanLiteral = true;

// Use expressions to produce values
let awesomeValue = (10 / 2) * 5 + 30;
let arrayWithValues = [1 + 1, 2 * 2, 3];

// Define a `fibonacci` function
let fibonacci = fn(x) {
  if (x == 0) {
    0                // Monkey supports implicit returning of values
  } else {
    if (x == 1) {
      return 1;      // ... and explicit return statements
    } else {
      fibonacci(x - 1) + fibonacci(x - 2); // Recursion! Yay!
    }
  }
};

// Here is an array containing two hashes, that use strings as keys and integers
// and strings as values
let people = [{"name": "Anna", "age": 24}, {"name": "Bob", "age": 99}];

// Getting elements out of the data types is also supported.
// Here is how we can access array elements by using index expressions:
fibonacci(myArray[4]);
// => 5

// We can also access hash elements with index expressions:
let getName = fn(person) { person["name"]; };

// And here we access array elements and call a function with the element as
// argument:
getName(people[0]); // => "Anna"
getName(people[1]); // => "Bob"

// Define the higher-order function `map`, that calls the given function `f`
// on each element in `arr` and returns an array of the produced values.
let map = fn(arr, f) {
  let iter = fn(arr, accumulated) {
    if (len(arr) == 0) {
      accumulated
    } else {
      iter(rest(arr), push(accumulated, f(first(arr))));
    }
  };

  iter(arr, []);
};

// Now let's take the `people` array and the `getName` function from above and
// use them with `map`.
map(people, getName); // => ["Anna", "Bob"]

// newGreeter returns a new function, that greets a `name` with the given
// `greeting`.
let newGreeter = fn(greeting) {
  // `puts` is a built-in function we add to the interpreter
  return fn(name) { puts(greeting + " " + name); }
};

// `hello` is a greeter function that says "Hello"
let hello = newGreeter("Hello");

// Calling it outputs the greeting:
hello("dear, future Reader!"); // => Hello dear, future Reader!
```

### Differences

```javascript
let fibonacci = fn(num) {
  let seen = {0: 0, 1: 1};
  let fib = fn(num) {
    let res = seen[num];
    if (!res) {
      res = fib(num - 1) + fib(num - 2);
      // Assignment to Table Keys.
      seen[num] = res;
    }
    return res;
  };
  fib(num);
};
puts("fibonacci(50):", fibonacci(50)); // => fibonacci(50): 12586269025

// Assignment at array indices.
let array = [1, 2, 3];
array[0] = 5;
puts("array is", array); // => array is [5, 2, 3]

// Assignment to global variables inside functions.
let global = 1;
fn() {
  global = 2;
}()
puts("global is", global) // => global is 2

// Assignment to free variables.
fn() {
  let empty;
  let num = 1;
  let array = [];
  fn() {
    // A free variable is a non-global variable not defined in the current
    // function.  All functions create shallow copies of their free variables
    // in a free list.

    // no effect outside this function, only changes the value in the free
    // list.
    num = 2;

    // has an effect, because the free list contains a shallow copy.
    push(array, 1);

    // no effect
    array = {};
    empty = 3;
  }()
  puts("empty is", empty); // empty is nothing
  puts("num is", num); // num is 1
  puts("array is", array); // array is [1]
}()

// Empty return statements
let sayIf = fn(message, condition) {
  // !object is true only if object is nothing or false
  if (!condition) { return; }
  puts(message);
}
sayIf("hi!", true);     // => "hi"
sayIf("hi!", nothing);  // =>

// Double `a`
let a = [1, 2, 3, 4];
let double = fn(x) { x * 2 };
let map = fn(f, arr) {
  let length = len(arr);
  let result = [nothing] * length;           // initialize array of given length
  for (let i = 0; i < length; i += 1) { // operator assignment
    result[i] = f(arr[i]);
  }
  return result
};
puts("a:", a, "doubled:", map(double, a));
```

## Tree Walking Interpreter

Available on a [separate branch](https://github.com/tofu345/monkelang/tree/evaluator).
