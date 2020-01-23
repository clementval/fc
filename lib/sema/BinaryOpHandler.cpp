// Copyright (c) 2019, Compiler Tree Technologies Pvt Ltd.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//--------------------------------BinaryOPHandler----------------------------//
//                                                                           //
// Handling special binary cases. For now handling following                 //
// 1, lhs = str1 // str2                                                     //
//    here we copy str1 to lhs so  that we can cal c-strcat for such cases.  //
//    So this will transformed into                                          //
//    lhs = str1                                                             //
//    lhs = lhs // str2                                                      //
//                                                                           //
// --------------------------------------------------------------------------//

#include "AST/ASTContext.h"
#include "AST/ASTPass.h"

#include "AST/ParseTreeBuilder.h"
#include "AST/StmtVisitor.h"
#include "AST/SymbolTable.h"
#include "AST/Type.h"
#include "common/Diagnostics.h"
#include "sema/ExpansionUtils.h"
#include "sema/Intrinsics.h"

using namespace fc;
using namespace ast;
namespace fc {

class BinaryOpHandler : public StmtVisitor<BinaryOpHandler, bool> {
  ParseTreeBuilder builder;
  Block *currBlock{nullptr};
  SymbolTable *currSymTable{nullptr};
  Stmt *currStmt{nullptr};
  ASTContext &C;

public:
  BinaryOpHandler(ASTContext &C) : builder(C), C(C) {}

  void setBlock(Block *b) { this->currBlock = b; }

  void setSymTable(SymbolTable *sym) { this->currSymTable = sym; }

  void setCurrStmt(Stmt *curr) { currStmt = curr; }

  // bool postBinaryExpr(BinaryExpr *) override;

  bool postAssignmentStmt(AssignmentStmt *stmt) override;

  Expr *splitExpr(Expr *result, Expr *LHS, Stmt *insertPoint);
};

Expr *BinaryOpHandler::splitExpr(Expr *result, Expr *lhs, Stmt *insertPoint) {
  auto loc = insertPoint->getSourceLoc();
  auto expr = llvm::dyn_cast<BinaryExpr>(lhs);
  if (!expr)
    return lhs;

  assert(expr->getOpKind() == BinaryOpKind::Concat);

  auto LHS = expr->getLHS();
  auto RHS = expr->getRHS();

  LHS = splitExpr(result, LHS, insertPoint);

  auto newAssignment =
      builder.buildAssignmentStmt(result->clone(), LHS->clone(), loc);
  currBlock->insertStmtBefore(newAssignment, insertPoint);

  auto newBinaryExpr = builder.buildExpr(
      result->clone(), RHS->clone(), BinaryOpKind::Concat, LHS->getType(), loc);
  return newBinaryExpr;
}

bool BinaryOpHandler::postAssignmentStmt(AssignmentStmt *stmt) {
  assert(&C);
  auto loc = stmt->getSourceLoc();
  auto result = stmt->getLHS();
  auto binaryExpr = llvm::dyn_cast<BinaryExpr>(stmt->getRHS());
  if (!binaryExpr)
    return true;

  if (binaryExpr->getOpKind() != BinaryOpKind::Concat) {
    return true;
  }

  auto LHS = binaryExpr->getLHS();
  auto RHS = binaryExpr->getRHS();

  LHS = splitExpr(result, LHS, stmt);

  auto newAssignment =
      builder.buildAssignmentStmt(result->clone(), LHS->clone(), loc);
  currBlock->insertStmtBefore(newAssignment, stmt);

  auto newBinaryExpr = builder.buildExpr(
      result->clone(), RHS->clone(), BinaryOpKind::Concat, LHS->getType(), loc);

  auto finalAssignment =
      builder.buildAssignmentStmt(result->clone(), newBinaryExpr->clone(), loc);
  currBlock->replaceWith(finalAssignment, stmt);
  return true;
}

class BinaryOpHandlerPass : public ASTBlockPass {
  BinaryOpHandler binaryOpHandler;

public:
  BinaryOpHandlerPass(ASTContext &C)
      : ASTBlockPass(C, "Format handling pass"), binaryOpHandler(C) {}

  bool runOnBlock(Block *block) override {
    binaryOpHandler.setBlock(block);
    binaryOpHandler.setSymTable(currPU->getSymbolTable());

    auto &stmtList = block->getStmtList();

    for (auto stmt : stmtList) {
      binaryOpHandler.setCurrStmt(stmt);
      if (!binaryOpHandler.visit(stmt)) {
        return false;
      }
    }
    return true;
  }
};

ASTPass *createBinaryOpHandlerPass(ASTContext &C) {
  return new BinaryOpHandlerPass(C);
}
} // namespace fc
