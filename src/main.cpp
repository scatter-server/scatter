#include <iostream>
#include "base/ServerStarter.h"

#ifdef ENABLE_BACKTRACE
#include <execinfo.h>

/** Print a demangled stack backtrace of the caller function to FILE* out. */
static inline void print_stacktrace(FILE *out = stderr, unsigned int max_frames = 63) {
    fprintf(out, "stack trace:\n");

    const size_t max_frames_res = max_frames + 1;
    // storage array for stack trace address data
    std::vector<void *> addrlist(max_frames_res);

    // retrieve current stack addresses
    int addrlen = backtrace(&addrlist[0], (int) max_frames_res);

    if (addrlen == 0) {
        fprintf(out, "  <empty, possibly corrupt>\n");
        return;
    }

    // resolve addresses into strings containing "filename(function+address)",
    // this array must be free()-ed
    char **symbollist = backtrace_symbols(&addrlist[0], addrlen);

    // allocate string which will be filled with the demangled function name
    size_t funcnamesize = 256;
    char *funcname = (char *) malloc(funcnamesize);

    // iterate over the returned symbol lines. skip the first, it is the
    // address of this function.
    for (int i = 1; i < addrlen; i++) {
        char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

        // find parentheses and +address offset surrounding the mangled name:
        // ./module(function+0x15c) [0x8048a6d]
        for (char *p = symbollist[i]; *p; ++p) {
            if (*p == '(')
                begin_name = p;
            else if (*p == '+')
                begin_offset = p;
            else if (*p == ')' && begin_offset) {
                end_offset = p;
                break;
            }
        }

        if (begin_name && begin_offset && end_offset
            && begin_name < begin_offset) {
            *begin_name++ = '\0';
            *begin_offset++ = '\0';
            *end_offset = '\0';

            // mangled name is now in [begin_name, begin_offset) and caller
            // offset in [begin_offset, end_offset). now apply
            // __cxa_demangle():

            int status;
            char *ret = abi::__cxa_demangle(begin_name,
                                            funcname, &funcnamesize, &status);
            if (status == 0) {
                funcname = ret; // use possibly realloc()-ed string
                fprintf(out, "  %s : %s+%s\n",
                        symbollist[i], funcname, begin_offset);
            } else {
                // demangling failed. Output function name as a C function with
                // no arguments.
                fprintf(out, "  %s : %s()+%s\n",
                        symbollist[i], begin_name, begin_offset);
            }
        } else {
            // couldn't parse the line? print the whole line.
            fprintf(out, "UNPARSED:  %s\n", symbollist[i]);
        }
    }

    free(funcname);
    free(symbollist);
}

void handler(int) {
    print_stacktrace();
    exit(1);
}
#endif

int main(int argc, char **argv) {

#ifdef ENABLE_BACKTRACE
    signal(SIGSEGV, handler);   // install segfault handler
    signal(SIGABRT, handler);   // install exception handler
#endif

    const auto **args = const_cast<const char **>(argv);
    wss::ServerStarter server(argc, args);

    if (!server.isValid()) {
        return 1;
    }

    server.run();

    return 0;
}