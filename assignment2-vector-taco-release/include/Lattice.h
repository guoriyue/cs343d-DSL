#pragma once

#include <vector>

#include "IndexStmt.h"
#include "LIR.h"
#include "SetExpr.h"
#include "IRVisitor.h"

// One possible interface for implementing MergeLattices


struct MergePoint {
    std::vector<LIR::ArrayLevel> iterators;
    IndexStmt body;

    // Whether this point dominates another point, useful for `MergeLattice::get_sub_points`.
    // Assumes both points contain only unique iterators.
    bool dominates(const MergePoint &point) const;
    MergePoint merge(const MergePoint &other) const;
    // MergePoint merge_union(const MergePoint &other) const;
};

struct MergeLattice {
    std::vector<MergePoint> points;

    static MergeLattice make(const SetExpr &sexpr, const IndexStmt &body, const FormatMap &formats);

    // Get all of the points from this MergeLattice that are sub-lattices of point,
    // all points that are dominated by point.
    std::vector<MergePoint> get_sub_points(const MergePoint &point) const;

    MergeLattice merge_intersection(const MergeLattice &other) const;
    MergeLattice merge_union(const MergeLattice &other) const;

    // LIR::Stmt lower(const IndexStmt &stmt, const FormatMap &formats) const;
};

// class MergeLatticeVisitor {
// public:
//     MergeLatticeVisitor(const FormatMap &formats);

//     MergeLattice make(const SetExpr &sexpr, const IndexStmt &body) {
//         // body.accept(this);
//         // return lattice;
//         MergeLattice l = lattice;
//         lattice = MergeLattice();
//         return l;
//     }
//     // MergeLattice make(const IndexStmt &stmt);

// private:
//     MergeLattice lattice;
//     const FormatMap &formats;
// };
