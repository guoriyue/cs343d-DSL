#include "runtime/array.h"


void kernel(array &A, array &B, array &C, array &D) {
uint64_t A_i = 0;
uint64_t B_i_iter = B.pos[0];
uint64_t C_i_iter = C.pos[0];
uint64_t D_i_iter = D.pos[0];
while ((B_i_iter < B.pos[1]) && (C_i_iter < C.pos[1]) && (D_i_iter < D.pos[1])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t C_i = C.crd[C_i_iter];
  uint64_t D_i = D.crd[D_i_iter];
  uint64_t i = min(B_i, min(C_i, D_i));
  if ((B_i == i) && (C_i == i) && (D_i == i)) {
    A.values[i] = (B.values[B_i_iter] + (C.values[C_i_iter] * D.values[D_i_iter]));
  }
  else if ((B_i == i)) {
    A.values[i] = (B.values[B_i_iter] + (C.values[C_i_iter] * D.values[D_i_iter]));
  }
  else if ((C_i == i) && (D_i == i)) {
    A.values[i] = (B.values[B_i_iter] + (C.values[C_i_iter] * D.values[D_i_iter]));
  }
  B_i_iter += (i == B_i);
  C_i_iter += (i == C_i);
  D_i_iter += (i == D_i);
}
while ((B_i_iter < B.pos[1]) && (C_i_iter < C.pos[1]) && (D_i_iter < D.pos[1])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t C_i = C.crd[C_i_iter];
  uint64_t D_i = D.crd[D_i_iter];
  uint64_t i = min(B_i, min(C_i, D_i));
  if ((B_i == i) && (C_i == i) && (D_i == i)) {
    A.values[i] = (B.values[B_i_iter] + (C.values[C_i_iter] * D.values[D_i_iter]));
  }
  else if ((B_i == i)) {
    A.values[i] = (B.values[B_i_iter] + (C.values[C_i_iter] * D.values[D_i_iter]));
  }
  else if ((C_i == i) && (D_i == i)) {
    A.values[i] = (B.values[B_i_iter] + (C.values[C_i_iter] * D.values[D_i_iter]));
  }
  B_i_iter += (i == B_i);
  C_i_iter += (i == C_i);
  D_i_iter += (i == D_i);
}
while ((B_i_iter < B.pos[1]) && (C_i_iter < C.pos[1]) && (D_i_iter < D.pos[1])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t C_i = C.crd[C_i_iter];
  uint64_t D_i = D.crd[D_i_iter];
  uint64_t i = min(B_i, min(C_i, D_i));
  if ((B_i == i) && (C_i == i) && (D_i == i)) {
    A.values[i] = (B.values[B_i_iter] + (C.values[C_i_iter] * D.values[D_i_iter]));
  }
  else if ((B_i == i)) {
    A.values[i] = (B.values[B_i_iter] + (C.values[C_i_iter] * D.values[D_i_iter]));
  }
  else if ((C_i == i) && (D_i == i)) {
    A.values[i] = (B.values[B_i_iter] + (C.values[C_i_iter] * D.values[D_i_iter]));
  }
  B_i_iter += (i == B_i);
  C_i_iter += (i == C_i);
  D_i_iter += (i == D_i);
}

}

