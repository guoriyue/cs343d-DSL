#include "Lower.h"

#include <algorithm>
#include <set>

#include "IRVisitor.h"
#include "IRPrinter.h"
#include "GatherIteratorSet.h"
#include "Lattice.h"

SetExpr convert(const Expr &expr) {
    // STUDENTS TODO:
    // assert(false);
    // return SetExpr();
    // Convert from CIN into Lowered SetExpr
    const ExprNode * n = expr.ptr.get();
    if (typeid(*n) == typeid(Add)) {
        printf("Add\n");
        Add * a = (Add *)n;
        SetExpr add_union = SetExpr(Union::make(convert(a->a), convert(a->b)));
        return add_union;
    } else if (typeid(*n) == typeid(Mul)) {
        printf("Mul\n");
        Mul * m = (Mul *)n;
        SetExpr mul_union = SetExpr(Intersection::make(convert(m->a), convert(m->b)));
        return mul_union;
    } else if (typeid(*n) == typeid(ArrayRead)) {
        printf("ArrayRead\n");
        ArrayRead * ar = (ArrayRead *)n;
        // const std::shared_ptr<const ArrayRead> array_read = ;
        SetExpr sexpr = SetExpr(ArrayDim::make(ar->access));
        printf("after ArrayRead\n");
        return sexpr;
    } else {
        printf("Unknown convert from CIN into Lowered SetExpr\n");
    }
}

IndexStmt lower(const Assignment &assignment) {
    // STUDENTS TODO:
    // static const std::shared_ptr<const ArrayAssignment> arrayAssignment = ArrayAssignment::make(assignment.access, assignment.rhs);
    // IndexStmt node = IndexStmt(arrayAssignment);
    // // return node;
    // // IRVisitor visitor = IRVisitor();
    // // node.accept(&visitor);
    // return node;
    Expr rhs = assignment.rhs;
    SetExpr sexpr = convert(rhs);
    printf("after convert\n");
    return ForAll::make(sexpr, ArrayAssignment::make(assignment.access, assignment.rhs));
    // std::shared_ptr<const IndexStmtNode> node;
    // if (typeid(rhs) == typeid(Add)) {
    //     printf("Add\n");
    // } else if (typeid(rhs) == typeid(Mul)) {
    //     printf("Mul\n");
    // } else if (typeid(rhs) == typeid(ArrayRead)) {
    //     printf("ArrayRead\n");
    // } else {
    //     printf("Unknown\n");
    // }
}

// LIR::Stmt convert(const IndexStmt &stmt, const FormatMap &formats) {
//     // STUDENTS TODO:
//     // assert(false);
//     // return StmtNode();
//     // Convert from CIN into Lowered StmtNode
//     const IndexStmtNode * n = stmt.ptr.get();
//     if (typeid(*n) == typeid(ForAll)) {
//         printf("ForAll\n");
//         const ForAll * fa = (ForAll *)n;
//         LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
//         static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, convert(fa->body, formats));
//         whileStmt->accept(nullptr);
//         return LIR::Stmt(whileStmt);
//     } else if (typeid(*n) == typeid(ArrayAssignment)) {
//         printf("ArrayAssignment\n");
//         const ArrayAssignment * aa = (ArrayAssignment *)n;
//         LIR::ArrayLevel al = LIR::access_to_array_level(aa->lhs, formats);
//         Expr rhs = aa->rhs;
//         // SetExpr sexpr = convert(rhs);
//         // LIR::Expr rhs =
//         // return LIR::Stmt(LIR::ArrayAssignment::make(al, rhs));
//     } else {
//         printf("Unknown 2\n");
//     }
// }

LIR::Expr convert_lir(const Expr &expr, const FormatMap &formats) {
    const ExprNode * n = expr.ptr.get();
    if (typeid(*n) == typeid(Add)) {
        printf("Add\n");
        Add * a = (Add *)n;
        LIR::Expr add_union = LIR::Expr(LIR::Add::make(convert_lir(a->a, formats), convert_lir(a->b, formats)));
        return add_union;
    } else if (typeid(*n) == typeid(Mul)) {
        printf("Mul\n");
        Mul * m = (Mul *)n;
        LIR::Expr mul_union = LIR::Expr(LIR::Mul::make(convert_lir(m->a, formats), convert_lir(m->b, formats)));
        return mul_union;
    } else if (typeid(*n) == typeid(ArrayRead)) {
        printf("ArrayRead\n");
        ArrayRead * ar = (ArrayRead *)n;
        LIR::Expr array_read = LIR::Expr(LIR::ArrayAccess::make(LIR::access_to_array_level(ar->access, formats)));
        return array_read;
    } else {
        printf("Unknown convert from CIN into Lowered Expr\n");
    }
}

