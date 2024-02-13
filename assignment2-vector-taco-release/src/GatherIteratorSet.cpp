#include "GatherIteratorSet.h"

#include <set>

#include "IRVisitor.h"

namespace {

struct GatherIteratorSet : public IRVisitor {
    const FormatMap &formats;

    GatherIteratorSet(const FormatMap &_formats) : formats(_formats) {}

    std::vector<LIR::ArrayLevel> iterators;
    std::set<std::string> seen;

    void visit(const ArrayRead *ir) override {
        const std::string &name = ir->access.name;
        if (seen.find(name) == seen.end()) {
            iterators.push_back(LIR::access_to_array_level(ir->access, formats));
            seen.insert(name);
        }
    }

    void visit(const ArrayAssignment *ir) override {
        const std::string &name = ir->lhs.name;
        // assert(vars.empty());
        assert(seen.find(name) == seen.end()); // Should never see a written-to tensor twice.
        iterators.push_back(LIR::access_to_array_level(ir->lhs, formats));
        seen.insert(name);

        ir->rhs.accept(this);
    }
};



}  // namespace

LIR::IteratorSet gather_iterator_set(const IndexStmt &stmt, const FormatMap &formats) {
    GatherIteratorSet gatherer(formats);
    stmt.accept(&gatherer);
    return LIR::IteratorSet{gatherer.iterators};
}
