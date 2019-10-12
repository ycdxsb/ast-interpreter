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
   explicit InterpreterVisitor(const ASTContext &context, Environment * env)
   : EvaluatedExprVisitor(context), mEnv(env) {}
   virtual ~InterpreterVisitor() {}

   virtual void VisitBinaryOperator (BinaryOperator * bop) {
      VisitStmt(bop);
	   mEnv->binop(bop);
   }
   virtual void VisitDeclRefExpr(DeclRefExpr * expr) {
      VisitStmt(expr);
	   mEnv->declref(expr);
   }
   virtual void VisitCastExpr(CastExpr * expr) {
      VisitStmt(expr);
	   mEnv->cast(expr);
   }
   virtual void VisitCallExpr(CallExpr * call) {
      VisitStmt(call);
	   mEnv->call(call);
      if (FunctionDecl* funcdecl = call->getDirectCallee()){
         if ((!funcdecl->getName().equals("GET")) && 
             (!funcdecl->getName().equals("PRINT")) && 
             (!funcdecl->getName().equals("MALLOC")) && 
             (!funcdecl->getName().equals("FREE"))){
             Visit(funcdecl->getBody());
             mEnv->popStackFrame();
         }
      }
   }

   virtual void VisitDeclStmt(DeclStmt * declstmt) {
      VisitStmt(declstmt);
	   mEnv->declstmt(declstmt);
   }

   virtual void VisitIfStmt(IfStmt* ifstmt){
      // clang/AST/stmt.h/ line 905
      // todo add StackFrame for then and else block
      Expr* cond = ifstmt->getCond();
      if(mEnv->expr(cond)){ // True
         // cout << "I can in then part" << endl;
         Stmt* thenstmt = ifstmt->getThen();
         Visit(thenstmt); //clang/AST/EvaluatedExprVisitor.h line 100
      }else{
         if(ifstmt->getElse()){
            // cout<< "this have elsestmt"<<endl;
            Stmt* elsestmt = ifstmt->getElse();
            Visit(elsestmt);
         }
      }
   }

   virtual void VisitWhileStmt(WhileStmt* whilestmt){
      // clang/AST/stmt.h/ line 1050
      Expr* cond = whilestmt->getCond();
      while(mEnv->expr(cond)){
         Visit(whilestmt->getBody());
         //mEnv->mStack.back().
      }
   }

   virtual void VisitForStmt(ForStmt* forstmt){
      // clang/AST/stmt.h/ line 1179
      Stmt* init = forstmt->getInit();
      if(init){
         for(Visit(init);mEnv->expr(forstmt->getCond());Visit(forstmt->getInc())){
            Visit(forstmt->getBody());
         }
      }
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
