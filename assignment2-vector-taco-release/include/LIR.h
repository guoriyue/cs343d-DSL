#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Access.h"
#include "Format.h"


// An imperative C-like IR that IndexStmts can be lowered to.

// Forward declare IRVisitor for StmtNode accept method.
struct IRVisitor;

// Use a new namespace to avoid naming conflicts with the front-end language
// (i.e. `Mul` in Expr.h)
namespace LIR {

// Expressions in the lowered IR.
struct ExprNode {
    virtual void accept(IRVisitor *v) const = 0;
    ExprNode() {}
    virtual ~ExprNode() = default;
};



// Statements
struct Expr {
    std::shared_ptr<const ExprNode> ptr;

    /** Make an undefined statment */
    Expr() = default;

    /** Make a expression from a concrete expression node pointer (e.g. CompressedIterator) */
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

struct ArrayLevel {
    // Which array and which dimension this refers to.
    std::string name;
    // Whether this is a dense or compressed level.
    Format format;
};

LIR::ArrayLevel access_to_array_level(const Access &access, const FormatMap &formats);

// Read a value from an array.
struct ArrayAccess : public ExprNode {
    // Which array level this refers to.
    const ArrayLevel array;

    ArrayAccess(const ArrayLevel &_array)
        : array(_array) {}
    ~ArrayAccess() override = default;

    static const std::shared_ptr<const ArrayAccess> make(const ArrayLevel &_array);
    void accept(IRVisitor *v) const override;
};

struct Add : public ExprNode {
    const Expr a, b;

    Add(const Expr &_a, const Expr &_b) : a(_a), b(_b) {
        assert(a.defined() && b.defined());
    }
    ~Add() override = default;

    static const std::shared_ptr<const Add> make(const Expr &_a, const Expr &_b) ;
    void accept(IRVisitor *v) const override;
};

struct Mul : public ExprNode {
    const Expr a, b;

    Mul(const Expr &_a, const Expr &_b) : a(_a), b(_b) {
        assert(a.defined() && b.defined());
    }
    ~Mul() override = default;

    static const std::shared_ptr<const Mul> make(const Expr &_a, const Expr &_b) ;
    void accept(IRVisitor *v) const override;
};


// Statements in the lowered IR.
struct StmtNode {
    virtual void accept(IRVisitor *v) const = 0;
    StmtNode() {}
    virtual ~StmtNode() = default;
};

struct Stmt {
    std::shared_ptr<const StmtNode> ptr;

    /** Make an undefined statment */
    Stmt() = default;

    /** Make a statement from a concrete statement node pointer (e.g. IfStmt) */
    Stmt(const std::shared_ptr<const StmtNode> p)
        : ptr(p) {
    }

    // Allow explicit Stmt construction from any shared_ptr of a base class of StmtNode.
    template<typename T, typename std::enable_if<std::is_base_of<StmtNode, T>::value, int>::type = 0>
    Stmt(const std::shared_ptr<const T> p)
        : ptr(std::static_pointer_cast<const StmtNode>(p)) {
    }

    bool defined() const {
        return ptr != nullptr;
    }

    // Forward a visitor to the underlying pointer.
    void accept(IRVisitor *) const;
};

// Represents:
// stmt[0]
// stmt[1]
// ...
struct SequenceStmt : public StmtNode {
    const std::vector<Stmt> stmts;

    SequenceStmt(const std::vector<Stmt> _stmts)
        : stmts(_stmts) {}
    ~SequenceStmt() override = default;

    static const std::shared_ptr<const SequenceStmt> make(const std::vector<Stmt> _stmts);
    void accept(IRVisitor *v) const override;
};

struct IteratorSet {
    const std::vector<ArrayLevel> iterators;
};

// Generates:
// while(condition) { body; }
struct WhileStmt : public StmtNode {
    // Represents a condition that all of the given iterators are valid.
    // i.e. for iterators a (Dense) and b (Compressed)
    // (a_i < a_i_max) && (b_i_iter < b_i_iter_max)
    const IteratorSet condition;
    const Stmt body;

    WhileStmt(const IteratorSet &_condition, const Stmt &_body)
        : condition(_condition), body(_body) {
        assert(body.defined());
    }
    ~WhileStmt() override = default;

