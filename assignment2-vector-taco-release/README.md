# Vector TACO

In this assignment, you will implement a simplified version of TACO for single-dimension dense or compressed arrays. We provide scaffolding for you, so you only need to implement two minor stages of lowering from TACO front-end code down to a C-like IR that prints directly to C. You will need to implement merge lattice construction. We provide more details below, but if anything is not answered here, please post on Ed, or email AJ (CA).

**Please note that you do not need to use any of the provided infrastructure.** As long as our tests pass(without modification to `run_test.sh` or any of the files in `tests/`) and you have indeed implemented merge lattice construction sufficiently, you will receive full points. We provide the scaffolding such that students do not need to spend time on IR design and printing to C, but you do not have to use it.


## Assignment

You are required to implement lowering from single-dimension TACO front-end code down to a C-like IR custom-designed for implementing sparse co-iteration. With the provided infrastructure, this corresponds to adding code to `Lower.cpp`, `MergeLattice.h`, and `MergeLattice.cpp`. However, as noted above, you are free to make changes to any files in the `src/` or `include/` directories, and are not required to use our scaffolding.

The [original TACO paper](https://fredrikbk.com/publications/taco.pdf) will be quite a useful resource for this assignment. More details are provided in [Fred's thesis](https://fredrikbk.com/publications/kjolstad-thesis.pdf). The [TACO web tool](http://tensor-compiler.org/codegen.html) can be quite useful for checking your implementation against a reference compiler.

Each of the 6 tests are taken from Figure 10 in the original TACO paper, that figure may help with debugging conceptually.

Each of the methods that need to be implemented are marked with `// STUDENTS TODO:`.


### Input Grammar

We provide a small embedded front-end for TACO in `Expr.h` (more details below). Your compiler needs to support both vector addition and vector multiplication (element-wise). The main object of compilation is an `Assignment` node (see `Array.h` for its definition). This object is constructed when assigning to an `Array` object (also defined in `Array.h`). See the sample program below:

```
Index i{"i"};
Array A{"A"}, B{"B"}, C{"C"};
Assignment a = (A(i) = B(i) * C(i));
std::cout << a << "\n";
```

The expected output is almost exactly the expression above:
```
A(i) = (B(i) * C(i))
```

Our scaffolding provides two IRs for lowering: a simplified version of Concrete Index Notation in `IndexStmt.h` (described in Fred's thesis, not the original paper), and a C-like IR designed to make it easy to generate co-iteration code, in `LIR.h` ("Lowered IR"). Note that `LIR.h` uses a different namespace `LIR`, which requires you to prefix calls to methods defined in `LIR.h` with a `LIR::`. This is to avoid naming conflicts with the front-end expression code in `Expr.h`.


### Concrete Index Notation

CIN as described in Fred's thesis is far more complicated than is necessary for this assigmnet, which only requires two types of statements - `ForAll` loops and `Assignment` nodes. Lowering to CIN should be somewhat-straight-forward, the real challenge is in generating the set expressions that `ForAll` loops iterate over. In the example above, we would lower to the following CIN:

```
std::cout << lower(a) << "\n";
```

```
∀ (B(i) ∩ C(i)) {
  A(i) = (B(i) * C(i))
}
```

Note that CIN is format-agnostic. That means we can lower to CIN without needing to know what formats are used for any of the corresponding arrays.


### Lowered IR (LIR)

As noted above, we provide a small C-like IR that can be used to implement co-iteration in `LIR.h`. We provide the printing to C interface in `IRPrinter.h` (this contains printing for the front-end code as well as the two IRs).

In our example, the merge lattice has a single point: the intersection of `B` and `C`. This makes code generation quite simple. Let `B` be a `Compressed` array and `C` be a dense array.

We would lower the example to a `LIR::SequenceStmt` that first defines iterators for each of the three arrays via 3 `LIR::IteratorDefinition`s, followed by a `LIR::WhileStmt` over the B and C iterators.
```
uint64_t A_i = 0;
uint64_t B_i_iter = B.pos[0];
uint64_t C_i = 0;
while ((B_i_iter < B.pos[1]) && (C_i < C.shape[0])) {
  ...
}
```
The body of the `LIR::WhileStmt` needs to derive the logical index of `B` by extracting it from `B.crd`, which is done with a `LIR::CompressedIndexDefinition`. Next, the overall logical index must be computed by taking a `min` of the logical indices we are iterating over.
```
uint64_t A_i = 0;
uint64_t B_i_iter = B.pos[0];
uint64_t C_i = 0;
while ((B_i_iter < B.pos[1]) && (C_i < C.shape[0])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t i = min(B_i, C_i);
  ...
}
```
Lastly, we need a guard to only perform computation if we are indeed in an intersection of the two iterators. This is done via a `LIR::IfStmt` over the `B` and `C` iterators, with a body of the compute expression, an `LIR::ArrayAssignment`.
```
uint64_t A_i = 0;
uint64_t B_i_iter = B.pos[0];
uint64_t C_i = 0;
while ((B_i_iter < B.pos[1]) && (C_i < C.shape[0])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t i = min(B_i, C_i);
  if ((B_i == i) && (C_i == i)) {
    A.values[i] = (B.values[B_i_iter] * C.values[i]);
  }
  ...
}
```
Lastly, we need to increment our iterators via a series of `IncrementIterator`s for `B` and `C`.
```
uint64_t A_i = 0;
uint64_t B_i_iter = B.pos[0];
uint64_t C_i = 0;
while ((B_i_iter < B.pos[1]) && (C_i < C.shape[0])) {
  uint64_t B_i = B.crd[B_i_iter];
  uint64_t i = min(B_i, C_i);
  if ((B_i == i) && (C_i == i)) {
    A.values[i] = (B.values[B_i_iter] * C.values[i]);
  }
  B_i_iter += (i == B_i);
  C_i++;
}
```
That is our kernel! This example is also the test case in `tests/test0.cpp`. Note that we don't require you to generate exactly the code we show here, or even following our naming conventions at all. Your code just needs to compile under our testing, and you are free to change anything you like.


### Optimizations

Note that we do not expect you to implement the optimizations given in the original TACO paper, only the merge lattice construction and other lowering phases.


### Testing

TBD

To build and run a particular test, run:
```
make bin/test<#>
```
Example:
```
make bin/test0
```
Outputs:
```
clang++ -g -O3 -std=c++17 -I ./include/ tests/test0.cpp bin/Array.o bin/Expr.o bin/GatherIteratorSet.o bin/IRPrinter.o bin/IRVisitor.o bin/IndexStmt.o bin/JIT.o bin/LIR.o bin/Lattice.o bin/Lower.o bin/SetExpr.o -o bin/test0
./bin/test0
Running test tests/test0_runner.cpp
Success
Success
Success
Success
Success
```


## Requirements.

The JIT infrastructure assumes that you are using a POSIX machine. If you do not have access to one (e.g. because you have a Windows laptop), you will need to conduct the assignment on the Stanford myth machines.

Both `run_test.sh` and the `Makefile` assume that your machine has `clang++` in your path. If that is not the case, please replace `clang++` with your favorite C++ compiler.


### C++ information

This project heavily uses `std::shared_ptr` for its IR. If you are not familiar with `std::shared_ptr`, please see [this](https://shaharmike.com/cpp/shared-ptr/).


### Build Notes

The provided `Makefile` will build any `.cpp` file in `src/` as long as it has a matching-named `.h` in `include/`, and include the object file in compiling any test executables. This means it should be relatively straight-forward to add files if you wish to do so.
