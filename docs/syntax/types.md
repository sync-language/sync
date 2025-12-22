# Types Syntax

Sync uses static typing. If a type cannot be inferred by the compiler, it requires explicit specification.

## Concrete Types

A concrete type lives in memory as is, using the C struct memory layout if in a struct, or just living on the stack.

```sync
i32 // signed 32 bit integer.

struct Numbers {
    num1: i32,  // starts at byte offset 0.
    num2: i8,   // starts at byte offset 4.
    num3: u32,  // starts at byte offset 8 due to padding.
}
// `Numbers` struct occupies 12 bytes regardless of where it is placed, and has an alignment of 4 bytes.
```

## Pointers / References

Sync allows holding non-owning references to concrete (or dynamic dispatch) types. All of the following have implicit lifetime specifiers, but also support explicit lifetime specifiers as will be described further below. All pointers/references are non-null in Sync, requiring extra syntax to specify nullability.

### Pointer

A pointer points at the beginning of the memory region that a singular object occupies.

```sync
*i32 // Pointer to an immutable signed 32 bit integer.

*mut i32 // Pointer to a mutable signed 32 bit integer.
```

### Slice

A slice is a pointer to a region of memory along with a length (in number of elements).

```sync
[]i32 // slice to an zero to many immutable signed 32 bit integers.

[]mut i32 // slice to zero to many mutable signed 32 bit integers.
```

### String Slice

A string slice is a special slice that is a utf8 valid string slice. It stores an always immutable pointer to a byte array along with a length in bytes, and is always guaranteed to be a valid utf8 string.

```sync
str // valid utf8 slice.
```

### Dynamic Dispatch

A dyn Trait pointer contains a pointer to the object itself along with a pointer to the vtable of the object for that trait.

```sync
dyn TraitName // pointer + vtable to an immutable object that implements TraitName.

dyn mut TraitName // pointer + vtable to a mutable object that implements TraitName.
```

### Function Pointers

Function pointers are incredibly useful. Function pointers are always immutable. `*mut fn` is illegal syntax.

```sync
*fn(i32, i32) // Pointer to a function that takes 2 signed 32 bit integers and returns nothing

*fn(f32) i32 // Pointer to a function that takes one 32 bit float, and returns a signed 32 bit integer.
```

## Lifetime Specifiers

