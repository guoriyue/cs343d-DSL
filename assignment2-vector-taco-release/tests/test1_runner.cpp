#include <cstdint>
#include <iostream>
#include <cstdlib>

#include "utils.h"


void reference(array &A, const array &B, const array &C, const array &D) {
  uint32_t iB = B.pos[0];
  uint32_t pB1_end = B.pos[1];
  uint32_t iC = C.pos[0];
  uint32_t pC1_end = C.pos[1];
  uint32_t iD = D.pos[0];
  uint32_t pD1_end = D.pos[1];

  while ((iB < pB1_end && iC < pC1_end) && iD < pD1_end) {
    uint32_t iB0 = B.crd[iB];
    uint32_t iC0 = C.crd[iC];
    uint32_t iD0 = D.crd[iD];
    uint32_t i = min(iB0, min(iC0, iD0));
    if ((iB0 == i && iC0 == i) && iD0 == i) {
      A.values[i] = B.values[iB] + C.values[iC] * D.values[iD];
    }
    else if (iC0 == i && iD0 == i) {
      A.values[i] = C.values[iC] * D.values[iD];
    }
    else if (iB0 == i) {
      A.values[i] = B.values[iB];
    }
    iB += (uint32_t)(iB0 == i);
    iC += (uint32_t)(iC0 == i);
    iD += (uint32_t)(iD0 == i);
  }
  while (iC < pC1_end && iD < pD1_end) {
    uint32_t iC0 = C.crd[iC];
    uint32_t iD0 = D.crd[iD];
    uint32_t i = min(iC0,iD0);
    if (iC0 == i && iD0 == i) {
      A.values[i] = C.values[iC] * D.values[iD];
    }
    iC += (uint32_t)(iC0 == i);
    iD += (uint32_t)(iD0 == i);
  }
  while (iB < pB1_end) {
    uint32_t i = B.crd[iB];
    A.values[i] = B.values[iB];
    iB++;
  }
}


void run_test(const int N, const double sparsity) {
    array A_kernel = empty_dense_array(N);
    array A_ref = empty_dense_array(N);
    array B = random_sparse_array(N, sparsity);
    array C = random_sparse_array(N, sparsity);
    array D = random_sparse_array(N, sparsity);

    kernel(A_kernel, B, C, D);
    reference(A_ref, B, C, D);

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
