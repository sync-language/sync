#pragma once
#ifndef SY_UTIL_ALIGN_HPP_
#define SY_UTIL_ALIGN_HPP_

constexpr size_t byteOffsetForAlignedMember(size_t existingSize, size_t otherAlign) {
    const size_t remainder = existingSize % otherAlign;

    if(remainder == 0) return existingSize;

    return existingSize + (otherAlign - remainder);
}

#endif // SY_UTIL_ALIGN_HPP_