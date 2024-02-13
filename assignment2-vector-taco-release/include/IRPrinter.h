#pragma once

/** \file
 * This header file defines operators that let you dump an IR
 * expression, statement, or type directly into an output stream
 * in a human readable form.
 * E.g:
 \code
 Expr foo = ...
 std::cout << "Foo is " << foo << "\n";
 \endcode
 *
 * Heavily borrowed from Halide's opensource implementation:
 * https://github.com/halide/Halide/blob/main/src/IRPrinter.h
 */

#include <ostream>
#include <string>

#include "Access.h"
#include "Array.h"
#include "Expr.h"
#include "IRVisitor.h"
#include "LIR.h"

/** Emit an expression on an output stream (such as std::cout) in
 * human-readable form */
std::ostream &operator<<(std::ostream &stream, const Assignment &);
std::ostream &operator<<(std::ostream &stream, const Expr &);
std::ostream &operator<<(std::ostream &stream, const IndexStmt &);
std::ostream &operator<<(std::ostream &stream, const SetExpr &);
std::ostream &operator<<(std::ostream &stream, const LIR::Expr &);
std::ostream &operator<<(std::ostream &stream, const LIR::Stmt &);

/** An IRVisitor that emits IR to the given output stream in a human
 * readable form. Can be subclassed if you want to modify the way in
 * which it prints.
 */
struct IRPrinter : public IRVisitor {
    /** Construct an IRPrinter pointed at a given output stream
     * (e.g. std::cout, or a std::ofstream) */
    explicit IRPrinter(std::ostream &);

    /** emit an expression on the output stream */
    void print(const Access &);
    void print(const Expr &);
    void print(const Index &);
    void print(const IndexStmt &);
    void print(const SetExpr &);
    void print(const LIR::Expr &);
    void print(const LIR::Stmt &);

    /** emit a comma delimited list */
    template<typename T>
    void print_list(const std::vector<T> &exprs);

    /** The current indentation level, useful for pretty-printing
     * statements */
    int indent = 0;
    void print_indent();

    /** The stream on which we're outputting */
    std::ostream &stream;

    /** Emit "(" */
    void open();

    /** Emit ")" */
    void close();

    /** Useful helper function for printing binary operations. */
    template<typename T>
    void print_binop(const T *, const std::string &);

    void visit(const ArrayRead *) override;
    void visit(const Add *) override;
    void visit(const Mul *) override;

    // SetExpr
    void visit(const ArrayDim *) override;
    void visit(const Union *) override;
    void visit(const Intersection *) override;

    // IndexStmt
    void visit(const ForAll *) override;
    void visit(const ArrayAssignment *) override;

    // Lowered IR (LIR).
    void visit(const LIR::ArrayAccess *) override;
    void visit(const LIR::Add *) override;
    void visit(const LIR::Mul *) override;
    void visit(const LIR::SequenceStmt *) override;
    void visit(const LIR::WhileStmt *) override;
    void visit(const LIR::IfStmt *) override;
    void visit(const LIR::IncrementIterator *) override;
    void visit(const LIR::CompressedIndexDefinition *) override;
    void visit(const LIR::LogicalIndexDefinition *) override;
    void visit(const LIR::IteratorDefinition *) override;
    void visit(const LIR::ArrayAssignment *) override;
};

