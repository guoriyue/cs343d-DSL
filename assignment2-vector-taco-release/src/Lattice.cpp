
#include "Lattice.h"

#include <algorithm>
#include <iostream>
#include <ostream>
#include <set>

#include "Expr.h"
#include "IRVisitor.h"
#include "IRPrinter.h"

namespace {

// For debugging.
void print_merge_lattice(std::ostream &stream, const MergeLattice &lattice) {
    for (const auto &p : lattice.points) {
        stream << "--------------------\n";
        for (const auto &i : p.iterators) {
            stream << i.name << "_i_" << ((i.format == Format::Compressed) ? "c" : "d") << "\n";
        }
        stream << "----------\n";
        stream << p.body;
        stream << "--------------------\n";
    }
}

}  // namespace


// Whether this point dominates another point, useful for `MergeLattice::get_sub_points`.
// Assumes both points contain only unique iterators.
bool MergePoint::dominates(const MergePoint &point) const {
    std::vector<std::string> iter_names;
    for (const auto &i : iterators) {
        iter_names.push_back(i.name);
    }
    for (const auto &i : point.iterators) {
        if (std::find(iter_names.begin(), iter_names.end(), i.name) == iter_names.end()) {
            return false;
        }
    }
    return true;
}

MergePoint MergePoint::merge_intersection(const MergePoint &other) const {
    std::vector<LIR::ArrayLevel> new_iterators;
    for (const auto &i : this->iterators) {
        new_iterators.push_back(i);
    }
    for (const auto &i : other.iterators) {
        new_iterators.push_back(i);
    }

    MergePoint point;
    point.iterators = new_iterators;
    return point;
}

MergeLattice MergeLattice::merge_intersection(const MergeLattice &other) const {
    std::vector<MergePoint> new_points;
    for (const auto &a : points) {
        for (const auto &b : other.points) {
            new_points.push_back(a.merge_intersection(b));
            // new_points.push_back(a);
        }
    }
    MergeLattice lattice;
    lattice.points = new_points;
    return lattice;
}

// MergePoint MergePoint::merge_union(const MergePoint &other) const {
//     std::vector<LIR::ArrayLevel> iterators;
//     for (const auto &i : this->iterators) {
//         iterators.push_back(i);
//     }
//     for (const auto &i : other.iterators) {
//         iterators.push_back(i);
//     }
//     IndexStmt body = LIR::Stmt::make(LIR::WhileStmt::make(LIR::IteratorSet(), LIR::Stmt()));
//     return MergePoint{iterators, body};
// }

MergeLattice MergeLattice::merge_union(const MergeLattice &other) const {
    std::vector<MergePoint> all_points;
    for (const auto &a : points) {
        for (const auto &b : other.points) {
            // all_points.push_back(a.merge_union(b));
            all_points.push_back(a);
        }
    }

    for (const auto &a : points) {
        all_points.push_back(a);
    }

    for (const auto &b : other.points) {
        all_points.push_back(b);
    }

    // std::vector<MergePoint> new_points;
    // std::vector<LIR::ArrayLevel> iterators = all_points[0].iterators;
    // for (const auto &p : all_points) {
    //     bool missing_iterators = false;
    //     for (const auto &i : iterators) {
    //         if (std::find(p.iterators.begin(), p.iterators.end(), i) == p.iterators.end()) {
    //             missing_iterators = true;
    //             break;
    //         }
    //     }
    //     if (!missing_iterators) {
    //         new_points.push_back(p);
    //     }
    // }

    MergeLattice lattice;
    lattice.points = all_points;
    return lattice;
}

