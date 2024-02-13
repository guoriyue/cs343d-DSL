#include "Expr.h"
#include "IRVisitor.h"

void Expr::accept(IRVisitor *v) const {
    ptr->accept(v);
}

Expr operator+(Expr a, Expr b) {
    return Add::make(a, b);
}

Expr operator*(Expr a, Expr b) {
    return Mul::make(a, b);
}


void ArrayRead::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const ArrayRead> ArrayRead::make(const Access &_access) {
    return std::make_shared<ArrayRead>(_access);
}

const std::shared_ptr<const Add> Add::make(Expr _a, Expr _b) {
    return std::make_shared<Add>(_a, _b);
}

void Add::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const Mul> Mul::make(Expr _a, Expr _b) {
    return std::make_shared<Mul>(_a, _b);
}

void Mul::accept(IRVisitor *v) const {
    v->visit(this);
}
