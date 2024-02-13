#include "IRVisitor.h"

void IRVisitor::visit(const ArrayRead *node) {
}

template<typename T>
void IRVisitor::visit_binop(const T *node) {
    node->a.accept(this);
    node->b.accept(this);
}

void IRVisitor::visit(const Add *node) {
    visit_binop(node);
}

void IRVisitor::visit(const Mul *node) {
    visit_binop(node);
}

void IRVisitor::visit(const ArrayDim *node) {
}

void IRVisitor::visit(const Union *node) {
    visit_binop(node);
}

void IRVisitor::visit(const Intersection *node) {
    visit_binop(node);
}

void IRVisitor::visit(const ForAll *node) {
    node->sexpr.accept(this);
    node->body.accept(this);
}

void IRVisitor::visit(const ArrayAssignment *node) {
    node->rhs.accept(this);
}

void IRVisitor::visit(const LIR::ArrayAccess *node) {
}

void IRVisitor::visit(const LIR::Add *node) {
    visit_binop(node);
}

void IRVisitor::visit(const LIR::Mul *node) {
    visit_binop(node);
}

void IRVisitor::visit(const LIR::SequenceStmt *node) {
    for (size_t i = 0; i < node->stmts.size(); i++) {
        node->stmts[i].accept(this);
    }
}

void IRVisitor::visit(const LIR::WhileStmt *node) {
    // node->condition.accept(this);
    node->body.accept(this);
}

void IRVisitor::visit(const LIR::IfStmt *node) {
    for (size_t i = 0; i < node->conditions.size(); i++) {
        // node->conditions[i].accept(this);
        node->bodies[i].accept(this);
    }
}

void IRVisitor::visit(const LIR::IncrementIterator *node) {
}

void IRVisitor::visit(const LIR::CompressedIndexDefinition *node) {
}

void IRVisitor::visit(const LIR::LogicalIndexDefinition *node) {
}

void IRVisitor::visit(const LIR::IteratorDefinition *node) {
}

void IRVisitor::visit(const LIR::ArrayAssignment *node) {
    node->value.accept(this);
}
