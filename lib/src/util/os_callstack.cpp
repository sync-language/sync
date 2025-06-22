#include "os_callstack.hpp"
#include "assert.hpp"
#include "../core.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <cstring>
#include <charconv>
#include <string_view>
#include <string>
#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <process.h>
#include <Windows.h>
#include <dbghelp.h>
#elif defined __APPLE || defined __GNUC__
#include <dlfcn.h>
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
#endif // MSVC APPLE GCC

#if defined __APPLE || defined __GNUC__

// Reasonable default
constexpr int defaultBacktraceDepth = 64;

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

#if defined __APPLE
static Backtrace::StackFrameInfo parseStackFrameInfo(const char* const buffer)
{
    Backtrace::StackFrameInfo self{};

    std::cout << buffer << std::endl;

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
#endif

#if defined __GNUC__ && !defined(__APPLE)
static Backtrace::StackFrameInfo addr2lineInfo(void* const address, const char* const message) {
    // Dl_info info;
    // dladdr(address, &info);

    size_t exeNameLen = 0;
    while(message[exeNameLen] != '(' && message[exeNameLen] != ' ' && message[exeNameLen] != '\0')
        exeNameLen += 1;

    const char* const exeOffsetAddr = &message[exeNameLen + std::strlen("(+")];
    size_t exeOffsetLen = 0;
    while(exeOffsetAddr[exeOffsetLen] != ')')
        exeOffsetLen += 1;

    std::stringstream cmd(std::ios_base::out);
    cmd << "addr2line ";
    cmd.write(exeOffsetAddr, exeOffsetLen);
    cmd << " -e ";
    cmd.write(message, exeNameLen);

    FILE* addr2line = popen(cmd.str().c_str(), "r");

    constexpr int kBufferSize = 256;
    char buffer[kBufferSize];

    fgets(buffer, kBufferSize, addr2line);
    pclose(addr2line);

    Backtrace::StackFrameInfo frameInfo{};
    frameInfo.address = address;
    frameInfo.obj = [&]() -> std::string {
        size_t i = exeNameLen;
        while(message[i] != '/') {
            i -= 1;
        }
        return std::string(&message[i + 1], exeNameLen - i - 1);
    }();

    if(std::strcmp(buffer, "??:?\n") != 0) { // line and file
        const size_t slen = std::strlen(buffer) - 1; // ends with null terminator

        const char* start = &buffer[slen - 1];
        {
            const std::string_view fullBufferSv{buffer, slen};
            const size_t found = fullBufferSv.rfind(" (discriminator");
            if(found != std::string_view::npos) {
                start = &buffer[found];
            }
        }

        while(!isdigit(*start)) {
            start--; // any trailing characters like ')'
        }
        
        const char* const end = start;
        while(isdigit(*start)) {
            start--;
        }
        std::string_view sv{&start[1], static_cast<size_t>(end - start)};
        auto result = std::from_chars(sv.data(), sv.data() + sv.size(), frameInfo.lineNumber);
        sy_assert(result.ptr == (sv.data() + sv.size()), "Couldn't parse for some reason");
        const char* endOfFileName = start; // include ':' character that seperates file and line name

        frameInfo.fullFilePath = std::string(buffer, static_cast<size_t>(endOfFileName - buffer));
    } else {
        frameInfo.fullFilePath = "??";
    }

    cmd << " -f -C"; // function names and demangle

    addr2line = popen(cmd.str().c_str(), "r");

    fgets(buffer, kBufferSize, addr2line);
    pclose(addr2line);

    frameInfo.functionName = std::string(buffer, std::strlen(buffer) - 1); // includes new line

    return frameInfo;
}
#endif

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

    #if __APPLE

    // We don't care about this function being called
    for (int i = 1; i < trace_size; ++i) {
        Dl_info info;
        dladdr(btAddr.addresses[i], &info);

        std::stringstream cmd(std::ios_base::out);
        cmd << "atos -o " << info.dli_fname << " -l " << std::hex
        << reinterpret_cast<uint64_t>(info.dli_fbase) << ' '
        << reinterpret_cast<uint64_t>(btAddr.addresses[i])
        << " -fullPath";

        FILE* atos = popen(cmd.str().c_str(), "r");

        constexpr int kBufferSize = 256;
        char buffer[kBufferSize];

        fgets(buffer, kBufferSize, atos);
        pclose(atos);

        self.frames.push_back(parseStackFrameInfo(buffer));
        self.frames[self.frames.size() - 1].address = btAddr.addresses[i];
    }

    #elif __GNUC__

    char **messages = (char **)NULL;
    messages = backtrace_symbols(btAddr.addresses, trace_size);

    // We don't care about this function being called
    for(int i = 1; i < trace_size; i++) {
        self.frames.push_back(addr2lineInfo(btAddr.addresses, messages[i]));
    }  

    #endif

    return self;
}

