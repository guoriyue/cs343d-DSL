#pragma once

#include <cstdlib>
#include <set>

#include "runtime/array.h"


// thanks StackOverflow https://stackoverflow.com/a/3767883
#define ASSERT(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message << std::endl; \
            std::terminate(); \
        } \
    } while (false)

// https://stackoverflow.com/a/28287865
std::set<uint64_t> random_sampling(const int N, const int k) {
    std::set<uint64_t> elems;
    for (int r = N - k; r < N; ++r) {
        uint64_t value = rand() % r;

        if (!elems.insert(value).second) {
            elems.insert(r);
        }
    }
    return elems;
}

// Generate a random sparse array.
array random_sparse_array(const int N, const double sparsity) {
    array A;
    A.shape = new uint64_t[1]();
    A.shape[0] = N;
    const int count = (N * sparsity);
    A.pos = new uint64_t[2]();
    A.crd = new uint64_t[count]();
    A.values = new float[count]();
    auto sample = random_sampling(N, count);
    assert(sample.size() == count);
    int i = 0;
    for (auto s : sample) {
        A.crd[i] = s;
        A.values[i] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        i++;
    }
    A.pos[0] = 0;
    A.pos[1] = count;
    return A;
}

array random_dense_array(const int N) {
    array A;
    A.shape = new uint64_t[1]();
    A.shape[0] = N;
    A.values = new float[N]();
    for (int i = 0; i < N; i++) {
        A.values[i] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
    return A;
}

array empty_dense_array(const int N) {
    array A;
    A.shape = new uint64_t[1]();
    A.shape[0] = N;
    A.values = new float[N]();
    return A;
}

void assert_dense_array_match(const array &A, const array &B, const int N) {
    for (int i = 0; i < N; i++) {
        ASSERT(A.values[i] == B.values[i], "received: " << A.values[i] << " but expected: " << B.values[i] << " at index " << i);
    }
}