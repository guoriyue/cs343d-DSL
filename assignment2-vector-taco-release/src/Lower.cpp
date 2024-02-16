#include "Lower.h"

#include <algorithm>
#include <set>

#include "IRVisitor.h"
#include "IRPrinter.h"
#include "GatherIteratorSet.h"
#include "Lattice.h"

SetExpr convert(const Expr &expr) {
    // STUDENTS TODO:
    // assert(false);
    // return SetExpr();
    // Convert from CIN into Lowered SetExpr
    const ExprNode * n = expr.ptr.get();
    if (typeid(*n) == typeid(Add)) {
        Add * a = (Add *)n;
        SetExpr add_union = SetExpr(Union::make(convert(a->a), convert(a->b)));
        return add_union;
    } else if (typeid(*n) == typeid(Mul)) {
        // printf("Mul\n");
        Mul * m = (Mul *)n;
        SetExpr mul_union = SetExpr(Union::make(convert(m->a), convert(m->b)));
    } else if (typeid(*n) == typeid(ArrayRead)) {
        // printf("ArrayRead\n");
        ArrayRead * ar = (ArrayRead *)n;
        // const std::shared_ptr<const ArrayRead> array_read = ;
        SetExpr sexpr = SetExpr(ArrayDim::make(ar->access));
        return sexpr;
    } else {
        printf("Unknown convert from CIN into Lowered SetExpr\n");
    }
}

IndexStmt lower(const Assignment &assignment) {
    // STUDENTS TODO:
    // static const std::shared_ptr<const ArrayAssignment> arrayAssignment = ArrayAssignment::make(assignment.access, assignment.rhs);
    // IndexStmt node = IndexStmt(arrayAssignment);
    // // return node;
    // // IRVisitor visitor = IRVisitor();
    // // node.accept(&visitor);
    // return node;
    Expr rhs = assignment.rhs;
    SetExpr sexpr = convert(rhs);
    return ForAll::make(sexpr, ArrayAssignment::make(assignment.access, assignment.rhs));
    // std::shared_ptr<const IndexStmtNode> node;
    // if (typeid(rhs) == typeid(Add)) {
    //     printf("Add\n");
    // } else if (typeid(rhs) == typeid(Mul)) {
    //     printf("Mul\n");
    // } else if (typeid(rhs) == typeid(ArrayRead)) {
    //     printf("ArrayRead\n");
    // } else {
    //     printf("Unknown\n");
    // }
}

// LIR::Stmt convert(const IndexStmt &stmt, const FormatMap &formats) {
//     // STUDENTS TODO:
//     // assert(false);
//     // return StmtNode();
//     // Convert from CIN into Lowered StmtNode
//     const IndexStmtNode * n = stmt.ptr.get();
//     if (typeid(*n) == typeid(ForAll)) {
//         printf("ForAll\n");
//         const ForAll * fa = (ForAll *)n;
//         LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
//         static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, convert(fa->body, formats));
//         whileStmt->accept(nullptr);
//         return LIR::Stmt(whileStmt);
//     } else if (typeid(*n) == typeid(ArrayAssignment)) {
//         printf("ArrayAssignment\n");
//         const ArrayAssignment * aa = (ArrayAssignment *)n;
//         LIR::ArrayLevel al = LIR::access_to_array_level(aa->lhs, formats);
//         Expr rhs = aa->rhs;
//         // SetExpr sexpr = convert(rhs);
//         // LIR::Expr rhs =
//         // return LIR::Stmt(LIR::ArrayAssignment::make(al, rhs));
//     } else {
//         printf("Unknown 2\n");
//     }
// }

LIR::Stmt lower(const IndexStmt &stmt, const FormatMap &formats) {
    // STUDENTS TODO:
    // assert(false);
    // const SetExpr &sexpr, const IndexStmt &body, const FormatMap &formats
    // SetExpr sexpr = SetExpr();
    // MergeLattice lattice = MergeLattice::make(sexpr, stmt, formats);
    const IndexStmtNode * n = stmt.ptr.get();
    if (typeid(*n) == typeid(ForAll)) {
        printf("ForAll\n");
        const ForAll * fa = (ForAll *)n;
        MergeLattice lattice = MergeLattice::make(fa->sexpr, fa->body, formats);
        // return lattice.lower();
        // LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
        // static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, lower(fa->body, formats));
        // whileStmt->accept(nullptr);
        // return LIR::Stmt(whileStmt);
    } else if (typeid(*n) == typeid(ArrayAssignment)) {
        printf("ArrayAssignment\n");
        // Access lhs;
        // Expr rhs;
        // const ArrayAssignment * aa = (ArrayAssignment *)n;
        // SetExpr sexpr = convert(aa->rhs);

        // MergeLattice lattice = MergeLattice::make(SetExpr(), stmt, formats);

        // Access lhs;
        // Expr rhs;
    
        // LIR::ArrayLevel al = LIR::access_to_array_level(aa->lhs, formats);
        // LIR::Expr rhs = convert(aa->rhs);
        // return LIR::Stmt(LIR::ArrayAssignment::make(al, rhs));
    } else {
        printf("Unknown\n");
    }

    // return LIR::Stmt();
    // Lower from CIN into Lowered Stmt

    // LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
    // static const std::shared_ptr<const LIR::IteratorDefinition> iter = LIR::IteratorDefinition::make(iset);
    // LIR::Stmt lstmt = LIR::Stmt(iter);
    // return lstmt;

    // static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, stmt);
    // for (auto const& x : formats) {
    //     printf("formats: %s\n", x.first.c_str());
    //     for (auto const& y : x.second) {
    //         printf("y: %d\n", (int)y);
    //         break;
    //     }
    // }
    // return convert(stmt, formats);
    // const IndexStmtNode * n = stmt.ptr.get();
    // if (typeid(*n) == typeid(ForAll)) {
    //     printf("ForAll\n");
    // } else if (typeid(*n) == typeid(ArrayAssignment)) {
    //     printf("ArrayAssignment\n");
    // } else {
    //     printf("Unknown\n");
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