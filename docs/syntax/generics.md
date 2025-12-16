# Generics

## Functions

```sync
// T must be known at compile time
fn add(T: Type, num1: T, num2: T) T {
    // generic implementation
}

specific fn add(i8, num1: i8, num2: i8) i8 {
    // specialized for signed 8 bit integers
    // must have similar substituted signature (number of args and return type)
}


add(u64, 1, 2) // 3
add(1 as i32, 2 as i32) // automatically infer types, 3
const n1: i8 = 1;
const n2: i8 = 2;
add(n1, n2) // the types match the specialized version of `add()`, so call that one
```

## Structs

```sync
struct Example(T: Type) {
    obj: T
}

specific struct Example(i32) {
    obj: i32
}

const example1 = Example{ .obj = 23 as u64 } // use generic
const example2 = Example{ .obj = 23 as i32 } // use specialized
```
