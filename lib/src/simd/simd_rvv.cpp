// SIMD implementations ONLY for rvv 1.0.

#if defined(__riscv) && (__riscv_xlen == 64)

#include "simd.hpp"

#if !defined(SYNC_NO_RVV)
#include <riscv_vector.h>
#endif

sy::Option<uint8_t> sy::internal::firstZeroIndex8x16(const uint8_t* mem) noexcept {
    assert_aligned(mem, 16);
    using Fn = sy::Option<uint8_t> (*)(const uint8_t*);
    static const Fn impl = []() -> Fn {
#if !defined(SYNC_NO_RVV)
        if (cpuHasRVV()) {
            return +[](const uint8_t* buf) -> sy::Option<uint8_t> {
                // Good lord RISC-V is complicated.
                // VLEN COULD be less than 128 bits (16 bytes), but this is basically never the
                // case for RISC-V64.
                size_t vl = __riscv_vsetvl_e8m1(16);
                vuint8m1_t v = __riscv_vle8_v_u8m1(buf, vl);
                vbool8_t mask = __riscv_vmseq_vx_u8m1_b8(v, 0, vl);
                long idx = __riscv_vfirst_m_b8(mask, vl);
                if (idx >= 0) {
                    return static_cast<uint8_t>(idx);
                }

                size_t base = vl;
                size_t remaining = 16 - vl;
                if (remaining > 0) [[unlikely]] {
                    // Just a sanity check of a hypothetical VLEN < 128 implementation.
                    // Probably will remove this in the future.
                    do {
                        vl = __riscv_vsetvl_e8m1(remaining);
                        v = __riscv_vle8_v_u8m1(buf + base, vl);
                        mask = __riscv_vmseq_vx_u8m1_b8(v, 0, vl);
                        idx = __riscv_vfirst_m_b8(mask, vl);
                        if (idx >= 0) {
                            return static_cast<uint8_t>(base + idx);
                        }
                        base += vl;
                        remaining -= vl;
                    } while (remaining > 0);
                }
                return std::nullopt;
            };
        }
#endif
        return +[](const uint8_t* buf) -> sy::Option<uint8_t> {
            for (uint8_t i = 0; i < 16; ++i) {
                if (buf[i] == 0) {
                    return i;
                }
            }
            return std::nullopt;
        };
    }();
    return impl(mem);
}

#endif
