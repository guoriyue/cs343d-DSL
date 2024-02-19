#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "project.h"



int main(const int argc, const char** argv) {
    Index i{"i"};
    Array A{"A"}, B{"B"}, C{"C"};

    Assignment a = (A(i) = B(i) * C(i));
    FormatMap formats = {
        {"A", {Format::Dense}},
        {"B", {Format::Compressed}},
        {"C", {Format::Compressed}},
    };

    // compile(a, formats, "tests/test0_out.cpp");

    compile_and_test(a, formats, "tests/test0_runner.cpp");

    return 0;
}

