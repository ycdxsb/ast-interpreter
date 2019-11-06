# ast-interpreter
## description
support a subset of C language constructs, as follows: 

Type: int | char | void | *
Operator: * | + | - | * | / | < | > | == | = | [ ] 
Statements: IfStmt | WhileStmt | ForStmt | DeclStmt 
Expr : BinaryOperator | UnaryOperator | DeclRefExpr | CallExpr | CastExpr 

We also need to support 4 external functions int GET(), void * MALLOC(int), void FREE (void *), void PRINT(int), the semantics of the 4 funcions are self-explanatory. 

A skelton implemnentation ast-interpreter.tgz is provided, and you are welcome to make any changes to the implementation. The provided implementation is able to interpreter the simple program like : 
```
extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int a;
   a = GET();
   PRINT(a);
}
```

## definition of the simple language
```
Program : DeclList
DeclList : Declaration DeclList | empty
Declaration : VarDecl FuncDecl
VarDecl : Type VarList;
Type : BaseType | QualType
BaseType : int | char | void
QualType : Type * 
VarList : ID, VarList |  | ID[num], VarList | emtpy
FuncDecl : ExtFuncDecl | FuncDefinition
ExtFuncDecl : extern int GET(); | extern void * MALLOC(int); | extern void FREE(void *); | extern void PRINT(int);
FuncDefinition : Type ID (ParamList) { StmtList }
ParamList : Param, ParamList | empty
Param : Type ID
StmtList : Stmt, StmtList | empty
Stmt : IfStmt | WhileStmt | ForStmt | DeclStmt | CompoundStmt | CallStmt | AssignStmt | 
IfStmt : if (Expr) Stmt | if (Expr) Stmt else Stmt
WhileStmt : while (Expr) Stmt
DeclStmt : Type VarList;
AssignStmt : DeclRefExpr = Expr;
CallStmt : CallExpr;
CompoundStmt : { StmtList }
ForStmt : for ( Expr; Expr; Expr) Stmt
Expr : BinaryExpr | UnaryExpr | DeclRefExpr | CallExpr | CastExpr | ArrayExpr | DerefExpr | (Expr) | num
BinaryExpr : Expr BinOP Expr
BinaryOP : + | - | * | / | < | > | ==
UnaryExpr : - Expr
DeclRefExpr : ID
CallExpr : DeclRefExpr (ExprList)
ExprList : Expr, ExprList | empty
CastExpr : (Type) Expr
ArrayExpr : DeclRefExpr [Expr]
DerefExpr : * DeclRefExpr
```

## score
![image](https://github.com/ycdxsb/ast-interpreter/blob/master/score.png)
