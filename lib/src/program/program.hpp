//! API
#pragma once
#ifndef SY_PROGRAM_PROGRAM_HPP_
#define SY_PROGRAM_PROGRAM_HPP_

#include "../core.h"
#include "../types/array/slice.hpp"
#include "../types/option/option.hpp"
#include "../types/string/string_slice.hpp"
#include "module_info.hpp"

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

class SY_API ProgramModule final {

    // module name and version
    ModuleVersion moduleInfo() const noexcept;

    // get function(s) by unqualified name
    Option<Slice<const Function*>> getFunctionsByUnqualifiedName(StringSlice unqualifiedName) const noexcept;

    // get function (singular) by fully qualified name
    Option<const Function*> getFunctionByQualifiedName(StringSlice qualifiedName) const noexcept;

    // get type(s) by unqualified name

    // get type (singular) by fully qualified name

    // globals too?

  private:
    void* inner_;
};

class Program final {

    // get all modules

    // get module by name and version (how to handle latest?)

  private:
    void* inner_;
};

} // namespace sy

#endif // _SY_PROGRAM_PROGRAM_H_