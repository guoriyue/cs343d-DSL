#pragma once

#include <string>
#include <vector>

#include "Array.h"
#include "LIR.h"
#include "Format.h"

// Performs full lowering + compilation into a file.
void compile(const Assignment &assignment, const FormatMap &formats, const std::string &filename);


// Compiles into a temporary file and runs the corresponding test.
void compile_and_test(const Assignment &assignment, const FormatMap &formats, const std::string &test_file);


// Helper method, compile stmt into the corresponding file.
void compile_to_file(const LIR::Stmt &stmt, const std::vector<std::string> &arg_list, const std::string &filename);

// Helper method, compile stmt into a kernel and run the test in `test_file`
void compile_and_test(const LIR::Stmt &stmt, const std::vector<std::string> &arg_list, const std::string &test_file);

