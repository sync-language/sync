#ifndef _SY_TYPES_ORDERING_ORDERING_H_
#define _SY_TYPES_ORDERING_ORDERING_H_

enum SyOrdering {
    SyOrderingLess = -1,
    SyOrderingEqual = 0,
    SyOrderingGreater = 1,

    _SY_ORDERING_MAX = 0x7FFFFFFF
};

#endif // _SY_TYPES_ORDERING_ORDERING_HPP_