LIR::Stmt lower(const IndexStmt &stmt, const FormatMap &formats) {

    // The code generation algorithm is a recursive function. It is called with an index expression and the first index variable from the expression’s iteration graph, and recurses on the index variables in the iteration graph in the forest ordering. 
    struct BuildLowerLIRs : public IRVisitor {
        const FormatMap &formats;
        MergeLattice lattice;
        std::vector<LIR::Stmt> stmts;

        BuildLowerLIRs(const FormatMap &formats) : formats(formats) {
        }

        void build(const IndexStmt &stmt) {
            stmt.accept(this);
        }


        void visit(const ForAll *op) {
            // stmts.push_back(lower(op->body, formats));
            printf("LIR ForAll\n");
            MergeLattice lattice = MergeLattice::make(op->sexpr, op->body, formats);
            // lattice.add_body(op->body, formats);
            // initializes
            std::vector<LIR::Stmt> code;
            code.push_back(LIR::Stmt(LIR::IteratorDefinition::make(LIR::IteratorSet{gather_iterator_set(op->body, formats)})));
            // a loop that emits one while loop per lattice point in level order
            for (const auto &p : lattice.points) {
                std::vector<LIR::ArrayLevel> iterators = p.iterators;
                // for (const auto &a : iterators) {
                //     printf("iterators: %s\n", a.name.c_str());
                // }
                printf("=================iterators.size() = %d\n", iterators.size());
                LIR::IteratorSet iset = LIR::IteratorSet{iterators};
                static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, code.back());
                // whileStmt->accept(this);
                code.push_back(LIR::Stmt(whileStmt));
                continue;

                // std::vector<LIR::Stmt> cids;
                // // initialize sparse idx variables
                // // bool has_compressed = false;
                // for (const auto &a : iterators) {
                //     if (a.format == Format::Compressed) {
                //         cids.push_back(LIR::Stmt(LIR::CompressedIndexDefinition::make(a)));
                //         // has_compressed = true;
                //     }
                // }
                // // LIR::Stmt compressedIndexDefinition = LIR::Stmt(LIR::CompressedIndexDefinition::make(iterators[0]));
                // // if (has_compressed) {
                // //     // code.push_back(LIR::Stmt(LIR::SequenceStmt::make(cids)));
                // //     LIR::Stmt mergedIdx = LIR::Stmt(LIR::LogicalIndexDefinition::make(iset));
                // //     cids.push_back(mergedIdx);
                // // }
                // LIR::Stmt mergedIdx = LIR::Stmt(LIR::LogicalIndexDefinition::make(iset));
                // cids.push_back(mergedIdx);
                // // LIR::SequenceStmt::make({compressedIndexDefinition, mergedIdx});
                
                // std::vector<LIR::IteratorSet> conditions;
                // std::vector<LIR::Stmt> bodies;
                // for (const auto &lq : lattice.get_sub_points(p)) {
                //     std::vector<LIR::ArrayLevel> equals;
                //     bool has_compressed = false;
                //     for (const auto &a : lq.iterators) {
                //         if (a.format == Format::Compressed) {
                //             equals.push_back(a);
                //             has_compressed = true;
                //         }
                //     }
                //     if (has_compressed) {
                //         conditions.push_back(LIR::IteratorSet{equals});
                //         Expr rhs = lq.expr;
                //         IndexStmt lf = op->body;
                //         ArrayAssignment * aa = (ArrayAssignment *)lf.ptr.get();
                //         Access lhs = aa->lhs;
                //         IndexStmt new_body = ArrayAssignment::make(lhs, rhs);
                //         bodies.push_back(lower(new_body, formats));
                //         // Access lhs = Access(op->body->lhs.name, equals);
                //         // IndexStmt new_body = ArrayAssignment::make(op->access, op->rhs);
                //         // bodies.push_back(lower(op->body, formats));
                //         // bodies.push_back(lower(lq.body, formats));
                //     }
                // }

                // if (conditions.size() != 0) {
                //     LIR::Stmt ifEqual = LIR::Stmt(LIR::IfStmt::make(conditions, bodies));
                //     cids.push_back(ifEqual);
                // } else {
                //     cids.push_back(lower(op->body, formats));
                // }
                

                // // increment
                // std::vector<LIR::Stmt> increments;
                // for (const auto &a : iterators) {
                //     increments.push_back(LIR::Stmt(LIR::IncrementIterator::make(a)));
                // }
                // cids.push_back(LIR::Stmt(LIR::SequenceStmt::make(increments)));
                // // p.body = op->body;
                // // IndexStmt body = p.body;
                // // Each while loop starts by loading (3) and merging (4) sparse idx variables.
                // // The resulting merged idx variable is the coordinate in the
                // // current index variable’s dimension.
                // LIR::Stmt whileLoop = LIR::Stmt(LIR::SequenceStmt::make(cids));
                // // op->body.accept(this);
                // // printf("iset.size() = %d\n", iset.iterators.size());
                // static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, whileLoop);
                // // whileStmt->accept(this);
                // code.push_back(LIR::Stmt(whileStmt));
                // // # compute dense pos variables
                // // Dense pos variables (e.g., pB1) are then computed by adding the merged idx variable 
                // // to the start of the variable’s corresponding tensor slice (5).
            }
            stmts.push_back(LIR::Stmt(LIR::SequenceStmt::make(code)));
            // std::vector<LIR::Stmt> code;
            // // Emit code to initialize pos variables:
            // LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
            // code.push_back(LIR::Stmt(LIR::IteratorDefinition::make(iset)));
            // lattice = MergeLattice::make(SetExpr(), stmt, formats);
            // stmt.accept(this);
        }

        void visit(const LIR::WhileStmt *op) {
            printf("LIR whileStmt\n");
            // stmts.push_back(LIR::Stmt(op));
        }

        void visit(const ArrayAssignment *aa) {
            printf("LIR ArrayAssignment\n");
            Access lhs = aa->lhs;
            Expr rhs = aa->rhs;
            LIR::ArrayLevel al = LIR::access_to_array_level(lhs, formats);
            SetExpr sexpr = convert(rhs);
            LIR::Expr lir_rhs = convert_lir(rhs, formats);
            stmts.push_back(LIR::Stmt(LIR::ArrayAssignment::make(al, lir_rhs)));
        }
    };

    BuildLowerLIRs b = BuildLowerLIRs(formats);
    b.build(stmt);
    return LIR::Stmt(LIR::SequenceStmt::make(b.stmts));

    // STUDENTS TODO:
    // assert(false);
    // const SetExpr &sexpr, const IndexStmt &body, const FormatMap &formats
    // SetExpr sexpr = SetExpr();
    // MergeLattice lattice = MergeLattice::make(sexpr, stmt, formats);
    // const IndexStmtNode * n = stmt.ptr.get();
    // if (typeid(*n) == typeid(ForAll)) {
    //     printf("ForAll\n");
    //     const ForAll * fa = (ForAll *)n;
    //     MergeLattice lattice = MergeLattice::make(fa->sexpr, fa->body, formats);
    //     // lattice.add_body(fa->body, formats);

    //     // As noted above, we provide a small C-like IR that can be used to implement co-iteration in LIR.h. We provide the printing to C interface in IRPrinter.h (this contains printing for the front-end code as well as the two IRs).

    //     // In our example, the merge lattice has a single point: the intersection of B and C. This makes code generation quite simple. Let B be a Compressed array and C be a dense array.

    //     // We would lower the example to a LIR::SequenceStmt that first defines iterators for each of the three arrays via 3 LIR::IteratorDefinitions, followed by a LIR::WhileStmt over the B and C iterators.

    //     // uint64_t A_i = 0;
    //     // uint64_t B_i_iter = B.pos[0];
    //     // uint64_t C_i = 0;
    //     // while ((B_i_iter < B.pos[1]) && (C_i < C.shape[0])) {
    //     // ...
    //     // }
    //     std::vector<LIR::Stmt> code;
    //     // Emit code to initialize pos variables:
    //     LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
    //     code.push_back(LIR::Stmt(LIR::IteratorDefinition::make(iset)));

    //     // std::vector<LIR::Stmt> whileLoop;
    //     // // follow the IRVisitor pattern

    //     // BuildBodyStmt b = BuildBodyStmt(formats, lattice);
    //     // b.build(fa->body);
    //     // Emit one loop per lattice point lp
    //     // std::vector<LIR::Stmt> loops;

    //     // for (MergePoint lp : lattice.points) {
    //     //     std::vector<MergePoint> sl = lattice.get_sub_points(lp);
    //     //     LIR::IteratorSet iset = LIR::IteratorSet{lp.iterators};
    //     //     // std::vector<LIR::Stmt> loopBody;
    //     //     IndexStmt body = lp.body;
    //     // }

    //     // // followed by a LIR::WhileStmt over the B and C iterators.
    //     // for (const auto &p : lattice.points) {
    //     //     std::vector<LIR::ArrayLevel> iterators = p.iterators;
    //     //     // iterators name and format
    //     //     // nice, only b and c
    //     //     // for (const auto &a : iterators) {
    //     //     //     printf("iterators: %s\n", a.name.c_str());
    //     //     // }
    //     //     code.push_back(LIR::Stmt(LIR::WhileStmt::make(iset, lower(p.body, formats))));
    //     //     // LIR::IteratorSet iset = gather_iterator_set(p.body, formats);
    //     //     // static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, lower(p.body, formats));
    //     //     // whileStmt->accept(nullptr);
    //     //     // code.push_back(LIR::Stmt(whileStmt));
    //     // }
    //     // code.push_back(LIR::Stmt(LIR::WhileStmt::make(iset, lower(fa->body, formats))));

    //     // The body of the LIR::WhileStmt needs to derive the logical index 
    //     // of B by extracting it from B.crd, which is done with a LIR::CompressedIndexDefinition. 
    //     // Next, the overall logical index must be computed by taking a min of the 
    //     // logical indices we are iterating over.




    //     // // return lattice.lower();
    //     // std::vector<LIR::Stmt> loops;
    //     // // Emit one loop per lattice point lp
    //     // for (const auto &lp : lattice.points) {
    //     //     std::vector<LIR::ArrayLevel> iterators = lp.iterators;
    //     //     IndexStmt body = lp.body;
    //     //     LIR::IteratorSet iset = gather_iterator_set(body, formats);
    //     //     static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, lower(body, formats));
    //     //     whileStmt->accept(nullptr);
    //     //     loops.push_back(LIR::Stmt(whileStmt));
    //     // }

    //     // // Emit code to initialize pos variables:
    //     // // B2_pos = B2_pos_arr[B1_pos];
    //     // for (const auto &p : lattice.points) {
    //     //     std::vector<LIR::ArrayLevel> iterators = p.iterators;
    //     //     for (const auto &a : iterators) {
    //     //         loops.push_back(LIR::Stmt(LIR::ArrayAssignment::make(a, LIR::Expr(LIR::ArrayAccess::make(a)))));
    //     //     }
    //     // }
    //     // printf("final points.size() = %d\n", lattice.points.size());

    //     // for (const auto &p : lattice.points) {
    //     //     std::vector<LIR::ArrayLevel> iterators = p.iterators;
            
    //     //     IndexStmt body = p.body;
    //     //     printf("before gather_iterator_set\n");
    //     //     LIR::IteratorSet iset = gather_iterator_set(body, formats);
    //     //     printf("after gather_iterator_set\n");
    //     //     static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, lower(body, formats));
    //     //     printf("after LIR::WhileStmt::make\n");
    //     //     whileStmt->accept(nullptr);
    //     //     loops.push_back(LIR::Stmt(whileStmt));
    //     // }
    //     // for (const auto &p : lattice.points) {
    //     //     std::vector<LIR::ArrayLevel> iterators = p.iterators;
            
    //     //     IndexStmt body = p.body;
    //     //     // printf("before gather_iterator_set\n");
    //     //     LIR::IteratorSet iset = gather_iterator_set(body, formats);
    //     //     static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, lower(body, formats));
    //     //     whileStmt->accept(nullptr);
    //     //     // loops.push_back(LIR::Stmt(whileStmt));
    //     // }
    //     printf("return LIR::Stmt(LIR::SequenceStmt::make(loops));\n");
    //     return LIR::Stmt(LIR::SequenceStmt::make(code));
    //     // return lattice.lower();
    //     // LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
    //     // static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, lower(fa->body, formats));
    //     // whileStmt->accept(nullptr);
    //     // return LIR::Stmt(whileStmt);
    // } else if (typeid(*n) == typeid(ArrayAssignment)) {
    //     printf("ArrayAssignment\n");
    //     const ArrayAssignment * aa = (ArrayAssignment *)n;
    //     Access lhs = aa->lhs;
    //     Expr rhs = aa->rhs;
    //     LIR::ArrayLevel al = LIR::access_to_array_level(lhs, formats);
    //     SetExpr sexpr = convert(rhs);
    //     LIR::Expr lir_rhs = convert_lir(rhs, formats);
    //     return LIR::Stmt(LIR::ArrayAssignment::make(al, lir_rhs));
    //     // Access lhs;
    //     // Expr rhs;
    //     // const ArrayAssignment * aa = (ArrayAssignment *)n;
    //     // SetExpr sexpr = convert(aa->rhs);

    //     // MergeLattice lattice = MergeLattice::make(SetExpr(), stmt, formats);

    //     // Access lhs;
    //     // Expr rhs;
    
    //     // LIR::ArrayLevel al = LIR::access_to_array_level(aa->lhs, formats);
    //     // LIR::Expr rhs = convert(aa->rhs);
    //     // return LIR::Stmt(LIR::ArrayAssignment::make(al, rhs));
    // } else {
    //     printf("Unknown\n");
    // }


