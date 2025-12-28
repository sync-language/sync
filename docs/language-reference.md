# Sync Language Reference

Sync is an embeddable, memory-safe, and thread-safe programming language designed to enable multithreaded scripting within any application. Sync uses the C struct memory layout, meaning you do not need to write translation layers for data that Sync uses. Your data types can work as is.

This allows custom logic, and dynamic modification to application functionality without having to compile anything ahead of time.

Native support for C, C++, Rust, and Zig is provided through CMake, Cargo, and Zig modules. You can compile the source files directly as is with a C11 and C++17 compiler without any extra flags required. See `build.md` for more info.

## Comments

Sync supports 2 types of comments.

### Normal Comments `//`

A normal comment is for the programmer, and is not considered part of full documentation.

```sync
fn add(num1: i32, num2: i32) i32 {
    // comments start with `//` and end at the end of line

    // the following line won't get executed
    // print(num1);
    
    // however the following line will, as it is uncommented
    print(num2);
    return num1 + num2;
}
```

### Doc Comments `///`

Documentation comments are above functions, structs, members, traits and global variables, describing their purpose. Multiple consecutive lines of `///` are combined together.

```sync
/// Contains relevant data to a person
struct Person {
    /// name of this person as `first last`
    name: str,
    /// person's age. Typically will be within life expectancy, but maybe someone will be immortal
    age: u16
}
```

## Primitives

### Primitive Types

| Type | C/C++ Equivalent | Description |
|------|--------------|-------------|
| bool | bool | true or false |
| i8 | int8_t | signed 8-bit integer |
| u8 | uint8_t | unsigned 8-bit integer |
| i16 | int16_t | signed 16-bit integer |
| u16 | uint16_t | unsigned 16-bit integer |
| i32 | int32_t | signed 32-bit integer |
| u32 | uint32_t | unsigned 32-bit integer |
| i64 | int64_t | signed 64-bit integer |
| u64 | uint64_t | unsigned 64-bit integer |
| usize | size_t | can store the maximum size of any possible object (include array) |
| f32 | float | 32-bit floating point |
| f64 | double | 64-bit floating point |
| char | SyChar / sy::Char | 32-bit unicode scalar value |
| Type | SyType / sy::Type | compile time type information |
| str | SyStringSlice / sy::StringSlice | pointer to the start of a utf8 string with a length |
| String | SyString / sy::String | memory managed utf8 string container |
| List(T) | sy::List<T> | memory managed dynamically resizeable contiguous array |
| Set(K) | sy::Set<K> | memory mananaged hash set |
| Map(K, V) | sy::Map<K, V> | memory mananaged hash map |
| Vec(N, T) | sy::Vec<N, T> | math vector N long containing T type |
| Mat(N, M, T) | sy::Mat<N, M, T> | math matrix of dimensions N*M of T type |
| Unique(T) | SyUnique / sy::Unique<T> | thread-safe single owned object |
| Shared(T) | SyShared / sy::Shared<T> | thread-safe multiple owned object |
| Weak(T) | SyWeak / sy::Weak<T> | thread-safe weak reference to Unique or Shared |

### Primitive Values

| Name | Description |
|------|-------------|
| true | boolean true value |
| false | boolean false value |
| null | the none value of any optional type |

### Literals

#### Number Literals

Number literals for now are just digits, optionally with a decimal to make a floating point number.

Currently hex, binary, and scientific notation is not supported, nor are numeric separators. Octal notation will not be supported.

Number literals can be cast to the appropriate types, which are validated at compile time.

```sync
fn main() {
    // Ok! 100 is small enough to fit in an unsigned 8-bit integer
    const num: u8 = 100 as u8;

    // COMPILER ERROR! 257 is too big to fit in an unsigned 8-bit integer
    const tooBig: u8 = 257 as u8; 
}
```

#### Escape Sequences

- \n
- \t
- \r
- \0
- \\\
- \\'
- \\"

#### Char Literals

