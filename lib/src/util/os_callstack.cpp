#include "os_callstack.hpp"
#include "assert.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cstring>
#include <charconv>
#include <string_view>

// Reasonable default
constexpr int defaultBacktraceDepth = 32;

struct BacktraceAddresses {
    void** addresses = nullptr;
    int capacity = 0;

    void alloc(const int inCapacity) {
        if(this->addresses != nullptr) {
            free(this->addresses);
        }
        this->addresses = reinterpret_cast<void**>(malloc(sizeof(void*) * capacity));
        this->capacity = inCapacity;
    }
};

Backtrace::StackFrameInfo Backtrace::StackFrameInfo::parse(const char* const buffer)
{
    Backtrace::StackFrameInfo self;

    #ifdef __APPLE__
    const size_t slen = std::strlen(buffer) - 1; // ends with null terminator

    const char* endOfFileName = nullptr;

    { // extract line number
        const char* start = &buffer[slen - 1];
        while(!isdigit(*start)) {
            start--; // any trailing characters like ')'
        }
        
        const char* const end = start;
        while(isdigit(*start)) {
            start--;
        }
        std::string_view sv{&start[1], static_cast<size_t>(end - start)};
        auto result = std::from_chars(sv.data(), sv.data() + sv.size(), self.lineNumber);
        sy_assert(result.ptr == (sv.data() + sv.size()), "Couldn't parse for some reason");
        endOfFileName = start; // include ':' character that seperates file and line name
    }

    size_t i = 0;
    { // extract function name
        bool foundStartParenthesis = false;
        bool foundEndParenthesis = false;
        for(; i < slen; i++) {
            if(buffer[i] == '(') {
                foundStartParenthesis = true;
            }
            else if(buffer[i] == ')' && foundStartParenthesis) {
                foundEndParenthesis = true;
            }
            // either both parenthesis were found, or neither
            else if(buffer[i] == ' ' && (foundStartParenthesis == foundEndParenthesis)) {
                break;
            }
        }

        std::string_view sv{buffer, i};
        self.functionName = sv;
    }

    i += std::strlen(" (in ");

    { // extract object name
        const size_t start = i;
        for(; i < slen; i++) {
            if(buffer[i] == ')') {
                break;
            }
        }

        std::string_view sv{buffer + start, i - start};
        self.obj = sv;
    }

    { // extract full file path
        i += std::strlen(") ");
        if(buffer[i] != '+') {
            i += 1; // '(' character
            const size_t start = i;
            const size_t len = static_cast<size_t>(endOfFileName - &buffer[start]);
            std::string_view sv{buffer + start, len};
            self.fullFilePath = sv;
        }
    }
    #endif // __APPLE__
    return self;
}

#ifdef __APPLE__
#include <dlfcn.h>
#include <execinfo.h>
#include <stdio.h>

Backtrace Backtrace::getBacktrace()
{
    BacktraceAddresses btAddr;

    // https://stackoverflow.com/a/78676617
    // https://www.manpagez.com/man/1/atos/osx-10.12.6.php
    
    int trace_size = 0;
    while(btAddr.capacity == trace_size || btAddr.addresses == nullptr) {
        btAddr.alloc(btAddr.capacity + defaultBacktraceDepth);
        trace_size = backtrace(btAddr.addresses, btAddr.capacity);
    }

    Backtrace self;

    // We do't care about this function being called
    // TODO what about the OS start? Do we need that?
    for (int i = 1; i < trace_size; ++i) {
        Dl_info info;
        dladdr(btAddr.addresses[i], &info);

        std::stringstream cmd(std::ios_base::out);
        cmd << "atos -o " << info.dli_fname << " -l " << std::hex
        << reinterpret_cast<uint64_t>(info.dli_fbase) << ' '
        << reinterpret_cast<uint64_t>(btAddr.addresses[i])
        << " -fullPath";

        FILE* atos = popen(cmd.str().c_str(), "r");

        constexpr int kBufferSize = 200;
        char buffer[kBufferSize];

        fgets(buffer, kBufferSize, atos);
        pclose(atos);

        self.frames.push_back(Backtrace::StackFrameInfo::parse(buffer));
    }

    return self;
}

#endif // __APPLE__

#if SYNC_LIB_TEST

#include "../doctest.h"
#include <iostream>

TEST_CASE("back trace example") {
    Backtrace bt = Backtrace::getBacktrace();
    for(size_t i = 0; i < bt.frames.size(); i++) {
        auto& frame = bt.frames[i];
        std::cout << frame.obj << " | " << frame.functionName << " | " << frame.fullFilePath << ':' << frame.lineNumber << std::endl;
    }


    //print_backtrace(nullptr, 0);
    // void* buf[1000];
    // const int bt = backtrace(buf, 1000);
    // std::cerr << "back trace returned " << bt << " addresses\n";

    // char** strings = backtrace_symbols(buf, bt);
    // for(int i = 1; i < bt; i++) {
    //     std::cerr << strings[i] << std::endl;
    //     #ifdef __APPLE__

    //     #endif

    //     // char syscom[256];
    //     // (void)sprintf(syscom,"addr2line %p -e sighandler", buf[i]); //last parameter is the name of this app
    //     // (void)system(syscom);
    // }

    // // https://stackoverflow.com/a/15130037


    // std::cerr << "hello\n"; 
}

#endif
