#include "IRPrinter.h"

std::ostream &operator<<(std::ostream &stream, const Assignment &ir) {
    IRPrinter p(stream);
    p.print(ir.access);
    stream << " = ";
    p.print(ir.rhs);
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const Expr &ir) {
    if (!ir.defined()) {
        stream << "(undefined)";
    } else {
        IRPrinter p(stream);
        p.print(ir);
    }
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const IndexStmt &ir) {
    if (!ir.defined()) {
        stream << "(undefined)";
    } else {
        IRPrinter p(stream);
        p.print(ir);
    }
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const SetExpr &ir) {
    if (!ir.defined()) {
        stream << "(undefined)";
    } else {
        IRPrinter p(stream);
        p.print(ir);
    }
    return stream;
}


std::ostream &operator<<(std::ostream &stream, const LIR::Expr &ir) {
    if (!ir.defined()) {
        stream << "(undefined)";
    } else {
        IRPrinter p(stream);
        p.print(ir);
    }
    return stream;
}

std::ostream &operator<<(std::ostream &stream, const LIR::Stmt &ir) {
    if (!ir.defined()) {
        stream << "(undefined)";
    } else {
        IRPrinter p(stream);
        p.print(ir);
    }
    return stream;
}

IRPrinter::IRPrinter(std::ostream &s)
    : stream(s) {
    s.setf(std::ios::fixed, std::ios::floatfield);
}

void IRPrinter::print(const Access &ir) {
    stream << ir.name;
    stream << "(";
    print_list(ir.indices);
    stream << ")";
}

void IRPrinter::print(const Expr &ir) {
    ir.accept(this);
}

void IRPrinter::print(const Index &ir) {
    stream << ir.name;
}

void IRPrinter::print(const IndexStmt &ir) {
    ir.accept(this);
}

void IRPrinter::print(const SetExpr &ir) {
    ir.accept(this);
}

void IRPrinter::print(const LIR::Expr &ir) {
    ir.accept(this);
}

void IRPrinter::print(const LIR::Stmt &ir) {
    ir.accept(this);
}

template<typename T>
void IRPrinter::print_list(const std::vector<T> &exprs) {
    for (size_t i = 0; i < exprs.size(); i++) {
        print(exprs[i]);
        if (i < exprs.size() - 1) {
            stream << ", ";
        }
    }
}

void IRPrinter::print_indent() {
    for (int i = 0; i < indent; i++) {
        stream << " ";
    }
}

void IRPrinter::open() {
    // if (!implicit_parens) {
        stream << "(";
    // }
}

void IRPrinter::close() {
    // if (!implicit_parens) {
        stream << ")";
    // }
}

template<typename T>
void IRPrinter::print_binop(const T *op, const std::string &opcode) {
    open();
    print(op->a);
    stream << " " << opcode << " ";
    print(op->b);
    close();
}

void IRPrinter::visit(const ArrayRead *op) {
    print(op->access);
}

void IRPrinter::visit(const Add *op) {
    print_binop(op, "+");
}

void IRPrinter::visit(const Mul *op) {
    print_binop(op, "*");
}

void IRPrinter::visit(const ArrayDim *op) {
    print(op->access);
}

void IRPrinter::visit(const Union *op) {
    print_binop(op, "∪");
}

void IRPrinter::visit(const Intersection *op) {
    print_binop(op, "∩");
}

void IRPrinter::visit(const ForAll *op) {
    print_indent();
    stream << "∀ ";
    print(op->sexpr);
    stream << " {\n";

    indent += 2;
    print(op->body);
    indent -= 2;

    print_indent();
    stream << "}";
}

void IRPrinter::visit(const ArrayAssignment *op) {
    print_indent();
    print(op->lhs);
    stream << " = ";
    print(op->rhs);
    stream << "\n";
}


void print_iterator(std::ostream &stream, const LIR::ArrayLevel array) {
    stream << array.name;
    // iterator is always dimension 0 for this assignment.
    stream << "_i";
    if (array.format == Format::Compressed) {
        stream << "_iter";
    }
}

void print_derived_index(std::ostream &stream, const LIR::ArrayLevel array) {
    assert(array.format == Format::Compressed);
    stream << array.name;
    // iterator is always dimension 0 for this assignment.
    stream << "_i";
}

void print_logical_index(std::ostream &stream) {
    // iterator is always dimension 0 for this assignment.
    stream << "i";
}

void print_iterator_bound(std::ostream &stream, const LIR::ArrayLevel array, const bool upper) {
    if (array.format == Format::Compressed) {
        stream << array.name;
        if (upper) {
            stream << ".pos[1]";
        } else {
            stream << ".pos[0]";
        }
    } else {
        // Dense.
        if (upper) {
            // iterator is always dimension 0 for this assignment.
            stream << array.name << ".shape[0]";
        } else {
            stream << "0";
        }
    }
}

