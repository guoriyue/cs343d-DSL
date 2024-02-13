#pragma once

#include <memory>
#include <type_traits>

#include "Access.h"
#include "Expr.h"
#include "SetExpr.h"

// Stmt nodes for Concrete Index Notation (CIN).
// Only ForAll and Assignment are required for
// this assignment, other nodes would be required
// to re-implement all of TACO.
// See https://fredrikbk.com/publications/taco-workspaces.pdf
// or Fred's thesis for more details.

// Forward declare IRVisitor for IndexStmtNode accept method.
struct IRVisitor;


struct IndexStmtNode {
    virtual void accept(IRVisitor *v) const = 0;
    IndexStmtNode() {}
    virtual ~IndexStmtNode() = default;
};

struct IndexStmt {
    std::shared_ptr<const IndexStmtNode> ptr;

    /** Make an undefined statment */
    IndexStmt() = default;

    /** Make a statement from a concrete statement node pointer (e.g. ForAll) */
    IndexStmt(const std::shared_ptr<const IndexStmtNode> p)
        : ptr(p) {
    }

    // Allow explicit Stmt construction from any shared_ptr of a base class of IndexStmtNode.
    template<typename T, typename std::enable_if<std::is_base_of<IndexStmtNode, T>::value, int>::type = 0>
    IndexStmt(const std::shared_ptr<const T> p)
        : ptr(std::static_pointer_cast<const IndexStmtNode>(p)) {
    }

    bool defined() const {
        return ptr != nullptr;
    }

    // Forward a visitor to the underlying pointer.
    void accept(IRVisitor *) const;
};


// ForAll loop.
struct ForAll : public IndexStmtNode {
    // represents forall (sexpr) { body }

    // Set expression to loop over.
    SetExpr sexpr;
    // For this assignment, "body" will always be an ArrayAssignment (no nested for loops).
    IndexStmt body;

    ForAll(SetExpr _sexpr, IndexStmt _body) : sexpr(_sexpr), body(_body) {}
    ~ForAll() override = default;

    static const std::shared_ptr<const ForAll> make(SetExpr _sexpr, IndexStmt _body);
    void accept(IRVisitor *v) const override;
};

// Array assignment.
struct ArrayAssignment : public IndexStmtNode {
    // represents lhs = rhs
    Access lhs;
    Expr rhs;

    ArrayAssignment(const Access &_lhs, Expr _rhs)
        : lhs(_lhs), rhs(_rhs) {}
    ~ArrayAssignment() override = default;

    static const std::shared_ptr<const ArrayAssignment> make(const Access &_lhs, Expr _rhs);
    void accept(IRVisitor *v) const override;
};
