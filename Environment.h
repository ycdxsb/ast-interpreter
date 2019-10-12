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
   std::map<Decl*, int64_t> mVars; // change int -> int64_t, so that is can store int* 
   std::map<Stmt*, int64_t> mExprs;
   /// The current stmt
   Stmt * mPC;
public:
   StackFrame() : mVars(), mExprs(), mPC() {
   }

   void bindDecl(Decl* decl, int64_t val) {
      mVars[decl] = val;
   }    
   int64_t getDeclVal(Decl * decl) {
      assert (mVars.find(decl) != mVars.end());
      return mVars.find(decl)->second;
   }
   void bindStmt(Stmt * stmt, int64_t val) {
	   mExprs[stmt] = val;
   }
   int64_t getStmtVal(Stmt * stmt) {
	   assert (mExprs.find(stmt) != mExprs.end());
	   return mExprs[stmt];
   }
   void setPC(Stmt * stmt) {
	   mPC = stmt;
   }
   Stmt * getPC() {
	   return mPC;
   }

   void pushStmtVal(Stmt *stmt,int64_t value) {
	   mExprs.insert(pair<Stmt* , int64_t> (stmt,value));
   }
   
   bool exprExits(Stmt *stmt){
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
	   mStack.push_back(StackFrame()); // put forward, else when process global will segmentfault because no StackFrame. 
	   for (TranslationUnitDecl::decl_iterator i =unit->decls_begin(), e = unit->decls_end(); i != e; ++ i) {
		   if (FunctionDecl * fdecl = dyn_cast<FunctionDecl>(*i) ) {
			   if (fdecl->getName().equals("FREE")) mFree = fdecl;
			   else if (fdecl->getName().equals("MALLOC")) mMalloc = fdecl;
			   else if (fdecl->getName().equals("GET")) mInput = fdecl;
			   else if (fdecl->getName().equals("PRINT")) mOutput = fdecl;
			   else if (fdecl->getName().equals("main")) mEntry = fdecl;
		   }else { // global var init
		        if (VarDecl * vardecl = dyn_cast<VarDecl>(*i)) {
					if(vardecl->getType().getTypePtr()->isIntegerType() || vardecl->getType().getTypePtr()->isPointerType()){
			        	if(vardecl->hasInit())
			            	mStack.back().bindDecl(vardecl, expr(vardecl->getInit()));
				    	else
					    	mStack.back().bindDecl(vardecl, 0);
				}else{ // todo array 
					
				}  	
		        }
		   }
       }
   }

   FunctionDecl * getEntry() {
	   return mEntry;
   }

   /// !TODO Support comparison operation
   void binop(BinaryOperator *binaryExpr) {
	   Expr * exprLeft = binaryExpr->getLHS();
	   Expr * exprRight = binaryExpr->getRHS();
	   if (binaryExpr->isAssignmentOp()) {
		   if (DeclRefExpr * declexpr = dyn_cast<DeclRefExpr>(exprLeft)) {
			   int64_t val = expr(exprRight);
		       mStack.back().bindStmt(exprLeft, val);
			   Decl * decl = declexpr->getFoundDecl();
			   mStack.back().bindDecl(decl, val);
		   }else if(auto array = dyn_cast<ArraySubscriptExpr>(exprLeft)){
			   if (DeclRefExpr* declexpr = dyn_cast<DeclRefExpr>(array->getLHS()->IgnoreImpCasts())){
                    Decl* decl = declexpr->getFoundDecl();
					int64_t val = expr(exprRight);
                    int64_t index = expr(array->getRHS());
					if (VarDecl * vardecl = dyn_cast<VarDecl>(decl)){
						if(auto array = dyn_cast<ConstantArrayType>(vardecl->getType().getTypePtr())){
							if(array->getElementType().getTypePtr()->isIntegerType()) { // IntegerArray
								int64_t tmp = mStack.back().getDeclVal(vardecl);
								int* p = (int*) tmp;
								*(p+index) = val;
							}
						}
					}
			    }
		   }
	   }else{
		   auto op = binaryExpr->getOpcode();
		   int64_t left;
		   int64_t right;
		   int64_t result;
		   switch (op){
	   	   case BO_Add: // +
			   result = expr(exprLeft)+expr(exprRight);
		       break;
	   	   case BO_Sub: // -
			   result = expr(exprLeft)-expr(exprRight);
	          break;
	       case BO_Mul: // *
	           result = expr(exprLeft)*expr(exprRight);
	           break;
	       case BO_Div: // - , a/b,check the b can not be 0;
	           right = expr(exprRight);
			   if(right==0)
			       cout << "the binaryOp / ,can not div 0 "<< endl;
				   exit(0);
			   left = expr(exprLeft); // float?
			   if(left%right!=0)
			       cout << "there are loss when div" << endl;
			   result = int64_t(left/right); 
	           break;
	       case BO_LT: // <
               result = expr(exprLeft)<expr(exprRight);
	           break;
	       case BO_GT: // >
	    	   result = expr(exprLeft)>expr(exprRight);
	           break;
	       case BO_EQ: // ==
	           result = expr(exprLeft)==expr(exprRight);
	           break;
	       default:
		       cout << "process binaryOp error"<< endl;
			   exit(0);
		       break;
		   }
		   if(mStack.back().exprExits(binaryExpr)){
				mStack.back().bindStmt(binaryExpr,result);
			}else{
	            mStack.back().pushStmtVal(binaryExpr,result);
			}
	   }
   }


   void declstmt(DeclStmt * declstmt) {
	   for (DeclStmt::decl_iterator it = declstmt->decl_begin(), ie = declstmt->decl_end();
			   it != ie; ++ it) {
		   Decl * decl = *it;
		   if (VarDecl * vardecl = dyn_cast<VarDecl>(decl)) {
				if(vardecl->getType().getTypePtr()->isIntegerType() || vardecl->getType().getTypePtr()->isPointerType()){
			        if(vardecl->hasInit())
			            mStack.back().bindDecl(vardecl, expr(vardecl->getInit()));
				    else
					    mStack.back().bindDecl(vardecl, 0);
				}else{ // todo array 
					if(auto array = dyn_cast<ConstantArrayType>(vardecl->getType().getTypePtr())){ // int/char A[100];
						int64_t length = array->getSize().getSExtValue();
						if(array->getElementType().getTypePtr()->isIntegerType()){ // IntegerArray
                            int* a = new int[length];
							for(int i = 0 ;i < length; i++){
								a[i] = 0;
							}
							mStack.back().bindDecl(vardecl,(int64_t)a);
						}else if(array->getElementType().getTypePtr()->isCharType()){ // Clang/AST/Type.h line 1652
							char* a = new char[length];
							for(int i = 0 ;i < length; i++){
								a[i] = 0;
							}
							mStack.back().bindDecl(vardecl,(int64_t)a);
						}
						if(vardecl->hasInit()){
							// todo , guess Stmt **VarDecl::getInitAddress 
						}
					}
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
	       cout<< "process unaryOp error"<<endl;
		   exit(0);
		   break;
	   }
   }
    

   int64_t expr(Expr *exp){
	    exp = exp->IgnoreImpCasts();
		if(auto decl = dyn_cast<DeclRefExpr>(exp)){
            declref(decl);
			int64_t result = mStack.back().getStmtVal(decl);
			return result;
		}else if(auto intLiteral = dyn_cast<IntegerLiteral>(exp)){ //a = 12
			llvm::APInt result = intLiteral->getValue();
			// http://www.cs.cmu.edu/~15745/llvm-doxygen/de/d4c/a04857_source.html
			return result.getSExtValue();
		}else if(auto charLiteral = dyn_cast<CharacterLiteral>(exp)){ // a = 'a'
		    return charLiteral->getValue(); // Clang/AST/Expr.h/ line 1369
		}else if(auto unaryExpr = dyn_cast<UnaryOperator>(exp)){ // a = -13 and a = +12;
		    unaryop(unaryExpr);
			int64_t result = mStack.back().getStmtVal(unaryExpr);
			return result;
		}else if(auto binaryExpr = dyn_cast<BinaryOperator>(exp)){ //+ - * / < > ==
			binop(binaryExpr);
			int64_t result = mStack.back().getStmtVal(binaryExpr);
			// cout << "binary result:\t" << result<<endl;
			return result;
		}else if(auto parenExpr = dyn_cast<ParenExpr>(exp)){ // (E)
			return expr(parenExpr->getSubExpr());
		}else if(auto array = dyn_cast<ArraySubscriptExpr>(exp)){
			if (DeclRefExpr* declexpr = dyn_cast<DeclRefExpr>(array->getLHS()->IgnoreImpCasts())){
                Decl* decl = declexpr->getFoundDecl();
				int64_t index = expr(array->getRHS());
				if (VarDecl * vardecl = dyn_cast<VarDecl>(decl)){
					if(auto array = dyn_cast<ConstantArrayType>(vardecl->getType().getTypePtr())){
						if(array->getElementType().getTypePtr()->isIntegerType()) { // IntegerArray
							int64_t tmp = mStack.back().getDeclVal(vardecl);
							int* p = (int*) tmp;
							return *(p+index);
						}
					}
				}
		    }
		}
		cout << "have not handle this situation"<< endl;
		return 0;
   }

   void declref(DeclRefExpr * declref) {
	   mStack.back().setPC(declref);
	   if (declref->getType()->isIntegerType()) {
		   Decl* decl = declref->getFoundDecl();
		   int64_t val = mStack.back().getDeclVal(decl);
		   // cout<< "declref:\t"<<val<<endl;
		   mStack.back().bindStmt(declref, val);
	   }
   }

   void cast(CastExpr * castexpr) {
	   mStack.back().setPC(castexpr);
	   if (castexpr->getType()->isIntegerType()) {
		   Expr * expr = castexpr->getSubExpr();
		   int64_t val = mStack.back().getStmtVal(expr);
		   mStack.back().bindStmt(castexpr, val );
	   }
   }

   /// !TODO Support Function Call
   void call(CallExpr * callexpr) {
	   mStack.back().setPC(callexpr);
	   int64_t val = 0;
	   FunctionDecl * callee = callexpr->getDirectCallee();
	   if (callee == mInput) {
		  cout << "Please Input an Integer Value : " << endl;
		  cin >> val;
		  mStack.back().bindStmt(callexpr, val);
	   } else if (callee == mOutput) { // todo when val is char , cout char type value
		   Expr * decl = callexpr->getArg(0);
		   Expr * exp = decl->IgnoreImpCasts();
		   if(auto array = dyn_cast<ArraySubscriptExpr>(exp)){
			   cout<< expr(decl)<<endl;
		   }else{
		      val = mStack.back().getStmtVal(decl);
		      cout << val << endl; 
		   }
	   } else if(callee == mMalloc){
		   
	   } else if(callee == mFree){

	   } // other callee
   }
};


