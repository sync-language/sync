# Coding Style Guide

Consistency in how code is written is critical to maintain an understandable library for those using it, and those maintaining it.

## C/C++ Headers

All headers that are usable as part of the C API must end in the `.h` extension. All headers that are usable as part of the C++ API must end in the `.hpp` extension. This distinction is important as sync maintains both a C and C++ API.

### API Headers

All headers that are included as part of the public API of sync must have `//! API` as the first line in the file. This makes it immediately obvious that all symbols within the file are for public use, unless specified otherwise, such as anything within `namespace detail` for C++ headers, or anything marked with a leading underscore such as

```C
struct Example {
    int _doNotTouch;
};
```

### Header Guards

All C and C++ headers should have `#pragma once` and `#ifndef` header guards to avoid multiple inclusion. The naming scheme depends on file path, as well as whether it's a C or C++ header. The format is in the form `*SY_<PATH>_<FILE>_H(PP)_*`

C

```C
// File lib/src/types/string/string.h
//! API <- Put this here cause API header
#pragma once
#ifndef SY_TYPES_STRING_STRING_H_
#define SY_TYPES_STRING_STRING_H_

// ... Header contents

#endif // SY_TYPES_STRING_H_
```

C++

```C++
// File lib/src/types/string/string.hpp
//! API <- Put this here cause API header
#pragma once
#ifndef SY_TYPES_STRING_STRING_HPP_
#define SY_TYPES_STRING_STRING_HPP_

// ... Header contents

#endif // SY_TYPES_STRING_STRING_HPP_
```