Sync uses a custom borrow checker rather than garbage collection. As a result, occasionally lifetime specifiers will need to be added. These work very similar to [Rust](https://doc.rust-lang.org/rust-by-example/scope/lifetime/explicit.html) lifetime specifiers

They can be one of the following:

1. Pointer/Reference annotation
2. Concrete annotation

### Pointer/Reference Annotation

A pointer is a memory address that we associate extra information to. The memory it points to should not become invalid while the pointer is still in use.

```sync
*'a i8 // Pointer with an explicit lifetime specifier to an immutable signed 8 bit integer.

*'a mut i8 // Pointer with an explicit lifetime specifier to a mutable signed 8 bit integer.

[]'a i16 // Slice (pointer + length) with an explicit lifetime specifier to an immutable array of signed 16 bit integers (not resizeable).

[]'a mut i16 // Slice (pointer + length) with an explicit lifetime specifier to a mutable array of signed 16 bit integers (not resizeable).

dyn'a TraitName // Dynamic dispatch reference (pointer + vtable) with an explicit lifetime specifier to an immutable trait object.

dyn'a mut TraitName // Dynamic dispatch reference (pointer + vtable) with an explicit lifetime specifier to a mutable trait object.
```

### Concrete Annotation

Sometimes you have a concrete type but it still has lifetime bounds. For instance, having a handle such as to [GPU buffers](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glCreateBuffers.xhtml), or most notably, a string slice.

```sync
str@'a // String slice (pointer to immutable utf8 character array + length) with an explicit lifetime specifier

u64@'a // Unsigned 64 bit integer with an explicit lifetime specifier.
```

## Nullability

As mentioned, pointers and references in Sync are always non-null. Any type, including reference types, can be made nullable, in which they either contain a value, or are `null` (keyword).

```sync
?i32 // Either null or a signed 32 bit integer.

?*i32 // Either null, or a valid pointer to an immutable signed 32 bit integer.

?*mut i32 // Either null, or a valid pointer to a mutable signed 32 bit integer.

?[]i32 // Either null, or a slice containing zero to many signed 32 bit integers.

*?i32 // A non-null pointer that points at an immutable nullable signed 32 bit integer.

*mut ?i32 // A non-null pointer that points at a mutable nullable signed 32 bit integer.

[]?i32 // A slice that contains zero to many nullable signed 32 bit integers.

?dyn TraitName // Either null, or a dynamic dispatch reference to an object that implements TraitName.

fn main() {
    const num1: ?i32 = null;
    if(num == null) {
        print("is null!");
    }

    const num2: ?i32 = 5;
    if(num != null) {
        print(num2.?);
    }
}
```

## Error / Result Types

Sync does not support exceptions, but does support errors as values. They function similar to Zig's Error union, except they can have payloads attached to them, and do not have a global error set.

Error unions are defined with the following syntax:

```sync
ErrorType!OkType
```

```sync
i16!f32 // Error union that contains either a signed 16 bit integer indicating an error, or a 32 bit float indicating no error.

str!i32 // Error union that contains either a string slice indicating an error, or a signed 32 bit integer indicating no error.
```

## Thread-safe Types

Any type can be prefixed with `Unique`, `Shared`, or `Weak` to mark it as a thread-safe, and able to be used across thread boundaries. All of these types come with reference counts (see Weak), and an rwlock.

### Unique

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

### Shared

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

### Weak

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

## Built in Containers

Since Sync is designed to be embedded, many generic containers are implemented and optimized for Sync.

### String

Sync supports a resizeable utf8 string. It is simlar to C++ std::string, Rust std::String, and Python str. It is written as `String`.

```sync
String // String type.

const s: String;
const slice: str = s; // Coerce to string slice.
```

### Static Array

Sync supports fixed sized arrays that live in-place in memory. The syntax is [N]T where N is the number of elements, and T is the actual elements themselves.

```sync
[8]u16 // Static array of 8 unsigned 16 bit integers

[256]?*u8 // Static array of 256 nullable pointers to immutable unsigned 8 bit integers.

[2][2]i32 // Static array of 2 sub-arrays, each containing 2 signed 32 bit integers.
```

### Dynamic Array / Array List

The first and arguably most important is a dynamic array. It is similar to C++ std::vector, Rust std::vec, Zig std.ArrayList.Managed, and Python List[T]. It is written as `List(T)` where T is a generic type.

```sync
List(i32) // Resizeable array of signed 32 bit integers.

const arr: List(u8);
const slice: []u8 = arr; // Coerce to slice
```

### Hash Map

The second, and extremely important is a hash map. Is it similar to C++ std::map, Rust std::collections::HashMap, Zig std.HashMap, and Python dict[T]. Is it written as `Map(K, V)` where K is a generic type for the map key, and V is a generic type for the map value. K must be hashable and be able to be equality compared.

```sync
Map(i32, f32) // Hash map of signed 32 bit integer keys and 32 bit float values.
```

### Hash Set

The third, and fairly important is a hash set. Is it similar to C++ std::set, Rust std::collections::HashSet, and Python set[T]. Is it written as `Set(K)` where K is a generic type for the map key. K must be hashable and be able to be equality compared.

```sync
Set(i32) // Hash set of signed 32 bit integer keys.
```

### Vectors

Sync supports vectors as their intended numerical types. The syntax is `Vec(N, T)` where N is the number of elements, and T is the type.

```sync
Vec(3, f32) // Equivalent to glsl vec3.
```

### Matrices

Sync supports matrices. The syntax is `Mat(N, M, T)` where N is the number of elements in one direction, M is the elements in another direction, and T is the type.

GLSL uses column major ordering, but 2d arrays in most languages use row major ordering. **TODO** determine which is best.

```sync
Mat(4, 3, f32) // Matrix of 4x3 containing all 32 bit floats.
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

Sync structs use the C memory layout. This enables zero translation layers across FFI boundaries.

```sync
struct Example {
    a: u32,     // Starts at byte offset 0
    b: bool,    // Starts at byte offset 4
    c: u64,     // Starts at byte offset 8 due to padding
} // Size: 16 bytes, Align: 8 bytes.

Example // Identifier referring to defined struct

const e: Example = { .a = 1, .b = false, .c = 2 } // Struct usage
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

All Sync non-tagged-union enums are either 1, 2, 4, or 8 bytes in size, picking the correct integer value for whatever is the value range of the enum variants by default. The integer can be specified.

```sync
enum FileType : i32 {
    Text,
    Image,
    Audio,
    Video,
}
```

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