    static const std::shared_ptr<const WhileStmt> make(const IteratorSet &_condition, const Stmt &_body);
    void accept(IRVisitor *v) const override;
};

// Generates:
// if (conditions[0]) { bodies[0]; }
// else if (conditions[1]) { bodies[1]; }
// ...
// else if (conditions.last()) { bodies.last(); }
struct IfStmt : public StmtNode {
    // Represents a condition that all of the given iterators match the logical index.
    // i.e. for iterators a, b, c:
    // (a_i == i) && (b_i == i) && (c_i == i)
    const std::vector<IteratorSet> conditions;
    // Bodies of each of the if statements.
    const std::vector<Stmt> bodies;

    IfStmt(const std::vector<IteratorSet> &_conditions, const std::vector<Stmt> _bodies)
        : conditions(_conditions), bodies(_bodies) {
        assert(conditions.size() == bodies.size());
        for (const auto &body : bodies) {
            assert(body.defined());
        }
    }
    ~IfStmt() override = default;

    static const std::shared_ptr<const IfStmt> make(const std::vector<IteratorSet> &_conditions, const std::vector<Stmt> _bodies);
    void accept(IRVisitor *v) const override;
};

// Represents a compressed or dense increment.
// Dense:
//  A_i++;
// Compressed:
//  B_i_iter += (i == B_i);
struct IncrementIterator : public StmtNode {
    const ArrayLevel array;
    // if always is set, then even on Compressed, this will just be a ++
    // useful for optimizing the single-iterator case.
    const bool always;

    IncrementIterator(const ArrayLevel &_array)
        : array(_array), always(false) {}
    IncrementIterator(const ArrayLevel &_array, const bool _always)
        : array(_array), always(_always) {}
    ~IncrementIterator() override = default;

    static const std::shared_ptr<const IncrementIterator> make(const ArrayLevel &_array);
    static const std::shared_ptr<const IncrementIterator> make(const ArrayLevel &_array, const bool _always);
    void accept(IRVisitor *v) const override;
};

// Represents:
//  uint64_t a_i = a.crd[a_i_iter]
struct CompressedIndexDefinition : public StmtNode {
    const ArrayLevel array;

    CompressedIndexDefinition(const ArrayLevel &_array)
        : array(_array) {
        assert(array.format == Format::Compressed);
    }
    ~CompressedIndexDefinition() override = default;

    static const std::shared_ptr<const CompressedIndexDefinition> make(const ArrayLevel &_array);
    void accept(IRVisitor *v) const override;
};

// Represents:
//  uint64_t i = min(a_i, min(b_i, ...)
struct LogicalIndexDefinition : public StmtNode {
    const IteratorSet iterators;

    LogicalIndexDefinition(const IteratorSet &_iterators)
        : iterators(_iterators) {}
    ~LogicalIndexDefinition() override = default;

    static const std::shared_ptr<const LogicalIndexDefinition> make(const IteratorSet &_iterators);
    void accept(IRVisitor *v) const override;
};

// Represents:
// for Dense iterator a:
//   uint64_t a_i = 0;
// for Compressed iterator b:
//   uint64_t b_i_iter = b.pos[0];
struct IteratorDefinition : public StmtNode {
    const IteratorSet iterators;

    IteratorDefinition(const IteratorSet &_iterators)
        : iterators(_iterators) {}
    ~IteratorDefinition() override = default;

    static const std::shared_ptr<const IteratorDefinition> make(const IteratorSet &_iterators);
    void accept(IRVisitor *v) const override;
};


// Represents:
// array[index] = value
struct ArrayAssignment : public StmtNode {
    // Which array level this refers to.
    const ArrayLevel array;
    // Value to assign.
    const Expr value;

    ArrayAssignment(const ArrayLevel &_array, const Expr &_value)
        : array(_array), value(_value) {
        assert(value.defined());
    }
    ~ArrayAssignment() override = default;

    static const std::shared_ptr<const ArrayAssignment> make(const ArrayLevel &_array, const Expr &_value);
    void accept(IRVisitor *v) const override;
};

}  // namespace LIR
