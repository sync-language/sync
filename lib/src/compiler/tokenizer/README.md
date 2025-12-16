# Tokenizer

All tokens are whitespace sensitive. For instance, `11 11` would not be the integer `1111` but rather two integers of `11`. Furthermore, `someVar -11` would not be the variable minus the value positive 11, but rather an identifier token and then an integer literal token of negative 11.

## Tokens

### Keywords

- const
- mut
- comptime
- return
- throw
- fn
- pub
- if
- else
- switch
- while
- for
- break
- continue
- struct
- enum
- union
  - Should maybe be like rust enums instead for simplicity?
- dyn
- dyn'
  - Lifetime bound dynamic dispatch reference (pointer + vtable)
- trait
- sync
- unsafe
  - Is this necessary?
- true
- false
- null
- and
- or
- import
  - How should imports be done?
- mod
  - How should modules work?
- extern
  - Scripts only work well realistically if they can call into the host program. This must be figured out precisely and safely.
- assert
  - How should asserts look?
- print
  - How should printing look?
- println
  - Should print also add a newline? Or should println be used instead?

### Primitive Types

Naturally there are more primitive types, such as arrays. The ones here are the ones that the tokenizer will have to look out for. Arrays for instance are a combination of multiple symbols, identifiers, keywords, and literals.

- bool
- i8
- i16
- i32
- i64
- u8
- u16
- u32
- u64
- usize
  - Equivalent to C size_t. Bit size changes depending on target (eg X86_64 vs wasm32). Must determine if should be assumed to be the same size as a pointer / reference.
- f32
  - For all targets this is just a C float
- f64
  - For all targets this is just a C double
- char
  - [Rust Char](https://doc.rust-lang.org/std/primitive.char.html) makes a lot of sense here.
- str
  - Utf8 string slice. Contains an immutable `const char*` to the string along with a length `size_t`. Doesn't have to be null terminated, and can be zeroed.
- String
  - Utf8 owned string. Is null terminated. May allocate memory.
- PartialOrd
  - Some types may result in invalid ordering such as NaN. Is this necessary?
- Ord
  - For types that have total ordering. Is this necessary?
- Owned
  - Allocated type with thread locking mechanisms, with single ownership.
- Shared
  - Allocated type with thread locking mechanisms, with shared ownership.
- Weak
  - Weak reference type with thread locking mechanisms, with no ownership.
- Type
- List
- Map
- Set

### Literals

- Number
  - Number strings that may have decimals. The compiler will convert the parsed value to the correct integer type when necessary.
- Char
  - String with a single unicode character in utf8 format. Is enclosed by `'` characters.
- Str
  - Utf8 string slice with one or many characters. Is enclosed by `"` characters.

### Identifiers

Identifiers must start with alphabetical characters, being [a-z] or [A-Z], or an underscore `_`. Everything else is invalid.

### Operators

- == (Equal)
- = (Assign)
- != (Not Equal)
- .! (Error Unwrap)
- .? (Option Unwrap)
- ! (Not)
  - Cannot be meaningfully distinguished from error result, as knowing if it's the not operator or the error result type can only be known with more information, such as if it's part of type information (return type for instance), the multiple symbols that came before, etc. As a result it will use `ExclamationSymbol`.
- .* (Dereference)
  - Is this needed?
- <= (Less Or Equal)
- < (Less)
- \>= (Greater or Equal)
- \> (Greater)
- \+= (Add Assign)
- \+ (Add)
- \-= (Subtract Assign)
- \- (Subtract)
- \*= (Multiply Assign)
- \* (Multiply)
- /= (Divide Assign)
- / (Divide)
- ^^= (Power Assign)
  - Makes more sense as just a built in function?
- ^^ (Power)
  - Makes more sense as just a built in function?
- %= (Modulo Assign)
  - Should be distinguished for positive and negative?
- % (Modulo)
  - Should be distinguished for positive and negative?
- \>>= (Bitshift Right Assign)
- \>> (Bitshift Right)
- <<= (Bitshift Left Assign)
- << (Bitshift Left)
  - Must be distinguished from make immutable reference. Must be after an identifier.
- &= (Bit And Assign)
- & (Bit And)
  - Cannot be meaningfully distinguished from reference at tokenizing time, as knowing if it's a reference type, make reference operator, or bit-and can only be known with more information, such as if it's part of function signature, the multiple symbols that came before, etc. As a result, it will use `AmpersandSymbol`.
- |= (Bit Or Assign)
- | (Bit Or)
- ^= (Bit Xor Assign)
- ^ (Bit Xor)
- ~= (Bit Not Assign)
- ~ (Bit Not)

### Symbols

[Difference between different bracket types](https://practicaltypography.com/parentheses-brackets-and-braces.html).

- ( (Left Parentheses)
- ) (Right Parentheses)
- [ (Left Bracket)
- ] (Right Bracket)
- { (Left Brace)
- } (Right Brace)
- : (Colon)
- ; (Semicolon)
- . (Dot)
- , (Comma)
- ? (Optional)
- ! (Error Result)
  - Cannot be meaningfully distinguished from not operator, as knowing if it's the not operator or the error result type can only be known with more information, such as if it's part of type information (return type for instance), the multiple symbols that came before, etc. As a result it will use `ExclamationSymbol`.
- & (Make Reference)
  - Cannot be meaningfully distinguished from bit-and at tokenizing time, as knowing if it's a reference type, make reference operator, or bit-and can only be known with more information, such as if it's part of function signature, the multiple symbols that came before, etc. As a result, it will use `AmpersandSymbol`.
- \* (Pointer)
  - Cannot be meaningfully disginguished from multiply at tokenizing time, as knowing which it is requires more information.
- \*' (Lifetime Pointer)
- @' (Conrete Lifetime)
  - For FFI, lifetime bounds may not be clear especially with types that don't contain references (handles such as GPU buffers?), so having a concrete lifetime annotation on a type is useful. This can also be used on string slices (str@'a).

## Token Bit Optimization

Checking [godbolt](godbolt.org) outputs, having the token tag be 8 bits, and the token location be 24 bits results in more optimal assembly output than 7-25. For instance, on X86_64 with gcc 15.1 compiling with flags `-O2 -std=c++17`, we see the following program:

```C++
struct Token {
    uint32_t tag : 8;
    uint32_t location : 24;
};

Token makeToken(int inTag, int inLocation) {
    Token t;
    t.tag = inTag;
    t.location = inLocation;
    return t;
}

uint32_t getTokenTag(Token t) {
    return t.tag;
}

uint32_t getTokenLocation(Token t) {
    return t.location;
}
```

results in the following assembly:

```asm
makeToken(int, int):
        sal     esi, 8
        movzx   eax, dil
        or      eax, esi
        ret
getTokenTag(Token):
        movzx   eax, dil
        ret
getTokenLocation(Token):
        mov     eax, edi
        shr     eax, 8
        ret
```

If instead we change tag to 7 bits, and location to 25 bits, we get the following assembly:

```asm
makeToken(int, int):
        mov     eax, esi
        and     edi, 127
        sal     eax, 7
        or      eax, edi
        ret
getTokenTag(Token):
        mov     eax, edi
        and     eax, 127
        ret
getTokenLocation(Token):
        mov     eax, edi
        shr     eax, 7
        ret
```

This makes no difference for the location, however most of the querying on the token will be for the type of the token. The extra add instruction may make a difference. Furthermore actually making the token is easier with this bit pattern.

As a result of all of that, since file location is stored in 24 bits, that means the maximum source code string size in bytes must be able to fit in 24 bits.
