#pragma once
#ifndef SY_UTIL_POW_OF_2_HPP_
#define SY_UTIL_POW_OF_2_HPP_

template <typename T> constexpr bool isPowOf2(T num) { return (num & (num - 1)) == 0; }

template <typename T> constexpr T nearestPowOf2(T num) {
    // https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
    if constexpr (sizeof(T) == 8) {
        num--;
        num |= num >> 1;
        num |= num >> 2;
        num |= num >> 4;
        num |= num >> 8;
        num |= num >> 16;
        num |= num >> 32;
        num++;
        return num;
    } else if constexpr (sizeof(T) == 4) {
        num--;
        num |= num >> 1;
        num |= num >> 2;
        num |= num >> 4;
        num |= num >> 8;
        num |= num >> 16;
        num++;
        return num;
    }
}

#endif // SY_UTIL_POW_OF_2_HPP_
