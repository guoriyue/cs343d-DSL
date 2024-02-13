#pragma once

#include "Array.h"
#include "Expr.h"
#include "Format.h"
#include "IndexStmt.h"
#include "LIR.h"

// Lower from a basic tensor assignment into CIN.
IndexStmt lower(const Assignment &assignment);

// Lower from CIN into Lowered Stmt
LIR::Stmt lower(const IndexStmt &stmt, const FormatMap &formats);
