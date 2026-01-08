#include "module_dependency_graph.hpp"
#include "../../core/core_internal.h"
#include "../../types/hash/map.hpp"
#include "../compiler.hpp"
#include <iostream>

using namespace sy;

/*
Circular dependency

If module A depends on B and B depends on A, this is an error

TODO prevent circular dependencies
*/

static Result<DynArray<const Module*>, ProgramError> makeFirstLayer(DynArray<const Module*>* modules) {
    DynArray<const Module*> nodes{modules->alloc()};
    size_t i = 0;
    while (i < modules->len()) {
        if ((*modules)[i]->dependencies().len() == 0) {
            if (nodes.push((*modules)[i]).hasErr()) {
                return Error(ProgramError::OutOfMemory);
            }
            modules->removeAt(i);
        } else {
            i++;
        }
    }

    if (nodes.len() == 0) {
        return Error(ProgramError::CompileModuleDependencyGraph);
    }

    return nodes;
}

static bool isModuleInLayers(const DynArray<DynArray<const Module*>>& layers, const Module* mod) noexcept {
    for (const auto& layer : layers) {
        for (const auto& layerEntry : layer) {
            if (layerEntry == mod) {
                return true;
            }
#ifndef NDEBUG
            const bool sameName = layerEntry->name() == mod->name();
            const bool sameVersion = layerEntry->version() == mod->version();
            if (sameName && sameVersion) {
                sy_assert(false, "Same module but different addresses");
            }
#endif
        }
    }
    return false;
}

static bool allDependenciesInLayers(const DynArray<DynArray<const Module*>>& layers, const Module* mod) noexcept {
    for (const auto dependencyEntry : mod->dependencies()) {
        const Module* dependency = dependencyEntry.value;
        if (isModuleInLayers(layers, dependency) == false) {
            return false;
        }
    }
    return true;
}

Result<ModuleDependencyGraph, ProgramError> sy::ModuleDependencyGraph::init(DynArray<const Module*> modules) noexcept {
    ModuleDependencyGraph self{};

    { // first layer
        auto res = makeFirstLayer(&modules);
        if (res.hasErr()) {
            return Error(res.takeErr());
        }
        if (self.layers.push(res.takeValue()).hasErr()) {
            return Error(ProgramError::OutOfMemory);
        }
    }

    while (modules.len() > 0) {
        DynArray<const Module*> newLayers(modules.alloc());
        size_t i = 0;
        while (i < modules.len()) {
            const Module* mod = modules[i];
            if (allDependenciesInLayers(self.layers, mod)) {
                if (newLayers.push(mod).hasErr()) {
                    return Error(ProgramError::OutOfMemory);
                }
                modules.removeAt(i);
            } else {
                i++;
            }
        }
        if (newLayers.len() == 0)
            return Error(ProgramError::CompileModuleDependencyGraph);
        ;

        if (self.layers.push(std::move(newLayers)).hasErr())
            return Error(ProgramError::OutOfMemory);
    }

    return self;
}

bool sy::ModuleDependencyGraph::Iterator::operator!=(const Iterator& other) noexcept {
    return (this->layer != other.layer) || (this->index != other.index);
}

const Module* sy::ModuleDependencyGraph::Iterator::operator*() const noexcept {
    return (*this->refLayers)[this->layer][this->index];
}

sy::ModuleDependencyGraph::Iterator& sy::ModuleDependencyGraph::Iterator::operator++() noexcept {
    auto& currLayer = (*this->refLayers)[this->layer];
    if ((this->index + 1) >= currLayer.len()) {
        this->layer += 1;
        this->index = 0;
    } else {
        this->index += 1;
    }
    return *this;
}

sy::ModuleDependencyGraph::Iterator sy::ModuleDependencyGraph::begin() const noexcept {
    return Iterator{&this->layers, 0, 0};
}

sy::ModuleDependencyGraph::Iterator sy::ModuleDependencyGraph::end() const noexcept {
    return Iterator{&this->layers, this->layers.len(), 0};
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
        CHECK_EQ(res.err(), ProgramError::CompileModuleDependencyGraph);
    }
    {
        auto modules = compiler.allModules().value();
        auto res = ModuleDependencyGraph::init(compiler.allModules().value());
        CHECK(res.hasErr());
        CHECK_EQ(res.err(), ProgramError::CompileModuleDependencyGraph);
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