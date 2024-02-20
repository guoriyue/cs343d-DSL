#include "runtime/array.h"


void kernel(array &A, array &B, array &C) {
uint64_t A_i = 0;
uint64_t B_i = 0;
uint64_t C_i = 0;
while ((B_i < B.shape[0]) && (C_i < C.shape[0])) {
  uint64_t i = min(B_i, C_i);
  A.values[i] = (B.values[i] + C.values[i]);
  B_i++;
  C_i++;
}
while ((B_i < B.shape[0]) && (C_i < C.shape[0])) {
  uint64_t i = min(B_i, C_i);
  A.values[i] = (B.values[i] + C.values[i]);
  B_i++;
  C_i++;
}
while ((B_i < B.shape[0]) && (C_i < C.shape[0])) {
  uint64_t i = min(B_i, C_i);
  A.values[i] = (B.values[i] + C.values[i]);
  B_i++;
  C_i++;
}

}

