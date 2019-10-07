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
   
   bool hasDeclared(Stmt *stmt){
	   return mExprs.find(stmt)!=mExprs.end();
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
   void binop(BinaryOperator *binaryExpr) {
	   Expr * exprLeft = binaryExpr->getLHS();
	   Expr * exprRight = binaryExpr->getRHS();
	   if (binaryExpr->isAssignmentOp()) {
		   expr(exprRight);
		   int val = mStack.back().getStmtVal(exprRight);
		   mStack.back().bindStmt(exprLeft, val);
		   if (DeclRefExpr * declexpr = dyn_cast<DeclRefExpr>(exprLeft)) {
			   Decl * decl = declexpr->getFoundDecl();
			   mStack.back().bindDecl(decl, val);
		   }
	   }else{
		   auto op = binaryExpr->getOpcode();
		   int left;
		   int right;
		   switch (op){
	   	   case BO_Add: // +
	           mStack.back().pushStmtVal(binaryExpr,expr(exprLeft)+expr(exprRight));
		       break;
	   	   case BO_Sub: // -
	           mStack.back().pushStmtVal(binaryExpr,expr(exprLeft)-expr(exprRight));
	          break;
	       case BO_Mul: // *
	           mStack.back().pushStmtVal(binaryExpr,expr(exprLeft)*expr(exprRight));
	           break;
	       case BO_Div: // - , a/b,check the b can not be 0;
	           right = expr(exprRight);
			   if(right==0)
			       cout << "the binaryOp / ,can not div 0 "<< endl;
				   exit(0);
			   left = expr(exprLeft); // float?
			   if(left%right!=0)
			       cout << "there are loss when div" << endl;
			   mStack.back().pushStmtVal(binaryExpr,int(left/right)); 
	           break;
	       case BO_LT: // <
	   		   mStack.back().pushStmtVal(binaryExpr,expr(exprLeft)<expr(exprRight));
	           break;
	       case BO_GT: // >
	    	   mStack.back().pushStmtVal(binaryExpr,expr(exprLeft)>expr(exprRight));
	           break;
	       case BO_EQ: // ==
	           mStack.back().pushStmtVal(binaryExpr,expr(exprLeft)==expr(exprRight));
	           break;
	       default:
		       break;
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

   void unaryop(UnaryOperator* unaryExpr){ // - +
	   // Clang/AST/Expr.h/ line 1714
	   auto op = unaryExpr->getOpcode();
	   auto exp = unaryExpr->getSubExpr();
	   switch (op)
	   {
	   case UO_Minus: //'-'
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
		if(auto decl = dyn_cast<DeclRefExpr>(exp)){
            declref(decl);
			int result = mStack.back().getStmtVal(decl);
			return result;
		}
	   	else if(auto IntLiteral = dyn_cast<IntegerLiteral>(exp)){ //a = 12
			llvm::APInt result = IntLiteral->getValue();
			// http://www.cs.cmu.edu/~15745/llvm-doxygen/de/d4c/a04857_source.html
			return result.getSExtValue();
		}else if(auto CharLiteral = dyn_cast<CharacterLiteral>(exp)){ // a = 'a'
		    return CharLiteral->getValue(); // Clang/AST/Expr.h/ line 1369
		}else if(auto unaryExpr = dyn_cast<UnaryOperator>(exp)){ // a = -13 and a = +12;
		    unaryop(unaryExpr);
			int result = mStack.back().getStmtVal(unaryExpr);
			return result;
		}else if(auto binaryExpr = dyn_cast<BinaryOperator>(exp)){
			binop(binaryExpr);
			int result = mStack.back().getStmtVal(binaryExpr);
			return result;
		}
		cout << "have not handle this situation"<< endl;
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


