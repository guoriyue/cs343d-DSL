#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "project.h"



int main(const int argc, const char** argv) {
    Index i{"i"};
    Array A{"A"}, B{"B"}, C{"C"}, D{"D"};

    Assignment a = (A(i) = B(i) + (C(i) * D(i)));
    FormatMap formats = {
        {"A", {Format::Dense}},
        {"B", {Format::Compressed}},
        {"C", {Format::Compressed}},
        {"D", {Format::Compressed}},
    };

    compile_and_test(a, formats, "tests/test1_runner.cpp");
    // compile(a, formats, "tests/test1_out.cpp");

    return 0;
}

