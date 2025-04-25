#include "function.h"

extern "C" {
    SY_API SyFunctionCallArgs sy_function_start_call(const SyFunction* self) {
        SyFunctionCallArgs args = {0};
        args.func = self;
        return args;
    }
}