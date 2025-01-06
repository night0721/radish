#ifndef STMT_H
#define STMT_H

#include "ast.h"
#include "scanner.h"
typedef enum {
    STMT_BLOCK,
    STMT_CLASS,
    STMT_EXPR,
    STMT_FUN,
    STMT_IF,
    STMT_PRINT,
    STMT_VAR,
    STMT_WHILE,
    STMT_COUNT,
} StmtType;
typedef struct Stmt Stmt;
struct Stmt{
    StmtType type;
    union {
        struct {
            Stmt **statements;
        } block;
        struct {
            Token name;
            Token superclass;
            Stmt **methods;
        } class;
        struct {
            Expr *expression;
        } expr;
        struct {
            Token name;
            TokenArray params;
            Stmt **body;
        } function;
        struct {
            Expr *condition;
            Stmt *thenBranch;
            Stmt *elseBranch;
        } _if;
        struct {
            Expr *expression;
        } print;
        struct {
            Token keyword;
            Expr *value;
        } _return;
        struct {
            Token name;
            Expr *initializer;
        } variable;
        struct {
            Expr *condition;
            Stmt *body;
        } _while;
    } as;
};
typedef struct {
    Stmt *statements;
    int count;
    int capacity;
    bool hadError;
} StmtArray;
#endif 
