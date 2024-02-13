#include "IndexStmt.h"
#include "IRVisitor.h"

void IndexStmt::accept(IRVisitor *v) const {
    ptr->accept(v);
}

void ForAll::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const ForAll> ForAll::make(SetExpr _sexpr, IndexStmt _body) {
    return std::make_shared<ForAll>(_sexpr, _body);
}

void ArrayAssignment::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const ArrayAssignment> ArrayAssignment::make(const Access &_lhs, Expr _rhs) {
    return std::make_shared<ArrayAssignment>(_lhs, _rhs);
}
