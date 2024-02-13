#pragma once

#include <string>
#include <vector>

// Forward declare.
struct Assignment;
struct Expr;

// Small wrapper for clarity.
struct Index {
    const std::string name;
};


// Represent an array access. Can be written to or read from.
struct Access {
    // Thin wrapper that can be converted into a read implicitly (producing an Expr),
    // or can be written to (via the assignment operator) to produce an Assingment node,
    // which is the primary object of compilation.

    const std::string name;
    // For this assignment, vars will always be length 1.
    const std::vector<Index> indices;

    // Implicitly convert to Expr (read from array)
    operator Expr() const;
    // Assign (write to array)
    Assignment operator=(const Expr &rhs);
};