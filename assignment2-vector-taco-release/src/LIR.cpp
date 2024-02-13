#include "LIR.h"
#include "IRVisitor.h"

namespace LIR {

void Expr::accept(IRVisitor *v) const {
    ptr->accept(v);
}

LIR::ArrayLevel access_to_array_level(const Access &access, const FormatMap &formats) {
    auto search = formats.find(access.name);
    assert(search != formats.end());
    const Format &format = search->second[0];
    return LIR::ArrayLevel{access.name, format};
}

void ArrayAccess::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const ArrayAccess> ArrayAccess::make(const ArrayLevel &_array) {
    return std::make_shared<ArrayAccess>(_array);
}

void Mul::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const Mul> Mul::make(const Expr &_a, const Expr &_b) {
    return std::make_shared<Mul>(_a, _b);
}

void Add::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const Add> Add::make(const Expr &_a, const Expr &_b) {
    return std::make_shared<Add>(_a, _b);
}


void Stmt::accept(IRVisitor *v) const {
    ptr->accept(v);
}

void SequenceStmt::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const SequenceStmt> SequenceStmt::make(const std::vector<Stmt> _stmts) {
    return std::make_shared<SequenceStmt>(_stmts);
}

void WhileStmt::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const WhileStmt> WhileStmt::make(const IteratorSet &_condition, const Stmt &_body) {
    return std::make_shared<WhileStmt>(_condition, _body);
}

void IfStmt::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const IfStmt> IfStmt::make(const std::vector<IteratorSet> &_conditions, const std::vector<Stmt> _bodies) {
    return std::make_shared<IfStmt>(_conditions, _bodies);
}

void IncrementIterator::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const IncrementIterator> IncrementIterator::make(const ArrayLevel &_array) {
    return std::make_shared<IncrementIterator>(_array);
}

const std::shared_ptr<const IncrementIterator> IncrementIterator::make(const ArrayLevel &_array, const bool _always) {
    return std::make_shared<IncrementIterator>(_array, _always);
}

void CompressedIndexDefinition::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const CompressedIndexDefinition> CompressedIndexDefinition::make(const ArrayLevel &_array) {
    return std::make_shared<CompressedIndexDefinition>(_array);
}

void LogicalIndexDefinition::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const LogicalIndexDefinition> LogicalIndexDefinition::make(const IteratorSet &_iterators) {
    return std::make_shared<LogicalIndexDefinition>(_iterators);
}

void IteratorDefinition::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const IteratorDefinition> IteratorDefinition::make(const IteratorSet &_iterators) {
    return std::make_shared<IteratorDefinition>(_iterators);
}

void ArrayAssignment::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const ArrayAssignment> ArrayAssignment::make(const ArrayLevel &_array, const Expr &_value) {
    return std::make_shared<ArrayAssignment>(_array, _value);
}

}  // namespace LIR
