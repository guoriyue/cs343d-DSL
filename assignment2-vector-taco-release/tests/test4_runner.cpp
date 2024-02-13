#include <cstdint>
#include <iostream>
#include <cstdlib>

#include "utils.h"


void reference(array &A, const array &B, const array &C) {
    uint64_t A_d0 = 0;
    uint64_t B_d0_iter = B.pos[0];
    uint64_t C_d0 = 0;
    while ((B_d0_iter < B.pos[1]) && (C_d0 < C.shape[0])) {
        uint64_t B_d0 = B.crd[B_d0_iter];
        uint64_t d0 = min(B_d0, C_d0);
        if ((B_d0 == d0) && (C_d0 == d0)) {
            A.values[d0] = (B.values[B_d0_iter] * C.values[d0]);
        }
        B_d0_iter += (d0 == B_d0);
        C_d0++;
    }
}


void run_test(const int N, const double sparsity) {
    array A_kernel = empty_dense_array(N);
    array A_ref = empty_dense_array(N);
    array B = random_sparse_array(N, sparsity);
    array C = random_dense_array(N);

    kernel(A_kernel, B, C);
    reference(A_ref, B, C);

    assert_dense_array_match(A_kernel, A_ref, N);

    std::cout << "Success\n";
}

int main(const int argc, const char** argv) {
    srand(0);
    run_test(10, 0.1);
    run_test(10, 0.3);
    run_test(10, 0.5);
    run_test(10, 0.7);
    run_test(10, 0.9);
}
