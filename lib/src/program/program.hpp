//! API
#pragma once
#ifndef _SY_PROGRAM_PROGRAM_HPP_
#define _SY_PROGRAM_PROGRAM_HPP_

namespace sy {
    namespace c {
        #include "program.h"
    }

    class Program {

    private:
        c::SyProgram program;
    };
}

#endif // _SY_PROGRAM_PROGRAM_H_