#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(i, j) (((i) < (j)) ? (i) : (j))

//
// tokenize.c
//

// Token
typedef enum {
  TK_RESERVED, // Keywords or punctuators
  TK_IDENT,    // Identifiers
  TK_NUM,      // Integer literals
  TK_EOF,      // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // Token kind
  Token *next;    // Next token
  int val;        // If kind is TK_NUM, its value
  char *str;      // Token string
  int len;        // Token length
};

void error(char *fmt, ...);
void error_at(char *loc, char *fmt, ...);
bool consume(char *op);
char *strndup(char *p, int len);
Token *consume_ident();
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize();

extern char *user_input; // Input program
extern Token *token; // Current token

//
// parse.c
//

// Local variable
typedef struct Var Var;
struct Var {
  Var *next;
  char *name; // Variable name
  int offset; // Offset from RBP
};

// 抽象構文木のノードの種類
typedef enum {
  ND_ADD,       // +
  ND_SUB,       // -
  ND_MUL,       // *
  ND_DIV,       // /
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_ASSIGN,    // =
  ND_RETURN,    // "return"
  ND_IF,        // "if"
  ND_WHILE,     // "while"
  ND_FOR,       // "for"
  ND_BLOCK,     // { ... }
  ND_EXPR_STMT, // Expression statement
  ND_VAR,       // Variable
  ND_FUNC,      // Function call
  ND_NUM,       // Integer
} NodeKind;

// AST node type (抽象構文木のノードの型)
typedef struct Node Node;
// Function call
typedef struct Func Func;

struct Node {
  NodeKind kind; // Node kind
  Node *next;    // Next node

  Node *lhs;     // Left-hand side
  Node *rhs;     // Right-hand side

  // "if", "while" or "for" statement
  Node *cond;
  Node *then;
  Node *els;     // Used if kind == ND_IF
  Node *init;    // Used if kind == ND_FOR
  Node *inc;     // Used if kind == ND_FOR

  // Block
  Node *body;

  Var *var;      // Used if kind == ND_VAR
  int val;       // Used if kind == ND_NUM

  Func *func;    // Used if kind == ND_FUNC
};

struct Func {
  char *name;
  Node **args;
  int argsLen;
};

typedef struct {
  Node *node;
  Var *locals;
  int stack_size;
} Program;

Program *program();

//
// codegen.c
//

void codegen(Program *prog);
