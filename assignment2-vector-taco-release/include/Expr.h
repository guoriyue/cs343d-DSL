#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "Access.h"

// Forward declare IRVisitor for ExprNode accept method.
struct IRVisitor;

struct ExprNode {
    virtual void accept(IRVisitor *v) const = 0;
    ExprNode() {}
    virtual ~ExprNode() = default;
};

struct Expr {
    std::shared_ptr<const ExprNode> ptr;

    /** Make an undefined expression */
    Expr() = default;

    /** Make an expression from a concrete expression node pointer (e.g. Add) */
    Expr(const std::shared_ptr<const ExprNode> p)
        : ptr(p) {
    }

    // Allow explicit Expr construction from any shared_ptr of a base class of ExprNode.
    template<typename T, typename std::enable_if<std::is_base_of<ExprNode, T>::value, int>::type = 0>
    Expr(const std::shared_ptr<const T> p)
        : ptr(std::static_pointer_cast<const ExprNode>(p)) {
    }

    bool defined() const {
        return ptr != nullptr;
    }

    // Forward a visitor to the underlying pointer.
    void accept(IRVisitor *) const;
};

// Supported operations on Exprs.
Expr operator+(Expr a, Expr b);
Expr operator*(Expr a, Expr b);

// Represent an array read.
struct ArrayRead : public ExprNode {
    const Access access;

    ArrayRead(const Access &_access)
        : access(_access) {}
    ~ArrayRead() override = default;

    static const std::shared_ptr<const ArrayRead> make(const Access &_access);
    void accept(IRVisitor *v) const override;
};


// Represent an addition.
struct Add : public ExprNode {
    Expr a, b;

    Add(Expr _a, Expr _b) : a(_a), b(_b) {}
    ~Add() override = default;

    static const std::shared_ptr<const Add> make(Expr _a, Expr _b);
    void accept(IRVisitor *v) const override;
};

// Represent a multiplication.
struct Mul : public ExprNode {
    Expr a, b;

    Mul(Expr _a, Expr _b) : a(_a), b(_b) {}
    ~Mul() override = default;

    static const std::shared_ptr<const Mul> make(Expr _a, Expr _b);
    void accept(IRVisitor *v) const override;
};