// LIR::Stmt MergeLattice::lower(const IndexStmt &stmt, const FormatMap &formats) const {
    struct BuildMergeLatticeBody : public IRVisitor {
        const FormatMap &formats;
        std::vector<LIR::ArrayLevel> iterators;
        LIR::Stmt body;

        BuildMergeLatticeBody(const FormatMap &formats) : formats(formats) {}

        LIR::Stmt build(const IndexStmt &stmt) {
            stmt.accept(this);
            LIR::Stmt b = body;
            body = LIR::Stmt();
            return b;
        }

        using IRVisitor::visit;

        void visit(const Forall *op) override {
            printf("Forall\n");
            op->body.accept(this);
        }

        void visit(const ArrayDim *op) override {
            printf("ArrayDim\n");
            LIR::ArrayLevel iterator = LIR::access_to_array_level(op->access, formats);
            iterators.push_back(iterator);
            body = LIR::Stmt(LIR::WhileStmt::make(LIR::IteratorSet(), LIR::Stmt()));
        }

        void visit(const Union *node) override {
            printf("Union\n");
            node->a.accept(this);
            LIR::Stmt a_body = body;
            node->b.accept(this);
            LIR::Stmt b_body = body;
            body = LIR::Stmt(LIR::SequenceStmt::make({a_body, b_body}));
        }

        void visit(const Intersection *node) override {
            printf("Intersection\n");
            node->a.accept(this);
            LIR::Stmt a_body = body;
            node->b.accept(this);
            LIR::Stmt b_body = body;
            body = LIR::Stmt(LIR::SequenceStmt::make({a_body, b_body}));
        }
    };
    // std::vector<LIR::Stmt> loops;
    // for (const auto &p : points) {
    //     std::vector<LIR::ArrayLevel> iterators = p.iterators;
    //     IndexStmt body = p.body;
    //     LIR::IteratorSet iset = gather_iterator_set(body, formats);
    //     static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, lower(body, formats));
    //     whileStmt->accept(nullptr);
    //     loops.push_back(LIR::Stmt(whileStmt));
    // }
    // return LIR::Stmt(LIR::SequenceStmt::make(loops));
}

void MergeLattice::add_body(const IndexStmt &stmt, const FormatMap &formats) {
    // "body" will always be an ArrayAssignment

    // The IfStmt will be inside the body of the generated WhileStmt! For an example, see lines 17-19 of Figure 3 and lines 12-17 of Figure 9 in the original TACO paper. 

    // I recommend generating the body of a mergepoint only after constructing the entire merge lattice - the body of each merge point will then just be the assignment simplified to remove any vectors that become zero (i.e. don't have an iterator in that point).

    // previously we only construct the ArrayLevel of MergePoint, now we need to construct the body of MergePoint
    struct BuildMergeLatticeBody : public IRVisitor {
        const FormatMap &formats;
        std::vector<LIR::ArrayLevel> iterators;
        LIR::Stmt body;

        BuildMergeLatticeBody(const FormatMap &formats) : formats(formats) {}

        LIR::Stmt build(const IndexStmt &stmt) {
            stmt.accept(this);
            LIR::Stmt b = body;
            body = LIR::Stmt();
            return b;
        }

        using IRVisitor::visit;

        void visit(const ArrayAssignment *op) override {
            printf("ArrayAssignment\n");
            LIR::ArrayLevel iterator = LIR::access_to_array_level(op->lhs, formats);
            iterators.push_back(iterator);
            body = LIR::Stmt(LIR::WhileStmt::make(LIR::IteratorSet(), LIR::Stmt()));
        }
    };
    std::vector<LIR::Stmt> loops;
    for (const auto &p : points) {
        p.body = BuildMergeLatticeBody(formats).build(p.body);
    }
}