void print_array_access(std::ostream &stream, const LIR::ArrayLevel array) {
    stream << array.name;
    stream << ".values[";
    if (array.format == Format::Dense) {
        // Use logical index if reading/writing a dense array.
        print_logical_index(stream);
    } else {
        // Otherwise use the compressed iterator to read.
        print_iterator(stream, array);
    }
    stream << "]";
}

void print_resolved_index(std::ostream &stream, const LIR::ArrayLevel array) {
    stream << array.name;
    // iterator is always dimension 0 for this assignment.
    stream << "_i";
}

void print_set_guard(std::ostream &stream, const LIR::IteratorSet &guard) {
    for (size_t i = 0; i < guard.iterators.size(); i++) {
        if (i != 0) {
            stream << " && ";
        }
        const auto &it = guard.iterators[i];
        stream << "(";
        print_resolved_index(stream, it);
        stream << " == ";
        print_logical_index(stream);
        stream << ")";
    }
}

void print_bounded_guard(std::ostream &stream, const LIR::IteratorSet &guard) {
    for (size_t i = 0; i < guard.iterators.size(); i++) {
        if (i != 0) {
            stream << " && ";
        }
        const auto &it = guard.iterators[i];
        stream << "(";
        print_iterator(stream, it);
        stream << " < ";
        print_iterator_bound(stream, it, true);
        stream << ")";
    }
}

void IRPrinter::visit(const LIR::ArrayAccess *op) {
    print_array_access(stream, op->array);
}

void IRPrinter::visit(const LIR::Add *op) {
    print_binop(op, "+");
}

void IRPrinter::visit(const LIR::Mul *op) {
    print_binop(op, "*");
}

void IRPrinter::visit(const LIR::SequenceStmt *node) {
    for (size_t i = 0; i < node->stmts.size(); i++) {
        print(node->stmts[i]);
    }
}

void IRPrinter::visit(const LIR::WhileStmt *op) {
    print_indent();
    stream << "while (";
    print_bounded_guard(stream, op->condition);
    stream << ") {\n";

    indent += 2;
    print(op->body);
    indent -= 2;

    print_indent();
    stream << "}\n";
}

void IRPrinter::visit(const LIR::IfStmt *op) {
    const size_t N = op->conditions.size();
    for (size_t i = 0; i < N; i++) {
        print_indent();

        if (i > 0) {
            stream << "else ";
        }
        stream << "if (";
        print_set_guard(stream, op->conditions[i]);
        stream << ") {\n";

        indent += 2;
        print(op->bodies[i]);
        indent -= 2;

        print_indent();
        stream << "}\n";
    }
}

void IRPrinter::visit(const LIR::IncrementIterator *op) {
    print_indent();
    print_iterator(stream, op->array);
    if ((!op->always) && op->array.format == Format::Compressed) {
        stream << " += (";
        print_logical_index(stream);
        stream << " == ";
        print_derived_index(stream, op->array);
        stream << ");";
    } else {
        // Dense.
        stream << "++;";
    }
    stream << "\n";
}

void IRPrinter::visit(const LIR::CompressedIndexDefinition *op) {
    assert(op->array.format == Format::Compressed);
    print_indent();
    stream << "uint64_t ";
    print_derived_index(stream, op->array);
    stream << " = " << op->array.name << ".crd[";
    print_iterator(stream, op->array);
    stream << "];\n";
}

void IRPrinter::visit(const LIR::LogicalIndexDefinition *op) {
    const auto &iterators = op->iterators.iterators;
    assert(!iterators.empty());

    print_indent();
    stream << "uint64_t ";
    // Should all have the same dimension, for this assignment.
    print_logical_index(stream);

    stream << " = ";
    for (size_t i = 0; i < (iterators.size() - 1); i++) {
        stream << "min(";
        print_resolved_index(stream, iterators[i]);
        stream << ", ";
    }
    print_resolved_index(stream, iterators.back());
    for (size_t i = 0; i < (iterators.size() - 1); i++) {
        stream << ")";
    }

    stream << ";\n";
}

void IRPrinter::visit(const LIR::IteratorDefinition *op) {
    const auto &iterators = op->iterators.iterators;
    assert(!iterators.empty());

    for (const auto &i : iterators) {
        print_indent();
        stream << "uint64_t ";
        print_iterator(stream, i);
        stream << " = ";
        print_iterator_bound(stream, i, false);
        stream << ";\n";
    }
}

void IRPrinter::visit(const LIR::ArrayAssignment *op) {
    print_indent();
    print_array_access(stream, op->array);
    stream << " = ";
    print(op->value);
    stream << ";\n";
    // and increment iterator of written-to array.
    if (op->array.format == Format::Compressed) {
        print_indent();
        print_iterator(stream, op->array);
        stream << "++;\n";
    }
}