#endif // defined __APPLE || defined __GNUC__

#ifdef _MSC_VER

#pragma comment(lib,"Dbghelp.lib")

// TODO maybe get function signature too?

Backtrace Backtrace::getBacktrace()
{
    // https://stackoverflow.com/a/50208684

    BOOL    result;
    HANDLE  process;
    HANDLE  thread;
    HMODULE hModule;

    STACKFRAME64        stack;
    ULONG               frame;    
    DWORD64             displacement;

    DWORD disp;
    IMAGEHLP_LINE64 *line;

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    char module[1024];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    // On x64, StackWalk64 modifies the context record, that could
    // cause crashes, so we create a copy to prevent it
    CONTEXT ctxCopy;
    RtlCaptureContext(&ctxCopy);

    memset( &stack, 0, sizeof( STACKFRAME64 ) );

    process                = GetCurrentProcess();
    thread                 = GetCurrentThread();
    displacement           = 0;
#if !defined(_M_AMD64)
    stack.AddrPC.Offset    = (*ctx).Eip;
    stack.AddrPC.Mode      = AddrModeFlat;
    stack.AddrStack.Offset = (*ctx).Esp;
    stack.AddrStack.Mode   = AddrModeFlat;
    stack.AddrFrame.Offset = (*ctx).Ebp;
    stack.AddrFrame.Mode   = AddrModeFlat;
#endif

    Backtrace self;

    SymInitialize( process, NULL, TRUE ); //load symbols

    // We don't care about this function being called
    (void)StackWalk64(
#if defined(_M_AMD64)
        IMAGE_FILE_MACHINE_AMD64,
#else
        IMAGE_FILE_MACHINE_I386,
#endif
        process,
        thread,
        &stack,
        &ctxCopy,
        NULL,
        SymFunctionTableAccess64,
        SymGetModuleBase64,
        NULL
    );
    
    for( frame = 0; ; frame++ )
    {
        //get next call from stack
        result = StackWalk64(
#if defined(_M_AMD64)
            IMAGE_FILE_MACHINE_AMD64,
#else
            IMAGE_FILE_MACHINE_I386,
#endif
            process,
            thread,
            &stack,
            &ctxCopy,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
        );

        if( !result ) break;        

        //get symbol name for address
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MAX_SYM_NAME;
        SymFromAddr(process, ( ULONG64 )stack.AddrPC.Offset, &displacement, pSymbol);

        line = (IMAGEHLP_LINE64 *)malloc(sizeof(IMAGEHLP_LINE64));
        line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);       

        //try to get line
        if (SymGetLineFromAddr64(process, stack.AddrPC.Offset, &disp, line))
        {
            hModule = NULL;
            lstrcpyA(module,""); 
            GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, 
                (LPCTSTR)(stack.AddrPC.Offset), &hModule);
            if(hModule != NULL)GetModuleFileNameA(hModule,module, 1024);

            const size_t moduleNameLen = std::strlen(module);
            const char* moduleName = &module[moduleNameLen - 1];
            while(*moduleName != '\\') moduleName--;

            Backtrace::StackFrameInfo info{
                &moduleName[1],
                pSymbol->Name,
                line->FileName,
                static_cast<int>(line->LineNumber),
                reinterpret_cast<void*>(pSymbol->Address)
            };
            self.frames.push_back(std::move(info));
        }
        else
        { 
            //failed to get line
            // printf("\tat %s, address 0x%0X.\n", pSymbol->Name, pSymbol->Address);
            // hModule = NULL;
            // lstrcpyA(module,"");        
            // GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, 
            //     (LPCTSTR)(stack.AddrPC.Offset), &hModule);

            // //at least print module name
            // if(hModule != NULL)GetModuleFileNameA(hModule,module, 1024);       

            // printf ("in %s\n",module);
        }       

        free(line);
        line = NULL;
    }

    return self;
}

#endif // _MSC_VER

#if SYNC_LIB_TEST

#include "../doctest.h"
#include <iostream>

template<typename T>
struct Example {
    void doThing() {
        Backtrace bt = Backtrace::getBacktrace();
        for(size_t i = 0; i < bt.frames.size(); i++) {
            auto& frame = bt.frames[i];
            std::cout << frame.obj << " | " << frame.functionName << " | " << frame.fullFilePath << ':' << frame.lineNumber << std::endl;
        }
    }
};

TEST_CASE("back trace example") {
    // Backtrace bt = Backtrace::getBacktrace();
    // for(size_t i = 0; i < bt.frames.size(); i++) {
    //     auto& frame = bt.frames[i];
    //     std::cout << frame.obj << " | " << frame.functionName << " | " << frame.fullFilePath << ':' << frame.lineNumber << std::endl;
    // }
    Example<int> e;
    e.doThing();

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
