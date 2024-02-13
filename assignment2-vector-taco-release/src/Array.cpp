#include "Array.h"

Access::operator Expr() const {
    return ArrayRead::make(*this);
}

Assignment Access::operator=(const Expr &rhs) {
    return Assignment(*this, rhs);
}
