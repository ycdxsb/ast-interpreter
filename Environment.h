//==--- tools/clang-check/ClangInterpreter.cpp - Clang Interpreter tool --------------===//
//===----------------------------------------------------------------------===//
#include <stdio.h>
#include <iostream>
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace std;
class StackFrame {
   /// StackFrame maps Variable Declaration to Value
   /// Which are either integer or addresses (also represented using an Integer value)
   std::map<Decl*, int> mVars;
   std::map<Stmt*, int> mExprs;
   /// The current stmt
   Stmt * mPC;
public:
   StackFrame() : mVars(), mExprs(), mPC() {
   }

   void bindDecl(Decl* decl, int val) {
      mVars[decl] = val;
   }    
   int getDeclVal(Decl * decl) {
      assert (mVars.find(decl) != mVars.end());
      return mVars.find(decl)->second;
   }
   void bindStmt(Stmt * stmt, int val) {
	   mExprs[stmt] = val;
   }
   int getStmtVal(Stmt * stmt) {
	   assert (mExprs.find(stmt) != mExprs.end());
	   return mExprs[stmt];
   }
   void setPC(Stmt * stmt) {
	   mPC = stmt;
   }
   Stmt * getPC() {
	   return mPC;
   }

   void pushStmtVal(Stmt *stmt,int value) {
	   mExprs.insert(pair<Stmt* , int> (stmt,value));
   }
   
};

/// Heap maps address to a value
/*
class Heap {
};
*/

class Environment {
   std::vector<StackFrame> mStack;

   FunctionDecl * mFree;				/// Declartions to the built-in functions
   FunctionDecl * mMalloc;
   FunctionDecl * mInput;
   FunctionDecl * mOutput;

   FunctionDecl * mEntry;
public:
   /// Get the declartions to the built-in functions
   Environment() : mStack(), mFree(NULL), mMalloc(NULL), mInput(NULL), mOutput(NULL), mEntry(NULL) {
   }


   /// Initialize the Environment
   void init(TranslationUnitDecl * unit) {
	   for (TranslationUnitDecl::decl_iterator i =unit->decls_begin(), e = unit->decls_end(); i != e; ++ i) {
		   if (FunctionDecl * fdecl = dyn_cast<FunctionDecl>(*i) ) {
			   if (fdecl->getName().equals("FREE")) mFree = fdecl;
			   else if (fdecl->getName().equals("MALLOC")) mMalloc = fdecl;
			   else if (fdecl->getName().equals("GET")) mInput = fdecl;
			   else if (fdecl->getName().equals("PRINT")) mOutput = fdecl;
			   else if (fdecl->getName().equals("main")) mEntry = fdecl;
		   }
	   }
	   mStack.push_back(StackFrame());
   }

   FunctionDecl * getEntry() {
	   return mEntry;
   }

   /// !TODO Support comparison operation
   void binop(BinaryOperator *bop) {
	   Expr * left = bop->getLHS();
	   Expr * right = bop->getRHS();

	   if (bop->isAssignmentOp()) {
		   int val = mStack.back().getStmtVal(right);
		   mStack.back().bindStmt(left, val);
		   if (DeclRefExpr * declexpr = dyn_cast<DeclRefExpr>(left)) {
			   Decl * decl = declexpr->getFoundDecl();
			   mStack.back().bindDecl(decl, val);
		   }
	   }
   }

   void decl(DeclStmt * declstmt) {
	   for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
			   it != ie; ++ it) {
		   Decl * decl = *it;
		   if (VarDecl * vardecl = dyn_cast<VarDecl>(decl)) {
			    // not array
				
				if(vardecl->getType().getTypePtr()->isIntegerType() || vardecl->getType().getTypePtr()->isPointerType()){
			        if(vardecl->hasInit())
			            mStack.back().bindDecl(vardecl, expr(vardecl->getInit()));//expr(vardecl->getInit()))
				    else
					    mStack.back().bindDecl(vardecl, 0);
				}else{ // todo array 
					
				}  
				
		   }
	   }
   }

   void unary(UnaryOperator* unaryExpr){
	   // Clang/AST/Expr.h/ line 1714
	   auto op = unaryExpr->getOpcode();
	   auto exp = unaryExpr->getSubExpr();
	   switch (op)
	   {
	   case UO_Minus: //'-'
	        cout<<"I am in minus"<<endl;
            mStack.back().pushStmtVal(unaryExpr,-1 * expr(exp));
		   	break;
	   case UO_Plus: //'+'
			mStack.back().pushStmtVal(unaryExpr,expr(exp));
	        break;
	   default:
		   break;
	   }
   }
    

   int expr(Expr *exp){
	    exp = exp->IgnoreImpCasts();
	   	if(auto IntLiteral = dyn_cast<IntegerLiteral>(exp)){ //a = 12
			llvm::APInt result = IntLiteral->getValue();
			cout<<"result:"<<result.getSExtValue()<<endl;
			// http://www.cs.cmu.edu/~15745/llvm-doxygen/de/d4c/a04857_source.html
			return result.getSExtValue();
		}else if(auto CharLiteral = dyn_cast<CharacterLiteral>(exp)){ // a = 'a'
		    return CharLiteral->getValue(); // Clang/AST/Expr.h/ line 1369
		}else if(auto unaryExpr = dyn_cast<UnaryOperator>(exp)){ // a = -13 and a = +12;
		    unary(unaryExpr);
			cout << "I am in unaryExpr"<<endl;
			int result = mStack.back().getStmtVal(unaryExpr);
			return result;
		}
		return 0;
   }

   void declref(DeclRefExpr * declref) {
	   mStack.back().setPC(declref);
	   if (declref->getType()->isIntegerType()) {
		   Decl* decl = declref->getFoundDecl();

		   int val = mStack.back().getDeclVal(decl);
		   mStack.back().bindStmt(declref, val);
	   }
   }

   void cast(CastExpr * castexpr) {
	   mStack.back().setPC(castexpr);
	   if (castexpr->getType()->isIntegerType()) {
		   Expr * expr = castexpr->getSubExpr();
		   int val = mStack.back().getStmtVal(expr);
		   mStack.back().bindStmt(castexpr, val );
	   }
   }

   /// !TODO Support Function Call
   void call(CallExpr * callexpr) {
	   mStack.back().setPC(callexpr);
	   int val = 0;
	   FunctionDecl * callee = callexpr->getDirectCallee();
	   if (callee == mInput) {
		  llvm::errs() << "Please Input an Integer Value : ";
		  cout << endl;
		  scanf("%d", &val);

		  mStack.back().bindStmt(callexpr, val);
	   } else if (callee == mOutput) {
		   Expr * decl = callexpr->getArg(0);
		   val = mStack.back().getStmtVal(decl);
		   llvm::errs() << val ;
		   cout << endl;
	   } else if(callee == mMalloc){
		   
	   } else if(callee == mFree){

	   } // other callee
   }
};


