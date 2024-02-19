#include "runtime/array.h"


void kernel(array &A, array &B, array &C) {
uint64_t A_i = 0;
uint64_t B_i_iter = B.pos[0];
uint64_t C_i_iter = C.pos[0];
while ((B_i_iter < B.pos[1]) && (C_i_iter < C.pos[1])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t C_i = C.crd[C_i_iter];
  uint64_t i = min(B_i, C_i);
  if ((B_i == i) && (C_i == i)) {
    A.values[i] = (B.values[B_i_iter] * C.values[C_i_iter]);
  }
  B_i_iter += (i == B_i);
  C_i_iter += (i == C_i);
}

}

