# Error Unions

Error unions are defined with the following syntax:

```text
OkType!ErrorType
```

This goes against the Zig syntax, but is more consistent with other error/result types such as Rust's `std::Result`, and C++ `std::expected`.

To return the ok type, you simply return it:

```text
fn example() i32!i32 {
    return 5;
}
```

To return the error type, you throw it:

```text
fn example() i32!i32 {
    throw 5;
}
```

Sync does not support exceptions at the language level. Throw is just convenient syntax for returning an error value, as it just takes less effort to type.
