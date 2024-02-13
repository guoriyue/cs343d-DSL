#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "Access.h"

// Very similar to Expr.h, but for set operations, used in ForAll bounds (see Stmt.h).

// Forward declare IRVisitor for SetExprNode accept method.
struct IRVisitor;

struct SetExprNode {
    virtual void accept(IRVisitor *v) const = 0;
    SetExprNode() {}
    virtual ~SetExprNode() = default;
};

struct SetExpr {
    std::shared_ptr<const SetExprNode> ptr;

    /** Make an undefined expression */
    SetExpr() = default;

    /** Make an expression from a concrete expression node pointer (e.g. Union) */
    SetExpr(const std::shared_ptr<const SetExprNode> p)
        : ptr(p) {
    }

    // Allow explicit SetExpr construction from any shared_ptr of a base class of SetExprNode.
    template<typename T, typename std::enable_if<std::is_base_of<SetExprNode, T>::value, int>::type = 0>
    SetExpr(const std::shared_ptr<const T> p)
        : ptr(std::static_pointer_cast<const SetExprNode>(p)) {
    }

    bool defined() const {
        return ptr != nullptr;
    }

    // Forward a visitor to the underlying pointer.
    void accept(IRVisitor *) const;
};

// Supported operations on SetExprs.
// Union
SetExpr operator|(SetExpr a, SetExpr b);
// Intersection
SetExpr operator&(SetExpr a, SetExpr b);

// Counterpart to the ArrayRead Expr
struct ArrayDim : public SetExprNode {
    const Access access;

    ArrayDim(const Access &_access)
        : access(_access) {}
    ~ArrayDim() override = default;

    static const std::shared_ptr<const ArrayDim> make(const Access &_access);
    void accept(IRVisitor *v) const override;
};

// Set union. Used for implementing array addition.
struct Union : public SetExprNode {
    SetExpr a, b;

    Union(SetExpr _a, SetExpr _b) : a(_a), b(_b) {}
    ~Union() override = default;

    static const std::shared_ptr<const Union> make(SetExpr _a, SetExpr _b);
    void accept(IRVisitor *v) const override;
};

// Set intersection. Used for implementing array multiplication.
struct Intersection : public SetExprNode {
    SetExpr a, b;

    Intersection(SetExpr _a, SetExpr _b) : a(_a), b(_b) {}
    ~Intersection() override = default;

    static const std::shared_ptr<const Intersection> make(SetExpr _a, SetExpr _b);
    void accept(IRVisitor *v) const override;
};
