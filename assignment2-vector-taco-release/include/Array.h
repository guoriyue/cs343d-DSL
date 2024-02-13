#pragma once

#include <string>
#include <type_traits>

#include "Access.h"
#include "Expr.h"


// Represents array[indices] = rhs;
struct Assignment {
    const Access access;
    const Expr rhs;

    Assignment(const Access &_access, const Expr &_rhs) : access(_access), rhs(_rhs) {}
};

// Provides a nice interface to constructing Exprs from Array objects.
struct Array {
    const std::string name;

    Access operator()(const Index &index) const {
        return Access{name, {index}};
    }
};
