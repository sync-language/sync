# Sync Borrow Checker Implementation

Sync provides explicit opt-in thread safety mechanisms. As such, many mutable and immutable references can be alive at the same time. This is because these references will never escape thread boundaries, and in order to get a reference to shared memory, the programmer is forced to use the thread safety mechanisms.

This is a lot less restrictive than Rust's borrow checker (although Rust's is for very good reason).

## Lifetime Relationships

- A reference cannot outlive the variable that it comes from
- A reference from another reference has the same lifetime constraints
- A reference to a field or index in an array/map/other has a lifetime constraint of the owning objects lifetime
- A function returning a reference uses whichever is the most restrictive lifetime of the possible options if there is branching

## Reference / Iterator Invalidation Conditions

1. Destructor
2. Move
3. Fundamental structural change

All of these impact field / index access, as getting a reference to an objects field is a chain.

### Destructor

Naturally, if an object's destructor is called, that object is no longer considered valid, and thus references to it are invalid.

### Move

If an object is moved in memory a reference to it is invalid. There are cases where the memory pointed to is still valid, as a result of being allocated elsewhere, however this should either be allowed with caution, or prohibited as well.

### Fundamental Structural Change

Many functions will fundamentally change the memory associated with a given data structure. For instance, appending an element to a dynamic array may trigger a re-allocation to grow its capacity. The reference to the dynamic array data structure is sound, but a reference to one of the elements it owns will suddenly become invalid. These operations must be known by the compiler.

Unlike in Rust, dynamic arrays, hash maps, and other data structures are supplied as part of the core language implementation, not just the standard library implementation like in Rust. As such, more implicit information can be supplied to the compiler to have less restrictive rules.

If a function takes a mutable reference, and causes a structural change, this counts as an invalidating operation.

## Control Flow

### Branches

A reference must be valid in all branches, as all could be explored.

Invalidation that occurs within a branch is invalid for the rest of the branch, and if execution escapes that branch (no early return).

### Loops

A reference must not become invalid inside a loop body if it could be used in subsequent iterations.

## Edge Cases

### External Functions

When returning a reference from an external function (such as a C function), having a lifetime annotation is required, as there is no way for the Sync compiler itself to know the lifetime bounds of that reference. For familiarity, using Rust style `*'a` may be the best decision.

Passing any mutable reference to an external function will automatically invalidate any existing reference, as there is no way of validating if any operations cause reference / iterator invalidation across boundaries.

## Relevant Reading

- [Oxide: The Essence of Rust](https://arxiv.org/pdf/1903.00982)
- [Rust Polonius Source](https://github.com/rust-lang/polonius)
- [Tree Borrows](https://perso.crans.org/vanille/treebor/)
- [C++ Container Iterator Invalidation](https://en.cppreference.com/w/cpp/container.html)
