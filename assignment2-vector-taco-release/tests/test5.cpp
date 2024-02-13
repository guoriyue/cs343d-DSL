#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>

#include "project.h"



int main(const int argc, const char** argv) {
    Index i{"i"};
    Array A{"A"}, B{"B"}, C{"C"}, D{"D"};

    Assignment a = (A(i) = (B(i) + C(i)) * D(i));
    FormatMap formats = {
        {"A", {Format::Dense}},
        {"B", {Format::Compressed}},
        {"C", {Format::Dense}},
        {"D", {Format::Compressed}},
    };

    compile_and_test(a, formats, "tests/test5_runner.cpp");

    return 0;
}

