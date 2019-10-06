//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/EvaluatedExprVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

#include "Environment.h"

class InterpreterVisitor : 
   public EvaluatedExprVisitor<InterpreterVisitor> {
public:
   int count = 0;
   explicit InterpreterVisitor(const ASTContext &context, Environment * env)
   : EvaluatedExprVisitor(context), mEnv(env) {}
   virtual ~InterpreterVisitor() {}

   virtual void VisitBinaryOperator (BinaryOperator * bop) {
      count++;
      //printf("I am in Binary Operator,count:%d\n",count);
	   VisitStmt(bop);
	   mEnv->binop(bop);
   }
   virtual void VisitDeclRefExpr(DeclRefExpr * expr) {
      count++;
      // printf("I am in DeclRefExpr,count:%d\n",count);
	   VisitStmt(expr);
	   mEnv->declref(expr);
   }
   virtual void VisitCastExpr(CastExpr * expr) {
      // printf("I am in CastExpr,count:%d\n",count);
	   VisitStmt(expr);
	   mEnv->cast(expr);
   }
   virtual void VisitCallExpr(CallExpr * call) {
      count++;
      // printf("I am in CallExpr,count:%d\n",count);
	   VisitStmt(call);
	   mEnv->call(call);
   }
   virtual void VisitDeclStmt(DeclStmt * declstmt) {
      // printf("I am in DeclStmt,count:%d\n",count);
      VisitStmt(declstmt);
	   mEnv->decl(declstmt);
   }
private:
   Environment * mEnv;
};

class InterpreterConsumer : public ASTConsumer {
public:
   explicit InterpreterConsumer(const ASTContext& context) : mEnv(),
   	   mVisitor(context, &mEnv) {
   }
   virtual ~InterpreterConsumer() {}

   virtual void HandleTranslationUnit(clang::ASTContext &Context) {
	   TranslationUnitDecl * decl = Context.getTranslationUnitDecl();
	   mEnv.init(decl);

	   FunctionDecl * entry = mEnv.getEntry();
	   mVisitor.VisitStmt(entry->getBody());
  }
private:
   Environment mEnv;
   InterpreterVisitor mVisitor;
};

class InterpreterClassAction : public ASTFrontendAction {
public: 
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    return std::unique_ptr<clang::ASTConsumer>(
        new InterpreterConsumer(Compiler.getASTContext()));
  }
};

int main (int argc, char ** argv) {
   if (argc > 1) {
       clang::tooling::runToolOnCode(new InterpreterClassAction, argv[1]);
   }
}
