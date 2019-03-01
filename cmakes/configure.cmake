include(CheckIncludeFileCXX)
include(CheckCXXSymbolExists)

string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
if ("${BUILD_TYPE}" STREQUAL "debug")
	check_include_file_cxx("execinfo.h" HAVE_EXECINFO_H)
	check_include_file_cxx("cxxabi.h" HAVE_CXXABI_H)
	check_cxx_symbol_exists("backtrace" "execinfo.h" HAVE_BACKTRACE_SYM)
	check_cxx_symbol_exists("backtrace_symbols" "execinfo.h" HAVE_BACKTRACE_SYMBOLS_SYM)
	check_cxx_symbol_exists("backtrace_symbols_fd" "execinfo.h" HAVE_BACKTRACE_SYMBOLS_FD_SYM)
	check_cxx_symbol_exists("abi::__cxa_demangle" "cxxabi.h" HAVE_DEMANGLER_SYM)

	if (HAVE_EXECINFO_H AND HAVE_BACKTRACE_SYM AND HAVE_BACKTRACE_SYMBOLS_FD_SYM AND HAVE_DEMANGLER_SYM AND HAVE_BACKTRACE_SYMBOLS_SYM)
		add_definitions(-DENABLE_BACKTRACE=1)
	endif ()
endif ()

#check_include_file_cxx("string_view" HAS_STRING_VIEW)
#unset(HAS_STRING_VIEW)


option(ENABLE_REDIS_TARGET "Enables redis target in event notifier" OFF)