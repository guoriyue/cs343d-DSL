#pragma once

#include <vector>

#include "Format.h"
#include "IndexStmt.h"
#include "LIR.h"

// Gathers iterators in in-order traversal.
LIR::IteratorSet gather_iterator_set(const IndexStmt &stmt, const FormatMap &formats);
