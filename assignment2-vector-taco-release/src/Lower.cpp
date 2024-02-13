#include "Lower.h"

#include <algorithm>
#include <set>

#include "IRVisitor.h"
#include "IRPrinter.h"
#include "GatherIteratorSet.h"
#include "Lattice.h"


IndexStmt lower(const Assignment &assignment) {
    // STUDENTS TODO:
    // assert(false);
    // return IndexStmt();
    // printf("assignment.access: %s\n", assignment.access.name.c_str());
    // lower to IndexStmt, CIN
    // const std::__1::shared_ptr<const ExprNode>
    static const std::shared_ptr<const ArrayAssignment> arrayAssignment = ArrayAssignment::make(assignment.access, assignment.rhs);
    IndexStmt node = IndexStmt(arrayAssignment);
    return node;
}

LIR::Stmt lower(const IndexStmt &stmt, const FormatMap &formats) {
    // STUDENTS TODO:
    // assert(false);
    // return LIR::Stmt();
    // Lower from CIN into Lowered Stmt
    LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
    static const std::shared_ptr<const LIR::IteratorDefinition> iter = LIR::IteratorDefinition::make(iset);
    LIR::Stmt lstmt = LIR::Stmt(iter);
    return lstmt;
    // static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, stmt);
    // for (auto const& x : formats) {
    //     printf("formats: %s\n", x.first.c_str());
    //     for (auto const& y : x.second) {
    //         printf("y: %d\n", (int)y);
    //         break;
    //     }
    // }

    // for (auto const& x : formats) {
    //     printf("formats: %s\n", x.first.c_str());
    //     for (auto const& y : x.second) {
    //         printf("y: %d\n", (int)y);
    //         break;
    //     }
    // }
    // formats: A
    // y: 0
    // formats: B
    // y: 1
    // formats: C
    // y: 1
}
