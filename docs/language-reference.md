# Sync Language Reference

Sync is an embeddable, memory-safe, and thread-safe programming language designed to enable multithreaded scripting within any application. Sync uses the C struct memory layout, meaning you do not need to write translation layers for data that Sync uses. Your data types can work as is.

This allows custom logic, and dynamic modification to application functionality without having to compile anything ahead of time.

Native support for C, C++, Rust, and Zig is provided through CMake, Cargo, and Zig modules. You can compile the source files directly as is with a C11 and C++17 compiler without any extra flags required. See `build.md` for more info.

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

### Primitive Values

| Name | Description |
|------|-------------|
| true | boolean true value |
| false | boolean false value |
| null | the none value of any optional type |

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

### Static Array

Sync supports fixed sized arrays that live in-place in memory. The syntax is [N]T where N is the number of elements, and T is the actual elements themselves.

```sync
[8]u16 // Static array of 8 unsigned 16 bit integers

[256]?*u8 // Static array of 256 nullable pointers to immutable unsigned 8 bit integers.

[2][2]i32 // Static array of 2 sub-arrays, each containing 2 signed 32 bit integers.
```

### Slice

A slice is a reference to a contiguous region of memory. It contains a pointer to the beginning of the memory region, as well as a length. Slices cannot grow in capacity.

```sync
fn main() {
    const slice: []i32 = [1, 2, 3, 4, 5, 6];

    mut arr: [3]i32 = [7, 8, 9]
    const mutableSlice: []mut i32 = arr;
    mutableSlice[0] = 1;
    print(arr); // [1, 8, 9]
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
    if(input == "true") {
        return true;
    }
    else if(input == "false") {
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
    const parsed: bool = try parseBool("hello") catch (err: str) {
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
        print(err);
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
Unique i32 // Single ownership to shared memory of a signed 32 bit integer

fn main() {
    mut num: Unique i32 = 0;
    sync mut num {
        num += 1;
    }
}
```

#### Shared

The `Shared` type modifier provides shared ownership. If the refcount is greater than 1, going out of scope or a destructor call does not invalidate the object, just decrements the refcount. It is only destroyed when the refcount reaches zero.

```sync
Shared i32 // Shared ownership to shared memory of a signed 32 bit integer

fn main() {
    mut num1: Shared i32 = 0;
    mut num2: Shared i32 = num1;
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
Weak i32 // Weak reference to another Unique or Shared which owns the i32

fn main() {
    mut num1: Unique i32 = 0;
    mut num2: Weak i32 = num1;
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
    mut num1: Shared i32 = 0;
    mut num2: Weak i32 = num1;
    someFunctionThatDestroys(num1);

    sync num2 {
        if(num2 == null) {
            print("invalid!"); // prints!
        }
    }
}
```

## Built-ins

### Built-in Types

| Type | C/C++ Equivalent | Description |
|------|--------------|-------------|
| str | SyStringSlice / sy::StringSlice | pointer to the start of a utf8 string with a length |
| String | SyString / sy::String | memory managed utf8 string container |
| List(T) | sy::List<T> | memory managed dynamically resizeable contiguous array |
| Set(K) | sy::Set<K> | memory mananaged hash set |
| Map(K, V) | sy::Map<K, V> | memory mananaged hash set |
| Vec(N, T) | sy::Vec<N, T> | math vector N long containing T type |
| Mat(N, M, T) | sy::Mat<N, M, T> | math matrix of dimensions N*M of T type |
| Unique(T) | SyUnique / sy::Unique<T> | thread-safe single owned object |
| Shared(T) | SyShared / sy::Shared<T> | thread-safe multiple owned object |
| Weak(T) | SyWeak / sy::Weak<T> | thread-safe weak reference to Unique or Shared |

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

## Traits

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

## Structs

Sync structs use the [C Structure Layout](https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Structure-Layout.html). This enables zero translation layers across FFI boundaries.

```sync
struct Example {
    a: u32,     // Starts at byte offset 0
    b: bool,    // Starts at byte offset 4
    c: u64,     // Starts at byte offset 8 due to padding
} // Size: 16 bytes, Align: 8 bytes.

Example // Identifier referring to defined struct

const e: Example = { .a = 1, .b = false, .c = 2 } // Struct usage
```

### Struct Member Accessibility

All struct members are not `pub` by default. You must mark them `pub` to access them outside of the file they are defined.

```sync
struct Example {
    pub a: u32, // accessible outside of the source file
    b: bool,    // not accessible
    pub c: u64, // accessible
}
```

## Enums

Sync supports traditional enums, as well as sum types explained below. Auto-incrementing from 0 is done by default, but specific values can be used.

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

## Sum Types

Sync support sum types. These types must be compatible with C, so they follow a strict memory layout. Like Rust, Sync uses the `enum` keyword for both C style enums, as well as tagged unions.

```sync
enum State {
    Walking
    Running(speedMultiplier: f32)
    Swimming(oxygenLeft: f32, onSurface: bool)
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

## Functions

```sync
// Not accessible outside of this file
fn add(a: i32, b: i32) i32 {
    return a + b;
}

// Accessible outside of this file
pub fn pubAdd(a: i32, b: i32) i32 {
    return a + b;
}

// External function linked to this module from the host program
extern fn externAdd(a: i32, b: i32);
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

## Operator Precedence

Based on [C++ Operator Precedence](https://en.cppreference.com/w/cpp/language/operator_precedence.html)

| Precedence | Operator | Description | Associativity |
|------------|----------|-------------|---------------|
