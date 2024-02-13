
#include "Lattice.h"

#include <algorithm>
#include <iostream>
#include <ostream>
#include <set>

#include "Expr.h"
#include "IRVisitor.h"
#include "IRPrinter.h"

namespace {

// For debugging.
void print_merge_lattice(std::ostream &stream, const MergeLattice &lattice) {
    for (const auto &p : lattice.points) {
        stream << "--------------------\n";
        for (const auto &i : p.iterators) {
            stream << i.name << "_i_" << ((i.format == Format::Compressed) ? "c" : "d") << "\n";
        }
        stream << "----------\n";
        stream << p.body;
        stream << "--------------------\n";
    }
}

}  // namespace

bool MergePoint::dominates(const MergePoint &point) const {
    // STUDENTS TODO:
    assert(false);
    return false;
}

MergeLattice MergeLattice::make(const SetExpr &sexpr, const IndexStmt &body, const FormatMap &formats) {
    // STUDENTS TODO:
    assert(false);
    return MergeLattice{};
}

std::vector<MergePoint> MergeLattice::get_sub_points(const MergePoint &point) const {
    // STUDENTS TODO:
    assert(false);
    return std::vector<MergePoint>{};
}

