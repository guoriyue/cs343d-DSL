#include "runtime/array.h"


void kernel(array &A, array &B, array &C) {
uint64_t A_i = 0;
uint64_t B_i = 0;
uint64_t C_i = 0;
while ((B_i < B.shape[0]) && (C_i < C.shape[0])) {
  if () {
    A.values[i] = (B.values[i] * C.values[i]);
  }
}

}

#include <cstdint>
#include <iostream>
#include <cstdlib>

#include "utils.h"


void reference(array &A, const array &B, const array &C) {
    for (uint32_t i = 0; i < C.shape[0]; i++) {
        A.values[i] = B.values[i] * C.values[i];
    }
}


void run_test(const int N, const double sparsity) {
    array A_kernel = empty_dense_array(N);
    array A_ref = empty_dense_array(N);
    array B = random_dense_array(N);
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
