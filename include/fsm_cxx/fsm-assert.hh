// fsm_cxx Library
// Copyright © 2021 Hedzr Yeh.
//
// This file is released under the terms of the MIT license.
// Read /LICENSE for more information.

//
// Created by Hedzr Yeh on 2021/9/26.
//

#ifndef __FSM_CXX_FSM_ASSERT_HH
#define __FSM_CXX_FSM_ASSERT_HH

#ifndef OS_WIN
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#ifndef OS_WIN
#define OS_WIN 1
#endif
#else
#define OS_WIN 0
#endif
#endif

#if !OS_WIN
#include <execinfo.h>
#endif

#ifdef __GNUG__
#include <cstdlib>
#include <cxxabi.h>
#include <memory>
#include <utility>
#endif

#ifdef __GNUG__
#include <signal.h>
#include <unistd.h>
#endif

#include <cstdio>
#include <iostream>

#ifndef __FUNCTION_NAME__
#ifdef __clang__
#define __FUNCTION_NAME__ __PRETTY_FUNCTION__
#elif defined(__GNUC__)
#define __FUNCTION_NAME__ __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define __FUNCTION_NAME__ __FUNCSIG__
#endif
#endif

#ifndef __PRINT_IF_FALSE
#define __PRINT_IF_FALSE
// for C++ assert:
//    assert(print_if_false(a==b, "want a equal to b"));
//
inline bool print_if_false(const bool assertion, const char *msg) {
  if (!assertion) {
    std::cerr << msg << '\n';
  }
  return assertion;
}
inline bool print_if_false(const bool assertion, const std::string &msg) { return print_if_false(assertion, msg.c_str()); }
#endif

#ifndef assertm
// assertm(a == b, "want a equal to b");
#ifdef _DEBUG
#define assertm(expr, msg) \
  __M_Assert(#expr, expr, __FILE__, __LINE__, __FUNCTION_NAME__, msg)
inline void __M_Assert(const char *expr_str, bool expr, const char *file, int line, const char *func, const char *msg) {
  if (!expr) {
    std::cerr << std::setfill(' ')
              << std::setw(19) << "Assert failed : " << msg << "\n"
              << std::setw(19) << "Expected : " << expr_str << "\n"
              << std::setw(19) << "Source : " << func << " at " << file << ':' << line << "\n";
    std::abort();
  }
}
inline void __M_Assert(const char *expr_str, bool expr,
                       const char *file, int line, const char *func,
                       const std::string &msg) { __M_Assert(expr_str, expr, file, line, func, msg.c_str()); }
#else
#define assertm(expr, msg) (void) 9
#endif
#endif

#endif // __FSM_CXX_FSM_ASSERT_HH
