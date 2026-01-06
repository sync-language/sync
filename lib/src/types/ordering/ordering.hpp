#ifndef _SY_TYPES_ORDERING_ORDERING_HPP_
#define _SY_TYPES_ORDERING_ORDERING_HPP_

#include "../../core/core.h"

namespace sy {
enum Ordering : int32_t {
    Less = -1,
    Equal = 0,
    Greater = 1,
};
}

#endif // _SY_TYPES_ORDERING_ORDERING_HPP_