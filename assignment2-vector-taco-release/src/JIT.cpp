#include "JIT.h"

#include "GatherIteratorSet.h"
#include "IndexStmt.h"
#include "IRPrinter.h"
#include "LIR.h"
#include "Lower.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

namespace {

// Gathers iterators in in-order traversal.
std::vector<std::string> get_arg_list(const IndexStmt &stmt, const FormatMap &formats) {
    LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
    std::vector<std::string> arg_list;
    std::transform(iset.iterators.cbegin(), iset.iterators.cend(), std::back_inserter(arg_list),
                  [](const LIR::ArrayLevel &a) { return a.name; });
    return arg_list;
}

}  // namespace

void compile(const Assignment &assignment, const FormatMap &formats, const std::string &filename) {
    IndexStmt stmt = lower(assignment);
    LIR::Stmt lstmt = lower(stmt, formats);
    std::vector<std::string> arg_list = get_arg_list(stmt, formats);
    compile_to_file(lstmt, arg_list, filename);
}

// Compiles into a temporary file and runs the corresponding test.
void compile_and_test(const Assignment &assignment, const FormatMap &formats, const std::string &test_file) {
    IndexStmt stmt = lower(assignment);
    printf("Lowered Stmt\n");
    LIR::Stmt lstmt = lower(stmt, formats);
    printf("Lowered LIR Stmt\n");
    std::vector<std::string> arg_list = get_arg_list(stmt, formats);
    compile_and_test(lstmt, arg_list, test_file);
}

void compile_to_file(const LIR::Stmt &stmt, const std::vector<std::string> &arg_list, const std::string &filename) {
    std::ofstream file;
    file.open(filename);

    file << "#include \"runtime/array.h\"\n\n\n";

    file << "void kernel(";

    const size_t n = arg_list.size();
    for (size_t i = 0; i < n; i++) {
        if (i != 0) {
            file << ", ";
        }
        file << "array &" << arg_list[i];
    }
    file << ") {\n";

    file << stmt << "\n";

    file << "}\n\n";

    file.close();
}


void compile_and_test(const LIR::Stmt &stmt, const std::vector<std::string> &arg_list, const std::string &test_file) {
    char name_template[] = "/tmp/cs343_test.XXXXXX";
    // Requires POSIX.
    mkstemp(name_template);
    const std::string filename = name_template;
    compile_to_file(stmt, arg_list, filename);
    const std::string command = "./run_test.sh " + filename + " " + test_file;
    std::cout << "Running test " << test_file << std::endl;
    system(command.c_str());
}