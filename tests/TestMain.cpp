#include "TestFramework.h"

#include <cstring>

int main(int argc, char** argv) {
    const char* filter = argc > 1 ? argv[1] : nullptr;
    int ran = 0, failedCases = 0;
    for (const auto& c : tt::registry()) {
        if (filter && std::strcmp(filter, c.suite) != 0) continue;
        const int before = tt::failures();
        c.fn();
        ++ran;
        const bool bad = tt::failures() != before;
        if (bad) ++failedCases;
        std::printf("[%s] %s.%s\n", bad ? "FAIL" : " ok ", c.suite, c.name);
    }
    std::printf("\n%d tests, %d failed\n", ran, failedCases);
    return (failedCases == 0 && ran > 0) ? 0 : 1;
}
