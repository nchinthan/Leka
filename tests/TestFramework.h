#pragma once
// Minimal dependency-free test harness (keeps third-party dependencies at zero).
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace tt {

struct Case { const char* suite; const char* name; void (*fn)(); };
inline std::vector<Case>& registry() { static std::vector<Case> r; return r; }
inline int& failures() { static int f = 0; return f; }
struct Registrar { Registrar(const char* s, const char* n, void (*f)()) { registry().push_back({s, n, f}); } };

template <class T> std::string show(const T& v) { std::ostringstream o; o << v; return o.str(); }
inline std::string show(const std::string& v) { return "\"" + v + "\""; }
inline std::string show(bool v) { return v ? "true" : "false"; }

inline void fail(const char* file, int line, const std::string& msg) {
    ++failures();
    std::printf("    FAIL %s:%d  %s\n", file, line, msg.c_str());
}

} // namespace tt

#define TEST(suite, name)                                                         \
    static void suite##_##name();                                                 \
    static ::tt::Registrar reg_##suite##_##name(#suite, #name, &suite##_##name);  \
    static void suite##_##name()

#define CHECK(cond) do { if (!(cond)) ::tt::fail(__FILE__, __LINE__, #cond); } while (0)
#define CHECK_EQ(a, b) do { auto&& va_ = (a); auto&& vb_ = (b); if (!(va_ == vb_)) \
    ::tt::fail(__FILE__, __LINE__, std::string(#a " == " #b "  (got ") + ::tt::show(va_) + " vs " + ::tt::show(vb_) + ")"); } while (0)