A char literal is a single `char` within the `''` characters. `'a'`. Char literals may use [Escape Sequences](#escape-sequences).

#### String Literals

String literals are always of type `str` or `*String`. They are enclosed in the `""` characters, with 0 to many characters between. They must be valid utf8. String literals may use [Escape Sequences](#escape-sequences).

#### Format Strings

Format strings are extremely similar to Python [F-Strings](https://www.w3schools.com/python/python_string_formatting.asp), but leveraging the Sync trait system. See [Format Trait](#format). Format strings may use [Escape Sequences](#escape-sequences).

#### Array Literals

Array literals are one or more values within `[]` separated by commas.

#### Struct Literals

Struct literals follow the format of `StructName{.member1 = value1, .member2 = value2, ...}`.

## Keyword Reference

| Keyword | Description |
|---------|-------------|
| and | Boolean and |
| as | Infallible cast between types, and conditionally extracting good value from optional and error unions |
| assert | Assert that a boolean condition is true, aborting the Sync program if false |
| break | Exit while or for loops |
| catch | Get the error value if the `try` error union has an error |
| comptime | Variable value must be known at compile time |
| const | Immutable variable |
| continue | Jump back to beginning of loop |
| dyn | Dynamic dispatch on trait |
| else | Alternate branch for if, switch, while, for |
| enum | Enumeration or tagged union |
| extern | A function linked to a Sync module, defined in the host program |
| fn | Function declaration |
| for | Loop on an iterator |
| if | Conditionally execute based on a boolean expression, optional value, or error union |
| impl | A block implementing methods on a type |
| import | Imports a module, or sync source file relative to the current one |
| mut | Mutable variable |
| or | Boolean or |
| panic | Abort the Sync program (does not abort the host process) |
| pub | Allow access of the symbol outside of the file it is declared in |
| return | Return a non-error value from a function to the caller |
| Self | The primary type in an `impl` block |
| specific | Provide an explicit generic specialization |
| struct | Defines a structure in the C structure layout |
| switch | Match on values or enum variants |
| sync | Lock one or more thread-safe objects shared across thread boundaries |
| test | Defines an executable test case |
| throw | Return an error value from a function to the caller |
| trait | Define shared behaviour of types |
| try | Attempt to get the non-error value from an error union |
| where | Compile time constraint on trait or generic type usage |
| while | Loop on boolean expression, optional, or error union |

## Variables

All declared variables can be either immutable with `const`, or mutable with `mut`. All function parameters are implicitly immutable, unless marked `mut`.

```sync
// num1 is immutable, and num2 is mutable
fn printNumbers(num1: i32, mut num2: i32) {
    print(num1);
    print(num2);
    num1 += 1; // COMPILE ERROR! Cannot mutate immutable variable
    num2 += 1; // allowed.
}

fn main() {
    mut firstName: str = "timothy";
    const lastName: str = "jones";

    // reassigning to mutable variable is fine. underlying compile time string doesn't get modified
    firstName = "jimothy";

    // COMPILER ERROR! lastName is marked const
    lastName = "tones";
}
```

### Mut

See [Pointer](#pointer). A variable marked with mut means the variable can be reassigned. A reference marked with mut means the data it is referencing is mutable.

```sync
// val is a variable, in which it's type is a pointer to a mutable signed 32-bit integer.
// val can be reassigned because it itself is marked `mut`.
mut val: *mut i32;
```

### Global Variables

Any immutable `const` global variable will be determined at compile time, and requires no thread safety mechanisms as it is read only. Any mutable global variable requires to be marked with either `Unique` or `Shared`. See [Thread Safety Types](#thread-safe-types). Both types of global variables require some form of compile time initializer.

```sync
struct Person {
    name: str
}

const person1 = Person{.name = "hannah"};
mut person2: Unique(Person) = Person{.name = "jerome"}

fn main() {
    // COMPILER ERROR! person1 is const
    person1.name = "nahnah";

    // COMPILER ERROR! person2 isn't in a sync block
    person2.name = "jeremy";

    sync mut person2 {
        person2.name = "jeremy";
    } catch panic;

    sync person2 {
        print(person2.name); // jeremy
    }
}
```

## Control Flow

### If

If statements work on boolean conditions, optional values, and error unions.

```sync
extern fn getRandomNumber() u8;

fn main() {
    if getRandomNumber() > 200 {
        print("large number!");
    }
}
```

```sync
extern fn getOptionalNumber() ?u8;

fn main() {
    // check an optional value, which evalutes to true if the value is non-null.
    // if non-null, extract to the binding `num`.
    if getOptionalNumber() as num {
        print(num);
    }
}
```

```sync
extern fn getNumber() GetNumError!u8;

fn main() {
    // check an error union, which evalutes to true if the value is not an error.
    // if not an error, extract to the binding `num`.
    if getNumber() as num {
        print(num);
    }
}
```

### Else

```sync
extern fn getRandomNumber() u8;

fn main() {
    if getRandomNumber() > 200 {
        print("large number!");
    }
    else {
        print("small number...");
    }
}
```

```sync
extern fn getOptionalNumber() ?u8;

fn main() {
    if getOptionalNumber() as num {
        print(num);
    }
    else {
        print("number was null");
    }
}
```

```sync
extern fn getNumber() GetNumError!u8;

fn main() {
    if getNumber() as num {
        print(num);
    }
    // the else will contain the error value
    else as err {
        print(err);
    }
}
```

This also works with multiple conditions as you'd expect.

```sync
extern fn getRandomNumber() u8;

fn main() {
    const randNum = randomNumber();
    if randNum == 1 {
        print("one");
    }
    else if randNum == 2 {
        print("two");
    }
    else {
        print("not one or two");
    }
}
```

**NOTE** in the above example, the function is not called more than once. Each if condition will evaluate the expression, calling the function. As the example function fetches a random value, this would fetch a random value again.

### While

```sync
extern fn getRandomNumber() u8;

fn main() {
    bool randomFlag = false;
    // loop on condition until condition evaluates to false
    while randomFlag == false {
        if getRandomNumber() > 200 {
            randomFlag = true;
        }
    }
}
```

```sync
extern fn getOptionalNumber() ?u8;

fn main() {
    // loop on an optional value, which evalutes to true if the value is non-null.
    // if non-null, extract to the binding `num`.
    while getOptionalNumber() as num {
        print(num);
    }
}
```

### For

TODO zip?

```sync
fn main() {
    // iterate over a range from 0 (inclusive) to 10 (exclusive)
    for i in 0..10 {
        print(i); // 0 to 9
    }    
    
    // iterate over a range from 0 (inclusive) to 10 (inclusive)
    for i in 0..=10 {
        print(i); // 0 to 10
    }

    const slice: []i32 = [0, 1, 2, 3, 4]; // compile time static array coerces to slice
    // iterate over a slice
    for item in slice {
        print(item); // 0 to 4
    }

    const names: []str = ["jim", "tim", "gim", "bum"];
    // get each element, and an index starting from 0
    for name in names, i {
        print(f"{i}: {name}") // "0: jim", "1: tim", etc
    }

    mut arr: [3]i32 = [1, 2, 3];
    // get mutable reference to each array element
    for elem in &mut arr {
        elem.* += 2;
    }
    print(arr); // 3, 4, 5
}
```

### Switch

#### Switching on Integers

```sync
fn main() {
    mut statusCode = 404;

    switch statusCode {
        200 { print("ok") }
        404 { print("not found") }
        500 { print("server error") }
        else { print("what??"); }
    } // not found

    statusCode = 654;

    switch statusCode {
        200 { print("ok") }
        404 { print("not found") }
        500 { print("server error") }
        else { print("what??"); }
    } // what??
}
```

You can switch on integer ranges as well.

```sync
fn main() {
    const num: i32 = 100;
    switch num {
        50..200 {
            print("in good range");
        }
        200..500 {
            print("in bad range");
        }
        else {
            print("outside of known range);
        }
    } // in good range
}
```

#### Switching on Strings

```sync
fn main() {
    mut s = "hello world!";

    switch s {
        "hello" { print("hmmm") }
        "hello world!" { print("wow hi!") }
        else { print("darn"); }
    } // wow hi!

    s = "world!";

    switch s {
        "hello" { print("hmmm") }
        "hello world!" { print("wow hi!") }
        else { print("darn"); }
    } // darn
}
```

#### Switching on Normal Enums

```sync
enum State {
    Walking,
    Running,
    Swimming,
    Jumping
}

fn main() {
    const state = State.Running;

    switch state {
        .Walking { ... }
        .Running { ... }
        .Swimming { ... }
        .Jumping { ... }
    }

    // You can also do else
    switch state {
        .Running { ... }
        else { ... }
    }

    print(state); // State.Running
}
```

#### Switching on Sum Types

See [Sum Types](#sum-types).

```sync
enum State {
    Walking
    Running(speedMultiplier: f32)
    Swimming(oxygenLeft: f32, onSurface: bool)
    Jumping(gravity: f32, force: f32)
}

fn main() {
    mut state = State.Running{.speedMultiplier = 5.5};

    switch state {
        .Walking { ... }
        // Make a binding called `speed` to the sum type member `speedMultipler`.
        .Running as speed { ... }
        // Make a mutable binding to `oxygenLeft`, and an immutable binding to `onSurface`.
        .Swimming as mut oxygen, surface { ... }
        .Jumping as gravity, force { ... }
    }

    switch &mut state {
        .Running as speed {
            speed.* += 2;
        }
        else { ... }
    }

    print(state); // State.Running(speedMultiplier = 7.5);
}
```

#### Fallthrough

Unlike C and C++, Sync does not do switch case fallthrough.

```sync
fn main() {
    mut statusCode = 404;

    switch statusCode {
        200, 404, 500 { print("valid code") }
        else { print("what??"); }
    } // valid code

    statusCode = 654;

    switch statusCode {
        200, 404, 500 { print("valid code") }
        else { print("what??"); }
    } // what??
}
```

### Break

Break is used to escape out of a while or for loop.

```sync
extern fn getRandomNumber() u8;

fn main() {
    while true {
        if getRandomNumber() == 127 {
            break;
        }
    }

    // continues with rest of function
}
```

```sync
fn main() {
    const slice = ["hi", "hello", "salutations", "greetings"];
    for greeting in slice {
        print(greeting); // "hi", "hello", "salutations"
        if greeting == "salutations" {
            break;
        }
    }

    // continues with rest of function
}
```

### Continue

Continue is used to step the loop in a while or for loop, and go back to the beginning of the loop body.

```sync
fn main() {
    mut num: i32 = 100;
    while num > 0 {
        num -= 1;
        if num < 95 {
            continue;
        }
    
        print(num); // 99, 98, 97, 96, 95
    }

    print(num); // 0

    // continues with rest of function
}
```

```sync
fn main() {
    const slice = ["hi", "hello", "salutations", "greetings", "salutations"];
    for greeting in slice {
        if greeting != "salutations" {
            continue;
        }

        print(greeting); // salutations, salutations
    }

    // continues with rest of function
}
```

## Operators

All math operators are checked for any failure conditions. Integer overflow is not permitted for signed or unsigned types. Divide or modulo by zero is also not allowed.

| Name | Syntax | Types | Description |
|------|--------|-------|-------------|
| Equality | a == b | Any that implements Compare | Compares two values for equality (value or pointer equality) |
| Is Null | a == null | Any optional type `?T` | Checks if the optional value is null |
| Inequality | a != b | Any that implements Compare | Compares two values for inequality (value or pointer equality) |
| Is Not Null | a != null | Any optional type `?T` | Checks if the optional value is not null |
| Ordering | a < b<br>a <= b<br>a > b<br>a >= b | Any that implements PartialOrd TODO evaluate this | Compares two values for ordering (value or pointer ordering) |
| Logical and | a and b | Any boolean expressions | If a and b is `true`, evaluates to `true`, otherwise `false` |
| Logical or | a or b | Any boolean expressions | If a or b is `true`, evaluates to `true`. If both are `false`, evaluates to `false` |
| Addition | a + b<br>a += b | Integers, Floats | Overflow checked for integers |
| Subtraction | a - b<br>a -= b | Integers, Floats | Overflow checked for integers |
| Multiply | a * b<br>a *= b | Integers, Floats | Overflow checked for integers |
| Divide | a / b<br>a /= b | Integers, Floats | Truncation division. Overflow checked for integers (min signed int divided by -1 for instance) |
| Remainder Division / Modulo | a % b<br>a %= b | Integers, Floats | TODO determine behaviour for negative values |
| Bit Shift Left | a << b<br>a <<= b | Integers | Moves bits to the left. TODO determine failure conditions |
| Bit Shift Right | a >> b<br>a >>= b | Integers | Moves bits to the right. TODO determine failure conditions |
| Bitwise And | a & b<br>a &= b | Integers | Bit AND on two integers |
| Bitwise Or | a \| b<br>a \|= b | Integers | Bit OR on two integers |
| Bitwise Xor | a ^ b<br>a ^= b | Integers | Bit XOR on two integers |
| Bitwise Not | ~a | Integers | Bitwise completemtn on an integer |
| Boolean Not | !a | Boolean expression | Converts `true` to `false` and `false` to `true` |
| Optional Unwrap | a.? | Any optional type `?T` | Unwraps an optional value, or aborts if null |
| Error Unwrap | a.! | Any error union `ErrorT!OkT` | Unwraps an ok value from an error union, or aborts if error |
| Dereference | a.* | Any pointer type `*T` | Dereferences a pointer to the underlying value. Identical to C `*a` |
| Address of | &a<br>&mut a | Any type | Gets the address of a value |

### Precedence

Based on [C++ Operator Precedence](https://en.cppreference.com/w/cpp/language/operator_precedence.html)

| Precedence | Operator | Description | Associativity |
|------------|----------|-------------|---------------|

## Ownership

Sync provides explicit opt-in thread safety mechanisms. As such, many mutable and immutable references can be alive at the same time. This is because these references will never escape thread boundaries, and in order to get a reference to shared memory, the programmer is forced to use the thread safety mechanisms.

This is a lot less restrictive than Rust's borrow checker (although Rust's is for very good reason).

### Lifetime Relationships

- A reference cannot outlive the variable that it comes from
- A reference from another reference has the same lifetime constraints
- A reference to a field or index in an array/map/other has a lifetime constraint of the owning objects lifetime
- A function returning a reference uses whichever is the most restrictive lifetime of the possible options if there is branching

### Reference / Iterator Invalidation Conditions

1. Destructor
2. Move
3. Fundamental structural change

All of these impact field / index access, as getting a reference to an objects field is a chain.

#### Destructor

Naturally, if an object's destructor is called, that object is no longer considered valid, and thus references to it are invalid.

#### Move

If an object is moved in memory a reference to it is invalid. There are cases where the memory pointed to is still valid, as a result of being allocated elsewhere, however this should either be allowed with caution, or prohibited as well.

#### Fundamental Structural Change

Many functions will fundamentally change the memory associated with a given data structure. For instance, appending an element to a dynamic array may trigger a re-allocation to grow its capacity. The reference to the dynamic array data structure is sound, but a reference to one of the elements it owns will suddenly become invalid. These operations must be known by the compiler.

Unlike in Rust, dynamic arrays, hash maps, and other data structures are supplied as part of the core language implementation, not just the standard library implementation like in Rust. As such, more implicit information can be supplied to the compiler to have less restrictive rules.

If a function takes a mutable reference, and causes a structural change, this counts as an invalidating operation.

### Ownership in Control Flow

#### Branches

A reference must be valid in all branches, as all could be explored.

Invalidation that occurs within a branch is invalid for the rest of the branch, and if execution escapes that branch (no early return).

#### Loops

A reference must not become invalid inside a loop body if it could be used in subsequent iterations.

### Edge Cases

#### External Functions

When returning a reference from an external function (such as a C function), having a lifetime annotation is required, as there is no way for the Sync compiler itself to know the lifetime bounds of that reference. For familiarity, using Rust style `*'identifier` may be the best decision.

Passing any mutable reference to an external function will automatically invalidate any existing reference, as there is no way of validating if any operations cause reference / iterator invalidation across boundaries.

### Relevant Reading

- [Oxide: The Essence of Rust](https://arxiv.org/pdf/1903.00982)
- [Rust Polonius Source](https://github.com/rust-lang/polonius)
- [Tree Borrows](https://perso.crans.org/vanille/treebor/)
- [C++ Container Iterator Invalidation](https://en.cppreference.com/w/cpp/container.html)

## Expression Blocks

Similar to Rust, a block of code encapsulated within `{}` can yield a value.

```sync
fn main() {
    const x: i32 = 50;
    const y = {
        val: i32 = x * 2
        if val < 0 {
            val *= -1;
        }
        val
    }
    print(y); // 100 
}
```

This also works for if else, and is how Sync does ternary.

```sync
fn main() {
    const x: i32 = -50;
    const y: i32 = if x < 0 { x * -1 } else { x };
    print(y); // 50
}
```

## Comptime

### Comptime Parameters

See [Generics](#generics).

### Comptime Blocks

A block of code marked comptime has is forcibly evaluated at compile time. If the block cannot be evaluated at compile time, this is a compile error.

```sync
fn main() {
    const indexWithDesiredValue: ?usize = comptime {
        const arr: [5]i32 = [1, 10, 100, 1000, 10000];
        for item in arr, i {
            if item == 100 {
                i
            }
        }

        null
    }

    print(indexWithDesiredValue); // 3
}
```

### Comptime If

The `comptime` keyword before an `if` / `else` chain marks all branches to be compile time evaluated. If one branch condition evaluates to `true`, or none do and there is an `else`, only that branch will be compiled.

```sync
extern const CONFIG_VALUE: i32;

fn main() {
    comptime if CONFIG_VALUE == 1000 {
        print("hello!"); // hello!
    } else {
        panic; // this will never execute and will be discarded from compilation
    }
}
```

## Pointer

Pointers are non-null in Sync, and are immutable by default, requiring the `mut` keyword to make the memory they point to mutable.

| Sync Pointer | C Pointer |
|--------------|-----------|
| *i32 | const int32_t* |
| *mut i32 | int32_t* |
| **i32 | const int32_t* const* |
| *mut *i32 | const int32_t** |

See [Lifetimes](#lifetimes).

## Arrays / Slices

Sync has static arrays, which are owned types, and slices, which are references types. Both are indexed starting from 0 using the `[]` operators. Trying to access an array out of bounds is a fatal Sync error, and will abort the Sync program (not the host program).

### Static Array

Sync supports fixed sized arrays that live in-place in memory. The syntax is [N]T where N is the number of elements, and T is the actual elements themselves.

```sync
[8]u16 // Static array of 8 unsigned 16 bit integers

[256]?*u8 // Static array of 256 nullable pointers to immutable unsigned 8 bit integers.

[2][2]i32 // Static array of 2 sub-arrays, each containing 2 signed 32 bit integers.
```

```sync
fn main() {
    const arr: [3]i32 = [1, 2, 3];
    print(arr)      // [1, 2, 3]
    print(arr[0])   // 1
    print(arr[1])   // 2
    print(arr[2])   // 3
    print(arr[3])   // abort!
}
```

### Slice

A slice is a reference to a contiguous region of memory. It contains a pointer to the beginning of the memory region, as well as a length. Slices cannot grow in capacity.

```sync
fn main() {
    const slice: []i32 = [1, 2, 3, 4, 5, 6];

    mut arr: [3]i32 = [7, 8, 9];
    const mutableSlice: []mut i32 = arr;
    mutableSlice[0] = 1;
    print(arr); // [1, 8, 9]
    print(arr[0]); // 1
}
```

You can also create a slice from part of an array.

```sync
fn main() {
    const arr: [6]i32 = [0, 1, 2, 3, 4, 5];
    const slice: []i32 = arr[1..4]; // start from index 1, end before index 4
    print(arr); // [1, 2, 3]
    print(arr[1]); // 2
}
```

See [Slice Lifetimes](#slice-lifetimes).

## Optionals

All Sync types are non-null by default, requiring opt-in nullability.

```sync
// normal integer that is non-null
const normalInt: i32 = 1;

// nullable integer that is set to a non-null value
const couldBeNullInt: ?i32 = 2;

// nullable integer that is set to null
const nullInt: ?i32 = null;
```

The same applies to pointers as well.

```sync
// non-nullable pointer
const pointer: *i32 = &normalInt;

// nullable pointer set to non-null value
const couldBeNullPointer: ?*i32 = &normalInt;

// nullable pointer set to null
const nullPointer: ?*i32 = null;
```

## Errors

Sync does not use exceptions, rather choosing to have errors as values. Similar to [Zig errors](https://ziglang.org/documentation/0.15.2/#Errors), error values have special syntax associated with them. Like [Rust std::Result](https://doc.rust-lang.org/std/result/), error values can be of any type, containing any user defined payload.

The syntax for an error type definition is `ErrorType!OkType`. The `ErrorType` may not be omitted, whereas the `OkType` does not have to be provided, indicated no value.

```sync
// error union containing a f32 as an ok value, rather than an i16 error
const okNumbers: i16!f32 = 0.1;
// error union containing an i16 as an error value, rather than an f32 ok value
const errorNumbers: i16!f32 = 1 as i16;

// error union containing a i32 as an ok value, rather than a str error
const okValues: str!i32 = 5;
// error union containing a str as an error value, rather than a i32 ok value
const errorValues: str!i32 = "failed!";
```

### Throw / Try / Catch

Sync uses the `throw`, `try`, and `catch` keywords for error handling, but it is notably different from exceptions. Error values can return from the callee to the caller, never going up past that relationship.

```sync
fn parseBool(input: str) str!bool {
    if input == "true"  {
        return true;
    }
    else if input == "false" {
        return false;
    }
    else {
        // return the string literal to the callee
        throw "Expected true or false";
    }
}

fn main() {
    // try to unwrap the error to get the ok value.
    // if it contains an error instead, catch it and do something with it.
    const parsed: bool = try parseBool("hello") catch as err {
        print(err);
        return;
    }   
    
    // declare the error type
    const parsedAgain: bool = try parseBool("hullo") catch as err: str {
        print(err);
        return;
    }
}
```

You can omit any arguments from the catch to discard them.

```sync
fn main() {
    // try to unwrap the error to get the ok value.
    // if it contains an error instead, catch it and do something with it.
    const parsed: bool = try parseBool("hello") catch {
        return;
    }
}
```

## Thread Safety

### Sync Block

The core feature of Sync is the `sync` block, in which one or more locks are acquired at once, with read-only or read-write protection. Deadlocks will not occur. Trying to acquire a lock that has already been acquired is a safe operation, handling re-entrance, contention, deadlock conditions, and more.

```sync
fn main() {
    const val: Unique(i32) = 5;

    sync val { // acquire a shared, read-only lock to val
        print(val); // prints 5
    } // lock is released here

    print(val) // COMPILER ERROR! Cannot use non-synced value from a Unique, Shared, or Weak data types.
}
```

These `sync` blocks can be read-write as well, having exclusive access.

```sync
fn main() {
    mut val: Unique(i32) = 5;

    sync mut val { // acquire an exclusive, read-write lock to val
        // any thread that tries to lock the same object will have to 
        // wait until the lock is released at the end of the scope
        val += 1;
        print(val); // prints 6
    } // lock is released here
    catch panic; // exclusive lock acquisition shouldn't fail here

    print(val) // COMPILER ERROR! Cannot use non-synced value from a Unique, Shared, or Weak data types.
}
```

### Deadlock Detection

Given that Sync can cross FFI boundaries, it is not possible to analyze if any locks are locked on the same thread before the sync code gets to use objects. As such, Sync uses re-entrant RwLocks with deadlock protection. Should a deadlock be detected, the sync block will fail. This can only ever happen with `mut` acquisition.

```sync
fn externCallableFunction(mut sharedVal: Shared(i32)) {
    sync mut sharedVal {
        
    } catch {
        print("deadlock detected");
        return;
    }
}

fn main() {
    const sharedVal: Shared(i32) = 8;
    externCallableFunction(sharedVal);
}
```

### Multiple Locks

```sync
fn main() {
    mut val1: Unique(i32) = 1;
    mut val2: Unique(i32) = 2;
    mut val3: Unique(i32) = 3;

    sync val1, mut val2, mut val3 {
        print(f"Val 1 before {val1}") // 1
        print(f"Val 2 before {val2}") // 2
        print(f"Val 3 before {val3}") // 3

        val2 += 1;
        val2 += 1;
    } catch panic;

    sync val1, val2, val3 {
        print(f"Val 1 before {val1}") // 1
        print(f"Val 2 before {val2}") // 3
        print(f"Val 3 before {val3}") // 4
    } // notice no catch here. read-only locks are infallible (excluding out of memory)
}
```

### Thread-safe Types

Any type can be prefixed with `Unique`, `Shared`, or `Weak` to mark it as a thread-safe, and able to be used across thread boundaries. All of these types come with reference counts (see Weak), and an rwlock.

#### Unique

The `Unique` type modifier gives single ownership. When the object goes out of scope or is generally destroyed, all references become invalidated.

```sync
Unique(i32) // Single ownership to shared memory of a signed 32 bit integer

fn main() {
    mut num: Unique(i32) = 0;
    sync mut num {
        num += 1;
    }
}
```

#### Shared

The `Shared` type modifier provides shared ownership. If the refcount is greater than 1, going out of scope or a destructor call does not invalidate the object, just decrements the refcount. It is only destroyed when the refcount reaches zero.

```sync
Shared(i32) // Shared ownership to shared memory of a signed 32 bit integer

fn main() {
    mut num1: Shared(i32) = 0;
    mut num2: Shared(i32) = num1;
    sync mut num1 {
        num1 += 1;
    }

    sync num2 {
        print(num2); // 1
    }
}
```

#### Weak

The `Weak` type modifier provides a weak reference that auto invalidates if the unique or shared object it references is destroyed.

```sync
Weak(i32) // Weak reference to another Unique or Shared which owns the i32

fn main() {
    mut num1: Unique(i32) = 0;
    mut num2: Weak(i32) = num1;
    sync mut num1 {
        num1 += 1;
    }

    sync num2 {
        print(num2.?); // 1
    }
}
```

And if the object gets destroyed, the weak reference is invalidated.

```sync
fn main() {
    mut num1: Shared(i32) = 0;
    mut num2: Weak(i32) = num1;
    someFunctionThatDestroys(num1);

    sync num2 {
        if num2 == null  {
            print("invalid!"); // prints!
        }
    }
}
```

## Built-ins

### str

A `str` is a special `[]u8` that is a utf8 valid string slice. It stores an always immutable pointer to a byte array along with a length in bytes, and is always guaranteed to be a valid utf8 string. The type of a string literal is `str`.

```sync
const fullName: str = "mclovin";
```

A `str` is a concrete type, but references memory that it doesn't own. Therefore, it can use [Concrete Lifetime Annotations](#concrete-lifetime-annotations).

```sync
const message: String = "hello world!";
// uses concrete lifetime annotation @'a, which is set as the lifetime of the variable `message`
const asStr: str@'a = message;
```

#### String

Sync supports a resizeable utf8 string. It is simlar to C++ std::string, Rust std::String, and Python str. It is written as `String`.

```sync
String // String type.

const s: String;
const slice: str = s; // Coerce to string slice.
```

#### Dynamic Array / Array List

The first and arguably most important is a dynamic array. It is similar to C++ std::vector, Rust std::vec, Zig std.ArrayList.Managed, and Python List[T]. It is written as `List(T)` where T is a generic type.

```sync
List(i32) // Resizeable array of signed 32 bit integers.

const arr: List(u8);
const slice: []u8 = arr; // Coerce to slice
```

#### Hash Map

The second, and extremely important is a hash map. Is it similar to C++ std::map, Rust std::collections::HashMap, Zig std.HashMap, and Python dict[T]. Is it written as `Map(K, V)` where K is a generic type for the map key, and V is a generic type for the map value. K must be hashable and be able to be equality compared.

```sync
Map(i32, f32) // Hash map of signed 32 bit integer keys and 32 bit float values.
```

#### Hash Set

The third, and fairly important is a hash set. Is it similar to C++ std::set, Rust std::collections::HashSet, and Python set[T]. Is it written as `Set(K)` where K is a generic type for the map key. K must be hashable and be able to be equality compared.

```sync
Set(i32) // Hash set of signed 32 bit integer keys.
```

#### Vectors

Sync supports vectors as their intended numerical types. The syntax is `Vec(N, T)` where N is the number of elements, and T is the type.

```sync
Vec(3, f32) // Equivalent to glsl vec3.
```

#### Matrices

Sync supports matrices. The syntax is `Mat(N, M, T)` where N is the number of elements in one direction, M is the elements in another direction, and T is the type.

GLSL uses column major ordering, but 2d arrays in most languages use row major ordering. **TODO** determine which is best.

```sync
Mat(4, 3, f32) // Matrix of 4x3 containing all 32 bit floats.
```

## Lifetimes

Since Sync does not use a garbage collector, a borrow checker is used to validate references. As a result, explicit lifetime syntax is supported, but ideally the compiler can determine it for you. The lifetime syntax is extremely similar to [Rust explicit lifetimes](https://doc.rust-lang.org/rust-by-example/scope/lifetime/explicit.html).

```sync
fn returnSamePtr(ptr: *'a i32) *'a i32 {
    return ptr;
}

fn main() {
    const val: i32 = 5;
    const ptr = returnSamePtr(&val);
}
```

### Slice Lifetimes

```sync
fn returnSameSlice(s: []'a i32) []'a i32 {
    return s;
}

fn main() {
    const arr: [6]i32 = [1, 2, 3, 4, 5, 6];
    const slice: []i32 = returnSameSlice(arr); // automatically coerce to slice
}
```

### Concrete Lifetime Annotations

Sometimes you have a concrete type but it still has lifetime bounds. For instance, having a handle such as to [GPU buffers](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateBuffers.xhtml), or most notably, a string slice.

```sync
str@'a // String slice (pointer to immutable utf8 character array + length) with an explicit lifetime specifier

u64@'a // Unsigned 64 bit integer with an explicit lifetime specifier.
```

## Tuples

Tuples are a modern stable of languages, and are extremely useful, especially when trying to return multiple values (especially across FFI boundaries) without having to make an explicit struct every time.

```sync
(f32, i32, i8) // Tuple containing a 32 bit float, signed 32 bit integer, and signed 8 bit integer.
(damage: u8, health: u16) // Tuple containing a named unsigned 8 byte integer, and a named unsigned 16 byte integer.

const tuple: (f32, i32, i8) = (1, 2, 3) // Tuple with values.

(i32) // Single element tuples not allowed, compiler will convert this to just i32.

() // Zero element tuples not allowed, compiler will convert this to no type and omit it entirely.
```

Tuple elements are accessed with array subscript notation.

```sync
fn main() {
    const tuple: (f32, i32, i8) = (1, 2, 3);
    print(tuple[0]); // 1.0
    print(tuple[1]); // 2
    print(tuple[2]); // 3
}
```

## Structs

Sync structs use the [C Structure Layout](https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Structure-Layout.html). This enables zero translation layers across FFI boundaries.

```sync
struct Example {
    a: u32,     // Starts at byte offset 0.
    b: bool,    // Starts at byte offset 4.
    c: u64,     // Starts at byte offset 8 due to padding.
} // Size: 16 bytes, Align: 8 bytes.

Example // Identifier referring to defined struct.

const e: Example = { .a = 1, .b = false, .c = 2 } // Struct usage.
```

### Struct Member Accessibility

All struct members are not `pub` by default. You must mark them `pub` to access them outside of the file they are defined.

```sync
struct Example {
    pub a: u32, // accessible outside of the source file.
    b: bool,    // not accessible.
    pub c: u64, // accessible.
}
```

## Functions

All functions have the following syntax.

```sync
fn name(arg1: arg1Type, arg2: arg2Type, ...) returnType
```

- `name` is the actual name of the function. This is omitted for anonymous functions.
- `argN` are the arguments. `mut` can be added in front to make the argument mutable, as function arguments are immutable.
- `argNType` is the type of that argument. It is required except for in anonymous functions.
- `returnType` is the the return type of the function. If omitted, the function has no return value. Sync does not have a Never type. All functions are expected to return in some form, even if it is a fatal return from the Sync program to the host program.

The amount of arguments in a function can range from 0 to many.

```sync
// Not accessible outside of this file.
fn add(a: i32, b: i32) i32 {
    return a + b;
}

// Accessible outside of this file.
pub fn pubAdd(a: i32, b: i32) i32 {
    return a + b;
}

// External function linked to this module from the host program
extern fn externAdd(a: i32, b: i32);
```

### Member Functions

Like Rust, Sync uses `impl` blocks to associate functions with a type.

```sync
struct Person {
    name: str,
    age: u16 // trailing comma optional, but not required
}

impl Person {
    // static function on Person.
    fn new(name: str, age: u16) Person {
        return Person{.name = name, .age = age};
    } 

    // member function, queryable by host program.
    fn birthday(self: *mut Self) {
        self.age += 1;
    }

    // member function. The `self` can be any name, not strictly `self`.
    // pub means this function can be used outside of this file.
    pub fn goBackInTime(this: *mut Self) {
        this.age -= 1;
    }
}

// multiple impl blocks is fine.
// impl blocks can also be defined outside of the file the struct / trait / enum is defined in.Is th
impl Person {
    // static function, since the first argument isn't one of `Self`, `*Self`, or `*mut Self`.
    fn changeName(name: str, self: *mut Self) {
        self.name = name;
    }
}

fn main() {
    mut person = Person.new("john smith", 32);
    person.birthday();
    Person.birthday(&mut person); // alternatively call with Type.function syntax.
    person.goBackInTime();
    Person.changeName("jane doe", &mut person); // have to use Type.function syntax because it's not a member function.
}
```

For convenience, [UFCS](https://en.wikipedia.org/wiki/Uniform_function_call_syntax) is also supported.

```sync
struct Person {
    name: str
    age: u16
}

fn makeToBaby(self: *mut Person) {
    self.age = 1;
}

fn main() {
    mut person = Person{.name = "donald", .age = 75};
    person.makeToBaby();
    makeToBaby(&mut person); // calling normally also works.
}
```

### Function Pointer

A function pointer points to metadata to execute a function. This memory is immutable, so a function pointer can never be `mut`. It also does not need a lifetime, as functions have static lifetime.

```sync
fn sayMessage(msg: str) {
    print(msg);
}

fn getSomething() i32 {
    return 1;
}

fn main() {
    const fptrMessage: *fn(str) = sayMessage;
    fptrMessage("hello!");

    const fptrNumber: *fn() i32 = getSomething;
    const num = fptrNumber();
}
```

### Extern Functions

Sync needs to communicate with the host program, meaning extern functions are a requirement. Extern functions are only linked to the module that the host application explicitly specifies, but like all functions, can be marked pub and allowed to use across module boundaries.

```sync
struct Person {
    name: str
    age: u16
}

extern fn writePersonToDatabase(person: *Person); // no function body.

fn main() {
    const person1 = Person{.name = "richard jingles", .age = 50};
    writePersonToDatabase(&person1);

    const person2 = Person{.name = "elizabeth jonkles", .age = 51};
    person2.writePersonToDatabase(); // UFCS works
}
```

Extern functions can also be namespaces, but must have a matching name.

```sync
struct Person {
    name: str
    age: u16
}

impl Person {
    extern fn writePersonToDatabase(person: *Person);
}

fn main() {
    const person1 = Person{.name = "richard jingles", .age = 50};
    person1.writePersonToDatabase();

    const person2 = Person{.name = "elizabeth jonkles", .age = 51};
    Person.writePersonToDatabase(&person2); // Type.function works too.
}
```

### Anonymous Functions / Closures / Lambdas

Anonymous functions are just function pointers.

```sync
fn callFunc(doThing: *fn(msg: str)) {
    doThing("hello?");
}

fn main() {
    callFunc(fn(msg: str) {
        print(msg);
    });
}
```

Anonymous functions cannot currently capture variables.

## Traits

Sync does not allow traditional OOP inheritance whatsoever. Sync does allow traits / interfaces without members. This is specifically to ensure exact memory layouts for host program compatibility, and stay compatible with everything. Function pointers and discriminants work on every valid target system and every supported programming language.

```sync
trait Speak {
    // any type that derives must implement this function.
    fn sayHi(self: *Self);

    // default implementation. any type that derives does not need to implement this.
    fn sayBye(self: *Self) {
        print("bye bye!");
    }
}

struct Person {
    name: str
    age: u16
}

struct Cat {
    name: str
    breed: str
}

impl Speak for Person {
    // implement the required function.
    fn sayHi(self: *Self) {
        print("hello there!");
    }

    // don't need to implement sayBye() because it has a default already.
}

impl Speak for Cat {
    // implement the required function.
    fn sayHi(self: *Self) {
        print("meow");
    }

    // override the default implementation of sayBye() explicitly.
    fn sayBye(self: *Self) {
        print("miau");
    }
}

fn main() {
    const person = Person{.name = "cat guy", .age = 38};
    const cat = Cat{.name = "bug", .breed = "orange"};

    person.sayHi();     // hello there!
    cat.sayHi();        // meow
    person.sayBye();    // bye bye!
    cat.sayBye();       // miau

    Person.sayHi(&person); // works like other functions too.
}
```

All trait functions are `pub` right now. This is open to being changed.

Traits abide by the [Orphan Rule](https://ianbull.com/notes/rusts-orphan-rule/) right now as well.

### Dynamic Dispatch

Sync uses fat pointers for dynamic dispatch, with the `dyn` keyword, which is a type that contains a pointer to some object, and a pointer to a [Virtual Method Table](https://en.wikipedia.org/wiki/Virtual_method_table).

```sync
fn callSayHi(obj: dyn Speak) {
    obj.sayHi();
}

fn main() {
    const person = Person{.name = "cat guy", .age = 38};
    const cat = Cat{.name = "bug", .breed = "orange"};
    callSayHi(&person); // hello there
    callSayHi(&cat);    // meow
}
```

For mutable trait references, use `dyn mut`. For lifetime bound trait references, use `dyn'a`

### Multiple Traits

Since traits don't add any extra memory to a type itself, you can add as many traits as you want to a type.

```sync
trait Jump {
    // trait functions
}

trait Swim {
    // trait functions
}

impl Jump for Person { ... }
impl Swim for Person { ... }
```

### Constrain Traits

Like generics, traits can be constrained to only allow certain types to implement them based on simple comptime evaluated boolean functions.

```sync
fn isMammal(T: Type) bool {
    return T == Person or T == Cat;
}

trait MakeNoise {
    where isMammal(Self);

    // trait methods
}
```

You can combine multiple conditions together.

```sync
trait BigNoise {
    where isMammal(Self) and @sizeOf(Self) > 32;

    // trait methods
}
```

### Built-in Traits

#### Destruct

```sync
trait Destruct {
    fn destruct(self: *mut Self);
}
```

`destruct` may not take additional arguments, and should never fail.

TODO evaluate validating infallible destructors

#### Equal

```sync
trait Equal {
    fn eq(self: *Self, other: *Self) bool;

    fn ne(self: *Self, other: *Self) bool {
        return !self.eq(other);
    }
}
```

#### Order

In reality, the only reason to distinguish between partial vs total ordering is from float NaN. If you think we are wrong, we are happy to evaluate if a distinction between partial vs total ordering is important enough for Sync. Given that Sync already does checked math, checking for non-NaN in cases where ordering is relevant such as sorting is acceptable.

```sync
enum Ordering: i32 {
    Less = -1,
    Equal = 0,
    Greater = 1,
}

trait Order {
    fn cmp(self: *Self, other: *Self) Ordering;
}
```

#### Hash

#### Iterator

TODO

#### Format

Sync has [Format Strings](#format-strings).

## Enums

Sync supports traditional enums, as well as sum types explained below. Auto-incrementing from 0 is done by default, but specific values can be used. Commas are required to separate the discriminants, but trailing commas are not required.

```sync
enum State {
    Walking,    // 0 implicitly
    Running,    // 1 ...
    Swimming,   // 2 ...
    Jumping,    // 3 ...
}

enum AnimationState {
    Idle = 4,
    Walking = State.Walking,
    Running = State.Running,
    Swimming = State.Swimming,
    Jumping = State.Jumping // no trailing comma, which is fine
}
```

All Sync non-tagged-union enums are either 1, 2, 4, or 8 bytes in size, picking the correct integer value for whatever is the value range of the enum discriminants by default. The integer can be specified.

```sync
enum FileType : i32 {
    Text,
    Image,
    Audio,
    Video,
}
```

All variants are `pub` if the enum itself is `pub`.

### Enum Member Functions

All Sync enums support impl blocks and can derive traits.

```sync
enum State {
    Walking,    // 0 implicitly
    Running,    // 1 ...
    Swimming,   // 2 ...
    Jumping,    // 3 ...
}

enum AnimationState {
    Idle = 4,
    Walking = State.Walking,
    Running = State.Running,
    Swimming = State.Swimming,
    Jumping = State.Jumping,
}

impl State {
    fn asAnimationState(self: Self) AnimationState {
        // convert enums
    }
}
```

This applies to [Sum Types](#sum-types) as well.

## Sum Types

Sync support sum types. These types must be compatible with C, so they follow a strict memory layout. Like Rust, Sync uses the `enum` keyword for both C style enums, as well as tagged unions.

```sync
enum State {
    Walking,
    Running(speedMultiplier: f32),
    Swimming(oxygenLeft: f32, onSurface: bool),
    Jumping(gravity: f32, force: f32)
}
```

Which is equivalent to the following in C due to only requiring 1 byte for the tag:

```C
struct State {
    uint8_t tag;
    union {
        uint8_t walking; // unused data
        float running; 
        struct { float oxygenLeft; bool onSurface; } swimming;
        struct { float gravity; float force; } jumping;
    } payload;
};
```

It is recommended to specify the size of the enum tag explicitly for FFI purposes.

```sync
enum State : i32 { // Explicitly use i32 as the tag representation
    Walking
    Running(speedMultiplier: f32)
    Swimming(oxygenLeft: f32, onSurface: bool)
    Jumping(gravity: f32, force: f32)
}
```

The above with the i32 tag is structurally equivalent to the following in C:

```C
struct State {
    int32_t tag;
    union {
        uint8_t walking; // unused data
        float running; 
        struct { float oxygenLeft; bool onSurface; } swimming;
        struct { float gravity; float force; } jumping;
    } payload;
};
```

You can also specify the discriminant of the sum type by assigning it a compile time integral value.

```sync
enum State {
    Walking = 5
    Running(speedMultiplier: f32) = 6
    Swimming(oxygenLeft: f32, onSurface: bool) = 7
    Jumping(gravity: f32, force: f32) = 8
}
```

All variants are `pub` if the enum itself is `pub`.

## Extending Built-ins

You may implement functions and traits on a built-in type within a module.

```sync
impl i32 {
    fn logToDatabase(self: Self) { ... }
}
```

## Generics

Generics in Sync work similar to Zig generics, but with specific / specialization support. Generics are considered compile time arguments, meaning all generic parameters excluding `Type` must be marked `comptime`. The `Type` type is only known at compile time, so it is considered implicitly generic.

### Function Generics

```sync
// T must be known at compile time
fn add(T: Type, num1: T, num2: T) T {
    // generic implementation
}

specific fn add(i8, num1: i8, num2: i8) i8 {
    // specialized for signed 8 bit integers
    // must have similar substituted signature (number of args and return type)
}

fn main() {
    add(u64, 1, 2); // 3
    add(1 as i32, 2 as i32); // automatically infer types, 3
    const n1: i8 = 1;
    const n2: i8 = 2;
    add(n1, n2); // the types match the specialized version of `add()`, so call that one
}
```

```sync
fn addBy(comptime N: i32, num: i32) i32 {
    return num + N;
}

extern fn getSomeUserInput() i32;

fn main() {
    add(1, 2); // 3
    add(1, 3); // 4, and calls the same function as above due to both using same generic value
    add(2, 2); // 4, but uses a different generated function

    const n1: i32 = getSomeUserInput();
    const n2: i32 = 2;
    add(n1, n2); // COMPILE ERROR! n1 is not known at compile time. 
}
```

All generic function arguments must appear as the first arguments, so that they can be omitted using generic parameter inferrence.

#### Constrain Function Generics

```sync
fn add(T: Type, num1: T, num2: T) T 
where @isInteger(T)
{
    return num1 + num2;
}
```

This also works for traits

```sync
trait Adder {
    fn performAdd(val1: Self, val2: Self) Self {
        return val1 + val2;
    }
}

fn add(T: Type, num1: T, num2: T) T 
where @implements(T, Adder)
{
    return num1.performAdd(num2);
}
```

### Struct Generics

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

#### Constrain Struct Generics

```sync
struct Example(T: Type)
where @sizeOf(T) == 16 // maybe this is required for padding reasons
{
    obj: T
}
```

## Imports

The `import` keyword can be used to import either modules, or files relative to the current one.

```sync
// src/player.sync

pub struct Player {
    pub health: f32
}

struct Enemy {
    damage: f32
}
```

```sync
// src/weapons/sword.sync
import("../player.sync");

pub struct Sword {
    pub material: str,
    pub durability: f32,
}

impl Sword {
    pub fn hitPlayer(self: *mut Self, player: *mut Player) {
        player.health -= 1;
        self.durability -= 1;
    }
}
```

```sync
// src/main.sync

// import all pub symbols from `player.sync`.
import("player.sync"); 
// import only the `Sword` struct and all of it's methods.
import("weapons/sword.sync").Sword; 

fn main() {
    mut player = Player{.health = 10};

    // COMPILE ERROR! Enemy is not marked `pub` and thus wasn't imported
    const enemy = Enemy{.damage = 5};

    mut sword = Sword{.material = "diamond", .durability = 100};
    sword.hitPlayer(&mut player);

    print(player.health);       // 9
    print(sword.durability);    // 99.0
}
```

### Circular Imports

CIRCULAR IMPORTS ARE OK!

```sync
// src/player.sync
import("weapons/sword.sync");

pub struct Player {
    pub health: f32,
    pub weapon: Sword,
}
```

```sync
// src/weapons/sword.sync
import("../player.sync");

pub struct Sword {
    pub material: str,
    pub durability: f32,
}

impl Sword {
    pub fn hitPlayer(self: *mut Self, player: *mut Player) {
        player.health -= 1;
        self.durability -= 1;
    }
}
```

There is a circular import here, as `player.sync` imports `weapons/sword.sync` which imports `player.sync`. This is fine as the compiler will correctly resolve which types depend on what. If `Sword` had a `Player` member variable that is not a reference type, then this would be a compiler error as the two struct definitions depend on each other.

### Re-exporting Symbols

There are two ways to re-export imported symbols. The first is through assigning it to a const variable.

```sync
// src/main.sync

// make a compile time global variable called `weapon` which contains all of the symbols in the import.
// the compiler will correctly identify the real types, not these aliases.
const weapon = import("weapons/sword.sync");

fn main() {
    const sword = weapon.Sword{.material = "iron", .durability = 50};
}
```

The second is by marking the import itself as pub, which makes any other file that imports this one to also include the imported symbols.

```sync
// src/weapons/sword.sync
pub struct Sword {
    pub material: str,
    pub durability: f32,
}
```

```sync
// src/player.sync

pub import("weapons/sword.sync");

pub struct Player {
    pub health: f32
}
```

```sync
// src/main.sync

// import all pub symbols from `player.sync`, which will also import any pub imports within that file recursively.
import("player.sync"); 

fn main() {
    const player = Player{.health = 10};
    const sword = Sword{.material = "wood", .durability = 15};
}
```

## Type Aliases

A type can be aliased like so:

```sync
const aliasName = i32;
```

This also works with imported types.

```sync
// src/weapons/sword.sync
pub struct Sword {
    pub material: str,
    pub durability: f32,
}
```

```sync
// src/main.sync

const Stick = import("weapons/sword.sync").Sword

fn main() {
    const s = Stick{.material = "stick", .durability = 4};
}
```

## Assert / Panic

Sync has `assert` as a dedicated keyword due to it's significant utility in writing good code.

Asserts come in one of two formats:

```sync
assert(condition);
assert(condition, message);
```

Sync has `panic` as a dedicated keyword to make writing `catch panic` simple, and generally to enforce the prevention of invalid program state even if asserts are disabled.

Panics comes in one of two formats:

```sync
panic;
panic(message);
```

If the `assert` condition evalutes to false, or a panic is executed, the Sync program is aborted, returning an error to the host program. After this, the Sync program is considered invalid and should be destroyed (`sy_program_destroy`). You are free to re-compile the Sync program again in the same process.

## Casting

Casts use the `as` keyword, following the syntax of `variable as T`.

TODO evaluate handling failed casts, such as downsizing integer.

## Helper Builtin Functions

### @sizeOf

```sync
@sizeOf(T: Type) usize
```

Get the size in bytes of a type `T`

### @alignOf

```sync
@alignOf(T: Type) usize
```

Get the align in bytes of a type `T`

### @implements

```sync
@implements(T: Type, Trait: Type) bool
```

Boolean test for if a type `T` implements the trait `Trait`.

### @skipTest

```sync
@skipTest()
```

Skips the current test case, logging it but not marking it as failed. See [Testing](#testing)

### @expectPanic

```sync
@expectPanic() { ... }
@expectPanic(msg) { ... }
```

Only for use within a test. Expect that the block of code correctly panics, optionally with an expected error message.

## Testing

TODO test metadata:

- Setup function
- Teardown function
- Skip of failure
- Should fail, optionally with error message

## Host

### Fetch Member Functions

As you can implement member functions on any type within a Sync program, the host program needs to be able to query these functions.

```sync
impl i32 {
    fn logToDatabase(self: i32) { ... }
}
```

```C
void getMembersExample(const SyProgram* program) {
    const SyType* i32TypeInfo = SY_TYPE_I32;
    const SyTypeFunctions* funcs = sy_program_get_members_of(program, i32TypeInfo);
    printf("%s\n", funcs.functions[0].name); // logToDatabase
    // do something with them
}
```