MergeLattice MergeLattice::make(const SetExpr &sexpr, const IndexStmt &body, const FormatMap &formats) {
    
    struct BuildMergeLattice : public IRVisitor {
        const FormatMap &formats;
        MergeLattice lattice;
        std::vector<LIR::ArrayLevel> iterators;

        BuildMergeLattice(const FormatMap &formats) : formats(formats) {}

        MergeLattice build(const SetExpr &sexpr) {
            sexpr.accept(this);
            MergeLattice l = lattice;
            lattice = MergeLattice();
            return l;
        }
        // IndexExpr == Expr
        // What about IndexStmt?

        using IRVisitor::visit;

        // void visit(const Forall* op) {
        //     // printf("Forall\n");
        //     // op->sexpr.accept(this);
        // }
        void visit(const ArrayDim *op) override {
            printf("ArrayDim\n");
            // op->access.accept(this);
            // ... set lattice for single point
            // lattice.points.push_back(MergePoint{iterators, bodies[0]});
            LIR::ArrayLevel iterator = LIR::access_to_array_level(op->access, formats);
            printf("access name = %s\n", op->access.name.c_str());
            MergePoint latticePoint;
            latticePoint.iterators = std::vector<LIR::ArrayLevel>{iterator};
            // latticePoint.body = ArrayAssignment::make(op->access, op->expr);
            MergeLattice l;
            l.points = std::vector<MergePoint>{latticePoint};
            lattice = l;
        }
        // Access lhs;
        // Expr rhs;
        void visit(const Union* node) override {
            printf("Union\n");
            node->a.accept(this);
            MergeLattice a_lattice = lattice;
            printf("a_lattice.points.size() = %d\n", a_lattice.points.size());
            for (const auto &p : a_lattice.points) {
                std::vector<LIR::ArrayLevel> iterators = p.iterators;
                for (const auto &i : iterators) {
                    printf("i.name = %s\n", i.name.c_str());
                }
            }
            node->b.accept(this);
            MergeLattice b_lattice = lattice;
            printf("b_lattice.points.size() = %d\n", b_lattice.points.size());
            for (const auto &p : b_lattice.points) {
                std::vector<LIR::ArrayLevel> iterators = p.iterators;
                for (const auto &i : iterators) {
                    printf("i.name = %s\n", i.name.c_str());
                }
            }
            // ... now set lattice to be union of the two lattices.
            lattice = a_lattice.merge_union(b_lattice);
            // MergeLattice l = buildLattice(node->rhs);
            // lattice = MergeLattice(l.getMergePoints(), {getIterator(node->lhs)});
        }

        void visit(const Intersection* node) override {
            printf("Intersection\n");
            // MergeLattice l = buildLattice(node->rhs);
            // lattice = MergeLattice(l.getMergePoints(), {getIterator(node->lhs)});
            node->a.accept(this); // a and b are SetExprs
            MergeLattice a_lattice = lattice;
            printf("a_lattice.points.size() = %d\n", a_lattice.points.size());
            for (const auto &p : a_lattice.points) {
                std::vector<LIR::ArrayLevel> iterators = p.iterators;
                for (const auto &i : iterators) {
                    printf("i.name = %s\n", i.name.c_str());
                }
            }
            node->b.accept(this);
            MergeLattice b_lattice = lattice;
            printf("b_lattice.points.size() = %d\n", b_lattice.points.size());
            for (const auto &p : b_lattice.points) {
                std::vector<LIR::ArrayLevel> iterators = p.iterators;
                for (const auto &i : iterators) {
                    printf("i.name = %s\n", i.name.c_str());
                }
            }
            // ... now set lattice to be intersection of the two lattices.
            lattice = a_lattice.merge_intersection(b_lattice);
            printf("finish visit Intersection\n");
        }

    };




    MergeLattice lattice;
    BuildMergeLattice b = BuildMergeLattice(formats);
    // lattice = b.buildLattice(body);
    lattice = b.build(sexpr);
    // generating the body of a mergepoint only after constructing the entire merge lattice
    printf("finish make\n");
    return lattice;
    // SetExpr to iterators
    // MergeLatticeVisitor visitor(formats);
    // return visitor.make(sexpr, body);

    // one for ForAll and the other for ArrayAssignment
}

// Get all of the points from this MergeLattice that are sub-lattices of point,
// all points that are dominated by point.
std::vector<MergePoint> MergeLattice::get_sub_points(const MergePoint &point) const {
    std::vector<MergePoint> sub_points;
    for (const auto &p : points) {
        if (point.dominates(p)) {
            sub_points.push_back(p);
        }
    }
    return sub_points;
}