// Stmt LowererImpl::lowerAssignment(Assignment assignment) {
//   TensorVar result = assignment.getLhs().getTensorVar();

//   if (generateComputeCode()) {
//     Expr varIR = getTensorVar(result);
//     Expr rhs = lower(assignment.getRhs());

//     // Assignment to scalar variables.
//     if (isScalar(result.getType())) {
//       if (!assignment.getOperator().defined()) {
//         return Assign::make(varIR, rhs);
//       }
//       else {
//         taco_iassert(isa<taco::Add>(assignment.getOperator()));
//         return Assign::make(varIR, ir::Add::make(varIR,rhs));
//       }
//     }
//     // Assignments to tensor variables (non-scalar).
//     else {
//       Expr valueArray = GetProperty::make(varIR, TensorProperty::Values);
//       return ir::Store::make(valueArray, generateValueLocExpr(assignment.getLhs()),
//                            rhs);
//       // When we're assembling while computing we need to allocate more
//       // value memory as we write to the values array.
//       if (generateAssembleCode()) {
//         // TODO
//       }
//     }
//   }
//   // We're only assembling so defer allocating value memory to the end when
//   // we'll know exactly how much we need.
//   else if (generateAssembleCode()) {
//     // TODO
//     return Stmt();
//   }
//   // We're neither assembling or computing so we emit nothing.
//   else {
//     return Stmt();
//   }
//   taco_unreachable;
//   return Stmt();
// }


    // return LIR::Stmt();
    // Lower from CIN into Lowered Stmt

    // LIR::IteratorSet iset = gather_iterator_set(stmt, formats);
    // static const std::shared_ptr<const LIR::IteratorDefinition> iter = LIR::IteratorDefinition::make(iset);
    // LIR::Stmt lstmt = LIR::Stmt(iter);
    // return lstmt;

    // static const std::shared_ptr<const LIR::WhileStmt> whileStmt = LIR::WhileStmt::make(iset, stmt);
    // for (auto const& x : formats) {
    //     printf("formats: %s\n", x.first.c_str());
    //     for (auto const& y : x.second) {
    //         printf("y: %d\n", (int)y);
    //         break;
    //     }
    // }
    // return convert(stmt, formats);
    // const IndexStmtNode * n = stmt.ptr.get();
    // if (typeid(*n) == typeid(ForAll)) {
    //     printf("ForAll\n");
    // } else if (typeid(*n) == typeid(ArrayAssignment)) {
    //     printf("ArrayAssignment\n");
    // } else {
    //     printf("Unknown\n");
    // }

    // for (auto const& x : formats) {
    //     printf("formats: %s\n", x.first.c_str());
    //     for (auto const& y : x.second) {
    //         printf("y: %d\n", (int)y);
    //         break;
    //     }
    // }
    // formats: A
    // y: 0
    // formats: B
    // y: 1
    // formats: C
    // y: 1
}
