#include "runtime/array.h"


void kernel(array &A, array &B, array &C, array &D) {
uint64_t A_i = 0;
uint64_t B_i_iter = B.pos[0];
uint64_t C_i = 0;
uint64_t D_i_iter = D.pos[0];
while ((B_i_iter < B.pos[1]) && (C_i < C.shape[0]) && (D_i_iter < D.pos[1])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t D_i = D.crd[D_i_iter];
  uint64_t i = min(B_i, min(C_i, D_i));
  if ((B_i == i) && (D_i == i)) {
    A.values[i] = ((B.values[B_i_iter] + C.values[i]) * D.values[D_i_iter]);
  }
  else if ((B_i == i) && (D_i == i)) {
    A.values[i] = ((B.values[B_i_iter] + C.values[i]) * D.values[D_i_iter]);
  }
  else if ((D_i == i)) {
    A.values[i] = ((B.values[B_i_iter] + C.values[i]) * D.values[D_i_iter]);
  }
  B_i_iter += (i == B_i);
  C_i++;
  D_i_iter += (i == D_i);
}
while ((B_i_iter < B.pos[1]) && (C_i < C.shape[0]) && (D_i_iter < D.pos[1])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t D_i = D.crd[D_i_iter];
  uint64_t i = min(B_i, min(C_i, D_i));
  if ((B_i == i) && (D_i == i)) {
    A.values[i] = ((B.values[B_i_iter] + C.values[i]) * D.values[D_i_iter]);
  }
  else if ((B_i == i) && (D_i == i)) {
    A.values[i] = ((B.values[B_i_iter] + C.values[i]) * D.values[D_i_iter]);
  }
  else if ((D_i == i)) {
    A.values[i] = ((B.values[B_i_iter] + C.values[i]) * D.values[D_i_iter]);
  }
  B_i_iter += (i == B_i);
  C_i++;
  D_i_iter += (i == D_i);
}
while ((B_i_iter < B.pos[1]) && (C_i < C.shape[0]) && (D_i_iter < D.pos[1])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t D_i = D.crd[D_i_iter];
  uint64_t i = min(B_i, min(C_i, D_i));
  if ((B_i == i) && (D_i == i)) {
    A.values[i] = ((B.values[B_i_iter] + C.values[i]) * D.values[D_i_iter]);
  }
  else if ((B_i == i) && (D_i == i)) {
    A.values[i] = ((B.values[B_i_iter] + C.values[i]) * D.values[D_i_iter]);
  }
  else if ((D_i == i)) {
    A.values[i] = ((B.values[B_i_iter] + C.values[i]) * D.values[D_i_iter]);
  }
  B_i_iter += (i == B_i);
  C_i++;
  D_i_iter += (i == D_i);
}

}

#include <cstdint>
#include <iostream>
#include <cstdlib>

#include "utils.h"


void reference(array &A, const array &B, const array &C, const array &D) {
  uint32_t iB = B.pos[0];
  uint32_t pB_end = B.pos[1];
  uint32_t iD = D.pos[0];
  uint32_t pD_end = D.pos[1];

  while (iB < pB_end && iD < pD_end) {
    uint32_t iB0 = B.crd[iB];
    uint32_t iD0 = D.crd[iD];
    uint32_t i = min(iB0, iD0);
    if (iB0 == i && iD0 == i) {
      A.values[i] = (B.values[iB] + C.values[i]) * D.values[iD];
    }
    else if (iD0 == i) {
      A.values[i] = C.values[i] * D.values[iD];
    }
    iB += (uint32_t)(iB0 == i);
    iD += (uint32_t)(iD0 == i);
  }
  while (iD < pD_end) {
    uint32_t i = D.crd[iD];
    A.values[i] = C.values[i] * D.values[iD];
    iD++;
  }
}


void run_test(const int N, const double sparsity) {
    array A_kernel = empty_dense_array(N);
    array A_ref = empty_dense_array(N);
    array B = random_sparse_array(N, sparsity);
    array C = random_dense_array(N);
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
