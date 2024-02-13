#pragma once

#include <cstdint>
#include <memory>

// An array for use in generated kernels

struct array {
    uint64_t* shape;
    // Compressed arrays will use pos/crd.
    uint64_t* pos;
    uint64_t* crd;
    // Both Compressed and Dense arrays use values.
    float* values;
};

// Used in resolving variables in codegen.
uint64_t min(const uint64_t &a, const uint64_t &b) {
    return (a > b) ? b : a;
}
