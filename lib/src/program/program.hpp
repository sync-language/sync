//! API
#pragma once
#ifndef SY_PROGRAM_PROGRAM_HPP_
#define SY_PROGRAM_PROGRAM_HPP_

#include "../core.h"

namespace sy {
class Function;

class CallStack {
  public:
    CallStack(const Function* const* inFunctions, size_t inLen);
    size_t len() const { return this->_len; }
    const Function* operator[](size_t idx) const;

  private:
    const Function* const* _functions;
    size_t _len;
};

class ProgramModule;

class Program {
  private:
    void* inner_;
};

class ProgramModule {
  private:
    void* inner_;
};
} // namespace sy

#endif // _SY_PROGRAM_PROGRAM_H_