#ifndef SY_COMPILER_GRAPHI_MODULE_DEPENDENCY_GRAPH_HPP_
#define SY_COMPILER_GRAPHI_MODULE_DEPENDENCY_GRAPH_HPP_

#include "../../core.h"
#include "../../program/program_error.hpp"
#include "../../types/array/dynamic_array.hpp"
#include "../../types/result/result.hpp"

namespace sy {
class Module;

/// @brief DAG
struct ModuleDependencyGraph {
    [[nodiscard]] static Result<ModuleDependencyGraph, ProgramError> init(DynArray<const Module*> modules) noexcept;

    // struct ModuleDepGraphNode {
    //     const Module* mod;
    //     DynArray<ModuleDepGraphNode> dependencies;
    // };

    DynArray<DynArray<const Module*>> layers;
};
} // namespace sy

#endif // SY_COMPILER_GRAPHI_MODULE_DEPENDENCY_GRAPH_HPP_