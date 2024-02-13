#pragma once

#include "Expr.h"
#include "IndexStmt.h"
#include "LIR.h"
#include "SetExpr.h"

/** Recursively walks over the IR. Subclasses should override methods
 * that they need to.
 */
struct IRVisitor {
    IRVisitor() = default;
    virtual ~IRVisitor() = default;

    template<typename T>
    void visit_binop(const T *);

    // Expr (computation)
    virtual void visit(const ArrayRead *);
    virtual void visit(const Add *);
    virtual void visit(const Mul *);

    // SetExpr
    virtual void visit(const ArrayDim *);
    virtual void visit(const Union *);
    virtual void visit(const Intersection *);

    // IndexStmt
    virtual void visit(const ForAll *);
    virtual void visit(const ArrayAssignment *);

    // Lowered IR (LIR).
    virtual void visit(const LIR::ArrayAccess *);
    virtual void visit(const LIR::Add *);
    virtual void visit(const LIR::Mul *);
    virtual void visit(const LIR::SequenceStmt *);
    virtual void visit(const LIR::WhileStmt *);
    virtual void visit(const LIR::IfStmt *);
    virtual void visit(const LIR::IncrementIterator *);
    virtual void visit(const LIR::CompressedIndexDefinition *);
    virtual void visit(const LIR::LogicalIndexDefinition *);
    virtual void visit(const LIR::IteratorDefinition *);
    virtual void visit(const LIR::ArrayAssignment *);
};

