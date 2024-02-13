#include "SetExpr.h"
#include "IRVisitor.h"

void SetExpr::accept(IRVisitor *v) const {
    ptr->accept(v);
}

SetExpr operator|(SetExpr a, SetExpr b) {
    return Union::make(a, b);
}

SetExpr operator&(SetExpr a, SetExpr b) {
    return Intersection::make(a, b);
}

void ArrayDim::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const ArrayDim> ArrayDim::make(const Access &_access) {
    return std::make_shared<ArrayDim>(_access);
}

const std::shared_ptr<const Union> Union::make(SetExpr _a, SetExpr _b) {
    return std::make_shared<Union>(_a, _b);
}

void Union::accept(IRVisitor *v) const {
    v->visit(this);
}

const std::shared_ptr<const Intersection> Intersection::make(SetExpr _a, SetExpr _b) {
    return std::make_shared<Intersection>(_a, _b);
}

void Intersection::accept(IRVisitor *v) const {
    v->visit(this);
}
