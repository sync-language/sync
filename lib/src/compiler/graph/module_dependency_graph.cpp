#include "module_dependency_graph.hpp"
#include "../../types/hash/map.hpp"
#include "../compiler.hpp"
#include <iostream>

using namespace sy;

/*
Circular dependency

If module A depends on B and B depends on A, this is an error
*/

static Result<DynArray<const Module*>, ProgramError> makeFirstLayer(DynArray<const Module*>* modules) {
    DynArray<const Module*> nodes{modules->alloc()};
    size_t i = 0;
    while (i < modules->len()) {
        if ((*modules)[i]->dependencies().len() == 0) {
            if (nodes.push((*modules)[i]).hasErr()) {
                return Error(ProgramError({}, ProgramError::Kind::OutOfMemory));
            }
            modules->removeAt(i);
        } else {
            i++;
        }
    }

    if (nodes.len() == 0) {
        return Error(ProgramError({}, ProgramError::Kind::CompileModuleDependencyGraph));
    }

    return nodes;
}

Result<ModuleDependencyGraph, ProgramError> sy::ModuleDependencyGraph::init(DynArray<const Module*> modules) noexcept {
    ModuleDependencyGraph self{};

    { // first layer
        auto res = makeFirstLayer(&modules);
        if (res.hasErr()) {
            return Error(res.takeErr());
        }
        if (self.layers.push(res.takeValue()).hasErr()) {
            return Error(ProgramError({}, ProgramError::Kind::OutOfMemory));
        }
    }

    while (modules.len() > 0) {
    }

    return self;
}

#if SYNC_LIB_WITH_TESTS

#include "../../doctest.h"

TEST_CASE("ModuleDependencyGraph zero modules") {
    Compiler compiler = Compiler::create().takeValue();
    {
        auto modules = compiler.allModules().value();
        CHECK_EQ(modules.len(), 0);
        auto res = makeFirstLayer(&modules);
        CHECK(res.hasErr());
        CHECK_EQ(res.err().kind(), ProgramError::Kind::CompileModuleDependencyGraph);
    }
    {
        auto modules = compiler.allModules().value();
        auto res = ModuleDependencyGraph::init(compiler.allModules().value());
        CHECK(res.hasErr());
        CHECK_EQ(res.err().kind(), ProgramError::Kind::CompileModuleDependencyGraph);
    }
}

TEST_CASE("ModuleDependencyGraph one module") {
    Compiler compiler = Compiler::create().takeValue();
    CHECK(compiler.addOrGetModule("a", {}).hasValue());
    {
        auto modules = compiler.allModules().value();
        CHECK_EQ(modules.len(), 1);
        auto res = makeFirstLayer(&modules);
        CHECK(res.hasValue());
        CHECK_EQ(modules.len(), 0);
        CHECK_EQ(res.value().len(), 1);
        CHECK_EQ(res.value()[0]->name(), "a");
    }
    {
        auto res = ModuleDependencyGraph::init(compiler.allModules().value());
        CHECK(res.hasValue());
        auto graph = res.takeValue();
        CHECK_EQ(graph.layers.len(), 1);
        CHECK_EQ(graph.layers[0].len(), 1);
        CHECK_EQ(graph.layers[0][0]->name(), "a");
    }
}

TEST_CASE("ModuleDependencyGraph two modules that don't depend on each other") {
    Compiler compiler = Compiler::create().takeValue();
    CHECK(compiler.addOrGetModule("a", {}).hasValue());
    CHECK(compiler.addOrGetModule("b", {}).hasValue());
    {
        auto modules = compiler.allModules().value();
        CHECK_EQ(modules.len(), 2);
        auto res = makeFirstLayer(&modules);
        CHECK(res.hasValue());
        CHECK_EQ(modules.len(), 0);
        CHECK_EQ(res.value().len(), 2);
        CHECK_EQ(res.value()[0]->name(), "a");
        CHECK_EQ(res.value()[1]->name(), "b");
    }
    {
        auto res = ModuleDependencyGraph::init(compiler.allModules().value());
        CHECK(res.hasValue());
        auto graph = res.takeValue();
        CHECK_EQ(graph.layers.len(), 1);
        CHECK_EQ(graph.layers[0].len(), 2);
        CHECK_EQ(graph.layers[0][0]->name(), "a");
        CHECK_EQ(graph.layers[0][1]->name(), "b");
    }
}

#endif // SYNC_LIB_WITH_TESTS